// SDVersionReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <afxwin.h>
#include <cstringt.h>
#include <time.h>
#include <fcntl.h>  // _O_NOINHERIT, _O_TEXT
#include <io.h>	  // pipe()

#include "../Viewer/Resource.h"
#include "../SDVersionReader/SDVersionReader.h"
#include "../Viewer/SimpleAgeViewerDocListener.h"

static bool FNewVersion(int iLine, int nVer, FILEREP& filerep);
static bool FDeleteVersionRange(int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 FILEREP& filerep);
static bool FCreateOrRestoreVersionRange(int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 int nVer,
	 FILEREP& filerep);
static bool FAppendVersion(int iLine, int nVer, FILEREP& filerep);
static bool FRemoveLastVersion(int iLine, FILEREP& filerep);


void FILEREP::Reset() {
	for (int i = 0; i < m_cLine; ++i) {
		if (m_rgpszFileBuffer[i] != NULL) {
			delete[] m_rgpszFileBuffer[i];
			m_rgpszFileBuffer[i] = NULL;
		}

		if (m_rgatomVer[i] != 0) {
			ATOM atomError = ::DeleteAtom(m_rgatomVer[i]);
			assert(atomError == 0);
			m_rgatomVer[i] = 0;
		}
	}

	m_cLine = 0;
	m_nCL = 0;
	m_nVer = 0;
	delete m_pdrListRoot;
	m_pdrListRoot = NULL;
	m_pdrParent = NULL;

	m_ptOrg.x = m_ptOrg.y = 0;
}

const TCHAR* FILEREP::GetLine(int iLine) const {
	if (iLine < 0 || iLine >= m_cLine) {
		assert(false);
		return NULL;
	}

	assert(m_rgpszFileBuffer[iLine] != NULL && m_rgpszFileBuffer[iLine][0] < 0xf000);
	return m_rgpszFileBuffer[iLine];
}

bool FILEREP::FAddLines(int iLine, int cLine) {
	if (iLine < 0 || iLine > m_cLine) {
		assert(false);
		return false;
	}
	if (m_cLine + cLine >= _countof(m_rgpszFileBuffer)) {
		assert(false);
		CString msg;
		msg.Format(_T("File too big (> %d lines)"), MAX_LINES);
		AfxMessageBox(msg);
		return false;	// over MAX_LINES
	}

	if (iLine < m_cLine) {
		memmove(&m_rgpszFileBuffer[iLine + cLine], &m_rgpszFileBuffer[iLine],
			 sizeof(m_rgpszFileBuffer[0]) * (m_cLine - iLine));
	}

	m_cLine += cLine;
	assert(0 <= m_cLine && m_cLine <= _countof(m_rgpszFileBuffer));

	for (; cLine-- > 0; ++iLine)
		m_rgpszFileBuffer[iLine] = new TCHAR[MAX_LINE];

	return true;
}

void FILEREP::RemoveLines(int iLine, int cLine) {
	if (iLine < 0 || iLine + cLine > m_cLine) {
		assert(false);
		return;
	}

	int cLineT = cLine;
	for (int i = iLine; cLineT-- > 0; ++i)
		delete m_rgpszFileBuffer[i];

	memmove(&m_rgpszFileBuffer[iLine], &m_rgpszFileBuffer[iLine + cLine],
		 sizeof(m_rgpszFileBuffer[0]) * (m_cLine - (iLine + cLine)));

	m_cLine -= cLine;
	assert(m_cLine >= 0);
}

void FILEREP::SetLine(int iLine, const TCHAR szLine[]) {
	if (iLine < 0 || iLine > m_cLine) {
		assert(false);
		return;
	}

	assert(m_rgpszFileBuffer[iLine] != NULL);
	if (m_rgpszFileBuffer[iLine] != NULL)
		_tcsncpy_s(m_rgpszFileBuffer[iLine], MAX_LINE, szLine, _TRUNCATE);
}


class DIFFBLOCK {
	public:
	DIFFBLOCK() { memset(this, 0, sizeof(*this)); }

	~DIFFBLOCK() {
		delete[] szOld;
		delete[] szNew;
		delete m_pNext;
		if (rgatomDeleted != NULL) {
			for (int i = 0; i < cAtomDeleted; ++i) {
				assert(rgatomDeleted[i] != 0);
				ATOM atomError = ::DeleteAtom(rgatomDeleted[i]);
				assert(atomError == 0);
			}
			delete[] rgatomDeleted;
		}
	}

	void AddNext(DIFFBLOCK* pNext) {
		assert(m_pNext == NULL);
		m_pNext = pNext;
	}
	DIFFBLOCK* PdbNext() const { return m_pNext; }

	public:
	TCHAR chCommand;

	int iLineOld;
	int cLineOld;

	TCHAR* szOld;
	int cchOld;
	int cchzMaxOld;

	TCHAR* szNew;
	int cchNew;
	int cchzMaxNew;

	int iLineNew;
	int cLineNew;

	mutable ATOM* rgatomDeleted;
	mutable int cAtomDeleted;

	private:
	DIFFBLOCK* m_pNext;
};


DIFFRECORD::~DIFFRECORD() {
	if (m_pfrChild != NULL) {
		assert(m_pfrChild->GetPdrParent() == this);
		m_pfrChild->SetPdrParent(NULL);
		delete m_pfrChild;
	}

	delete m_pPrev;

	delete m_pdb;

	delete[] m_rgatomVerSav;

	delete[] m_szChangeComment;
}


static bool FProccessDiffCommandLine(const TCHAR* wzLine, DIFFBLOCK& db) {
	bool fRet = true;
	const TCHAR* pchNumBegin = NULL;
	int* piCur = &db.iLineOld;
	assert(db.chCommand == '\0');

	// start [end] c start [end]
	for (const TCHAR* pch = wzLine; *pch != '\0'; ++pch) {
		switch (*pch) {
		case 'a':
		case 'c':
		case 'd':
			// commit line
			assert(piCur != NULL);
			if (pchNumBegin == NULL) {
				fRet = false;
				assert(false);
				goto LDone;
			}
			if (piCur == &db.iLineOld) {
				db.iLineOld = _ttoi(pchNumBegin) - 1;
				db.cLineOld = 1;
			}
			else
				*piCur = _ttoi(pchNumBegin) - db.iLineOld;
			pchNumBegin = NULL;

			db.chCommand = *pch;
			piCur = &db.iLineNew;
			break;

		case ',':
			assert(piCur != NULL);
			if (pchNumBegin == NULL) {
				fRet = false;
				assert(false);
				goto LDone;
			}
			*piCur = _ttoi(pchNumBegin) - 1;
			if (piCur == &db.iLineOld)
				piCur = &db.cLineOld;
			else
				piCur = &db.cLineNew;

			pchNumBegin = NULL;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (pchNumBegin == NULL)
				pchNumBegin = pch;
			break;
		}
	}

	if (pchNumBegin != NULL) {
		assert(piCur != NULL);
		if (piCur == &db.iLineNew) {
			db.iLineNew = _ttoi(pchNumBegin) - 1;
			db.cLineNew = 1;
		}
		else
			*piCur = _ttoi(pchNumBegin) - db.iLineNew;
	}

LDone:
	return fRet;
}

static bool FAddString(TCHAR** psz, int* pcch, int* pcchzMax, const TCHAR szLine[], int cch) {
	bool fRet = true;

	if (*psz == NULL) {
		assert(*pcch == 0);
		assert(*pcchzMax == 0);
		int cchz = cch + 1;
		int cchzMax = (cchz + 7 / 8 * 8) * 2;	// align to QWORD and double
		*psz = new TCHAR[cchzMax];
		if (*psz == NULL) {
			fRet = false;
			goto LDone;
		}
		_tcsncpy_s(*psz, cchz, szLine, _TRUNCATE);
		*pcchzMax = cchzMax;

		*pcch = cch;
	}
	else {
		assert(*pcch >= 0);
		assert(*pcchzMax > 0);
		int cchCur = *pcch;
		int cchz = cch + 1;
		int cchzNew = cchCur + cchz;
		if (*pcchzMax < cchzNew) {
			int cchzMax = max(*pcchzMax * 2, cchzNew);

			TCHAR* szT = new TCHAR[cchzMax];
			if (szT == NULL) {
				fRet = false;
				goto LDone;
			}

			memcpy(szT, *psz, cchCur * sizeof(*szT));
			_tcsncpy_s(szT + cchCur, cchz, szLine, _TRUNCATE);

			delete *psz;
			*psz = szT;

			*pcchzMax = cchzMax;
		}
		else {
			_tcsncpy_s(*psz + cchCur, cchz, szLine, _TRUNCATE);
		}

		assert(cchzNew > 0);
		*pcch = cchzNew - 1;
	}


LDone:
	return fRet;
}

static bool FProccessDiffHeader(const TCHAR* szLine, DIFFRECORD& dr) {
	bool fRet = true;
	int nConvert = _stscanf_s(szLine, _T("... #%d change %d %s on %d/%d/%d %d:%d:%d by %s (%[^)]"),
		 &dr.nVer, &dr.nCL, dr.szAction, _countof(dr.szAction), &dr.tm.tm_year, &dr.tm.tm_mon,
		 &dr.tm.tm_mday, &dr.tm.tm_hour, &dr.tm.tm_min, &dr.tm.tm_sec, dr.szAuthor,
		 _countof(dr.szAuthor), dr.szType, _countof(dr.szType));

	if (nConvert != 11) {
		assert(false);
		fRet = false;
		goto LDone;
	}

	assert(dr.tm.tm_mon > 0);
	--dr.tm.tm_mon;  // ack: month is zero-based (stupid!)

	assert(dr.tm.tm_year >= 1900);
	dr.tm.tm_year -= 1900;

	// round trip thru _mkgmtime/_gmtime_s to get the day filled-in for when we output this via
	// asctime
	time_t timeT = _mkgmtime(&dr.tm);
	gmtime_s(&dr.tm, &timeT);

LDone:
	return fRet;
}

static bool FProccessDiffHeaderBranch(const TCHAR* szLine, DIFFRECORD& dr) {
	bool fRet = true;
	TCHAR szAction[16];
	szAction[0] = '\0';
	int nConvert =
		 _stscanf_s(szLine, _T("... ... ignored %s"), dr.szBranchFrom, _countof(dr.szBranchFrom));

	if (nConvert == 1) {
		_tcsncpy_s(dr.szAction, _countof(dr.szAction), _T("ignored"), _TRUNCATE);
	}
	else {
		nConvert = _stscanf_s(szLine, _T("... ... %s from %s"), szAction, _countof(szAction),
			 dr.szBranchFrom, _countof(dr.szBranchFrom));
		if (nConvert != 2)
			goto LDone;

		_tcsncpy_s(dr.szAction, _countof(dr.szAction), szAction, _TRUNCATE);
	}

	// handle version numbers at the end of paths like
	//   "//depot/dev14override/uex/mso/uxAAPreview.cpp#35,#37"
	// look backwards for '#' (since '#' is a valid filename char)
	TCHAR* pch = dr.szBranchFrom + _tcslen(dr.szBranchFrom) - 1;
	while (pch > dr.szBranchFrom) {
		switch (*pch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':

		case ',':
			--pch;
			break;

		case '#':
			if (pch[-1] == _T(',')) {
				dr.nVerBranchEnd = _ttoi(pch + 1);
				--pch;
				--pch;
			}
			else {
				dr.nVerBranchStart = _ttoi(pch + 1);
				*pch = _T('\0');	// strip off version ending
				goto LDone;			// exit
			}
			break;

		default:
			goto LDone;	 // exit
		}
	}

LDone:
	return fRet;
}

bool DIFFRECORD::FAppendChangeComment(const TCHAR szLine[], int cchLine) {
	int cchCur = m_szChangeComment != NULL ? _tcslen(m_szChangeComment) : 0;
	int cchzNew = cchLine + cchCur + 1;
	TCHAR* szNew = new TCHAR[cchzNew];
	if (szNew == NULL)
		return false;
	if (m_szChangeComment != NULL)
		_tcscpy_s(szNew, cchzNew, m_szChangeComment);
	_tcscpy_s(szNew + cchCur, cchzNew - cchCur, szLine);
	delete[] m_szChangeComment;
	m_szChangeComment = szNew;

	return true;
}

// reads in a diff file fh and outputs a linked list of DIFFRECORDS
static DIFFRECORD* _PdrProccessDiffLines(FILE* fh,
	 const DIFFRECORD** ppdrMax,
	 FILEREP& frRoot,
	 CWnd* pwndStatus) {
	bool fRet = true;
	TCHAR szLine[2048];

	if (ppdrMax != NULL)
		*ppdrMax = NULL;

	DIFFRECORD* pdrCur = NULL;
	DIFFBLOCK* pdbCur = NULL;
	const DIFFRECORD* pdrMax = NULL;
	while (fRet && _fgetts(szLine, _countof(szLine), fh)) {
		int cch = _tcslen(szLine);
		if (cch > 0 && szLine[cch - 1] != '\n') {
			TCHAR szLineT[1024];
			int cchT = 0;
			do {	// long line; strip off remainder
				if (!_fgetts(szLineT, _countof(szLineT), fh))
					break;
				cchT = _tcslen(szLineT);
			} while (cchT > 0 && szLineT[cchT - 1] != '\n' && _fgetts(szLineT, _countof(szLineT), fh));

			if (cchT > 0 && szLineT[cchT - 1] == '\n')  // only add a final CR if the line does have a
																	  // CR more to the line (e.g. non-EOF case)
				szLine[cch - 1] = '\n';
		}

		switch (szLine[0]) {
		case '.':
			if (!_tcsncmp(szLine + 1, _T(".. #"), 4)) {
				DIFFRECORD* pdrCurT = new DIFFRECORD(pdrCur, frRoot);	 // adds to list of pdrs
				if (pdrCurT == NULL)
					goto LDone;
				pdrCur = pdrCurT;
				fRet = FProccessDiffHeader(szLine, *pdrCur /*in/out*/);
				//::MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_INPUT);  // prevent window ghosting
				if (pwndStatus != NULL) {
					CString strLine;
					strLine.Format(IDS_LOADING_SCANNING_ARG, pdrCur->nVer);
					pwndStatus->SetWindowText(strLine);
				}
				if (pdrMax == NULL)
					pdrMax = pdrCur;

				pdbCur = NULL;
			}
			else if (!_tcsncmp(szLine + 1, _T(".. ... "), 7)) {
				fRet = FProccessDiffHeaderBranch(szLine, *pdrCur /*in/out*/);
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			DIFFBLOCK* pdbCurT = new DIFFBLOCK();
			if (pdbCurT == NULL)
				goto LDone;
			if (pdbCur != NULL)
				pdbCur->AddNext(pdbCurT);
			else
				pdrCur->AddDiffBlockChain(pdbCurT);
			pdbCur = pdbCurT;
			fRet = FProccessDiffCommandLine(szLine, *pdbCur /*id/out*/);
		} break;
		case '<':
			if (pdbCur != nullptr)
				fRet = FAddString(&pdbCur->szOld, &pdbCur->cchOld, &pdbCur->cchzMaxOld, szLine + 2,
					 cch - 2);	// +2 --> skip "< "
			break;
		case '>':
			if (pdbCur != nullptr)
				fRet = FAddString(&pdbCur->szNew, &pdbCur->cchNew, &pdbCur->cchzMaxNew, szLine + 2,
					 cch - 2);	// +2 --> skip "> "
			break;
		case '\t':
			fRet = pdrCur->FAppendChangeComment(szLine + 1, cch - 1);
			break;

		case '/':
			if (szLine[1] == '/')
				frRoot.SetDepotFilePath(szLine);
			break;
		}
	}

	if (ppdrMax != NULL)
		*ppdrMax = pdrMax;

LDone:
	if (pdrCur != NULL && pwndStatus != NULL) {
		CString strLine;
		strLine.Format(IDS_LOADING_SCANNING_DONE, pdrCur->nVer);
		pwndStatus->SetWindowText(strLine);
	}

	return pdrCur;
}


static bool _FReadFile(FILE* fh, FILEREP& filerep, CWnd* pwndStatus) {
	bool fRet = true;

	TCHAR szLine[MAX_LINE];
	DWORD dwLastTick = ::GetTickCount();
	bool fTruncatedLine = false;
	while (_fgetts(szLine, _countof(szLine), fh)) {
		if (fTruncatedLine) {  // ensure we throw away all line ending remainders (else the line count
									  // will get messed up)
			fTruncatedLine = _tcschr(szLine, '\n') == NULL;
			continue;
		}

		if (!filerep.FAddLines(filerep.CLine(), 1))
			return false;	// file too big?
		filerep.SetLine(filerep.CLine() - 1, szLine);
		assert(!fTruncatedLine);
		if (_tcschr(szLine, '\n') == NULL)
			fTruncatedLine = true;

		filerep.m_rgatomVer[filerep.CLine() - 1] = ::AddAtom(_T("1"));

		if (pwndStatus != NULL && (::GetTickCount() - dwLastTick) >=
												200)	// update every .2 secs (as fast as the eye can see)
		{
			CString strStatus;
			strStatus.Format(IDS_LOADING_BASELINE_ARG, filerep.CLine());
			pwndStatus->SetWindowText(strStatus);

			dwLastTick = ::GetTickCount();
		}
	}
#if _DEBUG
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		assert(filerep.m_rgatomVer[iLineT] != 0);
		TCHAR szAtom[256];
		if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
			 ::GetLastError() != 0) {
			assert(false);
		}
		assert(szAtom[0]);
	}
#endif  // _DEBUG

	if (fRet && pwndStatus != NULL) {
		CString strStatus;
		strStatus.LoadString(IDS_LOADING_BASELINE_DONE);
		pwndStatus->SetWindowText(strStatus);
	}

	// LDone:
	return fRet;
}

static bool FAddBlockAtLine(const TCHAR szNew[],
	 int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead) {
	bool fRet = true;
	assert(nVer >= 1);
	assert(nVer == filerep.NVer() - 1 || nVer == filerep.NVer() + 1);
	assert(iLine >= 0);
	assert(iLine <= filerep.CLine());
	if (iLine > filerep.CLine())
		return false;

	assert(cLine > 0);

	memmove(&filerep.m_rgatomVer[iLine + cLine], &filerep.m_rgatomVer[iLine],
		 sizeof(filerep.m_rgatomVer[0]) * (filerep.CLine() - iLine));
#if _DEBUG
	memset(&filerep.m_rgatomVer[iLine], 0, sizeof(filerep.m_rgatomVer[0]) * cLine);
#endif  // _DEBUG

#if _DEBUG_ATOM
	int cLineOld = filerep.CLine();	// REVIEW: necessary?
#endif
	if (!filerep.FAddLines(iLine, cLine))
		return false;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < cLineOld; ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	assert(nVer == filerep.NVer() - 1 || nVer == filerep.NVer() + 1);
	for (int i = 0; i < cLine; ++i) {
		TCHAR* szNewT = const_cast<TCHAR*>(_tcschr(szNew, '\n'));
		TCHAR chSav;
		if (szNewT != NULL) {
			chSav = szNewT[1];
			szNewT[1] = '\0';
		}
		filerep.SetLine(iLine + i, szNew);
		if (szNewT != NULL) {
			szNewT[1] = chSav;
			szNew = szNewT + 1;
		}
	}

	fRet = FCreateOrRestoreVersionRange(iLine, cLine, prgatomDeleted, pcAtomDeleted, nVer, filerep);
	assert(fRet);

	assert(filerep.CLine() >= 0);
	assert(filerep.CLine() <= MAX_LINES);

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		assert(filerep.m_rgatomVer[iLineT] != 0);
		TCHAR szAtom[256];
		if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
			 ::GetLastError() != 0) {
			assert(false);
		}
		assert(szAtom[0]);
	}
#endif  // _DEBUG_ATOM

	CSimpleAgeViewerDocListener* pListener = pListenerHead;
	while (pListener != NULL) {
		pListener->DocEditNotification(iLine, cLine);
		pListener = pListener->m_pNext;
	}

	// LDone:
	return fRet;
}

static bool FDeleteBlockAtLine(int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead) {
	bool fRet = true;
	assert(iLine >= 0);
	assert(iLine < filerep.CLine());
	if (iLine >= filerep.CLine())
		return false;

	assert(cLine > 0);

	assert(filerep.CLine() <= MAX_LINES);
	int cLineStart = filerep.CLine();
	assert(filerep.CLine() >= 0);

	filerep.RemoveLines(iLine, cLine);

	fRet = FDeleteVersionRange(iLine, cLine, prgatomDeleted, pcAtomDeleted, filerep);
	assert(fRet);
	memmove(&filerep.m_rgatomVer[iLine], &filerep.m_rgatomVer[iLine + cLine],
		 sizeof(filerep.m_rgatomVer[0]) * (cLineStart - (iLine + cLine)));

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	CSimpleAgeViewerDocListener* pListener = pListenerHead;
	while (pListener != NULL) {
		pListener->DocEditNotification(iLine, -cLine);
		pListener = pListener->m_pNext;
	}

	// LDone:
	return fRet;
}

static bool FChangeBlockAtLine(const TCHAR szNew[],
	 int iLine,
	 int cLineOld,
	 int cLineNew,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead) {
	bool fRet = true;
	assert(iLine >= 0);
	assert(iLine < filerep.CLine());
	if (iLine >= filerep.CLine())
		return false;

	int cLineChange = min(cLineOld, cLineNew);
	assert(cLineChange > 0);
	int cLineDelta = cLineNew - cLineOld;

	// assert(filerep.NVer() <= nVer);  // nVer will be equal when subtracting diff versions
	bool fToNewerVersion = filerep.NVer() < nVer;
	if (fToNewerVersion) {
		assert(nVer == filerep.NVer() - 1 || nVer == filerep.NVer() + 1);
		for (int i = 0; i < cLineChange; ++i) {
			FAppendVersion(iLine + i, nVer, filerep);
		}
	}
	else {
		assert(nVer == filerep.NVer() - 1);
		for (int i = 0; i < cLineChange; ++i) {
			FRemoveLastVersion(iLine + i, filerep);
		}
	}

	if (cLineDelta < 0) {
		int cLinePrev = filerep.CLine();
		filerep.RemoveLines(iLine + cLineChange, -cLineDelta);
		fRet = FDeleteVersionRange(
			 iLine + cLineChange, -cLineDelta, prgatomDeleted, pcAtomDeleted, filerep);
		assert(fRet);

		assert(nVer == filerep.NVer() - 1 || nVer == filerep.NVer() + 1);
		memmove(&filerep.m_rgatomVer[iLine + cLineChange],
			 &filerep.m_rgatomVer[iLine + cLineChange - cLineDelta],
			 sizeof(filerep.m_rgatomVer[0]) * (cLinePrev - (iLine + cLineChange - cLineDelta)));

#if _DEBUG_ATOM
		for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
			if (filerep.m_rgatomVer[iLineT] != 0) {
				TCHAR szAtom[256];
				if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
					 ::GetLastError() != 0) {
					assert(false);
				}
				assert(szAtom[0]);
			}
		}
#endif  // _DEBUG_ATOM
	}
	else if (cLineDelta > 0) {
		int cLinePrev = filerep.CLine();
		if (!filerep.FAddLines(iLine + cLineChange, cLineDelta))
			return false;

		memmove(&filerep.m_rgatomVer[iLine + cLineChange + cLineDelta],
			 &filerep.m_rgatomVer[iLine + cLineChange],
			 sizeof(filerep.m_rgatomVer[0]) * (cLinePrev - (cLineChange + iLine)));
#if _DEBUG
		memset(&filerep.m_rgatomVer[iLine + cLineChange], 0,
			 sizeof(filerep.m_rgatomVer[0]) * cLineDelta);
#endif  // _DEBUG

		fRet = FCreateOrRestoreVersionRange(
			 iLine + cLineChange, cLineDelta, prgatomDeleted, pcAtomDeleted, nVer, filerep);
		assert(fRet);
		assert(nVer == filerep.NVer() - 1 || nVer == filerep.NVer() + 1);

#if _DEBUG_ATOM
		for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
			if (filerep.m_rgatomVer[iLineT] != 0) {
				TCHAR szAtom[256];
				if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
					 ::GetLastError() != 0) {
					assert(false);
				}
				assert(szAtom[0]);
			}
		}
#endif  // _DEBUG_ATOM
	}

	for (int i = 0; i < cLineNew; ++i) {
		TCHAR* szNewT = const_cast<TCHAR*>(_tcschr(szNew, '\n'));
		TCHAR chSav;
		if (szNewT != NULL) {
			chSav = szNewT[1];
			szNewT[1] = '\0';
		}
		filerep.SetLine(iLine + i, szNew);
		if (szNewT != NULL) {
			szNewT[1] = chSav;
			szNew = szNewT + 1;
		}
	}

	assert(filerep.CLine() >= 0);
	assert(filerep.CLine() <= MAX_LINES);

	if (cLineDelta != 0) {
		CSimpleAgeViewerDocListener* pListener = pListenerHead;
		while (pListener != NULL) {
			pListener->DocEditNotification(iLine, cLineDelta);
			pListener = pListener->m_pNext;
		}
	}

	// LDone:
	return fRet;
}


static bool FApplyDiffBlockList(const DIFFBLOCK* pdbList,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead) {
	bool fRet = true;

	const DIFFBLOCK* pdbCur = pdbList;
	while (fRet && pdbCur != NULL) {
		switch (pdbCur->chCommand) {
		case 'a':
			fRet = FAddBlockAtLine(pdbCur->szNew, pdbCur->iLineNew, pdbCur->cLineNew,
				 &pdbCur->rgatomDeleted, &pdbCur->cAtomDeleted, nVer, filerep, pListenerHead);
			break;
		case 'd':
			fRet = FDeleteBlockAtLine(pdbCur->iLineNew + 1, pdbCur->cLineOld, &pdbCur->rgatomDeleted,
				 &pdbCur->cAtomDeleted, nVer, filerep,
				 pListenerHead);	// REVIEW: this "+ 1" is emperically required but is very odd
			break;
		case 'c':
			fRet =
				 FChangeBlockAtLine(pdbCur->szNew, pdbCur->iLineNew, pdbCur->cLineOld, pdbCur->cLineNew,
					  &pdbCur->rgatomDeleted, &pdbCur->cAtomDeleted, nVer, filerep, pListenerHead);
			break;

		default:
			assert(false);
			break;
		}

		pdbCur = pdbCur->PdbNext();
	}

	// filerep.SetVer(nVer);

	// LDone:
	return fRet;
}

static bool FRemoveDiffBlockList(DIFFBLOCK* pdbList,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead) {
	bool fRet = true;

	DIFFBLOCK* pdbCur = pdbList;
	while (fRet && pdbCur != NULL) {
		switch (pdbCur->chCommand) {
		case 'a':
			fRet = FDeleteBlockAtLine(pdbCur->iLineOld + 1, pdbCur->cLineNew, &pdbCur->rgatomDeleted,
				 &pdbCur->cAtomDeleted, nVer, filerep,
				 pListenerHead);	// REVIEW: this "+ 1" is emperically required but is very odd
			break;
		case 'd':
			fRet = FAddBlockAtLine(pdbCur->szOld, pdbCur->iLineOld, pdbCur->cLineOld,
				 &pdbCur->rgatomDeleted, &pdbCur->cAtomDeleted, nVer, filerep, pListenerHead);
			break;
		case 'c':
			fRet =
				 FChangeBlockAtLine(pdbCur->szOld, pdbCur->iLineOld, pdbCur->cLineNew, pdbCur->cLineOld,
					  &pdbCur->rgatomDeleted, &pdbCur->cAtomDeleted, nVer, filerep, pListenerHead);
			break;

		default:
			assert(false);
			break;
		}

		pdbCur = pdbCur->PdbNext();
	}

	// filerep.SetVer(nVer);

	// LDone:
	return fRet;
}


int NVerMinFromPdr(const DIFFRECORD* pdrList) {
	int nVerMin = -1;
	const DIFFRECORD* pdrCur = pdrList;

	while (pdrCur != NULL) {
		nVerMin = pdrCur->nVer;
		pdrCur = pdrCur->PdrNext();
	}

	return nVerMin;
}

int NVerMaxFromPdr(const DIFFRECORD* pdrList) {
	int nVerMax = -1;
	const DIFFRECORD* pdrCur = pdrList;

	while (pdrCur != NULL) {
		nVerMax = pdrCur->nVer;
		pdrCur = pdrCur->PdrPrev();
	}

	return nVerMax;
}


static bool FNewVersion(int iLine, int nVer, FILEREP& filerep) {
	assert(iLine >= 0);

	TCHAR szNver[16];
	_itot_s(nVer, szNver, _countof(szNver), 10);

	ATOM atomNew = ::AddAtom(szNver);
	assert(atomNew != 0);
	if (atomNew == 0)
		return false;

	filerep.m_rgatomVer[iLine] = atomNew;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	return true;
}

static bool FDeleteVersionRange(int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 FILEREP& filerep) {
	assert(iLine >= 0);
	assert(cLine >= 0);
	assert(prgatomDeleted != NULL);
	assert(pcAtomDeleted != NULL);

	assert(*pcAtomDeleted == 0);

	if (*prgatomDeleted == NULL) {
		*prgatomDeleted = new ATOM[cLine];
		if (*prgatomDeleted == NULL)
			return false;
#if _DEBUG
		memset(*prgatomDeleted, 0, cLine * sizeof(**prgatomDeleted));
#endif
	}

	for (int i = 0; i < cLine; ++i) {
		ATOM atom = filerep.m_rgatomVer[iLine + i];
#if _DEBUG
		filerep.m_rgatomVer[iLine + i] = 0;
#endif
		assert(atom != 0);
		assert((*prgatomDeleted)[i] == 0);
		(*prgatomDeleted)[i] = atom;
	}

	*pcAtomDeleted = cLine;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	return true;
}

static bool FCreateOrRestoreVersionRange(int iLine,
	 int cLine,
	 ATOM* prgatomDeleted[],
	 int* pcAtomDeleted,
	 int nVer,
	 FILEREP& filerep) {
	assert(iLine >= 0);
	assert(cLine >= 0);
	assert(prgatomDeleted != NULL);
	assert(pcAtomDeleted != NULL);

	if (*prgatomDeleted == 0) {  //  create
		for (int i = 0; i < cLine; ++i) {
			bool fRet = FNewVersion(iLine + i, nVer, filerep);
			assert(fRet);
		}
	}
	else {  // restore
		assert(*prgatomDeleted != NULL);
		assert(cLine == *pcAtomDeleted);
		for (int i = 0; i < cLine; ++i) {
			assert(filerep.m_rgatomVer[iLine + i] == 0);
			assert((*prgatomDeleted)[i] != 0);
			filerep.m_rgatomVer[iLine + i] = (*prgatomDeleted)[i];
#if _DEBUG
			(*prgatomDeleted)[i] = 0;
#endif  // _DEBUG
		}
	}

	*pcAtomDeleted = 0;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	return true;
}

static bool FAppendVersion(int iLine, int nVer, FILEREP& filerep) {
	assert(iLine >= 0);
	ATOM atom = filerep.m_rgatomVer[iLine];

	assert(nVer > 0);
	TCHAR szAtom[256];
	int err;
	if (!::GetAtomName(atom, szAtom, _countof(szAtom)) && (err = ::GetLastError()) != 0) {
		assert(false);
		return false;
	}

	TCHAR szNver[16];
	if (szAtom[0] != '\0') {
		szNver[0] = ',';
		szNver[1] = ' ';
		_itot_s(nVer, szNver + 2, _countof(szNver) - 2, 10);
	}
	else {
		_itot_s(nVer, szNver, _countof(szNver), 10);
	}

	_tcscat_s(szAtom, _countof(szAtom), szNver);

	ATOM atomNew = ::AddAtom(szAtom);
	assert(atomNew != 0);
	if (atomNew == 0)
		return false;

	assert(atomNew != atom);
	BOOL fRet = ::DeleteAtom(atom);
	assert(!fRet);
	if (fRet) {
		::DeleteAtom(atomNew);
		return false;
	}

	filerep.m_rgatomVer[iLine] = atomNew;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtom[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtom, _countof(szAtom)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtom[0]);
		}
	}
#endif  // _DEBUG_ATOM

	return true;
}

static bool FRemoveLastVersion(int iLine, FILEREP& filerep) {
	assert(iLine >= 0);
	ATOM atom = filerep.m_rgatomVer[iLine];

	TCHAR szAtom[256];
	if (!::GetAtomName(atom, szAtom, _countof(szAtom)) && ::GetLastError() != 0) {
		assert(false);
		return false;
	}

	TCHAR* pszCommaLast = _tcsrchr(szAtom, ',');
	if (pszCommaLast != NULL)
		*pszCommaLast = '\0';

	ATOM atomNew = ::AddAtom(szAtom);  // n.b. we need to re-add the atom even if it hasn't changed
												  // (need to inc ref count)
	assert(atomNew != 0);
	if (atomNew == 0)
		return false;

	// if (atomNew != atom)
	{
		BOOL fRet = ::DeleteAtom(atom);
		assert(!fRet);
		if (fRet) {
			::DeleteAtom(atomNew);
			return false;
		}
	}

	filerep.m_rgatomVer[iLine] = atomNew;

#if _DEBUG_ATOM
	for (int iLineT = 0; iLineT < filerep.CLine(); ++iLineT) {
		if (filerep.m_rgatomVer[iLineT] != 0) {
			TCHAR szAtomT[256];
			if (!::GetAtomName(filerep.m_rgatomVer[iLineT], szAtomT, _countof(szAtomT)) &&
				 ::GetLastError() != 0) {
				assert(false);
			}
			assert(szAtomT[0]);
		}
	}
#endif  // _DEBUG_ATOM

	return true;
}

// finds the last version (or nVerMax, if it is > 0) that has changed this line
int NverLast(int iLine, const FILEREP& filerep, int nVerMax) {
	assert(iLine >= 0);
	if (filerep.CLine() <= iLine)
		return 0;  // can happen when no doc is open

	ATOM atom = filerep.m_rgatomVer[iLine];

	TCHAR szAtom[256];
	if (!::GetAtomName(atom, szAtom, _countof(szAtom)) && ::GetLastError() != 0) {
		assert(false);
		return 0;
	}

	TCHAR* pszCommaLast = szAtom;
	while ((pszCommaLast = _tcsrchr(szAtom, ',')) != NULL) {
		assert(pszCommaLast[1] == ' ');
		int nVer = _tstoi(pszCommaLast + 2);
		if (nVerMax <= 0 || nVer < nVerMax)
			return nVer;
		*pszCommaLast = '\0';
	}

	return _tstoi(szAtom);
}


const DIFFRECORD* PdrVerFromVersion(int nVer, const DIFFRECORD* pdrList) {
	const DIFFRECORD* pdrCur = pdrList;

	while (pdrCur != NULL && pdrCur->nVer != nVer) {
		if (pdrCur->nVer > nVer)
			pdrCur = pdrCur->PdrNext();
		else
			pdrCur = pdrCur->PdrPrev();
	}

	return pdrCur;
}

static bool FIsAncestor(const DIFFRECORD* pdrAncestor, const DIFFRECORD* pdrCanidate) {
	const DIFFRECORD* pdrT = pdrCanidate;
	while (pdrT != NULL) {
		if (pdrT == pdrAncestor)
			return true;

		pdrT = pdrT->PdrParent();
	}

	return false;
}

const DIFFRECORD* PdrHead(const DIFFRECORD* pdrList) {
	const DIFFRECORD* pdrT = pdrList;
	while (pdrT != NULL) {
		if (pdrT->PdrPrev() == NULL)
			return pdrT;

		pdrT = pdrT->PdrPrev();
	}

	return NULL;
}

static int _NverMax(const DIFFRECORD* pdr) {
	const DIFFRECORD* pdrT = PdrHead(pdr);
	if (pdrT != NULL)
		return pdrT->nVer;

	return -1;
}


static bool FApplyBranchToFileRep(HINSTANCE hinst,
	 const DIFFRECORD* pdrBranch,
	 DIFFRECORD* pdrCurrent,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead,
	 CWnd* pwndStatus) {
	if (pdrBranch == NULL)
		return true;  // no-op

	if (pdrCurrent == NULL) {
		assert(false);
		return false;
	}

	assert(PdrHead(pdrCurrent) == filerep.PdrListRoot());

	// save off old version if not done yet
	int crgatomver;
	ATOM* rgatomver = pdrCurrent->GetVersions(&crgatomver);
	if (rgatomver == NULL) {
		ATOM* rgatomverOld = new ATOM[filerep.CLine()];
		memcpy(rgatomverOld, filerep.m_rgatomVer, sizeof(*rgatomverOld) * crgatomver);
		pdrCurrent->SetVersions(rgatomverOld, crgatomver);
	}

	// now we need to build the current save point if it isn't there (i.e.
	// pdrCurRoot->PdrParent().GetVersions() is NULL)

	ATOM* prgatomverNew = pdrBranch->GetVersions(&crgatomver);
	if (prgatomverNew == NULL) {
		// apply versions from base up to PdrHead(pdrCurRoot->PdrParent()) to pdrCurRoot->PdrParent()
		// it is currently just easier to get base version and apply the diffs again

		VERIFY(FReadBaseFileIntoFilerep(hinst, pdrBranch->szBranchFrom, filerep, pwndStatus));
		VERIFY(FEditFileToVersion(hinst, pdrBranch->nVer, filerep, pListenerHead, pwndStatus));
	}
	else {
		memcpy(filerep.m_rgatomVer, prgatomverNew, sizeof(*filerep.m_rgatomVer) * crgatomver);
		VERIFY(FEditFileToVersion(hinst, pdrBranch->nVer, filerep, pListenerHead, pwndStatus));
	}

	return true;
}


bool FEditFileToVersion(HINSTANCE hinst,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListenerHead,
	 CWnd* pwndStatus) {
	const DIFFRECORD* pdrCurRoot = filerep.PdrListRoot();

	// find current record for this FILEREP state
	const DIFFRECORD* pdrCur =
		 PdrVerFromVersion(nVer > filerep.NVer() ? filerep.NVer() + 1 : filerep.NVer(), pdrCurRoot);
	if (pdrCur == NULL) {
		// no requested version
		return false;
	}

	bool fChanged = false;
	while (pdrCur != NULL && filerep.NVer() != nVer) {
		bool fRet;
		if (nVer > filerep.NVer()) {
			assert(pdrCur->nVer == filerep.NVer() + 1);

			fRet = FApplyDiffBlockList(pdrCur->Pdb(), pdrCur->nVer, filerep, pListenerHead);
			if (!fRet) {
				assert(false);
				return false;
			}

			filerep.SetNclNver(pdrCur->nCL, pdrCur->nVer);

			pdrCur = pdrCur->PdrPrev();
		}
		else {
			if (pdrCur->nVer <= 1)
				break;  // version limit

			assert(pdrCur->nVer == filerep.NVer());

			fRet = FRemoveDiffBlockList(pdrCur->Pdb(), pdrCur->nVer - 1, filerep, pListenerHead);
			if (!fRet) {
				assert(false);
				return false;
			}


			pdrCur = pdrCur->PdrNext();
			assert(pdrCur != NULL);	 // this is ensured by the nVer check above
			if (pdrCur != NULL)
				filerep.SetNclNver(pdrCur->nCL, pdrCur->nVer);
		}

		if (pwndStatus != NULL) {
			CString strStatus;
			strStatus.Format(IDS_LOADING_MERGING_ARG, filerep.NVer());
			pwndStatus->SetWindowText(strStatus);
		}

		fChanged = true;
		assert(filerep.NVer() > 0);
	}

	if (fChanged) {
		CSimpleAgeViewerDocListener* pListener = pListenerHead;
		while (pListener != NULL) {
			pListener->DocVersionChangedNotification(filerep.NVer());
			pListener = pListener->m_pNext;
		}

		if (pwndStatus != NULL) {
			CString strStatus;
			strStatus.LoadString(IDS_LOADING_MERGING_DONE);
			pwndStatus->SetWindowText(strStatus);
		}
	}

	return fChanged;
}

void GetVersionsFromLines(int iLine, int cLine, const FILEREP& filerep, CUIntArray& ary) {
	for (int i = iLine; i < iLine + cLine; ++i) {
		UINT nVer = 0;
		do {
			UINT nVerPrev = nVer;
			nVer = NverLast(i, filerep, nVerPrev);
			if (nVer == nVerPrev)
				break;

			assert(nVer > 0);
			bool fInserted = false;
			for (int j = 0; j < ary.GetCount(); ++j) {
				UINT nVerAt = ary.GetAt(j);
				if (nVerAt >= nVer) {
					if (nVerAt > nVer)
						ary.InsertAt(j, nVer);
					fInserted = true;
					break;
				}
			}

			if (!fInserted)
				ary.InsertAt(ary.GetCount(), nVer);
		} while (nVer > 1);
	}
}


enum PIPES { PIPE_READ, PIPE_WRITE, _PIPE_MAX }; /* Constants 0 and 1 for READ and WRITE */

static FILE* _ProcessinfoCreate(const TCHAR szAppName[],
	 TCHAR szCommandLine[] /*note: this *can* be modified*/,
	 const TCHAR szDir[],
	 __out PROCESS_INFORMATION& pi) {
	STARTUPINFO si = {
		sizeof(si),
	};
	// si.wShowWindow = SW_MINIMIZE;
	memset(&pi, 0, sizeof(pi));
	int rgfdPipe[_PIPE_MAX] = {};

	_pipe(rgfdPipe, 1024, _O_TEXT);

	si.dwFlags = STARTF_USESTDHANDLES;
	// si.hStdInput = reinterpret_cast<HANDLE>(_get_osfhandle(rgfdPipe[PIPE_READ]));
	si.hStdOutput = reinterpret_cast<HANDLE>(_get_osfhandle(rgfdPipe[PIPE_WRITE]));
	// si.hStdError = reinterpret_cast<HANDLE>(_get_osfhandle(rgfdPipe[PIPE_WRITE]));

	// Start the child process.
	if (!::CreateProcess(szAppName,	// No module name (use command line)
			  szCommandLine,				// Command line
			  NULL,							// Process handle not inheritable
			  NULL,							// Thread handle not inheritable
			  TRUE,							// Set handle inheritance to *TRUE*
			  CREATE_NO_WINDOW,			// creation flags
			  NULL,							// Use parent's environment block
			  szDir,							// Use parent's starting directory
			  &si,							// Pointer to STARTUPINFO structure
			  &pi)							// Pointer to PROCESS_INFORMATION structure
	) {
		DWORD err = ::GetLastError();
		assert(false);
		_close(rgfdPipe[PIPE_READ]);
		_close(rgfdPipe[PIPE_WRITE]);

		return NULL;
	}
	_close(rgfdPipe[PIPE_WRITE]);	 // this has been inherited by the child process

	FILE* fhRet = _fdopen(rgfdPipe[PIPE_READ], "r");
	if (fhRet == NULL)
		_close(rgfdPipe[PIPE_READ]);

	return fhRet;	// if non-NULL, fh now "owns" rgfdPipe[PIPE_READ]
}


// Find the root (i.e. the first occurence of the file 'sd.ini' on the path taken from the root)
// NOTE: return string has ending '\'
static size_t _CchzGetSdRootDirWz(HINSTANCE hinst, WCHAR wzRootRet[], size_t cchzRootRet) {
	if (wzRootRet[0] == '\0') {
		int cchz = ::GetModuleFileName(
			 hinst /*theApp.m_hInstance*/, wzRootRet, static_cast<DWORD>(cchzRootRet));
		if (cchz <= 0)
			return 0;
		cchzRootRet -= cchz;
	}

	static WCHAR s_wzRootDir[_MAX_PATH + 1];
	if (s_wzRootDir[0] != '\0') {
		wcscpy_s(wzRootRet, cchzRootRet, s_wzRootDir);
		return wcslen(wzRootRet) + 1;
	}

	// Find the current Office root (the first place the hidden file sd.ini is located)
	WCHAR* pwchPath = wzRootRet;
	while (*pwchPath != L'\0') {
		if (*pwchPath == L'\\') {
			++pwchPath;	 // include '\\'
			WCHAR wzFilePathT[_MAX_PATH + 1];
			size_t iOffset = pwchPath - wzRootRet;
			memcpy(wzFilePathT, wzRootRet, iOffset * sizeof(WCHAR));
			wcscpy_s(wzFilePathT + iOffset, _countof(wzFilePathT) - iOffset, L"sd.ini");
			if (::GetFileAttributes(wzFilePathT) != -1) {
				*pwchPath = L'\0';  // stop at root
				wcscpy_s(s_wzRootDir + 1, _countof(s_wzRootDir) - 1, wzRootRet + 1);
				s_wzRootDir[0] = wzRootRet[0];  // make thread safe (atomic)
				return iOffset + 1;				  // found root
			}
		}
		++pwchPath;
		--cchzRootRet;
	}

	return 0;
}

static CString _CstrFindSDExe(HINSTANCE hinst, const TCHAR szDir[]) {
	TCHAR szSdExe[MAX_PATH];
	TCHAR* pchName = NULL;
	if (::SearchPath(NULL, _T("sd.exe"), NULL /*szExt*/, _countof(szSdExe), szSdExe, &pchName)) {
		// found it
	}
	else {
		TCHAR szSdRoot[MAX_PATH];
		_tcscpy_s(szSdRoot, _countof(szSdRoot), szDir);
		int cchzSdDir = _CchzGetSdRootDirWz(hinst, szSdRoot, _countof(szSdRoot));
		if (cchzSdDir <= 0) {
			assert(false);
			*szSdExe = _T('\0');

			goto LDone;
		}
		_tcscpy_s(szSdExe, _countof(szSdExe), szSdRoot);
		_tcscpy_s(szSdExe + cchzSdDir - 1, _countof(szSdExe) - cchzSdDir,
			 _T("dev14\\otools\\bin\\sd.exe"));	 // TODO: fix
	}

LDone:
	return CString(szSdExe);
}


bool FReadBaseFileIntoFilerep(HINSTANCE hinst,
	 const CString& cstrFilepath,
	 FILEREP& filerep,
	 CWnd* pwndStatus) {
	bool fRet = true;
	FILE* fhBase = NULL;
	PROCESS_INFORMATION pi = {};

	TCHAR szDir[MAX_PATH];
	_tcscpy_s(szDir, _countof(szDir), cstrFilepath);
	TCHAR* pDir = _tcsrchr(szDir, '\\');
	if (pDir != NULL)
		*pDir = '\0';

	const CString cstrSdExe = _CstrFindSDExe(hinst, szDir);

	// read
	TCHAR szCommand[MAX_PATH];
	_stprintf_s(szCommand, _countof(szCommand), _T("%s print -q %s#1"), (const TCHAR*)cstrSdExe,
		 (const TCHAR*)cstrFilepath);
	fhBase = _ProcessinfoCreate(NULL, szCommand, cstrFilepath[0] == '/' ? NULL : szDir, pi /*ref*/);
	if (fhBase == NULL)
		goto LDone;

	fpos_t fpost_tStart = 0;
	fsetpos(fhBase, &fpost_tStart);
	if (!_FReadFile(fhBase, filerep, pwndStatus)) {
		assert(false);
		fRet = false;
		goto LDone;
	}
	// filerep.SetVer(1);

	::WaitForSingleObject(pi.hProcess, 120 * 1000 /*ms*/);
	VERIFY(::CloseHandle(pi.hThread));
	pi.hThread = NULL;
	VERIFY(::CloseHandle(pi.hProcess));
	pi.hProcess = NULL;

LDone:
	if (pi.hThread != NULL)
		VERIFY(::CloseHandle(pi.hThread));
	if (pi.hProcess != NULL) {
		::TerminateProcess(pi.hProcess, -1);  // ensure it's dead
		VERIFY(::CloseHandle(pi.hProcess));
	}

	if (fhBase != NULL)
		VERIFY(!fclose(fhBase));

	return fRet;
}

FILEREP* PfrThreadedFromFilepath(HINSTANCE hinst, const CString& cstrFilepath, CWnd* pwndStatus) {
	DIFFRECORD* pdrList = NULL;
	TCHAR szDir[MAX_PATH];
	PROCESS_INFORMATION pi2 = {};
	FILE* fhDiff = NULL;

	FILEREP* pfrRoot = new FILEREP;

	if (!FReadBaseFileIntoFilerep(hinst, cstrFilepath, *pfrRoot, pwndStatus))
		goto LDone;

	_tcscpy_s(szDir, _countof(szDir), cstrFilepath);
	TCHAR* pDir = _tcsrchr(szDir, '\\');
	if (pDir != NULL)
		*pDir = '\0';

	{
		const CString cstrSdExe = _CstrFindSDExe(hinst, szDir);

		// read
		TCHAR szCommand[MAX_PATH];
		_stprintf_s(szCommand, _countof(szCommand), _T("%s filelog -l -d %s"),
			 (const TCHAR*)cstrSdExe, (const TCHAR*)cstrFilepath);

		fhDiff =
			 _ProcessinfoCreate(NULL, szCommand, cstrFilepath[0] == '/' ? NULL : szDir, pi2 /*ref*/);
		if (fhDiff == NULL)
			goto LDone;

		pdrList = _PdrProccessDiffLines(fhDiff, NULL /*ppdrMax*/, *pfrRoot, pwndStatus);
		if (pdrList == NULL)
			goto LDone;

		::WaitForSingleObject(pi2.hProcess, 120 * 1000 /*ms*/);
	}

	pfrRoot->SetPdrListRoot(pdrList);
	pdrList = NULL;

LDone:
	if (pdrList != NULL)
		delete pdrList;

	if (fhDiff != NULL)
		VERIFY(!fclose(fhDiff));

	if (pi2.hThread != NULL)
		VERIFY(::CloseHandle(pi2.hThread));
	if (pi2.hProcess != NULL) {
		::TerminateProcess(pi2.hProcess, -1);	// ensure it's dead
		VERIFY(::CloseHandle(pi2.hProcess));
	}

	return pfrRoot;
}



static bool FPrintFile(FILEREP& filerep) {
	bool fRet = true;

	for (int iLine = 0; iLine < filerep.CLine(); ++iLine)
		_ftprintf(stdout, _T("%s"), filerep.GetLine(iLine));
	//		_ftprintf(stdout, _T("%3d: %s"), filerep.m_rgnVer[iLine], filerep.GetLine(iLine));

	// LDone:
	return fRet;
}

int _tmain(int argc, _TCHAR* argv[]) {
	int err = 0;
	FILE* fhDiff = NULL;
	FILE* fhBase = NULL;
	FILEREP* pfilerep = new FILEREP;
	DIFFRECORD* pdrList = NULL;

	if (argc != 4)
		goto LDone;

	err = _tfopen_s(&fhDiff, argv[1], _T("r"));
	if (err != 0 || fhDiff == NULL)
		goto LDone;

	pdrList = _PdrProccessDiffLines(fhDiff, NULL /*ppdrMax*/, *pfilerep, NULL);

	err = _tfopen_s(&fhBase, argv[2], _T("r"));
	if (err != 0 || fhBase == NULL) {
		assert(false);
		goto LDone;
	}

	if (!_FReadFile(fhBase, *pfilerep, NULL /*pwndStatus*/)) {
		err = -1;
		goto LDone;
	}
	// pfilerep->SetVer(1);
	if (pdrList != NULL)
		pfilerep->SetNclNver(pdrList->nCL, pdrList->nVer);

	if (!FEditFileToVersion(AfxGetInstanceHandle(), _tstoi(argv[3]), *pfilerep)) {
		err = -1;
		goto LDone;
	}
	if (!FPrintFile(*pfilerep)) {
		err = -1;
		goto LDone;
	}

LDone:
	delete pdrList;
	delete pfilerep;
	if (fhBase != NULL)
		fclose(fhBase);
	if (fhDiff != NULL)
		fclose(fhDiff);
	if (err != 0)
		fprintf(stderr, "Usage: diff-stream-file base-version-file version-num");
	return err;
}

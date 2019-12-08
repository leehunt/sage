#include <atltypes.h>
#include <assert.h>

class CWnd;
class CUIntArray;
class DIFFRECORD;

const int MAX_LINES = 100000;
const int MAX_LINE = 256;

struct FILEREP {
	FILEREP() : m_cLine(0), m_nCL(0), m_nVer(0), m_pdrListRoot(NULL), m_pdrParent(NULL) {
		m_rgpszFileBuffer[0] = '\0';
		m_ptOrg.x = m_ptOrg.y = 0;
	}

	~FILEREP() { Reset(); }

	void Reset();

	static const TCHAR* _tcsistr(_In_z_ const TCHAR* sz, _In_z_ const TCHAR* szSub) {
		const TCHAR* pch = sz;
		int cchSub = _tcslen(szSub);
		while (*pch != '\0') {
			if (!_tcsncicmp(pch, szSub, cchSub))
				return pch;
			++pch;
		}

		return NULL;
	}

	CPoint CPointFindString(const TCHAR* wzFind,
		 bool fCaseSensitive,
		 const CPoint& cpStart = CPoint(0, 0)) const {
		int iCol = cpStart.x;
		assert(iCol >= 0);
		if (iCol < 0 || iCol >= MAX_LINE)
			return CPoint(-1, -1);

		int iLine = cpStart.y;
		assert(iLine >= 0);
		if (iLine < 0 || iLine >= m_cLine)
			return CPoint(-1, -1);

		const TCHAR* wzLine = GetLine(iLine);
		if (fCaseSensitive) {
			if (iCol > 0 && iCol < (int)_tcslen(wzLine)) {
				const TCHAR* wzFound = _tcsstr(wzLine + iCol, wzFind);
				if (wzFound != NULL)
					return CPoint(wzFound - GetLine(iLine), iLine);
				++iLine;
			}

			for (; iLine < m_cLine; ++iLine) {
				wzLine = GetLine(iLine);
				const TCHAR* wzFound = _tcsstr(GetLine(iLine), wzFind);
				if (wzFound != NULL)
					return CPoint(wzFound - GetLine(iLine), iLine);
			}
		}
		else {
			if (iCol > 0 && iCol < (int)_tcslen(GetLine(iLine))) {
				const TCHAR* wzFound = _tcsistr(&GetLine(iLine)[iCol], wzFind);
				if (wzFound != NULL)
					return CPoint(wzFound - GetLine(iLine), iLine);
				++iLine;
			}

			for (; iLine < m_cLine; ++iLine) {
				wzLine = GetLine(iLine);
				const TCHAR* wzFound = _tcsistr(GetLine(iLine), wzFind);
				if (wzFound != NULL)
					return CPoint(wzFound - GetLine(iLine), iLine);
			}
		}

		return CPoint(-1, -1);
	}

	CPoint CPointFindStringBackwards(const TCHAR* wzFind,
		 bool fCaseSensitive,
		 const CPoint& cpStart = CPoint(0, 0)) const {
		int iCol = cpStart.x;
		assert(iCol >= 0);
		if (iCol < 0 || iCol >= MAX_LINE)
			return CPoint(-1, -1);

		int iLine = cpStart.y;
		assert(iLine >= 0);
		if (iLine < 0 || iLine >= m_cLine)
			return CPoint(-1, -1);

		const TCHAR* wzLine = GetLine(iLine);
		if (fCaseSensitive) {
			// look for change in current line
			if (iCol >= 0 && iCol < (int)_tcslen(wzLine)) {
				const TCHAR* wzLook = wzLine + iCol;
				while (wzLook > wzLine) {
					const TCHAR* wzFound = _tcsstr(wzLook, wzFind);
					if (wzFound != NULL && wzFound <= wzLook)
						return CPoint(wzFound - wzLine, iLine);
					--wzLook;
				}
				--iLine;
			}

			for (; iLine >= 0; --iLine) {
				const TCHAR* wzLine = GetLine(iLine);
				const TCHAR* wzFoundFirst = _tcsstr(wzLine, wzFind);
				if (wzFoundFirst != NULL) {
					const TCHAR* wzLook = wzLine + _tcslen(wzLine);
					while (wzLook >= wzFoundFirst)  // we found one, now ensure it is the last one
					{
						const TCHAR* wzFound = _tcsstr(wzLook, wzFind);
						if (wzFound != NULL)
							return CPoint(wzFound - wzLine, iLine);
						--wzLook;
					}
					assert(false);
				}
			}
		}
		else {
			// look for change in current line
			if (iCol >= 0 && iCol < (int)_tcslen(wzLine)) {
				const TCHAR* wzLook = wzLine + iCol;
				while (wzLook > wzLine) {
					const TCHAR* wzFound = _tcsistr(wzLook, wzFind);
					if (wzFound != NULL && wzFound <= wzLook)
						return CPoint(wzFound - wzLine, iLine);
					--wzLook;
				}
				--iLine;
			}

			for (; iLine >= 0; --iLine) {
				const TCHAR* wzLine = GetLine(iLine);
				const TCHAR* wzFoundFirst = _tcsistr(wzLine, wzFind);
				if (wzFoundFirst != NULL) {
					const TCHAR* wzLook = wzLine + _tcslen(wzLine);
					while (wzLook >= wzFoundFirst)  // we found one, now ensure it is the last one
					{
						const TCHAR* wzFound = _tcsistr(wzLook, wzFind);
						if (wzFound != NULL)
							return CPoint(wzFound - wzLine, iLine);
						--wzLook;
					}
					assert(false);
				}
			}
		}

		return CPoint(-1, -1);
	}


	// Find the location in a file given a version string (e.g. "1") starting at cpStart.
	// FUTURE: consider supporting x-dimension
	CPoint CPointFindVer(const TCHAR szVer[],
		 bool fNot,
		 const CPoint& cpStart = CPoint(0, 0)) const {
		int iLine = cpStart.y;

		if (iLine >= m_cLine)
			return CPoint(-1, -1);

		ATOM atomVer = ::FindAtom(szVer);
		if (atomVer == 0)
			return CPoint(-1, -1);	// no version(s) found

		if (fNot) {
			for (; iLine < m_cLine; ++iLine) {
				if (m_rgatomVer[iLine] != atomVer)
					return CPoint(0, iLine);
			}
		}
		else {
			for (; iLine < m_cLine; ++iLine) {
				if (m_rgatomVer[iLine] == atomVer)
					return CPoint(0, iLine);
			}
		}

		return CPoint(-1, -1);
	}

	CPoint CPointFindVerBackwards(const TCHAR szVer[],
		 bool fNot,
		 const CPoint& cpStart = CPoint(0, 0)) const {
		int iLine = cpStart.y;

		if (iLine >= m_cLine)
			return CPoint(-1, -1);

		ATOM atomVer = ::FindAtom(szVer);
		if (atomVer == 0)
			return CPoint(-1, -1);	// no version(s) found

		if (fNot) {
			for (; iLine >= 0; --iLine) {
				if (m_rgatomVer[iLine] != atomVer)
					return CPoint(0, iLine);
			}
		}
		else {
			for (; iLine >= 0; --iLine) {
				if (m_rgatomVer[iLine] == atomVer)
					return CPoint(0, iLine);
			}
		}

		return CPoint(-1, -1);
	}

	int CLine() const { return m_cLine; }	// the number of lines in the FILEREP
	const TCHAR* GetLine(int iLine) const;

	bool FAddLines(int iLine, int cLine);
	void RemoveLines(int iLine, int cLine);
	void SetLine(int iLine, const TCHAR szLine[]);

	int NVer() const { return m_nVer; }	 // the current version this FILEREP represents
	int Ncl() const { return m_nCL; }	 // the current change list this FILEREP represents
	void SetNclNver(int nCl, int nVer) {
		m_nCL = nCl;
		m_nVer = nVer;
	}

	const DIFFRECORD* PdrListRoot() const { return m_pdrListRoot; }
	DIFFRECORD* PdrListRoot() { return const_cast<DIFFRECORD*>(m_pdrListRoot); }	// REVIEW: const
	void SetPdrListRoot(const DIFFRECORD* pdrListRoot) { m_pdrListRoot = pdrListRoot; }

	void SetPdrParent(const DIFFRECORD* pdrParent) const /*mutable*/ { m_pdrParent = pdrParent; }
	const DIFFRECORD* GetPdrParent() const { return m_pdrParent; }

	void SetViewOrg(POINT ptOrg) const { m_ptOrg = ptOrg; }	// mutable so it is const
	const POINT& GetViewOrg() const { return m_ptOrg; }

	void SetDepotFilePath(const TCHAR szLine[]) { cstrDepotFilePath = szLine; }
	const CString& GetDepotFilepath() const { return cstrDepotFilePath; }

	ATOM m_rgatomVer[MAX_LINES] = {};  // TODO: accesorize

	private:
	TCHAR* m_rgpszFileBuffer[MAX_LINES];  // FUTURE: consider using a gap buffer (but this currently
													  // isn't a bottleneck)
	int m_cLine;
	int m_nCL;
	int m_nVer;
	CString cstrDepotFilePath;
	const DIFFRECORD* m_pdrListRoot;	 // the root of the DIFFRECORD linked list that created the this
												 // FILEREP for m_nVer
	mutable const DIFFRECORD* m_pdrParent;

	mutable POINT m_ptOrg;
};

class DIFFBLOCK;
class DIFFRECORD {
	public:
	DIFFRECORD(const DIFFRECORD* pPrev, FILEREP& frRoot) : m_pPrev(pPrev), m_frRoot(frRoot) {
		nVer = 0;
		nCL = 0;
		memset(&tm, 0, sizeof(tm));
		szAction[0] = '\0';
		szAuthor[0] = '\0';
		szType[0] = '\0';
		szBranchFrom[0] = '\0';
		nVerBranchStart = 0;
		nVerBranchEnd = 0;
		m_pdb = NULL;
		// m_pPrev = pPrev;  // set above
		m_pNext = NULL;
		m_pfrChild = NULL;
		m_pParent = NULL;
		m_rgatomVerSav = NULL;
		m_catomVerSav = 0;
		m_szChangeComment = NULL;

		if (pPrev) {
			assert(pPrev->m_pNext == NULL);
			pPrev->m_pNext = this;
		}
	}

	~DIFFRECORD();

	const DIFFRECORD* PdrPrev() const { return m_pPrev; }
	const DIFFRECORD* PdrNext() const { return m_pNext; }

	const DIFFRECORD* PdrChild() const {
		if (m_pfrChild != NULL)
			return m_pfrChild->PdrListRoot();
		return NULL;
	}
	const DIFFRECORD* PdrParent() const { return m_pParent; }

	void AddChildFilerep(FILEREP* pfrChild) {
		if (pfrChild != NULL)
			pfrChild->SetPdrParent(this);
		m_pfrChild = pfrChild;
	}

	FILEREP& GetRootFilerep() { return m_frRoot; }

	void AddDiffBlockChain(DIFFBLOCK* pdb) { m_pdb = pdb; }
	DIFFBLOCK* Pdb() const { return m_pdb; }

	ATOM* GetVersions(int* pcatomVerSav) const {
		if (pcatomVerSav != NULL)
			*pcatomVerSav = m_catomVerSav;
		return m_rgatomVerSav;
	}
	void SetVersions(ATOM* rgatomVerSav, int catomVerSav) {
		ASSERT(m_rgatomVerSav == NULL);
		m_rgatomVerSav = rgatomVerSav;
		m_catomVerSav = catomVerSav;
	}

	bool FAppendChangeComment(const TCHAR szLine[], int cchLine);
	const TCHAR* GetChangeComment() const { return m_szChangeComment; }

	public:
	int nVer;
	int nCL;
	struct tm tm;
	TCHAR szAction[16];
	TCHAR szAuthor[48];
	TCHAR szType[16];

	TCHAR szBranchFrom[256];
	int nVerBranchStart;
	int nVerBranchEnd;

	private:
	DIFFBLOCK* m_pdb;
	mutable const DIFFRECORD* m_pPrev;
	mutable const DIFFRECORD* m_pNext;

	// branch support
	mutable const FILEREP* m_pfrChild;
	mutable const DIFFRECORD* m_pParent;

	FILEREP& m_frRoot;

	ATOM* m_rgatomVerSav;
	int m_catomVerSav;

	TCHAR* m_szChangeComment;
};


FILEREP* PfrThreadedFromFilepath(HINSTANCE hinst, const CString& cstrFilepath, CWnd* pwndStatus);
class CSimpleAgeViewerDocListener;
bool FEditFileToVersion(HINSTANCE hinst,
	 int nVer,
	 FILEREP& filerep,
	 CSimpleAgeViewerDocListener* pListener = NULL,
	 CWnd* pwndStatus = NULL);
bool FReadBaseFileIntoFilerep(HINSTANCE hinst,
	 const CString& cstrFilepath,
	 FILEREP& filerep,
	 CWnd* pwndStatus);

const DIFFRECORD* PdrVerFromVersion(int nVer, const DIFFRECORD* pdrList);
const DIFFRECORD* PdrHead(const DIFFRECORD* pdrList);

int NVerMinFromPdr(const DIFFRECORD* pdrList);
int NVerMaxFromPdr(const DIFFRECORD* pdrList);

int NverLast(int iLine, const FILEREP& filerep, int nVerMax = 0);
void GetVersionsFromLines(int iLine, int cLine, const FILEREP& filerep, CUIntArray& ary);

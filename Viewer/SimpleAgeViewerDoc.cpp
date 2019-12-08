// SimpleAgeViewerDoc.cpp : implementation of the CSimpleAgeViewerDoc class
//

#include "stdafx.h"
#include <io.h>  // _get_osfhandle()

#include "SimpleAgeViewer.h"
#include "MainFrm.h"

#include "SimpleAgeViewerDoc.h"
#include "VersionsWnd.h"  // CVersionsWnd::SelectVersionNum()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSimpleAgeViewerDoc

IMPLEMENT_DYNCREATE(CSimpleAgeViewerDoc, CDocument)

BEGIN_MESSAGE_MAP(CSimpleAgeViewerDoc, CDocument)
	ON_UPDATE_COMMAND_UI(IDR_PROPERTIES_GRID, &CSimpleAgeViewerDoc::OnUpdatePropertiesPaneGrid)
	ON_UPDATE_COMMAND_UI(IDR_VERSIONS_TREE, &CSimpleAgeViewerDoc::OnUpdateVersionsTree)
END_MESSAGE_MAP()


// CSimpleAgeViewerDoc construction/destruction

CSimpleAgeViewerDoc::CSimpleAgeViewerDoc() : m_pfilerep(NULL), m_pfilerepRoot(NULL), m_pDocListenerHead(NULL), m_fNewDoc(false)
{
	// TODO: add one-time construction code here
}

CSimpleAgeViewerDoc::~CSimpleAgeViewerDoc()
{
	delete m_pfilerepRoot;
}

BOOL CSimpleAgeViewerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

class CSimpleAgeViewerDummyFile : public CFile
{
public:
	CSimpleAgeViewerDummyFile(HANDLE h, const TCHAR szFilePathActual[]) : CFile(h), m_strFileNameActual(szFilePathActual) {}

protected:
	virtual CString GetFilePath() const
	{
		return m_strFileName;
	}

	void Close()
	{
		__super::Close();
		if (m_strFileNameActual.GetLength() > 0)
		{
			BOOL fRet = ::DeleteFile(m_strFileNameActual);
			assert(fRet);
			if (fRet)
				m_strFileNameActual.Empty();
		}
	}

	CString m_strFileNameActual;
};

CFile* CSimpleAgeViewerDoc::GetFile(LPCTSTR lpszFileName, UINT nOpenFlags,
	CFileException* pError)
{
	if (*lpszFileName != _T('/'))
		return __super::GetFile(lpszFileName, nOpenFlags, pError);

	CFile* pFile = NULL;
	// attach a dummy file of 1 length to keep things happy
	TCHAR szTempFile[MAX_PATH];
	::GetTempPath(_countof(szTempFile), szTempFile);
	::GetTempFileName(szTempFile, _T("dum"), 0, szTempFile);
	FILE* fh;
	_tfopen_s(&fh, szTempFile, _T("w"));
	if (fh)  // stupid: the temp file needs to be non-null for the MFC file open code to unarchive it
	{
		fputc('x', fh);  // make file length non-null
		fflush(fh);  // update opened length (such that CFile::GetLength() won't return zero -- see CDocument::OnNewDocument())
		pFile = new CSimpleAgeViewerDummyFile((HANDLE)_get_osfhandle(_fileno(fh)), szTempFile);
		ASSERT(pFile != NULL);
		pFile->SetFilePath(lpszFileName);
	}

	return pFile;
}

BOOL CSimpleAgeViewerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	delete m_pfilerepRoot;
	m_pfilerep = m_pfilerepRoot = NULL;
	m_fNewDoc = true;  // HACK

	CSimpleAgeViewerDocListener* pListener = m_pDocListenerHead;
	while (pListener != NULL)
	{
		pListener->DocVersionChangedNotification(0);
		pListener = pListener->m_pNext;
	}

	if (!CDocument::OnOpenDocument(lpszPathName))
	{
		UpdateAllViews(NULL/*pSender*/);  // NULL - also update this view
		return FALSE;
	}

	// TODO: add open code here
	// (SDI documents will reuse this document)

	return TRUE;
}


// CSimpleAgeViewerDoc serialization

void CSimpleAgeViewerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		POSITION pos = GetFirstViewPosition();
		CView* pView = GetNextView(pos/*in/out*/);
		CWnd* pwndStatus = NULL;
		if (pView != NULL)
		{
			CFrameWnd* pParentFrame = pView->GetParentFrame();
			assert(pParentFrame != NULL);
			if (pParentFrame != NULL && pParentFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)))
			{
				CMainFrame* pMainFrame = static_cast<CMainFrame*>(pParentFrame);
				pwndStatus = pMainFrame != NULL ? &pMainFrame->GetStatusWnd() : NULL;
			}
		}

		m_pfilerep = m_pfilerepRoot = PfrThreadedFromFilepath(theApp.m_hInstance, ar.GetFile()->GetFilePath(), pwndStatus);
		if (m_pfilerep != NULL)
		{
			int nVerMax = NVerMaxFromPdr(m_pfilerep->PdrListRoot());
			if (!FEditFileToVersion(theApp.m_hInstance, nVerMax, *m_pfilerep, NULL/*pListenerHead*/, pwndStatus))
			{
				assert(false);
				goto LDone;
			}
		}

	LDone:
		if (m_pfilerep == NULL)
		{
			if (pwndStatus != NULL)
			{
				CString strStatus;
				strStatus.LoadString(IDS_ERROR_LOADING_FILE);
				pwndStatus->SetWindowText(strStatus);
			}

			AfxThrowArchiveException(CArchiveException::genericException);
		}
	}
}

void CSimpleAgeViewerDoc::OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI)
{
	// Update document-based properties (i.e. currently displayed version)
	CWnd* pWndT = CWnd::FromHandlePermanent(*pCmdUI->m_pOther);
	ASSERT_VALID(pWndT);

	CMFCPropertyGridCtrl* pGrid = static_cast<CMFCPropertyGridCtrl*>(pWndT);
	ASSERT_VALID(pGrid);
	if (pGrid == NULL)
		return;

	CMFCPropertyGridProperty* pPropVersionHeader = pGrid->GetProperty(0);
	ASSERT_VALID(pPropVersionHeader);
	if (pPropVersionHeader == NULL)
		return;

	CMFCPropertyGridProperty* pPropVersion = pPropVersionHeader->GetSubItem(0);
	ASSERT_VALID(pPropVersion);
	if (pPropVersion == NULL)
		return;

	const DIFFRECORD* pdrList = m_pfilerep != NULL ? m_pfilerep->PdrListRoot() : NULL;
	if (pdrList != NULL)
	{
		int nVerMax = NVerMaxFromPdr(pdrList);
		pPropVersion->EnableSpinControl(TRUE, NVerMinFromPdr(pdrList), nVerMax);
		pPropVersion->Enable(TRUE);

		if (pPropVersion->GetValue().iVal != m_pfilerep->NVer())
		{
			if (pPropVersion->IsModified())
			{
				if (this->FEditToFileVersion(pPropVersion->GetValue().iVal, *m_pfilerep))
				{
					pPropVersion->SetOriginalValue(COleVariant((long)m_pfilerep->NVer(), VT_I4));
					pPropVersion->ResetOriginalValue();  // clears modified flag
					UpdateAllViews(NULL);
				}
			}

			pPropVersion->SetValue(COleVariant((long)m_pfilerep->NVer(), VT_I4));

			CPropertiesWnd::UpdateGridBlock(PdrVerFromVersion(m_pfilerep->NVer(), pdrList), pPropVersionHeader, nVerMax, false/*fUpdateVersionConttrol*/);
		}
	}
	else
	{
		pPropVersion->EnableSpinControl(FALSE);
		pPropVersion->Enable(FALSE);
	}

}

void CSimpleAgeViewerDoc::OnUpdateVersionsTree(CCmdUI* pCmdUI)
{
	// Update selection-based properties (e.g. currently displayed change)
	CWnd* pWndT = CWnd::FromHandlePermanent(*pCmdUI->m_pOther);
	ASSERT_VALID(pWndT);

	CTreeCtrl* pTree = static_cast<CTreeCtrl*>(pWndT);
	ASSERT_VALID(pTree);
	if (pTree == NULL)
		return;

	pCmdUI->m_bContinueRouting = TRUE;  // ensure that we route to doc

	if (m_fNewDoc)
	{
		pTree->DeleteAllItems();
		m_fNewDoc = false;
	}

	if (PfilerepRead() == NULL)
		return;

	int nCL = PfilerepRead()->Ncl();
	const DIFFRECORD* pdrRootHead = PdrHead(PfilerepReadRoot()->PdrListRoot());
	if (CVersionsWnd::FEnsureTreeItemsAndSelection(*pTree, pTree->GetRootItem(), pdrRootHead, nCL, PfilerepReadRoot()->GetDepotFilepath()))
	{
		HTREEITEM htreeitemSelected = pTree->GetSelectedItem();
		if (htreeitemSelected != NULL)
		{
			DIFFRECORD* pdrItem = reinterpret_cast<DIFFRECORD*>(pTree->GetItemData(htreeitemSelected));
			assert(pdrItem != NULL);
			if (pdrItem != NULL)
			{
				if (nCL != pdrItem->nCL)
				{
					if (FEditToFileVersion(pdrItem->nVer, pdrItem->GetRootFilerep()))
						UpdateAllViews(NULL);  // NULL - also update this view
				}
			}
		}
	}
}


bool CSimpleAgeViewerDoc::FEditToFileVersion(int nVer, FILEREP& fr)
{
	CFrameWnd* pFrame = GetRoutingFrame();
	//assert(pFrame != NULL);
	CMainFrame* pMainFrame = NULL;
	if (pFrame != NULL && pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)))
		pMainFrame = static_cast<CMainFrame*>(pFrame);
	CWnd* pwndStatus = pMainFrame != NULL ? &pMainFrame->GetStatusWnd() : NULL;
	bool fFilerepChanged = m_pfilerep != &fr;
	m_pfilerep = &fr;
	return ::FEditFileToVersion(theApp.m_hInstance, nVer, fr, m_pDocListenerHead, pwndStatus) || fFilerepChanged;
}


void CSimpleAgeViewerDoc::AddDocListener(CSimpleAgeViewerDocListener& listener)
{
	assert(listener.m_pNext == NULL);
	assert(listener.m_pPrev == NULL);

	if (m_pDocListenerHead != NULL)
	{
		assert(m_pDocListenerHead->m_pPrev == NULL);
		m_pDocListenerHead->m_pPrev = &listener;

		listener.m_pNext = m_pDocListenerHead;
	}

	m_pDocListenerHead = &listener;
}

void CSimpleAgeViewerDoc::RemoveDocListener(CSimpleAgeViewerDocListener& listener)
{
	assert(m_pDocListenerHead != NULL);
	assert(m_pDocListenerHead != listener.m_pNext);

	if (listener.m_pNext != NULL)
	{
		assert(listener.m_pNext->m_pPrev == &listener);
		listener.m_pNext->m_pPrev = listener.m_pPrev;
	}

	if (listener.m_pPrev != NULL)
	{
		assert(m_pDocListenerHead != &listener);

		assert(listener.m_pPrev->m_pNext == &listener);
		listener.m_pPrev->m_pNext = listener.m_pNext;

	}
	else
	{
		assert(m_pDocListenerHead == &listener);
		m_pDocListenerHead = listener.m_pNext;
	}

	listener.m_pNext = NULL;
	listener.m_pPrev = NULL;

	assert(m_pDocListenerHead != &listener);
}


// CSimpleAgeViewerDoc diagnostics

#ifdef _DEBUG
void CSimpleAgeViewerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSimpleAgeViewerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CSimpleAgeViewerDoc commands

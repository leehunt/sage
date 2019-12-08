
#include "stdafx.h"
#include <assert.h>

#include "VersionsWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "SimpleAgeViewer.h"
#include "SimpleAgeViewerDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CVersionsWnd

CVersionsWnd::CVersionsWnd() : m_pDoc(NULL), m_pToolTipControl(NULL)
{
}

CVersionsWnd::~CVersionsWnd()
{
	delete m_wndTreeCtrl.SetToolTips(NULL);
}

BEGIN_MESSAGE_MAP(CVersionsWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_PROPERTIES1, OnProperties1)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDR_VERSIONS_TREE, OnTreeNotifyExpanding)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVersionsWnd message handlers

void CVersionsWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	m_wndTreeCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top,
		rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

class MyCToolTipCtrl : public CToolTipCtrl
{
};


static void _SetToolTip(CTreeCtrl& tree, const DIFFRECORD& dr, HTREEITEM htreeitem)
{
	CToolTipCtrl* ptooltip = tree.GetToolTips();
	if (ptooltip != NULL)
	{
		TCHAR wzTipLimited[1024];  // must limit tips to this size
		_tcsncpy_s(wzTipLimited, dr.GetChangeComment(), _countof(wzTipLimited) - 1);
		RECT rcItem;
		VERIFY(tree.GetItemRect(htreeitem, &rcItem, FALSE/*bTextOnly*/));
		ptooltip->AddTool(&tree, wzTipLimited, &rcItem, dr.nVer);
	}
}


int CVersionsWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	if (!m_wndTreeCtrl.Create(WS_VISIBLE | WS_CHILD | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_NOTOOLTIPS, rectDummy, this, IDR_VERSIONS_TREE))
	{
		TRACE0("Failed to create Versions Tree \n");
		return -1;      // fail to create
	}

	CTooltipManager::CreateToolTip(m_pToolTipControl, this, AFX_TOOLTIP_TYPE_DEFAULT);
	if (m_pToolTipControl != NULL)
	{
		//m_pToolTipControl->Create(&m_wndTreeCtrl);

		m_pToolTipControl->SetMaxTipWidth(1024); // make multi-line
		//m_pToolTipControl->SetDelayTime(TTDT_RESHOW, 500);

		m_wndTreeCtrl.SetToolTips(m_pToolTipControl);

		//EnableToolTips();  // use this to activate internal MFC tool tips when using 'this' as the CWnd to CTooltipManager::CreateToolTip() and tree.GetParent() in ptooltip->AddTool
		//m_wndTreeCtrl.EnableToolTips();
	}

	InitPropList();

	AdjustLayout();

	return 0;
}

void CVersionsWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CVersionsWnd::OnExpandAllProperties()
{
	//m_wndTreeCtrl.ExpandAll();
}

void CVersionsWnd::OnUpdateExpandAllProperties(CCmdUI* pCmdUI)
{
}

void CVersionsWnd::OnSortProperties()
{
	//m_wndTreeCtrl.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CVersionsWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	//pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CVersionsWnd::OnProperties1()
{
	// TODO: Add your command handler code here
}

void CVersionsWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CVersionsWnd::OnProperties2()
{
	// TODO: Add your command handler code here
}

void CVersionsWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CVersionsWnd::InitPropList()
{
	SetPropListFont();

	if (m_imageList == NULL)
	{
		VERIFY(m_imageList.Create(IDB_VERSION_ICONS, 16, 1/*nGrow*/, RGB(255, 0, 255)));
		VERIFY(m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL) == NULL/*no prev image list*/);
	}
}

#if REFERENCE // sdv uses this code to create the branch prefix
struct CGlobals
{
public:
	...
		BOOL  StringBeginsWithBranchPrefixSlash(LPCTSTR psz) const
	{
		return StrCmpNI(psz, _pszSdvBranchPrefixSlash, _cchSdvBranchPrefixSlash) == 0;
	}
};
== =
// where _pszSdvBranchPrefixSlash is:
	/*
	 *  Load %SDVBRANCHPREFIX%/
	 */
	_LoadVariable(TEXT("SDVBRANCHPREFIX"), IDS_DEFAULT_SDVBRANCHPREFIX,
		szVar, ARRAYSIZE(szVar) - 1); /* subtract 1 for slash */
_pszSdvBranchPrefix = szVar;
StringCchCat(szVar, MAX_PATH, TEXT("/"));
_pszSdvBranchPrefixSlash = szVar;
_cchSdvBranchPrefixSlash = lstrlen(_pszSdvBranchPrefixSlash);

AND

IDS_DEFAULT_SDVBRANCHPREFIX "private"
== =

/*****************************************************************************
 *
 *  BranchOf
 *
 *      Given a full depot path, append the branch name.
 *
 *****************************************************************************/

	_String & operator<<(_String & str, BranchOf bof)
{
	if (bof && bof[0] == TEXT('/') && bof[1] == TEXT('/')) {
		//
		//  Skip over the word "//depot" -- or whatever it is.
		//  Some admins are stupid and give the root of the depot
		//  some other strange name.
		//
		LPCTSTR pszBranch = StrChr(bof + 2, TEXT('/'));
		if (pszBranch) {
			BOOL fPrivate = FALSE;

			pszBranch++;
			//
			//  If the next phrase is "private", then we are in a
			//  private branch; skip a step.
			//
			if (GlobalSettings.StringBeginsWithBranchPrefixSlash(pszBranch)) {
				fPrivate = TRUE;
				pszBranch += GlobalSettings.GetBranchPrefixSlashLength();
			}

			LPCTSTR pszSlash = StrChr(pszBranch, TEXT('/'));
			if (pszSlash) {
				str << Substring(pszBranch - fPrivate, pszSlash);
			}
		}
	}
	return str;
}

/*****************************************************************************
 *
 *  GetRevDispInfo
 *
 *      Combine the pszPath and the pszRev to form the string displayed
 *      to the user.  Since the rev is the more important thing, I will
 *      display it in the form
 *
 *      19 /Lab06_DEV:foo.cpp
 *
 *****************************************************************************/

void GetRevDispInfo(NMTREELIST* ptl, int iRev, LPCTSTR pszPath, LPCTSTR pszAlternatePort, BOOL fMark)
{
	OutputStringBuffer str(ptl->pszText, ptl->cchTextMax);
	str << iRev << TEXT(" ");
	if (pszAlternatePort) {
		str << TEXT("[") <<
			FriendlyDepotOf(pszAlternatePort) <<
			TEXT("]:");
	}
	if (pszPath) {
		str << BranchOf(pszPath) << TEXT(":") <<
			FilenameOf(pszPath);
	}
	if (fMark) {
		str << TEXT("#have");
	}
}
#endif // REFERENCE

static CString _CStringGetBranch(const CString& cstrDepotFilePath)
{
	CString cstr;
	const TCHAR* szFile = cstrDepotFilePath;
	if (szFile[0] == _T('/') && szFile[1] == _T('/')) {
		LPCTSTR szBranch = _tcschr(szFile + 2, _T('/'));
		if (szBranch != NULL) {
			bool fPrivate = false;

			szBranch++;  // skip slash

			//
			//  If the next phrase is "private", then we are in a
			//  private branch; skip a step.
			//
			const CString& cstrBranchPrefixSlash = theApp.GetBranchPrefixSlash();

			if (!_tcsncicmp(szBranch, cstrBranchPrefixSlash, cstrBranchPrefixSlash.GetLength())) {
				fPrivate = true;
				szBranch += cstrBranchPrefixSlash.GetLength();
			}

			LPCTSTR szSlash = _tcschr(szBranch, _T('/'));
			if (szSlash != NULL) {
				CString cstrT(szBranch - fPrivate);
				cstr = cstrT.Left(szSlash - szBranch + fPrivate);
			}
		}
	}

	return cstr;
}

static CString _CStringCreateLabel(const CString& cstrDepotFilePath, int nVer)
{
	CString cstr;
	int ichPath = cstrDepotFilePath.ReverseFind(_T('/'));
	const TCHAR* szFileName = (const TCHAR*)cstrDepotFilePath + ichPath + 1;
	cstr.AppendFormat(_T("%d %s:%s"), nVer, (const TCHAR*)_CStringGetBranch(cstrDepotFilePath), szFileName);

	return cstr;
}

enum VERSION_IMAGES
{
	VERSION_IMAGELIST_PLAIN,
	VERSION_IMAGELIST_DELETE,
	VERSION_IMAGELIST_ADD,
	VERSION_IMAGELIST_INTEGRATE,
	VERSION_IMAGELIST_BRANCH,
	VERSION_IMAGELIST_COPY,
	VERSION_IMAGELIST_IGNORE,

	VERSION_IMAGELIST_MERGE_OVERLAY,
	VERSION_IMAGELIST_BLANK_OVERLAY,
	VERSION_IMAGELIST_EXCLAIM_OVERLAY,
};

static void _SetTreeItemDataFromDr(CTreeCtrl& tree, HTREEITEM htreeitem,
	const DIFFRECORD& dr, const CString& cstrDr)
{
	VERIFY(tree.SetItemText(htreeitem, cstrDr));
	VERIFY(tree.SetItemData(htreeitem, reinterpret_cast<DWORD_PTR>(&dr)));

	switch (*dr.szAction)
	{
	case 'a':  // add
	{
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_ADD, VERSION_IMAGELIST_ADD));


		HTREEITEM htreeitemChild = NULL;
		while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
			tree.DeleteItem(htreeitemChild);
		break;
	}
	case 'b':  // branch
	{
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_BRANCH, VERSION_IMAGELIST_BRANCH));

		if (!tree.ItemHasChildren(htreeitem))
			tree.InsertItem(_T("Dummy"), htreeitem);
		break;
	}
	case 'c':  // copy
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_COPY, VERSION_IMAGELIST_INTEGRATE));
		break;
	case 'd':  // delete
	{
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_DELETE, VERSION_IMAGELIST_DELETE));

		HTREEITEM htreeitemChild = NULL;
		while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
			tree.DeleteItem(htreeitemChild);
		break;
	}
	case 'e':  // edit
	{
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_PLAIN, VERSION_IMAGELIST_PLAIN));

		HTREEITEM htreeitemChild = NULL;
		while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
			tree.DeleteItem(htreeitemChild);
		break;
	}
	case 'i':
		if (dr.szAction[1] == 'n')  // integrate
			VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE, VERSION_IMAGELIST_INTEGRATE));
		else
		{
			assert(dr.szAction[1] == 'g');  // ignore
			VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE, VERSION_IMAGELIST_INTEGRATE));
		}
		break;
	case 'm':  // merge
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE, VERSION_IMAGELIST_INTEGRATE));
		break;

	default:
	{
		assert(false);
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_PLAIN, VERSION_IMAGELIST_PLAIN));

		HTREEITEM htreeitemChild = NULL;
		while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
			tree.DeleteItem(htreeitemChild);
		break;
	}
	}

	_SetToolTip(tree, dr, htreeitem);
}


static HTREEITEM HtreeitemFromPdr(CTreeCtrl& tree, const DIFFRECORD* pdrStart)
{
	HTREEITEM htreeitem = tree.GetRootItem();
	if (htreeitem == NULL)
		return NULL;
	const DIFFRECORD* pdrRoot = reinterpret_cast<const DIFFRECORD*>(tree.GetItemData(htreeitem));

	// look forwards
	int cNext = 0;

	const DIFFRECORD* pdr = pdrStart;
	while (pdr != NULL)
	{
		if (pdr == pdrRoot)
		{
			for (int i = 0; i < cNext; ++i)
			{
				htreeitem = tree.GetPrevSiblingItem(htreeitem);
				ASSERT(htreeitem != NULL);
			}

			return htreeitem;
		}

		const DIFFRECORD* pdrParent = pdr->PdrParent();
		if (pdrParent != NULL)
		{
			if (HtreeitemFromPdr(tree, pdrParent) == NULL)
				return NULL;

			htreeitem = tree.GetChildItem(htreeitem);
			ASSERT(htreeitem != NULL);

			for (int i = 0; i < cNext; ++i)
			{
				htreeitem = tree.GetPrevSiblingItem(htreeitem);
			}

			return htreeitem;
		}

		++cNext;
		pdr = pdr->PdrNext();
	}

	if (pdrStart != NULL)
	{
		// look backwards
		int cPrev = 1;

		pdr = pdrStart->PdrPrev();
		while (pdr != NULL)
		{
			if (pdr == pdrRoot)
			{
				for (int i = 0; i < cPrev; ++i)
				{
					htreeitem = tree.GetNextSiblingItem(htreeitem);
					ASSERT(htreeitem != NULL);
				}

				return htreeitem;
			}

			const DIFFRECORD* pdrParent = pdr->PdrParent();
			if (pdrParent != NULL)
			{
				if (HtreeitemFromPdr(tree, pdrParent) == NULL)
					return NULL;

				htreeitem = tree.GetChildItem(htreeitem);
				ASSERT(htreeitem != NULL);

				for (int i = 0; i < cPrev; ++i)
				{
					htreeitem = tree.GetNextSiblingItem(htreeitem);
					ASSERT(htreeitem != NULL);
				}

				return htreeitem;
			}

			++cPrev;
			pdr = pdr->PdrPrev();
		}
	}

	return NULL;
}

// returns true if the current version has changed due to a tree selection
/*static*/ bool CVersionsWnd::FEnsureTreeItemsAndSelection(CTreeCtrl& tree, HTREEITEM htreeitemRoot,
	const DIFFRECORD* pdrEnd, int nCLSelection, const CString& cstrDepotFilePath)
{
	BOOL fSelected = FALSE;
	bool fTreeFocused = tree.GetFocus() == &tree;

	// sync current items
	const DIFFRECORD* pdr = pdrEnd;
	HTREEITEM htreeitem = htreeitemRoot;
	while (pdr != NULL && htreeitem != NULL)
	{
		const DIFFRECORD* pdrItem = reinterpret_cast<const DIFFRECORD*>(tree.GetItemData(htreeitem));
		if (pdrItem == NULL || pdrItem->nCL != pdr->nCL)
		{
			CString cstrDr = _CStringCreateLabel(cstrDepotFilePath, pdr->nVer);
			_SetTreeItemDataFromDr(tree, htreeitem, *pdr, cstrDr);

			if (nCLSelection == pdr->nCL)
				VERIFY(fSelected = tree.SelectItem(htreeitem));
		}
		else
		{
			if (!fTreeFocused && nCLSelection == pdr->nCL)
				VERIFY(fSelected = tree.SelectItem(htreeitem));
		}

		const DIFFRECORD* pdrChild = pdr->PdrChild();
		if (pdrChild != NULL)
			fSelected = !FEnsureTreeItemsAndSelection(tree, tree.GetChildItem(htreeitem), PdrHead(pdrChild), nCLSelection, cstrDepotFilePath);

		pdr = pdr->PdrNext();
		htreeitem = tree.GetNextItem(htreeitem, TVGN_NEXT);
	}

	// add any new items
	if (pdr != NULL)
	{
		HTREEITEM htreeitemParent = tree.GetParentItem(htreeitemRoot);
		do
		{
			CString cstrDr = _CStringCreateLabel(cstrDepotFilePath, pdr->nVer);
			HTREEITEM htreeitemNew = tree.InsertItem(cstrDr, htreeitemParent);  // inserts at end
			if (htreeitemNew != NULL)
			{
				_SetTreeItemDataFromDr(tree, htreeitemNew, *pdr, cstrDr);

				if (nCLSelection == pdr->nCL)
					VERIFY(fSelected = tree.SelectItem(htreeitemNew));
			}
			else
			{
				if (!fTreeFocused && nCLSelection == pdr->nCL)
					VERIFY(fSelected = tree.SelectItem(htreeitem));
			}

			//fSelected = !FEnsureTreeItemsAndSelection(tree, htreeitemNew, pdr->PdrChild(), nCLSelection, cstrFileName);

			pdr = pdr->PdrNext();
		} while (pdr != NULL);
	}
	else if (htreeitem != NULL)
	{
		do
		{
#if _DEBUG
			TVITEM tvitem = {};
			tree.GetItem(&tvitem);
#endif  // _DEBUG
			VERIFY(tree.DeleteItem(htreeitem));

			htreeitem = tree.GetNextItem(htreeitem, TVGN_NEXT);
		} while (htreeitem != NULL);
	}
	//ptooltip->AddTool(tree, wzTipLimited, &rcItem, dr.nVer);

	//assert(fSelected || fTreeFocused || nCLSelection == -1);
	return !fSelected;
}

void CVersionsWnd::OnTreeNotifyExpanding(NMHDR* pNMHDR, LRESULT* plResult)
{
	const NMTREEVIEW* pTreeView = reinterpret_cast<const NMTREEVIEW*>(pNMHDR);

	HTREEITEM htreeitem = pTreeView->itemNew.hItem;
	DIFFRECORD* pdrItem = reinterpret_cast<DIFFRECORD*>(m_wndTreeCtrl.GetItemData(htreeitem));
	if (pdrItem == NULL)
	{
		assert(false);
		return;
	}

	if (pdrItem->PdrChild() == NULL)
	{
		FILEREP* pfrChild = PfrThreadedFromFilepath(theApp.m_hInstance, CString(pdrItem->szBranchFrom), NULL/*pwndStatus*/);
		if (pfrChild != NULL)
		{
			pdrItem->AddChildFilerep(pfrChild);
			assert(m_wndTreeCtrl.ItemHasChildren(htreeitem));
			VERIFY(FEnsureTreeItemsAndSelection(m_wndTreeCtrl, m_wndTreeCtrl.GetChildItem(htreeitem), pfrChild->PdrListRoot(), -1/*nCLSelection*/, CString(pdrItem->szBranchFrom)));
		}
	}
}

void CVersionsWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndTreeCtrl.SetFocus();
}

void CVersionsWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CVersionsWnd::SetPropListFont()
{
	::DeleteObject(m_fntVersionsList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntVersionsList.CreateFontIndirect(&lf);

	m_wndTreeCtrl.SetFont(&m_fntVersionsList);
}

void CVersionsWnd::DocEditNotification(int /*iLine*/, int /*cLine*/)
{
}

void CVersionsWnd::DocVersionChangedNotification(int nVer)
{
	if (nVer == 0)
		m_wndTreeCtrl.DeleteAllItems();
}

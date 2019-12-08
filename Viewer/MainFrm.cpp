
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "SimpleAgeViewer.h"
#include "SimpleAgeViewerDoc.h"
#include "SimpleAgeViewerView.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class MyCFindMFCToolBarComboBoxButton : public CMFCToolBarComboBoxButton
{
public:
	MyCFindMFCToolBarComboBoxButton() :
		CMFCToolBarComboBoxButton(ID_EDIT_FIND_DROPDOWN/*nID*/, GetCmdMgr()->GetCmdImage(ID_EDIT_FIND)/*iImage*/, CBS_DROPDOWN)
	{
		m_strText.LoadString(IDS_TOOLBAR_FIND);

		Initialize();
	}

	virtual void Serialize(CArchive& ar)
	{
		__super::Serialize(ar);
	}

	DECLARE_SERIAL(MyCFindMFCToolBarComboBoxButton)

protected:
	virtual CString GetPrompt() const
	{
		static CString s_cstrText;
		if (s_cstrText.IsEmpty())
			s_cstrText.LoadString(IDS_TOOLBAR_FIND);
		return s_cstrText;
	}
};

IMPLEMENT_SERIAL(MyCFindMFCToolBarComboBoxButton, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 1)


// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_EDIT_FIND_MENU_COMMAND, &CMainFrame::OnFindMenuCommand)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_DROPDOWN, &CMainFrame::OnFindEdit)
	ON_COMMAND(ID_EDIT_FIND_DROPDOWN, &CMainFrame::OnFindCommand)  // on Enter in find edit
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, &CMainFrame::OnFindCommand)
	ON_COMMAND(ID_EDIT_FIND, &CMainFrame::OnFindCommand)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_BACKWARD, &CMainFrame::OnFindCommand)
	ON_COMMAND(ID_EDIT_FIND_BACKWARD, &CMainFrame::OnFindCommandBackward)
	ON_UPDATE_COMMAND_UI(ID_FIND_CASE_SENSITIVE, &CMainFrame::OnFindCaseSensitive)
	ON_COMMAND(ID_FIND_CASE_SENSITIVE, &CMainFrame::OnFindCaseSensitive)
	ON_UPDATE_COMMAND_UI(ID_FIND_HIGHLIGHT_ALL, &CMainFrame::OnFindHighlightAll)
	ON_COMMAND(ID_FIND_HIGHLIGHT_ALL, &CMainFrame::OnFindHighlightAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_NEXT_CURVER_DIFF, &CMainFrame::OnGotoDiff)
	ON_COMMAND(ID_EDIT_NEXT_CURVER_DIFF, &CMainFrame::OnGotoNextCurverDiff)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PREV_CURVER_DIFF, &CMainFrame::OnGotoDiff)
	ON_COMMAND(ID_EDIT_PREV_CURVER_DIFF, &CMainFrame::OnGotoPrevCurverDiff)
	ON_UPDATE_COMMAND_UI(ID_EDIT_NEXT_ANYVER_DIFF, &CMainFrame::OnGotoDiff)
	ON_COMMAND(ID_EDIT_NEXT_ANYVER_DIFF, &CMainFrame::OnGotoNextDiff)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PREV_ANYVER_DIFF, &CMainFrame::OnGotoDiff)
	ON_COMMAND(ID_EDIT_PREV_ANYVER_DIFF, &CMainFrame::OnGotoPrevDiff)
	ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, &CMainFrame::OnToolbarReset)
END_MESSAGE_MAP()

const UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LINE,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;
	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	if (!m_wndToolBarFind.CreateEx(this,
		TBSTYLE_FLAT,
		WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
		CRect(1, 1, 1, 1)/*rcBorder*/,
		IDR_TOOLBAR_FIND) ||
		!m_wndToolBarFind.LoadBitmap(IDB_FIND_TOOLBAR_256, 0, 0, FALSE/*bLocked*/)  // REVIEW: low color?
		)
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	MyCFindMFCToolBarComboBoxButton MyFindComboBoxButton;
	m_wndToolBarFind.InsertButton(MyFindComboBoxButton);

	CMFCToolBarButton toolBarButtonFind(ID_EDIT_FIND, 2 + 8/*iImage*/, NULL/*lpszText*/, FALSE/*bUserButton*/, TRUE/*bLocked*/);
	m_wndToolBarFind.InsertButton(toolBarButtonFind);

	CMFCToolBarButton toolBarButtonFindBackward(ID_EDIT_FIND_BACKWARD, 1 + 8/*iImage*/, NULL/*lpszText*/, FALSE/*bUserButton*/, TRUE/*bLocked*/);
	m_wndToolBarFind.InsertButton(toolBarButtonFindBackward);

	CMFCToolBarButton toolBarButtonSeperator(0/*nID*/, -1);
	toolBarButtonSeperator.SetStyle(TBBS_SEPARATOR);
	m_wndToolBarFind.InsertButton(toolBarButtonSeperator);

	CMFCToolBarButton toolBarButtonFindHighlightAll(ID_FIND_HIGHLIGHT_ALL, 0 + 8/*iImage*/, NULL/*lpszText*/, FALSE/*bUserButton*/, TRUE/*bLocked*/);
	toolBarButtonFindHighlightAll.SetStyle(TBBS_CHECKBOX);
	m_wndToolBarFind.InsertButton(toolBarButtonFindHighlightAll);

	CString cstrCaseSenstiveCheckbox;
	cstrCaseSenstiveCheckbox.LoadStringW(IDS_FIND_CASE_SENSITIVE);
	CMFCToolBarButton toolBarButtonFindCaseSensitive(ID_FIND_CASE_SENSITIVE, -1/*iImage*/, cstrCaseSenstiveCheckbox/*lpszText*/, FALSE/*bUserButton*/, TRUE/*bLocked*/);
	toolBarButtonFindCaseSensitive.SetStyle(TBBS_CHECKBOX);
	m_wndToolBarFind.InsertButton(toolBarButtonFindCaseSensitive);

	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_FIND);
	ASSERT(bNameValid);
	m_wndToolBarFind.SetWindowText(strToolBarName);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(*indicators));

	// Delete these lines if you don't want the toolbar and menubar to be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBarFind.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

	EnableDocking(CBRS_ALIGN_ANY);

	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);
	DockPane(&m_wndToolBarFind);

	// this must happen *after* docking the pane
	m_wndToolBar.SetSiblingToolBar(&m_wndToolBarFind);
	m_wndToolBar.SetOneRowWithSibling();

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// create docking windows
	if (!CreateDockingWindows())
	{
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);

	m_wndVersions.EnableDocking(CBRS_ALIGN_ANY);
	//m_wndVersions.SetPaneStyle(m_wndVersions.GetPaneStyle() & ~CBRS_TOOLTIPS);  // REVIEW: how to remove large "help area" in this pane?
	DockPane(&m_wndVersions);

	// this must happen *after* docking the pane
	m_wndProperties.DockToWindow(&m_wndVersions, CBRS_ALIGN_BOTTOM);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			m_UserImages.SetImageSize(CSize(16, 16), FALSE);
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	return m_wndSplitter.Create(this,
		2/*cRow*/, 1/*cCol*/,
		CSize(10, 10)/*sizMinPane*/,
		pContext);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWndEx::PreCreateWindow(cs))
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

BOOL CMainFrame::CreateDockingWindows()
{
	BOOL bNameValid;
	// Create properties window
	CString strPropertiesWnd;
	bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);
	if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 280, 200), TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Properties window\n");
		return FALSE; // failed to create
	}

	// Create versions window
	CString strVersionsWnd;
	bNameValid = strVersionsWnd.LoadString(IDS_VERSIONS_WND);
	ASSERT(bNameValid);
	if (!m_wndVersions.Create(strVersionsWnd, this, CRect(0, 0, 280, 200), TRUE, ID_VIEW_VERSIONSWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Versions window\n");
		return FALSE; // failed to create
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);
	HICON hVersionsBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_VERSIONS_WND_HC : IDI_VERSIONS_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndVersions.SetIcon(hVersionsBarIcon, FALSE);
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp, lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}


void CMainFrame::OnFindMenuCommand()
{
	int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
	CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
	ASSERT_VALID(pButton);
	if (pButton == NULL)
		return;

	CEdit* pEdit = pButton->GetEditCtrl();
	ASSERT_VALID(pEdit);
	if (pEdit == NULL)
		return;

	m_wndToolBarFind.ShowPane(TRUE/*bShow*/, TRUE/*bDelay*/, TRUE/*bActivate*/);

	pEdit->SetSel(0, -1);
	pEdit->SetFocus();
}

// TODO: fix this to only fire on find control edit changes
void CMainFrame::OnFindEdit(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);  // TODO: use document state

	// TODO: use pCmdUI
	int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
	CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
	ASSERT_VALID(pButton);
	if (pButton == NULL)
		return;

	CEdit* pEdit = pButton->GetEditCtrl();
	ASSERT_VALID(pEdit);
	if (pEdit == NULL)
		return;

	TCHAR szFind[256];
	int cch = pEdit->GetLine(0, szFind, 256);
	szFind[min(cch, 256)] = '\0';

	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CDocument* pDoc = pView->GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	POSITION pos = pDoc->GetFirstViewPosition();
	while ((pView = pDoc->GetNextView(pos)) != NULL)
	{
		CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
		ASSERT_VALID(pSimpleAgeViewerView);
		pSimpleAgeViewerView->FindAndSelectString(szFind, false/*fScroll*/);
	}

}

void CMainFrame::OnFindCommand(CCmdUI* pCmdUI)
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CDocument* pDoc = pView->GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	CSimpleAgeViewerDoc* pSimpleAgeViewerDoc = static_cast<CSimpleAgeViewerDoc*>(pDoc);
	if (pSimpleAgeViewerDoc->PfilerepRead() == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
	CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
	ASSERT_VALID(pButton);
	if (pButton == NULL)
		return;

	CEdit* pEdit = pButton->GetEditCtrl();
	ASSERT_VALID(pEdit);
	if (pEdit == NULL)
		return;

	TCHAR szFind[256];
	int cch = pEdit->GetLine(0, szFind, 256);
	szFind[min(cch, 256)] = '\0';

	pCmdUI->Enable(szFind[0] != '\0');
}

void CMainFrame::OnFindCommand()
{
	int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
	CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
	ASSERT_VALID(pButton);
	if (pButton == NULL)
		return;

	CEdit* pEdit = pButton->GetEditCtrl();
	ASSERT_VALID(pEdit);
	if (pEdit == NULL)
		return;

	TCHAR szFind[256];
	int cch = pEdit->GetLine(0, szFind, 256);
	szFind[min(cch, 256)] = '\0';

	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	bool fWrapAround;
	pSimpleAgeViewerView->FindAndSelectString(szFind, true/*fScroll*/, false/*fBackward*/, &fWrapAround);
	if (fWrapAround)
	{
		CString cstrWrapped;
		cstrWrapped.LoadString(IDS_FIND_WRAPPED_DOC);
		m_wndStatusBar.SetWindowText(cstrWrapped);
	}
	else
	{
		m_wndStatusBar.SetWindowText(_T(""));
	}

	if (!CMFCToolBar::IsLastCommandFromButton(pButton))
		return;

	CComboBox* pCombo = pButton->GetComboBox();
	assert(pCombo != NULL);
	if (pCombo != NULL)
	{
		int iItem = pCombo->FindStringExact(-1, szFind);
		if (iItem != CB_ERR)
			pCombo->DeleteString(iItem);

		pCombo->InsertString(0, szFind);
	}
}

void CMainFrame::OnFindCommandBackward()
{
	int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
	CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
	ASSERT_VALID(pButton);
	if (pButton == NULL)
		return;

	CEdit* pEdit = pButton->GetEditCtrl();
	ASSERT_VALID(pEdit);
	if (pEdit == NULL)
		return;

	TCHAR szFind[256];
	int cch = pEdit->GetLine(0, szFind, 256);
	szFind[min(cch, 256)] = '\0';

	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	bool fWrapAround;
	pSimpleAgeViewerView->FindAndSelectString(szFind, true/*fScroll*/, true/*fBackward*/, &fWrapAround);
	if (fWrapAround)
	{
		CString cstrWrapped;
		cstrWrapped.LoadString(IDS_FIND_WRAPPED_DOC_BACKWARD);
		m_wndStatusBar.SetWindowText(cstrWrapped);
	}
	else
	{
		m_wndStatusBar.SetWindowText(_T(""));
	}

	if (!CMFCToolBar::IsLastCommandFromButton(pButton))
		return;

	CComboBox* pCombo = pButton->GetComboBox();
	assert(pCombo != NULL);
	if (pCombo != NULL)
	{
		int iItem = pCombo->FindStringExact(-1, szFind);
		if (iItem != CB_ERR)
			pCombo->DeleteString(iItem);

		pCombo->InsertString(0, szFind);
	}
}

void CMainFrame::OnFindCaseSensitive(CCmdUI* pCmdUI)
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	pCmdUI->SetCheck(pSimpleAgeViewerView->GetCaseSensitive());
}

void CMainFrame::OnFindCaseSensitive()
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	pSimpleAgeViewerView->SetCaseSensitive(!pSimpleAgeViewerView->GetCaseSensitive());
	pSimpleAgeViewerView->Invalidate();
}

void CMainFrame::OnFindHighlightAll(CCmdUI* pCmdUI)
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	pCmdUI->SetCheck(pSimpleAgeViewerView->GetHighlightAll());
}

void CMainFrame::OnFindHighlightAll()
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	pSimpleAgeViewerView->SetHighlightAll(!pSimpleAgeViewerView->GetHighlightAll());
	pSimpleAgeViewerView->Invalidate();
}


void CMainFrame::OnGotoDiff(CCmdUI* pCmdUI)
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CDocument* pDoc = pView->GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	CSimpleAgeViewerDoc* pSimpleAgeViewerDoc = static_cast<CSimpleAgeViewerDoc*>(pDoc);
	pCmdUI->Enable(pSimpleAgeViewerDoc->PfilerepRead() != NULL);
}


void CMainFrame::OnGotoDiff(bool fCurVer, bool fBackwards)
{
	CView* pView = GetActiveView();
	if (pView == NULL)
		return;

	CSimpleAgeViewerView* pSimpleAgeViewerView = static_cast<CSimpleAgeViewerView*>(pView);
	ASSERT_VALID(pSimpleAgeViewerView);
	if (pSimpleAgeViewerView == NULL)
		return;

	bool fWraparound = false;
	pSimpleAgeViewerView->FindAndSelectVersion(fCurVer, fBackwards, &fWraparound);
	if (fWraparound)
	{
		CString cstrWrapped;
		cstrWrapped.LoadString(IDS_FIND_WRAPPED_DOC_BACKWARD);
		m_wndStatusBar.SetWindowText(cstrWrapped);
	}
	else
	{
		m_wndStatusBar.SetWindowText(_T(""));
	}
}

void CMainFrame::OnGotoNextCurverDiff()
{
	OnGotoDiff(true/*fCurVer*/, false/*fBackwards*/);
}

void CMainFrame::OnGotoPrevCurverDiff()
{
	OnGotoDiff(true/*fCurVer*/, true/*fBackwards*/);
}

void CMainFrame::OnGotoNextDiff()
{
	OnGotoDiff(false/*fCurVer*/, false/*fBackwards*/);
}

void CMainFrame::OnGotoPrevDiff()
{
	OnGotoDiff(false/*fCurVer*/, true/*fBackwards*/);
}


void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}


BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}

LRESULT CMainFrame::OnToolbarReset(WPARAM wp, LPARAM lp)
{
	UINT nIDToolbar = static_cast<UINT>(wp);

	switch (nIDToolbar)
	{
	case IDR_MAINFRAME:
	case IDR_MAINFRAME_256:
		// FUTURE: enable these buttons as necessary
		m_wndToolBar.RemoveButton(m_wndToolBar.CommandToIndex(ID_FILE_NEW));
		m_wndToolBar.RemoveButton(m_wndToolBar.CommandToIndex(ID_FILE_SAVE));
		m_wndToolBar.RemoveButton(m_wndToolBar.CommandToIndex(ID_EDIT_CUT));
		m_wndToolBar.RemoveButton(m_wndToolBar.CommandToIndex(ID_EDIT_PASTE));
		m_wndToolBar.RemoveButton(m_wndToolBar.CommandToIndex(ID_FILE_PRINT));
		break;
	}

	return 0;
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		if (!__super::PreTranslateMessage(pMsg))
		{  // HACK - allow accelerators to be processed from the Find edit field
			int iComboControl = m_wndToolBarFind.CommandToIndex(ID_EDIT_FIND_DROPDOWN);
			CMFCToolBarComboBoxButton* pButton = static_cast<CMFCToolBarComboBoxButton*>(m_wndToolBarFind.GetButton(iComboControl));
			ASSERT_VALID(pButton);
			if (pButton == NULL)
				return FALSE;

			if (!pButton->HasFocus())  // MFC specifically doesn't translate toolbar edit items (see CFrameImpl::ProcessKeyboard())
				return FALSE;

			HACCEL hAccel = GetDefaultAccelerator();
			return hAccel != NULL && ::TranslateAccelerator(m_hWnd, hAccel, pMsg);
		}
		else
			return TRUE;

	default:
		return __super::PreTranslateMessage(pMsg);
	}
}


void CMainFrame::OnActivateView(BOOL bActivate, CView* pActivateView,
	CView* pDeactiveView)
{
}

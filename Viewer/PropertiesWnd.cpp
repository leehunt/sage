
#include "stdafx.h"
#include <assert.h>

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "SimpleAgeViewer.h"
#include "..\SDVersionReader\SDVersionReader.h"
#include "SimpleAgeViewerView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// a subclass of CMFCPropertyGridProperty that fixes read-only copy behavior
class CMFCPropertyGridReadOnlySelectableProperty : public CMFCPropertyGridProperty
{
	DECLARE_DYNAMIC(CMFCPropertyGridReadOnlySelectableProperty)

public:
	// Simple property
	CMFCPropertyGridReadOnlySelectableProperty(const CString& strName, const COleVariant& varValue, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0,
		LPCTSTR lpszEditMask = NULL, LPCTSTR lpszEditTemplate = NULL, LPCTSTR lpszValidChars = NULL) :
		CMFCPropertyGridProperty(strName, varValue, lpszDescr, dwData,
			lpszEditMask, lpszEditTemplate, lpszValidChars) {}

protected:
	virtual BOOL PushChar(UINT nChar)  // called on WM_KEYDOWN
	{
		// look for ctrl+c or clrl+ins
		if (!IsAllowEdit() && m_pWndList != NULL &&
			m_pWndInPlace != NULL && m_pWndInPlace->GetSafeHwnd() != NULL)
		{
			switch (nChar)
			{
			case VK_CONTROL:
				return TRUE;  // eat and don't destroy edit

			case _T('V'):
			case _T('X'):
			{
				if (::GetAsyncKeyState(VK_CONTROL) < 0)  // Ctrl + 'V' --> Paste, Ctrl + 'X' --> Cut
					return TRUE;  // eat and don't destroy edit
			}
			break;

			case VK_DELETE:
			{
				if (::GetAsyncKeyState(VK_SHIFT) < 0)  // Shift + VK_INSERT --> Cut
					return TRUE;  // eat and don't destroy edit
			}
			break;

			case VK_INSERT:
			{
				if (::GetAsyncKeyState(VK_SHIFT) < 0)  // Shift + VK_INSERT --> Paste
					return TRUE;  // eat and don't destroy edit
			}
			// fallthrough
			case _T('C'):
			{
				// sigh; this is copied from ProcessClipboardAccelerators() which is protected
				BOOL bIsCtrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000);

				if (bIsCtrl)  // Ctrl + VK_INSERT, Ctrl + 'C' --> Copy
				{
					m_pWndInPlace->SendMessage(WM_COPY);
					return TRUE;  // don't destroy edit
				}
			}
			break;

			case _T('A'):
			{
				BOOL bIsCtrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000);

				if (bIsCtrl)  // Ctrl + 'A' --> Select All
				{
					m_pWndInPlace->SendMessage(EM_SETSEL, 0, -1);
					return TRUE;  // don't destroy edit
				}
			}
			break;
			}
		}

		return __super::PushChar(nChar);
	}

};

IMPLEMENT_DYNAMIC(CMFCPropertyGridReadOnlySelectableProperty, CMFCPropertyGridProperty)


/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd

CPropertiesWnd::CPropertiesWnd()
{
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
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
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertiesWnd message handlers

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyCmb = 0;
	int cyTlb = 0;

	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyCmb + cyTlb, rectClient.Width(), rectClient.Height() - (cyCmb + cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create combo:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, IDR_PROPERTIES_GRID))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* pCmdUI)
{
}

void CPropertiesWnd::OnSortProperties()
{
	m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1()
{
	// TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

	CMFCPropertyGridProperty* pGroup0 = new CMFCPropertyGridProperty(
		_T("Currently displayed version info (click to change version number)"));
	CMFCPropertyGridProperty* pPropertyGridPropertyT = new CMFCPropertyGridProperty(
		_T("Current version"), COleVariant((long)0, VT_I4),
		_T("The currently displayed file version (Ctrl+Scroll wheel to change)"));
	pPropertyGridPropertyT->EnableSpinControl(TRUE, 0, 0);
	pPropertyGridPropertyT->Enable(FALSE);
	pGroup0->AddSubItem(pPropertyGridPropertyT);

	pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
		_T("Changelist"), (_variant_t)_T(""), _T("The changelist"));
	pPropertyGridPropertyT->AllowEdit(FALSE);
	pGroup0->AddSubItem(pPropertyGridPropertyT);

	pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
		_T("Date"), (_variant_t)_T(""), _T("The modification date and time of the changelist"));
	pPropertyGridPropertyT->AllowEdit(FALSE);
	pGroup0->AddSubItem(pPropertyGridPropertyT);

	pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
		_T("Author"), (_variant_t)_T(""), _T("The author of the changelist"));
	pPropertyGridPropertyT->AllowEdit(FALSE);
	pGroup0->AddSubItem(pPropertyGridPropertyT);

	pPropertyGridPropertyT = new CMFCPropertyGridColorProperty(
		_T("Color"), RGB(0xFF, 0xFF, 0xFF));
	pPropertyGridPropertyT->AllowEdit(FALSE);
	pPropertyGridPropertyT->Enable(FALSE);
	pGroup0->AddSubItem(pPropertyGridPropertyT);

	m_wndPropList.AddProperty(pGroup0);
}

/*static*/ void CPropertiesWnd::EnsureItems(CMFCPropertyGridCtrl& wndPropList, int cItem)
{
	CMFCPropertyGridProperty* pProp;
	while (wndPropList.GetPropertyCount() > cItem + 1 &&
		(pProp = wndPropList.GetProperty(cItem + 1)) != NULL)
	{
		// MFC has an annoying bug where if the selection is in a subitem
		// of an property that is deleted, it will crash.  The workaround
		// is to remove selection if the subitem is a child of the property
		CMFCPropertyGridProperty* pSel = wndPropList.GetCurSel();
		if (pSel != NULL)
		{
			while ((pSel = pSel->GetParent()) != NULL)
			{
				if (pSel == pProp)
				{
					wndPropList.SetCurSel(NULL);
					break;
				}
			}
		}
		VERIFY(wndPropList.DeleteProperty(pProp));
		assert(pProp == NULL);
	}

	for (int i = wndPropList.GetPropertyCount() - 1; i < cItem; ++i)
	{
		CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Selected change description"));

		CMFCPropertyGridProperty* pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
			_T("Version"), (_variant_t)_T(""), _T("The version that is currently selected"));
		pPropertyGridPropertyT->AllowEdit(FALSE);
		pGroup1->AddSubItem(pPropertyGridPropertyT);

		pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
			_T("Changelist"), (_variant_t)_T(""), _T("The changelist that is currently selected"));
		pPropertyGridPropertyT->AllowEdit(FALSE);
		pGroup1->AddSubItem(pPropertyGridPropertyT);

		pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
			_T("Date"), (_variant_t)_T(""), _T("The modification date and time of the change that is currently selected"));
		pPropertyGridPropertyT->AllowEdit(FALSE);
		pGroup1->AddSubItem(pPropertyGridPropertyT);

		pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
			_T("Author"), (_variant_t)_T(""), _T("The author of the change that is currently selected"));
		pPropertyGridPropertyT->AllowEdit(FALSE);
		pGroup1->AddSubItem(pPropertyGridPropertyT);

		pPropertyGridPropertyT = new CMFCPropertyGridColorProperty(
			_T("Color"), RGB(0xFF, 0xFF, 0xFF));
		pPropertyGridPropertyT->AllowEdit(FALSE);
		pPropertyGridPropertyT->Enable(FALSE);
		pGroup1->AddSubItem(pPropertyGridPropertyT);

		wndPropList.AddProperty(pGroup1);
	}
}

void CPropertiesWnd::UpdateGridBlock(const DIFFRECORD* pdrVer,
	CMFCPropertyGridProperty* pPropVersionHeader,
	int nVerMax, bool fUpdateVersionControl)
{
	CMFCPropertyGridProperty* pPropVersionVersion = pPropVersionHeader->GetSubItem(0);
	ASSERT_VALID(pPropVersionVersion);
	if (fUpdateVersionControl && pPropVersionVersion != NULL)
	{
		WCHAR wzVer[32];
		_itow_s(pdrVer->nVer, wzVer, _countof(wzVer), 10);
		_variant_t varWzNversT(wzVer);
		if (!(pPropVersionVersion->GetValue() == varWzNversT))
			pPropVersionVersion->SetValue(varWzNversT);
	}

	WCHAR wzChangeList[32];
	if (pdrVer != NULL)
		_itow_s(pdrVer->nCL, wzChangeList, _countof(wzChangeList), 10);
	else
		wzChangeList[0] = '\0';
	CMFCPropertyGridProperty* pPropVersionCL = pPropVersionHeader->GetSubItem(1);
	ASSERT_VALID(pPropVersionCL);
	if (pPropVersionCL != NULL)
	{
		_variant_t varWzChangeListT(wzChangeList);
		if (!(pPropVersionCL->GetValue() == varWzChangeListT))
			pPropVersionCL->SetValue(varWzChangeListT);
	}

	WCHAR wzDateTime[32];
	if (pdrVer != NULL)
		_tasctime_s(wzDateTime, _countof(wzDateTime), &pdrVer->tm);
	else
		wzDateTime[0] = '\0';
	CMFCPropertyGridProperty* pPropVersionDateTime = pPropVersionHeader->GetSubItem(2);
	ASSERT_VALID(pPropVersionDateTime);
	if (pPropVersionDateTime != NULL)
	{
		_variant_t varWzDateTimeT(wzDateTime);
		if (!(pPropVersionDateTime->GetValue() == varWzDateTimeT))
			pPropVersionDateTime->SetValue(varWzDateTimeT);
	}

	CMFCPropertyGridProperty* pPropVersionAuthor = pPropVersionHeader->GetSubItem(3);
	ASSERT_VALID(pPropVersionAuthor);
	if (pPropVersionAuthor != NULL)
	{
		_variant_t varWzAuthorT(pdrVer != NULL ? pdrVer->szAuthor : _T(""));
		if (!(pPropVersionAuthor->GetValue() == varWzAuthorT))
			pPropVersionAuthor->SetValue(varWzAuthorT);
	}

	CMFCPropertyGridProperty* pPropVersionColorT = pPropVersionHeader->GetSubItem(4);
	CMFCPropertyGridColorProperty* pPropVersionColor = static_cast<CMFCPropertyGridColorProperty*>(pPropVersionColorT);
	ASSERT_VALID(pPropVersionColor);
	if (pPropVersionColor != NULL)
	{
		COLORREF crVersion(pdrVer != NULL ? CSimpleAgeViewerView::CrBackgroundForVersion(pdrVer->nVer, nVerMax)
			: RGB(0xFF, 0xFF, 0xFF));
		if (!(pPropVersionColor->GetColor() == crVersion))
			pPropVersionColor->SetColor(crVersion);
	}
}



void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}

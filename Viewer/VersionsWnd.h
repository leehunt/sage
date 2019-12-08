
#pragma once
#include "../Viewer/SimpleAgeViewerDocListener.h"

class DIFFRECORD;
class CSimpleAgeViewerDoc;
class MyCToolTipCtrl;

class CVersionsWnd : public CDockablePane, public CSimpleAgeViewerDocListener
{
	// Construction
public:
	CVersionsWnd();

	void AdjustLayout();

	// Attributes
public:
	void SetVSDotNetLook(BOOL bSet)
	{
		//m_wndPropList.SetVSDotNetLook(bSet);
		//m_wndPropList.SetGroupNameFullWidth(bSet);
	}

protected:
	CFont m_fntVersionsList;
	CTreeCtrl m_wndTreeCtrl;
	CImageList m_imageList;
	CSimpleAgeViewerDoc* m_pDoc;  // not ref counted
	CToolTipCtrl* m_pToolTipControl;

	virtual void DocEditNotification(int iLine, int cLine);
	virtual void DocVersionChangedNotification(int nVer);

	// Implementation
public:
	virtual ~CVersionsWnd();

	static bool FEnsureTreeItemsAndSelection(CTreeCtrl& tree, HTREEITEM htreeitemRoot,
		const DIFFRECORD* pdrEnd, int nCLSelection, const CString& cstrDepotFilePath);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnProperties1();
	afx_msg void OnUpdateProperties1(CCmdUI* pCmdUI);
	afx_msg void OnProperties2();
	afx_msg void OnUpdateProperties2(CCmdUI* pCmdUI);
	afx_msg void OnTreeNotifyExpanding(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

	DECLARE_MESSAGE_MAP()

	void InitPropList();

	void SetPropListFont();
};


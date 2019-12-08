
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "PropertiesWnd.h"
#include "VersionsWnd.h"

class CMainFrame : public CFrameWndEx
{

protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:

	// Operations
public:
	CWnd& GetStatusWnd() { return m_wndStatusBar; }

	// Overrides
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual void OnActivateView(BOOL bActivate, CView* pActivateView,
		CView* pDeactiveView);

	// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnGotoDiff(bool fCurVer, bool fBackwards);

protected:  // control bar embedded members
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCToolBar       m_wndToolBarFind;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CPropertiesWnd    m_wndProperties;
	CVersionsWnd      m_wndVersions;

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnFindMenuCommand();
	afx_msg void OnFindEdit(CCmdUI* pCmdUI);
	afx_msg void OnFindCommand(CCmdUI* pCmdUI);
	afx_msg void OnFindCommand();
	afx_msg void OnFindCommandBackward();
	afx_msg void OnFindCaseSensitive(CCmdUI* pCmdUI);
	afx_msg void OnFindCaseSensitive();
	afx_msg void OnFindHighlightAll(CCmdUI* pCmdUI);
	afx_msg void OnFindHighlightAll();
	afx_msg void OnGotoDiff(CCmdUI* pCmdUI);
	afx_msg void OnGotoNextCurverDiff();
	afx_msg void OnGotoPrevCurverDiff();
	afx_msg void OnGotoNextDiff();
	afx_msg void OnGotoPrevDiff();
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg LRESULT OnToolbarReset(WPARAM wp, LPARAM lp);

	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
};




// SimpleAgeViewerView.h : interface of the CSimpleAgeViewerView class
//


#pragma once
#include "../Viewer/SimpleAgeViewerDocListener.h"

class CSimpleAgeViewerView : public CScrollView, public CSimpleAgeViewerDocListener
{
protected: // create from serialization only
	CSimpleAgeViewerView();
	DECLARE_DYNCREATE(CSimpleAgeViewerView)

	// Attributes
public:
	CSimpleAgeViewerDoc* GetDocument() const;

	// Operations
public:
	void FindAndSelectString(LPCTSTR szFind, bool fScroll, bool fBackward = false, bool* pfWraparound = NULL);
	void FindAndSelectVersion(bool fCurVer, bool fBackwards = false, bool* pfWraparound = NULL);

	void SetCaseSensitive(bool fCaseSensitive) { m_fCaseSensitive = fCaseSensitive; }
	bool GetCaseSensitive() const { return m_fCaseSensitive; }

	void SetHighlightAll(bool fHighlightAll) { m_fHighlightAll = fHighlightAll; }
	bool GetHighlightAll() const { return m_fHighlightAll; }

	static COLORREF CrBackgroundForVersion(int nVer, int nVerMax);

protected:
	// Overrides
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll);

	// Implementation
	void SetCustomFont();
	void UpdateScrollSizes(int dx);

	virtual void DocEditNotification(int iLine, int cLine);  // REVIEW: virtual?
	virtual void DocVersionChangedNotification(int nVer);  // REVIEW: virtual?

public:
	virtual ~CSimpleAgeViewerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void ScrollToLine(int iLine, float flScaleFromTop);

protected:
	CSize m_sizChar;
	CSize m_sizPage;
	CSize m_sizAll;

	int m_iSelStart;
	int m_iSelEnd;

	bool m_fLeftMouseDown;
	int m_iMouseDown;

	CString m_strSearchLast;
	CPoint m_ptSearchLast;
	bool m_fCaseSensitive;
	bool m_fHighlightAll;

	CSimpleAgeViewerDoc* m_pDocListened;

	CFont m_font;

	FILEREP* m_pfilerepLast;

	// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	afx_msg void OnEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();

	afx_msg void OnGotoDlg();  // REVIEW: place this in MainFrm.h

	afx_msg void OnJumpToChangeVersion(CCmdUI* pCmdUI);
	afx_msg void OnJumpToChangeVersion();

	afx_msg void OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI);

	afx_msg void OnUpdateVersionsTree(CCmdUI* pCmdUI);

	afx_msg void OnUpdateStatusLineNumber(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SimpleAgeViewerView.cpp
inline CSimpleAgeViewerDoc* CSimpleAgeViewerView::GetDocument() const
{
	return reinterpret_cast<CSimpleAgeViewerDoc*>(m_pDocument);
}
#endif

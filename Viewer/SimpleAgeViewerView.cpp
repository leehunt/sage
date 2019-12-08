
// SimpleAgeViewerView.cpp : implementation of the CSimpleAgeViewerView class
//

#include "stdafx.h"
#include "SimpleAgeViewer.h"

#include "SimpleAgeViewerDoc.h"  // REVIEW: this is icky (necc for SimpleAgeViewerView.h)
#include "SimpleAgeViewerView.h"
#include "PropertiesWnd.h"  // CPropertiesWnd::EnsureItems()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const int c_dxIndent = 2;

// CSimpleAgeViewerView

IMPLEMENT_DYNCREATE(CSimpleAgeViewerView, CScrollView)

BEGIN_MESSAGE_MAP(CSimpleAgeViewerView, CScrollView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(IDR_PROPERTIES_GRID, &CSimpleAgeViewerView::OnUpdatePropertiesPaneGrid)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CSimpleAgeViewerView::OnUpdateStatusLineNumber)
	ON_COMMAND(ID_EDIT_GOTOLINE_DLG, &CSimpleAgeViewerView::OnGotoDlg)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CSimpleAgeViewerView::OnEditCopy)
	ON_COMMAND(ID_EDIT_COPY, &CSimpleAgeViewerView::OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_JUMP_TO_CHANGE_VERSION, &CSimpleAgeViewerView::OnJumpToChangeVersion)
	ON_COMMAND(ID_EDIT_JUMP_TO_CHANGE_VERSION, &CSimpleAgeViewerView::OnJumpToChangeVersion)
END_MESSAGE_MAP()

// CSimpleAgeViewerView construction/destruction

CSimpleAgeViewerView::CSimpleAgeViewerView() : m_iSelStart(-1), m_iSelEnd(-1), m_fLeftMouseDown(false),
m_ptSearchLast(-1, 0), m_fCaseSensitive(false), m_fHighlightAll(true),
m_pDocListened(NULL), m_pfilerepLast(NULL)
{
	m_sizChar.cx = 0;
}

CSimpleAgeViewerView::~CSimpleAgeViewerView()
{
	if (m_pDocListened != NULL)
	{
		m_pDocListened->RemoveDocListener(*this);
		m_pDocListened = NULL;
	}
}

BOOL CSimpleAgeViewerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CSimpleAgeViewerView drawing

void CSimpleAgeViewerView::UpdateScrollSizes(int cy)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	CDC* pDC = GetDC();
	if (pDC != NULL)
	{
		if (!m_sizChar.cx)
			m_sizChar = pDC->GetTextExtent(_T("W"), 1);

		m_sizAll = m_sizChar;
		m_sizAll.cx *= 256;
		m_sizAll.cy *= pDoc->PfilerepRead() != NULL ? pDoc->PfilerepRead()->CLine() : 0;

		m_sizPage.cx = m_sizChar.cx;
		m_sizPage.cy = cy / m_sizChar.cy * m_sizChar.cy;

		SetScrollSizes(MM_TEXT, m_sizAll, m_sizPage, m_sizChar);

		ReleaseDC(pDC);
	}
}

void CSimpleAgeViewerView::OnInitialUpdate() // called first time after construct
{
	__super::OnInitialUpdate();

	m_iSelStart = m_iSelEnd = -1;

	m_ptSearchLast.SetPoint(-1, 0);
	m_strSearchLast.Empty();

	SetCustomFont();

	RECT rcClient;
	GetClientRect(&rcClient);
	UpdateScrollSizes(rcClient.bottom);
}

void CSimpleAgeViewerView::SetCustomFont()
{
	CDC* pdc = GetDC();
	assert(pdc != NULL);
	if (pdc != NULL)
	{
		CFont* pfnt = pdc->GetCurrentFont();
		assert(pfnt != NULL);
		if (pfnt != NULL)
		{
			LOGFONT lf = {};
			pfnt->GetLogFont(&lf);

			_tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), _T("Courier New"));

			HFONT hfont = static_cast<HFONT>(m_font.Detach());
			if (hfont != NULL)
				::DeleteObject(hfont);
			m_font.CreateFontIndirect(&lf);

			SetFont(&m_font);
		}
	}
}

BOOL CSimpleAgeViewerView::OnEraseBkgnd(CDC* pDC)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL || pDoc->PfilerepRead() == NULL || pDoc->PfilerepRead()->CLine() == 0)
		return __super::OnEraseBkgnd(pDC);

	return TRUE;  // we will erase when drawing
}


const COLORREF c_rgclrrefAge[] =
{
	RGB(0xFF, 0xFF, 0xFF),  // no change (ver 1)

//	RGB(0xF7, 0xFF, 0xF7),  // not visible
//	RGB(0xEF, 0xFF, 0xEF),
//	RGB(0xE7, 0xFF, 0xE7),
	RGB(0xDF, 0xFF, 0xDF),
	RGB(0xD7, 0xFF, 0xD7),
	RGB(0xCF, 0xFF, 0xCF),
	RGB(0xC7, 0xFF, 0xC7),
	RGB(0xBF, 0xFF, 0xBF),
	RGB(0xB7, 0xFF, 0xB7),
	RGB(0xAF, 0xFF, 0xAF),
	RGB(0xA7, 0xFF, 0xA7),
	RGB(0x9F, 0xFF, 0x9F),
	RGB(0x97, 0xFF, 0x97),
	RGB(0x8F, 0xFF, 0x8F),
	RGB(0x87, 0xFF, 0x87),
	RGB(0x7F, 0xFF, 0x7F),
	RGB(0x77, 0xFF, 0x77),
	RGB(0x6F, 0xFF, 0x6F),
	RGB(0x67, 0xFF, 0x67),
	RGB(0x5F, 0xFF, 0x5F),
	RGB(0x57, 0xFF, 0x57),
	RGB(0x4F, 0xFF, 0x4F),
	RGB(0x47, 0xFF, 0x47),
	RGB(0x3F, 0xFF, 0x3F),
	RGB(0x37, 0xFF, 0x37),
	RGB(0x2F, 0xFF, 0x2F),
	RGB(0x27, 0xFF, 0x27),
	RGB(0x1F, 0xFF, 0x1F),
	RGB(0x17, 0xFF, 0x17),
	RGB(0x0F, 0xFF, 0x0F),
	RGB(0x07, 0xFF, 0x07),
	RGB(0x00, 0xFF, 0x00),
	RGB(0x00, 0xF0, 0x00),
	RGB(0x00, 0xE0, 0x00),
	RGB(0x00, 0xD0, 0x00),
	RGB(0x00, 0xC0, 0x00),
	RGB(0x00, 0xB0, 0x00),
	RGB(0x00, 0xA0, 0x00),
	RGB(0x00, 0x90, 0x00),
	RGB(0x00, 0x80, 0x00),
};

/*static*/ COLORREF CSimpleAgeViewerView::CrBackgroundForVersion(int nVer, int nVerMax)
{
	// set colors
	assert(nVerMax >= nVer);

	int iColor = 0;  // no change color
	if (nVer > 1)
	{
		iColor = _countof(c_rgclrrefAge) - (nVerMax - nVer) - 1; // -1 to ignore the zeroth no-change color

		// check that changes outside of the version color table always be iColor 1 (the oldest change color)
		if (iColor <= 0)
			iColor = 1;
	}
	assert(0 <= iColor && iColor < _countof(c_rgclrrefAge));
	return c_rgclrrefAge[iColor];
}

static int IFindNoCase(CString cstrFind, CString cstrSub)
{
	const TCHAR* pchFind = FILEREP::_tcsistr(cstrFind, cstrSub);
	if (pchFind == NULL)
		return -1;
	return pchFind - cstrFind;
}

void CSimpleAgeViewerView::OnDraw(CDC* pDC)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	if (m_pDocListened != pDoc)
	{
		if (m_pDocListened != NULL)
			m_pDocListened->RemoveDocListener(*this);
		pDoc->AddDocListener(*this);

		m_pDocListened = pDoc;
	}

	RECT rcClient;
	GetClientRect(&rcClient);

	CFont* pfontOld = pDC->SelectObject(&m_font);

	const int iSearchOffset = GetHighlightAll() ? 0 : m_ptSearchLast.x;
	const FILEREP* pfilerep = pDoc->PfilerepRead();

	const CPoint cptViewport = pDC->GetViewportOrg();
	CPoint cptOld = cptViewport;
	if (m_pfilerepLast != pfilerep)
	{
		if (m_pfilerepLast != NULL)
			m_pfilerepLast->SetViewOrg(cptViewport);
		if (pfilerep != NULL)
		{
			// recalc for possible updated pDoc->FilerepRead().CLine()
			UpdateScrollSizes(rcClient.bottom);

			ScrollToPosition(pfilerep->GetViewOrg());

			pDC->SetViewportOrg(-GetDeviceScrollPosition());
			const_cast<CPoint&>(cptViewport) = pDC->GetViewportOrg();
		}

		m_pfilerepLast = const_cast<FILEREP*>(pfilerep);
	}

	const int yScrollPos = -cptViewport.y;
	assert(yScrollPos == GetDeviceScrollPosition().y);
	const int nVerCur = pfilerep != NULL ? pfilerep->NVer() : 0;
	const int cLine = pfilerep != NULL ? pfilerep->CLine() : 0;
	const int cchFind = m_strSearchLast.GetLength();
	for (int y = yScrollPos / m_sizChar.cy * m_sizChar.cy; y < rcClient.bottom + yScrollPos; y += m_sizChar.cy)
	{
		int i = y / m_sizChar.cy;

		assert(i < cLine || cLine == 0);
		if (i >= cLine)
			break;

		// set colors
		int nVer = NverLast(i, *pfilerep);
		COLORREF crBack = CrBackgroundForVersion(nVer, nVerCur);
		COLORREF crFore = RGB(0x00, 0x00, 0x00);
		if (m_iSelStart <= i && i <= m_iSelEnd)
		{
			crBack = ~crBack & 0x00FFFFFF;
			crFore = ~crFore & 0x00FFFFFF;
		}
		pDC->SetBkColor(crBack);
		if (crFore == crBack)  // REVIEW: change this check to catch visually similar colors
			pDC->SetTextColor(~crFore & 0x00FFFFFF);
		else
			pDC->SetTextColor(crFore);

		RECT rcDrawT = { 0, y, rcClient.right - (cptViewport.x + 0), m_sizChar.cy + y, };
		CString cstrTabbed(pfilerep->GetLine(i));
		int iFind;
		if (m_strSearchLast.IsEmpty() ||
			m_ptSearchLast.y != i && !GetHighlightAll() ||
			iSearchOffset < 0 || iSearchOffset >= cstrTabbed.GetLength() ||
			(iFind = GetCaseSensitive() ? cstrTabbed.Find(m_strSearchLast, iSearchOffset) - iSearchOffset :
				IFindNoCase(cstrTabbed.GetBuffer() + iSearchOffset, m_strSearchLast)) < 0)
		{
			cstrTabbed.Replace(_T("\t"), _T("    "));  // REVIEW: this could be pre-processed once
			pDC->ExtTextOut(rcDrawT.left + c_dxIndent, rcDrawT.top,
				ETO_OPAQUE, &rcDrawT,
				cstrTabbed,
				NULL);
		}
		else
			// draw any search highlight
		{
			iFind += iSearchOffset;
			// replace tabs with 4 spaces and update iFind
			int iTab = 0;
			while ((iTab = cstrTabbed.Find(_T("\t"), iTab)) >= 0)
			{
				cstrTabbed.Delete(iTab);
				cstrTabbed.Insert(iTab, _T("    "));
				if (iFind > iTab)
					iFind += 3;
				iTab += 4;
			}

			UINT uAlignPrev = pDC->SetTextAlign(TA_UPDATECP);
			pDC->MoveTo(c_dxIndent, y);
			do
			{
				CString cstrSubPrefix((LPCTSTR)cstrTabbed, iFind);
				CString cstrSubFound((LPCTSTR)cstrTabbed + iFind, cchFind);

				POINT ptPrev = pDC->GetCurrentPosition();
				pDC->TextOut(0, 0, cstrSubPrefix);
				POINT ptCur = pDC->GetCurrentPosition();

				crBack = ~crBack & 0x00FFFFFF;
				crFore = ~crFore & 0x00FFFFFF;
				pDC->SetBkColor(crBack);
				pDC->SetTextColor(crFore);

				pDC->TextOut(0, 0, cstrSubFound);

				crBack = ~crBack & 0x00FFFFFF;
				crFore = ~crFore & 0x00FFFFFF;
				pDC->SetBkColor(crBack);
				pDC->SetTextColor(crFore);

				cstrTabbed.Delete(0, iFind + cchFind);

				if (!GetHighlightAll())
					break;
				iFind = cstrTabbed.Find(m_strSearchLast);
			} while (iFind >= 0);

			POINT ptCur = pDC->GetCurrentPosition();
			rcDrawT.left = ptCur.x;
			pDC->ExtTextOut(0, 0,  // NOTE: drawing relative via TA_UPDATECP
				ETO_OPAQUE, &rcDrawT,
				cstrTabbed,
				NULL);

			pDC->SetTextAlign(uAlignPrev);
		}

	}

	pDC->SelectObject(pfontOld);
}


void CSimpleAgeViewerView::DocEditNotification(int iLine, int cLine)
{
	POINT ptCur = GetDeviceScrollPosition();

	SetRedraw(FALSE);

	// recalc for updated pDoc->FilerepRead().CLine()
	RECT rcClient;
	GetClientRect(&rcClient);
	UpdateScrollSizes(rcClient.bottom);

	int iLineTop = ptCur.y / m_sizChar.cy;
	int cLineOffset = m_iSelStart - iLineTop;
	if (m_iSelStart >= iLine)
	{
		if (cLine < 0 && m_iSelStart < iLine - cLine)
			m_iSelStart = iLine;
		else
			m_iSelStart += cLine;
		assert(m_iSelStart >= 0);
	}
	if (m_iSelEnd >= iLine)
	{
		if (cLine < 0 && m_iSelEnd < iLine - cLine)
			m_iSelEnd = iLine;
		else
			m_iSelEnd += cLine;
		assert(m_iSelEnd >= 0);
	}

	if (m_ptSearchLast.y >= iLine)
	{
		if (cLine < 0 && m_ptSearchLast.y < iLine - cLine)
			m_ptSearchLast.y = iLine;
		else
			m_ptSearchLast.y += cLine;
		assert(m_ptSearchLast.y >= 0);
	}

	if (iLineTop >= iLine)
	{
		if (cLine < 0 && iLineTop < iLine - cLine)
			iLineTop = iLine;
		else
			iLineTop += cLine;
		assert(iLineTop >= 0);
	}

	int iLineBottom = (ptCur.y + rcClient.bottom) / m_sizChar.cy;
	if (iLineBottom >= iLine)
	{
		if (cLine < 0 && iLineBottom < iLine - cLine)
			iLineBottom = iLine;
		else
			iLineBottom += cLine;
		assert(iLineBottom >= 0);
	}

	int iLineScrollAround = iLineTop <= m_iSelStart && m_iSelStart < iLineBottom ? m_iSelStart : iLineTop;

	if (iLineScrollAround >= iLine)
	{
		if (iLineScrollAround > iLineTop)
			ptCur.y = (iLineScrollAround - (cLineOffset)) * m_sizChar.cy;
		else
			ptCur.y += cLine * m_sizChar.cy;

		// momentarially turn off the visible bit to keep ScrollToPosition from using windows scrolling
		LONG lWindowStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
		::SetWindowLong(m_hWnd, GWL_STYLE, lWindowStyle & ~WS_VISIBLE);
		ScrollToPosition(ptCur);
		::SetWindowLong(m_hWnd, GWL_STYLE, lWindowStyle);
	}

	SetRedraw(TRUE);
}

void CSimpleAgeViewerView::DocVersionChangedNotification(int nVer)
{
	if (nVer == 0)
		m_pfilerepLast = NULL;
}


// CSimpleAgeViewerView message handlers

void CSimpleAgeViewerView::OnMouseMove(UINT nFlags, CPoint point)
{
	__super::OnMouseMove(nFlags, point);

	if (m_fLeftMouseDown)
	{
		CSimpleAgeViewerDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (pDoc == NULL)
			return;

		CPoint ptScroll = GetDeviceScrollPosition();
		int yMouse = ptScroll.y + point.y;
		int i = yMouse / m_sizChar.cy;
		int cLine = pDoc->PfilerepRead() != NULL ? pDoc->PfilerepRead()->CLine() : 0;
		if (i >= cLine)
			i = cLine - 1;
		if (i < 0)
			i = 0;

		if (i < m_iSelStart)
		{
			m_iSelStart = i;
			m_ptSearchLast.x = -1;
			m_ptSearchLast.y = i;
			Invalidate(FALSE/*bErase*/);
		}
		else if (i > m_iSelEnd)
		{
			m_iSelEnd = i;
			Invalidate(FALSE/*bErase*/);
		}
		// shrink the range if requested
		else if (m_iSelStart <= i && i <= m_iSelEnd && !(nFlags & MK_SHIFT))
		{
			if (m_iSelStart == m_iMouseDown)
			{
				if (m_iSelEnd > i)
				{
					m_iSelEnd = i;
					Invalidate(FALSE/*bErase*/);
				}
			}
			else
			{
				//assert(m_iSelEnd == m_iMouseDown);
				if (m_iSelStart < i)
				{
					m_iSelStart = i;
					m_ptSearchLast.x = -1;
					m_ptSearchLast.y = i;
					Invalidate(FALSE/*bErase*/);
				}
			}
		}

		// scrolling
		RECT rcClient;
		GetClientRect(&rcClient);
		CPoint ptScrollNew = ptScroll;
		if (point.y > rcClient.bottom)
			ptScrollNew.y += point.y - rcClient.bottom;
		else if (point.y < rcClient.top)
			ptScrollNew.y += point.y + rcClient.top;

		if (point.x > rcClient.right)
			ptScrollNew.x += point.x - rcClient.right;
		else if (point.x < rcClient.left)
			ptScrollNew.x += point.x + rcClient.left;

		if (ptScroll != ptScrollNew)
			ScrollToPosition(ptScrollNew);
	}
	else
		ReleaseCapture();  // just in case
}

void CSimpleAgeViewerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	__super::OnLButtonDown(nFlags, point);
	if (!m_fLeftMouseDown)
	{
		CSimpleAgeViewerDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (pDoc == NULL)
			return;

		int yMouse = GetDeviceScrollPosition().y + point.y;
		int i = yMouse / m_sizChar.cy;
		if (i < 0)
			return;
		int cLine = pDoc->PfilerepRead() != NULL ? pDoc->PfilerepRead()->CLine() : 0;
		if (i >= cLine)
			return;  // clicked outside of content

		m_fLeftMouseDown = true;
		SetCapture();
		if (m_iSelStart != i || m_iSelEnd != i || m_iMouseDown != i)
		{
			if (m_iSelStart != -1 && (nFlags & MK_SHIFT))
			{
				if (i > m_iSelStart)
					m_iSelEnd = i;
				else
				{
					m_iSelEnd = m_iSelStart;
					m_iSelStart = i;
				}
			}
			else
			{
				m_iSelStart = m_iSelEnd = i;
				m_ptSearchLast.x = -1;
				m_ptSearchLast.y = i;
			}
			m_iMouseDown = i;
			Invalidate(FALSE/*bErase*/);
		}
	}
}
void CSimpleAgeViewerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	__super::OnLButtonUp(nFlags, point);
	if (m_fLeftMouseDown)
	{
		m_fLeftMouseDown = false;
		::ReleaseCapture();
	}
}

void CSimpleAgeViewerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CSimpleAgeViewerView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}

void CSimpleAgeViewerView::OnSize(UINT nType, int cx, int cy)
{
	UpdateScrollSizes(cy);
}

void CSimpleAgeViewerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
		// dy
	case VK_HOME:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_TOP), 0, TRUE/*bDoScroll*/);
		break;
	case VK_END:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_BOTTOM), 0, TRUE/*bDoScroll*/);
		break;
	case VK_DOWN:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_LINEDOWN), 0, TRUE/*bDoScroll*/);
		break;
	case VK_UP:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_LINEUP), 0, TRUE/*bDoScroll*/);
		break;
	case VK_PRIOR:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_PAGEUP), 0, TRUE/*bDoScroll*/);
		break;
	case VK_NEXT:
		OnScroll(MAKEWORD(SB_ENDSCROLL, SB_PAGEDOWN), 0, TRUE/*bDoScroll*/);
		break;

		// dx
	case VK_LEFT:
		if (::GetAsyncKeyState(VK_CONTROL) < 0)
			OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL), GetScrollPos(SB_HORZ) - m_sizAll.cx, TRUE/*bDoScroll*/);
		else
			OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL), GetScrollPos(SB_HORZ) - m_sizChar.cx, TRUE/*bDoScroll*/);
		break;
	case VK_RIGHT:
		if (::GetAsyncKeyState(VK_CONTROL) < 0)
			OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL), GetScrollPos(SB_HORZ) + m_sizAll.cx, TRUE/*bDoScroll*/);
		else
			OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL), GetScrollPos(SB_HORZ) + m_sizChar.cx, TRUE/*bDoScroll*/);
		break;
	}
}

BOOL CSimpleAgeViewerView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// This sucks.  nPos is derived from the upper half of WM_VSCROLL/WM_HSCROLL's wParam
	// which is a short.  To support more that 65536 scrollpos, fix the value here
	WORD wScroll = LOWORD(nScrollCode);
	if (LOBYTE(wScroll) == 0xFF)
		/* fVert */
	{
		switch (HIBYTE(nScrollCode))
		{
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
		{
			SCROLLINFO si = { sizeof(si), SIF_TRACKPOS, };
			CScrollBar* pScrollBar = GetScrollBarCtrl(SB_VERT);
			assert(pScrollBar != NULL);
			if (pScrollBar != NULL)
			{
				pScrollBar->GetScrollInfo(&si, SIF_TRACKPOS);
				nPos = si.nTrackPos;
			}
		}
		break;
		}
	}
	else if (HIBYTE(wScroll) == 0xFF)
		/* fHoriz */
	{
		switch (LOBYTE(wScroll))
		{
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
		{
			SCROLLINFO si = { sizeof(si), SIF_TRACKPOS, };
			CScrollBar* pScrollBar = GetScrollBarCtrl(SB_HORZ);
			assert(pScrollBar != NULL);
			if (pScrollBar != NULL)
			{
				pScrollBar->GetScrollInfo(&si, SIF_TRACKPOS);
				nPos = si.nTrackPos;
			}
		}
		break;
		}
	}

	return __super::OnScroll(nScrollCode, nPos, bDoScroll);
}

BOOL CSimpleAgeViewerView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	if (bDoScroll)
	{
		// make scroll actions >= a line height always start drawing at the top of a line
		if (sizeScroll.cy >= m_sizChar.cy)
		{
			int y = GetScrollPos(SB_VERT);

			int dyOverage = sizeScroll.cy + y - (sizeScroll.cy + y) / m_sizChar.cy * m_sizChar.cy;
			sizeScroll.cy -= dyOverage;
		}
		else if (sizeScroll.cy <= -m_sizChar.cy)
		{
			int y = GetScrollPos(SB_VERT);

			int dyOverage = sizeScroll.cy + y - (sizeScroll.cy + y) / m_sizChar.cy * m_sizChar.cy;
			if (dyOverage)
				sizeScroll.cy += m_sizChar.cy - dyOverage;
		}

		// Turn Ctrl+scroll wheel into a version change
		if (::GetKeyState(VK_CONTROL) < 0)
		{
			CSimpleAgeViewerDoc* pDoc = GetDocument();
			ASSERT_VALID(pDoc);
			if (pDoc != NULL)
			{
				const FILEREP* pfr = pDoc->PfilerepRead();
				if (pfr != NULL)
				{
					if (sizeScroll.cy < 0)
					{
						if (pDoc->FEditToFileVersion(pfr->NVer() + 1, pDoc->Filerep()))
							pDoc->UpdateAllViews(NULL);  // NULL - also update this view
					}
					else if (sizeScroll.cy > 0)
					{
						if (pDoc->FEditToFileVersion(pfr->NVer() - 1, pDoc->Filerep()))
							pDoc->UpdateAllViews(NULL);  // NULL - also update this view
					}
					return TRUE;
				}
			}
		}

	}

	return __super::OnScrollBy(sizeScroll, bDoScroll);
}

void CSimpleAgeViewerView::OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI)
{
	// Update selection-based properties (e.g. currently displayed change)
	CWnd* pWndT = CWnd::FromHandlePermanent(*pCmdUI->m_pOther);
	ASSERT_VALID(pWndT);

	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	CMFCPropertyGridCtrl* pGrid = static_cast<CMFCPropertyGridCtrl*>(pWndT);
	ASSERT_VALID(pGrid);
	if (pGrid == NULL)
		return;

	pCmdUI->m_bContinueRouting = TRUE;  // ensure that we route to doc

	if (m_iSelStart <= -1)
	{
		CPropertiesWnd::EnsureItems(*pGrid, 0/*cItem*/);
		return;
	}

	CUIntArray aryVersion;
	assert(m_iSelEnd != -1);
	if (pDoc->PfilerepRead() != NULL)
		GetVersionsFromLines(m_iSelStart, m_iSelEnd - m_iSelStart + 1, *pDoc->PfilerepRead(), aryVersion);

	CPropertiesWnd::EnsureItems(*pGrid, aryVersion.GetCount());

	for (int iVers = 0; iVers < aryVersion.GetCount(); ++iVers)
	{
		CMFCPropertyGridProperty* pPropVersionHeader = pGrid->GetProperty(1 + iVers);
		ASSERT_VALID(pPropVersionHeader);
		if (pPropVersionHeader == NULL)
			return;
		int nVerSel = aryVersion.GetAt(iVers);

		const DIFFRECORD* pdrVer = nVerSel != -1 ? PdrVerFromVersion(nVerSel, pDoc->PdrListRead()) : NULL;

		CPropertiesWnd::UpdateGridBlock(pdrVer, pPropVersionHeader, NVerMaxFromPdr(pdrVer), true/*fUpdateVersionConttrol*/);
	}
}

void CSimpleAgeViewerView::OnUpdateStatusLineNumber(CCmdUI* pCmdUI)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	pCmdUI->Enable(pDoc->PfilerepRead() != NULL);

	CString strLine;
	if (m_iSelStart != -1)
	{
		if (m_iSelEnd == -1 || m_iSelEnd == m_iSelStart)
			strLine.Format(IDS_INDICATOR_LINE_ARG, m_iSelStart + 1);
		else
			strLine.Format(IDS_INDICATOR_LINE_ARGS, m_iSelStart + 1, m_iSelEnd + 1);
	}
	else
		strLine.LoadString(ID_INDICATOR_LINE);

	pCmdUI->SetText(strLine);
}



// Utilities

void CSimpleAgeViewerView::FindAndSelectString(LPCTSTR szFind, bool fScroll, bool fBackwards, bool* pfWraparound)
{
	assert(pfWraparound == NULL || fScroll);  // can only use pfWraparound if fScroll

	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	if (pDoc->PfilerepRead() == NULL)
		return;

	CPoint cpSearch = m_ptSearchLast;

	if (GetCaseSensitive() ? m_strSearchLast.Compare(szFind) : m_strSearchLast.CompareNoCase(szFind))
	{  // string changed
		m_strSearchLast = szFind;

		if (fScroll)
		{
			cpSearch.x = 0;  // start looking at beginning of line
		}
		else
		{
			Invalidate();
			return;
		}
	}
	else
	{
		if (!fScroll)
			return;

		// skip past last search
		assert(cpSearch.y >= 0);
		if (fBackwards)
		{
			--cpSearch.x;
			if (cpSearch.x < 0)
			{
				--cpSearch.y;
				if (cpSearch.y < 0)
					cpSearch.y = pDoc->PfilerepRead()->CLine() - 1;
				cpSearch.x = _tcslen(pDoc->PfilerepRead()->GetLine(cpSearch.y));
			}
		}
		else
		{
			++cpSearch.x;
			if (cpSearch.x >= (int)_tcslen(pDoc->PfilerepRead()->GetLine(cpSearch.y)))
			{
				++cpSearch.y;
				if (cpSearch.y >= pDoc->PfilerepRead()->CLine())
					cpSearch.y = 0;
				cpSearch.x = 0;
			}
		}
	}

	const CPoint cpFound = fBackwards ?
		pDoc->PfilerepRead()->CPointFindStringBackwards(szFind, GetCaseSensitive(), cpSearch) :
		pDoc->PfilerepRead()->CPointFindString(szFind, GetCaseSensitive(), cpSearch);
	if (cpFound.y != -1)
	{
		ScrollToLine(cpFound.y, 1.0f / 3/*flScaleFromTop*/);

		m_ptSearchLast = cpFound;

		if (pfWraparound != NULL)
			*pfWraparound = false;
	}
	else
	{
		if (fBackwards)
			m_ptSearchLast.y = pDoc->PfilerepRead()->CLine() - 1;  // restart at end
		else
			m_ptSearchLast.y = 0;  // restart at beginning

		if (pfWraparound != NULL)
			*pfWraparound = true;
	}

	Invalidate();
}

void CSimpleAgeViewerView::ScrollToLine(int iLine, float flScaleFromTop)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	RECT rcClient;
	GetClientRect(&rcClient);
	int cLine = pDoc->PfilerepRead() != NULL ? pDoc->PfilerepRead()->CLine() : 0;
	int dyPxMax = cLine * m_sizChar.cy - rcClient.bottom;
	if (dyPxMax < 0)
		dyPxMax = 0;

	POINT ptCur = GetDeviceScrollPosition();

	ptCur.y = static_cast<LONG>((iLine * m_sizChar.cy - flScaleFromTop * rcClient.bottom) * m_sizChar.cy / m_sizChar.cy);
	if (ptCur.y < 0)
		ptCur.y = 0;
	else if (ptCur.y > dyPxMax)
		ptCur.y = dyPxMax;

	if (cLine > 0)
	{
		m_iSelStart = m_iSelEnd = max(0, min(iLine, cLine - 1));
		Invalidate();
	}

	ScrollToPosition(ptCur);
}

void CSimpleAgeViewerView::FindAndSelectVersion(bool fCurVer, bool fBackwards, bool* pfWraparound)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	const FILEREP* pfr = pDoc->PfilerepRead();
	if (pfr == NULL)
		return;

	TCHAR szNvers[256];
	if (fCurVer)
	{
		_itot_s(pfr->NVer(), szNvers, 10/*radix*/);
	}
	else
	{
		szNvers[0] = '1';
		szNvers[1] = '\0';
	}

	CPoint cpCur(0, m_ptSearchLast.y);

	if (cpCur == m_ptSearchLast)
	{  // look for first non-occurence
		cpCur = fBackwards ?
			pfr->CPointFindVerBackwards(szNvers, fCurVer/*fNot*/, m_ptSearchLast) :
			pfr->CPointFindVer(szNvers, fCurVer/*fNot*/, m_ptSearchLast);
	}

	const CPoint cpFound = fBackwards ?
		pfr->CPointFindVerBackwards(szNvers, !fCurVer/*fNot*/, cpCur) :
		pfr->CPointFindVer(szNvers, !fCurVer/*fNot*/, cpCur);
	m_ptSearchLast = cpFound;
	if (cpFound.y != -1)
	{
		ScrollToLine(cpFound.y, 1.0f / 3/*flScaleFromTop*/);

		m_ptSearchLast = cpFound;

		if (pfWraparound != NULL)
			*pfWraparound = false;
	}
	else
	{
		if (fBackwards && pfr->CLine() > 0)
			m_ptSearchLast.y = pfr->CLine() - 1;  // restart at end
		else
			m_ptSearchLast.y = 0;  // restart at beginning

		if (pfWraparound != NULL)
			*pfWraparound = true;
	}

	Invalidate();
}


void CSimpleAgeViewerView::OnEditCopy(CCmdUI* pCmdUI)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	int cLine = pDoc->PfilerepRead() != NULL ? pDoc->PfilerepRead()->CLine() : 0;
	pCmdUI->Enable(m_iSelStart >= 0 && m_iSelEnd >= m_iSelStart && cLine > m_iSelEnd);
}

void CSimpleAgeViewerView::OnEditCopy()
{
	if (m_iSelStart >= 0)
	{
		CSimpleAgeViewerDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (pDoc == NULL)
			return;

		const FILEREP* pfr = pDoc->PfilerepRead();
		if (pfr == NULL)
			return;

		bool fClipboardOpen = false;
		HGLOBAL hGlob = NULL;
		if (!::OpenClipboard(NULL/*hWndNewOwner; NULL - use current task */))
		{
			CString msg;
			msg.Format(_T("Cannot open the Clipboard, error: %d"), ::GetLastError());
			AfxMessageBox(msg);
			goto LCleanup;
		}
		fClipboardOpen = true;

		// Remove the current Clipboard contents
		if (!::EmptyClipboard())
		{
			CString msg;
			msg.Format(_T("Cannot empty the Clipboard, error: %d"), ::GetLastError());
			AfxMessageBox(msg);
			goto LCleanup;
		}
		// Get the currently selected data
		C_ASSERT(sizeof(TCHAR) == 2);

		int dLine = m_iSelEnd - m_iSelStart + 1;
		int cbClip = (dLine * (MAX_LINE + 1/*LF*/) + 1/*ending NULL*/) * sizeof(TCHAR);
		hGlob = ::GlobalAlloc(GMEM_MOVEABLE, cbClip);
		if (hGlob == NULL)
		{
			CString msg;
			msg.Format(_T("Cannot alloc clipboard memory, error: %d"), ::GetLastError());
			AfxMessageBox(msg);
			goto LCleanup;
		}

		TCHAR* szClip = static_cast<TCHAR*>(::GlobalLock(hGlob));
		if (szClip == NULL)
		{
			CString msg;
			msg.Format(_T("Cannot alloc clipboard memory, error: %d"), ::GetLastError());
			AfxMessageBox(msg);
			goto LCleanup;
		}

		int cbRemain = cbClip;
		int cbUsed = 0;
		for (int iLine = m_iSelStart; iLine <= m_iSelEnd; ++iLine)
		{
			const TCHAR* szLine = pfr->GetLine(iLine);
			assert(szLine != NULL);
			if (szLine != NULL)
			{
				int cchLine = _tcslen(szLine);
				int cbLine = cchLine * sizeof(TCHAR);
				memcpy_s(szClip, cbRemain, szLine, cbLine);
				if (cchLine > 0 && szClip[cchLine - 1] == _T('\n'))  // add CR-LF
				{
					szClip[cchLine - 1] = _T('\r');
					szClip[cchLine++] = _T('\n');
					cbLine += sizeof(TCHAR);
				}
				cbUsed += cbLine;
				cbRemain -= cbLine;
				szClip += cchLine;
			}
			assert(cbClip >= cbUsed);
			assert(cbRemain >= 0);
		}

		// NUL terminate
		*szClip++ = '\0';
		cbUsed += sizeof(TCHAR);
		cbRemain -= sizeof(TCHAR);
		assert(cbClip >= cbUsed);
		assert(cbRemain >= 0);

		VERIFY(::GlobalUnlock(hGlob) == 0/*no locks*/);

		hGlob = ::GlobalReAlloc(hGlob, cbUsed, GMEM_MOVEABLE); // shrink down to used size
		assert(hGlob != NULL);

		if (hGlob == NULL || ::SetClipboardData(CF_UNICODETEXT, hGlob) == NULL)
		{
			CString msg;
			msg.Format(_T("Unable to set Clipboard data, error: %d"), ::GetLastError());
			AfxMessageBox(msg);
			goto LCleanup;
		}

	LCleanup:
		if (fClipboardOpen)
			::CloseClipboard();
		if (hGlob != NULL)
			::GlobalFree(hGlob);
	}
}


void CSimpleAgeViewerView::OnJumpToChangeVersion(CCmdUI* pCmdUI)
{
	CSimpleAgeViewerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (pDoc == NULL)
		return;

	const FILEREP* pfr = pDoc->PfilerepRead();
	if (pfr != NULL)
		pCmdUI->Enable(m_iSelStart >= 0 && pfr->CLine() > 0 && pfr->NVer() > NverLast(m_iSelStart, *pfr, 1));
	else
		pCmdUI->Enable(false);

}

void CSimpleAgeViewerView::OnJumpToChangeVersion()
{
	if (m_iSelStart >= 0)
	{
		CSimpleAgeViewerDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		if (pDoc == NULL)
			return;

		const FILEREP* pfr = pDoc->PfilerepRead();
		if (pfr != NULL)
		{
			int nVerTarget = NverLast(m_iSelStart, *pfr, pfr->NVer());
			if (nVerTarget > 0)
			{
				if (pDoc->FEditToFileVersion(nVerTarget, pDoc->Filerep()))
					pDoc->UpdateAllViews(NULL);  // NULL - also update this view
			}
		}
	}
}



// CGotoDlg dialog used for line selection

class CGotoDlg : public CDialog
{
public:
	CGotoDlg(CSimpleAgeViewerView& view);

	// Dialog Data
	enum { IDD = IDD_GOTOLINE };

	// Implementation
protected:
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	CSimpleAgeViewerView& m_view;
};

CGotoDlg::CGotoDlg(CSimpleAgeViewerView& view) : m_view(view), CDialog(CGotoDlg::IDD)
{
}

BEGIN_MESSAGE_MAP(CGotoDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CSimpleAgeViewerView::OnGotoDlg()
{
	CGotoDlg gotoDlg(*this);
	gotoDlg.DoModal();
}

void CGotoDlg::OnOK()
{
	BOOL fSucc = FALSE;
	int nLine = static_cast<int>(GetDlgItemInt(ID_GOTOLINE_EDIT, &fSucc));
	if (fSucc)
		m_view.ScrollToLine(nLine - 1, 1.0f / 3/*flScaleFromTop*/);

	__super::OnOK();
}


// CSimpleAgeViewerView diagnostics

#ifdef _DEBUG
void CSimpleAgeViewerView::AssertValid() const
{
	CView::AssertValid();
}

void CSimpleAgeViewerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSimpleAgeViewerDoc* CSimpleAgeViewerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSimpleAgeViewerDoc)));
	return (CSimpleAgeViewerDoc*)m_pDocument;
}
#endif //_DEBUG

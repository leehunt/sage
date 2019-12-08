
// SimpleAgeViewer.h : main header file for the SimpleAgeViewer application
//
#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CSimpleAgeViewerApp:
// See SimpleAgeViewer.cpp for the implementation of this class
//

class CSimpleAgeViewerApp : public CWinAppEx
{
public:
	CSimpleAgeViewerApp();


	// Overrides
public:
	virtual BOOL InitInstance();

	const CString& GetBranchPrefixSlash() const { return m_cstrBranchPrefixSlash; }
	const CString& GetSdPath() const { return m_cstrSdPath; }

	// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

protected:
	void ParseCommandLine(CCommandLineInfo& rCmdInfo);
	void AddDocTemplate(CDocTemplate* pTemplate);

	void _InitIniPaths();
	void _LoadVariable(LPCTSTR pszName, UINT idsFallback,
		__out_ecount(cchBuf) LPTSTR pszBuf, UINT cchBuf);
	void LoadGlobalSettings();

	CString m_cstrInitSdIni;
	CString m_cstrExeSdIni;
	CString m_cstrSdPath;
	CString m_cstrBranchPrefixSlash;
};

extern CSimpleAgeViewerApp theApp;

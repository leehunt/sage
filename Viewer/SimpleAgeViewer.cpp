
// SimpleAgeViewer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "SimpleAgeViewer.h"
#include "MainFrm.h"
#include "SimpleAgeViewerDoc.h"

#include "SimpleAgeViewerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CSimpleAgeViewerCommandLineInfo : public CCommandLineInfo
{
public:
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
	{
		if (!bFlag)
			__super::ParseParam(pszParam, bFlag, bLast);
		else
		{
			if (*pszParam == _T('/'))
			{
				CString cstrSDFileName(_T("/"));
				cstrSDFileName.Append(pszParam);

				TCHAR szTempFile[MAX_PATH];
				::GetTempPath(_countof(szTempFile), szTempFile);
				::GetTempFileName(szTempFile, _T("dum"), 0, szTempFile);
				FILE* fh;
				_tfopen_s(&fh, szTempFile, _T("w"));
				if (fh != NULL)  // stupid: the temp file needs to be non-null for the MFC file open code to unarchive it
				{
					fputc('x', fh);
					fclose(fh);
				}

				m_strFileName = szTempFile;
				m_nShellCommand = CCommandLineInfo::FileOpen;
			}
			else
			{
				__super::ParseParam(pszParam, bFlag, bLast);
			}
		}
	}
};

class CSimpleAgeViewerDocManager : public CDocManager
{
	CDocument* OpenDocumentFile(LPCTSTR lpszFileName)
	{
		if (lpszFileName == NULL)
		{
			AfxThrowInvalidArgException();
		}

		if (*lpszFileName != '/')
			return __super::OpenDocumentFile(lpszFileName);

		// Below is just like __super::OpenDocumentFile() but doesn't choke on forward slashes (e.g. doesn't call AfxFullPath())

		// find the highest confidence
		POSITION pos = m_templateList.GetHeadPosition();
		CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
		CDocTemplate* pBestTemplate = NULL;
		CDocument* pOpenDocument = NULL;

		TCHAR szPath[_MAX_PATH];
		Checked::tcsncpy_s(szPath, _countof(szPath), lpszFileName, _TRUNCATE);

		while (pos != NULL)
		{
			CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
			ASSERT_KINDOF(CDocTemplate, pTemplate);

			CDocTemplate::Confidence match;
			ASSERT(pOpenDocument == NULL);
			match = pTemplate->MatchDocType(szPath, pOpenDocument);
			if (match > bestMatch)
			{
				bestMatch = match;
				pBestTemplate = pTemplate;
			}
			if (match == CDocTemplate::yesAlreadyOpen)
				break;      // stop here
		}

		if (pOpenDocument != NULL)
		{
			POSITION posOpenDoc = pOpenDocument->GetFirstViewPosition();
			if (posOpenDoc != NULL)
			{
				CView* pView = pOpenDocument->GetNextView(posOpenDoc); // get first one
				ASSERT_VALID(pView);
				CFrameWnd* pFrame = pView->GetParentFrame();

				if (pFrame == NULL)
					TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
				else
				{
					pFrame->ActivateFrame();

					if (pFrame->GetParent() != NULL)
					{
						CFrameWnd* pAppFrame;
						if (pFrame != (pAppFrame = (CFrameWnd*)AfxGetApp()->m_pMainWnd))
						{
							ASSERT_KINDOF(CFrameWnd, pAppFrame);
							pAppFrame->ActivateFrame();
						}
					}
				}
			}
			else
				TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");

			return pOpenDocument;
		}

		if (pBestTemplate == NULL)
		{
			AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
			return NULL;
		}

		return pBestTemplate->OpenDocumentFile(szPath);
	}
};


// CSimpleAgeViewerApp

BEGIN_MESSAGE_MAP(CSimpleAgeViewerApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CSimpleAgeViewerApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CSimpleAgeViewerApp construction

CSimpleAgeViewerApp::CSimpleAgeViewerApp()
{
	m_nAppLook = 0;
	m_bHiColorIcons = TRUE;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CSimpleAgeViewerApp object

CSimpleAgeViewerApp theApp;

// CSimpleAgeViewerApp initialization

void CSimpleAgeViewerApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
	for (int i = 1; i < __argc; i++)
	{
		LPCTSTR pszParam = __targv[i];
		BOOL bFlag = FALSE;
		BOOL bLast = ((i + 1) == __argc);
		if (pszParam[0] == '-' /*|| pszParam[0] == '/'*/)  // don't look at '/' -- they may be SD paths
		{
			// remove flag specifier
			bFlag = TRUE;
			++pszParam;
		}
		rCmdInfo.ParseParam(pszParam, bFlag, bLast);
	}
}

void CSimpleAgeViewerApp::AddDocTemplate(CDocTemplate* pTemplate)
{
	if (m_pDocManager == NULL)
		m_pDocManager = new CSimpleAgeViewerDocManager;
	m_pDocManager->AddDocTemplate(pTemplate);
}

BOOL CSimpleAgeViewerApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	LoadGlobalSettings();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey(_T("HumaneElectron"));  // an anagram
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSimpleAgeViewerDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CSimpleAgeViewerView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CSimpleAgeViewerCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

// Find the root (i.e. the first occurence of the file 'sd.ini' on the path taken from the root)
// NOTE: return string has ending '\'
static size_t _CchzGetSdRootDirWz(HINSTANCE hinst, WCHAR wzRootRet[], size_t cchzRootRet)
{
	if (wzRootRet[0] == '\0')
	{
		int cchz = ::GetModuleFileName(hinst/*theApp.m_hInstance*/, wzRootRet, static_cast<DWORD>(cchzRootRet));
		if (cchz <= 0)
			return 0;
		cchzRootRet -= cchz;
	}

	static WCHAR s_wzRootDir[_MAX_PATH + 1];
	if (s_wzRootDir[0] != '\0')
	{
		wcscpy_s(wzRootRet, cchzRootRet, s_wzRootDir);
		return wcslen(wzRootRet) + 1;
	}

	// Find the current Office root (the first place the hidden file sd.ini is located)
	WCHAR* pwchPath = wzRootRet;
	while (*pwchPath != L'\0')
	{
		if (*pwchPath == L'\\')
		{
			++pwchPath;  // include '\\'
			WCHAR wzFilePathT[_MAX_PATH + 1];
			size_t iOffset = pwchPath - wzRootRet;
			memcpy(wzFilePathT, wzRootRet, iOffset * sizeof(WCHAR));
			wcscpy_s(wzFilePathT + iOffset, _countof(wzFilePathT) - iOffset, L"sd.ini");
			if (::GetFileAttributes(wzFilePathT) != -1)
			{

				*pwchPath = L'\0';  // stop at root
				wcscpy_s(s_wzRootDir + 1, _countof(s_wzRootDir) - 1, wzRootRet + 1);
				s_wzRootDir[0] = wzRootRet[0];  // make thread safe (atomic)
				return iOffset + 1;  // found root
			}
		}
		++pwchPath;
		--cchzRootRet;
	}

	return 0;
}


static CString _CstrFindSDExe(HINSTANCE hinst, const TCHAR szDir[])
{
	TCHAR szSdExe[MAX_PATH];
	TCHAR* pchName = NULL;
	if (::SearchPath(NULL, _T("sd.exe"), NULL/*szExt*/, _countof(szSdExe), szSdExe, &pchName))
	{
		// found it
	}
	else if (szDir != NULL)
	{
		TCHAR szSdRoot[MAX_PATH];
		_tcscpy_s(szSdRoot, _countof(szSdRoot), szDir);
		int cchzSdDir = _CchzGetSdRootDirWz(hinst, szSdRoot, _countof(szSdRoot));
		if (cchzSdDir <= 0)
		{
			assert(false);
			*szSdExe = _T('\0');

			goto LDone;
		}
		_tcscpy_s(szSdExe, _countof(szSdExe), szSdRoot);
		_tcscpy_s(szSdExe + cchzSdDir - 1, _countof(szSdExe) - cchzSdDir, _T("dev14\\otools\\bin\\sd.exe"));  // TODO: fix
	}

LDone:
	return CString(szSdExe);
}

void CSimpleAgeViewerApp::_InitIniPaths()
{
	TCHAR szPath[MAX_PATH];
	szPath[MAX_PATH - 1] = TEXT('\0');

	if (::GetModuleFileName(AfxGetInstanceHandle(), szPath, _countof(szPath)) &&
		PathRemoveFileSpec(szPath)) {
		if (m_cstrSdPath.IsEmpty()) {
			m_cstrSdPath = _CstrFindSDExe(AfxGetInstanceHandle(), szPath);
		}

		_tcscpy_s(szPath, m_cstrSdPath);
		if (PathRemoveFileSpec(szPath) && PathAppend(szPath, _T("tools.ini")) && PathFileExists(szPath)) {
			m_cstrExeSdIni = szPath;
		}
	}

	DWORD cb = ::GetEnvironmentVariable(_T("INIT"), szPath, _countof(szPath));
	if (cb && cb < _countof(szPath) &&
		PathAppend(szPath, _T("tools.ini")) &&
		PathFileExists(szPath)) {
		m_cstrInitSdIni = szPath;
	}

}

void CSimpleAgeViewerApp::_LoadVariable(LPCTSTR pszName, UINT idsFallback,
	__out_ecount(cchBuf) LPTSTR pszBuf, UINT cchBuf)
{
	DWORD cb = ::GetEnvironmentVariable(pszName, pszBuf, cchBuf);
	if (cb && cb < cchBuf) {
		return;             /* Found in environment */
	}

	if (!m_cstrInitSdIni.IsEmpty() &&
		::GetPrivateProfileString(_T("sdv"), pszName, TEXT(""),
			pszBuf, cchBuf, m_cstrInitSdIni)) {
		return;             /* Found in %INIT%\tools.ini */
	}

	if (!m_cstrExeSdIni.IsEmpty() &&
		::GetPrivateProfileString(_T("sdv"), pszName, TEXT(""),
			pszBuf, cchBuf, m_cstrExeSdIni)) {
		return;             /* Found in tools.ini next to EXE */
	}

	LoadString(AfxGetResourceHandle(), idsFallback, pszBuf, cchBuf);
}


void CSimpleAgeViewerApp::LoadGlobalSettings()
{
	_InitIniPaths();

	TCHAR szPrefix[MAX_PATH];
	_LoadVariable(_T("SDVBRANCHPREFIX"), IDS_DEFAULT_SDVBRANCHPREFIX,
		szPrefix, _countof(szPrefix));
	_tcscat_s(szPrefix, _countof(szPrefix), _T("/"));
	m_cstrBranchPrefixSlash = szPrefix;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CSimpleAgeViewerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CSimpleAgeViewerApp customization load/save methods

void CSimpleAgeViewerApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CSimpleAgeViewerApp::LoadCustomState()
{
}

void CSimpleAgeViewerApp::SaveCustomState()
{
}

// CSimpleAgeViewerApp message handlers


// SimpleAgeViewerDoc.h : interface of the CSimpleAgeViewerDoc class
//


#pragma once

#include "../SDVersionReader/SDVersionReader.h"

class CSimpleAgeViewerDoc : public CDocument
{
protected: // create from serialization only
	CSimpleAgeViewerDoc();
	DECLARE_DYNCREATE(CSimpleAgeViewerDoc)

	// Attributes
public:

	// Operations
public:

	// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void Serialize(CArchive& ar);
	virtual CFile* GetFile(LPCTSTR lpszFileName, UINT nOpenFlags,
		CFileException* pError);

	// Implementation
public:
	virtual ~CSimpleAgeViewerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	const FILEREP* PfilerepRead() const { return m_pfilerep; }
	FILEREP& Filerep() { return *m_pfilerep; }
	const DIFFRECORD* PdrListRead() const { return m_pfilerep != NULL ? m_pfilerep->PdrListRoot() : NULL; }

	const FILEREP* PfilerepReadRoot() const { return m_pfilerepRoot; }

	bool FEditToFileVersion(int nVer, FILEREP& pfr);

	void AddDocListener(CSimpleAgeViewerDocListener& listener);
	void RemoveDocListener(CSimpleAgeViewerDocListener& listener);

	afx_msg void OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVersionsTree(CCmdUI* pCmdUI);

protected:

	FILEREP* m_pfilerep;
	FILEREP* m_pfilerepRoot;
	CSimpleAgeViewerDocListener* m_pDocListenerHead;

	bool m_fNewDoc;  // HACK

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

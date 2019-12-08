#pragma once

#include <assert.h>

class CSimpleAgeViewerDocListener
{
public:
	CSimpleAgeViewerDocListener() : m_pNext(NULL), m_pPrev(NULL) {}
	~CSimpleAgeViewerDocListener() { assert(m_pNext == NULL); assert(m_pPrev == NULL); }

	virtual void DocEditNotification(int iLine, int cLine) = 0;
	virtual void DocVersionChangedNotification(int nVer) = 0;

	CSimpleAgeViewerDocListener* m_pNext;
	CSimpleAgeViewerDocListener* m_pPrev;
};


#ifndef CHANNEL_SCAN_H
#define CHANNEL_SCAN_H


#include "CoreEngine.h"
#include "ChannelList.h"
#include "Options.h"


class CChannelScan : public COptions {
	CCoreEngine *m_pCoreEngine;
	int m_ScanSpace;
	int m_ScanChannel;
	int m_NumChannels;
	CTuningSpaceList m_TuningSpaceList;
	CChannelList m_ScanningChannelList;
	bool m_fScanService;
	bool m_fIgnoreSignalLevel;
	bool m_fUpdated;
	bool m_fScaned;
	HWND m_hScanDlg;
	HANDLE m_hScanThread;
	HANDLE m_hCancelEvent;
	bool m_fOK;
	int m_SortColumn;
	bool m_fSortDescending;
	void InsertChannelInfo(int Index,const CChannelInfo *pChInfo);
	void SetChannelList(int Space);
	static DWORD WINAPI ScanProc(LPVOID lpParameter);
	static BOOL CALLBACK ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static CChannelScan *GetThis(HWND hDlg);
public:
	CChannelScan(CCoreEngine *pCoreEngine);
	~CChannelScan();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList);
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

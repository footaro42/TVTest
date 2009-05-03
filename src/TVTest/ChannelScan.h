#ifndef CHANNEL_SCAN_H
#define CHANNEL_SCAN_H


#include "CoreEngine.h"
#include "ChannelList.h"
#include "Options.h"


class CChannelScan : public COptions {
	enum {
		UPDATE_CHANNELLIST	= 0x0000001UL,
		UPDATE_PREVIEW		= 0x0000002UL
	};
	CCoreEngine *m_pCoreEngine;
	int m_ScanSpace;
	int m_ScanChannel;
	int m_NumChannels;
	const CTuningSpaceList *m_pOriginalTuningSpaceList;
	CTuningSpaceList m_TuningSpaceList;
	CChannelList m_ScanningChannelList;
	bool m_fScanService;
	bool m_fIgnoreSignalLevel;
	bool m_fUpdated;
	bool m_fScaned;
	bool m_fRestorePreview;
	HWND m_hScanDlg;
	HANDLE m_hScanThread;
	HANDLE m_hCancelEvent;
	bool m_fOK;
	int m_SortColumn;
	bool m_fSortDescending;
	bool m_fChanging;
	void InsertChannelInfo(int Index,const CChannelInfo *pChInfo);
	void SetChannelList(int Space);
	CChannelInfo *GetSelectedChannelInfo() const;
	static DWORD WINAPI ScanProc(LPVOID lpParameter);
	static BOOL CALLBACK ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK ChannelPropDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static CChannelScan *GetThis(HWND hDlg);

public:
	CChannelScan(CCoreEngine *pCoreEngine);
	~CChannelScan();
// COptions
	bool Apply(DWORD Flags);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CChannelScan
	bool SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList);
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

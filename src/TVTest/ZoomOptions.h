#ifndef ZOOM_OPTIONS_H
#define ZOOM_OPTIONS_H


#include "Command.h"
#include "Settings.h"


class CZoomOptions
{
public:
	struct ZoomRate {
		int Rate;
		int Factor;
	};

private:
	enum { NUM_ZOOM = 16 };
	enum { MAX_RATE = 1000 };

	struct ZoomInfo {
		const int Command;
		ZoomRate Zoom;
		bool fVisible;
	};
	static ZoomInfo m_ZoomList[NUM_ZOOM];

	const CCommandList *m_pCommandList;
	int m_Order[NUM_ZOOM];
	bool m_fChanging;

	static CZoomOptions *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static void CZoomOptions::SetItemState(HWND hDlg);

public:
	CZoomOptions(const CCommandList *pCommandList);
	~CZoomOptions();
	bool ShowDialog(HWND hwndOwner);
	bool SetMenu(HMENU hmenu,const ZoomRate *pCurRate) const;
	bool GetZoomRateByCommand(int Command,ZoomRate *pRate) const;
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
};


#endif

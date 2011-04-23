#ifndef ZOOM_OPTIONS_H
#define ZOOM_OPTIONS_H


#include "Command.h"
#include "Settings.h"
#include "Dialog.h"


class CZoomOptions : public CBasicDialog
{
public:
	struct ZoomRate {
		int Rate;
		int Factor;
	};

	CZoomOptions(const CCommandList *pCommandList);
	~CZoomOptions();
	bool Show(HWND hwndOwner) override;
	bool SetMenu(HMENU hmenu,const ZoomRate *pCurRate) const;
	bool GetZoomRateByCommand(int Command,ZoomRate *pRate) const;
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;

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

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
	static void CZoomOptions::SetItemState(HWND hDlg);
};


#endif

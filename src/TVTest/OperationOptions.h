#ifndef OPERATION_OPTIONS_H
#define OPERATION_OPTIONS_H


#include "Options.h"
#include "Command.h"


class COperationOptions : public COptions {
public:
	enum WheelMode {
		WHEEL_NONE,
		WHEEL_VOLUME,
		WHEEL_CHANNEL,
		WHEEL_AUDIO,
		WHEEL_ZOOM,
		WHEEL_ASPECTRATIO,
		WHEEL_FIRST=WHEEL_NONE,
		WHEEL_LAST=WHEEL_ZOOM
	};
	enum {
		WHEEL_CHANNEL_DELAY_MIN=100
	};

private:
	const CCommandList *m_pCommandList;
	WheelMode m_WheelMode;
	WheelMode m_WheelShiftMode;
	WheelMode m_WheelCtrlMode;
	WheelMode m_WheelTiltMode;
	bool m_fWheelChannelReverse;
	int m_WheelChannelDelay;
	int m_VolumeStep;
	int m_WheelZoomStep;
	bool m_fDisplayDragMove;
	int m_LeftDoubleClickCommand;
	int m_RightClickCommand;
	int m_MiddleClickCommand;

	bool Load(LPCTSTR pszFileName);
	static COperationOptions *GetThis(HWND hDlg);
	static void InitWheelModeList(HWND hDlg,int ID);

public:
	COperationOptions();
	~COperationOptions();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// COperationOptions
	bool Initialize(LPCTSTR pszFileName,const CCommandList *pCommandList);
	WheelMode GetWheelMode() const { return m_WheelMode; }
	WheelMode GetWheelShiftMode() const { return m_WheelShiftMode; }
	WheelMode GetWheelCtrlMode() const { return m_WheelCtrlMode; }
	WheelMode GetWheelTiltMode() const { return m_WheelTiltMode; }
	bool GetWheelChannelReverse() const { return m_fWheelChannelReverse; }
	int GetWheelChannelDelay() const { return m_WheelChannelDelay; }
	int GetVolumeStep() const { return m_VolumeStep; }
	int GetWheelZoomStep() const { return m_WheelZoomStep; }
	bool GetDisplayDragMove() const { return m_fDisplayDragMove; }
	int GetLeftDoubleClickCommand() const { return m_LeftDoubleClickCommand; }
	int GetRightClickCommand() const { return m_RightClickCommand; }
	int GetMiddleClickCommand() const { return m_MiddleClickCommand; }
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

#ifndef OPERATION_OPTIONS_H
#define OPERATION_OPTIONS_H


#include "Options.h"


class COperationOptions : public COptions {
public:
	enum WheelMode {
		WHEEL_NONE,
		WHEEL_VOLUME,
		WHEEL_CHANNEL,
		WHEEL_AUDIO,
		WHEEL_ZOOM,
		WHEEL_FIRST=WHEEL_NONE,
		WHEEL_LAST=WHEEL_ZOOM
	};
	enum {
		WHEEL_CHANNEL_DELAY_MIN=100
	};

private:
	WheelMode m_WheelMode;
	WheelMode m_WheelShiftMode;
	bool m_fWheelChannelReverse;
	int m_WheelChannelDelay;
	int m_VolumeStep;
	bool m_fDisplayDragMove;
	static COperationOptions *GetThis(HWND hDlg);

public:
	COperationOptions();
	~COperationOptions();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// COperationOptions
	WheelMode GetWheelMode() const { return m_WheelMode; }
	WheelMode GetWheelShiftMode() const { return m_WheelShiftMode; }
	bool GetWheelChannelReverse() const { return m_fWheelChannelReverse; }
	int GetWheelChannelDelay() const { return m_WheelChannelDelay; }
	int GetVolumeStep() const { return m_VolumeStep; }
	bool GetDisplayDragMove() const { return m_fDisplayDragMove; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

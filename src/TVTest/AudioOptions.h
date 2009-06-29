#ifndef AUDIO_OPTIONS_H
#define AUDIO_OPTIONS_H


#include "Options.h"


class CAudioOptions : public COptions {
	enum { MAX_AUDIO_DEVICE_NAME=128 };
	TCHAR m_szAudioDeviceName[MAX_AUDIO_DEVICE_NAME];
	bool m_fDownMixSurround;
	bool m_fRestoreMute;
	bool m_fUseAudioRendererClock;
	bool m_fAdjustAudioStreamTime;
	bool m_fMinTimerResolution;
	static CAudioOptions *GetThis(HWND hDlg);

public:
	CAudioOptions();
	~CAudioOptions();
	LPCTSTR GetAudioDeviceName() const { return m_szAudioDeviceName; }
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	bool GetRestoreMute() const { return m_fRestoreMute; }
	bool GetUseAudioRendererClock() const { return m_fUseAudioRendererClock; }
	bool GetAdjustAudioStreamTime() const { return m_fAdjustAudioStreamTime; }
	bool GetMinTimerResolution() const { return m_fMinTimerResolution; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
};


#endif

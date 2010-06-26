#ifndef UI_SKIN_H
#define UI_SKIN_H


#include "BonTsEngine/Exception.h"


class CUICore;

class ABSTRACT_CLASS(CUISkin)
{
protected:
	enum {
		TIMER_ID_UPDATE=1,
		TIMER_ID_OSD,
		TIMER_ID_DISPLAY,
		TIMER_ID_WHEELCHANNELCHANGE,
		TIMER_ID_PROGRAMLISTUPDATE,
		TIMER_ID_PROGRAMGUIDEUPDATE,
		TIMER_ID_VIDEOSIZECHANGED,
		TIMER_ID_RESETERRORCOUNT,
		TIMER_ID_HIDETOOLTIP,
		TIMER_ID_USER=256
	};

	CUICore *m_pCore;
	bool m_fWheelChannelChanging;

	virtual bool InitializeViewer()=0;
	virtual bool FinalizeViewer()=0;
	virtual bool EnableViewer(bool fEnable)=0;
	virtual bool IsViewerEnabled() const=0;
	virtual void OnVolumeChanged(bool fOSD) {}
	virtual void OnMuteChanged() {}
	virtual void OnStereoModeChanged() {}
	virtual void OnAudioStreamChanged() {}
	virtual bool OnStandbyChange(bool fStandby) { return true; }
	virtual bool OnFullscreenChange(bool fFullscreen)=0;
	virtual bool SetAlwaysOnTop(bool fTop)=0;
	virtual void OnTunerChanged() {}
	virtual void OnTunerOpened() {}
	virtual void OnTunerClosed() {}
	virtual void OnChannelListChanged() {}
	virtual void OnChannelChanged(bool fSpaceChanged) {}
	virtual void OnServiceChanged() {}
	virtual void OnRecordingStarted() {}
	virtual void OnRecordingStopped() {}

	void SetWheelChannelChanging(bool fChanging,DWORD Delay=0);

public:
	CUISkin();
	virtual ~CUISkin();
	virtual HWND GetMainWindow() const=0;
	virtual HWND GetVideoHostWindow() const=0;
	virtual int ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption=NULL,
							UINT Type=MB_OK | MB_ICONEXCLAMATION) const;
	virtual void ShowErrorMessage(LPCTSTR pszText) const;
	virtual void ShowErrorMessage(const CBonErrorHandler *pErrorHandler,
								  LPCTSTR pszTitle=NULL) const;
	bool IsWheelChannelChanging() const { return m_fWheelChannelChanging; }

	friend CUICore;
};


#endif

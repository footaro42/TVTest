#ifndef OSD_OPTIONS_H
#define OSD_OPTIONS_H


#include "Options.h"


class COSDOptions : public COptions {
	bool m_fShowOSD;
	bool m_fPseudoOSD;
	COLORREF m_TextColor;
	COLORREF m_CurTextColor;
	int m_Opacity;
	int m_FadeTime;

	bool m_fEnableNotificationBar;
	int m_NotificationBarDuration;
	unsigned int m_NotificationBarFlags;
	LOGFONT m_NotificationBarFont;
	LOGFONT m_CurNotificationBarFont;

	void EnableNotify(unsigned int Type,bool fEnabled);
	static COSDOptions *GetThis(HWND hDlg);

public:
	enum {
		NOTIFY_EVENTNAME	=0x00000001
	};

	COSDOptions();
	~COSDOptions();
	bool GetShowOSD() const { return m_fShowOSD; }
	bool GetPseudoOSD() const { return m_fPseudoOSD; }
	COLORREF GetTextColor() const { return m_TextColor; }
	int GetOpacity() const { return m_Opacity; }
	int GetFadeTime() const { return m_FadeTime; }
	bool IsNotificationBarEnabled() const { return m_fEnableNotificationBar; }
	int GetNotificationBarDuration() const { return m_NotificationBarDuration; }
	const LOGFONT *GetNotificationBarFont() const { return &m_NotificationBarFont; }
	bool IsNotifyEnabled(unsigned int Type) const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
};


#endif

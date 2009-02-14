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
	static COSDOptions *GetThis(HWND hDlg);
public:
	COSDOptions();
	~COSDOptions();
	bool GetShowOSD() const { return m_fShowOSD; }
	bool GetPseudoOSD() const { return m_fPseudoOSD; }
	COLORREF GetTextColor() const { return m_TextColor; }
	int GetOpacity() const { return m_Opacity; }
	int GetFadeTime() const { return m_FadeTime; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
};


#endif

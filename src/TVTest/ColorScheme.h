#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H


#include "Options.h"
#include "ColorPalette.h"
#include "Theme.h"


class CColorScheme {
public:
	enum {
		COLOR_STATUSBACK1,
		COLOR_STATUSBACK2,
		COLOR_STATUSTEXT,
		COLOR_STATUSHIGHLIGHTBACK1,
		COLOR_STATUSHIGHLIGHTBACK2,
		COLOR_STATUSHIGHLIGHTTEXT,
		COLOR_STATUSRECORDINGCIRCLE,
		COLOR_PANELBACK,
		COLOR_PANELTEXT,
		COLOR_PANELTABBACK1,
		COLOR_PANELTABBACK2,
		COLOR_PANELTABTEXT,
		COLOR_PANELTABBORDER,
		COLOR_PANELCURTABBACK1,
		COLOR_PANELCURTABBACK2,
		COLOR_PANELCURTABTEXT,
		COLOR_PANELCURTABBORDER,
		COLOR_PANELTABMARGIN,
		COLOR_PANELTITLEBACK1,
		COLOR_PANELTITLEBACK2,
		COLOR_PANELTITLETEXT,
		COLOR_PROGRAMINFOBACK,
		COLOR_PROGRAMINFOTEXT,
		COLOR_PROGRAMLISTBACK1,
		COLOR_PROGRAMLISTBACK2,
		COLOR_PROGRAMLISTTEXT,
		COLOR_PROGRAMLISTCURBACK1,
		COLOR_PROGRAMLISTCURBACK2,
		COLOR_PROGRAMLISTCURTEXT,
		COLOR_PROGRAMLISTTITLEBACK1,
		COLOR_PROGRAMLISTTITLEBACK2,
		COLOR_PROGRAMLISTTITLETEXT,
		COLOR_PROGRAMLISTCURTITLEBACK1,
		COLOR_PROGRAMLISTCURTITLEBACK2,
		COLOR_PROGRAMLISTCURTITLETEXT,
		COLOR_CHANNELPANELCHANNELNAMEBACK1,
		COLOR_CHANNELPANELCHANNELNAMEBACK2,
		COLOR_CHANNELPANELCHANNELNAMETEXT,
		COLOR_CHANNELPANELCURCHANNELNAMEBACK1,
		COLOR_CHANNELPANELCURCHANNELNAMEBACK2,
		COLOR_CHANNELPANELCURCHANNELNAMETEXT,
		COLOR_CHANNELPANELEVENTNAMEBACK1,
		COLOR_CHANNELPANELEVENTNAMEBACK2,
		COLOR_CHANNELPANELEVENTNAMETEXT,
		COLOR_CHANNELPANELCUREVENTNAMEBACK1,
		COLOR_CHANNELPANELCUREVENTNAMEBACK2,
		COLOR_CHANNELPANELCUREVENTNAMETEXT,
		COLOR_CONTROLPANELBACK1,
		COLOR_CONTROLPANELBACK2,
		COLOR_CONTROLPANELTEXT,
		COLOR_CONTROLPANELHIGHLIGHTBACK1,
		COLOR_CONTROLPANELHIGHLIGHTBACK2,
		COLOR_CONTROLPANELHIGHLIGHTTEXT,
		COLOR_CONTROLPANELMARGIN,
		COLOR_CAPTIONPANELBACK,
		COLOR_CAPTIONPANELTEXT,
		COLOR_TITLEBARBACK1,
		COLOR_TITLEBARBACK2,
		COLOR_TITLEBARTEXT,
		COLOR_TITLEBARHIGHLIGHTBACK1,
		COLOR_TITLEBARHIGHLIGHTBACK2,
		COLOR_TITLEBARHIGHLIGHTICON,
		COLOR_SIDEBARBACK1,
		COLOR_SIDEBARBACK2,
		COLOR_SIDEBARICON,
		COLOR_SIDEBARHIGHLIGHTBACK1,
		COLOR_SIDEBARHIGHLIGHTBACK2,
		COLOR_SIDEBARHIGHLIGHTICON,
		COLOR_NOTIFICATIONBARBACK1,
		COLOR_NOTIFICATIONBARBACK2,
		COLOR_NOTIFICATIONBARTEXT,
		COLOR_NOTIFICATIONBARERRORTEXT,
		COLOR_PROGRAMGUIDEBACK,
		COLOR_PROGRAMGUIDETEXT,
		COLOR_PROGRAMGUIDECHANNELBACK1,
		COLOR_PROGRAMGUIDECHANNELBACK2,
		COLOR_PROGRAMGUIDECHANNELTEXT,
		COLOR_PROGRAMGUIDECURCHANNELBACK1,
		COLOR_PROGRAMGUIDECURCHANNELBACK2,
		COLOR_PROGRAMGUIDECURCHANNELTEXT,
		COLOR_PROGRAMGUIDETIMEBACK1,
		COLOR_PROGRAMGUIDETIMEBACK2,
		COLOR_PROGRAMGUIDETIMETEXT,
		COLOR_PROGRAMGUIDETIMELINE,
		COLOR_PROGRAMGUIDE_CONTENT_NEWS,
		COLOR_PROGRAMGUIDE_CONTENT_SPORTS,
		COLOR_PROGRAMGUIDE_CONTENT_INFORMATION,
		COLOR_PROGRAMGUIDE_CONTENT_DRAMA,
		COLOR_PROGRAMGUIDE_CONTENT_MUSIC,
		COLOR_PROGRAMGUIDE_CONTENT_VARIETY,
		COLOR_PROGRAMGUIDE_CONTENT_MOVIE,
		COLOR_PROGRAMGUIDE_CONTENT_ANIME,
		COLOR_PROGRAMGUIDE_CONTENT_DOCUMENTARY,
		COLOR_PROGRAMGUIDE_CONTENT_THEATER,
		COLOR_PROGRAMGUIDE_CONTENT_EDUCATION,
		COLOR_PROGRAMGUIDE_CONTENT_WELFARE,
		COLOR_PROGRAMGUIDE_CONTENT_OTHER,
		COLOR_PROGRAMGUIDE_CONTENT_FIRST=COLOR_PROGRAMGUIDE_CONTENT_NEWS,
		COLOR_PROGRAMGUIDE_CONTENT_LAST=COLOR_PROGRAMGUIDE_CONTENT_OTHER,
		COLOR_LAST=COLOR_PROGRAMGUIDE_CONTENT_LAST,
		NUM_COLORS
	};
	enum {
		GRADIENT_STATUSBACK,
		GRADIENT_STATUSHIGHLIGHTBACK,
		GRADIENT_PANELTABBACK,
		GRADIENT_PANELCURTABBACK,
		GRADIENT_PANELTITLEBACK,
		GRADIENT_PROGRAMLISTBACK,
		GRADIENT_PROGRAMLISTCURBACK,
		GRADIENT_PROGRAMLISTTITLEBACK,
		GRADIENT_PROGRAMLISTCURTITLEBACK,
		GRADIENT_CHANNELPANELCHANNELNAMEBACK,
		GRADIENT_CHANNELPANELCURCHANNELNAMEBACK,
		GRADIENT_CHANNELPANELEVENTNAMEBACK,
		GRADIENT_CHANNELPANELCUREVENTNAMEBACK,
		GRADIENT_CONTROLPANELBACK,
		GRADIENT_CONTROLPANELHIGHLIGHTBACK,
		GRADIENT_TITLEBARBACK,
		GRADIENT_TITLEBARHIGHLIGHTBACK,
		GRADIENT_SIDEBARBACK,
		GRADIENT_SIDEBARHIGHLIGHTBACK,
		GRADIENT_NOTIFICATIONBARBACK,
		GRADIENT_PROGRAMGUIDECHANNELBACK,
		GRADIENT_PROGRAMGUIDECURCHANNELBACK,
		GRADIENT_PROGRAMGUIDETIMEBACK,
		NUM_GRADIENTS
	};
	enum GradientType {
		GRADIENT_TYPE_UNDEFINED=-1,
		GRADIENT_TYPE_NORMAL,
		GRADIENT_TYPE_GLOSSY,
		GRADIENT_TYPE_FIRST=GRADIENT_TYPE_NORMAL,
		GRADIENT_TYPE_LAST=GRADIENT_TYPE_GLOSSY
	};
	enum {
		BORDER_STATUS,
		BORDER_TITLEBAR,
		BORDER_SIDEBAR,
		NUM_BORDERS
	};

	CColorScheme();
	CColorScheme(const CColorScheme &ColorScheme);
	~CColorScheme();
	CColorScheme &operator=(const CColorScheme &ColorScheme);
	COLORREF GetColor(int Type) const;
	COLORREF GetColor(LPCTSTR pszText) const;
	bool SetColor(int Type,COLORREF Color);
	GradientType GetGradientType(int Gradient) const;
	GradientType GetGradientType(LPCTSTR pszText) const;
	bool SetGradientType(int Gradient,GradientType Type);
	bool GetGradientInfo(int Gradient,Theme::GradientInfo *pInfo) const;
	Theme::BorderType GetBorderType(int Border) const;
	bool SetBorderType(int Border,Theme::BorderType Type);
	LPCTSTR GetName() const { return m_pszName; }
	bool SetName(LPCTSTR pszName);
	LPCTSTR GetFileName() const { return m_pszFileName; }
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool SetFileName(LPCTSTR pszFileName);
	void SetDefault();
	bool IsLoaded(int Type) const;
	void SetLoaded();
	static LPCTSTR GetColorName(int Type);
	static COLORREF GetDefaultColor(int Type);
	static GradientType GetDefaultGradientType(int Gradient);
	static Theme::BorderType GetDefaultBorderType(int Border);
	static int GetColorGradient(int Type);

private:
	COLORREF m_ColorList[NUM_COLORS];
	GradientType m_GradientList[NUM_GRADIENTS];
	Theme::BorderType m_BorderList[NUM_BORDERS];
	LPTSTR m_pszName;
	LPTSTR m_pszFileName;
	struct ColorInfo {
		COLORREF DefaultColor;
		LPCTSTR pszText;
		LPCTSTR pszName;
	};
	struct GradientInfo {
		LPCTSTR pszText;
		int Color1;
		int Color2;
	};
	struct BorderInfo {
		LPCTSTR pszText;
		Theme::BorderType DefaultType;
	};
	DWORD m_LoadedFlags[(NUM_COLORS+31)/32];
	void SetLoadedFlag(int Color);
	static const ColorInfo m_ColorInfoList[NUM_COLORS];
	static const GradientInfo m_GradientInfoList[NUM_GRADIENTS];
	static const BorderInfo m_BorderInfoList[NUM_BORDERS];
};

class CColorSchemeList {
	int m_NumColorSchemes;
	CColorScheme **m_ppList;

public:
	CColorSchemeList();
	~CColorSchemeList();
	int NumColorSchemes() const { return m_NumColorSchemes; }
	bool Add(CColorScheme *pColorScheme);
	bool Load(LPCTSTR pszDirectory);
	void Clear();
	CColorScheme *GetColorScheme(int Index);
	bool SetColorScheme(int Index,const CColorScheme *pColorScheme);
};

class CColorSchemeOptions : public COptions {
public:
	typedef bool (*ApplyFunc)(const CColorScheme *pColorScheme);

private:
	CColorScheme *m_pColorScheme;
	CColorSchemeList m_PresetList;
	CColorScheme::GradientType m_GradientList[CColorScheme::NUM_GRADIENTS];
	Theme::BorderType m_BorderList[CColorScheme::NUM_BORDERS];
	CColorScheme *m_pPreviewColorScheme;
	bool m_fPreview;
	ApplyFunc m_pApplyFunc;
	CColorPalette m_ColorPalette;
	bool Apply(const CColorScheme *pColorScheme) const;
	void GetCurrentSettings(CColorScheme *pColorScheme);
	static const LPCTSTR m_pszExtension;
	static CColorSchemeOptions *GetThis(HWND hwnd);

public:
	CColorSchemeOptions();
	~CColorSchemeOptions();
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool SetApplyCallback(ApplyFunc pCallback);
	bool ApplyColorScheme() const;
	const CColorScheme *GetColorScheme() const { return m_pColorScheme; }
	COLORREF GetColor(int Type) const;
	COLORREF GetColor(LPCTSTR pszText) const;
	static bool GetThemesDirectory(LPTSTR pszDirectory,int MaxLength,bool fCreate=false);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

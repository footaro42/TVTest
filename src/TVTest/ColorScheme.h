#ifndef COLOR_SCHEME_H
#define COLOR_SCHEME_H


#include "Options.h"
#include "ColorPalette.h"


class CColorScheme {
public:
	enum {
		COLOR_STATUSBACK1,
		COLOR_STATUSBACK2,
		COLOR_STATUSTEXT,
		COLOR_STATUSHIGHLIGHTBACK1,
		COLOR_STATUSHIGHLIGHTBACK2,
		COLOR_STATUSHIGHLIGHTTEXT,
		COLOR_PANELBACK,
		COLOR_PANELTEXT,
		COLOR_PANELTABBACK,
		COLOR_PANELTABTEXT,
		COLOR_PANELTABBORDER,
		COLOR_PANELCURTABBACK,
		COLOR_PANELCURTABTEXT,
		COLOR_PANELCURTABBORDER,
		COLOR_PANELTABMARGIN,
		COLOR_PANELTITLEBACK,
		COLOR_PANELTITLETEXT,
		COLOR_PROGRAMINFOBACK,
		COLOR_PROGRAMINFOTEXT,
		COLOR_PROGRAMLISTBACK,
		COLOR_PROGRAMLISTTEXT,
		COLOR_PROGRAMLISTTITLEBACK,
		COLOR_PROGRAMLISTTITLETEXT,
		COLOR_CHANNELPANELCHANNELNAMEBACK,
		COLOR_CHANNELPANELCHANNELNAMETEXT,
		COLOR_CHANNELPANELEVENTNAMEBACK,
		COLOR_CHANNELPANELEVENTNAMETEXT,
		COLOR_CONTROLPANELHIGHLIGHTBACK,
		COLOR_CONTROLPANELHIGHLIGHTTEXT,
		COLOR_PROGRAMGUIDEBACK,
		COLOR_PROGRAMGUIDETEXT,
		COLOR_PROGRAMGUIDECHANNELBACK,
		COLOR_PROGRAMGUIDECHANNELTEXT,
		COLOR_PROGRAMGUIDETIMEBACK,
		COLOR_PROGRAMGUIDETIMETEXT,
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
		COLOR_PROGRAMGUIDE_FIRST=COLOR_PROGRAMGUIDEBACK,
		COLOR_PROGRAMGUIDE_LAST=COLOR_PROGRAMGUIDE_CONTENT_OTHER,
		COLOR_LAST=COLOR_PROGRAMGUIDE_LAST
	};
	enum { NUM_COLORS=COLOR_LAST+1 };
	CColorScheme();
	CColorScheme(const CColorScheme &ColorScheme);
	~CColorScheme();
	CColorScheme &operator=(const CColorScheme &ColorScheme);
	COLORREF GetColor(int Type) const;
	COLORREF GetColor(LPCTSTR pszText) const;
	bool SetColor(int Type,COLORREF Color);
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
private:
	COLORREF m_ColorList[NUM_COLORS];
	LPTSTR m_pszName;
	LPTSTR m_pszFileName;
	struct ColorInfo {
		COLORREF DefaultColor;
		LPCTSTR pszText;
		LPCTSTR pszName;
	};
	DWORD m_LoadedFlags[(NUM_COLORS+31)/32];
	static const ColorInfo m_ColorInfoList[NUM_COLORS];
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
	bool m_fPreview;
	ApplyFunc m_pApplyFunc;
	CColorPalette m_ColorPalette;
	bool Apply(const CColorScheme *pColorScheme) const;
	static void GetListColor(HWND hDlg,CColorScheme *pColorScheme);
	static const LPCTSTR m_pszExtension;
	static CColorSchemeOptions *GetThis(HWND hwnd);
public:
	CColorSchemeOptions();
	~CColorSchemeOptions();
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool SetApplyCallback(ApplyFunc pCallback);
	bool ApplyColorScheme() const;
	COLORREF GetColor(int Type) const;
	COLORREF GetColor(LPCTSTR pszText) const;
	static bool GetThemesDirectory(LPTSTR pszDirectory,int MaxLength,bool fCreate=false);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

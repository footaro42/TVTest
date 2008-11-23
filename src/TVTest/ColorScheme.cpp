#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "ColorScheme.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "resource.h"




const CColorScheme::ColorInfo CColorScheme::m_ColorInfoList[NUM_COLORS] = {
	{RGB(176,216,196),	TEXT("StatusBack"),					TEXT("ステータスバー背景1")},
	{RGB(128,192,160),	TEXT("StatusBack2"),				TEXT("ステータスバー背景2")},
	{RGB(64,96,80),		TEXT("StatusText"),					TEXT("ステータスバー文字")},
	{RGB(85,128,106),	TEXT("StatusHighlightBack"),		TEXT("ステータスバー選択背景1")},
	{RGB(64,96,80),		TEXT("StatusHighlightBack2"),		TEXT("ステータスバー選択背景2")},
	{RGB(128,192,160),	TEXT("StatusHighlightText"),		TEXT("ステータスバー選択文字")},
	{RGB(128,192,160),	TEXT("PanelBack"),					TEXT("パネル背景")},
	{RGB(64,96,80),		TEXT("PanelText"),					TEXT("パネル文字")},
	{RGB(238,238,238),	TEXT("PanelTabBack"),				TEXT("パネルタブ背景")},
	{RGB(85,85,85),		TEXT("PanelTabText"),				TEXT("パネルタブ文字")},
	{RGB(85,85,85),		TEXT("PanelTabBorder"),				TEXT("パネルタブ枠")},
	{RGB(128,192,160),	TEXT("PanelCurTabBack"),			TEXT("パネル選択タブ背景")},
	{RGB(64,96,80),		TEXT("PanelCurTabText"),			TEXT("パネル選択タブ文字")},
	{RGB(64,96,80),		TEXT("PanelCurTabBorder"),			TEXT("パネル選択タブ枠")},
	{RGB(238,238,238),	TEXT("PanelTabMargin"),				TEXT("パネルタブ余白")},
	{RGB(170,170,170),	TEXT("PanelTitleBack"),				TEXT("パネルタイトル背景")},
	{RGB(0,0,0),		TEXT("PanelTitleText"),				TEXT("パネルタイトル文字")},
	{RGB(128,192,160),	TEXT("ProgramInfoBack"),			TEXT("番組情報背景")},
	{RGB(64,96,80),		TEXT("ProgramInfoText"),			TEXT("番組情報文字")},
	{RGB(128,192,160),	TEXT("ProgramListBack"),			TEXT("番組表背景")},
	{RGB(64,96,80),		TEXT("ProgramListText"),			TEXT("番組表文字")},
	{RGB(64,96,80),		TEXT("ProgramListTitleBack"),		TEXT("番組表番組名背景")},
	{RGB(128,192,160),	TEXT("ProgramListTitleText"),		TEXT("番組表番組名文字")},
	{RGB(64,96,80),		TEXT("ControlPanelHighlightBack"),	TEXT("操作パネル選択背景")},
	{RGB(128,192,160),	TEXT("ControlPanelHighlightText"),	TEXT("操作パネル選択文字")},
	{RGB(255,255,255),	TEXT("ProgramGuideBack"),			TEXT("EPG番組表背景")},
	{RGB(0,0,0),		TEXT("ProgramGuideText"),			TEXT("EPG番組表文字")},
	{RGB(240,240,240),	TEXT("ProgramGuideChannelBack"),	TEXT("EPG番組表チャンネル名背景")},
	{RGB(0,0,0),		TEXT("ProgramGuideChannelText"),	TEXT("EPG番組表チャンネル名文字")},
	{RGB(240,240,240),	TEXT("ProgramGuideTimeBack"),		TEXT("EPG番組表時間背景")},
	{RGB(0,0,0),		TEXT("ProgramGuideTimeText"),		TEXT("EPG番組表時間文字")},
	{RGB(255,255,224),	TEXT("EPGContentNews"),				TEXT("EPG番組表ニュース番組")},
	{RGB(224,224,255),	TEXT("EPGContentSports"),			TEXT("EPG番組表スポーツ番組")},
	{RGB(255,224,240),	TEXT("EPGContentInformation"),		TEXT("EPG番組表情報番組")},
	{RGB(255,224,224),	TEXT("EPGContentDrama"),			TEXT("EPG番組表ドラマ")},
	{RGB(224,255,224),	TEXT("EPGContentMusic"),			TEXT("EPG番組表音楽番組")},
	{RGB(224,255,255),	TEXT("EPGContentVariety"),			TEXT("EPG番組表バラエティ番組")},
	{RGB(255,240,224),	TEXT("EPGContentMovie"),			TEXT("EPG番組表映画")},
	{RGB(255,224,255),	TEXT("EPGContentAnime"),			TEXT("EPG番組表アニメ")},
	{RGB(255,255,224),	TEXT("EPGContentDocumentary"),		TEXT("EPG番組表ドキュメンタリー番組")},
	{RGB(255,240,224),	TEXT("EPGContentTheater"),			TEXT("EPG番組表演劇")},
	{RGB(224,240,255),	TEXT("EPGContentEducation"),		TEXT("EPG番組表教育番組")},
	{RGB(224,240,255),	TEXT("EPGContentWelfare"),			TEXT("EPG番組表福祉番組")},
	{RGB(240,240,240),	TEXT("EPGContentOther"),			TEXT("EPG番組表その他の番組")},
};


CColorScheme::CColorScheme()
{
	SetDefault();
	m_pszName=NULL;
	m_pszFileName=NULL;
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
}


CColorScheme::CColorScheme(const CColorScheme &ColorScheme)
{
	m_pszName=NULL;
	m_pszFileName=NULL;
	*this=ColorScheme;
}


CColorScheme::~CColorScheme()
{
	delete [] m_pszName;
	delete [] m_pszFileName;
}


CColorScheme &CColorScheme::operator=(const CColorScheme &ColorScheme)
{
	::CopyMemory(m_ColorList,ColorScheme.m_ColorList,sizeof(m_ColorList));
	ReplaceString(&m_pszName,ColorScheme.m_pszName);
	ReplaceString(&m_pszFileName,ColorScheme.m_pszFileName);
	::CopyMemory(m_LoadedFlags,ColorScheme.m_LoadedFlags,sizeof(m_LoadedFlags));
	return *this;
}


COLORREF CColorScheme::GetColor(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return CLR_INVALID;
	return m_ColorList[Type];
}


COLORREF CColorScheme::GetColor(LPCTSTR pszText) const
{
	for (int i=0;i<NUM_COLORS;i++) {
		if (::lstrcmpi(m_ColorInfoList[i].pszText,pszText)==0)
			return m_ColorList[i];
	}
	return CLR_INVALID;
}


bool CColorScheme::SetColor(int Type,COLORREF Color)
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	m_ColorList[Type]=Color;
	return true;
}


bool CColorScheme::SetName(LPCTSTR pszName)
{
	return ReplaceString(&m_pszName,pszName);
}


bool CColorScheme::Load(LPCTSTR pszFileName)
{
	CSettings Settings;
	TCHAR szName[128];
	int i;

	if (!Settings.Open(pszFileName,TEXT("ColorScheme"),CSettings::OPEN_READ))
		return false;
	if (Settings.Read(TEXT("Name"),szName,lengthof(szName)))
		SetName(szName);
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
	for (i=0;i<NUM_COLORS;i++) {
		if (Settings.ReadColor(m_ColorInfoList[i].pszText,&m_ColorList[i]))
			m_LoadedFlags[i/32]|=1<<(i%32);
	}
	SetFileName(pszFileName);
	return true;
}


bool CColorScheme::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;
	int i;

	if (!Settings.Open(pszFileName,TEXT("ColorScheme"),CSettings::OPEN_WRITE))
		return false;
	Settings.Write(TEXT("Name"),m_pszName!=NULL?m_pszName:TEXT(""));
	for (i=0;i<NUM_COLORS;i++) {
		Settings.WriteColor(m_ColorInfoList[i].pszText,m_ColorList[i]);
	}
	return true;
}


bool CColorScheme::SetFileName(LPCTSTR pszFileName)
{
	return ReplaceString(&m_pszFileName,pszFileName);
}


void CColorScheme::SetDefault()
{
	int i;

	for (i=0;i<NUM_COLORS;i++)
		m_ColorList[i]=m_ColorInfoList[i].DefaultColor;
}


LPCTSTR CColorScheme::GetColorName(int Type)
{
	if (Type<0 || Type>=NUM_COLORS)
		return NULL;
	return m_ColorInfoList[Type].pszName;
}


COLORREF CColorScheme::GetDefaultColor(int Type)
{
	if (Type<0 || Type>=NUM_COLORS)
		return CLR_INVALID;
	return m_ColorInfoList[Type].DefaultColor;
}


bool CColorScheme::IsLoaded(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	return (m_LoadedFlags[Type/32]&(1<<(Type%32)))!=0;
}




CColorSchemeList::CColorSchemeList()
{
	m_NumColorSchemes=0;
	m_ppList=NULL;
}


CColorSchemeList::~CColorSchemeList()
{
	Clear();
}


bool CColorSchemeList::Add(CColorScheme *pColorScheme)
{
	m_ppList=(CColorScheme**)realloc(m_ppList,(m_NumColorSchemes+1)*sizeof(CColorScheme*));
	m_ppList[m_NumColorSchemes++]=pColorScheme;
	return true;
}


bool CColorSchemeList::Load(LPCTSTR pszDirectory)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szFileName[MAX_PATH];

	PathCombine(szFileName,pszDirectory,TEXT("*.httheme"));
	hFind=FindFirstFile(szFileName,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			CColorScheme *pColorScheme;

			PathCombine(szFileName,pszDirectory,wfd.cFileName);
			pColorScheme=new CColorScheme;
			if (pColorScheme->Load(szFileName))
				Add(pColorScheme);
			else
				delete pColorScheme;
		} while (FindNextFile(hFind,&wfd));
	}
	return true;
}


void CColorSchemeList::Clear()
{
	if (m_ppList!=NULL) {
		int i;

		for (i=0;i<m_NumColorSchemes;i++)
			delete m_ppList[i];
		free(m_ppList);
		m_ppList=NULL;
		m_NumColorSchemes=0;
	}
}


CColorScheme *CColorSchemeList::GetColorScheme(int Index)
{
	if (Index<0 || Index>=m_NumColorSchemes)
		return NULL;
	return m_ppList[Index];
}


bool CColorSchemeList::SetColorScheme(int Index,const CColorScheme *pColorScheme)
{
	if (Index<0 || Index>=m_NumColorSchemes)
		return true;
	*m_ppList[Index]=*pColorScheme;
	return true;
}




const LPCTSTR CColorSchemeOptions::m_pszExtension=TEXT(".httheme");


CColorSchemeOptions::CColorSchemeOptions()
{
	m_pColorScheme=new CColorScheme;
	m_pApplyFunc=NULL;
}


CColorSchemeOptions::~CColorSchemeOptions()
{
	delete m_pColorScheme;
}


bool CColorSchemeOptions::Load(LPCTSTR pszFileName)
{
	return m_pColorScheme->Load(pszFileName);
}


bool CColorSchemeOptions::Save(LPCTSTR pszFileName) const
{
	return m_pColorScheme->Save(pszFileName);
}


bool CColorSchemeOptions::SetApplyCallback(ApplyFunc pCallback)
{
	m_pApplyFunc=pCallback;
	return true;
}


bool CColorSchemeOptions::ApplyColorScheme() const
{
	return Apply(m_pColorScheme);
}


bool CColorSchemeOptions::Apply(const CColorScheme *pColorScheme) const
{
	if (m_pApplyFunc==NULL)
		return false;
	return m_pApplyFunc(pColorScheme);
}


COLORREF CColorSchemeOptions::GetColor(int Type) const
{
	return m_pColorScheme->GetColor(Type);
}


COLORREF CColorSchemeOptions::GetColor(LPCTSTR pszText) const
{
	return m_pColorScheme->GetColor(pszText);
}


void CColorSchemeOptions::GetListColor(HWND hDlg,CColorScheme *pColorScheme)
{
	int i;

	for (i=0;i<CColorScheme::NUM_COLORS;i++)
		pColorScheme->SetColor(i,
				(COLORREF)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETITEMDATA,i,0));
}


bool CColorSchemeOptions::GetThemesDirectory(LPTSTR pszDirectory,int MaxLength,bool fCreate)
{
	GetAppClass().GetAppDirectory(pszDirectory);
	::PathAppend(pszDirectory,TEXT("Themes"));
	if (fCreate && !::PathIsDirectory(pszDirectory))
		::CreateDirectory(pszDirectory,NULL);
	return true;
}


CColorSchemeOptions *CColorSchemeOptions::GetThis(HWND hwnd)
{
	return static_cast<CColorSchemeOptions*>(GetProp(hwnd,TEXT("This")));
}


BOOL CALLBACK CColorSchemeOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CColorSchemeOptions *pThis=dynamic_cast<CColorSchemeOptions*>(OnInitDialog(hDlg,lParam));
			TCHAR szDirectory[MAX_PATH];
			int i;
			int Index;

			GetThemesDirectory(szDirectory,lengthof(szDirectory));
			pThis->m_PresetList.Load(szDirectory);
			for (i=0;i<pThis->m_PresetList.NumColorSchemes();i++) {
				LPCTSTR pszName=pThis->m_PresetList.GetColorScheme(i)->GetName();

				Index=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_ADDSTRING,0,
									(LPARAM)(pszName!=NULL?pszName:TEXT("")));
				SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_SETITEMDATA,Index,i);
			}
			for (i=0;i<CColorScheme::NUM_COLORS;i++) {
				SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_ADDSTRING,0,pThis->m_pColorScheme->GetColor(i));
			}
			HDC hdc=GetDC(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST));
			HFONT hfontOld=SelectFont(hdc,(HFONT)SendDlgItemMessage(hDlg,
										IDC_COLORSCHEME_LIST,WM_GETFONT,0,0));
			long MaxWidth=0;
			for (i=0;i<CColorScheme::NUM_COLORS;i++) {
				LPCTSTR pszName=CColorScheme::GetColorName(i);
				SIZE sz;
				GetTextExtentPoint32(hdc,pszName,lstrlen(pszName),&sz);
				if (sz.cx>MaxWidth)
					MaxWidth=sz.cx;
			}
			SelectFont(hdc,hfontOld);
			ReleaseDC(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST),hdc);
			SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETHORIZONTALEXTENT,
				SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETITEMHEIGHT,0,0)*2+MaxWidth+2,0);

			pThis->m_fPreview=false;

			RECT rc;
			static const RGBQUAD BaseColors[18] = {
				{0x00, 0x00, 0xFF},
				{0x00, 0x66, 0xFF},
				{0x00, 0xCC, 0xFF},
				{0x00, 0xFF, 0xFF},
				{0x00, 0xFF, 0xCC},
				{0x00, 0xFF, 0x66},
				{0x00, 0xFF, 0x00},
				{0x66, 0xFF, 0x00},
				{0xCC, 0xFF, 0x00},
				{0xFF, 0xFF, 0x00},
				{0xFF, 0xCC, 0x00},
				{0xFF, 0x66, 0x00},
				{0xFF, 0x00, 0x00},
				{0xFF, 0x00, 0x66},
				{0xFF, 0x00, 0xCC},
				{0xFF, 0x00, 0xFF},
				{0xCC, 0x00, 0xFF},
				{0x66, 0x00, 0xFF},
			};
			RGBQUAD Palette[256];
			int j,k;

			CColorPalette::Initialize(GetWindowInstance(hDlg));
			pThis->m_ColorPalette.Create(hDlg,WS_CHILD | WS_VISIBLE,0,IDC_COLORSCHEME_PALETTE);
			GetWindowRect(GetDlgItem(hDlg,IDC_COLORSCHEME_PALETTEPLACE),&rc);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rc,2);
			pThis->m_ColorPalette.SetPosition(&rc);
			for (i=0;i<lengthof(BaseColors);i++) {
				RGBQUAD Color=BaseColors[i%2*(lengthof(BaseColors)/2)+i/2];

				for (j=0;j<4;j++) {
					Palette[i*8+j].rgbBlue=(Color.rgbBlue*(j+1))/4;
					Palette[i*8+j].rgbGreen=(Color.rgbGreen*(j+1))/4;
					Palette[i*8+j].rgbRed=(Color.rgbRed*(j+1))/4;
				}
				for (;j<8;j++) {
					Palette[i*8+j].rgbBlue=Color.rgbBlue+(255-Color.rgbBlue)*(j-3)/5;
					Palette[i*8+j].rgbGreen=Color.rgbGreen+(255-Color.rgbGreen)*(j-3)/5;
					Palette[i*8+j].rgbRed=Color.rgbRed+(255-Color.rgbRed)*(j-3)/5;
				}
			}
			i=lengthof(BaseColors)*8;
			for (j=0;j<16;j++) {
				Palette[i].rgbBlue=(255*j)/15;
				Palette[i].rgbGreen=(255*j)/15;
				Palette[i].rgbRed=(255*j)/15;
				i++;
			}
			for (j=0;j<CColorScheme::NUM_COLORS;j++) {
				COLORREF cr=pThis->m_pColorScheme->GetColor(j);

				for (k=0;k<i;k++) {
					if (cr==RGB(Palette[k].rgbRed,Palette[k].rgbGreen,Palette[k].rgbBlue))
						break;
				}
				if (k==i) {
					Palette[i].rgbBlue=GetBValue(cr);
					Palette[i].rgbGreen=GetGValue(cr);
					Palette[i].rgbRed=GetRValue(cr);
					i++;
				}
			}
			if (i<lengthof(Palette))
				ZeroMemory(&Palette[i],(lengthof(Palette)-i)*sizeof(RGBQUAD));
			pThis->m_ColorPalette.SetPalette(Palette,lengthof(Palette));
		}
		return TRUE;

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			pmis->itemHeight=7*HIWORD(GetDialogBaseUnits())/8;
			if (pmis->CtlID==IDC_COLORSCHEME_PRESET)
				pmis->itemHeight+=4;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			CColorSchemeOptions *pThis=GetThis(hDlg);
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (pdis->CtlID==IDC_COLORSCHEME_PRESET) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					if ((int)pdis->itemID<0) {
						::FillRect(pdis->hDC,&pdis->rcItem,
									reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
					} else {
						CColorScheme *pColorScheme=pThis->m_PresetList.GetColorScheme(pdis->itemID);
						int BkColor1,BkColor2;

						if ((pdis->itemState&ODS_SELECTED)==0) {
							BkColor1=CColorScheme::COLOR_STATUSBACK1;
							BkColor2=CColorScheme::COLOR_STATUSBACK2;
						} else {
							BkColor1=CColorScheme::COLOR_STATUSHIGHLIGHTBACK1;
							BkColor2=CColorScheme::COLOR_STATUSHIGHLIGHTBACK2;
						}
						DrawUtil::FillGradient(pdis->hDC,&pdis->rcItem,
											   pColorScheme->GetColor(BkColor1),
											   pColorScheme->GetColor(BkColor2),
											   DrawUtil::DIRECTION_VERT);
						if (pColorScheme->GetName()!=NULL) {
							int OldBkMode;
							COLORREF crOldTextColor;
							RECT rc;
							HFONT hfont,hfontOld;

							if ((pdis->itemState&ODS_SELECTED)!=0) {
								LOGFONT lf;

								hfontOld=static_cast<HFONT>(::GetCurrentObject(pdis->hDC,OBJ_FONT));
								::GetObject(hfontOld,sizeof(LOGFONT),&lf);
								lf.lfWeight=FW_BOLD;
								hfont=::CreateFontIndirect(&lf);
								SelectFont(pdis->hDC,hfont);
							} else
								hfont=NULL;
							OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
							crOldTextColor=::SetTextColor(pdis->hDC,
								pColorScheme->GetColor(
									(pdis->itemState&ODS_SELECTED)==0?
									CColorScheme::COLOR_STATUSTEXT:
									CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
							rc=pdis->rcItem;
							rc.left+=4;
							::DrawText(pdis->hDC,pColorScheme->GetName(),-1,&rc,
								DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
							::SetTextColor(pdis->hDC,crOldTextColor);
							::SetBkMode(pdis->hDC,OldBkMode);
							if (hfont!=NULL) {
								::SelectObject(pdis->hDC,hfontOld);
								::DeleteObject(hfont);
							}
						}
					}
					if ((pdis->itemState&ODS_FOCUS)==0)
						break;
				case ODA_FOCUS:
					::DrawFocusRect(pdis->hDC,&pdis->rcItem);
					break;
				}
			} else if (pdis->CtlID==IDC_COLORSCHEME_LIST) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					{
						HBRUSH hbr,hbrOld;
						HPEN hpenOld;
						RECT rc;
						int OldBkMode;
						COLORREF crOldTextColor;

						::FillRect(pdis->hDC,&pdis->rcItem,
							reinterpret_cast<HBRUSH>(
								(pdis->itemState&ODS_SELECTED)==0?
											COLOR_WINDOW+1:COLOR_HIGHLIGHT+1));
						hbr=::CreateSolidBrush((COLORREF)pdis->itemData);
						hbrOld=SelectBrush(pdis->hDC,hbr);
						hpenOld=SelectPen(pdis->hDC,::GetStockObject(BLACK_PEN));
						rc.left=pdis->rcItem.left+2;
						rc.top=pdis->rcItem.top+2;
						rc.bottom=pdis->rcItem.bottom-2;
						rc.right=rc.left+(rc.bottom-rc.top)*2;
						::Rectangle(pdis->hDC,rc.left,rc.top,rc.right,rc.bottom);
						::SelectObject(pdis->hDC,hpenOld);
						::SelectObject(pdis->hDC,hbrOld);
						::DeleteObject(hbr);
						OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
						crOldTextColor=::SetTextColor(pdis->hDC,GetSysColor(
							(pdis->itemState&ODS_SELECTED)==0?COLOR_WINDOWTEXT:
														COLOR_HIGHLIGHTTEXT));
						rc.left=rc.right+2;
						rc.top=pdis->rcItem.top;
						rc.right=pdis->rcItem.right;
						rc.bottom=pdis->rcItem.bottom;
						::DrawText(pdis->hDC,CColorScheme::GetColorName(pdis->itemID),-1,
							&rc,DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
						::SetTextColor(pdis->hDC,crOldTextColor);
						::SetBkMode(pdis->hDC,OldBkMode);
					}
					if ((pdis->itemState&ODS_FOCUS)==0)
						break;
				case ODA_FOCUS:
					::DrawFocusRect(pdis->hDC,&pdis->rcItem);
					break;
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COLORSCHEME_PRESET:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int Sel=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETCURSEL,0,0);

				if (Sel>=0) {
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int Index=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETITEMDATA,Sel,0);
					CColorScheme *pColorScheme=pThis->m_PresetList.GetColorScheme(Index);

					if (pColorScheme!=NULL) {
						int i;

						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (pColorScheme->IsLoaded(i))
								SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,
									LB_SETITEMDATA,i,pColorScheme->GetColor(i));
						}
						SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETSEL,FALSE,-1);
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
						pThis->m_ColorPalette.SetSel(-1);
						if (pThis->Apply(pColorScheme))
							pThis->m_fPreview=true;
					}
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_SAVE:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				TCHAR szName[MAX_PATH],szFileName[MAX_PATH];
				int Index;
				CColorScheme *pColorScheme;
				LPCTSTR pszFileName;

				GetDlgItemText(hDlg,IDC_COLORSCHEME_PRESET,szName,lengthof(szName));
				if (szName[0]=='\0') {
					MessageBox(hDlg,TEXT("名前を入力してください。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				pszFileName=szName;
				Index=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_FINDSTRINGEXACT,-1,(LPARAM)szName);
				if (Index>=0) {
					pColorScheme=pThis->m_PresetList.GetColorScheme(
						SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETITEMDATA,Index,0));
					if (pColorScheme!=NULL)
						pszFileName=PathFindFileName(pColorScheme->GetFileName());
				}
				GetThemesDirectory(szFileName,lengthof(szFileName),true);
				if (lstrlen(szFileName)+1+lstrlen(pszFileName)+lstrlen(m_pszExtension)>=MAX_PATH) {
					MessageBox(hDlg,TEXT("名前が長すぎます。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				PathAppend(szFileName,pszFileName);
				if (pszFileName==szName)
					lstrcat(szFileName,m_pszExtension);
				pColorScheme=new CColorScheme;
				pColorScheme->SetName(szName);
				GetListColor(hDlg,pColorScheme);
				if (!pColorScheme->Save(szFileName)) {
					delete pColorScheme;
					MessageBox(hDlg,TEXT("保存ができません。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if (Index<0) {
					pColorScheme->SetFileName(szFileName);
					pThis->m_PresetList.Add(pColorScheme);
					Index=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_ADDSTRING,0,(LPARAM)szName);
					SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_SETITEMDATA,Index,pThis->m_PresetList.NumColorSchemes()-1);
				} else {
					pThis->m_PresetList.SetColorScheme(Index,pColorScheme);
					delete pColorScheme;
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_DELETE:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				TCHAR szName[MAX_PATH],szFileName[MAX_PATH];
				int Index;
				CColorScheme *pColorScheme;
				LPCTSTR pszFileName;

				GetDlgItemText(hDlg,IDC_COLORSCHEME_PRESET,szName,lengthof(szName));
				if (szName[0]=='\0')
					break;
				Index=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_FINDSTRINGEXACT,-1,(LPARAM)szName);
				if (Index<0
						|| (pColorScheme=pThis->m_PresetList.GetColorScheme(
							SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETITEMDATA,Index,0)))==NULL)
					break;
				GetThemesDirectory(szFileName,lengthof(szFileName));
				pszFileName=PathFindFileName(pColorScheme->GetFileName());
				if (lstrlen(szFileName)+1+lstrlen(pszFileName)+lstrlen(m_pszExtension)>=MAX_PATH)
					break;
				PathAppend(szFileName,pszFileName);
				if (pszFileName==szName)
					lstrcat(szFileName,m_pszExtension);
				if (!DeleteFile(szFileName)) {
					MessageBox(hDlg,TEXT("ファイルを削除できません。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_DELETESTRING,Index,0);
			}
			return TRUE;

		case IDC_COLORSCHEME_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				{
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int SelCount=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETSELCOUNT,0,0);
					int i;
					COLORREF SelColor=CLR_INVALID,Color;

					if (SelCount==0) {
						pThis->m_ColorPalette.SetSel(-1);
						break;
					}
					if (SelCount==1) {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if ((LONG)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETSEL,i,0)>0) {
								SelColor=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETITEMDATA,i,0);
								break;
							}
						}
					} else {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if ((LONG)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETSEL,i,0)>0) {
								Color=SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETITEMDATA,i,0);
								if (SelColor==CLR_INVALID)
									SelColor=Color;
								else if (Color!=SelColor)
									break;
							}
						}
						if (i<CColorScheme::NUM_COLORS) {
							pThis->m_ColorPalette.SetSel(-1);
							break;
						}
					}
					if (SelColor!=CLR_INVALID)
						pThis->m_ColorPalette.SetSel(
									pThis->m_ColorPalette.FindColor(SelColor));
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PALETTE:
			switch (HIWORD(wParam)) {
			case CColorPalette::NOTIFY_SELCHANGE:
				{
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int Sel=pThis->m_ColorPalette.GetSel();
					COLORREF Color=pThis->m_ColorPalette.GetColor(Sel);
					int i;

					for (i=0;i<CColorScheme::NUM_COLORS;i++) {
						if ((LONG)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETSEL,i,0)>0)
							SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETITEMDATA,i,Color);
					}
					InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
				}
				break;
			case CColorPalette::NOTIFY_DOUBLECLICK:
				{
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int Sel=pThis->m_ColorPalette.GetSel();
					COLORREF Color=pThis->m_ColorPalette.GetColor(Sel);

					if (ChooseColorDialog(hDlg,&Color)) {
						pThis->m_ColorPalette.SetColor(Sel,Color);
						int i;

						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if ((LONG)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETSEL,i,0)>0)
								SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETITEMDATA,i,Color);
						}
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
					}
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PREVIEW:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				CColorScheme ColorScheme;

				GetListColor(hDlg,&ColorScheme);
				if (pThis->Apply(&ColorScheme))
					pThis->m_fPreview=true;
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				GetListColor(hDlg,pThis->m_pColorScheme);
				pThis->Apply(pThis->m_pColorScheme);
			}
			break;

		case PSN_RESET:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				if (pThis->m_fPreview)
					pThis->Apply(pThis->m_pColorScheme);
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CColorSchemeOptions *pThis=GetThis(hDlg);

			pThis->m_PresetList.Clear();
			pThis->OnDestroy();
		}
		break;
	}
	return FALSE;
}

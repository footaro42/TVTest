#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ColorScheme.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define HEXRGB(hex) RGB((hex)>>16,((hex)>>8)&0xFF,(hex)&0xFF)

const CColorScheme::ColorInfo CColorScheme::m_ColorInfoList[NUM_COLORS] = {
	{HEXRGB(0x777777),	TEXT("StatusBack"),							TEXT("ステータスバー背景1")},
	{HEXRGB(0x222222),	TEXT("StatusBack2"),						TEXT("ステータスバー背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("StatusText"),							TEXT("ステータスバー文字")},
	{HEXRGB(0x3384FF),	TEXT("StatusHighlightBack"),				TEXT("ステータスバー選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("StatusHighlightBack2"),				TEXT("ステータスバー選択背景2")},
	{HEXRGB(0x333333),	TEXT("StatusHighlightText"),				TEXT("ステータスバー選択文字")},
	{HEXRGB(0xDF3F00),	TEXT("StatusRecordingCircle"),				TEXT("ステータスバー録画●")},
	{HEXRGB(0x666666),	TEXT("PanelBack"),							TEXT("パネル背景")},
	{HEXRGB(0xCCCCCC),	TEXT("PanelText"),							TEXT("パネル文字")},
	{HEXRGB(0xCCCCCC),	TEXT("PanelTabBack"),						TEXT("パネルタブ背景1")},
	{HEXRGB(0x888888),	TEXT("PanelTabBack2"),						TEXT("パネルタブ背景2")},
	{HEXRGB(0x444444),	TEXT("PanelTabText"),						TEXT("パネルタブ文字")},
	{HEXRGB(0x444444),	TEXT("PanelTabBorder"),						TEXT("パネルタブ枠")},
	{HEXRGB(0x999999),	TEXT("PanelCurTabBack"),					TEXT("パネル選択タブ背景1")},
	{HEXRGB(0x666666),	TEXT("PanelCurTabBack2"),					TEXT("パネル選択タブ背景2")},
	{HEXRGB(0xEEEEEE),	TEXT("PanelCurTabText"),					TEXT("パネル選択タブ文字")},
	{HEXRGB(0x444444),	TEXT("PanelCurTabBorder"),					TEXT("パネル選択タブ枠")},
	{HEXRGB(0x888888),	TEXT("PanelTabMargin"),						TEXT("パネルタブ余白")},
	{HEXRGB(0x777777),	TEXT("PanelTitleBack"),						TEXT("パネルタイトル背景1")},
	{HEXRGB(0x222222),	TEXT("PanelTitleBack2"),					TEXT("パネルタイトル背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("PanelTitleText"),						TEXT("パネルタイトル文字")},
	{HEXRGB(0x777777),	TEXT("ProgramInfoBack"),					TEXT("情報パネル番組背景")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramInfoText"),					TEXT("情報パネル番組文字")},
	{HEXRGB(0x777777),	TEXT("ProgramListBack"),					TEXT("番組表パネル背景1")},
	{HEXRGB(0x888888),	TEXT("ProgramListBack2"),					TEXT("番組表パネル背景2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramListText"),					TEXT("番組表パネル文字")},
	{HEXRGB(0x777777),	TEXT("ProgramListCurBack"),					TEXT("番組表パネル強調背景1")},
	{HEXRGB(0x888888),	TEXT("ProgramListCurBack2"),				TEXT("番組表パネル強調背景2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramListCurText"),					TEXT("番組表パネル強調文字")},
	{HEXRGB(0x777777),	TEXT("ProgramListTitleBack"),				TEXT("番組表パネル番組名背景1")},
	{HEXRGB(0x555555),	TEXT("ProgramListTitleBack2"),				TEXT("番組表パネル番組名背景2")},
	{HEXRGB(0xCCCCCC),	TEXT("ProgramListTitleText"),				TEXT("番組表パネル番組名文字")},
	{HEXRGB(0x3384FF),	TEXT("ProgramListCurTitleBack"),			TEXT("番組表パネル番組名強調背景1")},
	{HEXRGB(0x33D6FF),	TEXT("ProgramListCurTitleBack2"),			TEXT("番組表パネル番組名強調背景2")},
	{HEXRGB(0x333333),	TEXT("ProgramListCurTitleText"),			TEXT("番組表パネル番組名強調文字")},
	{HEXRGB(0x777777),	TEXT("ChannelPanelChannelNameBack"),		TEXT("チャンネル名背景1")},
	{HEXRGB(0x555555),	TEXT("ChannelPanelChannelNameBack2"),		TEXT("チャンネル名背景2")},
	{HEXRGB(0xCCCCCC),	TEXT("ChannelPanelChannelNameText"),		TEXT("チャンネル名文字")},
	{HEXRGB(0x3384FF),	TEXT("ChannelPanelCurChannelNameBack"),		TEXT("チャンネル名選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("ChannelPanelCurChannelNameBack2"),	TEXT("チャンネル名選択背景2")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelCurChannelNameText"),		TEXT("チャンネル名選択文字")},
	{HEXRGB(0x777777),	TEXT("ChannelPanelEventNameBack"),			TEXT("チャンネル番組名背景1")},
	{HEXRGB(0x888888),	TEXT("ChannelPanelEventNameBack2"),			TEXT("チャンネル番組名背景2")},
	{HEXRGB(0xDDDDDD),	TEXT("ChannelPanelEventNameText"),			TEXT("チャンネル番組名文字")},
	{HEXRGB(0x777777),	TEXT("ChannelPanelCurEventNameBack"),		TEXT("チャンネル番組名選択背景1")},
	{HEXRGB(0x888888),	TEXT("ChannelPanelCurEventNameBack2"),		TEXT("チャンネル番組名選択背景2")},
	{HEXRGB(0xDDDDDD),	TEXT("ChannelPanelCurEventNameText"),		TEXT("チャンネル番組名選択文字")},
	{HEXRGB(0x666666),	TEXT("ControlPanelBack"),					TEXT("操作パネル背景1")},
	{HEXRGB(0x666666),	TEXT("ControlPanelBack2"),					TEXT("操作パネル背景2")},
	{HEXRGB(0xCCCCCC),	TEXT("ControlPanelText"),					TEXT("操作パネル文字")},
	{HEXRGB(0x3384FF),	TEXT("ControlPanelHighlightBack"),			TEXT("操作パネル選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("ControlPanelHighlightBack2"),			TEXT("操作パネル選択背景2")},
	{HEXRGB(0xEEEEEE),	TEXT("ControlPanelHighlightText"),			TEXT("操作パネル選択文字")},
	{HEXRGB(0x666666),	TEXT("ControlPanelMargin"),					TEXT("操作パネル余白")},
	{HEXRGB(0x777777),	TEXT("CaptionPanelBack"),					TEXT("字幕パネル背景")},
	{HEXRGB(0xDDDDDD),	TEXT("CaptionPanelText"),					TEXT("字幕パネル文字")},
	{HEXRGB(0x777777),	TEXT("TitleBarBack"),						TEXT("タイトルバー背景1")},
	{HEXRGB(0x222222),	TEXT("TitleBarBack2"),						TEXT("タイトルバー背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("TitleBarText"),						TEXT("タイトルバー文字")},
	{HEXRGB(0x3384FF),	TEXT("TitleBarHighlightBack"),				TEXT("タイトルバー選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("TitleBarHighlightBack2"),				TEXT("タイトルバー選択背景2")},
	{HEXRGB(0x444444),	TEXT("TitleBarHighlightIcon"),				TEXT("タイトルバー選択アイコン")},
	{HEXRGB(0x777777),	TEXT("SideBarBack"),						TEXT("サイドバー背景1")},
	{HEXRGB(0x222222),	TEXT("SideBarBack2"),						TEXT("サイドバー背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("SideBarIcon"),						TEXT("サイドバーアイコン")},
	{HEXRGB(0x3384FF),	TEXT("SideBarHighlightBack"),				TEXT("サイドバー選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("SideBarHighlightBack2"),				TEXT("サイドバー選択背景2")},
	{HEXRGB(0x444444),	TEXT("SideBarHighlightIcon"),				TEXT("サイドバー選択アイコン")},
	{HEXRGB(0x222222),	TEXT("NotificationBarBack"),				TEXT("通知バー背景1")},
	{HEXRGB(0x333333),	TEXT("NotificationBarBack2"),				TEXT("通知バー背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("NotificationBarText"),				TEXT("通知バー文字")},
	{HEXRGB(0xFF4444),	TEXT("NotificationBarErrorText"),			TEXT("通知バーエラー文字")},
	{HEXRGB(0x666666),	TEXT("ProgramGuideBack"),					TEXT("EPG番組表背景")},
	{HEXRGB(0x000000),	TEXT("ProgramGuideText"),					TEXT("EPG番組表文字")},
	{HEXRGB(0x777777),	TEXT("ProgramGuideChannelBack"),			TEXT("EPG番組表チャンネル名背景1")},
	{HEXRGB(0x222222),	TEXT("ProgramGuideChannelBack2"),			TEXT("EPG番組表チャンネル名背景2")},
	{HEXRGB(0xBBBBBB),	TEXT("ProgramGuideChannelText"),			TEXT("EPG番組表チャンネル名文字")},
	{HEXRGB(0x3384FF),	TEXT("ProgramGuideCurChannelBack"),			TEXT("EPG番組表チャンネル名選択背景1")},
	{HEXRGB(0x33D6FF),	TEXT("ProgramGuideCurChannelBack2"),		TEXT("EPG番組表チャンネル名選択背景2")},
	{HEXRGB(0x333333),	TEXT("ProgramGuideCurChannelText"),			TEXT("EPG番組表チャンネル名選択文字")},
	{HEXRGB(0x888888),	TEXT("ProgramGuideTimeBack"),				TEXT("EPG番組表日時背景1")},
	{HEXRGB(0x777777),	TEXT("ProgramGuideTimeBack2"),				TEXT("EPG番組表日時背景2")},
	{HEXRGB(0x004CBF),	TEXT("ProgramGuideTime0To2Back"),			TEXT("EPG番組表0～2時背景1")},
	{HEXRGB(0x00337F),	TEXT("ProgramGuideTime0To2Back2"),			TEXT("EPG番組表0～2時背景2")},
	{HEXRGB(0x0099BF),	TEXT("ProgramGuideTime3To5Back"),			TEXT("EPG番組表3～5時背景1")},
	{HEXRGB(0x00667F),	TEXT("ProgramGuideTime3To5Back2"),			TEXT("EPG番組表3～5時背景2")},
	{HEXRGB(0x00BF99),	TEXT("ProgramGuideTime6To8Back"),			TEXT("EPG番組表6～8時背景1")},
	{HEXRGB(0x007F66),	TEXT("ProgramGuideTime6To8Back2"),			TEXT("EPG番組表6～8時背景2")},
	{HEXRGB(0x99BF00),	TEXT("ProgramGuideTime9To11Back"),			TEXT("EPG番組表9～11時背景1")},
	{HEXRGB(0x667F00),	TEXT("ProgramGuideTime9To11Back2"),			TEXT("EPG番組表9～11時背景2")},
	{HEXRGB(0xBF9900),	TEXT("ProgramGuideTime12To14Back"),			TEXT("EPG番組表12～14時背景1")},
	{HEXRGB(0x7F6600),	TEXT("ProgramGuideTime12To14Back2"),		TEXT("EPG番組表12～14時背景2")},
	{HEXRGB(0xBF4C00),	TEXT("ProgramGuideTime15To17Back"),			TEXT("EPG番組表15～17時背景1")},
	{HEXRGB(0x7F3300),	TEXT("ProgramGuideTime15To17Back2"),		TEXT("EPG番組表15～17間背景2")},
	{HEXRGB(0xBF0099),	TEXT("ProgramGuideTime18To20Back"),			TEXT("EPG番組表18～20時背景1")},
	{HEXRGB(0x7F0066),	TEXT("ProgramGuideTime18To20Back2"),		TEXT("EPG番組表18～20時背景2")},
	{HEXRGB(0x9900BF),	TEXT("ProgramGuideTime21To23Back"),			TEXT("EPG番組表21～23時背景1")},
	{HEXRGB(0x66007F),	TEXT("ProgramGuideTime21To23Back2"),		TEXT("EPG番組表21～23時背景2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramGuideTimeText"),				TEXT("EPG番組表時間文字")},
	{HEXRGB(0xBBBBBB),	TEXT("ProgramGuideTimeLine"),				TEXT("EPG番組表時間線")},
	{HEXRGB(0xFF6600),	TEXT("ProgramGuideCurTimeLine"),			TEXT("EPG番組表現在時刻線")},
	{RGB(255,255,224),	TEXT("EPGContentNews"),						TEXT("EPG番組表ニュース番組")},
	{RGB(224,224,255),	TEXT("EPGContentSports"),					TEXT("EPG番組表スポーツ番組")},
	{RGB(255,224,240),	TEXT("EPGContentInformation"),				TEXT("EPG番組表情報番組")},
	{RGB(255,224,224),	TEXT("EPGContentDrama"),					TEXT("EPG番組表ドラマ")},
	{RGB(224,255,224),	TEXT("EPGContentMusic"),					TEXT("EPG番組表音楽番組")},
	{RGB(224,255,255),	TEXT("EPGContentVariety"),					TEXT("EPG番組表バラエティ番組")},
	{RGB(255,240,224),	TEXT("EPGContentMovie"),					TEXT("EPG番組表映画")},
	{RGB(255,224,255),	TEXT("EPGContentAnime"),					TEXT("EPG番組表アニメ")},
	{RGB(255,255,224),	TEXT("EPGContentDocumentary"),				TEXT("EPG番組表ドキュメンタリー番組")},
	{RGB(255,240,224),	TEXT("EPGContentTheater"),					TEXT("EPG番組表演劇")},
	{RGB(224,240,255),	TEXT("EPGContentEducation"),				TEXT("EPG番組表教育番組")},
	{RGB(224,240,255),	TEXT("EPGContentWelfare"),					TEXT("EPG番組表福祉番組")},
	{RGB(240,240,240),	TEXT("EPGContentOther"),					TEXT("EPG番組表その他の番組")},
};

const CColorScheme::GradientInfo CColorScheme::m_GradientInfoList[NUM_GRADIENTS] = {
	{TEXT("StatusBackGradient"),						Theme::DIRECTION_VERT,
		COLOR_STATUSBACK1,						COLOR_STATUSBACK2},
	{TEXT("StatusHighlightBackGradient"),				Theme::DIRECTION_VERT,
		COLOR_STATUSHIGHLIGHTBACK1,				COLOR_STATUSHIGHLIGHTBACK2},
	{TEXT("PanelTabBackGradient"),						Theme::DIRECTION_VERT,
		COLOR_PANELTABBACK1,					COLOR_PANELTABBACK2},
	{TEXT("PanelCurTabBackGradient"),					Theme::DIRECTION_VERT,
		COLOR_PANELCURTABBACK1,					COLOR_PANELCURTABBACK2},
	{TEXT("PanelTitleBackGradient"),					Theme::DIRECTION_VERT,
		COLOR_PANELTITLEBACK1,					COLOR_PANELTITLEBACK2},
	{TEXT("ProgramListBackGradient"),					Theme::DIRECTION_VERT,
		COLOR_PROGRAMLISTBACK1,					COLOR_PROGRAMLISTBACK2},
	{TEXT("ProgramListCurBackGradient"),				Theme::DIRECTION_VERT,
		COLOR_PROGRAMLISTCURBACK1,				COLOR_PROGRAMLISTCURBACK2},
	{TEXT("ProgramListTitleBackGradient"),				Theme::DIRECTION_VERT,
		COLOR_PROGRAMLISTTITLEBACK1,			COLOR_PROGRAMLISTTITLEBACK2},
	{TEXT("ProgramListCurTitleBackGradient"),			Theme::DIRECTION_VERT,
		COLOR_PROGRAMLISTCURTITLEBACK1,			COLOR_PROGRAMLISTCURTITLEBACK2},
	{TEXT("ChannelPanelChannelNameBackGradient"),		Theme::DIRECTION_VERT,
		COLOR_CHANNELPANELCHANNELNAMEBACK1,		COLOR_CHANNELPANELCHANNELNAMEBACK2},
	{TEXT("ChannelPanelCurChannelNameBackGradient"),	Theme::DIRECTION_VERT,
		COLOR_CHANNELPANELCURCHANNELNAMEBACK1,	COLOR_CHANNELPANELCURCHANNELNAMEBACK2},
	{TEXT("ChannelPanelEventNameBackGradient"),			Theme::DIRECTION_VERT,
		COLOR_CHANNELPANELEVENTNAMEBACK1,		COLOR_CHANNELPANELEVENTNAMEBACK2},
	{TEXT("ChannelPanelCurEventNameBackGradient"),		Theme::DIRECTION_VERT,
		COLOR_CHANNELPANELCUREVENTNAMEBACK1,	COLOR_CHANNELPANELCUREVENTNAMEBACK2},
	{TEXT("ControlPanelBackGradient"),					Theme::DIRECTION_VERT,
		COLOR_CONTROLPANELBACK1,				COLOR_CONTROLPANELBACK2},
	{TEXT("ControlPanelHighlightBackGradient"),			Theme::DIRECTION_VERT,
		COLOR_CONTROLPANELHIGHLIGHTBACK1,		COLOR_CONTROLPANELHIGHLIGHTBACK2},
	{TEXT("TitleBarBackGradient"),						Theme::DIRECTION_VERT,
		COLOR_TITLEBARBACK1,					COLOR_TITLEBARBACK2},
	{TEXT("TitleBarHighlightBackGradient"),				Theme::DIRECTION_VERT,
		COLOR_TITLEBARHIGHLIGHTBACK1,			COLOR_TITLEBARHIGHLIGHTBACK2},
	{TEXT("SideBarBackGradient"),						Theme::DIRECTION_HORZ,
		COLOR_SIDEBARBACK1,						COLOR_SIDEBARBACK2},
	{TEXT("SideBarHighlightBackGradient"),				Theme::DIRECTION_HORZ,
		COLOR_SIDEBARHIGHLIGHTBACK1,			COLOR_SIDEBARHIGHLIGHTBACK2},
	{TEXT("NotificationBarBackGradient"),				Theme::DIRECTION_VERT,
		COLOR_NOTIFICATIONBARBACK1,				COLOR_NOTIFICATIONBARBACK2},
	{TEXT("ProgramGuideChannelBackGradient"),			Theme::DIRECTION_VERT,
		COLOR_PROGRAMGUIDECHANNELBACK1,			COLOR_PROGRAMGUIDECHANNELBACK2},
	{TEXT("ProgramGuideCurChannelBackGradient"),		Theme::DIRECTION_VERT,
		COLOR_PROGRAMGUIDECURCHANNELBACK1,		COLOR_PROGRAMGUIDECURCHANNELBACK2},
	{TEXT("ProgramGuideTimeBackGradient"),				Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK1,			COLOR_PROGRAMGUIDETIMEBACK2},
	{TEXT("ProgramGuideTime0To2BackGradient"),			Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_0TO2_1,		COLOR_PROGRAMGUIDETIMEBACK_0TO2_2},
	{TEXT("ProgramGuideTime3To5BackGradient"),			Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_3TO5_1,		COLOR_PROGRAMGUIDETIMEBACK_3TO5_2},
	{TEXT("ProgramGuideTime6To8BackGradient"),			Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_6TO8_1,		COLOR_PROGRAMGUIDETIMEBACK_6TO8_2},
	{TEXT("ProgramGuideTime9To11BackGradient"),			Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_9TO11_1,		COLOR_PROGRAMGUIDETIMEBACK_9TO11_2},
	{TEXT("ProgramGuideTime12To14BackGradient"),		Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_12TO14_1,	COLOR_PROGRAMGUIDETIMEBACK_12TO14_2},
	{TEXT("ProgramGuideTime15To17BackGradient"),		Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_15TO17_1,	COLOR_PROGRAMGUIDETIMEBACK_15TO17_2},
	{TEXT("ProgramGuideTime18To20BackGradient"),		Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_18TO20_1,	COLOR_PROGRAMGUIDETIMEBACK_18TO20_2},
	{TEXT("ProgramGuideTime21To23BackGradient"),		Theme::DIRECTION_HORZ,
		COLOR_PROGRAMGUIDETIMEBACK_21TO23_1,	COLOR_PROGRAMGUIDETIMEBACK_21TO23_2},
};

const CColorScheme::BorderInfo CColorScheme::m_BorderInfoList[NUM_BORDERS] = {
	{TEXT("StatusBorder"),		Theme::BORDER_RAISED},
	{TEXT("TitleBarBorder"),	Theme::BORDER_RAISED},
	{TEXT("SideBarBorder"),		Theme::BORDER_RAISED},
};


CColorScheme::CColorScheme()
	: m_pszName(NULL)
	, m_pszFileName(NULL)
{
	SetDefault();
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
}


CColorScheme::CColorScheme(const CColorScheme &ColorScheme)
	: m_pszName(NULL)
	, m_pszFileName(NULL)
{
	*this=ColorScheme;
}


CColorScheme::~CColorScheme()
{
	delete [] m_pszName;
	delete [] m_pszFileName;
}


CColorScheme &CColorScheme::operator=(const CColorScheme &ColorScheme)
{
	if (&ColorScheme!=this) {
		::CopyMemory(m_ColorList,ColorScheme.m_ColorList,sizeof(m_ColorList));
		::CopyMemory(m_GradientList,ColorScheme.m_GradientList,sizeof(m_GradientList));
		::CopyMemory(m_BorderList,ColorScheme.m_BorderList,sizeof(m_BorderList));
		ReplaceString(&m_pszName,ColorScheme.m_pszName);
		ReplaceString(&m_pszFileName,ColorScheme.m_pszFileName);
		::CopyMemory(m_LoadedFlags,ColorScheme.m_LoadedFlags,sizeof(m_LoadedFlags));
	}
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


CColorScheme::GradientType CColorScheme::GetGradientType(int Gradient) const
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return GRADIENT_TYPE_UNDEFINED;
	return m_GradientList[Gradient];
}


CColorScheme::GradientType CColorScheme::GetGradientType(LPCTSTR pszText) const
{
	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (::lstrcmpi(m_GradientInfoList[i].pszText,pszText)==0)
			return m_GradientList[i];
	}
	return GRADIENT_TYPE_UNDEFINED;
}


bool CColorScheme::SetGradientType(int Gradient,GradientType Type)
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS
			|| Type<GRADIENT_TYPE_FIRST || Type>GRADIENT_TYPE_LAST)
		return false;
	m_GradientList[Gradient]=Type;
	return true;
}


bool CColorScheme::GetGradientInfo(int Gradient,Theme::GradientInfo *pInfo) const
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	pInfo->Type=(Theme::GradientType)m_GradientList[Gradient];
	pInfo->Direction=m_GradientInfoList[Gradient].Direction;
	pInfo->Color1=m_ColorList[m_GradientInfoList[Gradient].Color1];
	pInfo->Color2=m_ColorList[m_GradientInfoList[Gradient].Color2];
	return true;
}


Theme::BorderType CColorScheme::GetBorderType(int Border) const
{
	if (Border<0 || Border>=NUM_BORDERS)
		return Theme::BORDER_FLAT;
	return m_BorderList[Border];
}


bool CColorScheme::SetBorderType(int Border,Theme::BorderType Type)
{
	if (Border<0 || Border>=NUM_BORDERS
			|| Type<Theme::BORDER_FLAT || Type>Theme::BORDER_RAISED)
		return false;
	m_BorderList[Border]=Type;
	return true;
}


bool CColorScheme::SetName(LPCTSTR pszName)
{
	return ReplaceString(&m_pszName,pszName);
}


bool CColorScheme::Load(LPCTSTR pszFileName)
{
	CSettings Settings;
	TCHAR szText[128];
	int i;

	if (!Settings.Open(pszFileName,TEXT("ColorScheme"),CSettings::OPEN_READ))
		return false;
	if (Settings.Read(TEXT("Name"),szText,lengthof(szText)))
		SetName(szText);
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
	for (i=0;i<NUM_COLORS;i++) {
		if (Settings.ReadColor(m_ColorInfoList[i].pszText,&m_ColorList[i]))
			SetLoadedFlag(i);
	}
	for (i=0;i<NUM_COLORS;i++) {
		if (IsLoaded(i)) {
			for (int j=0;j<NUM_GRADIENTS;j++) {
				if (m_GradientInfoList[j].Color1==i
						|| m_GradientInfoList[j].Color2==i) {
					if (m_GradientInfoList[j].Color1==i
							&& !IsLoaded(m_GradientInfoList[j].Color2)) {
						m_ColorList[m_GradientInfoList[j].Color2]=m_ColorList[i];
						SetLoadedFlag(m_GradientInfoList[j].Color2);
					}
					m_GradientList[j]=GRADIENT_TYPE_NORMAL;
					break;
				}
			}
		} else {
			static const struct {
				int To,From;
			} Map[] = {
				{COLOR_PROGRAMLISTCURBACK1,				COLOR_PROGRAMLISTBACK1},
				{COLOR_PROGRAMLISTCURBACK2,				COLOR_PROGRAMLISTBACK2},
				{COLOR_PROGRAMLISTCURTEXT,				COLOR_PROGRAMLISTTEXT},
				{COLOR_PROGRAMLISTCURTITLEBACK1,		COLOR_PROGRAMLISTTITLEBACK1},
				{COLOR_PROGRAMLISTCURTITLEBACK2,		COLOR_PROGRAMLISTTITLEBACK2},
				{COLOR_PROGRAMLISTCURTITLETEXT,			COLOR_PROGRAMLISTTITLETEXT},
				{COLOR_CHANNELPANELCURCHANNELNAMEBACK1,	COLOR_CHANNELPANELCHANNELNAMEBACK1},
				{COLOR_CHANNELPANELCURCHANNELNAMEBACK2,	COLOR_CHANNELPANELCHANNELNAMEBACK2},
				{COLOR_CHANNELPANELCURCHANNELNAMETEXT,	COLOR_CHANNELPANELCHANNELNAMETEXT},
				{COLOR_CHANNELPANELCUREVENTNAMEBACK1,	COLOR_CHANNELPANELEVENTNAMEBACK1},
				{COLOR_CHANNELPANELCUREVENTNAMEBACK2,	COLOR_CHANNELPANELEVENTNAMEBACK2},
				{COLOR_CHANNELPANELCUREVENTNAMETEXT,	COLOR_CHANNELPANELEVENTNAMETEXT},
				{COLOR_CONTROLPANELBACK1,				COLOR_PANELBACK},
				{COLOR_CONTROLPANELBACK2,				COLOR_PANELBACK},
				{COLOR_CONTROLPANELTEXT,				COLOR_PANELTEXT},
				{COLOR_CONTROLPANELMARGIN,				COLOR_PANELBACK},
				{COLOR_CAPTIONPANELBACK,				COLOR_PROGRAMINFOBACK},
				{COLOR_CAPTIONPANELTEXT,				COLOR_PROGRAMINFOTEXT},
				{COLOR_TITLEBARBACK1,					COLOR_STATUSBACK1},
				{COLOR_TITLEBARBACK2,					COLOR_STATUSBACK2},
				{COLOR_TITLEBARTEXT,					COLOR_STATUSTEXT},
				{COLOR_TITLEBARHIGHLIGHTBACK1,			COLOR_STATUSHIGHLIGHTBACK1},
				{COLOR_TITLEBARHIGHLIGHTBACK2,			COLOR_STATUSHIGHLIGHTBACK2},
				{COLOR_TITLEBARHIGHLIGHTICON,			COLOR_STATUSHIGHLIGHTTEXT},
				{COLOR_SIDEBARBACK1,					COLOR_STATUSBACK1},
				{COLOR_SIDEBARBACK2,					COLOR_STATUSBACK2},
				{COLOR_SIDEBARICON,						COLOR_STATUSTEXT},
				{COLOR_SIDEBARHIGHLIGHTBACK1,			COLOR_STATUSHIGHLIGHTBACK1},
				{COLOR_SIDEBARHIGHLIGHTBACK2,			COLOR_STATUSHIGHLIGHTBACK2},
				{COLOR_SIDEBARHIGHLIGHTICON,			COLOR_STATUSHIGHLIGHTTEXT},
				{COLOR_PROGRAMGUIDECURCHANNELBACK1,		COLOR_PROGRAMGUIDECHANNELBACK1},
				{COLOR_PROGRAMGUIDECURCHANNELBACK2,		COLOR_PROGRAMGUIDECHANNELBACK2},
				{COLOR_PROGRAMGUIDECURCHANNELTEXT,		COLOR_PROGRAMGUIDECHANNELTEXT},
				{COLOR_PROGRAMGUIDETIMELINE,			COLOR_PROGRAMGUIDETIMETEXT},
			};

			for (int j=0;j<lengthof(Map);j++) {
				if (Map[j].To==i && IsLoaded(Map[j].From)) {
					m_ColorList[i]=m_ColorList[Map[j].From];
					SetLoadedFlag(i);
					break;
				}
			}
		}
	}
	for (i=0;i<NUM_GRADIENTS;i++) {
		if (Settings.Read(m_GradientInfoList[i].pszText,szText,lengthof(szText))) {
			if (szText[0]=='\0' || ::lstrcmpi(szText,TEXT("normal"))==0)
				m_GradientList[i]=GRADIENT_TYPE_NORMAL;
			else if (::lstrcmpi(szText,TEXT("glossy"))==0)
				m_GradientList[i]=GRADIENT_TYPE_GLOSSY;
		}
	}
	Settings.Close();

	for (i=0;i<NUM_BORDERS;i++)
		m_BorderList[i]=m_BorderInfoList[i].DefaultType;
	if (Settings.Open(pszFileName,TEXT("Style"),CSettings::OPEN_READ)) {
		for (i=0;i<NUM_BORDERS;i++) {
			if (Settings.Read(m_BorderInfoList[i].pszText,szText,lengthof(szText))) {
				if (::lstrcmpi(szText,TEXT("flat"))==0)
					m_BorderList[i]=Theme::BORDER_FLAT;
				else if (::lstrcmpi(szText,TEXT("sunken"))==0)
					m_BorderList[i]=Theme::BORDER_SUNKEN;
				else if (::lstrcmpi(szText,TEXT("raised"))==0)
					m_BorderList[i]=Theme::BORDER_RAISED;
			}
		}
		Settings.Close();
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
	for (i=0;i<NUM_COLORS;i++)
		Settings.WriteColor(m_ColorInfoList[i].pszText,m_ColorList[i]);
	for (i=0;i<NUM_GRADIENTS;i++) {
		static const LPCTSTR pszTypeName[] = {
			TEXT("normal"),	TEXT("glossy")
		};

		Settings.Write(m_GradientInfoList[i].pszText,pszTypeName[m_GradientList[i]]);
	}
	Settings.Close();

	if (Settings.Open(pszFileName,TEXT("Style"),CSettings::OPEN_WRITE)) {
		static const LPCTSTR pszTypeName[] = {
			TEXT("flat"),	TEXT("sunken"),	TEXT("raised")
		};

		for (i=0;i<NUM_BORDERS;i++)
			Settings.Write(m_BorderInfoList[i].pszText,pszTypeName[m_BorderList[i]]);
		Settings.Close();
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
	for (i=0;i<NUM_GRADIENTS;i++)
		m_GradientList[i]=GRADIENT_TYPE_NORMAL;
	for (i=0;i<NUM_BORDERS;i++)
		m_BorderList[i]=m_BorderInfoList[i].DefaultType;
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


CColorScheme::GradientType CColorScheme::GetDefaultGradientType(int Gradient)
{
	return GRADIENT_TYPE_NORMAL;
}


Theme::BorderType CColorScheme::GetDefaultBorderType(int Border)
{
	if (Border<0 || Border>=NUM_BORDERS)
		return Theme::BORDER_FLAT;
	return m_BorderInfoList[Border].DefaultType;
}


bool CColorScheme::IsLoaded(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	return (m_LoadedFlags[Type/32]&(1<<(Type%32)))!=0;
}


void CColorScheme::SetLoaded()
{
	::FillMemory(m_LoadedFlags,sizeof(m_LoadedFlags),0xFF);
}


int CColorScheme::GetColorGradient(int Type)
{
	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (m_GradientInfoList[i].Color1==Type
				|| m_GradientInfoList[i].Color2==Type)
			return i;
	}
	return -1;
}


void CColorScheme::SetLoadedFlag(int Color)
{
	m_LoadedFlags[Color/32]|=1<<(Color%32);
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
		FindClose(hFind);
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
	: m_pPreviewColorScheme(NULL)
	, m_pApplyFunc(NULL)
{
	m_pColorScheme=new CColorScheme;
}


CColorSchemeOptions::~CColorSchemeOptions()
{
	delete m_pColorScheme;
	delete m_pPreviewColorScheme;
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
	if (m_pPreviewColorScheme!=NULL)
		return m_pPreviewColorScheme->GetColor(Type);
	return m_pColorScheme->GetColor(Type);
}


COLORREF CColorSchemeOptions::GetColor(LPCTSTR pszText) const
{
	if (m_pPreviewColorScheme!=NULL)
		return m_pPreviewColorScheme->GetColor(pszText);
	return m_pColorScheme->GetColor(pszText);
}


void CColorSchemeOptions::GetCurrentSettings(CColorScheme *pColorScheme)
{
	int i;

	for (i=0;i<CColorScheme::NUM_COLORS;i++)
		pColorScheme->SetColor(i,
							   (COLORREF)DlgListBox_GetItemData(m_hDlg,IDC_COLORSCHEME_LIST,i));
	for (int i=0;i<CColorScheme::NUM_GRADIENTS;i++)
		pColorScheme->SetGradientType(i,m_GradientList[i]);
	for (int i=0;i<CColorScheme::NUM_BORDERS;i++)
		pColorScheme->SetBorderType(i,m_BorderList[i]);
}


bool CColorSchemeOptions::GetThemesDirectory(LPTSTR pszDirectory,int MaxLength,bool fCreate)
{
	GetAppClass().GetAppDirectory(pszDirectory);
	::PathAppend(pszDirectory,TEXT("Themes"));
	if (fCreate && !::PathIsDirectory(pszDirectory))
		::CreateDirectory(pszDirectory,NULL);
	return true;
}


CColorSchemeOptions *CColorSchemeOptions::GetThis(HWND hDlg)
{
	return static_cast<CColorSchemeOptions*>(GetOptions(hDlg));
}


INT_PTR CALLBACK CColorSchemeOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CColorSchemeOptions *pThis=static_cast<CColorSchemeOptions*>(OnInitDialog(hDlg,lParam));
			TCHAR szDirectory[MAX_PATH];
			int i;
			int Index;

			GetThemesDirectory(szDirectory,lengthof(szDirectory));
			pThis->m_PresetList.Load(szDirectory);
			for (i=0;i<pThis->m_PresetList.NumColorSchemes();i++) {
				LPCTSTR pszName=pThis->m_PresetList.GetColorScheme(i)->GetName();

				Index=(int)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_ADDSTRING,0,
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
			ExtendListBox(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST));

			for (int i=0;i<CColorScheme::NUM_GRADIENTS;i++)
				pThis->m_GradientList[i]=pThis->m_pColorScheme->GetGradientType(i);
			for (int i=0;i<CColorScheme::NUM_BORDERS;i++)
				pThis->m_BorderList[i]=pThis->m_pColorScheme->GetBorderType(i);

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
						CColorScheme *pColorScheme=pThis->m_PresetList.GetColorScheme((int)pdis->itemData);
						Theme::GradientInfo Gradient;

						pColorScheme->GetGradientInfo(
							(pdis->itemState&ODS_SELECTED)==0?
								CColorScheme::GRADIENT_STATUSBACK:
								CColorScheme::GRADIENT_STATUSHIGHLIGHTBACK,
							&Gradient);
						Theme::FillGradient(pdis->hDC,&pdis->rcItem,&Gradient);
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
				int Sel=(int)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETCURSEL,0,0);

				if (Sel>=0) {
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int Index=(int)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETITEMDATA,Sel,0);
					const CColorScheme *pColorScheme=pThis->m_PresetList.GetColorScheme(Index);

					if (pColorScheme!=NULL) {
						int i;

						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (pColorScheme->IsLoaded(i))
								SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,
									LB_SETITEMDATA,i,pColorScheme->GetColor(i));
						}
						SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETSEL,FALSE,-1);
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);

						for (i=0;i<CColorScheme::NUM_GRADIENTS;i++)
							pThis->m_GradientList[i]=pColorScheme->GetGradientType(i);
						for (i=0;i<CColorScheme::NUM_BORDERS;i++)
							pThis->m_BorderList[i]=pColorScheme->GetBorderType(i);

						pThis->m_ColorPalette.SetSel(-1);
						::SendMessage(hDlg,WM_COMMAND,IDC_COLORSCHEME_PREVIEW,0);
					}
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_SAVE:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				TCHAR szName[MAX_PATH],szFileName[MAX_PATH];
				int Index;
				CColorScheme *pColorScheme=NULL;
				LPCTSTR pszFileName;

				GetDlgItemText(hDlg,IDC_COLORSCHEME_PRESET,szName,lengthof(szName));
				if (szName[0]=='\0') {
					MessageBox(hDlg,TEXT("名前を入力してください。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				pszFileName=szName;
				Index=(int)DlgComboBox_FindStringExact(hDlg,IDC_COLORSCHEME_PRESET,-1,szName);
				if (Index>=0) {
					pColorScheme=pThis->m_PresetList.GetColorScheme(
						(int)DlgComboBox_GetItemData(hDlg,IDC_COLORSCHEME_PRESET,Index));
					if (pColorScheme==NULL)
						break;
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
				if (pColorScheme==NULL) {
					pColorScheme=new CColorScheme;
					pColorScheme->SetName(szName);
				}
				pThis->GetCurrentSettings(pColorScheme);
				if (!pColorScheme->Save(szFileName)) {
					if (Index<0)
						delete pColorScheme;
					MessageBox(hDlg,TEXT("保存ができません。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if (Index<0) {
					pColorScheme->SetFileName(szFileName);
					pColorScheme->SetLoaded();
					pThis->m_PresetList.Add(pColorScheme);
					Index=(int)DlgComboBox_AddString(hDlg,IDC_COLORSCHEME_PRESET,szName);
					DlgComboBox_SetItemData(hDlg,IDC_COLORSCHEME_PRESET,Index,pThis->m_PresetList.NumColorSchemes()-1);
				}
				MessageBox(hDlg,TEXT("配色を保存しました。"),TEXT("保存"),MB_OK | MB_ICONINFORMATION);
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
				Index=(int)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_FINDSTRINGEXACT,-1,(LPARAM)szName);
				if (Index<0
						|| (pColorScheme=pThis->m_PresetList.GetColorScheme(
							(int)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_PRESET,CB_GETITEMDATA,Index,0)))==NULL)
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
				MessageBox(hDlg,TEXT("ファイルを削除しました。"),TEXT("削除"),MB_OK | MB_ICONINFORMATION);
			}
			return TRUE;

		case IDC_COLORSCHEME_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				{
					CColorSchemeOptions *pThis=GetThis(hDlg);
					int SelCount=(int)DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST);
					int i;
					COLORREF SelColor=CLR_INVALID,Color;

					if (SelCount==0) {
						pThis->m_ColorPalette.SetSel(-1);
						break;
					}
					if (SelCount==1) {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i)) {
								SelColor=(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i);
								break;
							}
						}
					} else {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i)) {
								Color=(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i);
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

			case LBN_EX_RBUTTONDOWN:
				{
					CColorSchemeOptions *pThis=GetThis(hDlg);
					HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_COLORSCHEME));
					POINT pt;

					::EnableMenuItem(hmenu,IDC_COLORSCHEME_SELECTSAMECOLOR,
						MF_BYCOMMAND | (pThis->m_ColorPalette.GetSel()>=0?MFS_ENABLED:MFS_GRAYED));
					if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
						int Sel,Gradient;

						DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
						Gradient=CColorScheme::GetColorGradient(Sel);
						if (Gradient>=0) {
							::EnableMenuItem(hmenu,IDC_COLORSCHEME_GLOSSYGRADIENT,MF_BYCOMMAND | MFS_ENABLED);
							if (pThis->m_GradientList[Gradient]==CColorScheme::GRADIENT_TYPE_GLOSSY)
								::CheckMenuItem(hmenu,IDC_COLORSCHEME_GLOSSYGRADIENT,MF_BYCOMMAND | MFS_CHECKED);
						}
					}
					::GetCursorPos(&pt);
					::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,hDlg,NULL);
					::DestroyMenu(hmenu);
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
						if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i))
							DlgListBox_SetItemData(hDlg,IDC_COLORSCHEME_LIST,i,Color);
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
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i))
								DlgListBox_SetItemData(hDlg,IDC_COLORSCHEME_LIST,i,Color);
						}
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
					}
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_DEFAULT:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				int i;

				for (i=0;i<CColorScheme::NUM_COLORS;i++)
					DlgListBox_SetItemData(hDlg,IDC_COLORSCHEME_LIST,i,
										   CColorScheme::GetDefaultColor(i));
				for (i=0;i<CColorScheme::NUM_GRADIENTS;i++)
					pThis->m_GradientList[i]=CColorScheme::GetDefaultGradientType(i);
				for (i=0;i<CColorScheme::NUM_BORDERS;i++)
					pThis->m_BorderList[i]=CColorScheme::GetDefaultBorderType(i);
				InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
				::SendMessage(hDlg,WM_COMMAND,IDC_COLORSCHEME_PREVIEW,0);
			}
			return TRUE;

		case IDC_COLORSCHEME_PREVIEW:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				if (pThis->m_pPreviewColorScheme==NULL)
					pThis->m_pPreviewColorScheme=new CColorScheme;
				pThis->GetCurrentSettings(pThis->m_pPreviewColorScheme);
				pThis->Apply(pThis->m_pPreviewColorScheme);
			}
			return TRUE;

		case IDC_COLORSCHEME_SELECTSAMECOLOR:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);
				int Sel=pThis->m_ColorPalette.GetSel();

				if (Sel>=0) {
					COLORREF Color=pThis->m_ColorPalette.GetColor(Sel);
					int TopIndex=(int)DlgListBox_GetTopIndex(hDlg,IDC_COLORSCHEME_LIST);

					::SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,WM_SETREDRAW,FALSE,0);
					for (int i=0;i<CColorScheme::NUM_COLORS;i++) {
						DlgListBox_SetSel(hDlg,IDC_COLORSCHEME_LIST,i,
							(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i)==Color);
					}
					DlgListBox_SetTopIndex(hDlg,IDC_COLORSCHEME_LIST,TopIndex);
					::SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,WM_SETREDRAW,TRUE,0);
					::InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_GLOSSYGRADIENT:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
					int Sel,Gradient;

					DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
					Gradient=CColorScheme::GetColorGradient(Sel);
					if (Gradient>=0) {
						if (pThis->m_GradientList[Gradient]!=CColorScheme::GRADIENT_TYPE_GLOSSY)
							pThis->m_GradientList[Gradient]=CColorScheme::GRADIENT_TYPE_GLOSSY;
						else
							pThis->m_GradientList[Gradient]=CColorScheme::GRADIENT_TYPE_NORMAL;
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				pThis->GetCurrentSettings(pThis->m_pColorScheme);
				pThis->Apply(pThis->m_pColorScheme);
			}
			break;

		case PSN_RESET:
			{
				CColorSchemeOptions *pThis=GetThis(hDlg);

				if (pThis->m_pPreviewColorScheme!=NULL)
					pThis->Apply(pThis->m_pColorScheme);
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CColorSchemeOptions *pThis=GetThis(hDlg);

			if (pThis->m_pPreviewColorScheme!=NULL) {
				delete pThis->m_pPreviewColorScheme;
				pThis->m_pPreviewColorScheme=NULL;
			}
			pThis->m_PresetList.Clear();
			pThis->OnDestroy();
		}
		break;
	}
	return FALSE;
}

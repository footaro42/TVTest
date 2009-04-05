#define _WIN32_DCOM	// for CoInitializeEx()
#include "stdafx.h"
#include <shlwapi.h>
#include "DtvEngine.h"
#include "TVTest.h"
#include "AppMain.h"
#include "View.h"
#include "Menu.h"
#include "ChannelManager.h"
#include "ChannelScan.h"
#include "ResidentManager.h"
#include "DriverManager.h"
#include "StatusView.h"
#include "Panel.h"
#include "InfoPanel.h"
#include "Information.h"
#include "EpgProgramList.h"
#include "ProgramListView.h"
#include "ProgramGuide.h"
#include "ChannelPanel.h"
#include "ControlPanel.h"
#include "Splitter.h"
#include "TitleBar.h"
#include "Accelerator.h"
#include "RemoteController.h"
#include "NetworkRemocon.h"
#include "DriverOptions.h"
#include "Record.h"
#include "Capture.h"
#include "Plugin.h"
#include "ViewOptions.h"
#include "StatusOptions.h"
#include "PanelOptions.h"
#include "ColorScheme.h"
#include "OSDOptions.h"
#include "AudioOptions.h"
#include "CaptureOptions.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "InitialSettings.h"
#include "ChannelHistory.h"
#include "Settings.h"
#include "CommandLine.h"
#include "Logger.h"
#include "Help.h"
#include "Error.h"
#include "Image.h"
#include "StreamInfo.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "PseudoOSD.h"
#include "NotificationBar.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAIN_WINDOW_CLASS				APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS			APP_NAME TEXT(" Fullscreen")

#define TITLE_TEXT APP_NAME

#define PANE_ID_VIEW	1
#define PANE_ID_PANEL	2

enum {
	PANEL_TAB_INFORMATION,
	PANEL_TAB_PROGRAMLIST,
	PANEL_TAB_CHANNEL,
	PANEL_TAB_CONTROL
};

// 以下グローバル変数使いまくり
// 改善中...

static HINSTANCE hInst;
static CAppMain AppMain;
static CCoreEngine CoreEngine;
static CMainWindow MainWindow;
static CStatusView StatusView;
static CTitleBar TitleBar;
static CSplitter Splitter;
static CMainMenu MainMenu;
static CCommandList CommandList;
static CNotificationBar NotificationBar;
static CErrorDialog ErrorDialog;
static CHtmlHelp HtmlHelpClass;
static CPseudoOSD ChannelOSD;
static CPseudoOSD VolumeOSD;

static TCHAR szDriverFileName[MAX_PATH];
static bool fIncrementUDPPort=true;

static CCommandLineParser CmdLineParser;

static CChannelManager ChannelManager;
static CNetworkRemocon *pNetworkRemocon=NULL;
static CResidentManager ResidentManager;
static CDriverManager DriverManager;

static bool fShowPanelWindow=false;
static CPanelFrame PanelFrame;
static int PanelPaneIndex=0;
static CInfoPanel InfoPanel;
static int InfoPanelCurTab=0;

static CInformation InfoWindow;

static CEpgProgramList EpgProgramList;
static CProgramListView ProgramListView;
static unsigned int ProgramListUpdateTimerCount=0;

static CChannelPanel ChannelPanel;

static CControlPanel ControlPanel;
enum {
	CONTROLPANEL_ITEM_CHANNEL_1,
	CONTROLPANEL_ITEM_CHANNEL_2,
	CONTROLPANEL_ITEM_CHANNEL_3,
	CONTROLPANEL_ITEM_CHANNEL_4,
	CONTROLPANEL_ITEM_CHANNEL_5,
	CONTROLPANEL_ITEM_CHANNEL_6,
	CONTROLPANEL_ITEM_CHANNEL_7,
	CONTROLPANEL_ITEM_CHANNEL_8,
	CONTROLPANEL_ITEM_CHANNEL_9,
	CONTROLPANEL_ITEM_CHANNEL_10,
	CONTROLPANEL_ITEM_CHANNEL_11,
	CONTROLPANEL_ITEM_CHANNEL_12,
	CONTROLPANEL_ITEM_VOLUME,
	CONTROLPANEL_ITEM_OPTIONS
};

static CProgramGuide ProgramGuide;
static bool fShowProgramGuide=false;

static CStreamInfo StreamInfo;

static CChannelMenu ChannelMenu(&EpgProgramList);

static const BYTE VolumeNormalizeLevelList[] = {100, 125, 150, 200};

static const struct {
	WORD Num,Denom;
} ZoomRateList[] = {
	{  1,   5},
	{  1,   4},
	{  1,   3},
	{  1,   2},
	{  2,   3},
	{  3,   4},
	{  1,   1},
	{  3,   2},
	{  2,   1},
};
static int AspectRatioType=0;

static CViewOptions ViewOptions;
static CStatusOptions StatusOptions(&StatusView);
static CPanelOptions PanelOptions(&PanelFrame);
static CColorSchemeOptions ColorSchemeOptions;
static COSDOptions OSDOptions;
static CAccelerator Accelerator;
static CHDUSController HDUSController;
static CDriverOptions DriverOptions;
static CAudioOptions AudioOptions;
static CRecordOptions RecordOptions;
static CRecordManager RecordManager;
static CCaptureOptions CaptureOptions;
static CChannelScan ChannelScan(&CoreEngine);
static CEpgOptions EpgOptions(&CoreEngine);
static CProgramGuideOptions ProgramGuideOptions(&ProgramGuide);
static CPluginList PluginList;
static CPluginOptions PluginOptions(&PluginList);
static CNetworkRemoconOptions NetworkRemoconOptions;
static CLogger Logger;
static CChannelHistory ChannelHistory;

#define MAX_MPEG2_DECODER_NAME 128
static TCHAR szMpeg2DecoderName[MAX_MPEG2_DECODER_NAME];
static CVideoRenderer::RendererType VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
static bool fDescrambleCurServiceOnly=false;
static bool fKeepSingleTask=false;
static bool fNoScreenSaver=false;
static BOOL fScreenSaverActive=FALSE;
static bool fNoMonitorLowPower=false;
static bool fNoMonitorLowPowerActiveOnly=false;
static BOOL fLowPowerActiveOriginal=FALSE,fPowerOffActiveOriginal=FALSE;
static struct {
	int Space;
	int Channel;
	int Service;
	TCHAR szDriverName[MAX_PATH];
} RestoreChannelInfo;

static bool fEnablePlay=true;
static bool fMuteStatus=false;

enum {
	WHEEL_MODE_NONE,
	WHEEL_MODE_VOLUME,
	WHEEL_MODE_CHANNEL,
	WHEEL_MODE_STEREOMODE,
	WHEEL_MODE_ZOOM
};
static int WheelMode=WHEEL_MODE_VOLUME;
static int WheelShiftMode=WHEEL_MODE_CHANNEL;
static bool fWheelChannelReverse=false;
static unsigned int WheelChannelDelay=1000;
static bool fWheelChannelChanging=false;
static int VolumeStep=5;
static bool fFunctionKeyChangeChannel=true;
static bool fDigitKeyChangeChannel=true;
static bool fNumPadChangeChannel=true;

static CImageCodec ImageCodec;
static CCaptureWindow CaptureWindow;
static bool fShowCaptureWindow=false;
//static bool fUseGrabberFilter=false;




class CMyGetChannelReciver : public CNetworkRemoconReciver {
public:
	void OnRecive(LPCSTR pszText);
};

void CMyGetChannelReciver::OnRecive(LPCSTR pszText)
{
	int Channel;
	LPCSTR p;

	Channel=0;
	for (p=pszText;*p!='\0';p++)
		Channel=Channel*10+(*p-'0');
	PostMessage(MainWindow.GetHandle(),WM_APP_CHANNELCHANGE,Channel,0);
}


class CMyGetDriverReciver : public CNetworkRemoconReciver {
	HANDLE m_hEvent;
	TCHAR m_szCurDriver[64];
public:
	CMyGetDriverReciver() { m_hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL); }
	~CMyGetDriverReciver() { ::CloseHandle(m_hEvent); }
	void OnRecive(LPCSTR pszText);
	void Initialize() { ::ResetEvent(m_hEvent); }
	bool Wait(DWORD TimeOut) { return ::WaitForSingleObject(m_hEvent,TimeOut)==WAIT_OBJECT_0; }
	LPCTSTR GetCurDriver() const { return m_szCurDriver; }
};

void CMyGetDriverReciver::OnRecive(LPCSTR pszText)
{
	LPCSTR p;
	int Sel,i;

	m_szCurDriver[0]='\0';
	p=pszText;
	while (*p!='\t') {
		if (*p=='\0')
			goto End;
		p++;
	}
	p++;
	Sel=0;
	for (;*p>='0' && *p<='9';p++)
		Sel=Sel*10+(*p-'0');
	if (*p!='\t')
		goto End;
	p++;
	for (i=0;i<=Sel && *p!='\0';i++) {
		while (*p!='\t') {
			if (*p=='\0')
				goto End;
			p++;
		}
		p++;
		if (i==Sel) {
			int j;

			for (j=0;*p!='\t' && *p!='\0';j++) {
				m_szCurDriver[j]=*p++;
			}
			m_szCurDriver[j]='\0';
			break;
		} else {
			while (*p!='\t' && *p!='\0')
				p++;
			if (*p=='\t')
				p++;
		}
	}
End:
	::SetEvent(m_hEvent);
}


static CMyGetChannelReciver GetChannelReciver;
static CMyGetDriverReciver GetDriverReciver;




bool CAppMain::Initialize()
{
	TCHAR szModuleFileName[MAX_PATH];

	::GetModuleFileName(NULL,szModuleFileName,MAX_PATH);
	if (CmdLineParser.m_szIniFileName[0]=='\0') {
		::lstrcpy(m_szIniFileName,szModuleFileName);
		::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	} else {
		if (::PathIsFileSpec(CmdLineParser.m_szIniFileName)) {
			::lstrcpy(m_szIniFileName,szModuleFileName);
			::lstrcpy(::PathFindFileName(m_szIniFileName),CmdLineParser.m_szIniFileName);
		} else {
			::lstrcpy(m_szIniFileName,CmdLineParser.m_szIniFileName);
		}
	}
	::lstrcpy(m_szDefaultChannelFileName,szModuleFileName);
	::PathRenameExtension(m_szDefaultChannelFileName,TEXT(".ch2"));
	::lstrcpy(m_szChannelSettingFileName,szModuleFileName);
	::PathRenameExtension(m_szChannelSettingFileName,TEXT(".ch.ini"));
	/*
	// サンプルをデフォルトとして扱う
	if (!PathFileExists(m_szDefaultChannelFileName)) {
		TCHAR szSample[MAX_PATH];

		::lstrcpy(szSample,m_szDefaultChannelFileName);
		::PathRenameExtension(szSample,TEXT(".sample.ch"));
		::CopyFile(szSample,m_szDefaultChannelFileName,TRUE);
	}
	*/
	bool fExists=::PathFileExists(m_szIniFileName)!=FALSE;
	m_fFirstExecute=!fExists && CmdLineParser.m_szIniFileName[0]=='\0';
	if (fExists) {
		Logger.AddLog(TEXT("設定を読み込んでいます..."));
		LoadSettings();
	}
	m_fChannelScanning=false;
	return true;
}


bool CAppMain::Finalize()
{
	Logger.AddLog(TEXT("設定を保存しています..."));
	SaveSettings();
	SaveChannelSettings();
	return true;
}


HINSTANCE CAppMain::GetResourceInstance() const
{
	return hInst;
}


HINSTANCE CAppMain::GetInstance() const
{
	return hInst;
}


bool CAppMain::GetAppDirectory(LPTSTR pszDirectory) const
{
	if (::GetModuleFileName(NULL,pszDirectory,MAX_PATH)==0)
		return false;
	CFilePath FilePath(pszDirectory);
	FilePath.RemoveFileName();
	FilePath.GetPath(pszDirectory);
	return true;
}


bool CAppMain::AddLog(LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	bool fOK=Logger.AddLogV(pszText,Args);
	va_end(Args);
	return fOK;
}


bool CAppMain::InitializeChannel()
{
	bool fNetworkDriver=CoreEngine.IsNetworkDriver();
	CFilePath ChannelFilePath;

	ChannelManager.Clear();
	ChannelManager.MakeDriverTuningSpaceList(&CoreEngine.m_DtvEngine.m_BonSrcDecoder);
	if (!fNetworkDriver) {
		ChannelFilePath.SetPath(CoreEngine.GetDriverFileName());
		if (!ChannelFilePath.HasDirectory()) {
			TCHAR szDir[MAX_PATH];

			GetAppDirectory(szDir);
			ChannelFilePath.SetDirectory(szDir);
		}
		ChannelFilePath.SetExtension(TEXT(".ch2"));
		if (!ChannelFilePath.IsExists())
			ChannelFilePath.SetExtension(TEXT(".ch"));
	} else {
		bool fOK=false;

		if (NetworkRemoconOptions.IsEnable()) {
			if (NetworkRemoconOptions.CreateNetworkRemocon(&pNetworkRemocon)) {
				GetDriverReciver.Initialize();
				if (pNetworkRemocon->GetDriverList(&GetDriverReciver)
						&& GetDriverReciver.Wait(2000)
						&& GetDriverReciver.GetCurDriver()[0]!='\0') {
					TCHAR szFileName[MAX_PATH];

					if (NetworkRemoconOptions.FindChannelFile(
								GetDriverReciver.GetCurDriver(),szFileName)) {
						LPTSTR p;

						NetworkRemoconOptions.SetDefaultChannelFileName(szFileName);
						p=szFileName;
						while (*p!='(')
							p++;
						::lstrcpy(p,TEXT(".ch2"));
						ChannelFilePath.SetPath(szFileName);
						GetAppDirectory(szFileName);
						ChannelFilePath.SetDirectory(szFileName);
						fOK=ChannelFilePath.IsExists();
						if (!fOK) {
							ChannelFilePath.SetExtension(TEXT(".ch"));
							fOK=ChannelFilePath.IsExists();
						}
					}
				}
			}
			if (!fOK && NetworkRemoconOptions.GetChannelFileName()[0]!='\0') {
				TCHAR szFileName[MAX_PATH],*q;
				LPCTSTR p;

				GetAppDirectory(szFileName);
				ChannelFilePath.SetPath(szFileName);
				p=NetworkRemoconOptions.GetChannelFileName();
				q=szFileName;
				for (int i=0;*p!='(' && *p!='\0';i++) {
#ifndef UNICODE
					if (::IsDBCSLeadByteEx(CP_ACP,*p))
						*q++=*p++;
#endif
					*q++=*p++;
				}
				::lstrcpy(q,TEXT(".ch2"));
				ChannelFilePath.Append(szFileName);
				fOK=ChannelFilePath.IsExists();
				if (!fOK) {
					ChannelFilePath.SetExtension(TEXT(".ch"));
					fOK=ChannelFilePath.IsExists();
				}
			}
		}
		if (!fOK && szDriverFileName[0]!='\0'
				&& !CoreEngine.IsNetworkDriverFileName(szDriverFileName)) {
			ChannelFilePath.SetPath(szDriverFileName);
			ChannelFilePath.SetExtension(TEXT(".ch2"));
			if (!ChannelFilePath.IsExists())
				ChannelFilePath.SetExtension(TEXT(".ch"));
		}
	}
	if (ChannelFilePath.GetPath()[0]=='\0' || !ChannelFilePath.IsExists()) {
		ChannelFilePath.SetPath(m_szDefaultChannelFileName);
		if (!ChannelFilePath.IsExists()) {
			ChannelFilePath.SetExtension(TEXT(".ch"));
		}
	}
	if (ChannelManager.LoadChannelList(ChannelFilePath.GetPath()))
		Logger.AddLog(TEXT("チャンネル設定を \"%s\" から読み込みました。"),
												ChannelFilePath.GetFileName());
	TCHAR szFileName[MAX_PATH];
	bool fLoadChannelSettings=true;
	if (!fNetworkDriver) {
		::lstrcpy(szFileName,CoreEngine.GetDriverFileName());
	} else {
		if (!CoreEngine.IsNetworkDriverFileName(szDriverFileName)) {
			::lstrcpy(szFileName,szDriverFileName);
		} else {
			fLoadChannelSettings=false;
		}
	}
	if (fLoadChannelSettings)
		ChannelManager.LoadChannelSettings(m_szChannelSettingFileName,szFileName);

	RestoreChannelInfo.Space=-1;
	RestoreChannelInfo.Channel=-1;
	RestoreChannelInfo.Service=0;
	CDriverOptions::InitialChannelInfo InitChInfo;
	if (DriverOptions.GetInitialChannel(CoreEngine.GetDriverFileName(),&InitChInfo)) {
		RestoreChannelInfo.Space=InitChInfo.Space;
		RestoreChannelInfo.Channel=InitChInfo.Channel;
		RestoreChannelInfo.Service=InitChInfo.Service;
	} else /*if (!fNetworkDriver)*/ {
		CSettings Setting;

		if (Setting.Open(m_szIniFileName,TEXT("LastChannel"),CSettings::OPEN_READ)) {
			TCHAR szDriverName[MAX_PATH],szName[32+MAX_PATH];

			::lstrcpy(szDriverName,::PathFindFileName(CoreEngine.GetDriverFileName()));
			*::PathFindExtension(szDriverName)='\0';
			::wsprintf(szName,TEXT("Space_%s"),szDriverName);
			Setting.Read(szName,&RestoreChannelInfo.Space);
			::wsprintf(szName,TEXT("ChannelIndex_%s"),szDriverName);
			Setting.Read(szName,&RestoreChannelInfo.Channel);
			::wsprintf(szName,TEXT("Service_%s"),szDriverName);
			Setting.Read(szName,&RestoreChannelInfo.Service);
			Setting.Close();
		}
	}

	ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	ChannelManager.SetCurrentSpace(
		(!fNetworkDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
											CChannelManager::SPACE_ALL:0);
	*/
	ChannelManager.SetCurrentChannel(
		RestoreChannelInfo.Space>=0?RestoreChannelInfo.Space:0,
		CoreEngine.IsUDPDriver()?0:-1);
	ChannelManager.SetCurrentService(0);
	SetTuningSpaceMenu();
	SetChannelMenu();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	ChannelScan.SetTuningSpaceList(ChannelManager.GetTuningSpaceList());
	return true;
}


bool CAppMain::UpdateChannelList(const CTuningSpaceList *pList)
{
	bool fNetworkDriver=CoreEngine.IsNetworkDriver();

	ChannelManager.SetTuningSpaceList(pList);
	ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	ChannelManager.SetCurrentChannel(
		(!fNetworkDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
												CChannelManager::SPACE_ALL:0,
		CoreEngine.IsUDPDriver()?0:-1);
	*/
	ChannelManager.SetCurrentChannel(0,CoreEngine.IsUDPDriver()?0:-1);
	ChannelManager.SetCurrentService(0);
	SetTuningSpaceMenu();
	//SetChannelMenu();
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		FollowChannelChange(CoreEngine.m_DtvEngine.m_ProgManager.GetTransportStreamID(),ServiceID);
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	return true;
}


bool CAppMain::SaveChannelSettings()
{
	if (!CoreEngine.IsDriverLoaded() || CoreEngine.IsNetworkDriver())
		return true;
	return ChannelManager.SaveChannelSettings(m_szChannelSettingFileName,
											  CoreEngine.GetDriverFileName());
}


void CAppMain::SetTuningSpaceMenu(HMENU hmenu)
{
	TCHAR szText[MAX_PATH*2];
	int Length;
	int i,j;

	ClearMenu(hmenu);
	if ((!CoreEngine.IsNetworkDriver() || pNetworkRemocon==NULL)
			&& ChannelManager.GetAllChannelList()->NumChannels()>0)
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SPACE_ALL,TEXT("&A: すべて"));
	const CTuningSpaceList *pTuningSpaceList=ChannelManager.GetDriverTuningSpaceList();
	for (i=0;i<pTuningSpaceList->NumSpaces();i++) {
		LPCTSTR pszName=pTuningSpaceList->GetTuningSpaceName(i);

		Length=::wsprintf(szText,TEXT("&%d: "),i);
		CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
					   szText+Length,lengthof(szText)-Length);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SPACE_FIRST+i,szText);
	}
	::CheckMenuRadioItem(hmenu,CM_SPACE_ALL,CM_SPACE_ALL+pTuningSpaceList->NumSpaces(),
				CM_SPACE_FIRST+ChannelManager.GetCurrentSpace(),MF_BYCOMMAND);
	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	int CurDriver=-1;
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0)
			CurDriver=i;
	}
	if (CurDriver<0) {
		CopyToMenuText(CoreEngine.GetDriverFileName(),szText,lengthof(szText));
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		CurDriver=i++;
	}
	::CheckMenuRadioItem(hmenu,CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
						 CM_DRIVER_FIRST+CurDriver,MF_BYCOMMAND);
	Accelerator.SetMenuAccel(hmenu);
}


void CAppMain::SetTuningSpaceMenu()
{
	SetTuningSpaceMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE));
}


void CAppMain::SetChannelMenu(HMENU hmenu)
{
	if (pNetworkRemocon!=NULL) {
		SetNetworkRemoconChannelMenu(hmenu);
		return;
	}

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();
	int i;
	TCHAR szText[MAX_CHANNEL_NAME+4];

	ClearMenu(hmenu);
	if (pList==NULL)
		return;
	bool fControlKeyID=pList->HasRemoteControlKeyID();
	for (i=0;i<pList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

		if (pChInfo->IsEnabled()) {
			wsprintf(szText,TEXT("%d: %s"),
				fControlKeyID?pChInfo->GetChannelNo():i+1,pChInfo->GetName());
			AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
				| (i!=0 && i%12==0?MF_MENUBREAK:0),CM_CHANNEL_FIRST+i,szText);
		}
	}
	if (ChannelManager.GetCurrentChannel()>=0
			&& pList->IsEnabled(ChannelManager.GetCurrentChannel()))
		MainMenu.CheckRadioItem(CM_CHANNEL_FIRST,
			CM_CHANNEL_FIRST+pList->NumChannels()-1,
			CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel());
}


void CAppMain::SetChannelMenu()
{
	SetChannelMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL));
}


void CAppMain::SetNetworkRemoconChannelMenu(HMENU hmenu)
{
	const CChannelList &RemoconChList=pNetworkRemocon->GetChannelList();
	int i;
	TCHAR szText[MAX_CHANNEL_NAME+4];
	const CChannelList *pPortList;

	ClearMenu(hmenu);
	if (RemoconChList.NumChannels()>0) {
		int No,Min,Max;

		Min=1000;
		Max=0;
		for (i=0;i<RemoconChList.NumChannels();i++) {
			No=RemoconChList.GetChannelNo(i);
			if (No<Min)
				Min=No;
			if (No>Max)
				Max=No;
		}
		for (No=Min;No<=Max;No++) {
			for (i=0;i<RemoconChList.NumChannels();i++) {
				if (RemoconChList.GetChannelNo(i)==No) {
					wsprintf(szText,TEXT("%d: %s"),No,RemoconChList.GetName(i));
					AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,
											CM_CHANNELNO_FIRST+No-1,szText);
				}
			}
		}
		if (ChannelManager.GetNetworkRemoconCurrentChannel()>=0)
			MainMenu.CheckRadioItem(CM_CHANNELNO_FIRST,
				CM_CHANNELNO_FIRST+Max-1,
				CM_CHANNEL_FIRST+ChannelManager.GetNetworkRemoconCurrentChannel());
	}
	pPortList=ChannelManager.GetDriverChannelList(0);
	for (i=0;i<pPortList->NumChannels();i++) {
		wsprintf(szText,TEXT("%d: %s"),
							pPortList->GetChannelNo(i),pPortList->GetName(i));
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
			| ((i!=0 && i%12==0) || (i==0 && RemoconChList.NumChannels()>0)?
															MF_MENUBREAK:0),
												CM_CHANNEL_FIRST+i,szText);
	}
	if (ChannelManager.GetCurrentChannel()>=0)
		MainMenu.CheckRadioItem(CM_CHANNEL_FIRST,
			CM_CHANNEL_FIRST+pPortList->NumChannels()-1,
			CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel());
}


bool CAppMain::UpdateChannelMenu()
{
	SetChannelMenu();
	return true;
}


const CChannelInfo *CAppMain::GetCurrentChannelInfo() const
{
	return ChannelManager.GetCurrentChannelInfo();
}


bool CAppMain::SetChannel(int Space,int Channel,int Service/*=-1*/)
{
	const CChannelInfo *pPrevChInfo=ChannelManager.GetCurrentRealChannelInfo();
	int OldSpace=ChannelManager.GetCurrentSpace(),OldChannel=ChannelManager.GetCurrentChannel();

	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
	if (pChInfo==NULL) {
		ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
		return false;
	}
	MainWindow.OpenTuner();
	if (pPrevChInfo==NULL
			|| pChInfo->GetSpace()!=pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex()!=pPrevChInfo->GetChannelIndex()) {
		LPCTSTR pszTuningSpace=ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetService());
		AddLog(TEXT("BonDriverにチャンネル変更を要求しました。(チューニング空間 %d[%s] / Ch %d)"),
			   pChInfo->GetSpace(),pszTuningSpace!=NULL?pszTuningSpace:TEXT("???"),
			   pChInfo->GetChannelIndex());

		int OldService=ChannelManager.GetCurrentService();
		ChannelManager.SetCurrentService(Service);
		if (!CoreEngine.m_DtvEngine.SetChannel(pChInfo->GetSpace(),
											   pChInfo->GetChannelIndex())) {
			AddLog(CoreEngine.m_DtvEngine.GetLastErrorText());
			ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
			ChannelManager.SetCurrentService(OldService);
			return false;
		}
		if (pChInfo->GetService()>0 && pChInfo->GetServiceID() != 0)
			CoreEngine.m_DtvEngine.SetServiceByID(pChInfo->GetServiceID());
		PluginList.SendChannelChangeEvent();
	} else {
		ChannelManager.SetCurrentService(Service);
		if (Service>=0) {
			SetService(Service);
		} else {
			SetServiceByID(pChInfo->GetServiceID());
		}
	}
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	SetChannelMenu();
	MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
		CM_SPACE_FIRST+ChannelManager.GetCurrentSpace());
	MainWindow.OnChannelChange();
	return true;
}


bool CAppMain::FollowChannelChange(WORD TransportStreamID,WORD ServiceID)
{
	const CChannelList *pChannelList;
	const CChannelInfo *pChannelInfo;
	int i,j;
	int Space,Channel;
	bool fFinded=false;

	pChannelList=ChannelManager.GetCurrentRealChannelList();
	if (pChannelList!=NULL) {
		for (i=0;i<pChannelList->NumChannels();i++) {
			pChannelInfo=pChannelList->GetChannelInfo(i);
			if (pChannelInfo->GetTransportStreamID()==TransportStreamID
					&& pChannelInfo->GetServiceID()==ServiceID) {
				Space=ChannelManager.GetCurrentSpace();
				Channel=i;
				fFinded=true;
				break;
			}
		}
	} else {
		for (i=0;i<ChannelManager.NumSpaces();i++) {
			pChannelList=ChannelManager.GetChannelList(i);
			for (j=0;j<pChannelList->NumChannels();j++) {
				pChannelInfo=pChannelList->GetChannelInfo(j);
				if (pChannelInfo->GetTransportStreamID()==TransportStreamID
						&& pChannelInfo->GetServiceID()==ServiceID) {
					Space=i;
					Channel=j;
					fFinded=true;
					break;
				}
			}
		}
	}
	if (!fFinded)
		return false;
	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	ChannelManager.SetCurrentService(-1);
	PluginList.SendChannelChangeEvent();
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	SetChannelMenu();
	MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
							CM_SPACE_FIRST+Space);
	MainWindow.OnChannelChange();
	return true;
}


bool CAppMain::SetService(int Service)
{
	int NumServices=CoreEngine.m_DtvEngine.m_ProgManager.GetServiceNum();

	if (Service<0 || Service>=NumServices
			|| !CoreEngine.m_DtvEngine.SetService(Service))
		return false;
	if (pNetworkRemocon!=NULL)
		pNetworkRemocon->SetService(Service);
	WORD ServiceID=0;
	CoreEngine.m_DtvEngine.GetServiceID(&ServiceID);
	AddLog(TEXT("サービスを変更しました。(%d: SID %d)"),Service,ServiceID);
	MainMenu.CheckRadioItem(CM_SERVICE_FIRST,CM_SERVICE_FIRST+NumServices-1,
							CM_SERVICE_FIRST+Service);
	PluginList.SendServiceChangeEvent();
	return true;
}


bool CAppMain::SetServiceByIndex(int Service)
{
	if (!SetService(Service))
		return false;
	ChannelManager.SetCurrentService(Service);
	return true;
}


bool CAppMain::SetServiceByID(WORD ServiceID,int *pServiceIndex/*=NULL*/)
{
	if (ServiceID==0)
		return SetService(0);

	AddLog(TEXT("サービスを選択しています(SID %d)..."),ServiceID);
	WORD Index=CoreEngine.m_DtvEngine.m_ProgManager.GetServiceIndexByID(ServiceID);
	if (Index!=0xFFFF) {
		if (SetService(Index)) {
			if (pServiceIndex!=NULL)
				*pServiceIndex=Index;
			return true;
		}
		return false;
	}
	AddLog(TEXT("該当するサービスが見付かりません。指定されたSIDはPMT内にありません。"));
	ChannelManager.SetCurrentServiceID(ServiceID);
	return false;
}


bool CAppMain::SetDriver(LPCTSTR pszFileName)
{
	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	bool fOK;

	if (::lstrcmpi(CoreEngine.GetDriverFileName(),pszFileName)==0)
		return false;
	SaveCurrentChannel();
	SaveChannelSettings();
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.SetDriverFileName(pszFileName);
	fOK=CoreEngine.LoadDriver();
	if (fOK) {
		Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
		fOK=CoreEngine.OpenDriver();
		if (fOK) {
			AppMain.InitializeChannel();
			/*
			int i=ChannelManager.GetCurrentChannelList()->Find(
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace(),
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel(),0);
			if (i>=0)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+i);
			*/
			PluginList.SendDriverChangeEvent();
			::SetCursor(hcurOld);
		} else {
			PluginList.SendDriverChangeEvent();
			::SetCursor(hcurOld);
			Logger.AddLog(CoreEngine.GetLastErrorText());
			MainWindow.ShowErrorMessage(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
		}
	} else {
		TCHAR szMessage[MAX_PATH+64];

		PluginList.SendDriverChangeEvent();
		::SetCursor(hcurOld);
		::wsprintf(szMessage,TEXT("\"%s\" をロードできません。"),pszFileName);
		MainWindow.ShowErrorMessage(szMessage);
	}
	CoreEngine.m_DtvEngine.SetTracer(NULL);
	StatusView.SetSingleText(NULL);
	MainWindow.OnDriverChange();
	return fOK;
}


bool CAppMain::UpdateDriverMenu()
{
	SetTuningSpaceMenu();
	return true;
}


HMENU CAppMain::CreateTunerSelectMenu()
{
	HMENU hmenu=::CreatePopupMenu(),hmenuSpace;
	const CChannelList *pChannelList;
	const CTuningSpaceList *pTuningSpaceList;
	int Command,FirstCommand;
	int i,j;
	LPCTSTR pszName;
	TCHAR szText[MAX_PATH*2];
	int Length;
	POINT pt;

	Command=CM_SPACE_CHANNEL_FIRST;
	/*
	if ((!CoreEngine.IsNetworkDriver() || pNetworkRemocon==NULL)
			&& ChannelManager.GetAllChannelList()->NumChannels()>0) {
		hmenuSpace=::CreatePopupMenu();
		pChannelList=ChannelManager.GetAllChannelList();
		for (i=0;i<pChannelList->NumChannels();i++) {
			pszName=pChannelList->GetName(i);
			CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),szText,lengthof(szText));
			::AppendMenu(hmenuSub,MFT_STRING | MFS_ENABLED,Command++,szText);
		}
		::AppendMenu(hmenu,MFT_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&0: すべて"));
	}
	*/
	pTuningSpaceList=ChannelManager.GetDriverTuningSpaceList();
	for (i=0;i<pTuningSpaceList->NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		hmenuSpace=::CreatePopupMenu();
		FirstCommand=Command;
		bool fHasControlKeyID=pChannelList->HasRemoteControlKeyID();
		for (j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

			if (pChannelInfo->IsEnabled()) {
				Length=::wsprintf(szText,TEXT("%d : "),
								  fHasControlKeyID?pChannelInfo->GetChannelNo():j+1);
				pszName=pChannelInfo->GetName();
				CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
							   szText+Length,lengthof(szText)-Length);
				::AppendMenu(hmenuSpace,MFT_STRING | MFS_ENABLED,Command++,szText);
			}
		}
		if (ChannelManager.GetCurrentSpace()==i
				&& ChannelManager.GetCurrentChannel()>=0
				&& pChannelList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenuSpace,FirstCommand,Command-1,
								 FirstCommand+ChannelManager.GetCurrentChannel(),
								 MF_BYCOMMAND);
		}
		Length=::wsprintf(szText,TEXT("&%d: "),i);
		pszName=pTuningSpaceList->GetTuningSpaceName(i);
		CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
					   szText+Length,lengthof(szText)-Length);
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
	}
	AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	//int CurDriver=-1;
	for (i=0;i<DriverManager.NumDrivers();i++) {
		CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0) {
			/*
			CurDriver=i;
			CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED | MFS_CHECKED,
						 CM_DRIVER_FIRST+i,szText);
			*/
			continue;
		}
		if (pDriverInfo->LoadTuningSpaceList(
				!::PathMatchSpec(pDriverInfo->GetFileName(),TEXT("BonDriver_Spinel*.dll")))) {
			HMENU hmenuDriver=::CreatePopupMenu();

			pTuningSpaceList=pDriverInfo->GetTuningSpaceList();
			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (pTuningSpaceList->NumSpaces()>1)
					hmenuSpace=::CreatePopupMenu();
				else
					hmenuSpace=hmenuDriver;
				bool fHasControlKeyID=pChannelList->HasRemoteControlKeyID();
				for (int k=0;k<pChannelList->NumChannels();k++) {
					const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(k);

					if (pChannelInfo->IsEnabled()) {
						Length=::wsprintf(szText,TEXT("%d: "),
							fHasControlKeyID?pChannelInfo->GetChannelNo():k+1);
						pszName=pChannelInfo->GetName();
						CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
									   szText+Length,lengthof(szText)-Length);
						::AppendMenu(hmenuSpace,MFT_STRING | MFS_ENABLED,Command++,szText);
					}
				}
				if (hmenuSpace!=hmenuDriver) {
					pszName=pTuningSpaceList->GetTuningSpaceName(j);
					Length=::wsprintf(szText,TEXT("&%d: "),j+1);
					CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
								   szText+Length,lengthof(szText)-Length);
					::AppendMenu(hmenuDriver,MF_POPUP | MFS_ENABLED,
								 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
				}
			}
			if (pDriverInfo->GetTunerName()!=NULL) {
				TCHAR szTemp[lengthof(szText)];

				::wnsprintf(szTemp,lengthof(szTemp),TEXT("%s [%s]"),
							pDriverInfo->GetTunerName(),
							pDriverInfo->GetFileName());
				CopyToMenuText(szTemp,szText,lengthof(szText));
			} else {
				CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
			}
			::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
						 reinterpret_cast<UINT_PTR>(hmenuDriver),szText);
		} else {
			CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		}
	}
	/*
	if (CurDriver<0) {
		CopyToMenuText(CoreEngine.GetDriverFileName(),szText,lengthof(szText));
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED | MFS_CHECKED,
					 CM_DRIVER_FIRST+i,szText);
	}
	*/
	return hmenu;
}


bool CAppMain::ProcessTunerSelectMenu(int Command)
{
	if (Command<CM_SPACE_CHANNEL_FIRST || Command>CM_SPACE_CHANNEL_LAST)
		return false;

	const CTuningSpaceList *pTuningSpaceList;
	const CChannelList *pChannelList;
	int CommandBase;
	int i,j;

	CommandBase=CM_SPACE_CHANNEL_FIRST;
	/*
	if (!CoreEngine.IsNetworkDriver() || pNetworkRemocon==NULL) {
		pChannelList=ChannelManager.GetAllChannelList();
		if (pChannelList->NumChannels()>0) {
			if (Command-CommandBase<pChannelList->NumChannels())
				return SetChannel(-1,Command-CommandBase);
			CommandBase+=pChannelList->NumChannels();
		}
	}
	*/
	pTuningSpaceList=ChannelManager.GetDriverTuningSpaceList();
	for (int i=0;i<pTuningSpaceList->NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		if (Command-CommandBase<pChannelList->NumChannels())
			return SetChannel(i,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0)
			continue;
		if (pDriverInfo->IsTuningSpaceListLoaded()) {
			pTuningSpaceList=pDriverInfo->GetTuningSpaceList();
			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (Command-CommandBase<pChannelList->NumChannels()) {
					if (!SetDriver(pDriverInfo->GetFileName()))
						return false;
					::lstrcpy(szDriverFileName,pDriverInfo->GetFileName());
					return SetChannel(j,Command-CommandBase);
				}
				CommandBase+=pChannelList->NumChannels();
			}
		}
	}
	return false;
}


bool CAppMain::LoadSettings()
{
	CSettings Setting;

	if (Setting.Open(m_szIniFileName,TEXT("Settings"),CSettings::OPEN_READ)) {
		int Value;
		TCHAR szText[MAX_PATH];
		int Left,Top,Width,Height;
		bool f;

		MainWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("WindowLeft"),&Left);
		Setting.Read(TEXT("WindowTop"),&Top);
		Setting.Read(TEXT("WindowWidth"),&Width);
		Setting.Read(TEXT("WindowHeight"),&Height);
		MainWindow.SetPosition(Left,Top,Width,Height);
		MainWindow.MoveToMonitorInside();
		if (Setting.Read(TEXT("WindowMaximize"),&f))
			MainWindow.SetMaximizeStatus(f);
		if (Setting.Read(TEXT("AlwaysOnTop"),&f))
			MainWindow.SetAlwaysOnTop(f);
		if (Setting.Read(TEXT("ShowStatusBar"),&f))
			MainWindow.SetStatusBarVisible(f);
		if (Setting.Read(TEXT("ShowTitleBar"),&f))
			MainWindow.SetTitleBarVisible(f);
		if (Setting.Read(TEXT("Volume"),&Value))
			CoreEngine.SetVolume(Value<0?0:Value>CCoreEngine::MAX_VOLUME?CCoreEngine::MAX_VOLUME:Value);
		//Setting.Read(TEXT("VolumeNormalize"),&fVolumeNormalize);
		if (Setting.Read(TEXT("VolumeNormalizeLevel"),&Value))
			CoreEngine.SetVolumeNormalizeLevel(Value);
		Setting.Read(TEXT("ShowInfoWindow"),&fShowPanelWindow);
		Setting.Read(TEXT("Driver"),szDriverFileName,
												lengthof(szDriverFileName));
		Setting.Read(TEXT("Mpeg2Decoder"),szMpeg2DecoderName,
												lengthof(szMpeg2DecoderName));
		TCHAR szRenderer[16];
		if (Setting.Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
			if (szRenderer[0]=='\0') {
				VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
			} else {
				VideoRendererType=CVideoRenderer::ParseName(szRenderer);
				if (VideoRendererType==CVideoRenderer::RENDERER_UNDEFINED)
					VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
			}
		}
		/*
		if (Setting.Read(TEXT("HDUSDriverVersion"),&Value))
			fNewHDUSDriver=Value==2;
		*/
		Setting.Read(TEXT("DescrambleCurServiceOnly"),&fDescrambleCurServiceOnly);
		Setting.Read(TEXT("KeepSingleTask"),&fKeepSingleTask);
		if (Setting.Read(TEXT("Resident"),&f))
			ResidentManager.SetResident(f);
		if (Setting.Read(TEXT("NoDescramble"),&f))	// Backward compatibility
			CoreEngine.SetCardReaderType(f?CCardReader::READER_NONE:CCardReader::READER_SCARD);
		if (Setting.Read(TEXT("CardReader"),&Value))
			CoreEngine.SetCardReaderType((CCardReader::ReaderType)Value);
		Setting.Read(TEXT("NoScreenSaver"),&fNoScreenSaver);
		Setting.Read(TEXT("NoMonitorLowPower"),&fNoMonitorLowPower);
		Setting.Read(TEXT("NoMonitorLowPowerActiveOnly"),&fNoMonitorLowPowerActiveOnly);
		/*
		//Setting.Read(TEXT("CurChannel"),&RestoreChannelInfo.Channel);
		Setting.Read(TEXT("CurChannelIndex"),&RestoreChannelInfo.Channel);
		Setting.Read(TEXT("CurSpace"),&RestoreChannelInfo.Space);
		Setting.Read(TEXT("CurService"),&RestoreChannelInfo.Service);
		Setting.Read(TEXT("OldDriver"),RestoreChannelInfo.szDriverName,
									lengthof(RestoreChannelInfo.szDriverName));
		*/
		if (Setting.Read(TEXT("PacketBuffering"),&f))
			CoreEngine.SetPacketBuffering(f);
		unsigned int BufferLength;
		if (Setting.Read(TEXT("PacketBufferLength"),&BufferLength))
			CoreEngine.SetPacketBufferLength(BufferLength);
		if (Setting.Read(TEXT("PacketBufferPoolPercentage"),&Value))
			CoreEngine.SetPacketBufferPoolPercentage(Value);
		/*
		Setting.Read(TEXT("UseOSD"),&fUseOSD);
		Setting.Read(TEXT("PseudoOSD"),&fUsePseudoOSD);
		Setting.ReadColor(TEXT("OSDTextColor"),&crOSDTextColor);
		ChannelOSD.SetTextColor(crOSDTextColor);
		VolumeOSD.SetTextColor(crOSDTextColor);
		Setting.Read(TEXT("OSDFadeTime"),&OSDFadeTime);
		*/
		Setting.Read(TEXT("EnablePlay"),&fEnablePlay);
		Setting.Read(TEXT("Mute"),&fMuteStatus);
		Setting.Read(TEXT("WheelMode"),&WheelMode);
		Setting.Read(TEXT("WheelShiftMode"),&WheelShiftMode);
		Setting.Read(TEXT("ReverseWheelChannel"),&fWheelChannelReverse);
		Setting.Read(TEXT("WheelChannelDelay"),&WheelChannelDelay);
		Setting.Read(TEXT("VolumeStep"),&VolumeStep);
		if (Setting.Read(TEXT("RecOptionFileName"),szText,MAX_PATH) && szText[0]!='\0')
			RecordManager.SetFileName(szText);
		if (Setting.Read(TEXT("RecOptionExistsOperation"),&Value))
			RecordManager.SetFileExistsOperation(
								(CRecordManager::FileExistsOperation)Value);
		/*
		if (Setting.Read(TEXT("RecOptionStopTimeSpec"),&f))
			RecordManager.SetStopTimeSpec(f);
		unsigned int Time;
		if (Setting.Read(TEXT("RecOptionStopTime"),&Time))
			RecordManager.SetStopTime(Time);
		*/
		PanelFrame.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("InfoLeft"),&Left);
		Setting.Read(TEXT("InfoTop"),&Top);
		Setting.Read(TEXT("InfoWidth"),&Width);
		Setting.Read(TEXT("InfoHeight"),&Height);
		PanelFrame.SetPosition(Left,Top,Width,Height);
		PanelFrame.MoveToMonitorInside();
		if (Setting.Read(TEXT("PanelFloating"),&f))
			PanelFrame.SetFloating(f);
		if (Setting.Read(TEXT("PanelDockingWidth"),&Value))
			PanelFrame.SetDockingWidth(Value);
		if (Setting.Read(TEXT("PanelDockingIndex"),&Value)
				&& (Value==0 || Value==1))
			PanelPaneIndex=Value;
		Setting.Read(TEXT("InfoCurTab"),&InfoPanelCurTab);
		ProgramGuide.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("ProgramGuideLeft"),&Left);
		Setting.Read(TEXT("ProgramGuideTop"),&Top);
		Setting.Read(TEXT("ProgramGuideWidth"),&Width);
		Setting.Read(TEXT("ProgramGuideHeight"),&Height);
		ProgramGuide.SetPosition(Left,Top,Width,Height);
		ProgramGuide.MoveToMonitorInside();
		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("CapturePreviewLeft"),&Left);
		Setting.Read(TEXT("CapturePreviewTop"),&Top);
		Setting.Read(TEXT("CapturePreviewWidth"),&Width);
		Setting.Read(TEXT("CapturePreviewHeight"),&Height);
		CaptureWindow.SetPosition(Left,Top,Width,Height);
		CaptureWindow.MoveToMonitorInside();
		if (Setting.Read(TEXT("CaptureStatusBar"),&f))
			CaptureWindow.ShowStatusBar(f);
		// Experimental options
		//Setting.Read(TEXT("UseGrabber"),&fUseGrabberFilter);
		Setting.Read(TEXT("IncrementUDPPort"),&fIncrementUDPPort);
		if (Setting.Read(TEXT("UseAudioRendererClock"),&f))
			CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(f);
		ViewOptions.Read(&Setting);
		OSDOptions.Read(&Setting);
		PanelOptions.Read(&Setting);
		AudioOptions.Read(&Setting);
		RecordOptions.Read(&Setting);
		CaptureOptions.Read(&Setting);
		Accelerator.Read(&Setting);
		HDUSController.Read(&Setting);
		ChannelScan.Read(&Setting);
		PluginOptions.Read(&Setting);
		EpgOptions.Read(&Setting);
		NetworkRemoconOptions.Read(&Setting);
		Logger.Read(&Setting);
		Setting.Close();
	}
	StatusOptions.Load(m_szIniFileName);
	ColorSchemeOptions.Load(m_szIniFileName);
	//Accelerator.Load(m_szIniFileName);
	//HDUSController.Load(m_szIniFileName);
	DriverOptions.Load(m_szIniFileName);
	ProgramGuideOptions.Load(m_szIniFileName);
	PluginOptions.Load(m_szIniFileName);
	ChannelHistory.Load(m_szIniFileName);
	return true;
}


bool CAppMain::SaveSettings()
{
	CSettings Setting;

	if (Setting.Open(m_szIniFileName,TEXT("Settings"),CSettings::OPEN_WRITE)) {
		int Left,Top,Width,Height;
		int Value;

		MainWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("WindowLeft"),Left);
		Setting.Write(TEXT("WindowTop"),Top);
		Setting.Write(TEXT("WindowWidth"),Width);
		Setting.Write(TEXT("WindowHeight"),Height);
		Setting.Write(TEXT("WindowMaximize"),MainWindow.GetMaximizeStatus());
		Setting.Write(TEXT("AlwaysOnTop"),MainWindow.GetAlwaysOnTop());
		Setting.Write(TEXT("ShowTitleBar"),MainWindow.GetTitleBarVisible());
		Setting.Write(TEXT("Driver"),szDriverFileName);
		Setting.Write(TEXT("Volume"),CoreEngine.GetVolume());
		//Setting.Write(TEXT("VolumeNormalize"),CoreEngine.GetVolumeNormalizeLevel()!=100);
		Setting.Write(TEXT("VolumeNormalizeLevel"),CoreEngine.GetVolumeNormalizeLevel());
		Setting.Write(TEXT("ShowInfoWindow"),fShowPanelWindow);
		Setting.Write(TEXT("ShowStatusBar"),MainWindow.GetStatusBarVisible());
		Setting.Write(TEXT("Mpeg2Decoder"),szMpeg2DecoderName);
		Setting.Write(TEXT("Renderer"),
					CVideoRenderer::EnumRendererName((int)VideoRendererType));
		//Setting.Write(TEXT("NoDescramble"),fNoDescramble);
		Setting.Write(TEXT("CardReader"),(int)CoreEngine.GetCardReaderType());
		Setting.Write(TEXT("DescrambleCurServiceOnly"),fDescrambleCurServiceOnly);
		Setting.Write(TEXT("KeepSingleTask"),fKeepSingleTask);
		Setting.Write(TEXT("Resident"),ResidentManager.GetResident());
		Setting.Write(TEXT("NoScreenSaver"),fNoScreenSaver);
		Setting.Write(TEXT("NoMonitorLowPower"),fNoMonitorLowPower);
		Setting.Write(TEXT("NoMonitorLowPowerActiveOnly"),
												fNoMonitorLowPowerActiveOnly);
		/*
		//Setting.Write(TEXT("CurChannel"),RestoreChannelInfo.Channel);
		Setting.Write(TEXT("CurChannelIndex"),RestoreChannelInfo.Channel);
		Setting.Write(TEXT("CurSpace"),RestoreChannelInfo.Space);
		Setting.Write(TEXT("CurService"),RestoreChannelInfo.Service);
		Setting.Write(TEXT("OldDriver"),RestoreChannelInfo.szDriverName);
		*/
		Setting.Write(TEXT("PacketBuffering"),CoreEngine.GetPacketBuffering());
		Setting.Write(TEXT("PacketBufferLength"),
							(unsigned int)CoreEngine.GetPacketBufferLength());
		Setting.Write(TEXT("PacketBufferPoolPercentage"),
								CoreEngine.GetPacketBufferPoolPercentage());
		/*
		Setting.Write(TEXT("UseOSD"),fUseOSD);
		Setting.Write(TEXT("PseudoOSD"),fUsePseudoOSD);
		Setting.WriteColor(TEXT("OSDTextColor"),crOSDTextColor);
		Setting.Write(TEXT("OSDFadeTime"),OSDFadeTime);
		*/
		Setting.Write(TEXT("EnablePlay"),MainWindow.IsPreview());
		Setting.Write(TEXT("Mute"),CoreEngine.GetMute());
		Setting.Write(TEXT("WheelMode"),WheelMode);
		Setting.Write(TEXT("WheelShiftMode"),WheelShiftMode);
		Setting.Write(TEXT("ReverseWheelChannel"),fWheelChannelReverse);
		Setting.Write(TEXT("WheelChannelDelay"),WheelChannelDelay);
		Setting.Write(TEXT("VolumeStep"),VolumeStep);
		if (RecordManager.GetFileName()!=NULL)
			Setting.Write(TEXT("RecOptionFileName"),RecordManager.GetFileName());
		Setting.Write(TEXT("RecOptionExistsOperation"),
										RecordManager.GetFileExistsOperation());
		/*
		Setting.Write(TEXT("RecOptionStopTimeSpec"),
											RecordManager.GetStopTimeSpec());
		Setting.Write(TEXT("RecOptionStopTime"),RecordManager.GetStopTime());
		*/
		PanelFrame.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("InfoLeft"),Left);
		Setting.Write(TEXT("InfoTop"),Top);
		Setting.Write(TEXT("InfoWidth"),Width);
		Setting.Write(TEXT("InfoHeight"),Height);
		Setting.Write(TEXT("PanelFloating"),PanelFrame.GetFloating());
		Setting.Write(TEXT("PanelDockingWidth"),PanelFrame.GetDockingWidth());
		Setting.Write(TEXT("PanelDockingIndex"),PanelPaneIndex);
		Setting.Write(TEXT("InfoCurTab"),InfoPanel.GetCurTab());
		ProgramGuide.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("ProgramGuideLeft"),Left);
		Setting.Write(TEXT("ProgramGuideTop"),Top);
		Setting.Write(TEXT("ProgramGuideWidth"),Width);
		Setting.Write(TEXT("ProgramGuideHeight"),Height);
		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("CapturePreviewLeft"),Left);
		Setting.Write(TEXT("CapturePreviewTop"),Top);
		Setting.Write(TEXT("CapturePreviewWidth"),Width);
		Setting.Write(TEXT("CapturePreviewHeight"),Height);
		Setting.Write(TEXT("CaptureStatusBar"),CaptureWindow.IsStatusBarVisible());
		//Setting.Write(TEXT("HDUSDriverVersion"),fNewHDUSDriver?2:1);
		Setting.Write(TEXT("IncrementUDPPort"),fIncrementUDPPort);
		ViewOptions.Write(&Setting);
		OSDOptions.Write(&Setting);
		PanelOptions.Write(&Setting);
		AudioOptions.Write(&Setting);
		RecordOptions.Write(&Setting);
		CaptureOptions.Write(&Setting);
		Accelerator.Write(&Setting);
		HDUSController.Write(&Setting);
		ChannelScan.Write(&Setting);
		PluginOptions.Write(&Setting);
		EpgOptions.Write(&Setting);
		NetworkRemoconOptions.Write(&Setting);
		Logger.Write(&Setting);
		Setting.Close();
	}
	StatusOptions.Save(m_szIniFileName);
	ColorSchemeOptions.Save(m_szIniFileName);
	Accelerator.Save(m_szIniFileName);
	HDUSController.Save(m_szIniFileName);
	DriverOptions.Save(m_szIniFileName);
	ProgramGuideOptions.Save(m_szIniFileName);
	PluginOptions.Save(m_szIniFileName);
	ChannelHistory.Save(m_szIniFileName);
	return true;
}


bool CAppMain::SaveCurrentChannel()
{
	if (*CoreEngine.GetDriverFileName()!='\0') {
		const CChannelInfo *pInfo=ChannelManager.GetCurrentRealChannelInfo();
		int Space,Channel,Service;
		CSettings Setting;

		if (pInfo!=NULL) {
			Space=pInfo->GetSpace();
			Channel=pInfo->GetChannelIndex();
			Service=pInfo->GetService();
		} else {
			Space=-1;
			Channel=-1;
			Service=0;
		}
		if (Setting.Open(m_szIniFileName,TEXT("LastChannel"),CSettings::OPEN_WRITE)) {
			TCHAR szDriverName[MAX_PATH],szName[32+MAX_PATH];

			::lstrcpy(szDriverName,::PathFindFileName(CoreEngine.GetDriverFileName()));
			*::PathFindExtension(szDriverName)='\0';
			::wsprintf(szName,TEXT("Space_%s"),szDriverName);
			Setting.Write(szName,Space);
			::wsprintf(szName,TEXT("ChannelIndex_%s"),szDriverName);
			Setting.Write(szName,Channel);
			::wsprintf(szName,TEXT("Service_%s"),szDriverName);
			Setting.Write(szName,Service);
			Setting.Close();
		}
	}
	return true;
}


bool CAppMain::ShowHelpContent(int ID)
{
	return HtmlHelpClass.ShowContent(ID);
}


bool CAppMain::StartRecord(LPCTSTR pszFileName,
						   const CRecordManager::TimeSpecInfo *pStartTime,
						   const CRecordManager::TimeSpecInfo *pStopTime)
{
	if (RecordManager.IsRecording())
		return false;
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	RecordOptions.ApplyOptions(&RecordManager);
	if (CmdLineParser.m_fRecordCurServiceOnly)
		RecordManager.SetCurServiceOnly(true);
	if (RecordManager.IsReserved()) {
		StatusView.UpdateItem(STATUS_ITEM_RECORD);
		return true;
	}

	MainWindow.OpenTuner();

	TCHAR szFileName[MAX_PATH];
	if (pszFileName==NULL) {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFileName(szFileName,lengthof(szFileName),
											NULL,&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		pszFileName=szFileName;
		RecordManager.SetFileName(szFileName);
	}
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,pszFileName)) {
		MainWindow.ShowErrorMessage(&RecordManager);
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),pszFileName);
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	StatusView.UpdateItem(STATUS_ITEM_ERROR);
	MainMenu.EnableItem(CM_RECORDOPTION,false);
	MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
	PluginList.SendRecordStatusChangeEvent();
	return true;
}


bool CAppMain::ModifyRecord(LPCTSTR pszFileName,
							const CRecordManager::TimeSpecInfo *pStartTime,
							const CRecordManager::TimeSpecInfo *pStopTime)
{
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppMain::StartReservedRecord()
{
	TCHAR szFileName[MAX_PATH];

	/*
	if (!RecordManager.IsReserved())
		return false;
	*/
	if (RecordManager.GetFileName()!=NULL) {
		if (!RecordManager.DoFileExistsOperation(MainWindow.GetVideoHostWindow(),szFileName))
			return false;
	} else {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFileName(szFileName,lengthof(szFileName),
													NULL,&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		RecordManager.SetFileName(szFileName);
	}
	MainWindow.OpenTuner();
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
		RecordManager.CancelReserve();
		MainWindow.ShowErrorMessage(&RecordManager);
		return false;
	}
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	StatusView.UpdateItem(STATUS_ITEM_ERROR);
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	MainMenu.EnableItem(CM_RECORDOPTION,false);
	MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
	PluginList.SendRecordStatusChangeEvent();
	return true;
}


bool CAppMain::CancelReservedRecord()
{
	if (!RecordManager.CancelReserve())
		return false;
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


void CAppMain::BeginChannelScan()
{
	m_fChannelScanning=true;
}


void CAppMain::EndChannelScan()
{
	m_fChannelScanning=false;
}


bool CAppMain::IsDriverNoSignalLevel(LPCTSTR pszFileName) const
{
	return DriverOptions.IsNoSignalLevel(pszFileName);
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
}


COLORREF CAppMain::GetColor(LPCTSTR pszText) const
{
	return ColorSchemeOptions.GetColor(pszText);
}


CCoreEngine *CAppMain::GetCoreEngine()
{
	return &CoreEngine;
}


const CCoreEngine *CAppMain::GetCoreEngine() const
{
	return &CoreEngine;
}


CMainWindow *CAppMain::GetMainWindow()
{
	return &MainWindow;
}


const CChannelManager *CAppMain::GetChannelManager() const
{
	return &ChannelManager;
}


const CRecordManager *CAppMain::GetRecordManager() const
{
	return &RecordManager;
}


CAppMain &GetAppClass()
{
	return AppMain;
}




static void SetDisplayStatus()
{
	if (!fNoScreenSaver && fScreenSaverActive) {
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
		fScreenSaverActive=FALSE;
	}
	if (!fNoMonitorLowPower) {
		if (fPowerOffActiveOriginal) {
			SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			fPowerOffActiveOriginal=FALSE;
		}
		if (fLowPowerActiveOriginal) {
			SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			fLowPowerActiveOriginal=FALSE;
		}
	}
	if (fNoScreenSaver && fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
		// SetThreadExecutionState() を呼ぶタイマー
		SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_DISPLAY,30000,NULL);
	} else {
		KillTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_DISPLAY);
		if (fNoScreenSaver && !fScreenSaverActive) {
			if (!SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,
														&fScreenSaverActive,0))
				fScreenSaverActive=FALSE;
			if (fScreenSaverActive)
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
		}
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
			if (!fPowerOffActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETPOWEROFFACTIVE,0,
												&fPowerOffActiveOriginal,0))
					fPowerOffActiveOriginal=FALSE;
				if (fPowerOffActiveOriginal)
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			if (!fLowPowerActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETLOWPOWERACTIVE,0,
												&fLowPowerActiveOriginal,0))
					fLowPowerActiveOriginal=FALSE;
				if (fLowPowerActiveOriginal)
					SystemParametersInfo(SPI_SETLOWPOWERACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
		}
	}
}




class CChannelStatusItem : public CStatusItem {
public:
	CChannelStatusItem();
	LPCTSTR GetName() const { return TEXT("チャンネル"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CChannelStatusItem::CChannelStatusItem() : CStatusItem(STATUS_ITEM_CHANNEL,96)
{
}

void CChannelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CChannelInfo *pInfo;
	TCHAR szText[4+MAX_CHANNEL_NAME];

	if (fWheelChannelChanging) {
		COLORREF crText,crBack;

		crText=::GetTextColor(hdc);
		crBack=::GetBkColor(hdc);
		::SetTextColor(hdc,MixColor(crText,crBack,128));
		pInfo=ChannelManager.GetChangingChannelInfo();
		::wsprintf(szText,TEXT("%d: %s"),pInfo->GetChannelNo(),pInfo->GetName());
	} else if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		::wsprintf(szText,TEXT("%d: %s"),pInfo->GetChannelNo(),pInfo->GetName());
	} else
		::lstrcpy(szText,TEXT("<チャンネル>"));
	DrawText(hdc,pRect,szText);
}

void CChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("アフリカ中央テレビ"));
}

void CChannelStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	const CChannelList *pList;

	GetMenuPos(&pt);
	if (!CoreEngine.IsNetworkDriver()
			&& (pList=ChannelManager.GetCurrentChannelList())!=NULL
			&& pList->NumChannels()<=20) {
		ChannelMenu.Create(pList);
		ChannelMenu.Popup(TPM_RIGHTBUTTON,pt.x,pt.y,MainWindow.GetHandle());
		ChannelMenu.Destroy();
	} else {
		MainMenu.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,TPM_RIGHTBUTTON,
											pt.x,pt.y,MainWindow.GetHandle());
	}
}

void CChannelStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_SERVICE,TPM_RIGHTBUTTON,
											pt.x,pt.y,MainWindow.GetHandle());
}


class CVideoSizeStatusItem : public CStatusItem {
public:
	CVideoSizeStatusItem();
	LPCTSTR GetName() const { return TEXT("映像サイズ"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CVideoSizeStatusItem::CVideoSizeStatusItem() : CStatusItem(STATUS_ITEM_VIDEOSIZE,120)
{
}

void CVideoSizeStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	TCHAR szText[32];

	::wsprintf(szText,TEXT("%d x %d (%d %%)"),
			   CoreEngine.GetOriginalVideoWidth(),
			   CoreEngine.GetOriginalVideoHeight(),
			   MainWindow.CalcZoomRate());
	DrawText(hdc,pRect,szText);
}

void CVideoSizeStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("1920 x 1080 (100 %)"));
}

void CVideoSizeStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_ZOOM,TPM_RIGHTBUTTON,
											pt.x,pt.y,MainWindow.GetHandle());
}

void CVideoSizeStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO,TPM_RIGHTBUTTON,
											pt.x,pt.y,MainWindow.GetHandle());
}


class CVolumeStatusItem : public CStatusItem {
public:
	CVolumeStatusItem();
	LPCTSTR GetName() const { return TEXT("音量"); }
	void Draw(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
	void OnMouseMove(int x,int y);
};

CVolumeStatusItem::CVolumeStatusItem() : CStatusItem(STATUS_ITEM_VOLUME,80)
{
}

void CVolumeStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	HPEN hpen,hpenOld;
	HBRUSH hbr,hbrOld;
	RECT rc;
	COLORREF crBar;

	hpen=CreatePen(PS_SOLID,1,GetTextColor(hdc));
	hpenOld=(HPEN)SelectObject(hdc,hpen);
	hbrOld=(HBRUSH)SelectObject(hdc,GetStockObject(NULL_BRUSH));
	rc.left=pRect->left;
	rc.top=pRect->top+(pRect->bottom-pRect->top-8)/2;
	rc.right=pRect->right;
	rc.bottom=rc.top+8;
	Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectObject(hdc,hbrOld);
	SelectObject(hdc,hpenOld);
	DeleteObject(hpen);
	if (!CoreEngine.GetMute()) {
		crBar=GetTextColor(hdc);
	} else {
		crBar=MixColor(GetTextColor(hdc),GetBkColor(hdc),128);
	}
	hbr=CreateSolidBrush(crBar);
	rc.left+=2;
	rc.top+=2;
	rc.right=rc.left+(rc.right-2-rc.left)*CoreEngine.GetVolume()/CCoreEngine::MAX_VOLUME;
	rc.bottom-=2;
	FillRect(hdc,&rc,hbr);
	DeleteObject(hbr);
}

void CVolumeStatusItem::OnLButtonDown(int x,int y)
{
	OnMouseMove(x,y);
	SetCapture(m_pStatus->GetHandle());
}

void CVolumeStatusItem::OnRButtonDown(int x,int y)
{
	// メニューを出すようにしたら評判悪かった...
	/*
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_VOLUME,TPM_RIGHTBUTTON,pt.x,pt.y,
													MainWindow.GetHandle());
	*/
	MainWindow.SendCommand(CM_VOLUME_MUTE);
}

void CVolumeStatusItem::OnMouseMove(int x,int y)
{
	RECT rc;
	int Volume;

	GetClientRect(&rc);
	Volume=(x-2)*100/(rc.right-rc.left-4-1);
	if (Volume<0)
		Volume=0;
	else if (Volume>CCoreEngine::MAX_VOLUME)
		Volume=CCoreEngine::MAX_VOLUME;
	if (CoreEngine.GetMute() || Volume!=CoreEngine.GetVolume())
		MainWindow.SetVolume(Volume,false);
}


class CAudioChannelStatusItem : public CStatusItem {
public:
	CAudioChannelStatusItem();
	LPCTSTR GetName() const { return TEXT("音声"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CAudioChannelStatusItem::CAudioChannelStatusItem() : CStatusItem(STATUS_ITEM_AUDIOCHANNEL,56)
{
}

void CAudioChannelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	int NumChannels=CoreEngine.m_DtvEngine.GetAudioChannelNum();
	TCHAR szText[32];

	if (NumChannels>0) {
		switch (NumChannels) {
		case 1:
			lstrcpy(szText,TEXT("Mono"));
			break;
		case 2:
			lstrcpy(szText,TEXT("Stereo"));
			if (CoreEngine.GetStereoMode()!=0)
				lstrcat(szText,CoreEngine.GetStereoMode()==1?TEXT("(L)"):TEXT("(R)"));
			break;
		case 6:
			lstrcpy(szText,TEXT("5.1ch"));
			break;
		default:
			wsprintf(szText,TEXT("%dch"),NumChannels);
			break;
		}
	} else
		lstrcpy(szText,TEXT("<音声>"));
	DrawText(hdc,pRect,szText);
}

void CAudioChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("Stereo"));
}

void CAudioChannelStatusItem::OnLButtonDown(int x,int y)
{
	if (!MainWindow.SwitchAudio())
		OnRButtonDown(x,y);
}

void CAudioChannelStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_STEREOMODE,TPM_RIGHTBUTTON,
											pt.x,pt.y,MainWindow.GetHandle());
}


class CRecordStatusItem : public CStatusItem {
public:
	CRecordStatusItem();
	LPCTSTR GetName() const { return TEXT("録画"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CRecordStatusItem::CRecordStatusItem() : CStatusItem(STATUS_ITEM_RECORD,64)
{
}

void CRecordStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	int FontHeight=m_pStatus->GetFontHeight();
	RECT rc;
	TCHAR szText[32],*pszText;

	rc=*pRect;
	if (RecordManager.IsRecording()) {
		DWORD RecordSec=RecordManager.GetRecordTime()/1000;

		if (RecordManager.IsPaused()) {
			HBRUSH hbr=::CreateSolidBrush(::GetTextColor(hdc));
			RECT rc1;

			rc1.left=rc.left;
			rc1.top=rc.top+((rc.bottom-rc.top)-FontHeight)/2;
			rc1.right=rc1.left+FontHeight/2-1;
			rc1.bottom=rc1.top+FontHeight;
			::FillRect(hdc,&rc1,hbr);
			rc1.left=rc1.right+2;
			rc1.right=rc.left+FontHeight;
			::FillRect(hdc,&rc1,hbr);
			::DeleteObject(hbr);
		} else {
			/*
			HBRUSH hbrOld;
			HPEN hpenOld;

			rc1.right=rc1.left+FontHeight;
			hbrOld=SelectBrush(hdc,hbr);
			hpenOld=SelectPen(hdc,::GetStockObject(NULL_PEN));
			::Ellipse(hdc,rc1.left,rc1.top,rc1.right,rc1.bottom);
			SelectPen(hdc,hpenOld);
			SelectBrush(hdc,hbrOld);
			*/
			// Ellipseで小さい丸を描くと汚い
			::DrawText(hdc,TEXT("●"),-1,&rc,
										DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		}
		rc.left+=FontHeight+4;
		::wsprintf(szText,TEXT("%d:%02d:%02d"),
					RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60);
		pszText=szText;
	} else if (RecordManager.IsReserved()) {
		pszText=TEXT("■ 録画待機");
	} else {
		pszText=TEXT("■ <録画>");
	}
	DrawText(hdc,&rc,pszText);
}

void CRecordStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("● 0:24:15"));
}

void CRecordStatusItem::OnLButtonDown(int x,int y)
{
	MainWindow.SendCommand(RecordManager.IsReserved()?CM_RECORDOPTION:CM_RECORD);
}

void CRecordStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_RECORD));
	EnableMenuItem(hmenu,CM_RECORD_PAUSE,
		MF_BYCOMMAND | (RecordManager.IsRecording()?MFS_ENABLED:MFS_GRAYED));
	CheckMenuItem(hmenu,CM_RECORD_PAUSE,
		MF_BYCOMMAND | (RecordManager.IsPaused()?MFS_CHECKED:MFS_UNCHECKED));
	EnableMenuItem(hmenu,CM_RECORDSTOPTIME,
		MF_BYCOMMAND | (RecordManager.IsRecording()?MFS_ENABLED:MFS_GRAYED));
	CheckMenuItem(hmenu,CM_EXITONRECORDINGSTOP,
		MF_BYCOMMAND | (MainWindow.GetExitOnRecordingStop()?MFS_CHECKED:MFS_UNCHECKED));
	CheckMenuItem(hmenu,CM_DISABLEVIEWER,
		MF_BYCOMMAND | (MainWindow.IsPreview()?MFS_UNCHECKED:MFS_CHECKED));
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt);
	TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
}


class CCaptureStatusItem : public CStatusItem {
	HBITMAP m_hbmIcon;
public:
	CCaptureStatusItem();
	~CCaptureStatusItem();
	LPCTSTR GetName() const { return TEXT("キャプチャ"); }
	void Draw(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CCaptureStatusItem::CCaptureStatusItem() : CStatusItem(STATUS_ITEM_CAPTURE,16)
{
	m_MinWidth=16;
	m_hbmIcon=NULL;
}

CCaptureStatusItem::~CCaptureStatusItem()
{
	if (m_hbmIcon!=NULL)
		DeleteObject(m_hbmIcon);
}

void CCaptureStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	if (m_hbmIcon==NULL)
		m_hbmIcon=static_cast<HBITMAP>(LoadImage(hInst,
								MAKEINTRESOURCE(IDB_CAPTURE),IMAGE_BITMAP,0,0,
								LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));
	DrawIcon(hdc,pRect,m_hbmIcon,0,0,16,16);
}

void CCaptureStatusItem::OnLButtonDown(int x,int y)
{
	MainWindow.SendCommand(CM_CAPTURE);
}

void CCaptureStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_CAPTURE));
	CheckMenuRadioItem(hmenu,CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize(),MF_BYCOMMAND);
	if (fShowCaptureWindow)
		CheckMenuItem(hmenu,CM_CAPTUREPREVIEW,MF_BYCOMMAND | MFS_CHECKED);
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt);
	TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
}


class CErrorStatusItem : public CStatusItem {
public:
	CErrorStatusItem();
	LPCTSTR GetName() const { return TEXT("エラー"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
};

CErrorStatusItem::CErrorStatusItem() : CStatusItem(STATUS_ITEM_ERROR,120)
{
}

void CErrorStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	TCHAR szText[64];

#if 0
	int Length;
	Length=wsprintf(szText,TEXT("D %u / E %u"),
					CoreEngine.GetContinuityErrorPacketCount(),
					CoreEngine.GetErrorPacketCount());
	if (CoreEngine.GetDescramble()
			&& CoreEngine.GetCardReaderType()!=CCardReader::READER_NONE)
		wsprintf(szText+Length,TEXT(" / S %u"),CoreEngine.GetScramblePacketCount());
#else
	::wsprintf(szText,TEXT("D %u / E %u / S %u"),
			   CoreEngine.GetContinuityErrorPacketCount(),
			   CoreEngine.GetErrorPacketCount(),
			   CoreEngine.GetScramblePacketCount());
#endif
	DrawText(hdc,pRect,szText);
}

void CErrorStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("D 0 / E 0 / S 127"));
}

void CErrorStatusItem::OnLButtonDown(int x,int y)
{
	CoreEngine.ResetErrorCount();
	Update();
	InfoWindow.UpdateErrorCount();
}


class CSignalLevelStatusItem : public CStatusItem {
public:
	CSignalLevelStatusItem();
	LPCTSTR GetName() const { return TEXT("信号レベル"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
};

CSignalLevelStatusItem::CSignalLevelStatusItem() : CStatusItem(STATUS_ITEM_SIGNALLEVEL,120)
{
}

void CSignalLevelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	TCHAR szText[64];
	// 情報パネルと合わせるためにfloatで計算する
	int BitRate=(int)(CoreEngine.GetBitRateFloat()*100.0f);

	if (CoreEngine.IsNetworkDriver()
			//|| (DriverType==CCoreEngine::DRIVER_HDUS && !fNewHDUSDriver)) {
			|| DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName())) {
		// ビットレートのみ
		wsprintf(szText,TEXT("%d.%02d Mbps"),BitRate/100,BitRate%100);
	} else {
		int Level=(int)(CoreEngine.GetSignalLevel()*100.0f);

		wsprintf(szText,TEXT("%d.%02d dB / %d.%02d Mbps"),
								Level/100,Level%100,BitRate/100,BitRate%100);
	}
	DrawText(hdc,pRect,szText);
}

void CSignalLevelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("52.30 dB / 16.73 Mbps"));
}


class CClockStatusItem : public CStatusItem {
public:
	CClockStatusItem();
	LPCTSTR GetName() const { return TEXT("時計"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
};

CClockStatusItem::CClockStatusItem() : CStatusItem(STATUS_ITEM_CLOCK,48)
{
}

void CClockStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	SYSTEMTIME st;
	TCHAR szText[64];

	GetLocalTime(&st);
	wsprintf(szText,TEXT("%d:%02d:%02d"),st.wHour,st.wMinute,st.wSecond);
	DrawText(hdc,pRect,szText);
}

void CClockStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("13:25:30"));
}


class CProgramInfoStatusItem : public CStatusItem {
	bool m_fNext;
public:
	CProgramInfoStatusItem();
	LPCTSTR GetName() const { return TEXT("番組情報"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CProgramInfoStatusItem::CProgramInfoStatusItem() : CStatusItem(STATUS_ITEM_PROGRAMINFO,256)
{
	m_fNext=false;
}

void CProgramInfoStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CEpgDataInfo *pInfo=CoreEngine.GetEpgDataInfo(m_fNext);

	if (pInfo!=NULL) {
		TCHAR szText[256];
		SYSTEMTIME stStart,stEnd;

		pInfo->GetStartTime(&stStart);
		pInfo->GetEndTime(&stEnd);
		::wnsprintf(szText,lengthof(szText),L"%s%d:%02d～%d:%02d %s",
					m_fNext?TEXT("次: "):TEXT(""),
					stStart.wHour,
					stStart.wMinute,
					stEnd.wHour,
					stEnd.wMinute,
					pInfo->GetEventName());
		DrawText(hdc,pRect,szText);
	}
}

void CProgramInfoStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("1:00～1:30 今日のニュース"));
}

void CProgramInfoStatusItem::OnLButtonDown(int x,int y)
{
	m_fNext=!m_fNext;
	Update();
}

void CProgramInfoStatusItem::OnRButtonDown(int x,int y)
{
}


class CBufferingStatusItem : public CStatusItem {
public:
	CBufferingStatusItem();
	LPCTSTR GetName() const { return TEXT("バッファリング"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
};

CBufferingStatusItem::CBufferingStatusItem()
	: CStatusItem(STATUS_ITEM_BUFFERING,80)
{
}

void CBufferingStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	TCHAR szText[32];

	::wsprintf(szText,TEXT("R %lu / B %d%%"),
		CoreEngine.GetStreamRemain(),CoreEngine.GetPacketBufferUsedPercentage());
	DrawText(hdc,pRect,szText);
}

void CBufferingStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("R 52 / B 48%"));
}

void CBufferingStatusItem::OnLButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_BUFFERING));
	CheckMenuItem(hmenu,CM_ENABLEBUFFERING,
		MF_BYCOMMAND | (CoreEngine.GetPacketBuffering()?MFS_CHECKED:MFS_UNCHECKED));
	EnableMenuItem(hmenu,CM_RESETBUFFER,
		MF_BYCOMMAND | (CoreEngine.GetPacketBuffering()?MFS_ENABLED:MFS_GRAYED));
	GetMenuPos(&pt);
	TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
}


class CTunerStatusItem : public CStatusItem {
public:
	CTunerStatusItem();
	LPCTSTR GetName() const { return TEXT("チューナー"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CTunerStatusItem::CTunerStatusItem() : CStatusItem(STATUS_ITEM_TUNER,80)
{
}

void CTunerStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
	LPCTSTR pszText;

	if (pChInfo!=NULL || ChannelManager.GetCurrentSpace()>=0) {
		pszText=
			ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(
				pChInfo!=NULL?pChInfo->GetSpace():ChannelManager.GetCurrentSpace());
		if (pszText==NULL)
			pszText=TEXT("<チューナー>");
	} else if (ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL) {
		pszText=TEXT("すべて");
	} else {
		pszText=TEXT("<チューナー>");
	}
	DrawText(hdc,pRect,pszText);
}

void CTunerStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("地デジ"));
}

void CTunerStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_SPACE,TPM_RIGHTBUTTON,pt.x,pt.y,
						  MainWindow.GetHandle());
}

void CTunerStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu=AppMain.CreateTunerSelectMenu();
	POINT pt;

	GetMenuPos(&pt);
	::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON,pt.x,pt.y,0,MainWindow.GetHandle(),NULL);
	::DestroyMenu(hmenu);
}




class CVolumeControlItem : public CControlPanelItem {
public:
	CVolumeControlItem();
	void Draw(HDC hdc);
	bool Rayout(int Width,int Height);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
	void OnMouseMove(int x,int y);
};

CVolumeControlItem::CVolumeControlItem()
{
	m_Position.Left=4;
	m_Position.Height=8+4*2;
}

void CVolumeControlItem::Draw(HDC hdc)
{
	HPEN hpen,hpenOld;
	HBRUSH hbr,hbrOld;
	RECT rc;
	COLORREF crBar;

	hpen=CreatePen(PS_SOLID,1,GetTextColor(hdc));
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,GetStockObject(NULL_BRUSH));
	GetPosition(&rc);
	rc.left+=4;
	rc.right-=4;
	rc.top+=(rc.bottom-rc.top-8)/2;
	rc.bottom=rc.top+8;
	Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectBrush(hdc,hbrOld);
	SelectPen(hdc,hpenOld);
	DeleteObject(hpen);
	if (!CoreEngine.GetMute()) {
		crBar=GetTextColor(hdc);
	} else {
		COLORREF crText,crBk;

		crText=GetTextColor(hdc);
		crBk=GetBkColor(hdc);
		crBar=RGB((GetRValue(crText)+GetRValue(crBk))/2,
				  (GetGValue(crText)+GetGValue(crBk))/2,
				  (GetBValue(crText)+GetBValue(crBk))/2);
	}
	hbr=CreateSolidBrush(crBar);
	rc.left+=2;
	rc.top+=2;
	rc.right=rc.left+(rc.right-2-rc.left)*CoreEngine.GetVolume()/CCoreEngine::MAX_VOLUME;
	rc.bottom-=2;
	FillRect(hdc,&rc,hbr);
	DeleteObject(hbr);
}

bool CVolumeControlItem::Rayout(int Width,int Height)
{
	RECT rc;

	m_pControlPanel->GetItemPosition(CONTROLPANEL_ITEM_VOLUME-1,&rc);
	m_Position.Top=rc.bottom;
	m_Position.Width=Width-4*2;
	return true;
}

void CVolumeControlItem::OnLButtonDown(int x,int y)
{
	OnMouseMove(x,y);
	SetCapture(m_pControlPanel->GetHandle());
}

void CVolumeControlItem::OnRButtonDown(int x,int y)
{
	MainWindow.SendCommand(CM_VOLUME_MUTE);
}

void CVolumeControlItem::OnMouseMove(int x,int y)
{
	RECT rc;
	int Volume;

	GetPosition(&rc);
	Volume=(x-(4+2))*100/(rc.right-rc.left-(4+2)*2-1);
	if (Volume<0)
		Volume=0;
	else if (Volume>CCoreEngine::MAX_VOLUME)
		Volume=CCoreEngine::MAX_VOLUME;
	if (CoreEngine.GetMute() || Volume!=CoreEngine.GetVolume())
		MainWindow.SetVolume(Volume,false);
}


class COptionsControlItem : public CControlPanelButton {
public:
	COptionsControlItem();
	bool Rayout(int Width,int Height);
};

COptionsControlItem::COptionsControlItem() :
		CControlPanelButton(CM_OPTIONS,TEXT("設定"),4,0,0,0)
{
}

bool COptionsControlItem::Rayout(int Width,int Height)
{
	RECT rc;
	int FontHeight=m_pControlPanel->GetFontHeight();

	m_pControlPanel->GetItemPosition(CONTROLPANEL_ITEM_OPTIONS-1,&rc);
	m_Position.Top=rc.bottom+4;
	m_Position.Width=FontHeight*2+8;
	m_Position.Height=FontHeight+8;
	return true;
}




static bool ColorSchemeApplyProc(const CColorScheme *pColorScheme)
{
	StatusView.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
	TitleBar.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
	PanelFrame.SetTitleColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTITLEBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTITLETEXT));
	InfoPanel.SetBackColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABMARGIN),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK));
	InfoPanel.SetTabColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABBORDER));
	InfoPanel.SetCurTabColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELCURTABBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELCURTABTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELCURTABBORDER));
	InfoWindow.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT));
	InfoWindow.SetProgramInfoColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOTEXT));
	ProgramListView.SetColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTTITLEBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTTITLETEXT));
	ChannelPanel.SetColors(
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELCHANNELNAMEBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELCHANNELNAMETEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELEVENTNAMEBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELEVENTNAMETEXT));
	ControlPanel.SetColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELHIGHLIGHTBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELHIGHLIGHTTEXT));
	for (int i=0;i<=CProgramGuide::COLOR_LAST;i++)
		ProgramGuide.SetColor(i,pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_FIRST+i));
	ProgramGuide.Invalidate();
	/*
	NotificationBar.SetColors(
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT));
	*/
	CaptureWindow.SetStatusColor(
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK1),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTBACK2),
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
	PluginList.SendColorChangeEvent();
	return true;
}




class CGeneralOptions : public COptions {
	static CGeneralOptions *GetThis(HWND hDlg);
public:
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

CGeneralOptions *CGeneralOptions::GetThis(HWND hDlg)
{
	return static_cast<CGeneralOptions*>(::GetProp(hDlg,TEXT("This")));
}

BOOL CALLBACK CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		OnInitDialog(hDlg,lParam);
		{
			TCHAR szDirectory[MAX_PATH];

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVER,CB_LIMITTEXT,MAX_PATH-1,0);
			AppMain.GetAppDirectory(szDirectory);
			/*
			DriverManager.Find(szDirectory);
			AppMain.UpdateDriverMenu();
			*/
			for (int i=0;i<DriverManager.NumDrivers();i++) {
				const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

				::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVER,CB_ADDSTRING,
					0,reinterpret_cast<LPARAM>(pDriverInfo->GetFileName()));
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVER,szDriverFileName);
		}
		{
			CDirectShowFilterFinder FilterFinder;
			WCHAR szFilterName[MAX_MPEG2_DECODER_NAME];
			int Sel=-1;

			if (szMpeg2DecoderName[0]=='\0')
				Sel=0;
			SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,
									CB_ADDSTRING,0,(LPARAM)TEXT("デフォルト"));
			if (FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO)) {
				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
						int Index=SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,
							CB_ADDSTRING,0,(LPARAM)szFilterName);
						if (lstrcmpi(szFilterName,szMpeg2DecoderName)==0)
							Sel=Index;
					}
				}
			}
			SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,CB_SETCURSEL,Sel,0);
		}
		{
			LPCTSTR pszName;

			SendDlgItemMessage(hDlg,IDC_OPTIONS_RENDERER,CB_ADDSTRING,0,
								reinterpret_cast<LPARAM>(TEXT("デフォルト")));
			for (int i=1;(pszName=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				SendDlgItemMessage(hDlg,IDC_OPTIONS_RENDERER,CB_ADDSTRING,0,
								reinterpret_cast<LPARAM>(pszName));
			SendDlgItemMessage(hDlg,IDC_OPTIONS_RENDERER,CB_SETCURSEL,(WPARAM)VideoRendererType,0);
		}
		{
			static const LPCTSTR pszCardReaderList[] = {
				TEXT("なし(スクランブル解除しない)"),
				TEXT("スマートカードリーダ"),
				TEXT("HDUS内蔵カードリーダ")
			};

			for (int i=0;i<lengthof(pszCardReaderList);i++)
				SendDlgItemMessage(hDlg,IDC_OPTIONS_CARDREADER,CB_ADDSTRING,0,(LPARAM)pszCardReaderList[i]);
			SendDlgItemMessage(hDlg,IDC_OPTIONS_CARDREADER,CB_SETCURSEL,(WPARAM)CoreEngine.GetCardReaderType(),0);
		}
		CheckDlgButton(hDlg,IDC_OPTIONS_RESIDENT,
			ResidentManager.GetResident()?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_KEEPSINGLETASK,
									fKeepSingleTask?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOSCREENSAVER,
									fNoScreenSaver?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER,
								fNoMonitorLowPower?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
					fNoMonitorLowPowerActiveOnly?BST_CHECKED:BST_UNCHECKED);
		EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
														fNoMonitorLowPower);
		CheckDlgButton(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,
					   fDescrambleCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
		/*
		CheckDlgButton(hDlg,IDC_OPTIONS_NEWHDUSDRIVER,
									fNewHDUSDriver?BST_CHECKED:BST_UNCHECKED);
		*/
		// Buffering
		CheckDlgButton(hDlg,IDC_OPTIONS_ENABLEBUFFERING,
					CoreEngine.GetPacketBuffering()?BST_CHECKED:BST_UNCHECKED);
		EnableDlgItems(hDlg,IDC_OPTIONS_BUFFERING_FIRST,IDC_OPTIONS_BUFFERING_LAST,
					CoreEngine.GetPacketBuffering());
		SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,CoreEngine.GetPacketBufferLength(),FALSE);
		SendDlgItemMessage(hDlg,IDC_OPTIONS_BUFFERSIZE_UD,UDM_SETRANGE32,1,INT_MAX);
		SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,
					  CoreEngine.GetPacketBufferPoolPercentage(),TRUE);
		SendDlgItemMessage(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UD,UDM_SETRANGE32,0,100);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriverの選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVER,szFileName);
				}
			}
			return TRUE;

		case IDC_OPTIONS_NOMONITORLOWPOWER:
			EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
					IsDlgButtonChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER)==
																BST_CHECKED);
			return TRUE;

		case IDC_OPTIONS_ENABLEBUFFERING:
			EnableDlgItems(hDlg,IDC_OPTIONS_BUFFERING_FIRST,IDC_OPTIONS_BUFFERING_LAST,
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_ENABLEBUFFERING)==BST_CHECKED);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CGeneralOptions *pThis=GetThis(hDlg);
				TCHAR szDriver[MAX_PATH];

				GetDlgItemText(hDlg,IDC_OPTIONS_DRIVER,szDriver,lengthof(szDriver));
				if (szDriver[0]!='\0'
						&& lstrcmpi(szDriver,szDriverFileName)!=0) {
					::lstrcpy(szDriverFileName,szDriver);
					pThis->m_UpdateFlags|=UPDATE_DRIVER;
				}
			}
			{
				TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
				int Sel=SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,
															CB_GETCURSEL,0,0);
				if (Sel>0)
					SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,CB_GETLBTEXT,
											Sel,(LPARAM)szDecoder);
				else
					szDecoder[0]='\0';
				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					SendDlgItemMessage(hDlg,IDC_OPTIONS_RENDERER,CB_GETCURSEL,0,0);
				if (::lstrcmpi(szDecoder,szMpeg2DecoderName)!=0
						|| Renderer!=VideoRendererType) {
					::lstrcpy(szMpeg2DecoderName,szDecoder);
					VideoRendererType=Renderer;
					if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
						bool fPreview=MainWindow.IsPreview();

						CoreEngine.m_DtvEngine.SetTracer(&StatusView);
						MainWindow.BuildMediaViewer();
						CoreEngine.m_DtvEngine.SetTracer(NULL);
						StatusView.SetSingleText(NULL);
						if (fPreview)
							CoreEngine.EnablePreview(true);
					}
				}
				if (!CoreEngine.SetCardReaderType((CCardReader::ReaderType)
						SendDlgItemMessage(hDlg,IDC_OPTIONS_CARDREADER,CB_GETCURSEL,0,0))) {
					TCHAR szText[256];

					Logger.AddLog(CoreEngine.GetLastErrorText());
					CoreEngine.FormatLastErrorText(szText,lengthof(szText));
					::MessageBox(hDlg,szText,NULL,MB_OK | MB_ICONEXCLAMATION);
					CoreEngine.SetCardReaderType(CCardReader::READER_NONE);
				}
			}
			ResidentManager.SetResident(
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_RESIDENT)==BST_CHECKED);
			fKeepSingleTask=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_KEEPSINGLETASK)==BST_CHECKED;
			fNoScreenSaver=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_NOSCREENSAVER)==BST_CHECKED;
			fNoMonitorLowPower=IsDlgButtonChecked(hDlg,
								IDC_OPTIONS_NOMONITORLOWPOWER)==BST_CHECKED;
			fNoMonitorLowPowerActiveOnly=IsDlgButtonChecked(hDlg,
						IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY)==BST_CHECKED;
			SetDisplayStatus();
			{
				bool fCurOnly=IsDlgButtonChecked(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY)==BST_CHECKED;
				if (fCurOnly!=fDescrambleCurServiceOnly) {
					fDescrambleCurServiceOnly=fCurOnly;
					if (!RecordManager.IsRecording())
						CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(fDescrambleCurServiceOnly);
				}
			}
			/*
			fNewHDUSDriver=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_NEWHDUSDRIVER)==BST_CHECKED;
			*/
			CoreEngine.SetPacketBuffering(IsDlgButtonChecked(hDlg,IDC_OPTIONS_ENABLEBUFFERING)==BST_CHECKED);
			CoreEngine.SetPacketBufferLength(GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,NULL,FALSE));
			CoreEngine.SetPacketBufferPoolPercentage(
				GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,NULL,TRUE));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CGeneralOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


class COperationOptions : public COptions {
public:
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

BOOL CALLBACK COperationOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			static const LPCTSTR pszWheelMode[] = {
				TEXT("なし"),TEXT("音量"),TEXT("チャンネル")
			};
			int i;

			for (i=0;i<lengthof(pszWheelMode);i++)
				SendDlgItemMessage(hDlg,IDC_OPTIONS_WHEELMODE,CB_ADDSTRING,0,
									reinterpret_cast<LPARAM>(pszWheelMode[i]));
			SendDlgItemMessage(hDlg,IDC_OPTIONS_WHEELMODE,CB_SETCURSEL,
																WheelMode,0);
			for (i=0;i<lengthof(pszWheelMode);i++)
				SendDlgItemMessage(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,CB_ADDSTRING,
								0,reinterpret_cast<LPARAM>(pszWheelMode[i]));
			SendDlgItemMessage(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,CB_SETCURSEL,
															WheelShiftMode,0);
			CheckDlgButton(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE,
							fWheelChannelReverse?BST_CHECKED:BST_UNCHECKED);
			SetDlgItemInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,
													WheelChannelDelay,FALSE);
			SetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,VolumeStep,TRUE);
			SendDlgItemMessage(hDlg,IDC_OPTIONS_VOLUMESTEP_UD,UDM_SETRANGE32,1,100);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				WheelMode=SendDlgItemMessage(hDlg,IDC_OPTIONS_WHEELMODE,
															CB_GETCURSEL,0,0);
				WheelShiftMode=SendDlgItemMessage(hDlg,
								IDC_OPTIONS_WHEELSHIFTMODE,CB_GETCURSEL,0,0);
				fWheelChannelReverse=IsDlgButtonChecked(hDlg,
								IDC_OPTIONS_WHEELCHANNELREVERSE)==BST_CHECKED;
				WheelChannelDelay=GetDlgItemInt(hDlg,
									IDC_OPTIONS_WHEELCHANNELDELAY,NULL,FALSE);
			}
			VolumeStep=::GetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,NULL,TRUE);
			break;
		}
		break;
	}
	return FALSE;
}


static CGeneralOptions GeneralOptions;
static COperationOptions OperationOptions;


class COptionDialog {
public:
	enum {
		PAGE_GENERAL,
		PAGE_VIEW,
		PAGE_OSD,
		PAGE_STATUS,
		PAGE_PANEL,
		PAGE_COLORSCHEME,
		PAGE_OPERATION,
		PAGE_ACCELERATOR,
		PAGE_HDUSCONTROLLER,
		PAGE_DRIVER,
		PAGE_AUDIO,
		PAGE_RECORD,
		PAGE_CAPTURE,
		PAGE_CHANNELSCAN,
		PAGE_EPG,
		PAGE_PROGRAMGUIDE,
		PAGE_PLUGIN,
		PAGE_NETWORKREMOCON,
		PAGE_LOG,
		PAGE_LAST=PAGE_LOG
	};
	COptionDialog();
	~COptionDialog();
	bool ShowDialog(HWND hwndOwner,int StartPage=-1);
private:
	enum { NUM_PAGES=PAGE_LAST+1 };
	struct PageInfo {
		LPCTSTR pszTitle;
		LPCTSTR pszTemplate;
		DLGPROC pfnDlgProc;
		COptions *pOptions;
		COLORREF crTitleColor;
	};
	static const PageInfo m_PageList[NUM_PAGES];
	int m_CurrentPage;
	int m_StartPage;
	HWND m_hDlg;
	HWND m_hDlgList[NUM_PAGES];
	HFONT m_hfontTitle;
	DWORD m_UpdateFlags;
	void CreatePage(int Page);
	static COptionDialog *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),					MAKEINTRESOURCE(IDD_OPTIONS_GENERAL),
		CGeneralOptions::DlgProc,		&GeneralOptions,	RGB(128,0,0)},
	{TEXT("表示"),					MAKEINTRESOURCE(IDD_OPTIONS_VIEW),
		CViewOptions::DlgProc,			&ViewOptions,		RGB(32,192,64)},
	{TEXT("OSD"),					MAKEINTRESOURCE(IDD_OPTIONS_OSD),
		COSDOptions::DlgProc,			&OSDOptions,		RGB(0,128,0)},
	{TEXT("ステータスバー"),		MAKEINTRESOURCE(IDD_OPTIONS_STATUS),
		CStatusOptions::DlgProc,		&StatusOptions,		RGB(0,0,128)},
	{TEXT("パネル"),				MAKEINTRESOURCE(IDD_OPTIONS_PANEL),
		CPanelOptions::DlgProc,			&PanelOptions,		RGB(0,255,128)},
	{TEXT("配色"),					MAKEINTRESOURCE(IDD_OPTIONS_COLORSCHEME),
		CColorSchemeOptions::DlgProc,	&ColorSchemeOptions,RGB(0,128,255)},
	{TEXT("操作"),					MAKEINTRESOURCE(IDD_OPTIONS_OPERATION),
		COperationOptions::DlgProc,		&OperationOptions,	RGB(128,128,0)},
	{TEXT("キー割り当て"),			MAKEINTRESOURCE(IDD_OPTIONS_ACCELERATOR),
		CAccelerator::DlgProc,			&Accelerator,		RGB(128,255,64)},
	{TEXT("HDUSリモコン"),			MAKEINTRESOURCE(IDD_OPTIONS_HDUSCONTROLLER),
		CHDUSController::DlgProc,		&HDUSController,	RGB(255,255,128)},
	{TEXT("ドライバ別設定"),		MAKEINTRESOURCE(IDD_OPTIONS_DRIVER),
		CDriverOptions::DlgProc,		&DriverOptions,		RGB(128,255,128)},
	{TEXT("音声"),					MAKEINTRESOURCE(IDD_OPTIONS_AUDIO),
		CAudioOptions::DlgProc,			&AudioOptions,		RGB(32,64,192)},
	{TEXT("録画"),					MAKEINTRESOURCE(IDD_OPTIONS_RECORD),
		CRecordOptions::DlgProc,		&RecordOptions,		RGB(128,0,160)},
	{TEXT("キャプチャ"),			MAKEINTRESOURCE(IDD_OPTIONS_CAPTURE),
		CCaptureOptions::DlgProc,		&CaptureOptions,	RGB(0,128,128)},
	{TEXT("チャンネルスキャン"),	MAKEINTRESOURCE(IDD_OPTIONS_CHANNELSCAN),
		CChannelScan::DlgProc,			&ChannelScan,		RGB(0,160,255)},
	{TEXT("EPG"),					MAKEINTRESOURCE(IDD_OPTIONS_EPG),
		CEpgOptions::DlgProc,			&EpgOptions,		RGB(128,0,255)},
	{TEXT("EPG番組表"),				MAKEINTRESOURCE(IDD_OPTIONS_PROGRAMGUIDE),
		CProgramGuideOptions::DlgProc,	&ProgramGuideOptions,RGB(160,255,0)},
	{TEXT("プラグイン"),			MAKEINTRESOURCE(IDD_OPTIONS_PLUGIN),
		CPluginOptions::DlgProc,	&PluginOptions,			RGB(255,0,160)},
	{TEXT("ネットワークリモコン"),	MAKEINTRESOURCE(IDD_OPTIONS_NETWORKREMOCON),
		CNetworkRemoconOptions::DlgProc,&NetworkRemoconOptions,	RGB(255,128,0)},
	{TEXT("ログ"),					MAKEINTRESOURCE(IDD_OPTIONS_LOG),
		CLogger::DlgProc,				&Logger,			RGB(255,0,128)},
};


COptionDialog::COptionDialog()
{
	m_CurrentPage=0;
	m_hfontTitle=NULL;
}


COptionDialog::~COptionDialog()
{
	if (m_hfontTitle)
		::DeleteObject(m_hfontTitle);
}


void COptionDialog::CreatePage(int Page)
{
	if (m_hDlgList[Page]==NULL) {
		RECT rc;
		POINT pt;

		m_hDlgList[Page]=::CreateDialogParam(hInst,
			m_PageList[Page].pszTemplate,m_hDlg,m_PageList[Page].pfnDlgProc,
			reinterpret_cast<LPARAM>(m_PageList[Page].pOptions));
		::GetWindowRect(::GetDlgItem(m_hDlg,IDC_OPTIONS_PAGEPLACE),&rc);
		pt.x=rc.left;
		pt.y=rc.top;
		::ScreenToClient(m_hDlg,&pt);
		::SetWindowPos(m_hDlgList[Page],NULL,pt.x,pt.y,0,0,
													SWP_NOZORDER | SWP_NOSIZE);
	}
}


COptionDialog *COptionDialog::GetThis(HWND hDlg)
{
	return static_cast<COptionDialog*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK COptionDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COptionDialog *pThis=reinterpret_cast<COptionDialog*>(lParam);
			int i;
			RECT rc;
			POINT pt;

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;
			for (i=0;i<NUM_PAGES;i++) {
				pThis->m_hDlgList[i]=NULL;
				::SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_ADDSTRING,0,
					reinterpret_cast<LPARAM>(pThis->m_PageList[i].pszTitle));
			}
			if (pThis->m_StartPage>=0 && pThis->m_StartPage<NUM_PAGES)
				pThis->m_CurrentPage=pThis->m_StartPage;
			pThis->CreatePage(pThis->m_CurrentPage);
			::ShowWindow(pThis->m_hDlgList[pThis->m_CurrentPage],SW_SHOW);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_SETCURSEL,pThis->m_CurrentPage,0);
			if (pThis->m_hfontTitle==NULL) {
				HFONT hfont;
				LOGFONT lf;

				hfont=(HFONT)::SendMessage(hDlg,WM_GETFONT,0,0);
				::GetObject(hfont,sizeof(LOGFONT),&lf);
				lf.lfWeight=FW_BOLD;
				pThis->m_hfontTitle=::CreateFontIndirect(&lf);
			}
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			COptionDialog *pThis=GetThis(hDlg);
			LPDRAWITEMSTRUCT pdis=(LPDRAWITEMSTRUCT)lParam;
			HFONT hfontOld;
			COLORREF crOldText;
			int OldBkMode;
			RECT rc;

			DrawUtil::FillGradient(pdis->hDC,&pdis->rcItem,
				RGB(0,0,0),pThis->m_PageList[pThis->m_CurrentPage].crTitleColor);
			hfontOld=SelectFont(pdis->hDC,pThis->m_hfontTitle);
			crOldText=::SetTextColor(pdis->hDC,RGB(255,255,255));
			OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
			rc.left=pdis->rcItem.left+2;
			rc.top=pdis->rcItem.top;
			rc.right=pdis->rcItem.right-2;
			rc.bottom=pdis->rcItem.bottom;
			::DrawText(pdis->hDC,pThis->m_PageList[pThis->m_CurrentPage].pszTitle,-1,
					&rc,DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			SelectFont(pdis->hDC,hfontOld);
			::SetTextColor(pdis->hDC,crOldText);
			::SetBkMode(pdis->hDC,OldBkMode);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_LIST:
			if (HIWORD(wParam)==LBN_SELCHANGE) {
				COptionDialog *pThis=GetThis(hDlg);
				int NewPage=SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_GETCURSEL,0,0);

				if (pThis->m_hDlgList[NewPage]==NULL) {
					HCURSOR hcurOld;

					hcurOld=::SetCursor(LoadCursor(NULL,IDC_WAIT));
					pThis->CreatePage(NewPage);
					::SetCursor(hcurOld);
				}
				::ShowWindow(pThis->m_hDlgList[pThis->m_CurrentPage],SW_HIDE);
				::ShowWindow(pThis->m_hDlgList[NewPage],SW_SHOW);
				pThis->m_CurrentPage=NewPage;
				::InvalidateRect(::GetDlgItem(hDlg,IDC_OPTIONS_TITLE),NULL,TRUE);
			}
			return TRUE;

		case IDC_OPTIONS_HELP:
			{
				COptionDialog *pThis=GetThis(hDlg);

				HtmlHelpClass.ShowContent(HELP_ID_OPTIONS_FIRST+pThis->m_CurrentPage);
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			{
				COptionDialog *pThis=GetThis(hDlg);
				HCURSOR hcurOld;
				NMHDR nmh;
				int i;

				hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				nmh.code=LOWORD(wParam)==IDOK?PSN_APPLY:PSN_RESET;
				pThis->m_UpdateFlags=0;
				for (i=0;i<NUM_PAGES;i++) {
					if (pThis->m_hDlgList[i]!=NULL) {
						::SendMessage(pThis->m_hDlgList[i],WM_NOTIFY,0,(LPARAM)&nmh);
						pThis->m_UpdateFlags|=m_PageList[i].pOptions->GetUpdateFlags();
					}
				}
				::SetCursor(hcurOld);
				::EndDialog(hDlg,LOWORD(wParam));
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool COptionDialog::ShowDialog(HWND hwndOwner,int StartPage)
{
	m_StartPage=StartPage;
	if (::DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_OPTIONS),hwndOwner,
						 DlgProc,reinterpret_cast<LPARAM>(this))!=IDOK) {
		if (m_UpdateFlags&COptions::UPDATE_PREVIEW) {
			//CoreEngine.m_DtvEngine.SetChannel(0,0);
			if (MainWindow.IsPreview())
				CoreEngine.EnablePreview(true);
		}
		return false;
	}
	MainWindow.Update();
	AppMain.SaveSettings();
	if (m_UpdateFlags&COptions::UPDATE_DRIVER) {
		if (AppMain.SetDriver(szDriverFileName))
			m_UpdateFlags|=COptions::UPDATE_PREVIEW;
	}
	if (m_UpdateFlags&COptions::UPDATE_CHANNELLIST) {
		AppMain.UpdateChannelList(ChannelScan.GetTuningSpaceList());
		//MainWindow.PostCommand(CM_CHANNEL_FIRST);
	} else if (m_UpdateFlags&COptions::UPDATE_NETWORKREMOCON) {
		//NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
		//										 &CoreEngine,&ChannelManager);
		AppMain.InitializeChannel();
		if (pNetworkRemocon!=NULL)
			pNetworkRemocon->GetChannel(&GetChannelReciver);
	}
	if (m_UpdateFlags&COptions::UPDATE_PREVIEW) {
		if (MainWindow.IsPreview())
			CoreEngine.EnablePreview(true);
	}
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	return true;
}


static COptionDialog OptionDialog;




class CPanelStatus : public CPanelFrameEventHandler {
	POINT m_ptDragStartCursorPos;
	POINT m_ptStartPos;
	enum {
		EDGE_NONE,
		EDGE_LEFT,
		EDGE_RIGHT,
		EDGE_TOP,
		EDGE_BOTTOM
	} m_SnapEdge;
	int m_AttachOffset;
public:
	CPanelStatus();
	// CPanelFrameEventHandler
	bool OnClose();
	bool OnMoving(RECT *pRect);
	bool OnEnterSizeMove();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
	bool OnMouseWheel(WPARAM wParam,LPARAM lParam);
	void OnVisibleChange(bool fVisible);
	bool OnFloatingChange(bool fFloating);
	// CPanelStatus
	bool OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect);
	bool IsAttached();
};

CPanelStatus::CPanelStatus()
{
	m_SnapEdge=EDGE_NONE;
}

bool CPanelStatus::OnClose()
{
	MainWindow.SendCommand(CM_INFORMATION);
	return false;
}

bool CPanelStatus::OnMoving(RECT *pRect)
{
	if (!PanelFrame.GetFloating())
		return false;
	POINT pt;
	RECT rc;

	GetCursorPos(&pt);
	pt.x=m_ptStartPos.x+(pt.x-m_ptDragStartCursorPos.x);
	pt.y=m_ptStartPos.y+(pt.y-m_ptDragStartCursorPos.y);
	OffsetRect(pRect,pt.x-pRect->left,pt.y-pRect->top);
	if (PanelOptions.GetSnapAtMainWindow()) {
		int SnapMargin=PanelOptions.GetSnapMargin();
		int XOffset,YOffset;
		bool fSnap;

		GetWindowRect(MainWindow.GetHandle(),&rc);
		XOffset=YOffset=0;
		fSnap=false;
		if (pRect->top<rc.bottom && pRect->bottom>rc.top) {
			if (pRect->right>=rc.left-SnapMargin && pRect->right<=rc.left+SnapMargin) {
				XOffset=rc.left-pRect->right;
				fSnap=true;
			} else if (pRect->left>=rc.right-SnapMargin && pRect->left<=rc.right+SnapMargin) {
				XOffset=rc.right-pRect->left;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->top>=rc.top-SnapMargin && pRect->top<=rc.top+SnapMargin) {
					YOffset=rc.top-pRect->top;
				} else if (pRect->bottom>=rc.bottom-SnapMargin && pRect->bottom<=rc.bottom+SnapMargin) {
					YOffset=rc.bottom-pRect->bottom;
				}
			}
		}
		if (!fSnap && pRect->left<rc.right && pRect->right>rc.left) {
			if (pRect->bottom>=rc.top-SnapMargin && pRect->bottom<=rc.top+SnapMargin) {
				YOffset=rc.top-pRect->bottom;
				fSnap=true;
			} else if (pRect->top>=rc.bottom-SnapMargin && pRect->top<=rc.bottom+SnapMargin) {
				YOffset=rc.bottom-pRect->top;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->left>=rc.left-SnapMargin && pRect->left<=rc.left+SnapMargin) {
					XOffset=rc.left-pRect->left;
				} else if (pRect->right>=rc.right-SnapMargin && pRect->right<=rc.right+SnapMargin) {
					XOffset=rc.right-pRect->right;
				}
			}
		}
		OffsetRect(pRect,XOffset,YOffset);
	}
	return true;
}

bool CPanelStatus::OnEnterSizeMove()
{
	if (!PanelFrame.GetFloating())
		return false;
	int x,y;

	GetCursorPos(&m_ptDragStartCursorPos);
	PanelFrame.GetPosition(&x,&y,NULL,NULL);
	m_ptStartPos.x=x;
	m_ptStartPos.y=y;
	return true;
}

bool CPanelStatus::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}

bool CPanelStatus::OnMouseWheel(WPARAM wParam,LPARAM lParam)
{
	SendMessage(MainWindow.GetVideoHostWindow(),WM_MOUSEWHEEL,wParam,lParam);
	return true;
}

void CPanelStatus::OnVisibleChange(bool fVisible)
{
	if (!PanelFrame.GetFloating())
		return;
	if (!fVisible) {
		m_SnapEdge=EDGE_NONE;
		if (PanelOptions.GetAttachToMainWindow()) {
			RECT rcPanel,rcMain;

			PanelFrame.GetPosition(&rcPanel);
			MainWindow.GetPosition(&rcMain);
			if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
				if (rcPanel.right==rcMain.left)
					m_SnapEdge=EDGE_LEFT;
				else if (rcPanel.left==rcMain.right)
					m_SnapEdge=EDGE_RIGHT;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.top-rcMain.top;
			}
			if (rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
				if (rcPanel.bottom==rcMain.top)
					m_SnapEdge=EDGE_TOP;
				else if (rcPanel.top==rcMain.bottom)
					m_SnapEdge=EDGE_BOTTOM;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.left-rcMain.left;
			}
		}
	} else {
		if (m_SnapEdge!=EDGE_NONE) {
			RECT rcPanel,rcMain;
			int x,y;

			PanelFrame.GetPosition(&rcPanel);
			OffsetRect(&rcPanel,-rcPanel.left,-rcPanel.top);
			MainWindow.GetPosition(&rcMain);
			switch (m_SnapEdge) {
			case EDGE_LEFT:
				x=rcMain.left-rcPanel.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_RIGHT:
				x=rcMain.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_TOP:
				y=rcMain.top-rcPanel.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			case EDGE_BOTTOM:
				y=rcMain.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			}
			PanelFrame.SetPosition(x,y,rcPanel.right,rcPanel.bottom);
			PanelFrame.MoveToMonitorInside();
		}
	}
}

bool CPanelStatus::OnFloatingChange(bool fFloating)
{
	int Size;
	RECT rc;

	PanelPaneIndex=Splitter.IDToIndex(PANE_ID_PANEL);
	MainWindow.GetPosition(&rc);
	Size=PanelFrame.GetDockingWidth()+Splitter.GetBarWidth();
	if (!fFloating || rc.right-rc.left>Size) {
		if (PanelPaneIndex==0) {
			if (fFloating)
				rc.left+=Size;
			else
				rc.left-=Size;
		} else {
			if (fFloating)
				rc.right-=Size;
			else
				rc.right+=Size;
		}
		MainWindow.SetPosition(&rc);
	}
	return true;
}

bool CPanelStatus::OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect)
{
	if (fShowPanelWindow && PanelOptions.GetAttachToMainWindow()
			&& PanelFrame.GetFloating()) {
		RECT rc;
		int XOffset,YOffset;
		bool fAttached=false;

		PanelFrame.GetPosition(&rc);
		XOffset=YOffset=0;
		if (rc.top<pOldRect->bottom && rc.bottom>pOldRect->top) {
			if (rc.right==pOldRect->left) {
				XOffset=pNewRect->left-rc.right;
				fAttached=true;
			} else if (rc.left==pOldRect->right) {
				XOffset=pNewRect->right-rc.left;
				fAttached=true;
			}
			if (fAttached)
				YOffset=pNewRect->top-pOldRect->top;
		}
		if (!fAttached && rc.left<pOldRect->right && rc.right>pOldRect->left) {
			if (rc.bottom==pOldRect->top) {
				YOffset=pNewRect->top-rc.bottom;
				fAttached=true;
			} else if (rc.top==pOldRect->bottom) {
				YOffset=pNewRect->bottom-rc.top;
				fAttached=true;
			}
			if (fAttached)
				XOffset=pNewRect->left-pOldRect->left;
		}
		if (XOffset!=0 || YOffset!=0) {
			OffsetRect(&rc,XOffset,YOffset);
			PanelFrame.SetPosition(&rc);
			PanelFrame.MoveToMonitorInside();
		}
		return true;
	}
	return false;
}

bool CPanelStatus::IsAttached()
{
	bool fAttached=false;

	if (fShowPanelWindow && PanelOptions.GetAttachToMainWindow()
			&& PanelFrame.GetFloating()) {
		RECT rcPanel,rcMain;

		PanelFrame.GetPosition(&rcPanel);
		MainWindow.GetPosition(&rcMain);
		if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
			if (rcPanel.right==rcMain.left || rcPanel.left==rcMain.right)
				fAttached=true;
		}
		if (!fAttached && rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
			if (rcPanel.bottom==rcMain.top || rcPanel.top==rcMain.bottom)
				fAttached=true;
		}
	}
	return fAttached;
}


class CMyInfoPanelEventHandler : public CInfoPanelEventHandler {
public:
	void OnSelChange();
	void OnRButtonDown();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
};

void CMyInfoPanelEventHandler::OnSelChange()
{
	if (InfoPanel.GetCurTab()==PANEL_TAB_PROGRAMLIST
			&& ProgramListUpdateTimerCount>0) {
		WORD ServiceID=ChannelManager.GetCurrentServiceID();

		if (ServiceID==0)
			CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,
										CoreEngine.m_DtvEngine.GetService());
		if (ServiceID!=0)
			ProgramListView.UpdateProgramList(CoreEngine.m_DtvEngine.m_ProgManager.GetTransportStreamID(),ServiceID);
	} else if (InfoPanel.GetCurTab()==PANEL_TAB_CHANNEL) {
		::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
		::SetCursor(::LoadCursor(NULL,IDC_ARROW));
	}
}

void CMyInfoPanelEventHandler::OnRButtonDown()
{
	MainWindow.PopupMenu();
}

bool CMyInfoPanelEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


class CMyChannelPanelEventHandler : public CChannelPanel::CEventHandler {
public:
	void OnChannelClick(const CChannelInfo *pChannelInfo);
};

void CMyChannelPanelEventHandler::OnChannelClick(const CChannelInfo *pChannelInfo)
{
	const CChannelList *pList=ChannelManager.GetCurrentChannelList();

	if (pList!=NULL) {
		if (pNetworkRemocon!=NULL) {
			MainWindow.PostCommand(CM_CHANNELNO_FIRST+pChannelInfo->GetChannelNo()-1);
		} else {
			int Index=pList->Find(pChannelInfo->GetSpace(),
								  pChannelInfo->GetChannelIndex(),
								  pChannelInfo->GetService());
			if (Index>=0)
				AppMain.SetChannel(ChannelManager.GetCurrentSpace(),Index);
		}
	}
}


static CPanelStatus PanelStatus;
static CMyInfoPanelEventHandler InfoPanelEventHandler;
static CMyChannelPanelEventHandler ChannelPanelEventHandler;




class CMyProgramGuideEventHandler : public CProgramGuideEventHandler {
public:
	bool OnClose();
	void OnServiceTitleLButtonDown(WORD ServiceID);
	bool OnBeginUpdate();
	void OnEndUpdate();
	bool OnRefresh();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
};

bool CMyProgramGuideEventHandler::OnClose()
{
	fShowProgramGuide=false;
	MainMenu.CheckItem(CM_PROGRAMGUIDE,false);
	return true;
}

void CMyProgramGuideEventHandler::OnServiceTitleLButtonDown(WORD ServiceID)
{
	const CChannelList *pList=ChannelManager.GetCurrentChannelList();

	if (pList!=NULL) {
		int Index=pList->FindServiceID(ServiceID);

		if (Index>=0) {
			const CChannelInfo *pChInfo=pList->GetChannelInfo(Index);

			if (pChInfo!=NULL)
				MainWindow.PostCommand(CM_CHANNELNO_FIRST+pChInfo->GetChannelNo()-1);
		}
	}
}

bool CMyProgramGuideEventHandler::OnBeginUpdate()
{
	if (CoreEngine.IsNetworkDriver()) {
		MainWindow.ShowMessage(TEXT("UDP/TCPでは番組表の取得はできません。"),
							   TEXT("ごめん"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	if (RecordManager.IsRecording()) {
		MainWindow.ShowMessage(
					 TEXT("録画を停止させてから番組表を取得してください。"),
					 TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	return MainWindow.BeginProgramGuideUpdate();
}

void CMyProgramGuideEventHandler::OnEndUpdate()
{
	MainWindow.EndProgramGuideUpdate();
}

bool CMyProgramGuideEventHandler::OnRefresh()
{
	const CChannelList *pList;

	if (!CoreEngine.IsNetworkDriver())
		pList=ChannelManager.GetCurrentChannelList();
	else
		pList=ChannelManager.GetFileAllChannelList();
	if (pList!=NULL) {
		HCURSOR hcurOld;

		hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		EpgProgramList.UpdateProgramList();
		m_pProgramGuide->SetChannelList(pList);
		::SetCursor(hcurOld);
	}
	return true;
}

bool CMyProgramGuideEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


static CMyProgramGuideEventHandler ProgramGuideEventHandler;




bool CFullscreen::Initialize()
{
	WNDCLASS wc;

	wc.style=CS_DBLCLKS;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=NULL;
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=CreateSolidBrush(RGB(0,0,0));
	wc.lpszMenuName=NULL;
	wc.lpszClassName=FULLSCREEN_WINDOW_CLASS;
	return RegisterClass(&wc)!=0;
}


CFullscreen *CFullscreen::GetThis(HWND hwnd)
{
	return static_cast<CFullscreen*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CFullscreen::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CFullscreen *pThis=static_cast<CFullscreen*>(OnCreate(hwnd,lParam));

			pThis->m_pVideoContainer->SetParent(pThis);
			RECT rc;
			pThis->GetClientRect(&rc);
			pThis->m_pVideoContainer->SetPosition(&rc);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
									ViewOptions.GetFullscreenStretchMode());
			pThis->m_fShowCursor=true;
			pThis->m_fMenu=false;
			pThis->m_fShowStatusView=false;
			pThis->m_fShowTitleBar=false;
			::SetTimer(hwnd,1,1000,NULL);
		}
		return 0;

	case WM_SIZE:
		GetThis(hwnd)->m_pVideoContainer->SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnRButtonDown();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		MainWindow.PostCommand(CM_FULLSCREEN);
		return 0;

	case WM_MOUSEMOVE:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnMouseMove();
		}
		return 0;

	case WM_TIMER:
		{
			CFullscreen *pThis=GetThis(hwnd);

			if (!pThis->m_fMenu) {
				::SetCursor(NULL);
				CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(true);
				pThis->m_fShowCursor=false;
			}
			::KillTimer(hwnd,1);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CFullscreen *pThis=GetThis(hwnd);

			::SetCursor(pThis->m_fShowCursor?LoadCursor(NULL,IDC_ARROW):NULL);
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
		{
			CFullscreen *pThis=GetThis(hwnd);

			MainWindow.OnMouseWheel(wParam,lParam,pThis->m_fShowStatusView);
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp=reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter=HWND_TOPMOST;
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			MainWindow.SendCommand(CM_FULLSCREEN);
			return 0;
		}
	case WM_COMMAND:
		return MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_MONITORPOWER:
			if (fNoMonitorLowPower)
				return TRUE;
			break;
		}
		break;

	case WM_DESTROY:
		{
			CFullscreen *pThis=GetThis(hwnd);
			SIZE sz;

			pThis->m_pViewWindow->GetClientSize(&sz);
			pThis->m_pVideoContainer->SetParent(pThis->m_pViewWindow);
			pThis->m_pViewWindow->SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			//if (!m_fShowCursor)
				CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO);
			pThis->ShowStatusView(false);
			pThis->ShowTitleBar(false);
			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CFullscreen::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 FULLSCREEN_WINDOW_CLASS,NULL,hInst);
}


bool CFullscreen::Create(HWND hwndOwner,CVideoContainerWindow *pVideoContainer,CViewWindow *pViewWindow)
{
	HMONITOR hMonitor;
	int x,y,Width,Height;

	hMonitor=::MonitorFromWindow(MainWindow.GetHandle(),MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		x=mi.rcMonitor.left;
		y=mi.rcMonitor.top;
		Width=mi.rcMonitor.right-mi.rcMonitor.left;
		Height=mi.rcMonitor.bottom-mi.rcMonitor.top;
	} else {
		x=y=0;
		Width=::GetSystemMetrics(SM_CXSCREEN);
		Height=::GetSystemMetrics(SM_CYSCREEN);
	}
	SetPosition(x,y,Width,Height);
	m_pVideoContainer=pVideoContainer;
	m_pViewWindow=pViewWindow;
	return Create(hwndOwner,WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


CFullscreen::CFullscreen()
	: m_pVideoContainer(NULL)
	, m_pViewWindow(NULL)
{
}


CFullscreen::~CFullscreen()
{
	Destroy();
}


void CFullscreen::OnRButtonDown()
{
	// メニュー表示中はカーソルを消さない
	KillTimer(m_hwnd,1);
	m_fShowCursor=true;
	CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
	::SetCursor(LoadCursor(NULL,IDC_ARROW));
	m_fMenu=true;
	MainWindow.PopupMenu();
	m_fMenu=false;
	::SetTimer(m_hwnd,1,1000,NULL);
}


void CFullscreen::OnMouseMove()
{
	POINT pt;
	RECT rcClient,rc;

	if (m_fMenu)
		return;
	if (!m_fShowCursor) {
		::SetCursor(LoadCursor(NULL,IDC_ARROW));
		CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
		m_fShowCursor=true;
	}
	GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	GetClientRect(&rcClient);
	rc=rcClient;
	rc.top=rc.bottom-StatusView.GetHeight();
	if (::PtInRect(&rc,pt)) {
		if (!m_fShowStatusView)
			ShowStatusView(true);
		::KillTimer(m_hwnd,1);
	} else {
		if (m_fShowStatusView)
			ShowStatusView(false);
		rc.top=rcClient.top;
		rc.bottom=rc.top+TitleBar.GetHeight();
		if (::PtInRect(&rc,pt)) {
			if (!m_fShowTitleBar)
				ShowTitleBar(true);
			::KillTimer(m_hwnd,1);
		} else {
			if (m_fShowTitleBar)
				ShowTitleBar(false);
			::SetTimer(m_hwnd,1,1000,NULL);
		}
	}
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;
	if (fShow) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetVisible(false);
		StatusView.SetParent(m_pVideoContainer);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
	} else {
		StatusView.SetVisible(false);
		StatusView.SetParent(&MainWindow);
		if (MainWindow.GetStatusBarVisible()) {
			SIZE sz;

			MainWindow.GetClientSize(&sz);
			MainWindow.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			StatusView.SetVisible(true);
		}
	}
	m_fShowStatusView=fShow;
}


void CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow==m_fShowTitleBar)
		return;
	if (fShow) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=rc.top+TitleBar.GetHeight();
		TitleBar.SetVisible(false);
		TitleBar.SetParent(m_pVideoContainer);
		TitleBar.SetPosition(&rc);
		TitleBar.SetVisible(true);
	} else {
		TitleBar.SetVisible(false);
		TitleBar.SetParent(&MainWindow);
	}
	m_fShowTitleBar=fShow;
}


class CServiceUpdateInfo {
public:
	struct ServiceInfo {
		WORD ServiceID;
		TCHAR szServiceName[256];
	};
	ServiceInfo *m_pServiceList;
	int m_NumServices;
	int m_CurService;
	WORD m_NetworkID;
	WORD m_TransportStreamID;
	CServiceUpdateInfo(CDtvEngine *pEngine,CProgManager *pProgManager);
	~CServiceUpdateInfo();
};

CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CProgManager *pProgManager)
{
	m_NumServices=pProgManager->GetServiceNum();
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			if (!pProgManager->GetServiceID(&m_pServiceList[i].ServiceID,i))
				m_pServiceList[i].ServiceID=0;
			if (!pProgManager->GetServiceName(m_pServiceList[i].szServiceName,i))
				m_pServiceList[i].szServiceName[0]='\0';
		}
	} else {
		m_pServiceList=NULL;
	}
	m_CurService=pEngine->GetService();
	if (m_CurService>=m_NumServices)
		m_CurService=-1;
	m_NetworkID=pProgManager->GetNetworkID();
	m_TransportStreamID=pProgManager->GetTransportStreamID();
}

CServiceUpdateInfo::~CServiceUpdateInfo()
{
	delete [] m_pServiceList;
}


class MyDtvEngineHandler : public CDtvEngineHandler
{
	const DWORD OnDtvEngineEvent(CDtvEngine *pEngine,const DWORD dwEventID,
																PVOID pParam);
};

const DWORD MyDtvEngineHandler::OnDtvEngineEvent(CDtvEngine *pEngine,
											const DWORD dwEventID,PVOID pParam)
{
	switch (dwEventID) {
	case CDtvEngine::EID_SERVICE_INFO_UPDATED:
	case CDtvEngine::EID_SERVICE_LIST_UPDATED:
		{
			CProgManager *pProgManager=static_cast<CProgManager*>(pParam);
			CServiceUpdateInfo *pInfo=new CServiceUpdateInfo(pEngine,pProgManager);

			MainWindow.PostMessage(WM_APP_SERVICEUPDATE,
								   dwEventID==CDtvEngine::EID_SERVICE_LIST_UPDATED,
								   reinterpret_cast<LPARAM>(pInfo));
		}
		break;
	case CDtvEngine::EID_FILE_WRITE_ERROR:
		MainWindow.PostMessage(WM_APP_FILEWRITEERROR,0,0);
		break;
	}
	return 0;
}


static MyDtvEngineHandler DtvEngineHandler;




class CMyStatusViewEventHandler : public CStatusViewEventHandler {
public:
	void OnMouseLeave();
};

void CMyStatusViewEventHandler::OnMouseLeave()
{
	if (!MainWindow.GetFullscreen() && !MainWindow.GetStatusBarVisible())
		m_pStatusView->SetVisible(false);
}


class CMyTitleBarEventHandler : public CTitleBarEventHandler {
	bool m_fFixed;
public:
	CMyTitleBarEventHandler();
	bool OnClose();
	bool OnMinimize();
	bool OnMaximize();
	bool OnFullscreen();
	void OnMouseLeave();
	void OnLabelLButtonDown(int x,int y);
	void OnLabelRButtonDown(int x,int y);
	void EndDrag();
};

CMyTitleBarEventHandler::CMyTitleBarEventHandler()
{
	m_fFixed=false;
}

bool CMyTitleBarEventHandler::OnClose()
{
	MainWindow.PostCommand(CM_CLOSE);
	return true;
}

bool CMyTitleBarEventHandler::OnMinimize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,SC_MINIMIZE,0);
	return true;
}

bool CMyTitleBarEventHandler::OnMaximize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,
						   MainWindow.GetMaximize()?SC_RESTORE:SC_MAXIMIZE,0);
	return true;
}

bool CMyTitleBarEventHandler::OnFullscreen()
{
	MainWindow.SetFullscreen(!MainWindow.GetFullscreen());
	return true;
}

void CMyTitleBarEventHandler::OnMouseLeave()
{
	if (!m_fFixed)
		m_pTitleBar->SetVisible(false);
}

void CMyTitleBarEventHandler::OnLabelLButtonDown(int x,int y)
{
	if (m_pTitleBar->GetParent()==MainWindow.GetHandle()) {
		POINT pt;

		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
		MainWindow.SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
		m_fFixed=true;
	}
}

void CMyTitleBarEventHandler::OnLabelRButtonDown(int x,int y)
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
	m_fFixed=true;
	MainWindow.SendMessage(0x0313,0,MAKELPARAM(pt.x,pt.y));
	m_fFixed=false;
}

void CMyTitleBarEventHandler::EndDrag()
{
	m_fFixed=false;
}


static CMyStatusViewEventHandler StatusViewEventHandler;
static CMyTitleBarEventHandler TitleBarEventHandler;




class CMyCaptureWindowEvent : public CCaptureWindow::CEventHandler {
public:
	bool OnClose();
	bool OnSave(CCaptureImage *pImage);
	bool OnKeyDown(UINT KeyCode,UINT Flags);
};

bool CMyCaptureWindowEvent::OnClose()
{
	fShowCaptureWindow=false;
	MainMenu.CheckItem(CM_CAPTUREPREVIEW,false);
	m_pCaptureWindow->ClearImage();
	return true;
}

bool CMyCaptureWindowEvent::OnSave(CCaptureImage *pImage)
{
	return CaptureOptions.SaveImage(pImage);
}

bool CMyCaptureWindowEvent::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


static CMyCaptureWindowEvent CaptureWindowEventHandler;




bool CMainWindow::Initialize()
{
	WNDCLASS wc;

	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=MAIN_WINDOW_CLASS;
	return RegisterClass(&wc)!=0 && CFullscreen::Initialize();
}


CMainWindow::CMainWindow()
{
	// 適当にデフォルト位置を設定
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
	m_WindowPosition.Left=(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
	m_fFullscreen=false;
	m_pFullscreen=NULL;
	m_fMaximize=false;
	m_fAlwaysOnTop=false;
	m_fShowStatusBar=true;
	m_fShowTitleBar=true;
	m_fEnablePreview=false;
	m_fStandby=false;
	m_fStandbyInit=false;
	m_fMinimizeInit=false;
	m_fSrcFilterReleased=false;
	m_fProgramGuideUpdating=false;
	m_fExitOnRecordingStop=false;
	m_fClosing=false;
}


bool CMainWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_fAlwaysOnTop)
		ExStyle|=WS_EX_TOPMOST;
	if (!CreateBasicWindow(NULL,Style,ExStyle,ID,MAIN_WINDOW_CLASS,TITLE_TEXT,hInst))
		return false;
	return true;
}


bool CMainWindow::Show(int CmdShow)
{
	return ::ShowWindow(m_hwnd,m_fMaximize?SW_SHOWMAXIMIZED:CmdShow)!=FALSE;
}


bool CMainWindow::BuildMediaViewer()
{
	TCHAR szText[1024];

	if (!CoreEngine.BuildMediaViewer(m_VideoContainer.GetHandle(),m_hwnd,
									 VideoRendererType,szMpeg2DecoderName,
									 AudioOptions.GetAudioDeviceName())) {
		Logger.AddLog(CoreEngine.GetLastErrorText());
		if (!CmdLineParser.m_fSilent) {
			ShowErrorMessage(&CoreEngine,TEXT("DirectShowの初期化ができません。"));
		}
		InfoWindow.SetDecoderName(NULL);
		return false;
	}
	if (CoreEngine.m_DtvEngine.GetVideoDecoderName(szText,lengthof(szText)))
		InfoWindow.SetDecoderName(szText);
	return true;
}


bool CMainWindow::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (fFullscreen) {
			m_pFullscreen=new CFullscreen;
			if (!m_pFullscreen->Create(m_hwnd,&m_VideoContainer,&m_ViewWindow)) {
				delete m_pFullscreen;
				m_pFullscreen=NULL;
				return false;
			}
		} else {
			ForegroundWindow(m_hwnd);
			SAFE_DELETE(m_pFullscreen);
		}
		m_fFullscreen=fFullscreen;
		MainMenu.CheckItem(CM_FULLSCREEN,fFullscreen);
		PluginList.SendFullscreenChangeEvent(fFullscreen);
	}
	return true;
}


HWND CMainWindow::GetVideoHostWindow() const
{
	if (m_fStandby)
		return NULL;
	if (m_fFullscreen)
		return m_pFullscreen->GetHandle();
	return m_hwnd;
}


int CMainWindow::ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption,UINT Type) const
{
	return ::MessageBox(GetVideoHostWindow(),pszText,pszCaption,Type);
}


void CMainWindow::ShowErrorMessage(LPCTSTR pszText)
{
	ErrorDialog.Show(GetVideoHostWindow(),CErrorDialog::TYPE_WARNING,pszText);
}


void CMainWindow::ShowErrorMessage(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	TCHAR szText[1024];

	pErrorHandler->FormatLastErrorText(szText,lengthof(szText));
	ErrorDialog.Show(GetVideoHostWindow(),CErrorDialog::TYPE_WARNING,szText,
					 pszTitle,pErrorHandler->GetLastErrorSystemMessage());
}


void CMainWindow::AdjustWindowSize(int Width,int Height)
{
	RECT rcOld,rc;

	if (IsZoomed(m_hwnd))
		return;
	GetPosition(&rcOld);
	Splitter.GetScreenPosition(&rc);
	rc.right=rc.left+Width;
	rc.bottom=rc.top+Height;
	if (ViewOptions.GetClientEdge()) {
		rc.right+=GetSystemMetrics(SM_CXEDGE)*2;
		rc.bottom+=GetSystemMetrics(SM_CYEDGE)*2;
	}
	if (m_fShowStatusBar)
		rc.bottom+=StatusView.GetHeight();
	if (fShowPanelWindow && !PanelFrame.GetFloating())
		rc.right+=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
	::AdjustWindowRectEx(&rc,GetStyle(),FALSE,GetExStyle());

	// ウィンドウがモニタの外に出ないようにする
	HMONITOR hMonitor=::MonitorFromRect(&rcOld,MONITOR_DEFAULTTONULL);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(mi);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			if (rcOld.left>=mi.rcWork.left && rcOld.top>=mi.rcWork.top
					&& rcOld.right<=mi.rcWork.right && rcOld.bottom<=mi.rcWork.bottom) {
				if (rc.right>mi.rcWork.right && rc.left>mi.rcWork.left)
					::OffsetRect(&rc,max(mi.rcWork.right-rc.right,mi.rcWork.left-rc.left),0);
				if (rc.bottom>mi.rcWork.bottom && rc.top>mi.rcWork.top)
					::OffsetRect(&rc,0,max(mi.rcWork.bottom-rc.bottom,mi.rcWork.top-rc.top));
			}
		}
	}

	SetPosition(&rc);
	PanelStatus.OnOwnerMovingOrSizing(&rcOld,&rc);
}


void CMainWindow::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		m_fAlwaysOnTop=fTop;
		if (m_hwnd!=NULL) {
			::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,
											0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			MainMenu.CheckItem(CM_ALWAYSONTOP,fTop);
		}
	}
}


void CMainWindow::SetStatusBarVisible(bool fVisible)
{
	if (m_fShowStatusBar!=fVisible) {
		if (!m_fFullscreen) {
			RECT rc;

			m_fShowStatusBar=fVisible;
			if (::IsZoomed(m_hwnd)) {
				StatusView.SetVisible(fVisible);
				GetClientRect(&rc);
				SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
			} else {
				GetPosition(&rc);
				StatusView.SetVisible(fVisible);
				if (fVisible) {
					rc.bottom+=StatusView.GetHeight();
				} else {
					rc.bottom-=StatusView.GetHeight();
				}
				SetPosition(&rc);
			}
			MainMenu.CheckItem(CM_STATUSBAR,fVisible);
		}
	}
}


void CMainWindow::SetTitleBarVisible(bool fVisible)
{
	if (m_fShowTitleBar!=fVisible) {
		m_fShowTitleBar=fVisible;
		if (m_hwnd!=NULL) {
			bool fMaximize=GetMaximize();
			RECT rc;

			if (!fMaximize)
				GetPosition(&rc);
			SetStyle(GetStyle()^WS_CAPTION,fMaximize);
			if (!fMaximize) {
				int CaptionHeight=::GetSystemMetrics(SM_CYCAPTION);

				if (fVisible)
					rc.top-=CaptionHeight;
				else
					rc.top+=CaptionHeight;
				::SetWindowPos(m_hwnd,NULL,rc.left,rc.top,
							rc.right-rc.left,rc.bottom-rc.top,
							SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			}
			::MainMenu.CheckItem(CM_TITLEBAR,fVisible);
		}
	}
}


bool CMainWindow::OnCreate()
{
	Splitter.Create(m_hwnd,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CSplitter::STYLE_VERT);
	m_ViewWindow.Create(Splitter.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,
		ViewOptions.GetClientEdge()?WS_EX_CLIENTEDGE:0,IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_VideoContainer.Create(m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN,0,IDC_VIDEOCONTAINER,&CoreEngine.m_DtvEngine);
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);
	Splitter.SetPane(!PanelPaneIndex,&m_ViewWindow,PANE_ID_VIEW);
	Splitter.SetMinSize(PANE_ID_VIEW,32);
	Splitter.SetPaneVisible(PANE_ID_VIEW,true);
	StatusView.Create(m_hwnd,
		//WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		/*WS_EX_CLIENTEDGE*/WS_EX_STATICEDGE,IDC_STATUS);
	StatusView.SetEventHandler(&StatusViewEventHandler);
	StatusView.AddItem(new CChannelStatusItem);
	StatusView.AddItem(new CVideoSizeStatusItem);
	StatusView.AddItem(new CVolumeStatusItem);
	StatusView.AddItem(new CAudioChannelStatusItem);
	StatusView.AddItem(new CRecordStatusItem);
	StatusView.AddItem(new CCaptureStatusItem);
	StatusView.AddItem(new CErrorStatusItem);
	StatusView.AddItem(new CSignalLevelStatusItem);
	StatusView.AddItem(new CClockStatusItem);
	StatusView.AddItem(new CProgramInfoStatusItem);
	StatusView.AddItem(new CBufferingStatusItem);
	StatusView.AddItem(new CTunerStatusItem);
	StatusOptions.ApplyOptions();
	if (!m_fShowStatusBar) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetPosition(&rc);
		::BringWindowToTop(StatusView.GetHandle());
	}
	StatusView.SetSingleText(TEXT("起動中..."));

	TitleBar.Create(m_hwnd,WS_CHILD | WS_CLIPSIBLINGS,WS_EX_STATICEDGE,IDC_TITLEBAR);
	TitleBar.SetEventHandler(&TitleBarEventHandler);

	NotificationBar.Create(m_VideoContainer.GetHandle(),WS_CHILD | WS_CLIPSIBLINGS);

	MainMenu.Create(hInst);
	MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
							CM_ASPECTRATIO_FIRST+AspectRatioType);
	MainMenu.CheckItem(CM_ALWAYSONTOP,m_fAlwaysOnTop);
	for (int i=0;i<lengthof(VolumeNormalizeLevelList);i++) {
		if (CoreEngine.GetVolumeNormalizeLevel()==VolumeNormalizeLevelList[i]) {
			MainMenu.CheckRadioItem(CM_VOLUMENORMALIZE_FIRST,
						CM_VOLUMENORMALIZE_LAST,CM_VOLUMENORMALIZE_FIRST+i);
			break;
		}
	}
	MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+CoreEngine.GetStereoMode());
	MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
							CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize());
	MainMenu.CheckItem(CM_CAPTUREPREVIEW,fShowCaptureWindow);
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	MainMenu.CheckItem(CM_INFORMATION,fShowPanelWindow);
	MainMenu.CheckItem(CM_STATUSBAR,m_fShowStatusBar);
	MainMenu.CheckItem(CM_TITLEBAR,m_fShowTitleBar);

	HMENU hSysMenu;
	hSysMenu=GetSystemMenu(m_hwnd,FALSE);
	AppendMenu(hSysMenu,MFT_SEPARATOR,0,NULL);
	AppendMenu(hSysMenu,MFT_STRING | MFS_ENABLED,SC_ABOUT,
												TEXT("バージョン情報(&A)..."));

	::SetTimer(m_hwnd,TIMER_ID_UPDATE,500,NULL);
	return true;
}


void CMainWindow::OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify)
{
	switch (id) {
	case CM_ZOOM_20:
	case CM_ZOOM_25:
	case CM_ZOOM_33:
	case CM_ZOOM_50:
	case CM_ZOOM_66:
	case CM_ZOOM_75:
	case CM_ZOOM_100:
	case CM_ZOOM_150:
	case CM_ZOOM_200:
		{
			WORD Width,Height;

			if (m_fFullscreen)
				SetFullscreen(false);
			if (IsZoomed(hwnd))
				::ShowWindow(hwnd,SW_RESTORE);
			SetZoomRate(ZoomRateList[id-CM_ZOOM_FIRST].Num,
						ZoomRateList[id-CM_ZOOM_FIRST].Denom);
		}
		return;

	case CM_ASPECTRATIO:
		SendCommand(CM_ASPECTRATIO_FIRST+
			(AspectRatioType+1)%(CM_ASPECTRATIO_LAST-CM_ASPECTRATIO_FIRST+1));
		return;

	case CM_ASPECTRATIO_DEFAULT:
	case CM_ASPECTRATIO_16x9:
	case CM_ASPECTRATIO_LETTERBOX:
	case CM_ASPECTRATIO_SUPERFRAME:
	case CM_ASPECTRATIO_SIDECUT:
	case CM_ASPECTRATIO_4x3:
		{
			static const struct {
				BYTE XAspect,YAspect;
				BYTE PanAndScan;
			} AspectRatioList[] = {
				{ 0,0,CMediaViewer::PANANDSCAN_HORZ_DEFAULT | CMediaViewer::PANANDSCAN_VERT_DEFAULT},
				{16,9,CMediaViewer::PANANDSCAN_HORZ_NONE | CMediaViewer::PANANDSCAN_VERT_NONE},
				{16,9,CMediaViewer::PANANDSCAN_HORZ_NONE | CMediaViewer::PANANDSCAN_VERT_CUT},
				{16,9,CMediaViewer::PANANDSCAN_HORZ_CUT | CMediaViewer::PANANDSCAN_VERT_CUT},
				{ 4,3,CMediaViewer::PANANDSCAN_HORZ_CUT | CMediaViewer::PANANDSCAN_VERT_NONE},
				{ 4,3,CMediaViewer::PANANDSCAN_HORZ_NONE | CMediaViewer::PANANDSCAN_VERT_NONE},
			};
			int i=id-CM_ASPECTRATIO_FIRST;

			CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(
				AspectRatioList[i].XAspect,AspectRatioList[i].YAspect,
				AspectRatioList[i].PanAndScan);
			if (!m_fFullscreen && !::IsZoomed(hwnd)) {
				if (!ViewOptions.GetPanScanNoResizeWindow()) {
					int ZoomNum,ZoomDenom;
					int Width,Height;

					if (CalcZoomRate(&ZoomNum,&ZoomDenom)
							&& CoreEngine.GetVideoViewSize(&Width,&Height)) {
						AdjustWindowSize(Width*ZoomNum/ZoomDenom,
										 Height*ZoomNum/ZoomDenom);
					} else {
						WORD w,h;

						if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&w,&h))
							AdjustWindowSize(w,h);
					}
				} else {
					SIZE sz;
					int Width,Height;

					m_VideoContainer.GetClientSize(&sz);
					if (CoreEngine.GetVideoViewSize(&Width,&Height))
						AdjustWindowSize(Width*sz.cy/Height,sz.cy);
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				}
			}
			AspectRatioType=i;
			MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
									CM_ASPECTRATIO_FIRST+AspectRatioType);
			CheckZoomMenu();
		}
		return;

	case CM_FULLSCREEN:
		SetFullscreen(!m_fFullscreen);
		return;

	case CM_ALWAYSONTOP:
		SetAlwaysOnTop(!m_fAlwaysOnTop);
		return;

	case CM_VOLUME_UP:
	case CM_VOLUME_DOWN:
		{
			int Volume;

			if (id==CM_VOLUME_UP) {
				Volume=CoreEngine.GetVolume()+VolumeStep;
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume=CoreEngine.GetVolume()-VolumeStep;
				if (Volume<0)
					Volume=0;
			}
			if (Volume!=CoreEngine.GetVolume() || CoreEngine.GetMute())
				SetVolume(Volume);
		}
		return;

	case CM_VOLUME_MUTE:
		SetMute(!GetMute());
		return;

	case CM_VOLUMENORMALIZE_NONE:
	case CM_VOLUMENORMALIZE_125:
	case CM_VOLUMENORMALIZE_150:
	case CM_VOLUMENORMALIZE_200:
		CoreEngine.SetVolumeNormalizeLevel(
						VolumeNormalizeLevelList[id-CM_VOLUMENORMALIZE_FIRST]);
		MainMenu.CheckRadioItem(CM_VOLUMENORMALIZE_NONE,
								CM_VOLUMENORMALIZE_LAST,id);
		return;

	case CM_STEREO_THROUGH:
	case CM_STEREO_LEFT:
	case CM_STEREO_RIGHT:
		SetStereoMode(id-CM_STEREO_THROUGH);
		return;

	case CM_SWITCHAUDIO:
		SwitchAudio();
		return;

	case CM_CAPTURE:
		SendCommand(CaptureOptions.TranslateCommand(CM_CAPTURE));
		return;

	case CM_COPY:
	case CM_SAVEIMAGE:
		{
			HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
			BYTE *pDib;

			pDib=static_cast<BYTE*>(CoreEngine.GetCurrentImage());
			if (pDib==NULL) {
				::SetCursor(hcurOld);
				ShowMessage(TEXT("現在の画像を取得できません。\n")
							TEXT("レンダラやデコーダを変えてみてください。"),TEXT("ごめん"),
							MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			{
				BITMAPINFOHEADER *pbmih=(BITMAPINFOHEADER*)pDib;
				RECT rc;
				int Left,Top,Width,Height,OrigWidth,OrigHeight;
				SIZE_T Size;
				HGLOBAL hGlobal=NULL;

				OrigWidth=pbmih->biWidth;
				OrigHeight=abs(pbmih->biHeight);
				if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rc)) {
					WORD VideoWidth,VideoHeight;

					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetOriginalVideoSize(
													&VideoWidth,&VideoHeight)
							&& (VideoWidth!=OrigWidth
								|| VideoHeight!=OrigHeight)) {
						rc.left=rc.left*OrigWidth/VideoWidth;
						rc.top=rc.top*OrigHeight/VideoHeight;
						rc.right=rc.right*OrigWidth/VideoWidth;
						rc.bottom=rc.bottom*OrigHeight/VideoHeight;
					}
					if (rc.right>OrigWidth)
						rc.right=OrigWidth;
					if (rc.bottom>OrigHeight)
						rc.bottom=OrigHeight;
				} else {
					rc.left=0;
					rc.top=0;
					rc.right=OrigWidth;
					rc.bottom=OrigHeight;
				}
				switch (CaptureOptions.GetCaptureSizeType()) {
				case CCaptureOptions::SIZE_TYPE_ORIGINAL:
					CoreEngine.GetVideoViewSize(&Width,&Height);
					break;
				case CCaptureOptions::SIZE_TYPE_VIEW:
					{
						WORD w,h;

						CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&w,&h);
						Width=w;
						Height=h;
					}
					break;
				/*
				case CCaptureOptions::SIZE_RAW:
					rc.left=rc.top=0;
					rc.right=OrigWidth;
					rc.bottom=OrigHeight;
					Width=OrigWidth;
					Height=OrigHeight;
					break;
				*/
				case CCaptureOptions::SIZE_TYPE_PERCENTAGE:
					{
						int Num,Denom;

						CoreEngine.GetVideoViewSize(&Width,&Height);
						CaptureOptions.GetSizePercentage(&Num,&Denom);
						Width=Width*Num/Denom;
						Height=Height*Num/Denom;
					}
					break;
				case CCaptureOptions::SIZE_TYPE_CUSTOM:
					CaptureOptions.GetCustomSize(&Width,&Height);
					break;
				}
				hGlobal=ResizeImage((BITMAPINFO*)pbmih,
								pDib+CalcDIBInfoSize(pbmih),&rc,Width,Height);
				CoTaskMemFree(pDib);
				::SetCursor(hcurOld);
				if (hGlobal==NULL) {
					return;
				}
				CCaptureImage *pImage=new CCaptureImage(hGlobal);
				const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();
				const CEpgDataInfo *pEpgInfo=CoreEngine.GetEpgDataInfo();
				TCHAR szComment[1024];
				CaptureOptions.GetCommentText(szComment,lengthof(szComment),
								pChInfo!=NULL?pChInfo->GetName():NULL,
								pEpgInfo!=NULL?pEpgInfo->GetEventName():NULL);
				pImage->SetComment(szComment);
				CaptureWindow.SetImage(pImage);
				if (id==CM_COPY) {
					if (!pImage->SetClipboard(hwnd)) {
						ShowErrorMessage(TEXT("クリップボードにデータを設定できません。"));
					}
				} else {
					if (!CaptureOptions.SaveImage(pImage)) {
						ShowErrorMessage(TEXT("画像の保存でエラーが発生しました。"));
					}
				}
				if (!CaptureWindow.HasImage())
					delete pImage;
			}
		}
		return;

	case CM_CAPTUREPREVIEW:
		fShowCaptureWindow=!fShowCaptureWindow;
		if (fShowCaptureWindow) {
			CaptureWindow.Create(hwnd,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
					WS_VISIBLE | WS_CLIPCHILDREN,
				WS_EX_TOOLWINDOW);
		} else {
			CaptureWindow.Destroy();
			CaptureWindow.ClearImage();
		}
		MainMenu.CheckItem(CM_CAPTUREPREVIEW,fShowCaptureWindow);
		return;

	case CM_CAPTUREOPTIONS:
		if (IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd,COptionDialog::PAGE_CAPTURE);
		return;

	case CM_OPENCAPTUREFOLDER:
		CaptureOptions.OpenSaveFolder();
		return;

	case CM_RESET:
		CoreEngine.m_DtvEngine.ResetEngine();
		return;

	case CM_RECORD:
	case CM_RECORD_START:
	case CM_RECORD_STOP:
		if (id==CM_RECORD) {
			if (RecordManager.IsPaused()) {
				SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_START) {
			if (RecordManager.IsRecording()) {
				if (RecordManager.IsPaused())
					SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_STOP) {
			if (!RecordManager.IsRecording())
				return;
		}
		if (RecordManager.IsRecording()) {
			TCHAR szText[512],szFileName[MAX_PATH],szSize[32];

			::lstrcpy(szFileName,RecordManager.GetRecordTask()->GetFileName());
			RecordManager.StopRecord();
			UInt64ToString(CoreEngine.m_DtvEngine.m_FileWriter.GetWriteSize(),
						   szSize,lengthof(szSize));
			::wsprintf(szText,TEXT("録画停止 %s (書き出しサイズ %s Bytes)"),
					   szFileName,szSize);
			Logger.AddLog(szText);
			InfoWindow.SetRecordStatus(false);
			ResidentManager.SetStatus(0,CResidentManager::STATUS_RECORDING);
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
			MainMenu.EnableItem(CM_RECORDOPTION,true);
			MainMenu.EnableItem(CM_RECORDSTOPTIME,false);
			PluginList.SendRecordStatusChangeEvent();
			CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(fDescrambleCurServiceOnly);
			if (m_fStandby)
				CloseTuner();
			if (m_fExitOnRecordingStop)
				PostCommand(CM_EXIT);
		} else {
			if (RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\r\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\r\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
					return;
			}
			AppMain.StartRecord();
		}
		return;

	case CM_RECORD_PAUSE:
		if (RecordManager.IsRecording()) {
			RecordManager.PauseRecord();
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
			Logger.AddLog(RecordManager.IsPaused()?TEXT("録画一時停止"):TEXT("録画再開"));
			PluginList.SendRecordStatusChangeEvent();
		}
		return;

	case CM_RECORDOPTION:
		if (IsWindowEnabled(GetVideoHostWindow())) {
			if (RecordManager.GetFileName()==NULL) {
				TCHAR szFileName[MAX_PATH];

				if (RecordOptions.GetFilePath(szFileName,MAX_PATH))
					RecordManager.SetFileName(szFileName);
			}
			RecordOptions.ApplyOptions(&RecordManager);
			if (RecordManager.RecordDialog(GetVideoHostWindow())) {
				if (RecordManager.IsReserved()) {
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				} else {
					AppMain.StartReservedRecord();
				}
			}
		}
		return;

	case CM_RECORDSTOPTIME:
		if (RecordManager.ChangeStopTimeDialog(GetVideoHostWindow())) {
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
		}
		return;

	case CM_EXITONRECORDINGSTOP:
		m_fExitOnRecordingStop=!m_fExitOnRecordingStop;
		return;

	case CM_OPTIONS_RECORD:
		if (IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd,COptionDialog::PAGE_RECORD);
		return;

	case CM_DISABLEVIEWER:
		EnablePreview(!m_fEnablePreview);
		return;

	case CM_INFORMATION:
		fShowPanelWindow=!fShowPanelWindow;
		if (fShowPanelWindow) {
			PanelFrame.SetPanelVisible(true);
		} else {
			PanelFrame.SetPanelVisible(false);
			InfoWindow.Reset();
		}
		if (!PanelFrame.GetFloating()) {
			int Size;
			RECT rc;

			MainWindow.GetPosition(&rc);
			Size=PanelFrame.GetDockingWidth();
			if (fShowPanelWindow)
				rc.right+=Size;
			else
				rc.right-=Size;
			MainWindow.SetPosition(&rc);
		}
		MainMenu.CheckItem(CM_INFORMATION,fShowPanelWindow);
		return;

	case CM_PROGRAMGUIDE:
		fShowProgramGuide=!fShowProgramGuide;
		if (fShowProgramGuide) {
			HCURSOR hcurOld;

			hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
			ProgramGuide.Create(NULL,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
					WS_THICKFRAME | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE);
			SYSTEMTIME stFirst,stLast;
			ProgramGuideOptions.GetTimeRange(&stFirst,&stLast);
			ProgramGuide.SetTimeRange(&stFirst,&stLast);
			const CChannelList *pList;
			if (!CoreEngine.IsNetworkDriver())
				pList=ChannelManager.GetCurrentChannelList();
			else
				pList=ChannelManager.GetFileAllChannelList();
			if (pList!=NULL) {
				/*
				CProgramGuideServiceIDList ServiceIDList;

				for (int i=0;i<pList->NumChannels();i++) {
					WORD ServiceID=pList->GetChannelInfo(i)->GetServiceID();
					if (ServiceID!=0) {
						ServiceIDList.Add(ServiceID);
						EpgProgramList.UpdateProgramList(ServiceID);
					}
				}
				ProgramGuide.SetServiceIDList(&ServiceIDList);
				*/
				EpgProgramList.UpdateProgramList();
				ProgramGuide.SetChannelList(pList);
			}
			ProgramGuide.UpdateProgramGuide();
			::SetCursor(hcurOld);
		} else {
			ProgramGuide.Destroy();
		}
		MainMenu.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		return;

	case CM_STATUSBAR:
		SetStatusBarVisible(!m_fShowStatusBar);
		return;

	case CM_TITLEBAR:
		SetTitleBarVisible(!m_fShowTitleBar);
		return;

	case CM_DECODERPROPERTY:
		CoreEngine.m_DtvEngine.DisplayVideoDecoderProperty(hwnd);
		return;

	case CM_OPTIONS:
		if (IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd);
		return;

	case CM_STREAMINFO:
		StreamInfo.Show(hwnd);
		return;

	case CM_CLOSE:
		if (m_fStandby) {
			SetStandby(false);
		} else if (ResidentManager.GetResident()) {
			SetStandby(true);
		} else {
			PostMessage(WM_CLOSE,0,0);
		}
		return;

	case CM_EXIT:
		PostMessage(WM_CLOSE,0,0);
		return;

	case CM_SHOW:
		if (m_fStandby) {
			SetStandby(false);
		} else {
			SetWindowVisible();
		}
		return;

	case CM_CHANNEL_UP:
	case CM_CHANNEL_DOWN:
		{
			const CChannelInfo *pInfo=ChannelManager.GetNextChannelInfo(id==CM_CHANNEL_UP);

			if (pInfo!=NULL) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();

				if (pList->HasRemoteControlKeyID())
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelIndex());
			} else {
				SendCommand(CM_CHANNEL_FIRST);
			}
		}
		return;

	case CM_MENU:
		{
			POINT pt;

			pt.x=0;
			pt.y=0;
			::ClientToScreen(GetThis(hwnd)->m_ViewWindow.GetHandle(),&pt);
			PopupMenu(&pt);
		}
		return;

	case CM_ENABLEBUFFERING:
		CoreEngine.SetPacketBuffering(!CoreEngine.GetPacketBuffering());
		return;

	case CM_RESETBUFFER:
		CoreEngine.m_DtvEngine.ResetBuffer();
		return;

	default:
		if (id>=CM_AUDIOSTREAM_FIRST && id<=CM_AUDIOSTREAM_LAST) {
			if (CoreEngine.m_DtvEngine.SetAudioStream(id-CM_AUDIOSTREAM_FIRST)) {
				MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
										CM_AUDIOSTREAM_FIRST+CoreEngine.m_DtvEngine.GetAudioStreamNum()-1,
										id);
			}
			return;
		}
		if (id>=CM_CAPTURESIZE_FIRST && id<=CM_CAPTURESIZE_LAST) {
			int CaptureSize=id-CM_CAPTURESIZE_FIRST;

			CaptureOptions.SetPresetCaptureSize(CaptureSize);
			MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,id);
			return;
		}
		if (id>=CM_CHANNELNO_FIRST && id<=CM_CHANNELNO_LAST) {
			int No=id-CM_CHANNELNO_FIRST;

			if (pNetworkRemocon!=NULL) {
				if (RecordManager.IsRecording()) {
					if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
						return;
				}
				pNetworkRemocon->SetChannel(No);
				ChannelManager.SetNetworkRemoconCurrentChannel(
					ChannelManager.GetCurrentChannelList()->FindChannelNo(No+1));
				OnChannelChange();
				PluginList.SendChannelChangeEvent();
				return;
			} else {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();
				int Index;

				if (pList->HasRemoteControlKeyID()) {
					Index=pList->FindChannelNo(No+1);
					if (Index<0)
						return;
				} else {
					Index=No;
				}
				id=CM_CHANNEL_FIRST+Index;
			}
		}
		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			int Channel=id-CM_CHANNEL_FIRST;

			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
					return;
			}
			const CChannelList *pChList=ChannelManager.GetCurrentRealChannelList();
			if (pChList==NULL)
				return;
			const CChannelInfo *pChInfo=pChList->GetChannelInfo(Channel);
			if (pChInfo==NULL)
				return;
			AppMain.SetChannel(ChannelManager.GetCurrentSpace(),Channel);
			return;
		}
		if (id>=CM_SERVICE_FIRST && id<=CM_SERVICE_LAST) {

			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmServiceChange(GetVideoHostWindow(),
														&RecordManager))
					return;
			}
			AppMain.SetServiceByIndex(id-CM_SERVICE_FIRST);
			return;
		}
		if (id>=CM_SPACE_ALL && id<=CM_SPACE_LAST) {
			int Space=id-CM_SPACE_FIRST;

			if (Space!=ChannelManager.GetCurrentSpace())
				AppMain.SetChannel(Space,0);
			return;
		}
		if (id>=CM_DRIVER_FIRST && id<=CM_DRIVER_LAST) {
			int Driver=id-CM_DRIVER_FIRST;

			if (Driver<DriverManager.NumDrivers()) {
				const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(Driver);

				if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())!=0) {
					if (AppMain.SetDriver(pDriverInfo->GetFileName())) {
						::lstrcpy(szDriverFileName,pDriverInfo->GetFileName());
						if (RestoreChannelInfo.Space>=0
								&& RestoreChannelInfo.Channel>=0) {
							const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
							if (pList!=NULL) {
								int Index=pList->Find(RestoreChannelInfo.Space,
													  RestoreChannelInfo.Channel,
													  RestoreChannelInfo.Service);
								if (Index>=0)
									AppMain.SetChannel(RestoreChannelInfo.Space,Index);
							}
						}
					}
				}
			}
			return;
		}
		if (id>=CM_PLUGIN_FIRST && id<=CM_PLUGIN_LAST) {
			CPlugin *pPlugin=PluginList.GetPlugin(PluginList.FindPluginByCommand(id));

			if (pPlugin!=NULL)
				pPlugin->Enable(!pPlugin->IsEnabled());
			return;
		}
		if (id>=CM_SPACE_CHANNEL_FIRST && id<=CM_SPACE_CHANNEL_LAST) {
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
					return;
			}
			AppMain.ProcessTunerSelectMenu(id);
			return;
		}
		if (id>=CM_CHANNELHISTORY_FIRST && id<=CM_CHANNELHISTORY_LAST) {
			const CDriverChannelInfo *pChannelInfo=ChannelHistory.GetChannelInfo(id-CM_CHANNELHISTORY_FIRST);

			if (pChannelInfo!=NULL) {
				if (RecordManager.IsRecording()) {
					if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
						return;
				}
				if (::lstrcmpi(pChannelInfo->GetDriverFileName(),CoreEngine.GetDriverFileName())!=0) {
					if (!AppMain.SetDriver(pChannelInfo->GetDriverFileName()))
						return;
					::lstrcpy(szDriverFileName,pChannelInfo->GetDriverFileName());
				}
				const CChannelList *pList=ChannelManager.GetChannelList(pChannelInfo->GetSpace());
				if (pList!=NULL) {
					int Index=-1;
					bool fService=false;

					if (pChannelInfo->GetServiceID()!=0)
						Index=pList->FindServiceID(pChannelInfo->GetServiceID());
					if (Index<0) {
						Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),-1);
						fService=true;
					}
					if (Index>=0) {
						AppMain.SetChannel(pChannelInfo->GetSpace(),Index,
									fService?pChannelInfo->GetService():-1);
					}
				}
			}
			return;
		}
		if (id>=CM_PLUGINCOMMAND_FIRST && id<=CM_PLUGINCOMMAND_LAST) {
			PluginList.OnPluginCommand(CommandList.GetCommandText(CommandList.IDToIndex(id)));
		}
	}
}


BOOL CMainWindow::OnAppCommand(HWND hwnd,HWND hwndFrom,int nCommand,
													UINT uDevice,DWORD dwKeys)
{
#ifndef APPCOMMAND_MEDIA_PLAY
#define APPCOMMAND_OPEN					30
#define APPCOMMAND_CLOSE				31
#define APPCOMMAND_MEDIA_PLAY			46
#define APPCOMMAND_MEDIA_PAUSE			47
#define APPCOMMAND_MEDIA_RECORD			48
#define APPCOMMAND_MEDIA_FAST_FORWARD	49
#define APPCOMMAND_MEDIA_REWIND			50
#define APPCOMMAND_MEDIA_CHANNEL_UP		51
#define APPCOMMAND_MEDIA_CHANNEL_DOWN	52
#endif

	static const struct {
		WORD AppCommand;
		WORD Command;
	} CommandTable [] = {
		{APPCOMMAND_VOLUME_MUTE,			CM_VOLUME_MUTE},
		{APPCOMMAND_VOLUME_DOWN,			CM_VOLUME_DOWN},
		{APPCOMMAND_VOLUME_UP,				CM_VOLUME_UP},
		{APPCOMMAND_MEDIA_NEXTTRACK,		CM_CHANNEL_UP},
		{APPCOMMAND_MEDIA_PREVIOUSTRACK,	CM_CHANNEL_DOWN},
		//{APPCOMMAND_MEDIA_PLAY_PAUSE,		CM_RECORD_PAUSE},
		{APPCOMMAND_MEDIA_STOP,				CM_RECORD_STOP},
		{APPCOMMAND_CLOSE,					CM_CLOSE},
		{APPCOMMAND_MEDIA_PAUSE,			CM_RECORD_PAUSE},
		{APPCOMMAND_MEDIA_RECORD,			CM_RECORD_START},
		{APPCOMMAND_MEDIA_CHANNEL_UP,		CM_CHANNEL_UP},
		{APPCOMMAND_MEDIA_CHANNEL_DOWN,		CM_CHANNEL_DOWN},
	};
	int i;

	for (i=0;i<lengthof(CommandTable);i++) {
		if (nCommand==(int)CommandTable[i].AppCommand) {
			SendCommand(CommandTable[i].Command);
			break;
		}
	}
	return TRUE;
}


void CMainWindow::OnTimer(HWND hwnd,UINT id)
{
	switch (id) {
	case TIMER_ID_UPDATE:
		{
			static unsigned int TimerCount=0;
			const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();

			DWORD UpdateStatus=CoreEngine.UpdateAsyncStatus();
			DWORD UpdateStatistics=CoreEngine.UpdateStatistics();

			if ((UpdateStatus&CCoreEngine::STATUS_VIDEOSIZE)!=0) {
				StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				InfoWindow.SetVideoSize(CoreEngine.GetOriginalVideoWidth(),
										CoreEngine.GetOriginalVideoHeight(),
										CoreEngine.GetDisplayVideoWidth(),
										CoreEngine.GetDisplayVideoHeight());
				CheckZoomMenu();
			}

			if ((UpdateStatus&(CCoreEngine::STATUS_AUDIOCHANNELS
							 | CCoreEngine::STATUS_AUDIOCOMPONENTTYPE))!=0) {
				SetStereoMode(CoreEngine.m_DtvEngine.GetAudioComponentType()==0x02?1:0);
				StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
			}

			if ((UpdateStatus&CCoreEngine::STATUS_EVENTID)!=0) {
				TCHAR szText[256],szTitle[256+64];

				::lstrcpy(szTitle,TITLE_TEXT);
				if (pChInfo!=NULL) {
					::lstrcat(szTitle,TEXT(" - "));
					::lstrcat(szTitle,pChInfo->GetName());
				}
				if (CoreEngine.m_DtvEngine.GetEventName(szText,lengthof(szText))>0) {
					if (ViewOptions.GetNotifyEventName()) {
						TCHAR szBarText[256+16];
						SYSTEMTIME stStart,stEnd;

						if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,&stEnd)) {
							::wsprintf(szBarText,TEXT("%d:%02d～%d:%02d "),
									   stStart.wHour,stStart.wMinute,
									   stEnd.wHour,stEnd.wMinute);
						} else {
							szBarText[0]='\0';
						}
						::lstrcat(szBarText,szText);
						NotificationBar.SetText(szBarText);
						NotificationBar.Show(3000);
					}
					::lstrcat(szTitle,TEXT(" / "));
					::lstrcat(szTitle,szText);
				}
				::SetWindowText(m_hwnd,szTitle);

				CoreEngine.UpdateEpgDataInfo();
				StatusView.UpdateItem(STATUS_ITEM_PROGRAMINFO);

				if (ViewOptions.GetResetPanScanEventChange()
						&& AspectRatioType!=0) {
					CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(0,0);
					if (!m_fFullscreen && !::IsZoomed(hwnd)) {
						SIZE sz;
						int Width,Height;

						m_VideoContainer.GetClientSize(&sz);
						if (CoreEngine.GetVideoViewSize(&Width,&Height))
							AdjustWindowSize(Width*sz.cy/Height,sz.cy);
					}
					AspectRatioType=0;
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
					MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,CM_ASPECTRATIO_DEFAULT);
					CheckZoomMenu();
				}
			}

			if (RecordManager.IsRecording()) {
				if (RecordManager.QueryStop()) {
					SendCommand(CM_RECORD_STOP);
				} else if (!RecordManager.IsPaused()) {
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			} else {
				if (RecordManager.QueryStart())
					AppMain.StartReservedRecord();
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
				StatusView.UpdateItem(STATUS_ITEM_ERROR);
			}

			if (TimerCount%4==0) {	// 負荷軽減
				CoreEngine.UpdateEpgDataInfo();

				const CEpgDataInfo *pInfo=CoreEngine.GetEpgDataInfo();

				if (pInfo!=NULL) {
					WCHAR szText[2048];

					if (fShowPanelWindow) {
						if (InfoWindow.GetProgramInfoNext())
							pInfo=CoreEngine.GetEpgDataInfo(true);
						if (pInfo!=NULL) {
							SYSTEMTIME stStart,stEnd;

							pInfo->GetStartTime(&stStart);
							pInfo->GetEndTime(&stEnd);
							::wnsprintf(szText,lengthof(szText),
								L"%d/%d/%d(%s) %d:%02d～%d:%02d\r\n%s\r\n\r\n%s\r\n\r\n%s",
								stStart.wYear,
								stStart.wMonth,
								stStart.wDay,
								L"日\0月\0火\0水\0木\0金\0土"+stStart.wDayOfWeek*2,
								stStart.wHour,
								stStart.wMinute,
								stEnd.wHour,
								stEnd.wMinute,
								pInfo->GetEventName(),
								pInfo->GetEventText(),
								pInfo->GetEventExtText());
							InfoWindow.SetProgramInfo(szText);
						}
					}
				}
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
								 | CCoreEngine::STATISTIC_BITRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_SIGNALLEVEL);

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_STREAMREMAIN
								 | CCoreEngine::STATISTIC_PACKETBUFFERRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_BUFFERING);

			StatusView.UpdateItem(STATUS_ITEM_CLOCK);

			if (InfoWindow.IsVisible()) {
				BYTE AspectX,AspectY;
				if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
					InfoWindow.SetAspectRatio(AspectX,AspectY);

				if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
									 | CCoreEngine::STATISTIC_BITRATE))!=0) {
					InfoWindow.SetBitRate(CoreEngine.GetBitRateFloat());
					if (CoreEngine.IsNetworkDriver()
							//|| (DriverType==CCoreEngine::DRIVER_HDUS && !fNewHDUSDriver)) {
							|| DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName())) {
						InfoWindow.ShowSignalLevel(false);
					} else {
						InfoWindow.SetSignalLevel(CoreEngine.GetSignalLevel());
					}
				}

				if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
									 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
									 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
					InfoWindow.UpdateErrorCount();
				}

				if (RecordManager.IsRecording()) {
					const CRecordTask *pRecordTask=RecordManager.GetRecordTask();

					InfoWindow.SetRecordStatus(true,pRecordTask->GetFileName(),
						pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime());
				}
			}
			TimerCount++;
		}
		break;

	case TIMER_ID_OSD:
		CoreEngine.m_DtvEngine.m_MediaViewer.ClearOSD();
		::KillTimer(hwnd,TIMER_ID_OSD);
		break;

	case TIMER_ID_DISPLAY:
		::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		break;

	case TIMER_ID_WHEELCHANNELCHANGE:
		{
			const CChannelInfo *pInfo=ChannelManager.GetChangingChannelInfo();

			fWheelChannelChanging=false;
			ChannelManager.SetChangingChannel(-1);
			if (pInfo!=NULL) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();

				if (pList->HasRemoteControlKeyID())
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelIndex());
			}
			::KillTimer(hwnd,TIMER_ID_WHEELCHANNELCHANGE);
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		if (fShowPanelWindow) {
			if (InfoPanel.GetCurTab()==PANEL_TAB_PROGRAMLIST) {
				WORD ServiceID=ChannelManager.GetCurrentServiceID();

				if (ServiceID==0)
					CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,
											CoreEngine.m_DtvEngine.GetService());
				if (ServiceID!=0)
					ProgramListView.UpdateProgramList(CoreEngine.m_DtvEngine.m_ProgManager.GetTransportStreamID(),ServiceID);
			}
		}
		ProgramListUpdateTimerCount++;
		if (ProgramListUpdateTimerCount==12)
			// 更新頻度を下げる
			::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,3*60*1000,NULL);
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		if (!m_fStandby)
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
		{
			const CChannelInfo *pChInfo=ChannelManager.GetNextChannelInfo(true);
			if (pChInfo==NULL
					|| pChInfo->GetChannelIndex()==m_ProgramGuideUpdateStartChannel)
				ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
			else
				PostCommand(CM_CHANNEL_UP);
		}
		break;

	case TIMER_ID_CHANNELPANELUPDATE:
		if (fShowPanelWindow && InfoPanel.GetCurTab()==PANEL_TAB_CHANNEL) {
			ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
		}
		::KillTimer(hwnd,TIMER_ID_CHANNELPANELUPDATE);
		break;
	}
}


void CMainWindow::OnChannelChange()
{
	const CChannelInfo *pInfo;

	SetTitleText();
	if (OSDOptions.GetShowOSD())
		ShowChannelOSD();
	ProgramListView.ClearProgramList();
	BeginProgramListUpdateTimer();
	if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL)
		ControlPanel.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
								CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
	ChannelHistory.Add(CoreEngine.GetDriverFileName(),
					   ChannelManager.GetCurrentRealChannelInfo());
}


void CMainWindow::ShowChannelOSD()
{
	const CChannelInfo *pInfo;

	if (fWheelChannelChanging)
		pInfo=ChannelManager.GetChangingChannelInfo();
	else
		pInfo=ChannelManager.GetCurrentChannelInfo();
	if (pInfo!=NULL) {
		TCHAR szText[4+MAX_CHANNEL_NAME];
		int Length=0;

		if (pInfo->GetChannelNo()!=0)
			Length=wsprintf(szText,TEXT("%d "),pInfo->GetChannelNo());
		wsprintf(szText+Length,TEXT("%s"),pInfo->GetName());
		if (!OSDOptions.GetPseudoOSD()
				&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
			RECT rc;

			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rc)) {
				LOGFONT lf;
				HFONT hfont;

				GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
				lf.lfHeight=(rc.right-rc.left)/20;
				lf.lfQuality=NONANTIALIASED_QUALITY;
				hfont=CreateFontIndirect(&lf);
				rc.left+=16;
				rc.top+=48;
				if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,
						rc.left,rc.top,hfont,
						OSDOptions.GetTextColor(),OSDOptions.GetOpacity())) {
					if (OSDOptions.GetFadeTime()>0)
						SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_OSD,OSDOptions.GetFadeTime(),NULL);
				}
				DeleteObject(hfont);
			}
		} else {
			SIZE sz;
			COLORREF cr;

			ChannelOSD.Create(m_VideoContainer.GetHandle());
			ChannelOSD.SetTextHeight(32);
			ChannelOSD.SetText(szText);
			ChannelOSD.CalcTextSize(&sz);
			ChannelOSD.SetPosition(8,24,sz.cx+8,sz.cy+8);
			if (fWheelChannelChanging)
				cr=MixColor(OSDOptions.GetTextColor(),RGB(0,0,0),160);
			else
				cr=OSDOptions.GetTextColor();
			ChannelOSD.SetTextColor(cr);
			ChannelOSD.Show(OSDOptions.GetFadeTime(),!fWheelChannelChanging && !ChannelOSD.IsVisible());
		}
	}
}


void CMainWindow::OnDriverChange()
{
	if (fShowPanelWindow && InfoPanel.GetCurTab()==PANEL_TAB_CHANNEL) {
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	}
}


void CMainWindow::OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fStatus)
{
	int Mode;

	if (wParam&MK_SHIFT)
		Mode=WheelShiftMode;
	else
		Mode=WheelMode;
	if (fStatus && StatusView.GetVisible()) {
		POINT pt;
		RECT rc;

		pt.x=GET_X_LPARAM(lParam);
		pt.y=GET_Y_LPARAM(lParam);
		StatusView.GetScreenPosition(&rc);
		if (PtInRect(&rc,pt)) {
			switch (StatusView.GetCurItem()) {
			case STATUS_ITEM_CHANNEL:		Mode=WHEEL_MODE_CHANNEL;	break;
			//case STATUS_ITEM_VIDEOSIZE:		Mode=WHEEL_MODE_ZOOM;		break;
			case STATUS_ITEM_VOLUME:		Mode=WHEEL_MODE_VOLUME;		break;
			case STATUS_ITEM_AUDIOCHANNEL:	Mode=WHEEL_MODE_STEREOMODE;	break;
			}
		}
	}
	switch (Mode) {
	case WHEEL_MODE_VOLUME:
		SendCommand(GET_WHEEL_DELTA_WPARAM(wParam)>=0?CM_VOLUME_UP:CM_VOLUME_DOWN);
		break;
	case WHEEL_MODE_CHANNEL:
		{
			bool fUp;
			const CChannelInfo *pInfo;

			if (fWheelChannelReverse)
				fUp=GET_WHEEL_DELTA_WPARAM(wParam)>0;
			else
				fUp=GET_WHEEL_DELTA_WPARAM(wParam)<0;
			pInfo=ChannelManager.GetNextChannelInfo(fUp);
			if (pInfo!=NULL) {
				fWheelChannelChanging=true;
				ChannelManager.SetChangingChannel(ChannelManager.FindChannelInfo(pInfo));
				StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (OSDOptions.GetShowOSD())
					ShowChannelOSD();
				SetTimer(m_hwnd,TIMER_ID_WHEELCHANNELCHANGE,WheelChannelDelay,NULL);
			}
		}
		break;
	case WHEEL_MODE_STEREOMODE:
		SendCommand(CM_SWITCHAUDIO);
		break;
	case WHEEL_MODE_ZOOM:
		if (!IsZoomed(m_hwnd) && !m_fFullscreen) {
			int Zoom;

			Zoom=CalcZoomRate();
			if (GET_WHEEL_DELTA_WPARAM(wParam)>=0)
				Zoom+=5;
			else
				Zoom-=5;
			SetZoomRate(Zoom);
		}
		break;
	}
}


void CMainWindow::PopupMenu(const POINT *pPos/*=NULL*/)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	MainMenu.Popup(TPM_RIGHTBUTTON,pt.x,pt.y,m_hwnd,true);
}


bool CMainWindow::SetPreview(bool fPreview)
{
	m_VideoContainer.SetVisible(fPreview);
	CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(fPreview);
	CoreEngine.EnablePreview(fPreview);
	return true;
}


bool CMainWindow::EnablePreview(bool fEnable)
{
	if (m_fEnablePreview!=fEnable) {
		if (fEnable && !CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			bool fOK=BuildMediaViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			if (!fOK)
				return false;
		}
		SetPreview(fEnable);
		MainMenu.CheckItem(CM_DISABLEVIEWER,!fEnable);
		m_fEnablePreview=fEnable;
		PluginList.SendPreviewChangeEvent(fEnable);
	}
	return true;
}

int CMainWindow::GetVolume() const
{
	return CoreEngine.GetVolume();
}


bool CMainWindow::SetVolume(int Volume,bool fOSD)
{
	if (!CoreEngine.SetVolume(Volume))
		return false;
	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,false);
	PluginList.SendVolumeChangeEvent(Volume,false);
	if (fOSD && OSDOptions.GetShowOSD()) {
		TCHAR szText[64];
		int i;

		szText[0]='\0';
		for (i=0;i<Volume/5;i++)
			lstrcat(szText,TEXT("■"));
		for (;i<20;i++)
			lstrcat(szText,TEXT("□"));
		if (!OSDOptions.GetPseudoOSD()
				&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
			RECT rc;

			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rc)) {
				LOGFONT lf;
				HFONT hfont;

				GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
				lf.lfHeight=(rc.right-rc.left)/30;
				lf.lfQuality=NONANTIALIASED_QUALITY;
				hfont=CreateFontIndirect(&lf);
				rc.left+=16;
				rc.top=rc.bottom-(lf.lfHeight+16);
				if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,
						rc.left,rc.top,hfont,
						OSDOptions.GetTextColor(),OSDOptions.GetOpacity())) {
					if (OSDOptions.GetFadeTime()>0)
						SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_OSD,OSDOptions.GetFadeTime(),NULL);
				}
				DeleteObject(hfont);
			}
		} else {
			RECT rc;
			SIZE sz;

			m_VideoContainer.GetClientRect(&rc);
			VolumeOSD.Create(m_VideoContainer.GetHandle());
			VolumeOSD.SetTextHeight(LimitRange((int)(rc.right-rc.left-32)/20,8,16));
			VolumeOSD.SetText(szText);
			VolumeOSD.CalcTextSize(&sz);
			if (StatusView.GetParent()==m_VideoContainer.GetHandle())
				rc.bottom-=StatusView.GetHeight();
			VolumeOSD.SetPosition(8,rc.bottom-sz.cy-8,sz.cx,sz.cy);
			VolumeOSD.SetTextColor(OSDOptions.GetTextColor());
			VolumeOSD.Show(OSDOptions.GetFadeTime());
		}
	}
	return true;
}


bool CMainWindow::GetMute() const
{
	return CoreEngine.GetMute();
}


bool CMainWindow::SetMute(bool fMute)
{
	if (fMute!=GetMute()) {
		CoreEngine.SetMute(fMute);
		StatusView.UpdateItem(STATUS_ITEM_VOLUME);
		ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
		MainMenu.CheckItem(CM_VOLUME_MUTE,CoreEngine.GetMute());
		PluginList.SendVolumeChangeEvent(GetVolume(),true);
	}
	return true;
}


int CMainWindow::GetStereoMode() const
{
	return CoreEngine.GetStereoMode();
}


bool CMainWindow::SetStereoMode(int StereoMode)
{
	if (StereoMode!=GetStereoMode()) {
		if (!CoreEngine.SetStereoMode(StereoMode))
			return false;
		MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
								CM_STEREO_THROUGH+StereoMode);
		StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
		PluginList.SendStereoModeChangeEvent(StereoMode);
	}
	return true;
}


bool CMainWindow::SwitchAudio()
{
	int NumStreams=CoreEngine.m_DtvEngine.GetAudioStreamNum(CoreEngine.m_DtvEngine.GetService());

	if (NumStreams>1) {
		SendCommand(CM_AUDIOSTREAM_FIRST+
					(CoreEngine.m_DtvEngine.GetAudioStream()+1)%NumStreams);
	} else if (CoreEngine.m_DtvEngine.GetAudioChannelNum()==2) {
		SetStereoMode((GetStereoMode()+1)%3);
	} else {
		return false;
	}
	return true;
}


bool CMainWindow::SetZoomRate(int ZoomNum,int ZoomDenom/*=100*/)
{
	int Width,Height;

	if (ZoomNum<1 || ZoomDenom<1)
		return false;
	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0)
		AdjustWindowSize(Width*ZoomNum/ZoomDenom,Height*ZoomNum/ZoomDenom);
	return true;
}


int CMainWindow::CalcZoomRate()
{
	int Zoom=0;
	int Width,Height;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		/*
		SIZE sz;

		m_VideoContainer.GetClientSize(&sz);
		Zoom=(sz.cy*100+Height/2)/Height;
		*/
		WORD DstWidth,DstHeight;
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight)) {
			Zoom=(DstHeight*100+Height/2)/Height;
		}
	}
	return Zoom;
}


bool CMainWindow::CalcZoomRate(int *pNum,int *pDenom)
{
	bool fOK=false;
	int Width,Height;
	int Num=0,Denom=1;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		/*
		SIZE sz;

		m_VideoContainer.GetClientSize(&sz);
		Num=sz.cy;
		Denom=Height;
		*/
		WORD DstWidth,DstHeight;
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight)) {
			Num=DstHeight;
			Denom=Height;
		}
		fOK=true;
	}
	if (pNum)
		*pNum=Num;
	if (pDenom)
		*pDenom=Denom;
	return fOK;
}


void CMainWindow::CheckZoomMenu()
{
	int ZoomNum,ZoomDenom;

	CalcZoomRate(&ZoomNum,&ZoomDenom);
	for (int i=0;i<lengthof(ZoomRateList);i++) {
		MainMenu.CheckItem(CM_ZOOM_FIRST+i,
			ZoomRateList[i].Num*100/ZoomRateList[i].Denom==ZoomNum*100/ZoomDenom);
	}
}




/*
	ウィンドウの端が実際に表示されているか判定する
	これだと不完全(常に最前面のウィンドウを考慮していない)
*/
static bool IsWindowEdgeVisible(HWND hwnd,HWND hwndTop,const RECT *pRect,HWND hwndTarget)
{
	RECT rc,rcEdge;
	HWND hwndNext;

	if (hwndTop==hwnd || hwndTop==NULL)
		return true;
	GetWindowRect(hwndTop,&rc);
	hwndNext=GetNextWindow(hwndTop,GW_HWNDNEXT);
	if (hwndTop==hwndTarget || !IsWindowVisible(hwndTop)
			|| rc.left==rc.right || rc.top==rc.bottom)
		return IsWindowEdgeVisible(hwnd,hwndNext,pRect,hwndTarget);
	if (pRect->top==pRect->bottom) {
		if (rc.top<=pRect->top && rc.bottom>pRect->top) {
			if (rc.left<=pRect->left && rc.right>=pRect->right)
				return false;
			if (rc.left<=pRect->left && rc.right>pRect->left) {
				rcEdge=*pRect;
				rcEdge.right=min(rc.right,pRect->right);
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.left>pRect->left && rc.right>=pRect->right) {
				rcEdge=*pRect;
				rcEdge.left=rc.left;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.left>pRect->left && rc.right<pRect->right) {
				rcEdge=*pRect;
				rcEdge.right=rc.left;
				if (IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget))
					return true;
				rcEdge.left=rc.right;
				rcEdge.right=pRect->right;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			}
		}
	} else {
		if (rc.left<=pRect->left && rc.right>pRect->left) {
			if (rc.top<=pRect->top && rc.bottom>=pRect->bottom)
				return false;
			if (rc.top<=pRect->top && rc.bottom>pRect->top) {
				rcEdge=*pRect;
				rcEdge.bottom=min(rc.bottom,pRect->bottom);
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.top>pRect->top && rc.bottom>=pRect->bottom) {
				rcEdge=*pRect;
				rcEdge.top=rc.top;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.top>pRect->top && rc.bottom<pRect->bottom) {
				rcEdge=*pRect;
				rcEdge.bottom=rc.top;
				if (IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget))
					return true;
				rcEdge.top=rc.bottom;
				rcEdge.bottom=pRect->bottom;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			}
		}
	}
	return IsWindowEdgeVisible(hwnd,hwndNext,pRect,hwndTarget);
}


struct SnapWindowInfo {
	HWND hwnd;
	RECT rcOriginal;
	RECT rcNearest;
};

static BOOL CALLBACK SnapWindowProc(HWND hwnd,LPARAM lParam)
{
	SnapWindowInfo *pInfo=reinterpret_cast<SnapWindowInfo*>(lParam);

	if (IsWindowVisible(hwnd) && hwnd!=pInfo->hwnd
			&& (hwnd!=PanelFrame.GetHandle() || !PanelStatus.IsAttached())) {
		RECT rc,rcEdge;

		GetWindowRect(hwnd,&rc);
		if (rc.right>rc.left && rc.bottom>rc.top) {
			if (rc.top<pInfo->rcOriginal.bottom && rc.bottom>pInfo->rcOriginal.top) {
				if (abs(rc.left-pInfo->rcOriginal.right)<abs(pInfo->rcNearest.right)) {
					rcEdge.left=rc.left;
					rcEdge.right=rc.left;
					rcEdge.top=max(rc.top,pInfo->rcOriginal.top);
					rcEdge.bottom=min(rc.bottom,pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.right=rc.left-pInfo->rcOriginal.right;
				}
				if (abs(rc.right-pInfo->rcOriginal.left)<abs(pInfo->rcNearest.left)) {
					rcEdge.left=rc.right;
					rcEdge.right=rc.right;
					rcEdge.top=max(rc.top,pInfo->rcOriginal.top);
					rcEdge.bottom=min(rc.bottom,pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.left=rc.right-pInfo->rcOriginal.left;
				}
			}
			if (rc.left<pInfo->rcOriginal.right && rc.right>pInfo->rcOriginal.left) {
				if (abs(rc.top-pInfo->rcOriginal.bottom)<abs(pInfo->rcNearest.bottom)) {
					rcEdge.left=max(rc.left,pInfo->rcOriginal.left);
					rcEdge.right=min(rc.right,pInfo->rcOriginal.right);
					rcEdge.top=rc.top;
					rcEdge.bottom=rc.top;
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.bottom=rc.top-pInfo->rcOriginal.bottom;
				}
				if (abs(rc.bottom-pInfo->rcOriginal.top)<abs(pInfo->rcNearest.top)) {
					rcEdge.left=max(rc.left,pInfo->rcOriginal.left);
					rcEdge.right=min(rc.right,pInfo->rcOriginal.right);
					rcEdge.top=rc.bottom;
					rcEdge.bottom=rc.bottom;
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.top=rc.bottom-pInfo->rcOriginal.top;
				}
			}
		}
	}
	return TRUE;
}


static void SnapWindow(HWND hwnd,RECT *prc)
{
	HMONITOR hMonitor;
	RECT rc;
	SnapWindowInfo Info;
	int XOffset,YOffset;
	int x,y;

	hMonitor=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor,&mi);
		rc=mi.rcMonitor;
	} else {
		rc.left=0;
		rc.top=0;
		rc.right=GetSystemMetrics(SM_CXSCREEN);
		rc.bottom=GetSystemMetrics(SM_CYSCREEN);
	}
	Info.hwnd=hwnd;
	Info.rcOriginal=*prc;
	Info.rcNearest.left=rc.left-prc->left;
	Info.rcNearest.top=rc.top-prc->top;
	Info.rcNearest.right=rc.right-prc->right;
	Info.rcNearest.bottom=rc.bottom-prc->bottom;
	EnumWindows(SnapWindowProc,reinterpret_cast<LPARAM>(&Info));
	if (abs(Info.rcNearest.left)<abs(Info.rcNearest.right)
			|| Info.rcNearest.left==Info.rcNearest.right)
		XOffset=Info.rcNearest.left;
	else if (abs(Info.rcNearest.left)>abs(Info.rcNearest.right))
		XOffset=Info.rcNearest.right;
	else
		XOffset=0;
	if (abs(Info.rcNearest.top)<abs(Info.rcNearest.bottom)
			|| Info.rcNearest.top==Info.rcNearest.bottom)
		YOffset=Info.rcNearest.top;
	else if (abs(Info.rcNearest.top)>abs(Info.rcNearest.bottom))
		YOffset=Info.rcNearest.bottom;
	else
		YOffset=0;
	if (abs(XOffset)<=ViewOptions.GetSnapAtWindowEdgeMargin())
		prc->left+=XOffset;
	if (abs(YOffset)<=ViewOptions.GetSnapAtWindowEdgeMargin())
		prc->top+=YOffset;
	prc->right=prc->left+(Info.rcOriginal.right-Info.rcOriginal.left);
	prc->bottom=prc->top+(Info.rcOriginal.bottom-Info.rcOriginal.top);
}


CMainWindow *CMainWindow::GetThis(HWND hwnd)
{
	return static_cast<CMainWindow*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CMainWindow *pThis=static_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd,lParam));

			if (!pThis->OnCreate())
				return -1;
		}
		return 0;

	case WM_SIZE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (wParam==SIZE_MINIMIZED) {
				ResidentManager.SetStatus(CResidentManager::STATUS_MINIMIZED,
										  CResidentManager::STATUS_MINIMIZED);
				if (ViewOptions.GetDisablePreviewWhenMinimized()) {
					pThis->SetPreview(false);
					PluginList.SendPreviewChangeEvent(false);
				}
			} else if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
				pThis->SetWindowVisible();
			}

			if (wParam==SIZE_MAXIMIZED && !pThis->m_fShowTitleBar) {
				HMONITOR hMonitor=::MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi;

				mi.cbSize=sizeof(MONITORINFO);
				::GetMonitorInfo(hMonitor,&mi);
				pThis->SetPosition(&mi.rcWork);
				SIZE sz;
				pThis->GetClientSize(&sz);
				lParam=MAKELPARAM(sz.cx,sz.cy);
			}

			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			if ((pThis->m_fShowStatusBar || StatusView.GetVisible())
					&& StatusView.GetParent()==hwnd) {
				if (pThis->m_fShowStatusBar) {
					Height-=StatusView.GetHeight();
					StatusView.SetPosition(0,Height,Width,StatusView.GetHeight());
				} else {
					// 一時的な表示
					StatusView.SetPosition(0,Height-StatusView.GetHeight(),
											Width,StatusView.GetHeight());
				}
			}
			Splitter.SetPosition(0,0,Width,Height);
			if (TitleBar.GetVisible()) {
				RECT rc;

				pThis->m_ViewWindow.GetScreenPosition(&rc);
				MapWindowRect(NULL,hwnd,&rc);
				rc.bottom=TitleBar.GetHeight();
				TitleBar.SetPosition(&rc);
			}
			if (NotificationBar.GetVisible()) {
				RECT rc,rcView;

				NotificationBar.GetPosition(&rc);
				pThis->m_ViewWindow.GetClientRect(&rcView);
				rc.right=rcView.right;
				NotificationBar.SetPosition(&rc);
			}
			if (!pThis->m_fFullscreen) {
				if (wParam==SIZE_MAXIMIZED)
					CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
										ViewOptions.GetMaximizeStretchMode());
				else if (wParam==SIZE_RESTORED)
					CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO);
			}

			StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
			pThis->CheckZoomMenu();
		}
		return 0;

	case WM_SIZING:
		{
			CMainWindow *pThis=GetThis(hwnd);
			LPRECT prc=reinterpret_cast<LPRECT>(lParam);
			RECT rcOld;
			bool fChanged=false;

			pThis->GetPosition(&rcOld);
			if (GetKeyState(VK_SHIFT)<0?!ViewOptions.GetAdjustAspectResizing():
										ViewOptions.GetAdjustAspectResizing()) {
				BYTE XAspect,YAspect;

				if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(
														&XAspect,&YAspect)) {
					RECT rcWindow,rcClient;
					int XMargin,YMargin,Width,Height;

					pThis->GetPosition(&rcWindow);
					pThis->GetClientRect(&rcClient);
					if (ViewOptions.GetClientEdge()) {
						rcClient.right-=GetSystemMetrics(SM_CXEDGE)*2;
						rcClient.bottom-=GetSystemMetrics(SM_CYEDGE)*2;
					}
					if (pThis->m_fShowStatusBar)
						rcClient.bottom-=StatusView.GetHeight();
					if (fShowPanelWindow && !PanelFrame.GetFloating()) {
						rcClient.right-=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
					}
					if (rcClient.right<=0 || rcClient.bottom<=0)
						goto SizingEnd;
					XMargin=(rcWindow.right-rcWindow.left)-rcClient.right;
					YMargin=(rcWindow.bottom-rcWindow.top)-rcClient.bottom;
					Width=(prc->right-prc->left)-XMargin;
					Height=(prc->bottom-prc->top)-YMargin;
					if (Width<=0 || Height<=0)
						goto SizingEnd;
					if (wParam==WMSZ_LEFT || wParam==WMSZ_RIGHT)
						Height=Width*YAspect/XAspect;
					else if (wParam==WMSZ_TOP || wParam==WMSZ_BOTTOM)
						Width=Height*XAspect/YAspect;
					else if (Width*YAspect<Height*XAspect)
						Width=Height*XAspect/YAspect;
					else if (Width*YAspect>Height*XAspect)
						Height=Width*YAspect/XAspect;
					if (wParam==WMSZ_LEFT || wParam==WMSZ_TOPLEFT
													|| wParam==WMSZ_BOTTOMLEFT)
						prc->left=prc->right-(Width+XMargin);
					else
						prc->right=prc->left+Width+XMargin;
					if (wParam==WMSZ_TOP || wParam==WMSZ_TOPLEFT
													|| wParam==WMSZ_TOPRIGHT)
						prc->top=prc->bottom-(Height+YMargin);
					else
						prc->bottom=prc->top+Height+YMargin;
					fChanged=true;
				}
			}
		SizingEnd:
			PanelStatus.OnOwnerMovingOrSizing(&rcOld,prc);
			if (fChanged)
				return TRUE;
		}
		break;

	case WM_GETMINMAXINFO:
		{
			CMainWindow *pThis=GetThis(hwnd);
			if (pThis==NULL)
				break;

			LPMINMAXINFO pmmi=reinterpret_cast<LPMINMAXINFO>(lParam);
			RECT rc;

			::SetRect(&rc,0,0,32,0);
			if (ViewOptions.GetClientEdge()) {
				rc.right+=GetSystemMetrics(SM_CXEDGE)*2;
				rc.bottom+=GetSystemMetrics(SM_CYEDGE)*2;
			}
			if (pThis->m_fShowStatusBar)
				rc.bottom+=StatusView.GetHeight();
			if (fShowPanelWindow && !PanelFrame.GetFloating())
				rc.right+=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
			::AdjustWindowRectEx(&rc,GetWindowStyle(hwnd),FALSE,GetWindowExStyle(hwnd));
			pmmi->ptMinTrackSize.x=rc.right-rc.left;
			pmmi->ptMinTrackSize.y=rc.bottom-rc.top;
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pThis->m_pFullscreen->OnRButtonDown();
			} else {
				/*
				POINT pt;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				::ClientToScreen(hwnd,&pt);
				pThis->PopupMenu(&pt);
				*/
				pThis->PopupMenu();
			}
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam!=HTCAPTION)
			break;
		ForegroundWindow(hwnd);
	case WM_LBUTTONDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);

			/*
			pThis->m_ptDragStartPos.x=GET_X_LPARAM(lParam);
			pThis->m_ptDragStartPos.y=GET_Y_LPARAM(lParam);
			ClientToScreen(hwnd,&pThis->m_ptDragStartPos);
			*/
			::GetCursorPos(&pThis->m_ptDragStartPos);
			::GetWindowRect(hwnd,&pThis->m_rcDragStart);
			::SetCapture(hwnd);
		}
		return 0;

	case WM_NCLBUTTONUP:
	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			::ReleaseCapture();
			TitleBarEventHandler.EndDrag();
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pThis->m_pFullscreen->OnMouseMove();
				return 0;
			}
			if (GetCapture()==hwnd) {
				POINT pt;
				RECT rcOld,rc;

				/*
				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				ClientToScreen(hwnd,&pt);
				*/
				::GetWindowRect(hwnd,&rcOld);
				::GetCursorPos(&pt);
				rc.left=pThis->m_rcDragStart.left+(pt.x-pThis->m_ptDragStartPos.x);
				rc.top=pThis->m_rcDragStart.top+(pt.y-pThis->m_ptDragStartPos.y);
				rc.right=rc.left+(pThis->m_rcDragStart.right-pThis->m_rcDragStart.left);
				rc.bottom=rc.top+(pThis->m_rcDragStart.bottom-pThis->m_rcDragStart.top);
				if (::GetKeyState(VK_SHIFT)<0?!ViewOptions.GetSnapAtWindowEdge():
											  ViewOptions.GetSnapAtWindowEdge())
					SnapWindow(hwnd,&rc);
				pThis->SetPosition(&rc);
				PanelStatus.OnOwnerMovingOrSizing(&rcOld,&rc);
			} else {
				POINT pt;
				RECT rc;

				::GetCursorPos(&pt);
				::ScreenToClient(hwnd,&pt);
				if (!pThis->m_fShowTitleBar) {
					pThis->m_ViewWindow.GetScreenPosition(&rc);
					MapWindowRect(NULL,hwnd,&rc);
					rc.bottom=TitleBar.GetHeight();
					if (::PtInRect(&rc,pt)) {
						if (!TitleBar.GetVisible()) {
							TCHAR szText[1024];

							::GetWindowText(hwnd,szText,lengthof(szText));
							TitleBar.SetLabel(szText);
							TitleBar.SetMaximizeMode(pThis->GetMaximize());
							TitleBar.SetPosition(&rc);
							TitleBar.SetVisible(true);
							::BringWindowToTop(TitleBar.GetHandle());
						}
					} else {
						if (TitleBar.GetVisible())
							TitleBar.SetVisible(false);
					}
				}
				if (!pThis->m_fShowStatusBar) {
					pThis->m_ViewWindow.GetScreenPosition(&rc);
					MapWindowRect(NULL,hwnd,&rc);
					rc.top=rc.bottom-StatusView.GetHeight();
					if (::PtInRect(&rc,pt)) {
						if (!StatusView.GetVisible()) {
							StatusView.SetPosition(&rc);
							StatusView.SetVisible(true);
							::BringWindowToTop(StatusView.GetHandle());
						}
					} else {
						if (StatusView.GetVisible())
							StatusView.SetVisible(false);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		GetThis(hwnd)->SendCommand(CM_FULLSCREEN);
		return 0;

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);
			int Command;

			if (wParam>=VK_F1 && wParam<=VK_F12) {
				if (!Accelerator.IsFunctionKeyChannelChange())
					break;
				Command=CM_CHANNELNO_FIRST+(wParam-VK_F1);
			} else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9) {
				if (!Accelerator.IsNumPadChannelChange())
					break;
				if (wParam==VK_NUMPAD0)
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+(wParam-VK_NUMPAD1);
			} else if (wParam>='0' && wParam<='9') {
				if (!Accelerator.IsDigitKeyChannelChange())
					break;
				if (wParam=='0')
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+(wParam-'1');
			} else {
				/*
				static const struct {
					WORD KeyCode;
					WORD Command;
				} CommandMap[] = {
					{'A',		CM_ASPECTRATIO},
					{'C',		CM_COPY},
					{'E',		CM_PROGRAMGUIDE},
					{'I',		CM_INFORMATION},
					{'M',		CM_VOLUME_MUTE},
					{'R',		CM_RECORD},
					{'S',		CM_SWITCHAUDIO},
					{'T',		CM_ALWAYSONTOP},
					{'V',		CM_SAVEIMAGE},
					{VK_HOME,	CM_ZOOM_100},
					{VK_UP,		CM_VOLUME_UP},
					{VK_DOWN,	CM_VOLUME_DOWN},
					{VK_LEFT,	CM_CHANNEL_DOWN},
					{VK_RIGHT,	CM_CHANNEL_UP},
				};
				int i;

				for (i=0;i<lengthof(CommandMap);i++) {
					if (CommandMap[i].KeyCode==wParam)
						break;
				}
				if (i==lengthof(CommandMap))
					break;
				Command=CommandMap[i].Command;
				*/
				break;
			}
			pThis->SendCommand(Command);
		}
		return 0;

	case WM_MOUSEWHEEL:
		GetThis(hwnd)->OnMouseWheel(wParam,lParam,StatusView.GetParent()==hwnd);
		return 0;

	case WM_MEASUREITEM:
		if (ChannelMenu.OnMeasureItem(hwnd,wParam,lParam))
			return TRUE;
		break;

	case WM_DRAWITEM:
		if (ChannelMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		break;

#if 0
// 枠を細くできないか試したが、Vistaで問題が多いのでやめた
	case WM_NCACTIVATE:
		if (!MainWindow.GetTitleBarVisible())
			return TRUE;
		break;

	case WM_NCCALCSIZE:
		if (wParam!=0 && !MainWindow.GetTitleBarVisible()) {
			NCCALCSIZE_PARAMS *pnccsp=reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

			pnccsp->rgrc[0].left+=2;
			pnccsp->rgrc[0].top+=2;
			pnccsp->rgrc[0].right-=2;
			pnccsp->rgrc[0].bottom-=2;
			return 0;
		}
		break;

	case WM_NCPAINT:
		if (!MainWindow.GetTitleBarVisible()) {
			HDC hdc=::GetDCEx(hwnd,(HRGN)wParam,DCX_WINDOW | DCX_INTERSECTRGN);
			RECT rc;

			::GetWindowRect(hwnd,&rc);
			::OffsetRect(&rc,-rc.left,-rc.top);
			::FillRect(hdc,&rc,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::ReleaseDC(hwnd,hdc);
			return 0;
		}
		break;
	case WM_NCHITTEST:
		if (!MainWindow.GetTitleBarVisible()) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;
			const int BorderWidth=::GetSystemMetrics(SM_CXSIZEFRAME);
			const int BorderHeight=::GetSystemMetrics(SM_CYSIZEFRAME);
			int Code=HTCLIENT;

			::GetWindowRect(hwnd,&rc);
			if (x>=rc.left && x<rc.left+BorderWidth) {
				if (y>=rc.top) {
					if (y<rc.top+BorderHeight)
						Code=HTTOPLEFT;
					else if (y<rc.bottom-BorderHeight)
						Code=HTLEFT;
					else if (y<rc.bottom)
						Code=HTBOTTOMLEFT;
				}
			} else if (x>=rc.right-BorderWidth && x<rc.right) {
				if (y>=rc.top) {
					if (y<rc.top+BorderHeight)
						Code=HTTOPRIGHT;
					else if (y<rc.bottom-BorderHeight)
						Code=HTRIGHT;
					else if (y<rc.bottom)
						Code=HTBOTTOMRIGHT;
				}
			} else if (y>=rc.top && y<rc.top+BorderHeight) {
				Code=HTTOP;
			} else if (y>=rc.bottom-BorderHeight && y<rc.bottom) {
				Code=HTBOTTOM;
			}
			return Code;
		}
		break;
#endif

	case WM_INITMENUPOPUP:
		{
			HMENU hmenu=reinterpret_cast<HMENU>(wParam);

			if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
				PluginList.SetMenu(hmenu);
				Accelerator.SetMenuAccel(hmenu);
				return 0;
			} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
				ChannelHistory.SetMenu(hmenu);
				//Accelerator.SetMenuAccel(hmenu);
				return 0;
			}
		}
		break;

	case WM_SYSCOMMAND:
		switch ((wParam&0xFFFFFFF0UL)) {
		case SC_MONITORPOWER:
			if (fNoMonitorLowPower)
				return TRUE;
			break;

		case SC_ABOUT:
			{
				CAboutDialog AboutDialog;

				AboutDialog.Show(hwnd);
			}
			return 0;

		case SC_MINIMIZE:
		case SC_MAXIMIZE:
		case SC_RESTORE:
			{
				CMainWindow *pThis=GetThis(hwnd);

				if (pThis->m_fFullscreen)
					pThis->SetFullscreen(false);
			}
			break;

		case SC_CLOSE:
			GetThis(hwnd)->SendCommand(CM_CLOSE);
			return 0;
		}
		break;

	case WM_APPCOMMAND:
		return GetThis(hwnd)->OnAppCommand(hwnd,(HWND)wParam,
					GET_APPCOMMAND_LPARAM(lParam),GET_DEVICE_LPARAM(lParam),
					GET_KEYSTATE_LPARAM(lParam));

	case WM_HOTKEY:
		{
			int Command=Accelerator.TranslateHotKey(wParam,lParam);

			if (Command>0)
				::PostMessage(hwnd,WM_COMMAND,Command,0);
		}
		return 0;

	case WM_POWERBROADCAST:
		if (wParam==PBT_APMSUSPEND) {
			CMainWindow *pThis=GetThis(hwnd);

			Logger.AddLog(TEXT("サスペンドへの移行メッセージを受信しました。"));
			pThis->m_RestoreChannelSpec.Store(&ChannelManager);
			pThis->CloseTuner();
		} else if (wParam==PBT_APMRESUMESUSPEND) {
			CMainWindow *pThis=GetThis(hwnd);

			Logger.AddLog(TEXT("サスペンドからの復帰メッセージを受信しました。"));
			pThis->OpenTuner();
		}
		break;

	case WM_APP_SERVICEUPDATE:
		{
			CServiceUpdateInfo *pInfo=reinterpret_cast<CServiceUpdateInfo*>(lParam);
			HMENU hmenu;
			int i;

			hmenu=MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE);
			ClearMenu(hmenu);
			for (i=0;i<pInfo->m_NumServices;i++) {
				TCHAR szServiceName[256];
				if (pInfo->m_pServiceList[i].szServiceName[0]!='\0') {
					::wsprintf(szServiceName,TEXT("&%d: %s"),
							   i+1,pInfo->m_pServiceList[i].szServiceName);
				} else {
					::wsprintf(szServiceName,TEXT("&%d: サービス%d"),i+1,i+1);
				}
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SERVICE_FIRST+i,szServiceName);
			}
			MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
									CM_SERVICE_FIRST+pInfo->m_NumServices-1,
									CM_SERVICE_FIRST+pInfo->m_CurService);

			hmenu=MainMenu.GetSubMenu(CMainMenu::SUBMENU_STEREOMODE);
			for (i=::GetMenuItemCount(hmenu)-1;i>=4;i--)
				::DeleteMenu(hmenu,i,MF_BYPOSITION);
			int NumAudioStreams=CoreEngine.m_DtvEngine.GetAudioStreamNum();
			for (i=0;i<NumAudioStreams;i++) {
				TCHAR szText[32];

				::wsprintf(szText,TEXT("&%d: 音声%d"),i+1,i+1);
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_AUDIOSTREAM_FIRST+i,szText);
			}
			MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
									CM_AUDIOSTREAM_FIRST+NumAudioStreams-1,
									CM_AUDIOSTREAM_FIRST+CoreEngine.m_DtvEngine.GetAudioStream());

			if (!AppMain.IsChannelScanning()
					&& pInfo->m_NumServices>0 && pInfo->m_CurService>=0) {
				const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
				WORD ServiceID,TransportStreamID;

				TransportStreamID=pInfo->m_TransportStreamID;
				ServiceID=pInfo->m_pServiceList[pInfo->m_CurService].ServiceID;
				if (TransportStreamID!=0 && ServiceID!=0
						&& !CoreEngine.IsNetworkDriver()
						&& (pChInfo==NULL
						|| (pChInfo->GetTransportStreamID()!=0
						&& pChInfo->GetTransportStreamID()!=TransportStreamID))) {
					// 外部からチャンネル変更されたか、
					// ドライバが開かれたときのデフォルトチャンネル
					Logger.AddLog(TEXT("チャンネル変更を検知しました。(TSID %d / SID %d)"),
								  TransportStreamID,ServiceID);
					AppMain.FollowChannelChange(TransportStreamID,ServiceID);
				} else if (pChInfo!=NULL && ServiceID!=0 && !CoreEngine.IsNetworkDriver()) {
					// サービスを選択する
					// チャンネル切り替え直後はまだPMTが来ていないので
					// サービスの選択ができないため
					WORD SID=0;

					if (ChannelManager.GetCurrentServiceID()!=0) {
						SID=ChannelManager.GetCurrentServiceID();
					} else if (ChannelManager.GetCurrentService()<0
							&& pChInfo->GetServiceID()!=0) {
						SID=pChInfo->GetServiceID();
					}
					if (SID!=0 && SID!=ServiceID) {
						if (AppMain.SetServiceByID(SID,&pInfo->m_CurService))
							ChannelManager.SetCurrentService(pInfo->m_CurService);
					}

					if (OSDOptions.GetShowOSD() && wParam!=0)
						GetThis(hwnd)->ShowChannelOSD();
				}
				if (pChInfo!=NULL && !CoreEngine.IsNetworkDriver()) {
					// チャンネルの情報を更新する
					// 古いチャンネル設定ファイルにはNIDとTSIDの情報が含まれていないため
					WORD NetworkID=pInfo->m_NetworkID;

					for (i=0;i<pInfo->m_NumServices;i++) {
						ServiceID=pInfo->m_pServiceList[i].ServiceID;
						if (ServiceID!=0) {
							ChannelManager.UpdateStreamInfo(
								pChInfo->GetSpace(),
								pChInfo->GetChannelIndex(),i,
								NetworkID,TransportStreamID,ServiceID);
						}
					}
				}
				PluginList.SendServiceUpdateEvent();
			}
			delete pInfo;
			if (pNetworkRemocon!=NULL)
				pNetworkRemocon->GetChannel(&GetChannelReciver);
		}
		return 0;

	case WM_APP_CHANNELCHANGE:
		{
			const CChannelList &List=pNetworkRemocon->GetChannelList();

			ChannelManager.SetNetworkRemoconCurrentChannel((int)wParam);
			MainMenu.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
				CM_CHANNELNO_FIRST+List.GetChannelNo(ChannelManager.GetNetworkRemoconCurrentChannel())-1);
			StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
		}
		return 0;

	/*
	case WM_APP_IMAGESAVE:
		{
			MessageBox(NULL,TEXT("画像の保存でエラーが発生しました。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
		}
		return 0;
	*/

	case WM_APP_TRAYICON:
		switch (lParam) {
		case WM_RBUTTONDOWN:
			{
				CMainWindow *pThis=GetThis(hwnd);
				POINT pt;
				HMENU hmenu;

				hmenu=::LoadMenu(hInst,MAKEINTRESOURCE(IDM_TRAY));
				::EnableMenuItem(hmenu,CM_SHOW,
					MF_BYCOMMAND | (pThis->m_fStandby || pThis->IsMinimizeToTray()?MFS_ENABLED:MFS_GRAYED));
				::GetCursorPos(&pt);
				// お約束が必要な理由は以下を参照
				// http://support.microsoft.com/kb/135788/en-us
				ForegroundWindow(hwnd);				// お約束
				::TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
				::PostMessage(hwnd,WM_NULL,0,0);	// お約束
				::DestroyMenu(hmenu);
			}
			break;

		case WM_LBUTTONDOWN:
			{
				CMainWindow *pThis=GetThis(hwnd);

				pThis->SendCommand(CM_SHOW);
			}
			break;
		}
		return 0;

	case WM_APP_EXECUTE:
		{
			CMainWindow *pThis=GetThis(hwnd);
			ATOM atom=(ATOM)wParam;
			TCHAR szCmdLine[256];

			if (pThis==NULL)
				return 0;
			if (atom!=0) {
				if (::GlobalGetAtomName(atom,szCmdLine,lengthof(szCmdLine))!=0)
					pThis->OnExecute(szCmdLine);
				::GlobalDeleteAtom(atom);
			}
		}
		return 0;

	case WM_APP_QUERYPORT:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis!=NULL && !pThis->m_fClosing && CoreEngine.IsNetworkDriver()) {
				WORD Port=ChannelManager.GetCurrentChannel()+
										(CoreEngine.IsUDPDriver()?1234:2230);
				WORD RemoconPort=pNetworkRemocon!=NULL?pNetworkRemocon->GetPort():0;
				return MAKELRESULT(Port,RemoconPort);
			}
		}
		return 0;

	case WM_APP_FILEWRITEERROR:
		GetThis(hwnd)->ShowErrorMessage(TEXT("ファイルへの書き出しでエラーが発生しました。"));
		return 0;

	case WM_DISPLAYCHANGE:
		CoreEngine.m_DtvEngine.m_MediaViewer.DisplayModeChanged();
		break;

	case WM_CLOSE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (!pThis->ConfirmExit())
				return 0;

			pThis->m_fClosing=true;

			::SetCursor(::LoadCursor(NULL,IDC_WAIT));

			/*
			StatusView.SetSingleText(TEXT("終了処理を行っています..."));
			if (!StatusView.GetVisible()) {
				RECT rc;

				pThis->m_ViewWindow.GetScreenPosition(&rc);
				MapWindowRect(NULL,hwnd,&rc);
				rc.top=rc.bottom-StatusView.GetHeight();
				StatusView.SetPosition(&rc);
				StatusView.SetVisible(true);
				StatusView.Update();
				::BringWindowToTop(StatusView.GetHandle());
			}
			*/

			//CoreEngine.m_DtvEngine.EnablePreview(false);

			::KillTimer(hwnd,TIMER_ID_UPDATE);

			if (fScreenSaverActive)
				::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			if (fPowerOffActiveOriginal)
				::SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			if (fLowPowerActiveOriginal)
				::SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

			SAFE_DELETE(pThis->m_pFullscreen);
			SAFE_DELETE(pNetworkRemocon);
			ResidentManager.Finalize();
			HtmlHelpClass.Finalize();
			MainMenu.Destroy();
			Accelerator.Finalize();
			HDUSController.Finalize();
			AppMain.SaveCurrentChannel();
			pThis->m_fMaximize=pThis->GetMaximize();

			pThis->ShowFloatingWindows(false);
		}
		break;

	case WM_DESTROY:
		{
			CMainWindow *pThis=GetThis(hwnd);

			CoreEngine.m_DtvEngine.SetTracer(&Logger);
			CoreEngine.Close();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(NULL);

			Logger.AddLog(TEXT("プラグインを開放しています..."));
			PluginOptions.StorePluginOptions();
			PluginList.FreePlugins();
			AppMain.Finalize();
			EpgOptions.SaveEpgFile(&EpgProgramList);
			EpgOptions.Finalize();

			pThis->OnDestroy();
			::PostQuitMessage(0);
		}
		return 0;

	HANDLE_MSG(hwnd,WM_COMMAND,GetThis(hwnd)->OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,GetThis(hwnd)->OnTimer);

	default:
		if (HDUSController.HandleMessage(hwnd,uMsg,wParam,lParam))
			return 0;
		if (ResidentManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CMainWindow::SetTitleText()
{
	const CChannelInfo *pInfo;

	if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		TCHAR szText[64];

		::lstrcpy(szText,TITLE_TEXT TEXT(" - "));
		::lstrcat(szText,pInfo->GetName());
		::SetWindowText(m_hwnd,szText);
		ResidentManager.SetTipText(szText);
	} else {
		::SetWindowText(m_hwnd,TITLE_TEXT);
		ResidentManager.SetTipText(TITLE_TEXT);
	}
}


void CMainWindow::SetWindowVisible()
{
	bool fRestore=false;

	if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
		ResidentManager.SetStatus(0,CResidentManager::STATUS_MINIMIZED);
		fRestore=true;
	}
	if (!GetVisible()) {
		SetVisible(true);
		ForegroundWindow(m_hwnd);
		Update();
		fRestore=true;
	} else if (::IsIconic(m_hwnd)) {
		::ShowWindow(m_hwnd,SW_RESTORE);
		Update();
		fRestore=true;
	} else {
		ForegroundWindow(m_hwnd);
	}
	if (m_fMinimizeInit) {
		// 最小化状態での起動後最初の表示
		ShowFloatingWindows(true);
		m_fMinimizeInit=false;
	}
	if (fRestore && !m_fStandby) {
		if (m_fEnablePreview)
			SetPreview(true);
	}
}


void CMainWindow::ShowFloatingWindows(bool fShow)
{
	if (fShowPanelWindow && PanelFrame.GetFloating()) {
		PanelFrame.SetPanelVisible(fShow);
		if (fShow)
			PanelFrame.Update();
	}
	if (fShowProgramGuide)
		ProgramGuide.SetVisible(fShow);
	if (fShowCaptureWindow)
		CaptureWindow.SetVisible(fShow);
}


bool CMainWindow::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (fStandby) {
			if (m_fEnablePreview)
				SetPreview(false);
			m_fRestoreFullscreen=m_fFullscreen;
			if (m_fFullscreen)
				SetFullscreen(false);
			ShowFloatingWindows(false);
			SetVisible(false);
			PluginList.SendStandbyEvent(true);
			m_RestoreChannelSpec.Store(&ChannelManager);
			if (EpgOptions.GetUpdateWhenStandby()
					&& !RecordManager.IsRecording()
					&& !CoreEngine.IsNetworkDriver())
				BeginProgramGuideUpdate();
			if (!RecordManager.IsRecording() && !m_fProgramGuideUpdating)
				CloseTuner();
		} else {
			SetWindowVisible();
			::SetCursor(LoadCursor(NULL,IDC_WAIT));
			if (m_fStandbyInit) {
				TCHAR szText[1024];

				if (CoreEngine.GetDriverFileName()[0]!='\0') {
					StatusView.SetSingleText(TEXT("ドライバの読み込み中..."));
					if (CoreEngine.LoadDriver()) {
						Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
						if (!CoreEngine.OpenDriver()) {
							Logger.AddLog(CoreEngine.GetLastErrorText());
							ShowErrorMessage(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
						}
					} else {
						::wsprintf(szText,TEXT("\"%s\" を読み込めません。"),CoreEngine.GetDriverFileName());
						Logger.AddLog(szText);
						ShowErrorMessage(szText);
					}
				}
				CoreEngine.m_DtvEngine.SetTracer(&StatusView);
				if (!CoreEngine.OpenBcasCard()) {
					CoreEngine.FormatLastErrorText(szText,lengthof(szText));
					Logger.AddLog(TEXT("カードリーダがオープンできません。"));
					ShowErrorMessage(szText);
				}
				BuildMediaViewer();
				CoreEngine.m_DtvEngine.SetTracer(NULL);
				StatusView.SetSingleText(NULL);
				AppMain.InitializeChannel();
				SetDisplayStatus();
				m_fStandbyInit=false;
			}
			if (m_fRestoreFullscreen)
				SetFullscreen(true);
			ShowFloatingWindows(true);
			ForegroundWindow(m_hwnd);
			PluginList.SendStandbyEvent(false);
			OpenTuner();
			if (m_fEnablePreview)
				SetPreview(true);
			::SetCursor(LoadCursor(NULL,IDC_ARROW));
		}
		m_fStandby=fStandby;
	}
	return true;
}


bool CMainWindow::InitStandby()
{
	m_fEnablePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
						&& (!ViewOptions.GetRestorePlayStatus() || fEnablePlay);
	m_fRestoreFullscreen=CmdLineParser.m_fFullscreen;
	if (CoreEngine.GetDriverFileName()[0]!='\0')
		m_fSrcFilterReleased=true;
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.Service);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(RestoreChannelInfo.Space);
				m_RestoreChannelSpec.SetChannel(Index);
				m_RestoreChannelSpec.SetService(RestoreChannelInfo.Service);
			}
		}
	}
	ResidentManager.SetResident(true);
	m_fStandby=true;
	m_fStandbyInit=true;
	return true;
}


bool CMainWindow::InitMinimize()
{
	m_fEnablePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
						&& (!ViewOptions.GetRestorePlayStatus() || fEnablePlay);
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.Service);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(RestoreChannelInfo.Space);
				m_RestoreChannelSpec.SetChannel(Index);
				m_RestoreChannelSpec.SetService(RestoreChannelInfo.Service);
			}
		}
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_MINIMIZED,
							  CResidentManager::STATUS_MINIMIZED);
	if (!ResidentManager.GetMinimizeToTray())
		::ShowWindow(m_hwnd,SW_SHOWMINNOACTIVE/*SW_SHOWMINIMIZED*/);
	m_fMinimizeInit=true;
	return true;
}


bool CMainWindow::IsMinimizeToTray() const
{
	return ResidentManager.GetMinimizeToTray()
		&& (ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0;
}


void CMainWindow::CloseTuner()
{
	if (CoreEngine.IsDriverOpen()) {
		CoreEngine.UnloadDriver();
		ChannelManager.SetCurrentChannel(ChannelManager.GetCurrentSpace(),-1);
		m_fSrcFilterReleased=true;
		Logger.AddLog(TEXT("ドライバを閉じました。"));
	}
}


bool CMainWindow::OpenTuner()
{
	bool fOK=true;
	bool fRestoreCh=m_fSrcFilterReleased || m_fProgramGuideUpdating;

	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(false);
	if (!CoreEngine.IsBcasCardOpen()) {
		if (!CoreEngine.OpenBcasCard()) {
			Logger.AddLog(TEXT("カードリーダがオープンできません。"));
		}
	}
	if (m_fSrcFilterReleased) {
		TCHAR szText[1024];

		CoreEngine.m_DtvEngine.SetTracer(&StatusView);
		if (!CoreEngine.IsDriverLoaded()) {
			if (!CoreEngine.LoadDriver()) {
				::wsprintf(szText,TEXT("\"%s\" を読み込めません。"),CoreEngine.GetDriverFileName());
				Logger.AddLog(szText);
				ShowErrorMessage(szText);
				fOK=false;
			}
		}
		if (fOK && !CoreEngine.OpenDriver()) {
			Logger.AddLog(CoreEngine.GetLastErrorText());
			ShowErrorMessage(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
			fOK=false;
		}
		m_fSrcFilterReleased=false;
		CoreEngine.m_DtvEngine.SetTracer(NULL);
		StatusView.SetSingleText(NULL);
	}
	if (fOK && fRestoreCh) {
		AppMain.SetChannel(m_RestoreChannelSpec.GetSpace(),
						   m_RestoreChannelSpec.GetChannel(),
						   m_RestoreChannelSpec.GetService());
	}
	return fOK;
}


bool CMainWindow::ConfirmExit()
{
	return RecordOptions.ConfirmExit(GetVideoHostWindow(),&RecordManager);
}


bool CMainWindow::CommandLineRecord(LPCTSTR pszFileName,DWORD Delay,DWORD Duration)
{
	CRecordManager::TimeSpecInfo StartTime,StopTime;

	if (Delay>0) {
		StartTime.Type=CRecordManager::TIME_DURATION;
		StartTime.Time.Duration=Delay*1000;
	} else {
		StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
	}
	if (Duration>0) {
		StopTime.Type=CRecordManager::TIME_DURATION;
		StopTime.Time.Duration=Duration*1000;
	} else {
		StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
	}
	return AppMain.StartRecord(pszFileName[0]!='\0'?pszFileName:NULL,
								&StartTime,&StopTime);
}


static bool SetCommandLineChannel(const CCommandLineParser *pCmdLine)
{
	if (ChannelManager.IsNetworkRemoconMode()) {
		if (pCmdLine->m_ControllerChannel==0)
			return false;
		if (ChannelManager.GetCurrentChannelList()->FindChannelNo(pCmdLine->m_ControllerChannel)>=0) {
			MainWindow.SendCommand(CM_CHANNELNO_FIRST+pCmdLine->m_ControllerChannel-1);
			return true;
		}
		return false;
	}

	const CChannelList *pChannelList;
	int i,j;

	for (i=0;(pChannelList=ChannelManager.GetChannelList(i))!=NULL;i++) {
		if (pCmdLine->m_TuningSpace>=0 && i!=pCmdLine->m_TuningSpace)
			continue;
		for (j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

			if ((pCmdLine->m_Channel==0
					|| pCmdLine->m_Channel==pChannelInfo->GetChannel())
				&& (pCmdLine->m_ControllerChannel==0
					|| pCmdLine->m_ControllerChannel==pChannelInfo->GetChannelNo())
				&& (pCmdLine->m_ServiceID==0
					|| pCmdLine->m_ServiceID==pChannelInfo->GetServiceID())
				&& (pCmdLine->m_NetworkID==0
					|| pCmdLine->m_NetworkID==pChannelInfo->GetNetworkID())
				&& (pCmdLine->m_TransportStreamID==0
					|| pCmdLine->m_TransportStreamID==pChannelInfo->GetTransportStreamID())) {
				return AppMain.SetChannel(i,j);
			}
		}
	}
	return false;
}


bool CMainWindow::OnExecute(LPCTSTR pszCmdLine)
{
	CCommandLineParser CmdLine;

	CmdLine.Parse(pszCmdLine);
	if (CmdLine.m_fSilent)
		CmdLineParser.m_fSilent=true;
	if (CmdLine.m_fSaveLog)
		CmdLineParser.m_fSaveLog=true;
	SendCommand(CM_SHOW);
	if (CmdLine.m_fFullscreen)
		SetFullscreen(true);
	if (CmdLine.m_szDriverName[0]!='\0')
		AppMain.SetDriver(CmdLine.m_szDriverName);
	if (CmdLine.IsChannelSpecified())
		SetCommandLineChannel(&CmdLine);
	if (CmdLine.m_fRecord) {
		if (CmdLine.m_fRecordCurServiceOnly)
			CmdLineParser.m_fRecordCurServiceOnly=true;
		CommandLineRecord(CmdLine.m_szRecordFileName,
						  CmdLine.m_RecordDelay,CmdLine.m_RecordDuration);
	} else if (CmdLine.m_fRecordStop)
		SendCommand(CM_RECORD_STOP);
	return true;
}


bool CMainWindow::BeginProgramGuideUpdate()
{
	if (!m_fProgramGuideUpdating) {
		if (m_fStandby && m_fSrcFilterReleased) {
			if (!OpenTuner())
				return false;
		}
		m_fProgramGuideUpdating=true;
		EnablePreview(false);
		SendCommand(CM_CHANNEL_UP);
		const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();
		if (pChInfo==NULL)
			return false;
		m_ProgramGuideUpdateStartChannel=pChInfo->GetChannelIndex();
		::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,m_fStandby?40000:20000,NULL);
	}
	return true;
}


void CMainWindow::EndProgramGuideUpdate(bool fRelease/*=true*/)
{
	if (m_fProgramGuideUpdating) {
		::KillTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE);
		EpgProgramList.UpdateProgramList();
		EpgOptions.SaveEpgFile(&EpgProgramList);
		m_fProgramGuideUpdating=false;
		if (m_fStandby) {
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			if (fRelease)
				CloseTuner();
			m_fEnablePreview=true;	// 復帰時にプレビューを再開させる
		} else {
			EnablePreview(true);
		}
	}
}


void CMainWindow::BeginProgramListUpdateTimer()
{
	SetTimer(m_hwnd,TIMER_ID_PROGRAMLISTUPDATE,5000,NULL);
	ProgramListUpdateTimerCount=0;
}


bool CMainWindow::SetLogo(LPCTSTR pszFileName)
{
	if (pszFileName==NULL || pszFileName[0]=='\0')
		return m_ViewWindow.SetLogo(NULL);

	TCHAR szFileName[MAX_PATH];

	if (::PathIsFileSpec(pszFileName)) {
		AppMain.GetAppDirectory(szFileName);
		::PathAppend(szFileName,pszFileName);
	} else {
		::lstrcpy(szFileName,pszFileName);
	}
	HBITMAP hbm=static_cast<HBITMAP>(::LoadImage(NULL,szFileName,IMAGE_BITMAP,
								0,0,LR_LOADFROMFILE | LR_CREATEDIBSECTION));
	if (hbm==NULL)
		return false;
	return m_ViewWindow.SetLogo(hbm);
}


bool CMainWindow::SetViewWindowEdge(bool fEdge)
{
	DWORD ExStyle;

	ExStyle=m_ViewWindow.GetExStyle();
	if (((ExStyle&WS_EX_CLIENTEDGE)!=0)!=fEdge)
		m_ViewWindow.SetExStyle(ExStyle^WS_EX_CLIENTEDGE,true);
	return true;
}




#include <vector>

class CPortQuery {
	std::vector<WORD> m_UDPPortList;
	std::vector<WORD> m_RemoconPortList;
	static BOOL CALLBACK EnumProc(HWND hwnd,LPARAM lParam);
public:
	bool Query(WORD *pUDPPort,WORD MaxPort,WORD *pRemoconPort);
};

bool CPortQuery::Query(WORD *pUDPPort,WORD MaxPort,WORD *pRemoconPort)
{
	size_t i;

	m_UDPPortList.clear();
	m_RemoconPortList.clear();
	::EnumWindows(EnumProc,reinterpret_cast<LPARAM>(this));
	if (m_UDPPortList.size()>0) {
		WORD UDPPort;

		for (UDPPort=*pUDPPort;UDPPort<=MaxPort;UDPPort++) {
			for (i=0;i<m_UDPPortList.size();i++) {
				if (m_UDPPortList[i]==UDPPort)
					break;
			}
			if (i==m_UDPPortList.size())
				break;
		}
		if (UDPPort>MaxPort)
			UDPPort=0;
		*pUDPPort=UDPPort;
	}
	if (m_RemoconPortList.size()>0) {
		WORD RemoconPort;

		for (RemoconPort=*pRemoconPort;;RemoconPort++) {
			for (i=0;i<m_RemoconPortList.size();i++) {
				if (m_RemoconPortList[i]==RemoconPort)
					break;
			}
			if (i==m_RemoconPortList.size())
				break;
		}
		*pRemoconPort=RemoconPort;
	}
	return true;
}

BOOL CALLBACK CPortQuery::EnumProc(HWND hwnd,LPARAM lParam)
{
	TCHAR szClass[64];

	if (hwnd==MainWindow.GetHandle())
		return TRUE;
	if (::GetClassName(hwnd,szClass,lengthof(szClass))>0
			&& ::lstrcmpi(szClass,MAIN_WINDOW_CLASS)==0) {
		CPortQuery *pThis=reinterpret_cast<CPortQuery*>(lParam);
		DWORD_PTR Result;

		if (::SendMessageTimeout(hwnd,WM_APP_QUERYPORT,0,0,
								 SMTO_NORMAL | SMTO_ABORTIFHUNG,1000,&Result)) {
			WORD UDPPort=LOWORD(Result),RemoconPort=HIWORD(Result);

			TRACE(TEXT("CPortQuery::EnumProc %d %d\n"),UDPPort,RemoconPort);
			pThis->m_UDPPortList.push_back(UDPPort);
			if (RemoconPort>0)
				pThis->m_RemoconPortList.push_back(RemoconPort);
			Logger.AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました。(UDPポート %d / リモコンポート %d)"),UDPPort,RemoconPort);
		}
	}
	return TRUE;
}




class CAppMutex {
	HANDLE m_hMutex;
	bool m_fAlreadyExists;
public:
	CAppMutex(bool fEnable);
	~CAppMutex();
	bool AlreadyExists() const { return m_fAlreadyExists; }
};

CAppMutex::CAppMutex(bool fEnable)
{
	if (fEnable) {
		TCHAR szName[MAX_PATH];

		::GetModuleFileName(hInst,szName,lengthof(szName));
		::CharUpperBuff(szName,::lstrlen(szName));
		for (int i=0;szName[i]!='\0';i++) {
			if (szName[i]=='\\')
				szName[i]=':';
		}
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;
		::ZeroMemory(&sd,sizeof(sd));
		::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
		::ZeroMemory(&sa,sizeof(sa));
		sa.nLength=sizeof(sa);
		sa.lpSecurityDescriptor=&sd;
		m_hMutex=CreateMutex(&sa,FALSE,szName);
		m_fAlreadyExists=m_hMutex!=NULL && GetLastError()==ERROR_ALREADY_EXISTS;
	} else {
		m_hMutex=NULL;
		m_fAlreadyExists=false;
	}
}

CAppMutex::~CAppMutex()
{
	if (m_hMutex!=NULL) {
		/*
		if (!m_fAlreadyExists)
			ReleaseMutex(m_hMutex);
		*/
		CloseHandle(m_hMutex);
	}
}


struct FindWindowInfo {
	LPCTSTR pszFileName;
	HWND hwndFirst;
	HWND hwndFind;
};

static BOOL CALLBACK FindWindowCallback(HWND hwnd,LPARAM lParam)
{
	FindWindowInfo *pInfo=reinterpret_cast<FindWindowInfo*>(lParam);
	TCHAR szClassName[64],szFileName[MAX_PATH];

	if (::GetClassName(hwnd,szClassName,lengthof(szClassName))>0
			&& ::lstrcmpi(szClassName,MAIN_WINDOW_CLASS)==0) {
		if (pInfo->hwndFirst==NULL)
			pInfo->hwndFirst=hwnd;
		if (::GetModuleFileName(
				reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hwnd,GWLP_HINSTANCE)),
				szFileName,lengthof(szFileName))>0
				&& ::lstrcmpi(szFileName,pInfo->pszFileName)==0) {
			pInfo->hwndFind=hwnd;
			return FALSE;
		}
	}
	return TRUE;
}


#ifndef DEBUG
LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
	if (CmdLineParser.m_fSilent)
		return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif


int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
												LPTSTR pszCmdLine,int nCmdShow)
{
#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	SetUnhandledExceptionFilter(ExceptionFilter);
#endif

	Logger.AddLog(TEXT("******** 起動 ********"));

	hInst=hInstance;

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

	if (pszCmdLine[0]!='\0')
		CmdLineParser.Parse(pszCmdLine);

	AppMain.Initialize();

	//CAppMutex Mutex(fKeepSingleTask || CmdLineParser.m_fSchedule);
	CAppMutex Mutex(true);

	// 複数起動のチェック
	if (Mutex.AlreadyExists()
			&& (fKeepSingleTask || CmdLineParser.m_fSchedule)) {
		TCHAR szFileName[MAX_PATH];
		FindWindowInfo Info;

		::GetModuleFileName(hInst,szFileName,lengthof(szFileName));
		Info.pszFileName=szFileName;
		Info.hwndFirst=NULL;
		Info.hwndFind=NULL;
		::EnumWindows(FindWindowCallback,reinterpret_cast<LPARAM>(&Info));
		HWND hwnd=Info.hwndFind!=NULL?Info.hwndFind:Info.hwndFirst;
		if (::IsWindow(hwnd)) {
			ATOM atom;

			if (pszCmdLine[0]!='\0')
				atom=::GlobalAddAtom(pszCmdLine);
			else
				atom=0;
			::PostMessage(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0);
			return FALSE;
		} else if (!CmdLineParser.m_fSchedule) {
			// 固まった場合でも WinMain は抜けるのでこれは無意味...
			if (!CmdLineParser.m_fSilent)
				MessageBox(NULL,
					APP_NAME TEXT(" は既に起動しています。\r\n")
					TEXT("ウィンドウが見当たらない場合はタスクマネージャに隠れていますので\r\n")
					TEXT("強制終了させてください。"),TITLE_TEXT,
												MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	{
		INITCOMMONCONTROLSEX iccex;

		iccex.dwSize=sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC=ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
		InitCommonControlsEx(&iccex);
	}

	{
		TCHAR szDirectory[MAX_PATH];

		AppMain.GetAppDirectory(szDirectory);
		DriverManager.Find(szDirectory);
	}
	DriverOptions.Initialize(&DriverManager);

	if (CmdLineParser.m_fInitialSettings
			|| (AppMain.IsFirstExecute() && CmdLineParser.m_szDriverName[0]=='\0')) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.ShowDialog(NULL))
			return FALSE;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		InitialSettings.GetMpeg2DecoderName(szMpeg2DecoderName,lengthof(szMpeg2DecoderName));
		VideoRendererType=InitialSettings.GetVideoRenderer();
		CoreEngine.SetCardReaderType(InitialSettings.GetCardReader());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	}

	ColorSchemeOptions.SetApplyCallback(ColorSchemeApplyProc);
	ColorSchemeOptions.ApplyColorScheme();

	CMainWindow::Initialize();
	CViewWindow::Initialize(hInst);
	CVideoContainerWindow::Initialize(hInst);
	CStatusView::Initialize(hInst);
	CSplitter::Initialize(hInst);
	CTitleBar::Initialize(hInst);
	CPanelFrame::Initialize(hInst);
	CInfoPanel::Initialize(hInst);
	CInformation::Initialize(hInst);
	CProgramListView::Initialize(hInst);
	CChannelPanel::Initialize(hInst);
	CControlPanel::Initialize(hInst);
	CProgramGuide::Initialize(hInst);
	CCaptureWindow::Initialize(hInst);
	CPseudoOSD::Initialize(hInst);
	CNotificationBar::Initialize(hInst);

	if (!MainWindow.Create(NULL,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		if (!CmdLineParser.m_fSilent)
			MessageBox(NULL,TEXT("ウィンドウが作成できません。"),NULL,
														MB_OK | MB_ICONSTOP);
		return FALSE;
	}
	if (nCmdShow==SW_SHOWMINIMIZED || nCmdShow==SW_MINIMIZE)
		CmdLineParser.m_fMinimize=true;
	if (CmdLineParser.m_fStandby && CmdLineParser.m_fMinimize)
		CmdLineParser.m_fMinimize=false;
	if (!CmdLineParser.m_fStandby && !CmdLineParser.m_fMinimize) {
		MainWindow.Show(nCmdShow);
		MainWindow.Update();
	} else {
		::ShowWindow(MainWindow.GetHandle(),SW_HIDE);
	}
	if (!MainWindow.GetTitleBarVisible()) {
		// この段階でスタイルを変えないとおかしくなる
		// 最初からこのスタイルにしてもキャプションが表示される
		// ShowWindow の前に入れると、タイトルバーを表示させてもアイコンが出ない
		MainWindow.SetStyle(MainWindow.GetStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	ResidentManager.Initialize(MainWindow.GetHandle());
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	if (CmdLineParser.m_fMinimize)
		MainWindow.InitMinimize();

	if (ViewOptions.GetShowLogo() && ViewOptions.GetLogoFileName()[0]!='\0')
		MainWindow.SetLogo(ViewOptions.GetLogoFileName());

	CoreEngine.SetDriverFileName(CmdLineParser.m_szDriverName[0]!='\0'?
								CmdLineParser.m_szDriverName:szDriverFileName);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	if (!CmdLineParser.m_fNoDriver && !CmdLineParser.m_fStandby) {
		if (CoreEngine.GetDriverFileName()[0]!='\0') {
			StatusView.SetSingleText(TEXT("ドライバの読み込み中..."));
			if (CoreEngine.LoadDriver()) {
				Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
			} else {
				TCHAR szMessage[MAX_PATH+64];

				wsprintf(szMessage,TEXT("\"%s\" を読み込めません。"),CoreEngine.GetDriverFileName());
				Logger.AddLog(szMessage);
				if (!CmdLineParser.m_fSilent)
					MainWindow.ShowErrorMessage(szMessage);
			}
		} else {
			Logger.AddLog(TEXT("BonDriverが設定されていません。"));
			if (!CmdLineParser.m_fSilent)
				MainWindow.ShowMessage(
						TEXT("設定で使用するドライバを指定してください。"),
						TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
		}
	}

	CoreEngine.SetDescramble(!CmdLineParser.m_fNoDescramble);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(fDescrambleCurServiceOnly);
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	//CoreEngine.m_DtvEngine.m_MediaViewer.SetGrabber(fUseGrabberFilter);
	CoreEngine.BuildDtvEngine(&DtvEngineHandler);

	if (!CmdLineParser.m_fStandby && !CoreEngine.OpenBcasCard()) {
		if (!CmdLineParser.m_fSilent) {
			TCHAR szText[1024];

			wsprintf(szText,TEXT("%s\r\n利用可能なカードリーダを検索しますか?"),
					CoreEngine.GetLastErrorText());
			if (MainWindow.ShowMessage(szText,NULL,MB_YESNO | MB_ICONEXCLAMATION)==IDYES) {
				CCardReader::ReaderType CurReader=CoreEngine.GetCardReaderType();

				if (!CoreEngine.SetCardReaderType(
						CurReader==CCardReader::READER_SCARD?
						CCardReader::READER_HDUS:CCardReader::READER_SCARD)
						&& !CoreEngine.SetCardReaderType(CurReader)) {
					Logger.AddLog(TEXT("カードリーダがオープンできません。"));
					MainWindow.ShowErrorMessage(
						TEXT("利用可能なカードリーダが見付かりませんでした。"));
				}
			}
		} else {
			Logger.AddLog(TEXT("カードリーダがオープンできません。"));
		}
	}

	if (CoreEngine.IsBcasCardOpen()) {
		LPCTSTR pszName=CoreEngine.m_DtvEngine.m_TsDescrambler.GetCardReaderName();

		if (pszName!=NULL) {
			TCHAR szCardID[32];

			CoreEngine.m_DtvEngine.m_TsDescrambler.FormatBcasCardID(szCardID,lengthof(szCardID));
			Logger.AddLog(TEXT("カードリーダ \"%s\" をオープンしました"),pszName);
			Logger.AddLog(TEXT("(B-CASカードID %s / メーカ識別 %c / バージョン %d)"),
				szCardID,
				CoreEngine.m_DtvEngine.m_TsDescrambler.GetBcasCardManufacturerID(),
				CoreEngine.m_DtvEngine.m_TsDescrambler.GetBcasCardVersion());
		}
	}

	if (!CmdLineParser.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];

		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		AppMain.GetAppDirectory(szPluginDir);
		::PathAppend(szPluginDir,TEXT("Plugins"));
		PluginList.LoadPlugins(szPluginDir);
		PluginOptions.RestorePluginOptions();
	}

	if (!CmdLineParser.m_fStandby && !CmdLineParser.m_fNoDirectShow) {
		MainWindow.BuildMediaViewer();
	}

	EpgOptions.InitializeEpgDataCap();
	EpgOptions.AsyncLoadEpgData();

	if (CoreEngine.IsDriverLoaded() && !CoreEngine.OpenDriver()) {
		Logger.AddLog(CoreEngine.GetLastErrorText());
		if (!CmdLineParser.m_fSilent) {
			MainWindow.ShowErrorMessage(&CoreEngine,
								TEXT("BonDriverの初期化ができません。"));
		}
	}

	CoreEngine.m_DtvEngine.m_MediaViewer.SetIgnoreDisplayExtension(ViewOptions.GetIgnoreDisplayExtension());
	CoreEngine.SetDownMixSurround(AudioOptions.GetDownMixSurround());
	if (AudioOptions.GetRestoreMute() && fMuteStatus)
		MainWindow.SetMute(true);
	if ((!ViewOptions.GetRestorePlayStatus() || fEnablePlay)
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!CmdLineParser.m_fNoView && !CmdLineParser.m_fMinimize)
			MainWindow.EnablePreview(true);
	}

	if (CoreEngine.IsNetworkDriver()) {
		if (fIncrementUDPPort) {
			CPortQuery PortQuery;
			WORD UDPPort=CmdLineParser.m_UDPPort>0?(WORD)CmdLineParser.m_UDPPort:
											CoreEngine.IsUDPDriver()?1234:2230;
			WORD RemoconPort=NetworkRemoconOptions.GetPort();

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(&UDPPort,CoreEngine.IsUDPDriver()?1243:2239,&RemoconPort);
			CmdLineParser.m_UDPPort=UDPPort;
			NetworkRemoconOptions.SetTempPort(RemoconPort);
		}
		if (CmdLineParser.m_fUseNetworkRemocon)
			NetworkRemoconOptions.SetTempEnable(true);
	}

	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	AppMain.InitializeChannel();

	if (!CmdLineParser.m_fStandby)
		SetDisplayStatus();

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	if (!MainWindow.GetStatusBarVisible())
		StatusView.SetVisible(false);
	StatusView.SetSingleText(NULL);

	EpgProgramList.SetEpgDataCapDllUtil(
			CoreEngine.m_DtvEngine.m_TsPacketParser.GetEpgDataCapDllUtil());

	InfoPanel.SetEventHandler(&InfoPanelEventHandler);
	InfoPanel.Create(MainWindow.GetHandle(),WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	InfoWindow.Create(InfoPanel.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	InfoPanel.AddWindow(&InfoWindow,TEXT("情報"));

	ProgramListView.SetEpgProgramList(&EpgProgramList);
	ProgramListView.Create(InfoPanel.GetHandle(),WS_CHILD | WS_VSCROLL);
	InfoPanel.AddWindow(&ProgramListView,TEXT("番組表"));
	MainWindow.BeginProgramListUpdateTimer();

	ChannelPanel.SetEpgProgramList(&EpgProgramList);
	ChannelPanel.SetEventHandler(&ChannelPanelEventHandler);
	ChannelPanel.Create(InfoPanel.GetHandle(),WS_CHILD | WS_VSCROLL);
	InfoPanel.AddWindow(&ChannelPanel,TEXT("チャンネル"));
	::SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_CHANNELPANELUPDATE,5000,NULL);

	ControlPanel.SetSendMessageWindow(MainWindow.GetHandle());
	{
		const CChannelList *pList=ChannelManager.GetCurrentChannelList();
		int FontHeight=ControlPanel.GetFontHeight();
		int ButtonSize;
		int i;
		TCHAR szText[4];

		ButtonSize=FontHeight+8;
		for (i=0;i<12;i++) {
			CControlPanelButton *pItem;
			bool fEnable;

			wsprintf(szText,TEXT("%d"),i+1);
			pItem=new CControlPanelButton(CM_CHANNELNO_FIRST+i,szText,
					i%6*ButtonSize+4,i/6*ButtonSize+4,ButtonSize,ButtonSize);
			if (pList!=NULL) {
				fEnable=pList->FindChannelNo(i+1)>=0;
			} else {
				fEnable=false;
			}
			pItem->SetEnable(fEnable);
			ControlPanel.AddItem(pItem);
		}
		ControlPanel.AddItem(new CVolumeControlItem);
		ControlPanel.AddItem(new COptionsControlItem);
	}
	ControlPanel.Create(InfoPanel.GetHandle(),WS_CHILD);
	InfoPanel.AddWindow(&ControlPanel,TEXT("操作"));

	InfoPanel.SetCurTab(InfoPanelCurTab);
	Splitter.SetPane(PanelPaneIndex,NULL,PANE_ID_PANEL);
	Splitter.SetMinSize(PANE_ID_PANEL,64);
	Splitter.SetFixedPane(PANE_ID_PANEL);
	PanelFrame.Create(MainWindow.GetHandle(),&Splitter,PANE_ID_PANEL,&InfoPanel,TEXT("パネル"));
	PanelFrame.SetEventHandler(&PanelStatus);
	if (fShowPanelWindow
			&& (!PanelFrame.GetFloating()
				|| (!CmdLineParser.m_fStandby && !CmdLineParser.m_fMinimize))) {
		PanelFrame.SetPanelVisible(true,true);
		PanelFrame.Update();
	}

	/*
	if (EpgOptions.LoadEpgFile(&EpgProgramList))
		Logger.AddLog(TEXT("EPGデータを \"%s\" から読み込みました"),EpgOptions.GetEpgFileName());
	*/
	EpgOptions.AsyncLoadEpgFile(&EpgProgramList);

	ProgramGuide.SetEpgProgramList(&EpgProgramList);
	ProgramGuide.SetEventHandler(&ProgramGuideEventHandler);

	CaptureWindow.SetEventHandler(&CaptureWindowEventHandler);

	CommandList.Initialize(&DriverManager,&PluginList);
	Accelerator.Initialize(MainWindow.GetHandle(),&MainMenu,
						   AppMain.GetIniFileName(),&CommandList);
	HDUSController.Initialize(MainWindow.GetHandle(),
							  AppMain.GetIniFileName(),&CommandList);

	if (CoreEngine.m_DtvEngine.IsSrcFilterOpen()) {
		if (CoreEngine.IsBuildComplete()) {
			if (CmdLineParser.m_fFullscreen)
				MainWindow.SetFullscreen(true);
		}

		if (CoreEngine.IsNetworkDriver()) {
			const int FirstPort=CoreEngine.IsUDPDriver()?1234:2230;
			int Port=FirstPort;
			if ((int)CmdLineParser.m_UDPPort>FirstPort && (int)CmdLineParser.m_UDPPort<FirstPort+10)
				Port=CmdLineParser.m_UDPPort;
			else if (RestoreChannelInfo.Channel>=0 && RestoreChannelInfo.Channel<10)
				Port=FirstPort+RestoreChannelInfo.Channel;
			if (Port!=FirstPort)
				MainWindow.SendCommand(CM_CHANNEL_FIRST+(Port-FirstPort));
			if (CmdLineParser.m_ControllerChannel>0)
				SetCommandLineChannel(&CmdLineParser);
		} else if (AppMain.IsFirstExecute()) {
			if (ChannelManager.GetFileAllChannelList()->NumChannels()==0) {
				if (MainWindow.ShowMessage(
						TEXT("最初にチャンネルスキャンを行うことをおすすめします。\r\n")
						TEXT("今すぐチャンネルスキャンを行いますか?"),
						TEXT("チャンネルスキャンの確認"),
						MB_YESNO | MB_ICONQUESTION)==IDYES) {
					OptionDialog.ShowDialog(MainWindow.GetHandle(),
											COptionDialog::PAGE_CHANNELSCAN);
				}
			}
		} else if (CmdLineParser.IsChannelSpecified()) {
			SetCommandLineChannel(&CmdLineParser);
		} else if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
			const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
			if (pList!=NULL) {
				int Index=pList->Find(RestoreChannelInfo.Space,
									  RestoreChannelInfo.Channel,
									  RestoreChannelInfo.Service);
				if (Index>=0)
					AppMain.SetChannel(RestoreChannelInfo.Space,Index);
			}
		} else {
			// 初期チャンネルに設定する
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i=pList->Find(
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace(),
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel(),0);

			if (i>=0)
				MainWindow.SendCommand(CM_CHANNEL_FIRST+i);
		}

		if (CmdLineParser.m_fRecord)
			MainWindow.CommandLineRecord(CmdLineParser.m_szRecordFileName,
				CmdLineParser.m_RecordDelay,CmdLineParser.m_RecordDuration);
	}
	if (CmdLineParser.m_fStandby)
		MainWindow.InitStandby();
	if (CmdLineParser.m_fExitOnRecordEnd)
		MainWindow.SendCommand(CM_EXITONRECORDINGSTOP);

	MSG msg;

	while (GetMessage(&msg,NULL,0,0)) {
		if (HtmlHelpClass.PreTranslateMessage(&msg))
			continue;
		if (!Accelerator.TranslateMessage(MainWindow.GetHandle(),&msg)
				&& !HDUSController.TranslateMessage(MainWindow.GetHandle(),&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CoUninitialize();

	Logger.AddLog(TEXT("******** 終了 ********"));
	if (CmdLineParser.m_fSaveLog && !Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		Logger.GetDefaultLogFileName(szFileName);
		Logger.SaveToFile(szFileName,true);
	}

	return msg.wParam;
}

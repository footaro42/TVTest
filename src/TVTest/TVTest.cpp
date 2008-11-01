#include "stdafx.h"
#include <shlwapi.h>
#include "DtvEngine.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CoreEngine.h"
#include "MainWindow.h"
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
#include "ControlPanel.h"
#include "Splitter.h"
#include "RemoteController.h"
#include "NetworkRemocon.h"
#include "Record.h"
#include "Capture.h"
#include "StatusOptions.h"
#include "PanelOptions.h"
#include "ColorScheme.h"
#include "CaptureOptions.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "InitialSettings.h"
#include "Settings.h"
#include "CommandLine.h"
#include "Logger.h"
#include "Help.h"
#include "Image.h"
#include "MiscDialog.h"
#include "DrawUtil.h"
#include "PseudoOSD.h"
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

#define PANEL_TAB_INFORMATION	0
#define PANEL_TAB_PROGRAMLIST	1
#define PANEL_TAB_CONTROL		2

// 以下グローバル変数使いまくり
// 改善中...

static HINSTANCE hInst;
static CAppMain AppMain;
static CCoreEngine CoreEngine;
static CMainWindow MainWindow;
static CVideoContainerWindow VideoContainerWindow;
static CViewWindow ViewWindow;
static CStatusView StatusView;
static CSplitter Splitter;
static CMainMenu MainMenu;
static HACCEL hAccel=NULL;
static CHtmlHelp HtmlHelpClass;

static TCHAR szDriverFileName[MAX_PATH];
static bool fNewHDUSDriver=true;
static bool fIncrementUDPPort=true;

static CCommandLineParser CmdLineParser;

static CChannelManager ChannelManager;
static CRemoteController *pRemoteController=NULL;
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

static CStatusOptions StatusOptions(&StatusView);
static CPanelOptions PanelOptions;
static CColorSchemeOptions ColorSchemeOptions;
static CRecordOptions RecordOptions;
static CRecordManager RecordManager;
static CCaptureOptions CaptureOptions;
static CChannelScan ChannelScan(&CoreEngine);
static CEpgOptions EpgOptions(&CoreEngine);
static CProgramGuideOptions ProgramGuideOptions(&ProgramGuide);
static CNetworkRemoconOptions NetworkRemoconOptions;
static CLogger Logger;

static TCHAR szMpeg2DecoderName[128];
static CVideoRenderer::RendererType VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
static bool fDescrambleCurServiceOnly=true;
static bool fKeepSingleTask=false;
static bool fNoKeyHook=false;
static bool fNoRemoteController=false;
static bool fNoScreenSaver=false;
static BOOL fScreenSaverActive=FALSE;
static bool fNoMonitorLowPower=false;
static bool fNoMonitorLowPowerActiveOnly=false;
static BOOL fLowPowerActiveOriginal=FALSE,fPowerOffActiveOriginal=FALSE;
static bool fRestoreChannel=false;
static struct {
	int Channel;
	int Space;
	int Service;
	TCHAR szDriverName[MAX_PATH];
} RestoreChannelInfo = {-1, -1, 0};

static bool fAdjustAspectResizing=false;
static bool fSnapAtWindowEdge=false;
static int SnapAtWindowEdgeMargin=8;
static bool fPanScanNoResizeWindow=false;
static CMediaViewer::ViewStretchMode FullscreenStretchMode=CMediaViewer::STRETCH_KEEPASPECTRATIO;
static CMediaViewer::ViewStretchMode MaximizeStretchMode=CMediaViewer::STRETCH_KEEPASPECTRATIO;
static bool fClientEdge=true;

static bool fUseOSD=false;
static bool fUsePseudoOSD=true;
static COLORREF crOSDTextColor=RGB(0,255,128);
static unsigned int OSDFadeTime=2000;
static CPseudoOSD PseudoOSD;

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
static bool fFunctionKeyChangeChannel=true;
static bool fDigitKeyChangeChannel=true;
static bool fNumPadChangeChannel=true;

static CImageCodec ImageCodec;
static CCapturePreview CapturePreview;
static bool fShowCapturePreview=false;
static bool fUseGrabberFilter=false;




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
	::lstrcpy(m_szIniFileName,szModuleFileName);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
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
	m_fFirstExecute=!::PathFileExists(m_szIniFileName);
	if (!m_fFirstExecute)
		LoadSettings();
	return true;
}


bool CAppMain::Finalize()
{
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


bool CAppMain::InitializeChannel()
{
	bool fUDPDriver=CoreEngine.IsUDPDriver();
	CFilePath ChannelFilePath;

	ChannelManager.MakeDriverTuningSpaceList(&CoreEngine.m_DtvEngine.m_BonSrcDecoder);
	if (!fUDPDriver) {
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
				&& ::lstrcmpi(::PathFindFileName(szDriverFileName),
						  TEXT("BonDriver_UDP.dll"))!=0) {
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
		Logger.AddLog(TEXT("チャンネル設定を \"%s\" から読み込みました"),
												ChannelFilePath.GetFileName());
	TCHAR szFileName[MAX_PATH];
	bool fLoadChannelSettings=true;
	if (!fUDPDriver) {
		::lstrcpy(szFileName,CoreEngine.GetDriverFileName());
	} else {
		if (::lstrcmpi(::PathFindFileName(szDriverFileName),TEXT("BonDriver_UDP.dll"))!=0) {
			::lstrcpy(szFileName,szDriverFileName);
		} else {
			fLoadChannelSettings=false;
		}
	}
	if (fLoadChannelSettings)
		ChannelManager.LoadChannelSettings(m_szChannelSettingFileName,szFileName);
	ChannelManager.SetUseDriverChannelList(fUDPDriver);
	ChannelManager.SetCurrentSpace(
		(!fUDPDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
											CChannelManager::SPACE_ALL:0);
	ChannelManager.SetCurrentChannel(fUDPDriver?0:-1);
	ChannelManager.SetCurrentService(0);
	SetTuningSpaceMenu();
	SetChannelMenu();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	return true;
}


bool CAppMain::UpdateChannelList(const CTuningSpaceList *pList)
{
	bool fUDPDriver=CoreEngine.IsUDPDriver();

	ChannelManager.SetTuningSpaceList(pList);
	ChannelManager.SetCurrentSpace(
		(!fUDPDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
											CChannelManager::SPACE_ALL:0);
	ChannelManager.SetCurrentChannel(fUDPDriver?0:-1);
	ChannelManager.SetCurrentService(0);
	SetTuningSpaceMenu();
	SetChannelMenu();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	return true;
}


bool CAppMain::SaveChannelSettings()
{
	if (!CoreEngine.IsDriverLoaded())
		return true;
	return ChannelManager.SaveChannelSettings(m_szChannelSettingFileName,
											CoreEngine.GetDriverFileName());
}


void CAppMain::SetTuningSpaceMenu(HMENU hmenu)
{
	int NumSpaces,i;
	LPCTSTR pszName;

	ClearMenu(hmenu);
	if ((!CoreEngine.IsUDPDriver() || pNetworkRemocon==NULL)
			&& ChannelManager.GetAllChannelList()->NumChannels()>0)
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SPACE_ALL,TEXT("すべて"));
	NumSpaces=0;
	for (i=0;(pszName=CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetSpaceName(i))!=NULL;i++) {
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SPACE_FIRST+i,pszName);
		NumSpaces++;
	}
	CheckMenuRadioItem(hmenu,CM_SPACE_ALL,CM_SPACE_ALL+NumSpaces,
				CM_SPACE_FIRST+ChannelManager.GetCurrentSpace(),MF_BYCOMMAND);
	AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	int CurDriver=-1;
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,
				   pDriverInfo->GetFileName());
		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0)
			CurDriver=i;
	}
	if (CurDriver<0) {
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,
				   CoreEngine.GetDriverFileName());
		CurDriver=i++;
	}
	CheckMenuRadioItem(hmenu,CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
					   CM_DRIVER_FIRST+CurDriver,MF_BYCOMMAND);
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
	for (i=0;i<pList->NumChannels();i++) {
		wsprintf(szText,TEXT("%d: %s"),pList->GetChannelNo(i),pList->GetName(i));
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
				| (i!=0 && i%12==0?MF_MENUBREAK:0),CM_CHANNEL_FIRST+i,szText);
	}
	if (ChannelManager.GetCurrentChannel()>=0)
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


bool CAppMain::SetChannel(int Space,int Channel,int Service/*=-1*/)
{
	const CChannelInfo *pPrevChInfo=ChannelManager.GetCurrentRealChannelInfo();
	int OldSpace=ChannelManager.GetCurrentSpace(),OldChannel=ChannelManager.GetCurrentChannel();

	if (!ChannelManager.SetCurrentSpace(Space)
			|| !ChannelManager.SetCurrentChannel(Channel))
		return false;
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
	if (pChInfo==NULL) {
		ChannelManager.SetCurrentSpace(OldSpace);
		ChannelManager.SetCurrentChannel(OldChannel);
		return false;
	}
	if (pPrevChInfo==NULL
			|| pChInfo->GetSpace()!=pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex()!=pPrevChInfo->GetChannelIndex()) {
		if (!CoreEngine.m_DtvEngine.SetChannel(pChInfo->GetSpace(),
											   pChInfo->GetChannelIndex())) {
			ChannelManager.SetCurrentSpace(OldSpace);
			ChannelManager.SetCurrentChannel(OldChannel);
			return false;
		}
		ChannelManager.SetCurrentService(Service);
	} else {
		ChannelManager.SetCurrentService(Service);
		if (Service>=0) {
			SetService(Service);
		} else {
			SetServiceByID(pChInfo->GetServiceID());
		}
	}
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	SetChannelMenu();
	MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
		CM_SPACE_FIRST+ChannelManager.GetCurrentSpace());
	MainMenu.CheckRadioItem(CM_CHANNEL_FIRST,
		CM_CHANNEL_FIRST+ChannelManager.GetCurrentRealChannelList()->NumChannels()-1,
		CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel());
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
	MainMenu.CheckRadioItem(CM_SERVICE_FIRST,CM_SERVICE_FIRST+NumServices-1,
							CM_SERVICE_FIRST+Service);
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

	int NumServices=CoreEngine.m_DtvEngine.m_ProgManager.GetServiceNum();

	for (int i=0;i<NumServices;i++) {
		WORD SID;

		if (CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&SID,i)
				&& SID==ServiceID) {
			if (SetService(i)) {
				if (pServiceIndex!=NULL)
					*pServiceIndex=i;
				return true;
			}
			return false;
		}
	}
	return SetService(0);
}


bool CAppMain::SetDriver(LPCTSTR pszFileName)
{
	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	bool fOK;

	SaveChannelSettings();
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.SetDriverFileName(pszFileName);
	fOK=CoreEngine.LoadDriver();
	if (fOK) {
		Logger.AddLog(TEXT("%s を読み込みました"),CoreEngine.GetDriverFileName());
		fOK=CoreEngine.OpenDriver();
		if (fOK) {
			AppMain.InitializeChannel();
			int i=ChannelManager.GetCurrentChannelList()->Find(0,0,0);
			if (i>=0)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+i);
			::SetCursor(hcurOld);
		} else {
			::SetCursor(hcurOld);
			::MessageBox(MainWindow.GetHandle(),
						 TEXT("BonDriverの初期化ができません。"),NULL,
						 MB_OK | MB_ICONEXCLAMATION);
		}
	} else {
		TCHAR szMessage[MAX_PATH+64];

		::SetCursor(hcurOld);
		::wsprintf(szMessage,TEXT("\"%s\" をロードできません。"),pszFileName);
		::MessageBox(MainWindow.GetHandle(),szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
	}
	StatusView.SetSingleText(NULL);
	return fOK;
}


bool CAppMain::UpdateDriverMenu()
{
	SetTuningSpaceMenu();
	return true;
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
		if (Setting.Read(TEXT("HDUSDriverVersion"),&Value))
			fNewHDUSDriver=Value==2;
		Setting.Read(TEXT("DescrambleCurServiceOnly"),&fDescrambleCurServiceOnly);
		Setting.Read(TEXT("KeepSingleTask"),&fKeepSingleTask);
		if (Setting.Read(TEXT("Resident"),&f))
			ResidentManager.SetResident(f);
		if (Setting.Read(TEXT("NoDescramble"),&f))	// Backward compatibility
			CoreEngine.SetCardReaderType(f?CCardReader::READER_NONE:CCardReader::READER_SCARD);
		if (Setting.Read(TEXT("CardReader"),&Value))
			CoreEngine.SetCardReaderType((CCardReader::ReaderType)Value);
		Setting.Read(TEXT("NoRemoteController"),&fNoRemoteController);
		Setting.Read(TEXT("NoKeyHook"),&fNoKeyHook);
		Setting.Read(TEXT("NoScreenSaver"),&fNoScreenSaver);
		Setting.Read(TEXT("NoMonitorLowPower"),&fNoMonitorLowPower);
		Setting.Read(TEXT("NoMonitorLowPowerActiveOnly"),&fNoMonitorLowPowerActiveOnly);
		Setting.Read(TEXT("RestoreChannel"),&fRestoreChannel);
		//Setting.Read(TEXT("CurChannel"),&RestoreChannelInfo.Channel);
		Setting.Read(TEXT("CurChannelIndex"),&RestoreChannelInfo.Channel);
		Setting.Read(TEXT("CurSpace"),&RestoreChannelInfo.Space);
		Setting.Read(TEXT("CurService"),&RestoreChannelInfo.Service);
		Setting.Read(TEXT("OldDriver"),RestoreChannelInfo.szDriverName,
									lengthof(RestoreChannelInfo.szDriverName));
		if (Setting.Read(TEXT("PacketBuffering"),&f))
			CoreEngine.SetPacketBuffering(f);
		unsigned int BufferLength;
		if (Setting.Read(TEXT("PacketBufferLength"),&BufferLength))
			CoreEngine.SetPacketBufferLength(BufferLength);
		if (Setting.Read(TEXT("PacketBufferPoolPercentage"),&Value))
			CoreEngine.SetPacketBufferPoolPercentage(Value);
		Setting.Read(TEXT("AdjustAspectResizing"),&fAdjustAspectResizing);
		Setting.Read(TEXT("SnapToWindowEdge"),&fSnapAtWindowEdge);
		Setting.Read(TEXT("PanScanNoResizeWindow"),&fPanScanNoResizeWindow);
		if (Setting.Read(TEXT("FullscreenStretchMode"),&Value))
			FullscreenStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
										   CMediaViewer::STRETCH_KEEPASPECTRATIO;
		if (Setting.Read(TEXT("MaximizeStretchMode"),&Value))
			MaximizeStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
										 CMediaViewer::STRETCH_KEEPASPECTRATIO;
		Setting.Read(TEXT("ClientEdge"),&fClientEdge);
		Setting.Read(TEXT("UseOSD"),&fUseOSD);
		Setting.Read(TEXT("PseudoOSD"),&fUsePseudoOSD);
		Setting.ReadColor(TEXT("OSDTextColor"),&crOSDTextColor);
		PseudoOSD.SetTextColor(crOSDTextColor);
		Setting.Read(TEXT("OSDFadeTime"),&OSDFadeTime);
		Setting.Read(TEXT("WheelMode"),&WheelMode);
		Setting.Read(TEXT("WheelShiftMode"),&WheelShiftMode);
		Setting.Read(TEXT("ReverseWheelChannel"),&fWheelChannelReverse);
		Setting.Read(TEXT("WheelChannelDelay"),&WheelChannelDelay);
		Setting.Read(TEXT("FuncKeyChangeChannel"),&fFunctionKeyChangeChannel);
		Setting.Read(TEXT("DigitKeyChangeChannel"),&fDigitKeyChangeChannel);
		Setting.Read(TEXT("NumPadChangeChannel"),&fNumPadChangeChannel);
		if (Setting.Read(TEXT("RecOptionFileName"),szText,MAX_PATH))
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
		CapturePreview.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("CapturePreviewLeft"),&Left);
		Setting.Read(TEXT("CapturePreviewTop"),&Top);
		Setting.Read(TEXT("CapturePreviewWidth"),&Width);
		Setting.Read(TEXT("CapturePreviewHeight"),&Height);
		CapturePreview.SetPosition(Left,Top,Width,Height);
		CapturePreview.MoveToMonitorInside();
		// Experimental options
		Setting.Read(TEXT("UseGrabber"),&fUseGrabberFilter);
		Setting.Read(TEXT("IncrementUDPPort"),&fIncrementUDPPort);
		PanelOptions.Read(&Setting);
		RecordOptions.Read(&Setting);
		CaptureOptions.Read(&Setting);
		ChannelScan.Read(&Setting);
		EpgOptions.Read(&Setting);
		NetworkRemoconOptions.Read(&Setting);
		Setting.Close();
	} else
		return false;
	StatusOptions.Load(m_szIniFileName);
	ColorSchemeOptions.Load(m_szIniFileName);
	ProgramGuideOptions.Load(m_szIniFileName);
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
		Setting.Write(TEXT("NoRemoteController"),fNoRemoteController);
		Setting.Write(TEXT("NoKeyHook"),fNoKeyHook);
		Setting.Write(TEXT("NoScreenSaver"),fNoScreenSaver);
		Setting.Write(TEXT("NoMonitorLowPower"),fNoMonitorLowPower);
		Setting.Write(TEXT("NoMonitorLowPowerActiveOnly"),
												fNoMonitorLowPowerActiveOnly);
		Setting.Write(TEXT("RestoreChannel"),fRestoreChannel);
		//Setting.Write(TEXT("CurChannel"),RestoreChannelInfo.Channel);
		Setting.Write(TEXT("CurChannelIndex"),RestoreChannelInfo.Channel);
		Setting.Write(TEXT("CurSpace"),RestoreChannelInfo.Space);
		Setting.Write(TEXT("CurService"),RestoreChannelInfo.Service);
		Setting.Write(TEXT("OldDriver"),RestoreChannelInfo.szDriverName);
		Setting.Write(TEXT("PacketBuffering"),CoreEngine.GetPacketBuffering());
		Setting.Write(TEXT("PacketBufferLength"),
							(unsigned int)CoreEngine.GetPacketBufferLength());
		Setting.Write(TEXT("PacketBufferPoolPercentage"),
								CoreEngine.GetPacketBufferPoolPercentage());
		Setting.Write(TEXT("AdjustAspectResizing"),fAdjustAspectResizing);
		Setting.Write(TEXT("SnapToWindowEdge"),fSnapAtWindowEdge);
		Setting.Write(TEXT("PanScanNoResizeWindow"),fPanScanNoResizeWindow);
		Setting.Write(TEXT("FullscreenStretchMode"),(int)FullscreenStretchMode);
		Setting.Write(TEXT("MaximizeStretchMode"),(int)MaximizeStretchMode);
		Setting.Write(TEXT("ClientEdge"),fClientEdge);
		Setting.Write(TEXT("UseOSD"),fUseOSD);
		Setting.Write(TEXT("PseudoOSD"),fUsePseudoOSD);
		Setting.WriteColor(TEXT("OSDTextColor"),crOSDTextColor);
		Setting.Write(TEXT("OSDFadeTime"),OSDFadeTime);
		Setting.Write(TEXT("WheelMode"),WheelMode);
		Setting.Write(TEXT("WheelShiftMode"),WheelShiftMode);
		Setting.Write(TEXT("ReverseWheelChannel"),fWheelChannelReverse);
		Setting.Write(TEXT("WheelChannelDelay"),WheelChannelDelay);
		Setting.Write(TEXT("FuncKeyChangeChannel"),fFunctionKeyChangeChannel);
		Setting.Write(TEXT("DigitKeyChangeChannel"),fDigitKeyChangeChannel);
		Setting.Write(TEXT("NumPadChangeChannel"),fNumPadChangeChannel);
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
		CapturePreview.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("CapturePreviewLeft"),Left);
		Setting.Write(TEXT("CapturePreviewTop"),Top);
		Setting.Write(TEXT("CapturePreviewWidth"),Width);
		Setting.Write(TEXT("CapturePreviewHeight"),Height);
		Setting.Write(TEXT("HDUSDriverVersion"),fNewHDUSDriver?2:1);
		Setting.Write(TEXT("IncrementUDPPort"),fIncrementUDPPort);
		PanelOptions.Write(&Setting);
		RecordOptions.Write(&Setting);
		CaptureOptions.Write(&Setting);
		ChannelScan.Write(&Setting);
		EpgOptions.Write(&Setting);
		NetworkRemoconOptions.Write(&Setting);
		Setting.Close();
	} else
		return false;
	StatusOptions.Save(m_szIniFileName);
	ColorSchemeOptions.Save(m_szIniFileName);
	ProgramGuideOptions.Save(m_szIniFileName);
	return true;
}


bool CAppMain::ShowHelpContent(int ID)
{
	return HtmlHelpClass.ShowContent(ID);
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
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
	::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	::DrawText(hdc,TEXT("アフリカ中央テレビ"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CChannelStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	const CChannelList *pList;

	GetMenuPos(&pt);
	if (!CoreEngine.IsUDPDriver()
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
	WORD Width,Height;
	TCHAR szText[32];

	if (!CoreEngine.m_DtvEngine.GetVideoSize(&Width,&Height))
		Width=Height=0;
	::wsprintf(szText,TEXT("%d x %d (%d %%)"),Width,Height,MainWindow.CalcZoomRate());
	::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CVideoSizeStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	::DrawText(hdc,TEXT("1920 x 1080 (100 %)"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
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

CAudioChannelStatusItem::CAudioChannelStatusItem() : CStatusItem(STATUS_ITEM_AUDIOCHANNEL,64)
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
	DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CAudioChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,TEXT("Stereo"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CAudioChannelStatusItem::OnLButtonDown(int x,int y)
{
	if (CoreEngine.m_DtvEngine.GetAudioChannelNum()==2)
		MainWindow.SendCommand(CM_STEREOMODE);
	else
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
	} else {
		pszText=TEXT("■ <録画>");
	}
	::DrawText(hdc,pszText,-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CRecordStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	::DrawText(hdc,TEXT("● 0:24:15"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CRecordStatusItem::OnLButtonDown(int x,int y)
{
	MainWindow.SendCommand(CM_RECORD);
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
	CheckMenuItem(hmenu,CM_DISABLEVIEWER,
		MF_BYCOMMAND | (MainWindow.IsPreview()?MFS_UNCHECKED:MFS_CHECKED));
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
	DeleteObject(m_hbmIcon);
}

void CCaptureStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	HDC hdcMem;
	HBITMAP hbmOld;
	RGBQUAD Palette[2];
	COLORREF cr,crTrans;

	if (m_hbmIcon==NULL)
		m_hbmIcon=static_cast<HBITMAP>(LoadImage(hInst,
								MAKEINTRESOURCE(IDB_CAPTURE),IMAGE_BITMAP,0,0,
								LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));
	hdcMem=CreateCompatibleDC(hdc);
	hbmOld=(HBITMAP)SelectObject(hdcMem,m_hbmIcon);
	cr=GetTextColor(hdc);
	Palette[0].rgbBlue=GetBValue(cr);
	Palette[0].rgbGreen=GetGValue(cr);
	Palette[0].rgbRed=GetRValue(cr);
	crTrans=cr^0x00FFFFFF;
	Palette[1].rgbBlue=GetBValue(crTrans);
	Palette[1].rgbGreen=GetGValue(crTrans);
	Palette[1].rgbRed=GetRValue(crTrans);
	SetDIBColorTable(hdcMem,0,2,Palette);
	TransparentBlt(hdc,
					pRect->left+(pRect->right-pRect->left-16)/2,
					pRect->top+(pRect->bottom-pRect->top-16)/2,
					16,16,hdcMem,0,0,16,16,crTrans);
	SelectObject(hdcMem,hbmOld);
	DeleteDC(hdcMem);
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
			CM_CAPTURESIZE_FIRST+CaptureOptions.GetCaptureSize(),MF_BYCOMMAND);
	if (fShowCapturePreview)
		CheckMenuItem(hmenu,CM_CAPTUREPREVIEW,MF_BYCOMMAND | MFS_CHECKED);
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
};

CErrorStatusItem::CErrorStatusItem() : CStatusItem(STATUS_ITEM_ERROR,100)
{
}

void CErrorStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	TCHAR szText[64];
	int Length;

	Length=wsprintf(szText,TEXT("E %u"),CoreEngine.GetErrorPacketCount());
	if (CoreEngine.GetDescramble()
			&& CoreEngine.GetCardReaderType()!=CCardReader::READER_NONE)
		wsprintf(szText+Length,TEXT(" / S %u"),CoreEngine.GetScramblePacketCount());
	DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CErrorStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,TEXT("E 0 / S 127"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}


class CSignalLevelStatusItem : public CStatusItem {
public:
	CSignalLevelStatusItem();
	LPCTSTR GetName() const { return TEXT("受信レベル"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
};

CSignalLevelStatusItem::CSignalLevelStatusItem() : CStatusItem(STATUS_ITEM_SIGNALLEVEL,120)
{
}

void CSignalLevelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CCoreEngine::DriverType DriverType=CoreEngine.GetDriverType();
	float SignalLevel;
	int Level;
	TCHAR szText[64];

	SignalLevel=CoreEngine.GetSignalLevel();
	Level=(int)(SignalLevel*100.0f);
	if (DriverType==CCoreEngine::DRIVER_UDP
			|| (DriverType==CCoreEngine::DRIVER_HDUS && !fNewHDUSDriver)) {
		// ビットレートのみ
		wsprintf(szText,TEXT("%d.%02d Mbps"),Level/100,Level%100);
	} else {
		// 情報パネルと合わせるためにfloatで計算する
		int BitRate=(int)(CoreEngine.GetBitRateFloat()*100.0f);

		wsprintf(szText,TEXT("%d.%02d dB / %d.%02d Mbps"),
								Level/100,Level%100,BitRate/100,BitRate%100);
	}
	DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CSignalLevelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,TEXT("52.30 dB / 16.73 Mbps"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
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
	DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CClockStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,TEXT("13:25:30"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}


class CProgramInfoStatusItem : public CStatusItem {
	LPTSTR m_pszProgramInfo;
public:
	CProgramInfoStatusItem();
	~CProgramInfoStatusItem();
	LPCTSTR GetName() const { return TEXT("番組情報"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnVisibleChange(bool fVisible);
	bool SetProgramInfo(LPCTSTR pszInfo);
};

CProgramInfoStatusItem::CProgramInfoStatusItem() : CStatusItem(STATUS_ITEM_PROGRAMINFO,256)
{
	m_pszProgramInfo=NULL;
}

CProgramInfoStatusItem::~CProgramInfoStatusItem()
{
	delete [] m_pszProgramInfo;
}

void CProgramInfoStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	if (m_pszProgramInfo!=NULL)
		DrawText(hdc,m_pszProgramInfo,-1,const_cast<LPRECT>(pRect),
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CProgramInfoStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,TEXT("01:00～01:30 今日のニュース"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CProgramInfoStatusItem::OnVisibleChange(bool fVisible)
{
	if (!fVisible) {
		delete [] m_pszProgramInfo;
		m_pszProgramInfo=NULL;
	}
}

bool CProgramInfoStatusItem::SetProgramInfo(LPCTSTR pszInfo)
{
	if (pszInfo!=NULL && lstrcmp(pszInfo,m_pszProgramInfo)==0)
		return true;
	delete [] m_pszProgramInfo;
	m_pszProgramInfo=DuplicateString(pszInfo);
	m_pStatus->UpdateItem(m_ID);
	return true;
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
	::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void CBufferingStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	::DrawText(hdc,TEXT("R 52 / B 48%"),-1,const_cast<LPRECT>(pRect),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
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
	ControlPanel.SetColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELHIGHLIGHTBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELHIGHLIGHTTEXT));
	for (int i=0;i<=CProgramGuide::COLOR_LAST;i++)
		ProgramGuide.SetColor(i,pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_FIRST+i));
	ProgramGuide.Invalidate();
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
			DriverManager.Find(szDirectory);
			AppMain.UpdateDriverMenu();
			for (int i=0;i<DriverManager.NumDrivers();i++) {
				const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

				::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVER,CB_ADDSTRING,
					0,reinterpret_cast<LPARAM>(pDriverInfo->GetFileName()));
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVER,szDriverFileName);
		}
		{
			CDirectShowFilterFinder FilterFinder;
			WCHAR szFilterName[128];
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
		CheckDlgButton(hDlg,IDC_OPTIONS_USEREMOTECONTROLLER,
								fNoRemoteController?BST_UNCHECKED:BST_CHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
										fNoKeyHook?BST_CHECKED:BST_UNCHECKED);
		EnableDlgItem(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
														!fNoRemoteController);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOSCREENSAVER,
									fNoScreenSaver?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER,
								fNoMonitorLowPower?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
					fNoMonitorLowPowerActiveOnly?BST_CHECKED:BST_UNCHECKED);
		EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
														fNoMonitorLowPower);
		CheckDlgButton(hDlg,IDC_OPTIONS_KEEPSINGLETASK,
									fKeepSingleTask?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_RESIDENT,
			ResidentManager.GetResident()?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_RESTORECHANNEL,
									fRestoreChannel?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_NEWHDUSDRIVER,
									fNewHDUSDriver?BST_CHECKED:BST_UNCHECKED);
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

		case IDC_OPTIONS_USEREMOTECONTROLLER:
			EnableDlgItem(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
					IsDlgButtonChecked(hDlg,IDC_OPTIONS_USEREMOTECONTROLLER)==
																BST_CHECKED);
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
				int Sel=SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,
															CB_GETCURSEL,0,0);
				if (Sel>0)
					SendDlgItemMessage(hDlg,IDC_OPTIONS_DECODER,CB_GETLBTEXT,
											Sel,(LPARAM)szMpeg2DecoderName);
				else
					szMpeg2DecoderName[0]='\0';
				VideoRendererType=(CVideoRenderer::RendererType)
					SendDlgItemMessage(hDlg,IDC_OPTIONS_RENDERER,CB_GETCURSEL,0,0);
/*
CoreEngine.m_DtvEngine.SetTracer(&StatusView);
CoreEngine.m_DtvEngine.RebuildMediaViewer(VideoContainerWindow.GetHandle(),MainWindow.GetHandle(),
										VideoRendererType,szMpeg2DecoderName);
CoreEngine.EnablePreview(true);
CoreEngine.m_DtvEngine.SetTracer(NULL);
StatusView.SetSingleText(NULL);
*/
				CoreEngine.SetCardReaderType((CCardReader::ReaderType)
					SendDlgItemMessage(hDlg,IDC_OPTIONS_CARDREADER,CB_GETCURSEL,0,0));
			}
			{
				bool f1,f2;
				f1=IsDlgButtonChecked(hDlg,
							IDC_OPTIONS_USEREMOTECONTROLLER)==BST_UNCHECKED;
				f2=IsDlgButtonChecked(hDlg,
						IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY)==BST_CHECKED;
				if (f1!=fNoRemoteController || f2!=fNoKeyHook) {
					if (f1 || f2) {
						SAFE_DELETE(pRemoteController);
					} else {
						if (pRemoteController==NULL) {
							pRemoteController=new CRemoteController(MainWindow.GetHandle());
							pRemoteController->BeginHook();
						}
					}
					if (f1) {
						hAccel=NULL;
					} else {
						hAccel=LoadAccelerators(hInst,
									MAKEINTRESOURCE(f2?IDA_ACCEL:IDA_ACCEL2));
					}
					fNoRemoteController=f1;
					fNoKeyHook=f2;
				}
			}
			fNoScreenSaver=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_NOSCREENSAVER)==BST_CHECKED;
			fNoMonitorLowPower=IsDlgButtonChecked(hDlg,
								IDC_OPTIONS_NOMONITORLOWPOWER)==BST_CHECKED;
			fNoMonitorLowPowerActiveOnly=IsDlgButtonChecked(hDlg,
						IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY)==BST_CHECKED;
			SetDisplayStatus();
			fKeepSingleTask=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_KEEPSINGLETASK)==BST_CHECKED;
			ResidentManager.SetResident(
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_RESIDENT)==BST_CHECKED);
			fRestoreChannel=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_RESTORECHANNEL)==BST_CHECKED;
			fNewHDUSDriver=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_NEWHDUSDRIVER)==BST_CHECKED;
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


class CViewOptions : public COptions {
public:
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

BOOL CALLBACK CViewOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static COLORREF crOSDText;

	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg,IDC_OPTIONS_ADJUSTASPECTRESIZING,
			fAdjustAspectResizing?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_SNAPATWINDOWEDGE,
			fSnapAtWindowEdge?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_PANSCANNORESIZEWINDOW,
			fPanScanNoResizeWindow?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME,
			FullscreenStretchMode==CMediaViewer::STRETCH_CUTFRAME?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME,
			MaximizeStretchMode==CMediaViewer::STRETCH_CUTFRAME?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_CLIENTEDGE,
			fClientEdge?BST_CHECKED:BST_UNCHECKED);
		// OSD
		CheckDlgButton(hDlg,IDC_OPTIONS_USEOSD,fUseOSD?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg,IDC_OPTIONS_PSEUDOOSD,fUsePseudoOSD?BST_CHECKED:BST_UNCHECKED);
		crOSDText=crOSDTextColor;
		SetDlgItemInt(hDlg,IDC_OPTIONS_OSDFADETIME,OSDFadeTime/1000,FALSE);
		SendDlgItemMessage(hDlg,IDC_OPTIONS_OSDFADETIME_UD,UDM_SETRANGE,0,
													MAKELPARAM(UD_MAXVAL,1));
		EnableDlgItems(hDlg,IDC_OPTIONS_OSD_FIRST,IDC_OPTIONS_OSD_LAST,fUseOSD);
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=(LPDRAWITEMSTRUCT)lParam;
			RECT rc;

			rc=pdis->rcItem;
			DrawEdge(pdis->hDC,&rc,BDR_SUNKENOUTER,BF_RECT | BF_ADJUST);
			DrawUtil::Fill(pdis->hDC,&rc,crOSDText);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_USEOSD:
			EnableDlgItems(hDlg,IDC_OPTIONS_OSD_FIRST,IDC_OPTIONS_OSD_LAST,
					IsDlgButtonChecked(hDlg,IDC_OPTIONS_USEOSD)==BST_CHECKED);
			return TRUE;

		case IDC_OPTIONS_OSDTEXTCOLOR:
			if (ChooseColorDialog(hDlg,&crOSDText))
				InvalidateDlgItem(hDlg,IDC_OPTIONS_OSDTEXTCOLOR);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			fAdjustAspectResizing=IsDlgButtonChecked(hDlg,
								IDC_OPTIONS_ADJUSTASPECTRESIZING)==BST_CHECKED;
			fSnapAtWindowEdge=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_SNAPATWINDOWEDGE)==BST_CHECKED;
			fPanScanNoResizeWindow=
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_PANSCANNORESIZEWINDOW)==BST_CHECKED;
			FullscreenStretchMode=
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME)==BST_CHECKED?
				CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
			if (MainWindow.GetFullscreen())
				CoreEngine.m_DtvEngine.m_MediaViewer.ResizeVideoWindow();
			MaximizeStretchMode=
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME)==BST_CHECKED?
				CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
			if (MainWindow.GetMaximize())
				CoreEngine.m_DtvEngine.m_MediaViewer.ResizeVideoWindow();
			{
				bool fEdge=IsDlgButtonChecked(hDlg,IDC_OPTIONS_CLIENTEDGE)==BST_CHECKED;
				if (fEdge!=fClientEdge) {
					fClientEdge=fEdge;
					ViewWindow.SetExStyle(ViewWindow.GetExStyle()^WS_EX_CLIENTEDGE,true);
				}
			}
			fUseOSD=IsDlgButtonChecked(hDlg,IDC_OPTIONS_USEOSD)==BST_CHECKED;
			fUsePseudoOSD=
				IsDlgButtonChecked(hDlg,IDC_OPTIONS_PSEUDOOSD)==BST_CHECKED;
			crOSDTextColor=crOSDText;
			OSDFadeTime=GetDlgItemInt(hDlg,IDC_OPTIONS_OSDFADETIME,NULL,FALSE)*1000;
			break;
		}
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
			CheckDlgButton(hDlg,IDC_OPTIONS_CHANGECH_FUNCTION,
						fFunctionKeyChangeChannel?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hDlg,IDC_OPTIONS_CHANGECH_DIGIT,
						fDigitKeyChangeChannel?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hDlg,IDC_OPTIONS_CHANGECH_NUMPAD,
						fNumPadChangeChannel?BST_CHECKED:BST_UNCHECKED);
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
				fFunctionKeyChangeChannel=IsDlgButtonChecked(hDlg,
								IDC_OPTIONS_CHANGECH_FUNCTION)==BST_CHECKED;
				fDigitKeyChangeChannel=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_CHANGECH_DIGIT)==BST_CHECKED;
				fNumPadChangeChannel=IsDlgButtonChecked(hDlg,
									IDC_OPTIONS_CHANGECH_NUMPAD)==BST_CHECKED;
			}
			break;
		}
		break;
	}
	return FALSE;
}


static CGeneralOptions GeneralOptions;
static CViewOptions ViewOptions;
static COperationOptions OperationOptions;


class COptionDialog {
	enum { NUM_PAGES=13 };
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
public:
	enum {
		PAGE_GENERAL,
		PAGE_VIEW,
		PAGE_STATUS,
		PAGE_PANEL,
		PAGE_COLORSCHEME,
		PAGE_OPERATION,
		PAGE_RECORD,
		PAGE_CAPTURE,
		PAGE_CHANNELSCAN,
		PAGE_EPG,
		PAGE_PROGRAMGUIDE,
		PAGE_NETWORKREMOCON,
		PAGE_LOG
	};
	COptionDialog();
	~COptionDialog();
	bool ShowDialog(HWND hwndOwner,int StartPage=-1);
};


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),					MAKEINTRESOURCE(IDD_OPTIONS_GENERAL),
		CGeneralOptions::DlgProc,		&GeneralOptions,	RGB(128,0,0)},
	{TEXT("表示"),					MAKEINTRESOURCE(IDD_OPTIONS_VIEW),
		CViewOptions::DlgProc,			&ViewOptions,		RGB(0,128,0)},
	{TEXT("ステータスバー"),		MAKEINTRESOURCE(IDD_OPTIONS_STATUS),
		CStatusOptions::DlgProc,		&StatusOptions,		RGB(0,0,128)},
	{TEXT("パネル"),				MAKEINTRESOURCE(IDD_OPTIONS_PANEL),
		CPanelOptions::DlgProc,			&PanelOptions,		RGB(0,255,128)},
	{TEXT("配色"),					MAKEINTRESOURCE(IDD_OPTIONS_COLORSCHEME),
		CColorSchemeOptions::DlgProc,	&ColorSchemeOptions,RGB(0,128,255)},
	{TEXT("操作"),					MAKEINTRESOURCE(IDD_OPTIONS_OPERATION),
		COperationOptions::DlgProc,		&OperationOptions,	RGB(128,128,0)},
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
			CoreEngine.m_DtvEngine.SetChannel(0,0);
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
		MainWindow.PostCommand(CM_CHANNEL_FIRST);
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
	return true;
}


static COptionDialog OptionDialog;




class CFullscreen : public CBasicWindow {
	bool m_fShowCursor;
	bool m_fMenu;
	bool m_fShowStatusView;
	void ShowStatusView(bool fShow);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static CFullscreen *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CFullscreen();
	~CFullscreen();
	bool Create(HWND hwndOwner);
	void OnRButtonDown();
	void OnMouseMove();
	HWND GetHandle() const { return m_hwnd; }
	static bool Initialize();
};


static CFullscreen *pFullscreen=NULL;




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

	MainWindow.GetPosition(&rc);
	Size=PanelFrame.GetDockingWidth()+Splitter.GetBarWidth();
	if (!fFloating || rc.right-rc.left>Size) {
		if (Splitter.IDToIndex(PANE_ID_PANEL)==0) {
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
		WORD ServiceID;

		if (CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,
										ChannelManager.GetCurrentService()))
			ProgramListView.UpdateProgramList(ServiceID);
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


static CPanelStatus PanelStatus;
static CMyInfoPanelEventHandler InfoPanelEventHandler;




class CMyProgramGuideEventHandler : public CProgramGuideEventHandler {
public:
	bool OnClose();
	void OnServiceTitleLButtonDown(WORD ServiceID);
	bool OnBeginUpdate();
	void OnEndUpdate();
	bool OnRefresh();
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
	if (CoreEngine.IsUDPDriver()) {
		::MessageBox(MainWindow.GetHandle(),
					 TEXT("UDPでは番組表の取得はできません。"),
					 TEXT("ごめん"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	if (RecordManager.IsRecording()) {
		::MessageBox(MainWindow.GetHandle(),
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

	if (!CoreEngine.IsUDPDriver())
		pList=ChannelManager.GetCurrentChannelList();
	else
		pList=ChannelManager.GetFileAllChannelList();
	if (pList!=NULL) {
		CProgramGuideServiceIDList ServiceIDList;
		HCURSOR hcurOld;

		hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		for (int i=0;i<pList->NumChannels();i++) {
			WORD ServiceID=pList->GetChannelInfo(i)->GetServiceID();
			if (ServiceID!=0) {
				ServiceIDList.Add(ServiceID);
				EpgProgramList.UpdateProgramList(ServiceID);
			}
		}
		m_pProgramGuide->SetServiceIDList(&ServiceIDList);
		::SetCursor(hcurOld);
	}
	return true;
}


static CMyProgramGuideEventHandler ProgramGuideEventHandler;




bool CFullscreen::Initialize()
{
	WNDCLASS wc;

	wc.style=0;
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
	return reinterpret_cast<CFullscreen*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CFullscreen::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CFullscreen *pFullscreen=dynamic_cast<CFullscreen*>(OnCreate(hwnd,lParam));

			VideoContainerWindow.SetParent(pFullscreen);
			RECT rc;
			pFullscreen->GetClientRect(&rc);
			VideoContainerWindow.SetPosition(&rc);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(FullscreenStretchMode);
			pFullscreen->m_fShowCursor=true;
			pFullscreen->m_fMenu=false;
			pFullscreen->m_fShowStatusView=false;
			::SetTimer(hwnd,1,1000,NULL);
		}
		return 0;

	case WM_SIZE:
		VideoContainerWindow.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		{
			CFullscreen *pFullscreen=GetThis(hwnd);

			pFullscreen->OnRButtonDown();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		MainWindow.SendCommand(CM_FULLSCREEN);
		return 0;

	case WM_MOUSEMOVE:
		{
			CFullscreen *pFullscreen=GetThis(hwnd);

			pFullscreen->OnMouseMove();
		}
		return 0;

	case WM_TIMER:
		{
			CFullscreen *pFullscreen=GetThis(hwnd);

			if (!pFullscreen->m_fMenu) {
				::SetCursor(NULL);
				CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(true);
				pFullscreen->m_fShowCursor=false;
			}
			::KillTimer(hwnd,1);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CFullscreen *pFullscreen=GetThis(hwnd);

			::SetCursor(pFullscreen->m_fShowCursor?
											LoadCursor(NULL,IDC_ARROW):NULL);
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
		{
			CFullscreen *pFullscreen=GetThis(hwnd);

			MainWindow.OnMouseWheel(wParam,lParam,pFullscreen->m_fShowStatusView);
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
			CFullscreen *pFullscreen=GetThis(hwnd);
			SIZE sz;

			ViewWindow.GetClientSize(&sz);
			VideoContainerWindow.SetParent(&ViewWindow);
			ViewWindow.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			//if (!m_fShowCursor)
				CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO);
			pFullscreen->ShowStatusView(false);
			pFullscreen->OnDestroy();
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


bool CFullscreen::Create(HWND hwndOwner)
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
	return Create(hwndOwner,WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


CFullscreen::CFullscreen()
{
}


CFullscreen::~CFullscreen()
{
	/*
	if (m_hwnd!=NULL)
		SendMessage(m_hwnd,WM_CLOSE,0,0);
	*/
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
	RECT rc;

	if (m_fMenu)
		return;
	if (!m_fShowCursor) {
		::SetCursor(LoadCursor(NULL,IDC_ARROW));
		CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
		m_fShowCursor=true;
	}
	GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	GetClientRect(&rc);
	rc.top=rc.bottom-StatusView.Height();
	if (::PtInRect(&rc,pt)) {
		if (!m_fShowStatusView) {
			ShowStatusView(true);
			::KillTimer(m_hwnd,1);
		}
	} else {
		if (m_fShowStatusView)
			ShowStatusView(false);
		::SetTimer(m_hwnd,1,1000,NULL);
	}
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;
	if (fShow) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.Height();
		StatusView.SetVisible(false);
		StatusView.SetParent(&VideoContainerWindow);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
	} else {
		SIZE sz;

		StatusView.SetVisible(false);
		StatusView.SetParent(&MainWindow);
		MainWindow.GetClientSize(&sz);
		MainWindow.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
		StatusView.SetVisible(true);
	}
	m_fShowStatusView=fShow;
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
			int NumServices,i;
			TCHAR szServiceName[1024];
			LPTSTR *ppszList;
			bool fReady=false;

			NumServices=pProgManager->GetServiceNum();
			ppszList=new LPTSTR[NumServices];
			for (i=0;i<NumServices;i++) {
				if (!pProgManager->GetServiceName(szServiceName,i)) {
					lstrcpy(szServiceName,TEXT("サービス名取得中..."));
					fReady=true;
				}
				ppszList[i]=new TCHAR[lstrlen(szServiceName)+1];
				lstrcpy(ppszList[i],szServiceName);
			}
			MainWindow.PostMessage(WM_APP_SERVICEUPDATE,
				MAKEWPARAM(MAKEWORD(pEngine->GetService(),NumServices),fReady),
															(LPARAM)ppszList);
		}
		break;
	case CDtvEngine::EID_FILE_WRITE_ERROR:
		MainWindow.PostMessage(WM_APP_FILEWRITEERROR,0,0);
		break;
	}
	return 0;
}


static MyDtvEngineHandler DtvEngineHandler;




class CMyCapturePreviewEvent : public CCapturePreviewEvent {
public:
	bool OnClose();
	bool OnSave(CCaptureImage *pImage);
};

bool CMyCapturePreviewEvent::OnClose()
{
	fShowCapturePreview=false;
	MainMenu.CheckItem(CM_CAPTUREPREVIEW,false);
	m_pCapturePreview->ClearImage();
	return true;
}

bool CMyCapturePreviewEvent::OnSave(CCaptureImage *pImage)
{
	return CaptureOptions.SaveImage(pImage);
}


static CMyCapturePreviewEvent CapturePreviewEvent;




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
	return RegisterClass(&wc)!=0;
}


CMainWindow::CMainWindow()
{
	// 適当にデフォルト位置を設定
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
	m_WindowPosition.Left=(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
	m_fFullscreen=false;
	m_fMaximize=false;
	m_fAlwaysOnTop=false;
	m_fShowStatusBar=true;
	m_fShowTitleBar=true;
	m_fEnablePreview=false;
	m_fStandby=false;
	m_fProgramGuideUpdating=false;
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


bool CMainWindow::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (fFullscreen) {
			pFullscreen=new CFullscreen;
			if (!pFullscreen->Create(m_hwnd)) {
				delete pFullscreen;
				pFullscreen=NULL;
				return false;
			}
		} else {
			ForegroundWindow(m_hwnd);
			SAFE_DELETE(pFullscreen);
		}
		m_fFullscreen=fFullscreen;
		MainMenu.CheckItem(CM_FULLSCREEN,fFullscreen);
	}
	return true;
}


HWND CMainWindow::GetVideoHostWindow() const
{
	if (m_fFullscreen)
		return pFullscreen->GetHandle();
	return m_hwnd;
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
	if (fClientEdge) {
		rc.right+=GetSystemMetrics(SM_CXEDGE)*2;
		rc.bottom+=GetSystemMetrics(SM_CYEDGE)*2;
	}
	if (m_fShowStatusBar)
		rc.bottom+=StatusView.Height();
	if (fShowPanelWindow && !PanelFrame.GetFloating())
		rc.right+=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
	::AdjustWindowRectEx(&rc,GetStyle(),FALSE,GetExStyle());
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
					rc.bottom+=StatusView.Height();
				} else {
					rc.bottom-=StatusView.Height();
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
	ViewWindow.Create(Splitter.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,
		fClientEdge?WS_EX_CLIENTEDGE:0,IDC_VIEW);
	ViewWindow.SetMessageWindow(m_hwnd);
	Splitter.SetPane(!PanelPaneIndex,&ViewWindow,PANE_ID_VIEW);
	Splitter.SetMinSize(PANE_ID_VIEW,32);
	Splitter.SetPaneVisible(PANE_ID_VIEW,true);
	VideoContainerWindow.Create(ViewWindow.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,0,IDC_VIDEOCONTAINER);
	ViewWindow.SetVideoContainer(&VideoContainerWindow);
	StatusView.Create(m_hwnd,
		WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		/*WS_EX_CLIENTEDGE*/WS_EX_STATICEDGE,IDC_STATUS);
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
	StatusOptions.ApplyOptions();
	StatusView.SetSingleText(TEXT("起動中..."));

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
							CM_CAPTURESIZE_FIRST+CaptureOptions.GetCaptureSize());
	MainMenu.CheckItem(CM_CAPTUREPREVIEW,fShowCapturePreview);
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
				{ 0,0,0},
				{16,9,0},
				{16,9,CMediaViewer::PANANDSCAN_VERT},
				{16,9,CMediaViewer::PANANDSCAN_HORZ |
												CMediaViewer::PANANDSCAN_VERT},
				{ 4,3,CMediaViewer::PANANDSCAN_HORZ},
				{ 4,3,0},
			};
			int i=id-CM_ASPECTRATIO_FIRST;
			int Zoom=CalcZoomRate();

			CoreEngine.m_DtvEngine.m_MediaViewer.ForceAspectRatio(
					AspectRatioList[i].XAspect,AspectRatioList[i].YAspect);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(
												AspectRatioList[i].PanAndScan);
			CoreEngine.m_DtvEngine.m_MediaViewer.ResizeVideoWindow();
			if (!m_fFullscreen && !::IsZoomed(hwnd)) {
				if (!fPanScanNoResizeWindow) {
					int Width,Height;

					if (Zoom>0 && CoreEngine.GetVideoViewSize(&Width,&Height)) {
						AdjustWindowSize(Width*Zoom/100,Height*Zoom/100);
					} else {
						WORD w,h;

						if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&w,&h))
							AdjustWindowSize(w,h);
					}
				} else
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
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
				Volume=CoreEngine.GetVolume()+5;
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume=CoreEngine.GetVolume()-5;
				if (Volume<0)
					Volume=0;
			}
			if (Volume!=CoreEngine.GetVolume() || CoreEngine.GetMute())
				SetVolume(Volume);
		}
		return;

	case CM_VOLUME_MUTE:
		CoreEngine.SetMute(!CoreEngine.GetMute());
		StatusView.UpdateItem(STATUS_ITEM_VOLUME);
		ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
		MainMenu.CheckItem(CM_VOLUME_MUTE,CoreEngine.GetMute());
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
	case CM_STEREOMODE:
		{
			int Mode=CoreEngine.GetStereoMode();

			if (id==CM_STEREOMODE)
				Mode=(Mode+1)%3;
			else
				Mode=id-CM_STEREO_THROUGH;
			CoreEngine.SetStereoMode(Mode);
			MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
									CM_STEREO_THROUGH+Mode);
			StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
		}
		return;

	case CM_CAPTURE:
		SendCommand(CaptureOptions.TranslateCommand(CM_CAPTURE));
		return;

	case CM_COPY:
	case CM_SAVEIMAGE:
		{
			BYTE *pDib;

			pDib=static_cast<BYTE*>(CoreEngine.GetCurrentImage());
			if (pDib==NULL) {
				MessageBox(GetVideoHostWindow(),
						TEXT("現在の画像を取得できません。"),TEXT("ごめん"),
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
				switch (CaptureOptions.GetCaptureSize()) {
				case CCaptureOptions::SIZE_ORIGINAL:
					CoreEngine.GetVideoViewSize(&Width,&Height);
					break;
				case CCaptureOptions::SIZE_VIEW:
					{
						WORD w,h;

						CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&w,&h);
						Width=w;
						Height=h;
					}
					break;
				case CCaptureOptions::SIZE_RAW:
					rc.left=rc.top=0;
					rc.right=OrigWidth;
					rc.bottom=OrigHeight;
					Width=OrigWidth;
					Height=OrigHeight;
					break;
				default:
					CaptureOptions.GetCustomSize(CaptureOptions.GetCaptureSize(),
												 &Width,&Height);
				}
				hGlobal=ResizeImage((BITMAPINFO*)pbmih,
								pDib+CalcDIBInfoSize(pbmih),&rc,Width,Height);
				CoTaskMemFree(pDib);
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
				CapturePreview.SetImage(pImage);
				if (id==CM_COPY) {
					if (!pImage->SetClipboard(hwnd)) {
						MessageBox(GetVideoHostWindow(),
							TEXT("クリップボードにデータを設定できません。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
					}
				} else {
					if (!CaptureOptions.SaveImage(pImage)) {
						MessageBox(GetVideoHostWindow(),
							TEXT("画像の保存でエラーが発生しました。"),NULL,
							MB_OK | MB_ICONEXCLAMATION);
					}
				}
				if (!CapturePreview.HasImage())
					delete pImage;
			}
		}
		return;

	case CM_CAPTUREPREVIEW:
		fShowCapturePreview=!fShowCapturePreview;
		if (fShowCapturePreview) {
			CapturePreview.Create(hwnd,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
																	WS_VISIBLE,
				WS_EX_TOOLWINDOW);
		} else {
			CapturePreview.Destroy();
			CapturePreview.ClearImage();
		}
		MainMenu.CheckItem(CM_CAPTUREPREVIEW,fShowCapturePreview);
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
			RecordManager.StopRecord();
			InfoWindow.SetRecordStatus(false);
			ResidentManager.SetStatus(0,CResidentManager::STATUS_RECORDING);
			Logger.AddLog(TEXT("録画停止"));
			CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(fDescrambleCurServiceOnly);
			if (m_fStandby) {
				CoreEngine.m_DtvEngine.ReleaseSrcFilter();
				m_fSrcFilterReleased=true;
			}
		} else {
			TCHAR szFileName[MAX_PATH];
			LPCTSTR pszErrorMessage;

			if (!RecordOptions.GenerateFileName(szFileName,lengthof(szFileName),
												NULL,&pszErrorMessage)) {
				MessageBox(GetVideoHostWindow(),pszErrorMessage,NULL,
												MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			RecordManager.SetDescrambleCurServiceOnly(RecordOptions.GetDescrambleCurServiceOnly());
			if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
				MessageBox(GetVideoHostWindow(),
										TEXT("ファイルが開けません。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			RecordManager.SetStopTimeSpec(false);
			RecordManager.SetStopTime(60*60*1000);
			ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
									  CResidentManager::STATUS_RECORDING);
			Logger.AddLog(TEXT("録画開始 %s"),szFileName);
		}
		StatusView.UpdateItem(STATUS_ITEM_RECORD);
		MainMenu.EnableItem(CM_RECORDOPTION,!RecordManager.IsRecording());
		MainMenu.EnableItem(CM_RECORDSTOPTIME,RecordManager.IsRecording());
		return;

	case CM_RECORD_PAUSE:
		if (RecordManager.IsRecording()) {
			RecordManager.PauseRecord();
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
			Logger.AddLog(RecordManager.IsPaused()?TEXT("録画一時停止"):TEXT("録画再開"));
		}
		return;

	case CM_RECORDOPTION:
		if (IsWindowEnabled(GetVideoHostWindow())) {
			RecordManager.SetDescrambleCurServiceOnly(RecordOptions.GetDescrambleCurServiceOnly());
			if (RecordManager.RecordDialog(GetVideoHostWindow(),hInst)) {
				TCHAR szFileName[MAX_PATH];

				if (RecordManager.DoFileExistsOperation(
											GetVideoHostWindow(),szFileName)) {
					// 録画中は選択できないはずだが、一応
					if (RecordManager.IsRecording())
						SendCommand(CM_RECORD_STOP);
					if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
						MessageBox(GetVideoHostWindow(),
										TEXT("ファイルが開けません。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
						return;
					}
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
					ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
											  CResidentManager::STATUS_RECORDING);
					Logger.AddLog(TEXT("録画開始 %s"),szFileName);
					MainMenu.EnableItem(CM_RECORDOPTION,false);
					MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
				}
			}
		}
		return;

	case CM_RECORDSTOPTIME:
		if (RecordManager.ChangeStopTimeDialog(GetVideoHostWindow(),hInst)) {
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
		}
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
			if (!CoreEngine.IsUDPDriver())
				pList=ChannelManager.GetCurrentChannelList();
			else
				pList=ChannelManager.GetFileAllChannelList();
			if (pList!=NULL) {
				CProgramGuideServiceIDList ServiceIDList;

				for (int i=0;i<pList->NumChannels();i++) {
					WORD ServiceID=pList->GetChannelInfo(i)->GetServiceID();
					if (ServiceID!=0) {
						ServiceIDList.Add(ServiceID);
						EpgProgramList.UpdateProgramList(ServiceID);
					}
				}
				ProgramGuide.SetServiceIDList(&ServiceIDList);
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

	case CM_CLOSE:
		if (m_fStandby) {
			SetStandby(false);
		} else {
			SendMessage(WM_CLOSE,0,0);
		}
		return;

	case CM_EXIT:
		if (ConfirmExit()) {
			Destroy();
		}
		return;

	case CM_SHOW:
		SetStandby(false);
		return;

	case CM_CHANNEL_UP:
	case CM_CHANNEL_DOWN:
		{
			const CChannelInfo *pInfo=ChannelManager.GetNextChannelInfo(id==CM_CHANNEL_UP);

			if (pInfo!=NULL)
				SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
			else
				SendCommand(CM_CHANNEL_FIRST);
		}
		return;

	case CM_MENU:
		{
			POINT pt;

			pt.x=0;
			pt.y=0;
			::ClientToScreen(ViewWindow.GetHandle(),&pt);
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
		if (id>=CM_CAPTURESIZE_FIRST && id<=CM_CAPTURESIZE_LAST) {
			int CaptureSize=id-CM_CAPTURESIZE_FIRST;

			CaptureOptions.SetCaptureSize(CaptureSize);
			MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
									CM_CAPTURESIZE_FIRST+CaptureSize);
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
			} else {
				int Index=ChannelManager.GetCurrentChannelList()->FindChannelNo(No+1);

				if (Index>=0)
					SendCommand(CM_CHANNEL_FIRST+Index);
			}
			return;
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
					if (AppMain.SetDriver(pDriverInfo->GetFileName()))
						::lstrcpy(szDriverFileName,pDriverInfo->GetFileName());
				}
			}
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

			DWORD UpdateStatus=CoreEngine.UpdateAsyncStatus();
			DWORD UpdateStatistics=CoreEngine.UpdateStatistics();

			if ((UpdateStatus&CCoreEngine::STATUS_VIDEOSIZE)!=0) {
				WORD Width,Height;

				StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				if (!CoreEngine.m_DtvEngine.GetVideoSize(&Width,&Height))
					Width=Height=0;
				InfoWindow.SetVideoSize(Width,Height);
				CheckZoomMenu();
			}

			if ((UpdateStatus&CCoreEngine::STATUS_AUDIOCHANNELS)!=0) {
				SendCommand(CM_STEREO_THROUGH);
				//StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
			}

			if (RecordManager.IsRecording()) {
				if (RecordManager.QueryStop()) {
					SendCommand(CM_RECORD_STOP);
				} else if (!RecordManager.IsPaused()) {
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
				StatusView.UpdateItem(STATUS_ITEM_ERROR);
			}

			if (TimerCount%4==0) {	// 負荷軽減
				CoreEngine.UpdateEpgDataInfo();

				const CEpgDataInfo *pInfo=CoreEngine.GetEpgDataInfo();

				if (pInfo!=NULL) {
					WCHAR szText[2048];
					const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();

					if (pChInfo!=NULL)
						::wsprintf(szText,TITLE_TEXT TEXT(" - %s / %s"),
									pChInfo->GetName(),pInfo->GetEventName());
					else
						::lstrcpy(szText,TITLE_TEXT);
					::GetWindowText(MainWindow.GetHandle(),szText+lengthof(szText)/2,lengthof(szText)/2);
					if (::lstrcmp(szText,szText+lengthof(szText)/2)!=0)
						::SetWindowText(MainWindow.GetHandle(),szText);

					CProgramInfoStatusItem *pPgInfoItem=
						dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
					if (pPgInfoItem!=NULL && pPgInfoItem->GetVisible()) {
						SYSTEMTIME stStart,stEnd;

						pInfo->GetStartTime(&stStart);
						pInfo->GetEndTime(&stEnd);
						::_snwprintf(szText,lengthof(szText),
							L"%d:%02d～%d:%02d %s",
							stStart.wHour,
							stStart.wMinute,
							stEnd.wHour,
							stEnd.wMinute,
							pInfo->GetEventName());
						pPgInfoItem->SetProgramInfo(szText);
					}
					if (fShowPanelWindow) {
						if (InfoWindow.GetProgramInfoNext()) {
							delete pInfo;
							pInfo=CoreEngine.m_DtvEngine.GetEpgDataInfo(
								ChannelManager.GetCurrentService(),true);
						}
						if (pInfo!=NULL) {
							SYSTEMTIME stStart,stEnd;

							pInfo->GetStartTime(&stStart);
							pInfo->GetEndTime(&stEnd);
							::_snwprintf(szText,lengthof(szText),
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
				if (CoreEngine.m_DtvEngine.GetVideoAspectRatio(&AspectX,&AspectY))
					InfoWindow.SetAspectRatio(AspectX,AspectY);

				if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
									 | CCoreEngine::STATISTIC_BITRATE))!=0) {
					CCoreEngine::DriverType DriverType=CoreEngine.GetDriverType();
					if (DriverType==CCoreEngine::DRIVER_UDP
							|| (DriverType==CCoreEngine::DRIVER_HDUS && !fNewHDUSDriver)) {
						InfoWindow.SetBitRate(CoreEngine.GetSignalLevel());
						InfoWindow.ShowSignalLevel(false);
					} else {
						InfoWindow.SetSignalLevel(CoreEngine.GetSignalLevel());
						InfoWindow.SetBitRate(CoreEngine.GetBitRateFloat());
					}
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
			if (pInfo!=NULL)
				SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
			::KillTimer(hwnd,TIMER_ID_WHEELCHANNELCHANGE);
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		if (fShowPanelWindow && InfoPanel.GetCurTab()==PANEL_TAB_PROGRAMLIST) {
			WORD ServiceID;

			if (CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,
										ChannelManager.GetCurrentService()))
				ProgramListView.UpdateProgramList(ServiceID);
		}
		ProgramListUpdateTimerCount++;
		if (ProgramListUpdateTimerCount==12)
			// 更新頻度を下げる
			::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,3*60*1000,NULL);
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		if (!m_fStandby)
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
		const CChannelInfo *pChInfo=ChannelManager.GetNextChannelInfo(true);
		if (pChInfo==NULL
				|| pChInfo->GetChannelIndex()==m_ProgramGuideUpdateStartChannel)
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
		else
			PostCommand(CM_CHANNEL_UP);
		break;
	}
}


void CMainWindow::OnChannelChange()
{
	const CChannelInfo *pInfo;

	SetTitleText();
	if (fUseOSD)
		ShowChannelOSD();
	ProgramListView.ClearProgramList();
	BeginProgramListUpdateTimer();
	if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL)
		ControlPanel.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
								CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
}


void CMainWindow::ShowChannelOSD()
{
	const CChannelInfo *pInfo;
	TCHAR szText[4+MAX_CHANNEL_NAME];

	if (fWheelChannelChanging)
		pInfo=ChannelManager.GetChangingChannelInfo();
	else
		pInfo=ChannelManager.GetCurrentChannelInfo();
	if (pInfo!=NULL) {
		wsprintf(szText,TEXT("%d"),pInfo->GetChannelNo());
		if (!fUsePseudoOSD
				&& CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_VMR9) {
			LOGFONT lf;
			HFONT hfont;
			RECT rc;

			GetObject(GetStockObject(DEFAULT_GUI_FONT),
											sizeof(LOGFONT),&lf);
			lf.lfHeight=80;
			lf.lfQuality=NONANTIALIASED_QUALITY;
			hfont=CreateFontIndirect(&lf);
			rc.left=200;
			rc.top=120;
			rc.right=rc.left+80;
			rc.bottom=rc.top+100;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,hfont,
										crOSDTextColor,80,&rc)) {
				if (OSDFadeTime>0)
					SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_OSD,OSDFadeTime,NULL);
			}
			DeleteObject(hfont);
		} else {
			SIZE sz;
			COLORREF cr;

			wsprintf(szText+lstrlen(szText),TEXT(" %s"),pInfo->GetName());
			PseudoOSD.Create(VideoContainerWindow.GetHandle());
			PseudoOSD.SetTextHeight(32);
			PseudoOSD.SetText(szText);
			PseudoOSD.CalcTextSize(&sz);
			if (fWheelChannelChanging)
				cr=RGB(GetRValue(crOSDTextColor)/2,GetGValue(crOSDTextColor)/2,
					   GetBValue(crOSDTextColor)/2);
			else
				cr=crOSDTextColor;
			PseudoOSD.SetTextColor(cr);
			PseudoOSD.SetPosition(8,8,sz.cx+8,sz.cy+8);
			PseudoOSD.Show(OSDFadeTime);
		}
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
				if (fUseOSD)
					ShowChannelOSD();
				SetTimer(m_hwnd,TIMER_ID_WHEELCHANNELCHANGE,WheelChannelDelay,NULL);
			}
		}
		break;
	case WHEEL_MODE_STEREOMODE:
		if (CoreEngine.m_DtvEngine.GetAudioChannelNum()==2)
			SendCommand(CM_STEREOMODE);
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


bool CMainWindow::EnablePreview(bool fEnable)
{
	if (m_fEnablePreview!=fEnable) {
		// なぜか成功してもfalseが返ってくる
		//if (!CoreEngine.EnablePreview(fEnable))
		//	return false;
		CoreEngine.EnablePreview(fEnable);
		CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(fEnable);
		VideoContainerWindow.SetVisible(fEnable);
		MainMenu.CheckItem(CM_DISABLEVIEWER,!fEnable);
		m_fEnablePreview=fEnable;
	}
	return true;
}


bool CMainWindow::SetVolume(int Volume,bool fOSD)
{
	if (!CoreEngine.SetVolume(Volume))
		return false;
	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,false);
	if (fUseOSD && fOSD) {
		TCHAR szText[64];
		int i;

		szText[0]='\0';
		for (i=0;i<Volume/5;i++)
			lstrcat(szText,TEXT("■"));
		for (;i<20;i++)
			lstrcat(szText,TEXT("□"));
		if (!fUsePseudoOSD
				&& CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_VMR9) {
			LOGFONT lf;
			HFONT hfont;
			RECT rc;

			GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
			lf.lfHeight=40;
			lf.lfQuality=NONANTIALIASED_QUALITY;
			hfont=CreateFontIndirect(&lf);
			rc.left=220;
			rc.top=880;
			rc.right=rc.left+400;
			rc.bottom=rc.top+40;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,hfont,crOSDTextColor,
																	80,&rc)) {
				if (OSDFadeTime>0)
					SetTimer(MainWindow.GetHandle(),CMainWindow::TIMER_ID_OSD,OSDFadeTime,NULL);
			}
			DeleteObject(hfont);
		} else {
			SIZE sz;
			RECT rc;

			PseudoOSD.Create(VideoContainerWindow.GetHandle());
			PseudoOSD.SetTextHeight(16);
			PseudoOSD.SetText(szText);
			PseudoOSD.CalcTextSize(&sz);
			VideoContainerWindow.GetClientRect(&rc);
			if (StatusView.GetParent()==VideoContainerWindow.GetHandle())
				rc.bottom-=StatusView.Height();
			PseudoOSD.SetPosition(8,rc.bottom-sz.cy-8,sz.cx,sz.cy);
			PseudoOSD.Show(OSDFadeTime);
		}
	}
	return true;
}


void CMainWindow::SetZoomRate(int ZoomNum,int ZoomDenom/*=100*/)
{
	int Width,Height;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0)
		AdjustWindowSize(Width*ZoomNum/ZoomDenom,Height*ZoomNum/ZoomDenom);
}


int CMainWindow::CalcZoomRate()
{
	int Width,Height;
	int Zoom;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		SIZE sz;

		VideoContainerWindow.GetClientSize(&sz);
		Zoom=(sz.cy*100+Height/2)/Height;
	} else
		Zoom=0;
	return Zoom;
}


void CMainWindow::CheckZoomMenu()
{
	int Zoom;
	int i;

	Zoom=CalcZoomRate();
	for (i=0;i<lengthof(ZoomRateList);i++) {
		MainMenu.CheckItem(CM_ZOOM_FIRST+i,ZoomRateList[i].Num*100/ZoomRateList[i].Denom==Zoom);
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
/*
TCHAR sz[64];
if (!GetWindowText(hwndTop,sz,64))
sz[0]='\0';
*/
	if (pRect->top==pRect->bottom) {
/*
TRACE(TEXT("horz edge %d,%d,%d,%d [%d,%d,%d,%d] %08X %s\n"),pRect->left,pRect->top,pRect->right,pRect->bottom,rc.left,rc.top,rc.right,rc.bottom,hwndTop,sz);
*/
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
/*
TRACE(TEXT("vert edge %d,%d,%d,%d [%d,%d,%d,%d] %08X %s\n"),pRect->left,pRect->top,pRect->right,pRect->bottom,rc.left,rc.top,rc.right,rc.bottom,hwndTop,sz);
*/
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
					pInfo->rcNearest.top=rc.bottom-pInfo->rcOriginal.top;
					rcEdge.left=max(rc.left,pInfo->rcOriginal.left);
					rcEdge.right=min(rc.right,pInfo->rcOriginal.right);
					rcEdge.top=rc.bottom;
					rcEdge.bottom=rc.bottom;
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.bottom=rc.top-pInfo->rcOriginal.bottom;
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
	if (abs(XOffset)<=SnapAtWindowEdgeMargin)
		prc->left+=XOffset;
	if (abs(YOffset)<=SnapAtWindowEdgeMargin)
		prc->top+=YOffset;
	prc->right=prc->left+(Info.rcOriginal.right-Info.rcOriginal.left);
	prc->bottom=prc->top+(Info.rcOriginal.bottom-Info.rcOriginal.top);
}


CMainWindow *CMainWindow::GetThis(HWND hwnd)
{
	return reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CMainWindow *pThis=dynamic_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd,lParam));

			if (!pThis->OnCreate())
				return -1;
		}
		return 0;

	case WM_SIZE:
		{
			CMainWindow *pThis=GetThis(hwnd);

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

			int Height;

			Height=HIWORD(lParam);
			if (pThis->m_fShowStatusBar && StatusView.GetParent()==hwnd) {
				Height-=StatusView.Height();
				StatusView.SetPosition(0,Height,LOWORD(lParam),StatusView.Height());
			}
			Splitter.SetPosition(0,0,LOWORD(lParam),Height);
			if (!pThis->m_fFullscreen) {
				if (wParam==SIZE_MAXIMIZED)
					CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(MaximizeStretchMode);
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
			if (GetKeyState(VK_SHIFT)<0?!fAdjustAspectResizing:
													fAdjustAspectResizing) {
				BYTE XAspect,YAspect;

				if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(
														&XAspect,&YAspect)) {
					RECT rcWindow,rcClient;
					int XMargin,YMargin,Width,Height;

					pThis->GetPosition(&rcWindow);
					pThis->GetClientRect(&rcClient);
					if (fClientEdge) {
						rcClient.right-=GetSystemMetrics(SM_CXEDGE)*2;
						rcClient.bottom-=GetSystemMetrics(SM_CYEDGE)*2;
					}
					if (pThis->m_fShowStatusBar)
						rcClient.bottom-=StatusView.Height();
					if (fShowPanelWindow && !PanelFrame.GetFloating()) {
						rcClient.right-=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
					}
					if (rcClient.right<=0 || rcClient.bottom<=0)
						goto MovingEnd;
					XMargin=(rcWindow.right-rcWindow.left)-rcClient.right;
					YMargin=(rcWindow.bottom-rcWindow.top)-rcClient.bottom;
					Width=(prc->right-prc->left)-XMargin;
					Height=(prc->bottom-prc->top)-YMargin;
					if (Width<=0 || Height<=0)
						goto MovingEnd;
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
		MovingEnd:
			PanelStatus.OnOwnerMovingOrSizing(&rcOld,prc);
			if (fChanged)
				return TRUE;
		}
		break;

	case WM_RBUTTONDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pFullscreen->OnRButtonDown();
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
		if (::GetCapture()==hwnd)
			::ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pFullscreen->OnMouseMove();
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
				if (::GetKeyState(VK_SHIFT)<0?!fSnapAtWindowEdge:fSnapAtWindowEdge)
					SnapWindow(hwnd,&rc);
				pThis->SetPosition(&rc);
				PanelStatus.OnOwnerMovingOrSizing(&rcOld,&rc);
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		GetThis(hwnd)->SendCommand(CM_FULLSCREEN);
		return 0;

	case WM_KEYDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);
			int Command;

			if (wParam>=VK_F1 && wParam<=VK_F12) {
				if (!fFunctionKeyChangeChannel)
					break;
				Command=CM_CHANNELNO_FIRST+(wParam-VK_F1);
			} else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9) {
				if (!fNumPadChangeChannel)
					break;
				if (wParam==VK_NUMPAD0)
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+(wParam-VK_NUMPAD1);
			} else if (wParam>='0' && wParam<='9') {
				if (!fDigitKeyChangeChannel)
					break;
				if (wParam=='0')
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+(wParam-'1');
			} else {
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
					{'S',		CM_STEREOMODE},
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

	case WM_SYSCOMMAND:
		switch (wParam) {
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
		}
		break;

	case WM_APPCOMMAND:
		return GetThis(hwnd)->OnAppCommand(hwnd,(HWND)wParam,
					GET_APPCOMMAND_LPARAM(lParam),GET_DEVICE_LPARAM(lParam),
					GET_KEYSTATE_LPARAM(lParam));

	case WM_APP_SERVICEUPDATE:
		{
			HMENU hmenu=MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE);
			int CurService=LOWORD(LOBYTE(wParam));
			int NumServices=LOWORD(HIBYTE(wParam));
			LPTSTR *ppszList=(LPTSTR*)lParam;
			int i;

			ClearMenu(hmenu);
			for (i=0;i<NumServices;i++) {
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SERVICE_FIRST+i,
																ppszList[i]);
				delete [] ppszList[i];
			}
			delete [] ppszList;
			const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
			if (ChannelManager.GetCurrentService()<0
					&& pChInfo!=NULL && pChInfo->GetServiceID()!=0) {
				WORD ServiceID;

				if (CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,CurService)) {
					if (ServiceID!=pChInfo->GetServiceID()) {
						AppMain.SetServiceByID(pChInfo->GetServiceID(),&CurService);
					}
				}
			}
			if (HIWORD(wParam)==0) {
				MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
										CM_SERVICE_FIRST+NumServices-1,
										CM_SERVICE_FIRST+CurService);
				if (pChInfo!=NULL) {
					WORD ServiceID;

					for (i=0;i<NumServices;i++) {
						if (CoreEngine.m_DtvEngine.m_ProgManager.GetServiceID(&ServiceID,i)) {
							ChannelManager.SetServiceID(pChInfo->GetSpace(),
										pChInfo->GetChannelIndex(),i,ServiceID);
						}
					}
				}
			}
			/*
			if (fUseOSD && HIWORD(wParam)==0)
				ShowChannelOSD();
			*/
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
					MF_BYCOMMAND | (pThis->m_fStandby?MFS_ENABLED:MFS_DISABLED));
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

				if (pThis->m_fStandby)
					pThis->SetStandby(false);
				else
					ForegroundWindow(hwnd);
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

			if (pThis!=NULL && !pThis->m_fClosing && CoreEngine.IsUDPDriver()) {
				WORD Port=ChannelManager.GetCurrentChannel()+1234;
				WORD RemoconPort=pNetworkRemocon!=NULL?pNetworkRemocon->GetPort():0;
				return MAKELRESULT(Port,RemoconPort);
			}
		}
		return 0;

	case WM_APP_FILEWRITEERROR:
		::MessageBox(MainWindow.GetVideoHostWindow(),
					 TEXT("ファイルへの書き出しでエラーが発生しました。"),NULL,
					 MB_OK | MB_ICONEXCLAMATION);
		return 0;

	case WM_DISPLAYCHANGE:
		CoreEngine.m_DtvEngine.m_MediaViewer.DisplayModeChanged();
		break;

	case WM_CLOSE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (ResidentManager.GetResident()) {
				pThis->SetStandby(true);
				return 0;
			}
			if (!pThis->ConfirmExit())
				return 0;
			pThis->m_fClosing=true;
		}
		break;

	case WM_DESTROY:
		{
			CMainWindow *pThis=GetThis(hwnd);

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

			SAFE_DELETE(pFullscreen);
			SAFE_DELETE(pNetworkRemocon);
			SAFE_DELETE(pRemoteController);
			InfoWindow.Destroy();
			ProgramGuide.Destroy();
			ResidentManager.Finalize();
			HtmlHelpClass.Finalize();

			RestoreChannelInfo.Space=-1;
			RestoreChannelInfo.Channel=-1;
			RestoreChannelInfo.Service=0;
			::lstrcpy(RestoreChannelInfo.szDriverName,CoreEngine.GetDriverFileName());
			if (!CoreEngine.IsUDPDriver()) {
				const CChannelInfo *pInfo=ChannelManager.GetCurrentChannelInfo();

				if (pInfo!=NULL) {
					RestoreChannelInfo.Space=pInfo->GetSpace();
					RestoreChannelInfo.Channel=pInfo->GetChannelIndex();
					RestoreChannelInfo.Service=pInfo->GetService();
				}
			}
			pThis->m_fMaximize=pThis->GetMaximize();
			AppMain.Finalize();
			EpgOptions.SaveEpgFile(&EpgProgramList);

			MainMenu.Destroy();

			CoreEngine.Close();

			pThis->OnDestroy();
			::PostQuitMessage(0);
		}
		return 0;

	HANDLE_MSG(hwnd,WM_COMMAND,GetThis(hwnd)->OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,GetThis(hwnd)->OnTimer);

	default:
		if (pRemoteController!=NULL
				&& pRemoteController->TranslateMessage(uMsg,wParam,lParam))
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
	} else
		::SetWindowText(m_hwnd,TITLE_TEXT);
}


bool CMainWindow::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (fStandby) {
			if (m_fEnablePreview) {
				CoreEngine.EnablePreview(false);
				//CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(false);
				VideoContainerWindow.SetVisible(false);
			}
			m_fRestoreFullscreen=m_fFullscreen;
			if (m_fFullscreen)
				SetFullscreen(false);
			if (EpgOptions.GetUpdateWhenStandby() && !CoreEngine.IsUDPDriver())
				BeginProgramGuideUpdate();
			if (fShowPanelWindow && PanelFrame.GetFloating())
				PanelFrame.SetVisible(false);
			if (fShowProgramGuide)
				ProgramGuide.SetVisible(false);
			if (fShowCapturePreview)
				CapturePreview.SetVisible(false);
			SetVisible(false);
			if (!RecordManager.IsRecording() && !m_fProgramGuideUpdating) {
				CoreEngine.m_DtvEngine.ReleaseSrcFilter();
				m_fSrcFilterReleased=true;
			} else
				m_fSrcFilterReleased=false;
		} else {
			SetVisible(true);
			Update();
			if (m_fRestoreFullscreen)
				SetFullscreen(true);
			if (fShowPanelWindow && PanelFrame.GetFloating()) {
				PanelFrame.SetVisible(true);
				PanelFrame.Update();
			}
			if (fShowProgramGuide)
				ProgramGuide.SetVisible(true);
			if (fShowCapturePreview)
				CapturePreview.SetVisible(true);
			ForegroundWindow(m_hwnd);
			if (m_fSrcFilterReleased) {
				::SetCursor(LoadCursor(NULL,IDC_WAIT));
				CoreEngine.m_DtvEngine.SetTracer(&StatusView);
				CoreEngine.OpenDriver();
				CoreEngine.m_DtvEngine.SetTracer(NULL);
				StatusView.SetSingleText(NULL);
				::SetCursor(LoadCursor(NULL,IDC_ARROW));
			}
			if (m_fSrcFilterReleased) {
				const CChannelList *pList=ChannelManager.GetChannelList(ChannelManager.GetCurrentSpace());
				if (pList!=NULL) {
					const CChannelInfo *pChannelInfo=
						pList->GetChannelInfo(ChannelManager.GetCurrentChannel());
					if (pChannelInfo!=NULL) {
						CoreEngine.m_DtvEngine.SetChannel(pChannelInfo->GetSpace(),pChannelInfo->GetChannelIndex());
					}
				}
			}
			if (m_fEnablePreview) {
				CoreEngine.EnablePreview(true);
				//CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(true);
				VideoContainerWindow.SetVisible(true);
			}
		}
		m_fStandby=fStandby;
	}
	return true;
}


bool CMainWindow::ConfirmExit()
{
	if (RecordManager.IsRecording()
			&& !RecordOptions.ConfirmExit(GetVideoHostWindow())) {
		return false;
	}
	return true;
}


bool CMainWindow::OnExecute(LPCTSTR pszCmdLine)
{
	CCommandLineParser CmdLineParser;

	CmdLineParser.Parse(pszCmdLine);
	if (m_fStandby) {
		SetStandby(false);
	} else {
		if (::IsIconic(m_hwnd))
			::ShowWindow(m_hwnd,SW_RESTORE);
		else
			ForegroundWindow(m_hwnd);
	}
	if (CmdLineParser.m_fFullscreen)
		SetFullscreen(true);
	if (CmdLineParser.m_Channel>0 || CmdLineParser.m_ControllerChannel>0) {
		if (CmdLineParser.m_Channel>0) {
			const CChannelList *pList=ChannelManager.GetChannelList(ChannelManager.GetCurrentSpace());
			int i;

			i=pList->FindChannel(CmdLineParser.m_Channel);
			if (i>=0)
				SendCommand(CM_CHANNEL_FIRST+i);
		} else {
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();

			if (pList->FindChannelNo(CmdLineParser.m_ControllerChannel)>=0)
				SendCommand(CM_CHANNELNO_FIRST+CmdLineParser.m_ControllerChannel-1);
		}
	}
	if (CmdLineParser.m_fRecord)
		PostCommand(CM_RECORD_START);
	return true;
}


bool CMainWindow::BeginProgramGuideUpdate()
{
	if (!m_fProgramGuideUpdating) {
		if (m_fStandby && m_fSrcFilterReleased) {
			CoreEngine.OpenDriver();
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


void CMainWindow::EndProgramGuideUpdate()
{
	if (m_fProgramGuideUpdating) {
		::KillTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE);
		EpgProgramList.UpdateProgramList();
		EpgOptions.SaveEpgFile(&EpgProgramList);
		m_fProgramGuideUpdating=false;
		if (m_fStandby) {
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			CoreEngine.m_DtvEngine.ReleaseSrcFilter();
			m_fSrcFilterReleased=true;
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




#include <vector>

class CPortQuery {
	std::vector<WORD> m_UDPPortList;
	std::vector<WORD> m_RemoconPortList;
	static BOOL CALLBACK EnumProc(HWND hwnd,LPARAM lParam);
public:
	bool Query(WORD *pUDPPort,WORD *pRemoconPort);
};

bool CPortQuery::Query(WORD *pUDPPort,WORD *pRemoconPort)
{
	size_t i;

	m_UDPPortList.clear();
	m_RemoconPortList.clear();
	::EnumWindows(EnumProc,reinterpret_cast<LPARAM>(this));
	if (m_UDPPortList.size()>0) {
		WORD UDPPort;

		for (UDPPort=*pUDPPort;UDPPort<=1243;UDPPort++) {
			for (i=0;i<m_UDPPortList.size();i++) {
				if (m_UDPPortList[i]==UDPPort)
					break;
			}
			if (i==m_UDPPortList.size())
				break;
		}
		if (UDPPort>1243)
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
			Logger.AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました (UDPポート %d / リモコンポート %d)"),UDPPort,RemoconPort);
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
		m_hMutex=CreateMutex(NULL,TRUE,APP_NAME TEXT(" Mutex"));
		m_fAlreadyExists=m_hMutex!=NULL && GetLastError()==ERROR_ALREADY_EXISTS;
	} else {
		m_hMutex=NULL;
		m_fAlreadyExists=false;
	}
}

CAppMutex::~CAppMutex()
{
	if (m_hMutex!=NULL) {
		if (!m_fAlreadyExists)
			ReleaseMutex(m_hMutex);
		CloseHandle(m_hMutex);
	}
}


int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
												LPTSTR pszCmdLine,int nCmdShow)
{
#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#endif

	Logger.AddLog(TEXT("起動"));

	hInst=hInstance;

	AppMain.Initialize();

	if (pszCmdLine[0]!='\0')
		CmdLineParser.Parse(pszCmdLine);

	CAppMutex Mutex(fKeepSingleTask || CmdLineParser.m_fSchedule);

	// 複数起動のチェック
	if (Mutex.AlreadyExists()) {
		HWND hwnd;

		hwnd=::FindWindow(MAIN_WINDOW_CLASS,NULL);
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
			MessageBox(NULL,
				APP_NAME TEXT(" は既に起動しています。\r\n")
				TEXT("ウィンドウが見当たらない場合はタスクマネージャに隠れていますので\r\n")
				TEXT("強制終了させてください。"),TITLE_TEXT,
												MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	{
		TCHAR szDirectory[MAX_PATH];

		AppMain.GetAppDirectory(szDirectory);
		DriverManager.Find(szDirectory);
	}

	if (CmdLineParser.m_fInitialSettings
			|| (AppMain.IsFirstExecute() && CmdLineParser.m_szDriverName[0]=='\0')) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.ShowDialog(NULL))
			return FALSE;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		InitialSettings.GetMpeg2DecoderName(szMpeg2DecoderName,lengthof(szMpeg2DecoderName));
		VideoRendererType=InitialSettings.GetVideoRenderer();
		CoreEngine.SetCardReaderType(InitialSettings.GetCardReader());
	}

	ColorSchemeOptions.SetApplyCallback(ColorSchemeApplyProc);
	ColorSchemeOptions.ApplyColorScheme();

	CMainWindow::Initialize();
	CViewWindow::Initialize(hInst);
	CVideoContainerWindow::Initialize(hInst,&CoreEngine.m_DtvEngine);
	CFullscreen::Initialize();
	CStatusView::Initialize(hInst);
	CSplitter::Initialize(hInst);
	CPanelFrame::Initialize(hInst);
	CInfoPanel::Initialize(hInst);
	CInformation::Initialize(hInst);
	CProgramListView::Initialize(hInst);
	CControlPanel::Initialize(hInst);
	CProgramGuide::Initialize(hInst);
	CCapturePreview::Initialize(hInst);
	CPseudoOSD::Initialize(hInst);

	{
		INITCOMMONCONTROLSEX iccex;

		iccex.dwSize=sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC=ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
		InitCommonControlsEx(&iccex);
	}

	if (!MainWindow.Create(NULL,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		MessageBox(NULL,TEXT("ウィンドウが作成できません。"),NULL,
														MB_OK | MB_ICONSTOP);
		return FALSE;
	}
	MainWindow.Show(nCmdShow);
	MainWindow.Update();
	if (!MainWindow.GetTitleBarVisible()) {
		// この段階でスタイルを変えないとおかしくなる
		// 最初からこのスタイルにしてもキャプションが表示される
		// ShowWindow の前に入れると、タイトルバーを表示させてもアイコンが出ない
		MainWindow.SetStyle(MainWindow.GetStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	ResidentManager.Initialize(MainWindow.GetHandle(),hInst);

	CoreEngine.SetDriverFileName(CmdLineParser.m_szDriverName[0]!='\0'?
								CmdLineParser.m_szDriverName:szDriverFileName);
	if (CoreEngine.GetDriverFileName()[0]!='\0') {
		StatusView.SetSingleText(TEXT("ドライバの読み込み中..."));
		if (CoreEngine.LoadDriver()) {
			Logger.AddLog(TEXT("%s を読み込みました"),CoreEngine.GetDriverFileName());
		} else {
			TCHAR szMessage[MAX_PATH+64];

			wsprintf(szMessage,TEXT("\"%s\" をロードできません。"),CoreEngine.GetDriverFileName());
			MessageBox(MainWindow.GetHandle(),szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
		}
	} else {
		MessageBox(MainWindow.GetHandle(),
			TEXT("設定で使用するドライバを指定してください。"),TEXT("お願い"),
			MB_OK | MB_ICONINFORMATION);
	}
	CoreEngine.SetDescramble(!CmdLineParser.m_fNoDescramble);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(fDescrambleCurServiceOnly);
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_MediaViewer.SetGrabber(fUseGrabberFilter);
	if (!CoreEngine.BuildDtvEngine(&DtvEngineHandler)) {
		/*
		TCHAR szText[1024];

		CoreEngine.FormatLastErrorText(szText,lengthof(szText));
		MessageBox(MainWindow.GetHandle(),szText,NULL,MB_OK | MB_ICONEXCLAMATION);
		*/
		MessageBox(MainWindow.GetHandle(),TEXT("DtvEngineを構築できません。"),
				   NULL,MB_OK | MB_ICONEXCLAMATION);
	} else {
		if (!CoreEngine.OpenBcasCard()) {
			TCHAR szText[1024];

			wsprintf(szText,TEXT("%s\r\n利用可能なカードリーダを検索しますか?"),
					 CoreEngine.GetLastErrorText());
			if (MessageBox(MainWindow.GetHandle(),szText,NULL,MB_YESNO | MB_ICONEXCLAMATION)==IDYES) {
				CCardReader::ReaderType CurReader=CoreEngine.GetCardReaderType();

				if (!CoreEngine.SetCardReaderType(
						CurReader==CCardReader::READER_SCARD?
							CCardReader::READER_HDUS:CCardReader::READER_SCARD)
						&& !CoreEngine.SetCardReaderType(CurReader)) {
					MessageBox(MainWindow.GetHandle(),
						TEXT("利用可能なカードリーダが見付かりませんでした。"),
						NULL,MB_OK | MB_ICONEXCLAMATION);
				}
			}
		}
		if (CoreEngine.m_DtvEngine.m_TsDescrambler.IsBcasCardOpen()) {
			LPCTSTR pszName=CoreEngine.m_DtvEngine.m_TsDescrambler.GetCardReaderName();

			if (pszName!=NULL) {
				BYTE CardID[6];
				ULONGLONG ID;

				CoreEngine.m_DtvEngine.m_TsDescrambler.GetBcasCardID(CardID);
				ID=0;
				for (int i=0;i<6;i++)
					ID=(ID<<8)|(ULONGLONG)CardID[i];
				Logger.AddLog(TEXT("カードリーダ \"%s\" をオープンしました (B-CASカードID %04d %04d %04d %04d %04d"),pszName,
					(DWORD)(ID/(10000ULL*10000ULL*10000ULL*10000ULL))%10000,
					(DWORD)(ID/(10000ULL*10000ULL*10000ULL))%10000,
					(DWORD)(ID/(10000ULL*10000ULL))%10000,
					(DWORD)(ID/10000ULL)%10000,(DWORD)(ID%10000ULL));
			}
		}
		if (!CoreEngine.BuildMediaViewer(VideoContainerWindow.GetHandle(),MainWindow.GetHandle(),
									VideoRendererType,szMpeg2DecoderName)) {
			TCHAR szText[1024];
			int Length;

			lstrcpy(szText,TEXT("DirectShowの初期化ができません。\n"));
			Length=lstrlen(szText);
			CoreEngine.FormatLastErrorText(szText+Length,lengthof(szText)-Length);
			MessageBox(MainWindow.GetHandle(),szText,NULL,MB_OK | MB_ICONEXCLAMATION);
			Logger.AddLog(CoreEngine.GetLastErrorText());
		}
		if (CoreEngine.IsDriverLoaded() && !CoreEngine.OpenDriver()) {
			MessageBox(MainWindow.GetHandle(),TEXT("BonDriverの初期化ができません。"),
					NULL,MB_OK | MB_ICONEXCLAMATION);
		}
		EpgOptions.InitializeEpgDataCap();
		if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
				&& !CmdLineParser.m_fNoView)
			MainWindow.EnablePreview(true);

		TCHAR szDecoder[128];
		if (CoreEngine.m_DtvEngine.GetVideoDecoderName(szDecoder,lengthof(szDecoder)))
			InfoWindow.SetDecoderName(szDecoder);
	}
	if (CoreEngine.IsUDPDriver()) {
		if (fIncrementUDPPort) {
			CPortQuery PortQuery;
			WORD UDPPort=CmdLineParser.m_UDPPort>0?(WORD)CmdLineParser.m_UDPPort:1234;
			WORD RemoconPort=NetworkRemoconOptions.GetPort();

			StatusView.SetSingleText(TEXT("UDPの空きポートを検索しています..."));
			PortQuery.Query(&UDPPort,&RemoconPort);
			CmdLineParser.m_UDPPort=UDPPort;
			NetworkRemoconOptions.SetTempPort(RemoconPort);
		}
		if (CmdLineParser.m_fUseNetworkRemocon)
			NetworkRemoconOptions.SetTempEnable(true);
	}
	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	AppMain.InitializeChannel();
	ChannelScan.SetTuningSpaceList(ChannelManager.GetTuningSpaceList());

	if (!fNoKeyHook && !fNoRemoteController) {
		pRemoteController=new CRemoteController(MainWindow.GetHandle());
		pRemoteController->BeginHook();
	}

	if (!fNoRemoteController)
		hAccel=LoadAccelerators(hInst,
							MAKEINTRESOURCE(fNoKeyHook?IDA_ACCEL:IDA_ACCEL2));

	SetDisplayStatus();

	CoreEngine.m_DtvEngine.SetTracer(NULL);
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
	Splitter.SetPane(PanelPaneIndex,&InfoPanel,PANE_ID_PANEL);
	Splitter.SetMinSize(PANE_ID_PANEL,64);
	Splitter.SetFixedPane(PANE_ID_PANEL);
	PanelFrame.Create(MainWindow.GetHandle(),&Splitter,PANE_ID_PANEL,&InfoPanel,TEXT("パネル"));
	PanelFrame.SetEventHandler(&PanelStatus);
	if (fShowPanelWindow) {
		PanelFrame.SetPanelVisible(true);
		PanelFrame.Update();
	}

	/*
	if (EpgOptions.LoadEpgFile(&EpgProgramList))
		Logger.AddLog(TEXT("EPGデータを \"%s\" から読み込みました"),EpgOptions.GetEpgFileName());
	*/
	EpgOptions.AsyncLoadEpgFile(&EpgProgramList);

	ProgramGuide.SetEpgProgramList(&EpgProgramList);
	ProgramGuide.SetEventHandler(&ProgramGuideEventHandler);

	CapturePreview.SetEventHandler(&CapturePreviewEvent);

	if (CoreEngine.IsBuildComplete()) {
		if (CmdLineParser.m_fFullscreen)
			MainWindow.SetFullscreen(true);

		if (CoreEngine.IsUDPDriver()) {
			if (CmdLineParser.m_UDPPort>1234 && CmdLineParser.m_UDPPort<=1243)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+(CmdLineParser.m_UDPPort-1234));
		} else if (AppMain.IsFirstExecute()) {
			if (ChannelManager.GetFileAllChannelList()->NumChannels()==0) {
				if (MessageBox(MainWindow.GetHandle(),
						TEXT("最初にチャンネルスキャンを行うことをおすすめします。\r\n")
						TEXT("今すぐチャンネルスキャンを行いますか?"),
						TEXT("チャンネルスキャンの確認"),
						MB_YESNO | MB_ICONQUESTION)==IDYES) {
					OptionDialog.ShowDialog(MainWindow.GetHandle(),
										 COptionDialog::PAGE_CHANNELSCAN);
				}
			}
		} else if (CmdLineParser.m_Channel>0 || CmdLineParser.m_ControllerChannel>0) {
			const CChannelList *pList=ChannelManager.GetAllChannelList();
			int i;

			if (CmdLineParser.m_Channel>0) {
				i=pList->FindChannel(CmdLineParser.m_Channel);
			} else {
				i=pList->FindChannelNo(CmdLineParser.m_ControllerChannel);
			}
			if (i>=0)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+i);
		} else if (fRestoreChannel
				&& RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0
				&& lstrcmpi(RestoreChannelInfo.szDriverName,CoreEngine.GetDriverFileName())==0) {
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i;

			i=pList->Find(RestoreChannelInfo.Space,RestoreChannelInfo.Channel,
						  RestoreChannelInfo.Service);
			if (i>=0) {
				MainWindow.PostCommand(CM_CHANNEL_FIRST+i);
			} else {
				pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
				if (pList!=NULL) {
					i=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.Service);
					if (i>=0)
						AppMain.SetChannel(RestoreChannelInfo.Space,i);
				}
			}
		} else {
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i=pList->Find(0,0,0);

			if (i>=0)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+i);
		}

		if (CmdLineParser.m_fRecord)
			MainWindow.PostCommand(CM_RECORD_START);
	}

	MSG msg;

	while (GetMessage(&msg,NULL,0,0)) {
		if (HtmlHelpClass.PreTranslateMessage(&msg))
			continue;
		if (hAccel==NULL || !TranslateAccelerator(MainWindow.GetHandle(),hAccel,&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

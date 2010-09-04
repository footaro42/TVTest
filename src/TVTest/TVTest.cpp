#include "stdafx.h"
#include "DtvEngine.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ResidentManager.h"
#include "InformationPanel.h"
#include "ProgramListPanel.h"
#include "ChannelPanel.h"
#include "CaptionPanel.h"
#include "ControlPanel.h"
#include "ControlPanelItems.h"
#include "Accelerator.h"
#include "Controller.h"
#include "GeneralOptions.h"
#include "ViewOptions.h"
#include "OSDOptions.h"
#include "StatusOptions.h"
#include "SideBarOptions.h"
#include "PanelOptions.h"
#include "ColorScheme.h"
#include "OperationOptions.h"
#include "DriverOptions.h"
#include "PlaybackOptions.h"
#include "RecordOptions.h"
#include "CaptureOptions.h"
#include "ChannelScan.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "Plugin.h"
#include "NetworkRemocon.h"
#include "Logger.h"
#include "CommandLine.h"
#include "InitialSettings.h"
#include "ChannelHistory.h"
#include "Help.h"
#include "StreamInfo.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "PseudoOSD.h"
#include "DisplayMenu.h"
#include "Taskbar.h"
#include "EventInfoPopup.h"
#include "CardReaderDialog.h"
#include "ZoomOptions.h"
#include "LogoManager.h"
#include "ToolTip.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif


static HINSTANCE hInst;
static CAppMain AppMain;
static CCoreEngine CoreEngine;
static CMainMenu MainMenu;
static CCommandList CommandList;
static CEpgProgramList EpgProgramList(&CoreEngine.m_DtvEngine.m_EventManager);
static CMainWindow MainWindow;
static CStatusView StatusView;
static CSideBar SideBar(&CommandList);
static CNotificationBar NotificationBar;
static CHtmlHelp HtmlHelpClass;
static CIconMenu AspectRatioIconMenu;
static CTaskbarManager TaskbarManager;
static CChannelDisplayMenu ChannelDisplayMenu(&EpgProgramList);
static CBalloonTip NotifyBalloonTip;

static bool fIncrementUDPPort=true;

static CCommandLineParser CmdLineParser;

static CChannelManager ChannelManager;
static CNetworkRemocon *pNetworkRemocon=NULL;
static CResidentManager ResidentManager;
static CDriverManager DriverManager;
static CLogoManager LogoManager;

static bool fShowPanelWindow=false;
static CPanelFrame PanelFrame;
static int PanelPaneIndex=1;
static CPanelForm PanelForm;

static CInformationPanel InfoPanel;
static CProgramListPanel ProgramListPanel;
static CChannelPanel ChannelPanel;
static CCaptionPanel CaptionPanel;
static CControlPanel ControlPanel;

static CProgramGuide ProgramGuide;
static CProgramGuideFrame ProgramGuideFrame(&ProgramGuide);
static CProgramGuideDisplay ProgramGuideDisplay(&ProgramGuide);
static bool fShowProgramGuide=false;

static CStreamInfo StreamInfo;

static CChannelMenu ChannelMenu(&EpgProgramList,&LogoManager);

static CZoomOptions ZoomOptions(&CommandList);
static CGeneralOptions GeneralOptions;
static CViewOptions ViewOptions;
static COSDOptions OSDOptions;
static COSDManager OSDManager(&OSDOptions);
static CStatusOptions StatusOptions(&StatusView);
static CSideBarOptions SideBarOptions(&SideBar,&ZoomOptions);
static CPanelOptions PanelOptions(&PanelFrame);
static CColorSchemeOptions ColorSchemeOptions;
static COperationOptions OperationOptions;
static CAccelerator Accelerator;
static CControllerManager ControllerManager;
static CDriverOptions DriverOptions;
static CPlaybackOptions PlaybackOptions;
static CRecordOptions RecordOptions;
static CRecordManager RecordManager;
static CCaptureOptions CaptureOptions;
static CChannelScan ChannelScan(&CoreEngine);
static CEpgOptions EpgOptions(&CoreEngine,&LogoManager);
static CProgramGuideOptions ProgramGuideOptions(&ProgramGuide);
static CPluginList PluginList;
static CPluginOptions PluginOptions(&PluginList);
static CNetworkRemoconOptions NetworkRemoconOptions;
static CLogger Logger;
static CRecentChannelList RecentChannelList;
static CChannelHistory ChannelHistory;

static struct {
	int Space;
	int Channel;
	int ServiceID;
	bool fAllChannels;
	TCHAR szDriverName[MAX_PATH];
} RestoreChannelInfo;

static bool fEnablePlay=true;
static bool fMuteStatus=false;

static CImageCodec ImageCodec;
static CCaptureWindow CaptureWindow;
static bool fShowCaptureWindow=false;

static const BYTE g_AudioGainList[] = {100, 125, 150, 200};

static const struct {
	CMediaViewer::PropertyFilter Filter;
	int Command;
} g_DirectShowFilterPropertyList[] = {
	{CMediaViewer::PROPERTY_FILTER_VIDEODECODER,		CM_VIDEODECODERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_VIDEORENDERER,		CM_VIDEORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIOFILTER,			CM_AUDIOFILTERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIORENDERER,		CM_AUDIORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_MPEG2DEMULTIPLEXER,	CM_DEMULTIPLEXERPROPERTY},
};


class CMyGetChannelReceiver : public CNetworkRemoconReceiver {
public:
	void OnReceive(LPCSTR pszText);
};

void CMyGetChannelReceiver::OnReceive(LPCSTR pszText)
{
	int Channel;
	LPCSTR p;

	Channel=0;
	for (p=pszText;*p!='\0';p++)
		Channel=Channel*10+(*p-'0');
	PostMessage(MainWindow.GetHandle(),WM_APP_CHANNELCHANGE,Channel,0);
}


class CMyGetDriverReceiver : public CNetworkRemoconReceiver {
	HANDLE m_hEvent;
	TCHAR m_szCurDriver[64];
public:
	CMyGetDriverReceiver() { m_hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL); }
	~CMyGetDriverReceiver() { ::CloseHandle(m_hEvent); }
	void OnReceive(LPCSTR pszText);
	void Initialize() { ::ResetEvent(m_hEvent); }
	bool Wait(DWORD TimeOut) { return ::WaitForSingleObject(m_hEvent,TimeOut)==WAIT_OBJECT_0; }
	LPCTSTR GetCurDriver() const { return m_szCurDriver; }
};

void CMyGetDriverReceiver::OnReceive(LPCSTR pszText)
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


static CMyGetChannelReceiver GetChannelReceiver;
static CMyGetDriverReceiver GetDriverReceiver;


class CTotTimeAdjuster {
	bool m_fEnable;
	DWORD m_TimeOut;
	DWORD m_StartTime;
	SYSTEMTIME m_PrevTime;
public:
	CTotTimeAdjuster()
		: m_fEnable(false)
	{
	}
	bool BeginAdjust(DWORD TimeOut=10000UL)
	{
		m_TimeOut=TimeOut;
		m_StartTime=::GetTickCount();
		m_PrevTime.wYear=0;
		m_fEnable=true;
		return true;
	}
	bool AdjustTime()
	{
		if (!m_fEnable)
			return false;
		if (DiffTime(m_StartTime,::GetTickCount())>=m_TimeOut) {
			m_fEnable=false;
			return false;
		}

		SYSTEMTIME st;
		if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
			return false;
		if (m_PrevTime.wYear==0) {
			m_PrevTime=st;
			return false;
		} else if (memcmp(&m_PrevTime,&st,sizeof(SYSTEMTIME))==0) {
			return false;
		}

		bool fOK=false;
		HANDLE hToken;
		if (::OpenProcessToken(::GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)) {
			LUID luid;
			if (::LookupPrivilegeValue(NULL,SE_SYSTEMTIME_NAME,&luid)) {
				TOKEN_PRIVILEGES tkp;
				tkp.PrivilegeCount=1;
				tkp.Privileges[0].Luid=luid;
				tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
				if (::AdjustTokenPrivileges(hToken,FALSE, &tkp,sizeof(TOKEN_PRIVILEGES),NULL,0)
						&& ::GetLastError()==ERROR_SUCCESS) {
					// バッファがあるので少し時刻を戻す
					OffsetSystemTime(&st,-2000);
					if (::SetLocalTime(&st)) {
						Logger.AddLog(TEXT("TOTで時刻合わせを行いました。(%d/%d/%d %d/%02d/%02d)"),
									  st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
						fOK=true;
					}
				}
			}
			::CloseHandle(hToken);
		}
		m_fEnable=false;
		return fOK;
	}
};

static CTotTimeAdjuster TotTimeAdjuster;




CAppMain::CAppMain()
	: m_fSilent(false)
	, m_fFirstExecute(false)
{
}


bool CAppMain::Initialize()
{
	m_UICore.SetSkin(&MainWindow);

	TCHAR szModuleFileName[MAX_PATH];

	::GetModuleFileName(NULL,szModuleFileName,MAX_PATH);
	if (CmdLineParser.m_IniFileName.IsEmpty()) {
		::lstrcpy(m_szIniFileName,szModuleFileName);
		::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	} else {
		LPCTSTR pszIniFileName=CmdLineParser.m_IniFileName.Get();
		if (::PathIsRelative(pszIniFileName)) {
			TCHAR szTemp[MAX_PATH];
			::lstrcpy(szTemp,szModuleFileName);
			::lstrcpy(::PathFindFileName(szTemp),pszIniFileName);
			::PathCanonicalize(m_szIniFileName,szTemp);
		} else {
			::lstrcpy(m_szIniFileName,pszIniFileName);
		}
	}
	::lstrcpy(m_szDefaultChannelFileName,szModuleFileName);
	::PathRenameExtension(m_szDefaultChannelFileName,CHANNEL_FILE_EXTENSION);
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
	m_fFirstExecute=!fExists && CmdLineParser.m_IniFileName.IsEmpty();
	if (fExists) {
		AddLog(TEXT("設定を読み込んでいます..."));
		LoadSettings();
	}
	return true;
}


bool CAppMain::Finalize()
{
	AddLog(TEXT("設定を保存しています..."));
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


bool CAppMain::GetDriverDirectory(LPTSTR pszDirectory) const
{
	return CoreEngine.GetDriverDirectory(pszDirectory);
}


void CAppMain::AddLog(LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	Logger.AddLogV(pszText,Args);
	va_end(Args);
}


void CAppMain::OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	if (pErrorHandler==NULL)
		return;
	Logger.AddLog(pErrorHandler->GetLastErrorText());
	if (!m_fSilent)
		m_UICore.GetSkin()->ShowErrorMessage(pErrorHandler,pszTitle);
}


bool CAppMain::InitializeChannel()
{
	bool fNetworkDriver=CoreEngine.IsNetworkDriver();
	CFilePath ChannelFilePath;
	TCHAR szNetworkDriverName[MAX_PATH];

	ChannelManager.Clear();
	ChannelManager.MakeDriverTuningSpaceList(&CoreEngine.m_DtvEngine.m_BonSrcDecoder);
	if (!fNetworkDriver) {
		ChannelFilePath.SetPath(CoreEngine.GetDriverFileName());
		if (ChannelFilePath.IsRelative()) {
			TCHAR szDir[MAX_PATH];
			GetAppDirectory(szDir);
			ChannelFilePath.RemoveDirectory();
			ChannelFilePath.SetDirectory(szDir);
		}
#ifndef TVH264
		ChannelFilePath.SetExtension(TEXT(".ch2"));
		if (!ChannelFilePath.IsExists())
			ChannelFilePath.SetExtension(TEXT(".ch"));
#else
		ChannelFilePath.SetExtension(TEXT(".ch1"));
		if (!ChannelFilePath.IsExists()) {
			ChannelFilePath.SetExtension(TEXT(".ch2"));
			if (!ChannelFilePath.IsExists())
				ChannelFilePath.SetExtension(TEXT(".ch"));
		}
#endif
	} else {
		bool fOK=false;

		if (NetworkRemoconOptions.IsEnable()) {
			if (NetworkRemoconOptions.CreateNetworkRemocon(&pNetworkRemocon)) {
				GetDriverReceiver.Initialize();
				if (pNetworkRemocon->GetDriverList(&GetDriverReceiver)
						&& GetDriverReceiver.Wait(2000)
						&& GetDriverReceiver.GetCurDriver()[0]!='\0') {
					TCHAR szFileName[MAX_PATH];

					if (NetworkRemoconOptions.FindChannelFile(
								GetDriverReceiver.GetCurDriver(),szFileName)) {
						LPTSTR p;

						NetworkRemoconOptions.SetDefaultChannelFileName(szFileName);
						p=szFileName;
						while (*p!='(')
							p++;
						*p='\0';
						::wsprintf(szNetworkDriverName,TEXT("%s.dll"),szFileName);
						::lstrcpy(p,CHANNEL_FILE_EXTENSION);
						ChannelFilePath.SetPath(szFileName);
						GetAppDirectory(szFileName);
						ChannelFilePath.SetDirectory(szFileName);
						fOK=ChannelFilePath.IsExists();
						if (!fOK) {
#ifndef TVH264
							ChannelFilePath.SetExtension(TEXT(".ch"));
							fOK=ChannelFilePath.IsExists();
#else
							ChannelFilePath.SetExtension(TEXT(".ch2"));
							fOK=ChannelFilePath.IsExists();
							if (!fOK) {
								ChannelFilePath.SetExtension(TEXT(".ch"));
								fOK=ChannelFilePath.IsExists();
							}
#endif
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
				*q='\0';
				::wsprintf(szNetworkDriverName,TEXT("%s.dll"),szFileName);
				::lstrcpy(q,CHANNEL_FILE_EXTENSION);
				ChannelFilePath.Append(szFileName);
				fOK=ChannelFilePath.IsExists();
				if (!fOK) {
#ifndef TVH264
					ChannelFilePath.SetExtension(TEXT(".ch"));
					fOK=ChannelFilePath.IsExists();
#else
					ChannelFilePath.SetExtension(TEXT(".ch2"));
					fOK=ChannelFilePath.IsExists();
					if (!fOK) {
						ChannelFilePath.SetExtension(TEXT(".ch"));
						fOK=ChannelFilePath.IsExists();
					}
#endif
				}
			}
		}
		if (!fOK)
			szNetworkDriverName[0]='\0';
	}
	if (ChannelFilePath.GetPath()[0]=='\0' || !ChannelFilePath.IsExists()) {
		ChannelFilePath.SetPath(m_szDefaultChannelFileName);
		if (!ChannelFilePath.IsExists())
			ChannelFilePath.SetExtension(TEXT(".ch"));
	}
	if (ChannelManager.LoadChannelList(ChannelFilePath.GetPath()))
		Logger.AddLog(TEXT("チャンネル設定を \"%s\" から読み込みました。"),
												ChannelFilePath.GetFileName());
	TCHAR szFileName[MAX_PATH];
	bool fLoadChannelSettings=true;
	if (!fNetworkDriver) {
		::lstrcpy(szFileName,CoreEngine.GetDriverFileName());
	} else {
		if (szNetworkDriverName[0]!='\0') {
			::lstrcpy(szFileName,szNetworkDriverName);
		} else {
			fLoadChannelSettings=false;
		}
	}
	if (fLoadChannelSettings)
		ChannelManager.LoadChannelSettings(m_szChannelSettingFileName,szFileName);

	CDriverOptions::ChannelInfo InitChInfo;
	if (DriverOptions.GetInitialChannel(CoreEngine.GetDriverFileName(),&InitChInfo)) {
		RestoreChannelInfo.Space=InitChInfo.Space;
		RestoreChannelInfo.Channel=InitChInfo.Channel;
		RestoreChannelInfo.ServiceID=InitChInfo.ServiceID;
		RestoreChannelInfo.fAllChannels=InitChInfo.fAllChannels;
	} else {
		RestoreChannelInfo.Space=-1;
		RestoreChannelInfo.Channel=-1;
		RestoreChannelInfo.ServiceID=-1;
		RestoreChannelInfo.fAllChannels=false;
	}

	ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	ChannelManager.SetCurrentSpace(
		(!fNetworkDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
											CChannelManager::SPACE_ALL:0);
	*/
	ChannelManager.SetCurrentChannel(
		RestoreChannelInfo.fAllChannels?-1:max(RestoreChannelInfo.Space,0),
		-1);
	ChannelManager.SetCurrentServiceID(0);
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	m_UICore.OnChannelListChanged();
	ChannelScan.SetTuningSpaceList(ChannelManager.GetTuningSpaceList());
	return true;
}


bool CAppMain::RestoreChannel()
{
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		int Space=RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:RestoreChannelInfo.Space;
		const CChannelList *pList=ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index<0 && RestoreChannelInfo.ServiceID>0)
				Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel);
			if (Index>=0)
				return AppMain.SetChannel(Space,Index);
		}
	}
	return false;
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
	int Space=-1;
	bool fAllChannels=false;
	for (int i=0;i<pList->NumSpaces();i++) {
		if (pList->GetTuningSpaceType(i)!=CTuningSpaceInfo::SPACE_TERRESTRIAL) {
			fAllChannels=false;
			break;
		}
		if (pList->GetChannelList(i)->NumChannels()>0) {
			if (Space>=0)
				fAllChannels=true;
			else
				Space=i;
		}
	}
	ChannelManager.SetCurrentChannel(
					fAllChannels?CChannelManager::SPACE_ALL:(Space>=0?Space:0),
					CoreEngine.IsUDPDriver()?0:-1);
	ChannelManager.SetCurrentServiceID(0);
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		FollowChannelChange(CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),ServiceID);
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	m_UICore.OnChannelListChanged();
	return true;
}


bool CAppMain::SaveChannelSettings()
{
	if (!CoreEngine.IsDriverLoaded() || CoreEngine.IsNetworkDriver())
		return true;
	return ChannelManager.SaveChannelSettings(m_szChannelSettingFileName,
											  CoreEngine.GetDriverFileName());
}


const CChannelInfo *CAppMain::GetCurrentChannelInfo() const
{
	return ChannelManager.GetCurrentChannelInfo();
}


bool CAppMain::SetChannel(int Space,int Channel,int ServiceID/*=-1*/)
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
	if (pPrevChInfo==NULL
			|| pChInfo->GetSpace()!=pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex()!=pPrevChInfo->GetChannelIndex()) {
		if (ServiceID<=0 /*&& pChInfo->GetService()>0*/ && pChInfo->GetServiceID()>0)
			ServiceID=pChInfo->GetServiceID();

		LPCTSTR pszTuningSpace=ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetSpace());
		AddLog(TEXT("BonDriverにチャンネル変更を要求しました。(チューニング空間 %d[%s] / Ch %d[%s] / Sv %d)"),
			   pChInfo->GetSpace(),pszTuningSpace!=NULL?pszTuningSpace:TEXT("???"),
			   pChInfo->GetChannelIndex(),pChInfo->GetName(),ServiceID);

		if (!CoreEngine.m_DtvEngine.SetChannel(
							pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
							ServiceID>0?ServiceID:CDtvEngine::SID_INVALID)) {
			AddLog(CoreEngine.m_DtvEngine.GetLastErrorText());
			ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
			return false;
		}
#ifdef TVH264_FOR_1SEG
		// 予めTSIDを設定して表示を早くする
		if ((pChInfo->GetTransportStreamID() & 0xFF00) == 0x7F00
				|| (pChInfo->GetTransportStreamID() & 0xFF00) == 0x7E00) {
			CoreEngine.m_DtvEngine.m_TsPacketParser.SetTransportStreamID(pChInfo->GetTransportStreamID());
		}
#endif
		ChannelManager.SetCurrentServiceID(ServiceID);
		PluginList.SendChannelChangeEvent();
	} else {
		ChannelManager.SetCurrentServiceID(ServiceID);
		if (ServiceID>0) {
			SetServiceByID(ServiceID);
		} else if (pChInfo->GetServiceID()>0) {
			SetServiceByID(pChInfo->GetServiceID());
		}
	}
	m_UICore.OnChannelChanged(Space!=OldSpace);
	return true;
}


bool CAppMain::FollowChannelChange(WORD TransportStreamID,WORD ServiceID)
{
	const CChannelList *pChannelList;
	const CChannelInfo *pChannelInfo;
	int i,j;
	int Space,Channel;
	bool fFound=false;

	pChannelList=ChannelManager.GetCurrentRealChannelList();
	if (pChannelList!=NULL) {
		for (i=0;i<pChannelList->NumChannels();i++) {
			pChannelInfo=pChannelList->GetChannelInfo(i);
			if (pChannelInfo->GetTransportStreamID()==TransportStreamID
					&& pChannelInfo->GetServiceID()==ServiceID) {
				Space=ChannelManager.GetCurrentSpace();
				Channel=i;
				fFound=true;
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
					fFound=true;
					goto End;
				}
			}
		}
	End:
		;
	}
	if (!fFound)
		return false;
	if (Space==ChannelManager.GetCurrentSpace()
			&& Channel==ChannelManager.GetCurrentChannel())
		return true;
	Logger.AddLog(TEXT("ストリームの変化を検知しました。(TSID %d / SID %d)"),
				  TransportStreamID,ServiceID);
	const bool fSpaceChanged=Space!=ChannelManager.GetCurrentSpace();
	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	ChannelManager.SetCurrentServiceID(0);
	m_UICore.OnChannelChanged(fSpaceChanged);
	PluginList.SendChannelChangeEvent();
	return true;
}


bool CAppMain::SetService(int Service)
{
	int NumServices=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceNum();

	if (Service<0 || Service>=NumServices
			|| !CoreEngine.m_DtvEngine.SetService(Service))
		return false;
	if (pNetworkRemocon!=NULL)
		pNetworkRemocon->SetService(Service);
	WORD ServiceID=0;
	CoreEngine.m_DtvEngine.GetServiceID(&ServiceID);
	AddLog(TEXT("サービスを変更しました。(%d: SID %d)"),Service,ServiceID);
	m_UICore.OnServiceChanged();
	return true;
}


bool CAppMain::SetServiceByIndex(int Service)
{
	if (!SetService(Service))
		return false;
	WORD ServiceID=0;
	CoreEngine.m_DtvEngine.GetServiceID(&ServiceID);
	ChannelManager.SetCurrentServiceID(Service);
	return true;
}


bool CAppMain::SetServiceByID(WORD ServiceID,int *pServiceIndex/*=NULL*/)
{
	if (ServiceID==0)
		return SetService(0);

	AddLog(TEXT("サービスを選択しています(SID %d)..."),ServiceID);
	WORD Index=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
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
	if (CoreEngine.IsDriverOpen()
			&& ::lstrcmpi(CoreEngine.GetDriverFileName(),pszFileName)==0)
		return true;

	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	bool fOK;

	SaveCurrentChannel();
	SaveChannelSettings();
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.SetDriverFileName(pszFileName);
	fOK=CoreEngine.LoadDriver();
	if (!fOK) {
		PluginList.SendDriverChangeEvent();
		::SetCursor(hcurOld);
		OnError(&CoreEngine);
	}
	if (CoreEngine.IsBcasCardOpen()
			&& (DriverOptions.IsDescrambleDriver(pszFileName)
				|| CoreEngine.IsDriverCardReader())) {
		if (CoreEngine.CloseBcasCard()) {
			Logger.AddLog(TEXT("カードリーダを閉じました。"));
		}
	}
	if (fOK) {
		Logger.AddLog(TEXT("%s を読み込みました。"),pszFileName);
		if (!CoreEngine.IsBcasCardOpen()
				&& !DriverOptions.IsDescrambleDriver(pszFileName)) {
			OpenBcasCard();
		}
		CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(
			!DriverOptions.IsIgnoreInitialStream(pszFileName));
		fOK=CoreEngine.OpenDriver();
		if (fOK) {
			CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetPurgeStreamOnChannelChange(
				DriverOptions.IsPurgeStreamOnChannelChange(pszFileName));
			InitializeChannel();
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
			OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
		}
	}
	CoreEngine.m_DtvEngine.SetTracer(NULL);
	StatusView.SetSingleText(NULL);
	m_UICore.OnTunerChanged();
	return fOK;
}


bool CAppMain::SetDriverAndChannel(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
{
	if (RecordManager.IsRecording()) {
		if (!RecordOptions.ConfirmChannelChange(m_UICore.GetSkin()->GetVideoHostWindow()))
			return false;
	}
	if (::lstrcmpi(CoreEngine.GetDriverFileName(),pszDriverFileName)!=0) {
		if (!SetDriver(pszDriverFileName))
			return false;
	}
	const CChannelList *pList=ChannelManager.GetChannelList(pChannelInfo->GetSpace());
	if (pList==NULL)
		return false;
	int Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),
						  pChannelInfo->GetServiceID());
	if (Index<0)
		return false;
	return AppMain.SetChannel(pChannelInfo->GetSpace(),Index);
}


bool CAppMain::OpenTuner()
{
	bool fOK=true;

	if (!CoreEngine.IsDriverSpecified())
		return true;

	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	if (!CoreEngine.IsBcasCardOpen()
			&& !DriverOptions.IsDescrambleDriver(CoreEngine.GetDriverFileName())) {
		OpenBcasCard();
	}
	if (!CoreEngine.IsDriverOpen()) {
		if (!CoreEngine.IsDriverLoaded()) {
			if (CoreEngine.LoadDriver()) {
				Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
				if (CoreEngine.OpenDriver()) {
					m_UICore.OnTunerOpened();
				} else {
					OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
					fOK=false;
				}
			} else {
				OnError(&CoreEngine);
				fOK=false;
			}
		}
	}
	CoreEngine.m_DtvEngine.SetTracer(NULL);
	StatusView.SetSingleText(NULL);
	return fOK;
}


bool CAppMain::CloseTuner()
{
	if (CoreEngine.IsBcasCardOpen()) {
		if (CoreEngine.CloseBcasCard()) {
			Logger.AddLog(TEXT("カードリーダを閉じました。"));
		}
	}
	if (CoreEngine.IsDriverOpen()) {
		CoreEngine.UnloadDriver();
		ChannelManager.SetCurrentChannel(ChannelManager.GetCurrentSpace(),-1);
		Logger.AddLog(TEXT("ドライバを閉じました。"));
		m_UICore.OnTunerClosed();
	}
	return true;
}


bool CAppMain::OpenBcasCard(bool fRetry)
{
	if (!CoreEngine.IsBcasCardOpen()) {
		if (!CoreEngine.OpenBcasCard()) {
			Logger.AddLog(TEXT("カードリーダがオープンできません。"));
			if (fRetry) {
#if 0
				TCHAR szText[1024];
				int Length;

				Length=::wsprintf(szText,TEXT("%s\n"),CoreEngine.GetLastErrorText());
				if (CoreEngine.GetLastErrorSystemMessage()!=NULL)
					Length+=::wsprintf(szText+Length,TEXT("(%s)\n"),CoreEngine.GetLastErrorSystemMessage());
				::lstrcpy(szText+Length,
						  TEXT("利用可能なカードリーダを検索しますか?\n")
						  TEXT("※もし正常に視聴できるのにこのダイアログが表示される場合、\n")
						  TEXT("設定でカードリーダに「なし」を選択してください。"));
				if (MainWindow.ShowMessage(szText,NULL,MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2)==IDYES) {
					CCoreEngine::CardReaderType CurReader=CoreEngine.GetCardReaderType();

					if (!CoreEngine.SetCardReaderType(
							CurReader==CCoreEngine::CARDREADER_SCARD?
							CCoreEngine::CARDREADER_HDUS:CCoreEngine::CARDREADER_SCARD)
							&& !CoreEngine.SetCardReaderType(CurReader)) {
						MainWindow.ShowErrorMessage(
							TEXT("利用可能なカードリーダが見付かりませんでした。"));
					}
				}
#else
				TCHAR szText[1024];
				int Length;
				CCardReaderErrorDialog Dialog;

				Length=::wsprintf(szText,TEXT("%s\r\n"),CoreEngine.GetLastErrorText());
				if (CoreEngine.GetLastErrorSystemMessage()!=NULL)
					Length+=::wsprintf(szText+Length,TEXT("(%s)\r\n"),CoreEngine.GetLastErrorSystemMessage());
				::lstrcpy(szText+Length,
						  TEXT("※もし正常に視聴できるのにこのダイアログが表示される場合、")
						  TEXT("設定でカードリーダに「なし」を選択してください。"));
				Dialog.SetMessage(szText);
				while (Dialog.Show(MainWindow.GetHandle())) {
					if (CoreEngine.SetCardReaderType(Dialog.GetReaderType(),
													 Dialog.GetReaderName())
							|| Dialog.GetReaderType()==CCoreEngine::CARDREADER_NONE)
						break;
				}
#endif
			} else {
				if (!m_fSilent)
					MainWindow.ShowErrorMessage(&CoreEngine);
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
	}
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

		if (Setting.Read(TEXT("Volume"),&Value))
			CoreEngine.SetVolume(Value<0?0:Value>CCoreEngine::MAX_VOLUME?CCoreEngine::MAX_VOLUME:Value);
		int Gain=100,SurroundGain;
		Setting.Read(TEXT("VolumeNormalizeLevel"),&Gain);
		if (!Setting.Read(TEXT("SurroundGain"),&SurroundGain))
			SurroundGain=Gain;
		CoreEngine.SetAudioGainControl(Gain,SurroundGain);
		Setting.Read(TEXT("ShowInfoWindow"),&fShowPanelWindow);
		Setting.Read(TEXT("EnablePlay"),&fEnablePlay);
		Setting.Read(TEXT("Mute"),&fMuteStatus);
		if (Setting.Read(TEXT("RecOptionFileName"),szText,MAX_PATH) && szText[0]!='\0')
			RecordManager.SetFileName(szText);
		/*
		if (Setting.Read(TEXT("RecOptionExistsOperation"),&Value))
			RecordManager.SetFileExistsOperation(
								(CRecordManager::FileExistsOperation)Value);
		*/
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
		ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("ProgramGuideLeft"),&Left);
		Setting.Read(TEXT("ProgramGuideTop"),&Top);
		Setting.Read(TEXT("ProgramGuideWidth"),&Width);
		Setting.Read(TEXT("ProgramGuideHeight"),&Height);
		ProgramGuideFrame.SetPosition(Left,Top,Width,Height);
		ProgramGuideFrame.MoveToMonitorInside();
		if (Setting.Read(TEXT("ProgramGuideMaximized"),&f) && f)
			ProgramGuideFrame.SetMaximize(f);
		if (Setting.Read(TEXT("ProgramGuideAlwaysOnTop"),&f))
			ProgramGuideFrame.SetAlwaysOnTop(f);
		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("CapturePreviewLeft"),&Left);
		Setting.Read(TEXT("CapturePreviewTop"),&Top);
		Setting.Read(TEXT("CapturePreviewWidth"),&Width);
		Setting.Read(TEXT("CapturePreviewHeight"),&Height);
		CaptureWindow.SetPosition(Left,Top,Width,Height);
		CaptureWindow.MoveToMonitorInside();
		if (Setting.Read(TEXT("CaptureStatusBar"),&f))
			CaptureWindow.ShowStatusBar(f);
		StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("StreamInfoLeft"),&Left);
		Setting.Read(TEXT("StreamInfoTop"),&Top);
		Setting.Read(TEXT("StreamInfoWidth"),&Width);
		Setting.Read(TEXT("StreamInfoHeight"),&Height);
		StreamInfo.SetPosition(Left,Top,Width,Height);
		//StreamInfo.MoveToMonitorInside();
		// Experimental options
		Setting.Read(TEXT("IncrementUDPPort"),&fIncrementUDPPort);
		MainWindow.ReadSettings(&Setting);
		GeneralOptions.Read(&Setting);
		ViewOptions.Read(&Setting);
		OSDOptions.Read(&Setting);
		PanelOptions.Read(&Setting);
		OperationOptions.Read(&Setting);
		PlaybackOptions.Read(&Setting);
		RecordOptions.Read(&Setting);
		CaptureOptions.Read(&Setting);
		Accelerator.Read(&Setting);
		ControllerManager.Read(&Setting);
		ChannelScan.Read(&Setting);
		PluginOptions.Read(&Setting);
		EpgOptions.Read(&Setting);
		NetworkRemoconOptions.Read(&Setting);
		Logger.Read(&Setting);
		ZoomOptions.Read(&Setting);
		Setting.Close();
	}
	StatusOptions.Load(m_szIniFileName);
	SideBarOptions.Load(m_szIniFileName);
	ColorSchemeOptions.Load(m_szIniFileName);
	//Accelerator.Load(m_szIniFileName);
	//ControllerManager.Load(m_szIniFileName);
	DriverOptions.Load(m_szIniFileName);
	ProgramGuideOptions.Load(m_szIniFileName);
	PluginOptions.Load(m_szIniFileName);
	RecentChannelList.Load(m_szIniFileName);
	InfoPanel.Load(m_szIniFileName);
	return true;
}


bool CAppMain::SaveSettings()
{
#ifdef UNICODE
	if (!::PathFileExists(m_szIniFileName)) {
		AddLog(TEXT("設定ファイルを作成します。\"%s\""),m_szIniFileName);
		HANDLE hFile=::CreateFile(m_szIniFileName,GENERIC_WRITE,0,NULL,
								  CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile==INVALID_HANDLE_VALUE) {
			TCHAR szMessage[256+MAX_PATH];
			::wsprintf(szMessage,
					   TEXT("設定ファイル \"%s\" が作成できません。(エラーコード %#lx)"),
					   m_szIniFileName,::GetLastError());
			AddLog(szMessage);
			if (!m_fSilent)
				MainWindow.ShowErrorMessage(szMessage);
		} else {
			// Unicode(UTF-16)のiniファイルになるようにBOMを書き出す
			DWORD Write;
			::WriteFile(hFile,"\xFF\xFE",2,&Write,NULL);
			::FlushFileBuffers(hFile);
			::CloseHandle(hFile);
		}
	}
#endif

	CSettings Setting;

	if (Setting.Open(m_szIniFileName,TEXT("Settings"),CSettings::OPEN_WRITE)) {
		int Left,Top,Width,Height;

		if (!Setting.Write(TEXT("Version"),VERSION_TEXT)) {
			TCHAR szMessage[256+MAX_PATH];
			::wsprintf(szMessage,
					   TEXT("設定ファイル \"%s\" に書き込みできません。(エラーコード %#lx)"),
					   m_szIniFileName,::GetLastError());
			AddLog(szMessage);
			/*
			if (!m_fSilent)
				MainWindow.ShowErrorMessage(szMessage);
			*/
		}
		Setting.Write(TEXT("Volume"),CoreEngine.GetVolume());
		int Gain,SurroundGain;
		CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
		Setting.Write(TEXT("VolumeNormalizeLevel"),Gain);
		Setting.Write(TEXT("SurroundGain"),SurroundGain);
		Setting.Write(TEXT("ShowInfoWindow"),fShowPanelWindow);
		Setting.Write(TEXT("EnablePlay"),fEnablePlay);
		Setting.Write(TEXT("Mute"),CoreEngine.GetMute());
		if (RecordManager.GetFileName()!=NULL)
			Setting.Write(TEXT("RecOptionFileName"),RecordManager.GetFileName());
		/*
		Setting.Write(TEXT("RecOptionExistsOperation"),
										RecordManager.GetFileExistsOperation());
		*/
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
		Setting.Write(TEXT("InfoCurTab"),PanelForm.GetCurPageID());
		ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("ProgramGuideLeft"),Left);
		Setting.Write(TEXT("ProgramGuideTop"),Top);
		Setting.Write(TEXT("ProgramGuideWidth"),Width);
		Setting.Write(TEXT("ProgramGuideHeight"),Height);
		Setting.Write(TEXT("ProgramGuideMaximized"),ProgramGuideFrame.GetMaximize());
		Setting.Write(TEXT("ProgramGuideAlwaysOnTop"),ProgramGuideFrame.GetAlwaysOnTop());
		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("CapturePreviewLeft"),Left);
		Setting.Write(TEXT("CapturePreviewTop"),Top);
		Setting.Write(TEXT("CapturePreviewWidth"),Width);
		Setting.Write(TEXT("CapturePreviewHeight"),Height);
		Setting.Write(TEXT("CaptureStatusBar"),CaptureWindow.IsStatusBarVisible());
		StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("StreamInfoLeft"),Left);
		Setting.Write(TEXT("StreamInfoTop"),Top);
		Setting.Write(TEXT("StreamInfoWidth"),Width);
		Setting.Write(TEXT("StreamInfoHeight"),Height);
		Setting.Write(TEXT("IncrementUDPPort"),fIncrementUDPPort);
		MainWindow.WriteSettings(&Setting);
		GeneralOptions.Write(&Setting);
		ViewOptions.Write(&Setting);
		OSDOptions.Write(&Setting);
		PanelOptions.Write(&Setting);
		OperationOptions.Write(&Setting);
		PlaybackOptions.Write(&Setting);
		RecordOptions.Write(&Setting);
		CaptureOptions.Write(&Setting);
		Accelerator.Write(&Setting);
		ControllerManager.Write(&Setting);
		ChannelScan.Write(&Setting);
		PluginOptions.Write(&Setting);
		EpgOptions.Write(&Setting);
		NetworkRemoconOptions.Write(&Setting);
		Logger.Write(&Setting);
		ZoomOptions.Write(&Setting);
		Setting.Close();
	}
	StatusOptions.Save(m_szIniFileName);
	SideBarOptions.Save(m_szIniFileName);
	ColorSchemeOptions.Save(m_szIniFileName);
	Accelerator.Save(m_szIniFileName);
	//ControllerManager.Save(m_szIniFileName);
	DriverOptions.Save(m_szIniFileName);
	ProgramGuideOptions.Save(m_szIniFileName);
	PluginOptions.Save(m_szIniFileName);
	RecentChannelList.Save(m_szIniFileName);
	InfoPanel.Save(m_szIniFileName);
	return true;
}


bool CAppMain::SaveCurrentChannel()
{
	if (*CoreEngine.GetDriverFileName()!='\0') {
		const CChannelInfo *pInfo=ChannelManager.GetCurrentRealChannelInfo();
		CDriverOptions::ChannelInfo ChInfo;

		if (pInfo!=NULL) {
			ChInfo.Space=pInfo->GetSpace();
			ChInfo.Channel=pInfo->GetChannelIndex();
			ChInfo.ServiceID=pInfo->GetServiceID();
		} else {
			ChInfo.Space=-1;
			ChInfo.Channel=-1;
			ChInfo.ServiceID=-1;
		}
		ChInfo.fAllChannels=ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL;
		DriverOptions.SetLastChannel(CoreEngine.GetDriverFileName(),&ChInfo);
	}
	return true;
}


bool CAppMain::ShowHelpContent(int ID)
{
	return HtmlHelpClass.ShowContent(ID);
}


bool CAppMain::GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName) const
{
	CRecordManager::EventInfo EventInfo;
	const CChannelInfo *pChannelInfo=ChannelManager.GetCurrentChannelInfo();
	WORD ServiceID;
	TCHAR szServiceName[32],szEventName[256];

	EventInfo.pszChannelName=NULL;
	EventInfo.ChannelNo=0;
	EventInfo.pszServiceName=NULL;
	EventInfo.pszEventName=NULL;
	if (pChannelInfo!=NULL) {
		EventInfo.pszChannelName=pChannelInfo->GetName();
		if (pChannelInfo->GetChannelNo()!=0)
			EventInfo.ChannelNo=pChannelInfo->GetChannelNo();
		else if (pChannelInfo->GetServiceID()>0)
			EventInfo.ChannelNo=pChannelInfo->GetServiceID();
	}
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID)) {
		int Index=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(Index,szServiceName,lengthof(szServiceName)))
			EventInfo.pszServiceName=szServiceName;
		if (!CoreEngine.m_DtvEngine.GetServiceID(&EventInfo.ServiceID))
			EventInfo.ServiceID=0;
		bool fNext=false;
		SYSTEMTIME stCur,stStart;
		if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&stCur))
			::GetLocalTime(&stCur);
		if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,NULL,true)) {
			LONGLONG Diff=DiffSystemTime(&stCur,&stStart);
			if (Diff>=0 && Diff<60*1000)
				fNext=true;
		}
		if (CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName),fNext))
			EventInfo.pszEventName=szEventName;
		EventInfo.EventID=CoreEngine.m_DtvEngine.GetEventID(fNext);
		EventInfo.stTotTime=stCur;
	}
	return RecordManager.GenerateFileName(pszFileName,MaxFileName,&EventInfo);
}


bool CAppMain::StartRecord(LPCTSTR pszFileName,
						   const CRecordManager::TimeSpecInfo *pStartTime,
						   const CRecordManager::TimeSpecInfo *pStopTime,
						   CRecordManager::RecordClient Client,
						   bool fTimeShift)
{
	if (RecordManager.IsRecording())
		return false;
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	RecordManager.SetStopOnEventEnd(false);
	RecordManager.SetClient(Client);
	RecordOptions.ApplyOptions(&RecordManager);
	if (CmdLineParser.m_fRecordCurServiceOnly)
		RecordManager.SetCurServiceOnly(true);
	if (RecordManager.IsReserved()) {
		StatusView.UpdateItem(STATUS_ITEM_RECORD);
		return true;
	}

	OpenTuner();

	TCHAR szFileName[MAX_PATH*2];
	if (pszFileName==NULL) {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
											&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		RecordManager.SetFileName(szFileName);
	}
	if (!GenerateRecordFileName(szFileName,MAX_PATH))
		return false;
	PluginList.SendStartRecordEvent(&RecordManager,szFileName,lengthof(szFileName));
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName,fTimeShift)) {
		OnError(&RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	m_UICore.OnRecordingStarted();
	return true;
}


bool CAppMain::ModifyRecord(LPCTSTR pszFileName,
							const CRecordManager::TimeSpecInfo *pStartTime,
							const CRecordManager::TimeSpecInfo *pStopTime,
							CRecordManager::RecordClient Client)
{
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	RecordManager.SetClient(Client);
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
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
		/*
		if (!RecordManager.DoFileExistsOperation(MainWindow.GetVideoHostWindow(),szFileName))
			return false;
		*/
	} else {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
											&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		RecordManager.SetFileName(szFileName);
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
	}
	OpenTuner();
	PluginList.SendStartRecordEvent(&RecordManager,szFileName,lengthof(szFileName));
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
		RecordManager.CancelReserve();
		OnError(&RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	m_UICore.OnRecordingStarted();
	return true;
}


bool CAppMain::CancelReservedRecord()
{
	if (!RecordManager.CancelReserve())
		return false;
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppMain::StopRecord()
{
	if (!RecordManager.IsRecording())
		return false;

	TCHAR szText[MAX_PATH+64],szFileName[MAX_PATH],szSize[32];
	::lstrcpy(szFileName,RecordManager.GetRecordTask()->GetFileName());

	RecordManager.StopRecord();
	RecordOptions.Apply(CRecordOptions::UPDATE_RECORDSTREAM);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(
						GeneralOptions.GetDescrambleCurServiceOnly());

	UInt64ToString(CoreEngine.m_DtvEngine.m_FileWriter.GetWriteSize(),
				   szSize,lengthof(szSize));
	::wsprintf(szText,TEXT("録画停止 %s (書き出しサイズ %s Bytes)"),
			   szFileName,szSize);
	Logger.AddLog(szText);

	ResidentManager.SetStatus(0,CResidentManager::STATUS_RECORDING);
	m_UICore.OnRecordingStopped();
	return true;
}


bool CAppMain::RelayRecord(LPCTSTR pszFileName)
{
	if (pszFileName==NULL || !RecordManager.IsRecording())
		return false;
	if (!RecordManager.RelayFile(pszFileName)) {
		OnError(&RecordManager,TEXT("録画ファイルを切り替えできません。"));
		return false;
	}
	Logger.AddLog(TEXT("録画ファイルを切り替えました %s"),pszFileName);
	PluginList.SendRelayRecordEvent(pszFileName);
	return true;
}


LPCTSTR CAppMain::GetDefaultRecordFolder() const
{
	return RecordOptions.GetSaveFolder();
}


bool CAppMain::IsChannelScanning() const
{
	return ChannelScan.IsScanning();
}


bool CAppMain::IsDriverNoSignalLevel(LPCTSTR pszFileName) const
{
	return DriverOptions.IsNoSignalLevel(pszFileName);
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
}


void CAppMain::SetProgress(int Pos,int Max)
{
	TaskbarManager.SetProgress(Pos,Max);
}


void CAppMain::EndProgress()
{
	TaskbarManager.EndProgress();
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


CUICore *CAppMain::GetUICore()
{
	return &m_UICore;
}


const CChannelManager *CAppMain::GetChannelManager() const
{
	return &ChannelManager;
}


const CRecordManager *CAppMain::GetRecordManager() const
{
	return &RecordManager;
}


const CDriverManager *CAppMain::GetDriverManager() const
{
	return &DriverManager;
}


CControllerManager *CAppMain::GetControllerManager() const
{
	return &ControllerManager;
}


CLogoManager *CAppMain::GetLogoManager() const
{
	return &LogoManager;
}


const CCommandList *CAppMain::GetCommandList() const
{
	return &CommandList;
}


CAppMain &GetAppClass()
{
	return AppMain;
}




static bool ChannelMenuInitPopup(HMENU hmenuParent,HMENU hmenu)
{
	bool fChannelMenu=false;
	int Count=::GetMenuItemCount(hmenuParent);
	int i;
	for (i=0;i<Count;i++) {
		if (::GetSubMenu(hmenuParent,i)==hmenu) {
			fChannelMenu=true;
			break;
		}
		if ((::GetMenuState(hmenuParent,i,MF_BYPOSITION)&MF_POPUP)==0)
			break;
	}

	if (fChannelMenu) {
		const CChannelList *pChannelList;
		int Command=CM_SPACE_CHANNEL_FIRST;

		pChannelList=ChannelManager.GetAllChannelList();
		if (ChannelManager.NumSpaces()>1) {
			if (i==0) {
				unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO | CChannelMenu::FLAG_SPACEBREAK;
				if (pChannelList->NumEnableChannels()<=30)
					MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
				else
					MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
				ChannelMenu.Create(pChannelList,
								   ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL?
								   ChannelManager.GetCurrentChannel():-1,
								   Command,hmenu,MainWindow.GetHandle(),MenuFlags,
								   (MenuFlags&CChannelMenu::FLAG_SHOWEVENTINFO)==0?24:0);
				return true;
			}
			i--;
		}
		if (i>=ChannelManager.NumSpaces()) {
			TRACE(TEXT("ChannelMenuInitPopup() : Invalid space %d\n"),i);
			ClearMenu(hmenu);
			return true;
		}
		Command+=pChannelList->NumChannels();
		for (int j=0;j<i;j++) {
			pChannelList=ChannelManager.GetChannelList(j);
			Command+=pChannelList->NumChannels();
		}
		pChannelList=ChannelManager.GetChannelList(i);
		unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO;
		if (pChannelList->NumEnableChannels()<=30)
			MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
		else
			MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
		ChannelMenu.Create(pChannelList,
						   ChannelManager.GetCurrentSpace()==i?
						   ChannelManager.GetCurrentChannel():-1,
						   Command,hmenu,MainWindow.GetHandle(),MenuFlags,
						   (MenuFlags&CChannelMenu::FLAG_SHOWEVENTINFO)==0?24:0);
		return true;
	}

	return false;
}


class CTunerSelectMenu
{
	struct PopupInfo {
		const CChannelList *pChannelList;
		int Command;
		PopupInfo(const CChannelList *pList,int Cmd)
			: pChannelList(pList)
			, Command(Cmd)
		{
		}
	};

	HMENU m_hmenu;
	HWND m_hwnd;
	std::vector<PopupInfo> m_PopupList;

public:
	CTunerSelectMenu();
	~CTunerSelectMenu();
	bool Create(HWND hwnd);
	void Destroy();
	bool Popup(UINT Flags,int x,int y);
	bool OnInitMenuPopup(HMENU hmenu);
};

CTunerSelectMenu::CTunerSelectMenu()
	: m_hmenu(NULL)
{
}

CTunerSelectMenu::~CTunerSelectMenu()
{
	Destroy();
}

bool CTunerSelectMenu::Create(HWND hwnd)
{
	Destroy();

	m_hmenu=::CreatePopupMenu();
	m_hwnd=hwnd;

	HMENU hmenuSpace;
	const CChannelList *pChannelList;
	int Command;
	int i,j;
	LPCTSTR pszName;
	TCHAR szText[MAX_PATH*2];
	int Length;

	Command=CM_SPACE_CHANNEL_FIRST;
	pChannelList=ChannelManager.GetAllChannelList();
	if (ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		::AppendMenu(m_hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&A: すべて"));
	}
	Command+=pChannelList->NumChannels();
	for (i=0;i<ChannelManager.NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		hmenuSpace=::CreatePopupMenu();
		Length=::wsprintf(szText,TEXT("&%d: "),i);
		pszName=ChannelManager.GetTuningSpaceName(i);
		if (pszName!=NULL)
			CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
		else
			::wsprintf(szText+Length,TEXT("チューニング空間%d"),i);
		::AppendMenu(m_hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
		Command+=pChannelList->NumChannels();
	}

	::AppendMenu(m_hmenu,MFT_SEPARATOR,0,NULL);

	for (i=0;i<DriverManager.NumDrivers();i++) {
		CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0) {
			continue;
		}
		if (pDriverInfo->LoadTuningSpaceList(CDriverInfo::LOADTUNINGSPACE_NOLOADDRIVER)) {
			HMENU hmenuDriver=::CreatePopupMenu();
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (pChannelList->NumEnableChannels()==0) {
					Command+=pChannelList->NumChannels();
					continue;
				}
				if (pTuningSpaceList->NumSpaces()>1)
					hmenuSpace=::CreatePopupMenu();
				else
					hmenuSpace=hmenuDriver;
				m_PopupList.push_back(PopupInfo(pChannelList,Command));
				MENUINFO mi;
				mi.cbSize=sizeof(mi);
				mi.fMask=MIM_MENUDATA;
				mi.dwMenuData=m_PopupList.size()-1;
				::SetMenuInfo(hmenuSpace,&mi);
				Command+=pChannelList->NumChannels();
				if (hmenuSpace!=hmenuDriver) {
					pszName=pTuningSpaceList->GetTuningSpaceName(j);
					Length=::wsprintf(szText,TEXT("&%d: "),j);
					if (pszName!=NULL)
						CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
					else
						::wsprintf(szText,TEXT("チューニング空間%d"),j);
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
			::AppendMenu(m_hmenu,MF_POPUP | MFS_ENABLED,
						 reinterpret_cast<UINT_PTR>(hmenuDriver),szText);
		} else {
			CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
			::AppendMenu(m_hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		}
	}
	return true;
}

void CTunerSelectMenu::Destroy()
{
	if (m_hmenu!=NULL) {
		::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
	}
	m_hwnd=NULL;
	m_PopupList.clear();
}

bool CTunerSelectMenu::Popup(UINT Flags,int x,int y)
{
	if (m_hmenu==NULL)
		return false;
	::TrackPopupMenu(m_hmenu,Flags,x,y,0,m_hwnd,NULL);
	return true;
}

bool CTunerSelectMenu::OnInitMenuPopup(HMENU hmenu)
{
	if (m_hmenu==NULL)
		return false;

	if (ChannelMenuInitPopup(m_hmenu,hmenu))
		return true;

	bool fChannelMenu=false;
	int Count=::GetMenuItemCount(m_hmenu);
	int i,j;
	i=ChannelManager.NumSpaces();
	if (i>1)
		i++;
	for (i++;i<Count;i++) {
		HMENU hmenuChannel=::GetSubMenu(m_hmenu,i);
		int Items=::GetMenuItemCount(hmenuChannel);

		if (hmenuChannel==hmenu) {
			if (Items>0)
				return true;
			fChannelMenu=true;
			break;
		}
		if (Items>0) {
			for (j=0;j<Items;j++) {
				if (::GetSubMenu(hmenuChannel,j)==hmenu)
					break;
			}
			if (j<Items) {
				fChannelMenu=true;
				break;
			}
		}
	}

	if (fChannelMenu) {
		MENUINFO mi;

		mi.cbSize=sizeof(mi);
		mi.fMask=MIM_MENUDATA;
		if (!::GetMenuInfo(hmenu,&mi) || mi.dwMenuData>=m_PopupList.size())
			return false;
		const PopupInfo &Info=m_PopupList[mi.dwMenuData];
		unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO;
		if (Info.pChannelList->NumEnableChannels()<=30)
			MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
		else
			MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
		ChannelMenu.Create(Info.pChannelList,-1,Info.Command,hmenu,
						   m_hwnd,MenuFlags,
						   (MenuFlags&CChannelMenu::FLAG_SHOWEVENTINFO)==0?24:0);
		return true;
	}

	return false;
}

static CTunerSelectMenu TunerSelectMenu;


enum LanguageTextType {
	LANGUAGE_TEXT_LONG,
	LANGUAGE_TEXT_SIMPLE,
	LANGUAGE_TEXT_SHORT
};

static LPCTSTR GetLanguageText(DWORD LanguageCode,LanguageTextType Type)
{
	static const struct {
		DWORD LanguageCode;
		LPCTSTR pszLongText;
		LPCTSTR pszSimpleText;
		LPCTSTR pszShortText;
	} LanguageList[] = {
		{LANGUAGE_CODE_JPN,	TEXT("日本語"),		TEXT("日本語"),	TEXT("日")},
		{LANGUAGE_CODE_ENG,	TEXT("英語"),		TEXT("英語"),	TEXT("英")},
		{LANGUAGE_CODE_DEU,	TEXT("ドイツ語"),	TEXT("独語"),	TEXT("独")},
		{LANGUAGE_CODE_FRA,	TEXT("フランス語"),	TEXT("仏語"),	TEXT("仏")},
		{LANGUAGE_CODE_ITA,	TEXT("イタリア語"),	TEXT("伊語"),	TEXT("伊")},
		{LANGUAGE_CODE_RUS,	TEXT("ロシア語"),	TEXT("露語"),	TEXT("露")},
		{LANGUAGE_CODE_ZHO,	TEXT("中国語"),		TEXT("中国語"),	TEXT("中")},
		{LANGUAGE_CODE_KOR,	TEXT("韓国語"),		TEXT("韓国語"),	TEXT("韓")},
		{LANGUAGE_CODE_SPA,	TEXT("スペイン語"),	TEXT("西語"),	TEXT("西")},
		{LANGUAGE_CODE_ETC,	TEXT("外国語"),		TEXT("外国語"),	TEXT("外")},
	};

	int i;
	for (i=0;i<lengthof(LanguageList)-1;i++) {
		if (LanguageList[i].LanguageCode==LanguageCode)
			break;
	}
	switch (Type) {
	case LANGUAGE_TEXT_LONG:	return LanguageList[i].pszLongText;
	case LANGUAGE_TEXT_SIMPLE:	return LanguageList[i].pszSimpleText;
	case LANGUAGE_TEXT_SHORT:	return LanguageList[i].pszShortText;
	}
	return TEXT("");
}




CUICore::CUICore()
	: m_pSkin(NULL)
	, m_fStandby(false)
	, m_fFullscreen(false)
	, m_fAlwaysOnTop(false)

	, m_hicoLogoBig(NULL)
	, m_hicoLogoSmall(NULL)

	, m_fViewerInitializeError(false)

	, m_fScreenSaverActiveOriginal(FALSE)
	/*
	, m_fLowPowerActiveOriginal(FALSE)
	, m_fPowerOffActiveOriginal(FALSE)
	*/
{
}

CUICore::~CUICore()
{
	if (m_hicoLogoBig!=NULL)
		::DeleteObject(m_hicoLogoBig);
	if (m_hicoLogoSmall!=NULL)
		::DeleteObject(m_hicoLogoSmall);
}

bool CUICore::SetSkin(CUISkin *pSkin)
{
	if (m_pSkin!=NULL)
		m_pSkin->m_pCore=NULL;
	if (pSkin!=NULL)
		pSkin->m_pCore=this;
	m_pSkin=pSkin;
	return true;
}

HWND CUICore::GetMainWindow() const
{
	if (m_pSkin==NULL)
		return NULL;
	return m_pSkin->GetMainWindow();
}

bool CUICore::InitializeViewer()
{
	if (m_pSkin==NULL)
		return false;
	bool fOK=m_pSkin->InitializeViewer();
	m_fViewerInitializeError=!fOK;
	return fOK;
}

bool CUICore::FinalizeViewer()
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->FinalizeViewer();
}

bool CUICore::IsViewerEnabled() const
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->IsViewerEnabled();
}

bool CUICore::EnableViewer(bool fEnable)
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->EnableViewer(fEnable);
}

bool CUICore::SetZoomRate(int Rate,int Factor)
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->SetZoomRate(Rate,Factor);
}

bool CUICore::GetZoomRate(int *pRate,int *pFactor) const
{
	if (m_pSkin==NULL)
		return false;
	if (!m_pSkin->GetZoomRate(pRate,pFactor)
			|| (pRate!=NULL && *pRate<1) || (pFactor!=NULL && *pFactor<1))
		return false;
	return true;
}

int CUICore::GetZoomPercentage() const
{
	if (m_pSkin==NULL)
		return 0;
	int Rate,Factor;
	if (!m_pSkin->GetZoomRate(&Rate,&Factor) || Factor==0)
		return false;
	return (Rate*100+Factor/2)/Factor;
}

int CUICore::GetVolume() const
{
	return CoreEngine.GetVolume();
}

bool CUICore::SetVolume(int Volume,bool fOSD)
{
	if (!CoreEngine.SetVolume(Volume))
		return false;
	if (m_pSkin!=NULL)
		m_pSkin->OnVolumeChanged(fOSD);
	PluginList.SendVolumeChangeEvent(Volume,false);
	return true;
}

bool CUICore::GetMute() const
{
	return CoreEngine.GetMute();
}

bool CUICore::SetMute(bool fMute)
{
	if (fMute!=GetMute()) {
		if (!CoreEngine.SetMute(fMute))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnMuteChanged();
		PluginList.SendVolumeChangeEvent(GetVolume(),fMute);
	}
	return true;
}

int CUICore::GetStereoMode() const
{
	return CoreEngine.GetStereoMode();
}

bool CUICore::SetStereoMode(int StereoMode)
{
	if (StereoMode!=GetStereoMode()) {
		if (!CoreEngine.SetStereoMode(StereoMode))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnStereoModeChanged();
		PluginList.SendStereoModeChangeEvent(StereoMode);
	}
	return true;
}

int CUICore::GetAudioStream() const
{
	return CoreEngine.m_DtvEngine.GetAudioStream();
}

int CUICore::GetNumAudioStreams() const
{
	return CoreEngine.m_DtvEngine.GetAudioStreamNum();
}

bool CUICore::SetAudioStream(int Stream)
{
	if (Stream!=GetAudioStream()) {
		if (!CoreEngine.m_DtvEngine.SetAudioStream(Stream))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnAudioStreamChanged();
		PluginList.SendAudioStreamChangeEvent(Stream);
	}
	return true;
}

bool CUICore::SwitchStereoMode()
{
	return SetStereoMode((GetStereoMode()+1)%3);
}

bool CUICore::SwitchAudio()
{
	const int NumChannels=CoreEngine.m_DtvEngine.GetAudioChannelNum();
	bool fResult;

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		fResult=SwitchStereoMode();
	} else {
		const int NumStreams=CoreEngine.m_DtvEngine.GetAudioStreamNum();

		if (NumStreams>1)
			fResult=SetAudioStream((GetAudioStream()+1)%NumStreams);
		else if (NumChannels==2)
			fResult=SwitchStereoMode();
		else
			fResult=false;
	}
	return fResult;
}

int CUICore::FormatCurrentAudioText(LPTSTR pszText,int MaxLength) const
{
	if (pszText==NULL || MaxLength<1)
		return 0;

	const int NumChannels=CoreEngine.m_DtvEngine.GetAudioChannelNum();
	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_INVALID)
		return 0;

	TCHAR szText[32];
	LPTSTR p=szText;

	if (CoreEngine.m_DtvEngine.GetAudioStreamNum()>1)
		p+=::wsprintf(szText,TEXT("#%d: "),CoreEngine.m_DtvEngine.GetAudioStream()+1);
	switch (NumChannels) {
	case 1:
		::lstrcpy(p,TEXT("Mono"));
		break;
	case CMediaViewer::AUDIO_CHANNEL_DUALMONO:
	case 2:
		{
			const int StereoMode=AppMain.GetUICore()->GetStereoMode();
			CTsAnalyzer::EventAudioInfo AudioInfo;
			bool fValidAudioInfo=CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo);

			if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO
					/*|| (fValidAudioInfo && AudioInfo.ComponentType==0x02)*/) {
				// Dual mono
				// ES multilingual flag が立っているのに両方日本語の場合がある…
				if (fValidAudioInfo
						&& AudioInfo.bESMultiLingualFlag
						&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
					// 2カ国語
					p+=::wsprintf(p,TEXT("[二] "));
					switch (StereoMode) {
					case 0:
						::wsprintf(p,TEXT("%s+%s"),
								   GetLanguageText(AudioInfo.LanguageCode,LANGUAGE_TEXT_SHORT),
								   GetLanguageText(AudioInfo.LanguageCode2,LANGUAGE_TEXT_SHORT));
						break;
					case 1:
						::lstrcpy(p,GetLanguageText(AudioInfo.LanguageCode,LANGUAGE_TEXT_SIMPLE));
						break;
					case 2:
						::lstrcpy(p,GetLanguageText(AudioInfo.LanguageCode2,LANGUAGE_TEXT_SIMPLE));
						break;
					}
				} else {
					::wsprintf(p,TEXT("Mono (%s)"),
							   StereoMode==0?TEXT("主+副"):
							   StereoMode==1?TEXT("主"):TEXT("副"));
				}
			} else {
				::lstrcpy(p,TEXT("Stereo"));
				if (StereoMode!=0)
					::lstrcat(p,StereoMode==1?TEXT("(L)"):TEXT("(R)"));
			}
		}
		break;
	case 6:
		::lstrcpy(p,TEXT("5.1ch"));
		break;
	default:
		::wsprintf(p,TEXT("%dch"),NumChannels);
		break;
	}

	::lstrcpyn(pszText,szText,MaxLength);
	return ::lstrlen(pszText);
}

bool CUICore::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (m_pSkin!=NULL) {
			if (!m_pSkin->OnStandbyChange(fStandby))
				return false;
		}
		m_fStandby=fStandby;
	}
	return true;
}

bool CUICore::GetResident() const
{
	return ResidentManager.GetResident();
}

bool CUICore::SetResident(bool fResident)
{
	return ResidentManager.SetResident(fResident);
}

bool CUICore::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (m_pSkin==NULL)
			return false;
		if (!m_pSkin->OnFullscreenChange(fFullscreen))
			return false;
		m_fFullscreen=fFullscreen;
		PluginList.SendFullscreenChangeEvent(fFullscreen);
	}
	return true;
}

bool CUICore::ToggleFullscreen()
{
	return SetFullscreen(!m_fFullscreen);
}

bool CUICore::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		if (m_pSkin==NULL)
			return false;
		if (!m_pSkin->SetAlwaysOnTop(fTop))
			return false;
		m_fAlwaysOnTop=fTop;
	}
	return true;
}

bool CUICore::PreventDisplaySave(bool fPrevent)
{
	HWND hwnd=GetMainWindow();

	if (fPrevent) {
		bool fNoScreenSaver=ViewOptions.GetNoScreenSaver();
		bool fNoMonitorLowPower=ViewOptions.GetNoMonitorLowPower();
		bool fNoMonitorLowPowerActiveOnly=ViewOptions.GetNoMonitorLowPowerActiveOnly();

		if (!fNoScreenSaver && m_fScreenSaverActiveOriginal) {
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								 SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
		if (!fNoMonitorLowPower || fNoMonitorLowPowerActiveOnly) {
#if 1
			if (hwnd!=NULL)
				::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
#else
			if (m_fPowerOffActiveOriginal) {
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
									 SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
				m_fPowerOffActiveOriginal=FALSE;
			}
			if (m_fLowPowerActiveOriginal) {
				SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
									 SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
				m_fLowPowerActiveOriginal=FALSE;
			}
#endif
		}
		if (fNoScreenSaver && !m_fScreenSaverActiveOriginal) {
			if (!SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,
									  &m_fScreenSaverActiveOriginal,0))
				m_fScreenSaverActiveOriginal=FALSE;
			if (m_fScreenSaverActiveOriginal)
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,NULL,
									 0/*SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE*/);
		}
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
#if 1
			// SetThreadExecutionState() を呼ぶタイマー
			if (hwnd!=NULL)
				::SetTimer(hwnd,CUISkin::TIMER_ID_DISPLAY,10000,NULL);
#else
			if (!m_fPowerOffActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETPOWEROFFACTIVE,0,
										  &m_fPowerOffActiveOriginal,0))
					m_fPowerOffActiveOriginal=FALSE;
				if (m_fPowerOffActiveOriginal)
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			if (!m_fLowPowerActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETLOWPOWERACTIVE,0,
										  &m_fLowPowerActiveOriginal,0))
					m_fLowPowerActiveOriginal=FALSE;
				if (m_fLowPowerActiveOriginal)
					SystemParametersInfo(SPI_SETLOWPOWERACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
#endif
		}
	} else {
		if (hwnd!=NULL)
			::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
		if (m_fScreenSaverActiveOriginal) {
			::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
#if 0
		if (m_fPowerOffActiveOriginal) {
			::SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
								   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			m_fPowerOffActiveOriginal=FALSE;
		}
		if (m_fLowPowerActiveOriginal) {
			::SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
								   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			m_fLowPowerActiveOriginal=FALSE;
		}
#endif
	}
	return true;
}

void CUICore::PopupMenu(const POINT *pPos)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	MainMenu.Popup(TPM_RIGHTBUTTON,pt.x,pt.y,m_pSkin->GetMainWindow(),true);
}

void CUICore::PopupSubMenu(int SubMenu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	MainMenu.PopupSubMenu(SubMenu,Flags,pt.x,pt.y,m_pSkin->GetMainWindow());
}

bool CUICore::ShowSpecialMenu(MenuType Menu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	switch (Menu) {
	case MENU_TUNERSELECT:
		TunerSelectMenu.Create(GetMainWindow());
		TunerSelectMenu.Popup(Flags,pt.x,pt.y);
		TunerSelectMenu.Destroy();
		break;

	case MENU_RECORD:
		{
			CPopupMenu Menu(hInst,IDM_RECORD);

			Menu.CheckItem(CM_RECORDEVENT,RecordManager.GetStopOnEventEnd());
			Menu.EnableItem(CM_RECORD_PAUSE,RecordManager.IsRecording());
			Menu.CheckItem(CM_RECORD_PAUSE,RecordManager.IsPaused());
			bool fTimeShift=RecordOptions.IsTimeShiftRecordingEnabled();
			Menu.EnableItem(CM_TIMESHIFTRECORDING,fTimeShift && !RecordManager.IsRecording());
			Menu.CheckItem(CM_ENABLETIMESHIFTRECORDING,fTimeShift);
			Menu.EnableItem(CM_SHOWRECORDREMAINTIME,
				RecordManager.IsRecording() && RecordManager.IsStopTimeSpecified());
			Menu.CheckItem(CM_SHOWRECORDREMAINTIME,RecordOptions.GetShowRemainTime());
			Menu.CheckItem(CM_EXITONRECORDINGSTOP,MainWindow.GetExitOnRecordingStop());
			Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Popup(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CAPTURE:
		{
			CPopupMenu Menu(hInst,IDM_CAPTURE);

			Menu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
				CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize());
			if (fShowCaptureWindow)
				Menu.CheckItem(CM_CAPTUREPREVIEW,true);
			Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Popup(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_BUFFERING:
		{
			CPopupMenu Menu(hInst,IDM_BUFFERING);

			Menu.CheckItem(CM_ENABLEBUFFERING,CoreEngine.GetPacketBuffering());
			Menu.EnableItem(CM_RESETBUFFER,CoreEngine.GetPacketBuffering());
			Menu.Popup(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_STREAMERROR:
		{
			CPopupMenu Menu(hInst,IDM_ERROR);

			Menu.Popup(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CLOCK:
		{
			CPopupMenu Menu(hInst,IDM_TIME);

			Menu.CheckItem(CM_SHOWTOTTIME,StatusOptions.GetShowTOTTime());
			Menu.Popup(GetMainWindow(),&pt,Flags);
		}
		break;

	default:
		return false;
	}
	return true;
}

void CUICore::InitChannelMenu(HMENU hmenu)
{
	if (pNetworkRemocon!=NULL) {
		InitNetworkRemoconChannelMenu(hmenu);
		return;
	}

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();

	ChannelMenu.Destroy();
	ClearMenu(hmenu);
	if (pList==NULL)
		return;

	if (!CoreEngine.IsNetworkDriver()) {
		unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO;
		if (pList->NumEnableChannels()<=30)
			MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
		else
			MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
		ChannelMenu.Create(pList,ChannelManager.GetCurrentChannel(),
						   CM_CHANNEL_FIRST,hmenu,GetMainWindow(),MenuFlags,
						   (MenuFlags&CChannelMenu::FLAG_SHOWEVENTINFO)==0?18:0);
	} else {
		bool fControlKeyID=pList->HasRemoteControlKeyID();
		for (int i=0,j=0;i<pList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pList->GetChannelInfo(i);
			TCHAR szText[MAX_CHANNEL_NAME+4];

			if (pChInfo->IsEnabled()) {
				wsprintf(szText,TEXT("%d: %s"),
					fControlKeyID?pChInfo->GetChannelNo():i+1,pChInfo->GetName());
				AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
					| (j!=0 && j%18==0?MF_MENUBREAK:0),CM_CHANNEL_FIRST+i,szText);
				j++;
			}
		}
		if (ChannelManager.GetCurrentChannel()>=0
				&& pList->IsEnabled(ChannelManager.GetCurrentChannel()))
			::CheckMenuRadioItem(hmenu,CM_CHANNEL_FIRST,
				CM_CHANNEL_FIRST+pList->NumChannels()-1,
				CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
				MF_BYCOMMAND);
	}
}

void CUICore::InitNetworkRemoconChannelMenu(HMENU hmenu)
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
			::CheckMenuRadioItem(hmenu,
				CM_CHANNELNO_FIRST,CM_CHANNELNO_FIRST+Max-1,
				CM_CHANNEL_FIRST+ChannelManager.GetNetworkRemoconCurrentChannel(),
				MF_BYCOMMAND);
	}
	pPortList=ChannelManager.GetDriverChannelList(0);
	for (i=0;i<pPortList->NumChannels();i++) {
		wsprintf(szText,TEXT("%d: %s"),
							pPortList->GetChannelNo(i),pPortList->GetName(i));
		AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
			| ((i!=0 && i%16==0) || (i==0 && RemoconChList.NumChannels()>0)?
															MF_MENUBREAK:0),
												CM_CHANNEL_FIRST+i,szText);
	}
	if (ChannelManager.GetCurrentChannel()>=0)
		::CheckMenuRadioItem(hmenu,
			CM_CHANNEL_FIRST,CM_CHANNEL_FIRST+pPortList->NumChannels()-1,
			CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
			MF_BYCOMMAND);
}

void CUICore::InitTunerMenu(HMENU hmenu)
{
	TCHAR szText[MAX_PATH*2];
	int Length;
	int i;

	ChannelMenu.Destroy();
	ClearMenu(hmenu);

	// 各チューニング空間のメニューを追加する
	// 実際のメニューの設定は WM_INITMENUPOPUP で行っている
	HMENU hmenuSpace;
	LPCTSTR pszName;
	if (ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&A: すべて"));
	}
	for (i=0;i<ChannelManager.NumSpaces();i++) {
		hmenuSpace=::CreatePopupMenu();
		Length=::wsprintf(szText,TEXT("&%d: "),i);
		pszName=ChannelManager.GetTuningSpaceName(i);
		if (pszName!=NULL)
			CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
		else
			::wsprintf(szText+Length,TEXT("チューニング空間%d"),i);
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
	}

	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);

	::LoadString(hInst,CM_CHANNELDISPLAYMENU,szText,lengthof(szText));
	::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED | (ChannelDisplayMenu.GetVisible()?MFS_CHECKED:MFS_UNCHECKED),
				 CM_CHANNELDISPLAYMENU,szText);
	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	int CurDriver=-1;
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		CopyToMenuText(pDriverInfo->GetFileName(),szText,lengthof(szText));
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0)
			CurDriver=i;
	}
	if (CurDriver<0 && CoreEngine.IsDriverLoaded()) {
		CopyToMenuText(CoreEngine.GetDriverFileName(),szText,lengthof(szText));
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_FIRST+i,szText);
		CurDriver=i++;
	}
	::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_BROWSE,TEXT("参照..."));
	if (CurDriver>=0)
		::CheckMenuRadioItem(hmenu,CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
							 CM_DRIVER_FIRST+CurDriver,MF_BYCOMMAND);
	Accelerator.SetMenuAccel(hmenu);
}

bool CUICore::ProcessTunerMenu(int Command)
{
	if (Command<CM_SPACE_CHANNEL_FIRST || Command>CM_SPACE_CHANNEL_LAST)
		return false;

	const CChannelList *pChannelList;
	int CommandBase;
	int i,j;

	CommandBase=CM_SPACE_CHANNEL_FIRST;
	pChannelList=ChannelManager.GetAllChannelList();
	if (pChannelList->NumChannels()>0) {
		if (Command-CommandBase<pChannelList->NumChannels())
			return AppMain.SetChannel(-1,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (int i=0;i<ChannelManager.NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		if (Command-CommandBase<pChannelList->NumChannels())
			return AppMain.SetChannel(i,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())==0)
			continue;
		if (pDriverInfo->IsTuningSpaceListLoaded()) {
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (Command-CommandBase<pChannelList->NumChannels()) {
					if (!AppMain.SetDriver(pDriverInfo->GetFileName()))
						return false;
					return AppMain.SetChannel(j,Command-CommandBase);
				}
				CommandBase+=pChannelList->NumChannels();
			}
		}
	}
	return false;
}

bool CUICore::DoCommand(int Command)
{
	if (m_pSkin==NULL || Command<=0 || Command>0xFFFF)
		return false;
	::SendMessage(m_pSkin->GetMainWindow(),WM_COMMAND,MAKEWPARAM(Command,0),0);
	return true;
}

bool CUICore::DoCommand(LPCTSTR pszCommand)
{
	if (pszCommand==NULL)
		return false;
	int Command=CommandList.ParseText(pszCommand);
	if (Command==0)
		return false;
	return DoCommand(Command);
}

bool CUICore::ConfirmStopRecording()
{
	HWND hwnd;

	if (m_pSkin!=NULL)
		hwnd=m_pSkin->GetVideoHostWindow();
	else
		hwnd=NULL;
	return RecordOptions.ConfirmStatusBarStop(hwnd);
}

bool CUICore::UpdateIcon()
{
	HICON hicoBig=NULL,hicoSmall=NULL;

	if (ViewOptions.GetUseLogoIcon() && CoreEngine.IsDriverOpen()) {
		const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();

		if (pCurChannel!=NULL) {
			HBITMAP hbmLogo;

			hbmLogo=LogoManager.GetAssociatedLogoBitmap(
				pCurChannel->GetNetworkID(),pCurChannel->GetServiceID(),
				CLogoManager::LOGOTYPE_BIG);
			if (hbmLogo!=NULL) {
				int Width=::GetSystemMetrics(SM_CXICON);
				int Height=::GetSystemMetrics(SM_CYICON);
				hicoBig=CreateIconFromBitmap(hbmLogo,Width,Height,
											 Width,(Width*9+15)/16);
			}
			hbmLogo=LogoManager.GetAssociatedLogoBitmap(
				pCurChannel->GetNetworkID(),pCurChannel->GetServiceID(),
				CLogoManager::LOGOTYPE_SMALL);
			if (hbmLogo!=NULL) {
				int Width=::GetSystemMetrics(SM_CXSMICON);
				int Height=::GetSystemMetrics(SM_CYSMICON);
				hicoSmall=CreateIconFromBitmap(hbmLogo,Width,Height,
											   // 本来の比率より縦長にしている(見栄えのため)
											   Width,Width*11/16);
			}
		}
	}
	HWND hwnd=GetMainWindow();
	if (hwnd!=NULL) {
		HICON hicoDefault;

		if (hicoBig==NULL || hicoSmall==NULL)
			hicoDefault=::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
		::SendMessage(hwnd,WM_SETICON,ICON_BIG,
					  reinterpret_cast<LPARAM>(hicoBig!=NULL?hicoBig:hicoDefault));
		::SendMessage(hwnd,WM_SETICON,ICON_SMALL,
					  reinterpret_cast<LPARAM>(hicoSmall!=NULL?hicoSmall:hicoDefault));
	}
	if (m_hicoLogoBig!=NULL)
		::DestroyIcon(m_hicoLogoBig);
	m_hicoLogoBig=hicoBig;
	if (m_hicoLogoSmall!=NULL)
		::DestroyIcon(m_hicoLogoSmall);
	m_hicoLogoSmall=hicoSmall;
	return true;
}

void CUICore::OnTunerChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerChanged();
	UpdateIcon();
}

void CUICore::OnTunerOpened()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerOpened();
	UpdateIcon();
}

void CUICore::OnTunerClosed()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerClosed();
	UpdateIcon();
}

void CUICore::OnChannelListChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnChannelListChanged();
}

void CUICore::OnChannelChanged(bool fSpaceChanged)
{
	if (m_pSkin!=NULL)
		m_pSkin->OnChannelChanged(fSpaceChanged);
	UpdateIcon();
}

void CUICore::OnServiceChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnServiceChanged();
	PluginList.SendServiceChangeEvent();
}

void CUICore::OnRecordingStarted()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnRecordingStarted();
	PluginList.SendRecordStatusChangeEvent();
}

void CUICore::OnRecordingStopped()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnRecordingStopped();
	PluginList.SendRecordStatusChangeEvent();
}




// 操作パネルのアイテムを設定する
static void InitControlPanel()
{
	ControlPanel.AddItem(new CTunerControlItem);
	ControlPanel.AddItem(new CChannelControlItem);

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();
	for (int i=0;i<12;i++) {
		TCHAR szText[4];
		CControlPanelButton *pItem;

		wsprintf(szText,TEXT("%d"),i+1);
		pItem=new CControlPanelButton(CM_CHANNELNO_FIRST+i,szText,i%6==0,1);
		if (pList==NULL || pList->FindChannelNo(i+1)<0)
			pItem->SetEnable(false);
		ControlPanel.AddItem(pItem);
	}

	ControlPanel.AddItem(new CVideoControlItem);
	ControlPanel.AddItem(new CVolumeControlItem);
	ControlPanel.AddItem(new CAudioControlItem);
}


static void UpdateControlPanelStatus()
{
	const CChannelList *pList=ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();

	for (int i=0;i<12;i++) {
		CControlPanelItem *pItem=ControlPanel.GetItem(CONTROLPANEL_ITEM_CHANNEL_1+i);
		if (pItem!=NULL) {
			pItem->SetEnable(pList!=NULL && pList->FindChannelNo(i+1)>=0);
			pItem->SetCheck(false);
		}
	}
	if (pCurChannel!=NULL) {
		if (pCurChannel->GetChannelNo()>=1 && pCurChannel->GetChannelNo()<=12)
			ControlPanel.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
										CM_CHANNELNO_FIRST+pCurChannel->GetChannelNo()-1);
	}
}




/*
	配色を適用する
*/
static bool ColorSchemeApplyProc(const CColorScheme *pColorScheme)
{
	Theme::GradientInfo Gradient1,Gradient2,Gradient3,Gradient4;
	Theme::BorderInfo Border;

	MainWindow.ApplyColorScheme(pColorScheme);

	CStatusView::ThemeInfo StatusTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_STATUSITEM,
						   &StatusTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_STATUSHIGHLIGHTITEM,
						   &StatusTheme.HighlightItemStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_STATUS,
								&StatusTheme.Border);
	StatusView.SetTheme(&StatusTheme);
	CaptureWindow.SetStatusTheme(&StatusTheme);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_PROGRAMGUIDESTATUS,
								&StatusTheme.Border);
	ProgramGuideFrame.SetStatusTheme(&StatusTheme);
	ProgramGuideDisplay.SetStatusTheme(&StatusTheme);
	CRecordStatusItem *pRecordStatus=dynamic_cast<CRecordStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_RECORD));
	if (pRecordStatus!=NULL)
		pRecordStatus->SetCircleColor(pColorScheme->GetColor(CColorScheme::COLOR_STATUSRECORDINGCIRCLE));

	CSideBar::ThemeInfo SideBarTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_SIDEBARITEM,
						   &SideBarTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_SIDEBARHIGHLIGHTITEM,
						   &SideBarTheme.HighlightItemStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SIDEBAR,
								&SideBarTheme.Border);
	SideBar.SetTheme(&SideBarTheme);

	CPanel::ThemeInfo PanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TITLE,
						   &PanelTheme.TitleStyle);
	PanelFrame.SetTheme(&PanelTheme);

	CPanelForm::ThemeInfo PanelFormTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TAB,
						   &PanelFormTheme.TabStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_CURTAB,
						   &PanelFormTheme.CurTabStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TABMARGIN,
						   &PanelFormTheme.TabMarginStyle);
	PanelFormTheme.BackColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	PanelFormTheme.BorderColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABLINE);
	PanelForm.SetTheme(&PanelFormTheme);

	InfoPanel.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT));
	InfoPanel.SetProgramInfoColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOTEXT));

	CProgramListPanel::ThemeInfo ProgramListPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_EVENT,
						   &ProgramListPanelTheme.EventTextStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_CUREVENT,
						   &ProgramListPanelTheme.CurEventTextStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_TITLE,
						   &ProgramListPanelTheme.EventNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_CURTITLE,
						   &ProgramListPanelTheme.CurEventNameStyle);
	ProgramListPanelTheme.MarginColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	ProgramListPanel.SetTheme(&ProgramListPanelTheme);

	CChannelPanel::ThemeInfo ChannelPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CHANNELNAME,
						   &ChannelPanelTheme.ChannelNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELNAME,
						   &ChannelPanelTheme.CurChannelNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_EVENTNAME1,
						   &ChannelPanelTheme.EventStyle[0]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_EVENTNAME2,
						   &ChannelPanelTheme.EventStyle[1]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME1,
						   &ChannelPanelTheme.CurChannelEventStyle[0]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME2,
						   &ChannelPanelTheme.CurChannelEventStyle[1]);
	ChannelPanelTheme.MarginColor=pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	ChannelPanel.SetTheme(&ChannelPanelTheme);

	CControlPanel::ThemeInfo ControlPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_CONTROLPANELITEM,
						   &ControlPanelTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CONTROLPANELHIGHLIGHTITEM,
						   &ControlPanelTheme.OverItemStyle);
	ControlPanelTheme.MarginColor=
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELMARGIN);
	ControlPanel.SetTheme(&ControlPanelTheme);

	CaptionPanel.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_CAPTIONPANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CAPTIONPANELTEXT));

	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_NOTIFICATIONBARBACK,&Gradient1);
	NotificationBar.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARWARNINGTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARERRORTEXT));

	static const struct {
		int From,To;
	} ProgramGuideColorMap[] = {
		{CColorScheme::COLOR_PROGRAMGUIDEBACK,				CProgramGuide::COLOR_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDETEXT,				CProgramGuide::COLOR_TEXT},
		{CColorScheme::COLOR_PROGRAMGUIDECHANNELTEXT,		CProgramGuide::COLOR_CHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDECURCHANNELTEXT,	CProgramGuide::COLOR_CURCHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDETIMETEXT,			CProgramGuide::COLOR_TIMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDETIMELINE,			CProgramGuide::COLOR_TIMELINE},
		{CColorScheme::COLOR_PROGRAMGUIDECURTIMELINE,		CProgramGuide::COLOR_CURTIMELINE},
	};
	for (int i=0;i<lengthof(ProgramGuideColorMap);i++)
		ProgramGuide.SetColor(ProgramGuideColorMap[i].To,
							  pColorScheme->GetColor(ProgramGuideColorMap[i].From));
	for (int i=CProgramGuide::COLOR_CONTENT_FIRST,j=0;i<=CProgramGuide::COLOR_CONTENT_LAST;i++,j++)
		ProgramGuide.SetColor(i,pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_CONTENT_FIRST+j));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECHANNELBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECURCHANNELBACK,&Gradient2);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDETIMEBACK,&Gradient3);
	Theme::GradientInfo TimeGradients[CProgramGuide::TIME_BAR_BACK_COLORS];
	for (int i=0;i<CProgramGuide::TIME_BAR_BACK_COLORS;i++)
		pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDETIME0TO2BACK+i,&TimeGradients[i]);
	ProgramGuide.SetBackColors(&Gradient1,&Gradient2,&Gradient3,TimeGradients);

	PluginList.SendColorChangeEvent();

	return true;
}


static void ApplyEventInfoFont()
{
	ProgramListPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	ChannelPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	ProgramGuide.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	CProgramInfoStatusItem *pProgramInfo=dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pProgramInfo!=NULL)
		pProgramInfo->SetEventInfoFont(EpgOptions.GetEventInfoFont());
}




class COptionDialog : public COptionFrame
{
public:
	enum {
		PAGE_GENERAL,
		PAGE_VIEW,
		PAGE_OSD,
		PAGE_STATUS,
		PAGE_SIDEBAR,
		PAGE_PANEL,
		PAGE_COLORSCHEME,
		PAGE_OPERATION,
		PAGE_ACCELERATOR,
		PAGE_CONTROLLER,
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
	HBITMAP m_hbmIcons;
	DrawUtil::CFont m_TitleFont;
	bool m_fSettingError;

	void CreatePage(int Page);
	void SetPage(int Page);
	static COptionDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
// COptionFrame
	void OnSettingError(COptions *pOptions);
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
	{TEXT("サイドバー"),			MAKEINTRESOURCE(IDD_OPTIONS_SIDEBAR),
		CSideBarOptions::DlgProc,		&SideBarOptions,	RGB(192,64,128)},
	{TEXT("パネル"),				MAKEINTRESOURCE(IDD_OPTIONS_PANEL),
		CPanelOptions::DlgProc,			&PanelOptions,		RGB(0,255,128)},
	{TEXT("配色"),					MAKEINTRESOURCE(IDD_OPTIONS_COLORSCHEME),
		CColorSchemeOptions::DlgProc,	&ColorSchemeOptions,RGB(0,128,255)},
	{TEXT("操作"),					MAKEINTRESOURCE(IDD_OPTIONS_OPERATION),
		COperationOptions::DlgProc,		&OperationOptions,	RGB(128,128,0)},
	{TEXT("キー割り当て"),			MAKEINTRESOURCE(IDD_OPTIONS_ACCELERATOR),
		CAccelerator::DlgProc,			&Accelerator,		RGB(128,255,64)},
	{TEXT("リモコン"),				MAKEINTRESOURCE(IDD_OPTIONS_CONTROLLER),
		CControllerManager::DlgProc,	&ControllerManager,	RGB(255,255,128)},
	{TEXT("ドライバ別設定"),		MAKEINTRESOURCE(IDD_OPTIONS_DRIVER),
		CDriverOptions::DlgProc,		&DriverOptions,		RGB(128,255,128)},
	{TEXT("再生"),					MAKEINTRESOURCE(IDD_OPTIONS_PLAYBACK),
		CPlaybackOptions::DlgProc,		&PlaybackOptions,	RGB(32,64,192)},
	{TEXT("録画"),					MAKEINTRESOURCE(IDD_OPTIONS_RECORD),
		CRecordOptions::DlgProc,		&RecordOptions,		RGB(128,0,160)},
	{TEXT("キャプチャ"),			MAKEINTRESOURCE(IDD_OPTIONS_CAPTURE),
		CCaptureOptions::DlgProc,		&CaptureOptions,	RGB(0,128,128)},
	{TEXT("チャンネルスキャン"),	MAKEINTRESOURCE(IDD_OPTIONS_CHANNELSCAN),
		CChannelScan::DlgProc,			&ChannelScan,		RGB(0,160,255)},
	{TEXT("EPG/番組情報"),			MAKEINTRESOURCE(IDD_OPTIONS_EPG),
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
	: m_CurrentPage(0)
	, m_hbmIcons(NULL)
{
}


COptionDialog::~COptionDialog()
{
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


void COptionDialog::SetPage(int Page)
{
	if (Page>=0 && Page<NUM_PAGES && m_CurrentPage!=Page) {
		if (m_hDlgList[Page]==NULL) {
			HCURSOR hcurOld;

			hcurOld=::SetCursor(LoadCursor(NULL,IDC_WAIT));
			CreatePage(Page);
			::SetCursor(hcurOld);
		}
		::ShowWindow(m_hDlgList[m_CurrentPage],SW_HIDE);
		::ShowWindow(m_hDlgList[Page],SW_SHOW);
		m_CurrentPage=Page;
		DlgListBox_SetCurSel(m_hDlg,IDC_OPTIONS_LIST,Page);
		InvalidateDlgItem(m_hDlg,IDC_OPTIONS_TITLE);
	}
}


COptionDialog *COptionDialog::GetThis(HWND hDlg)
{
	return static_cast<COptionDialog*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK COptionDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const int ICON_WIDTH=16,ICON_HEIGHT=16,LIST_MARGIN=1,ICON_TEXT_MARGIN=4;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COptionDialog *pThis=reinterpret_cast<COptionDialog*>(lParam);
			int i;

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;
			COptions::ClearGeneralUpdateFlags();
			for (i=0;i<NUM_PAGES;i++) {
				m_PageList[i].pOptions->ClearUpdateFlags();
				pThis->m_hDlgList[i]=NULL;
				/*
				::SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_ADDSTRING,0,
					reinterpret_cast<LPARAM>(pThis->m_PageList[i].pszTitle));
				*/
				::SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_ADDSTRING,0,i);
			}
			if (pThis->m_StartPage>=0 && pThis->m_StartPage<NUM_PAGES)
				pThis->m_CurrentPage=pThis->m_StartPage;
			pThis->CreatePage(pThis->m_CurrentPage);
			::ShowWindow(pThis->m_hDlgList[pThis->m_CurrentPage],SW_SHOW);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_SETCURSEL,pThis->m_CurrentPage,0);
			pThis->m_hbmIcons=::LoadBitmap(hInst,MAKEINTRESOURCE(IDB_OPTIONS));
			if (!pThis->m_TitleFont.IsCreated()) {
				HFONT hfont;
				LOGFONT lf;

				hfont=(HFONT)::SendMessage(hDlg,WM_GETFONT,0,0);
				::GetObject(hfont,sizeof(LOGFONT),&lf);
				lf.lfWeight=FW_BOLD;
				pThis->m_TitleFont.Create(&lf);
			}
		}
		return TRUE;

	case WM_MEASUREITEM:
		if (wParam==IDC_OPTIONS_LIST) {
			MEASUREITEMSTRUCT *pmis=reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);

			//pmis->itemHeight=ICON_HEIGHT+LIST_MARGIN*2;
			pmis->itemHeight=ICON_HEIGHT;
			return TRUE;
		}

	case WM_DRAWITEM:
		{
			COptionDialog *pThis=GetThis(hDlg);
			LPDRAWITEMSTRUCT pdis=(LPDRAWITEMSTRUCT)lParam;

			if (wParam==IDC_OPTIONS_LIST) {
				const bool fSelected=(pdis->itemState&ODS_SELECTED)!=0;
				COLORREF crText,crOldText;
				int OldBkMode;
				RECT rc;

				/*
				::FillRect(pdis->hDC,&pdis->rcItem,
						   reinterpret_cast<HBRUSH>(fSelected?COLOR_HIGHLIGHT+1:COLOR_WINDOW+1));
				crOldText=::SetTextColor(pdis->hDC,::GetSysColor(fSelected?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
				*/
				if (fSelected) {
					rc=pdis->rcItem;
					rc.left+=LIST_MARGIN+ICON_WIDTH+ICON_TEXT_MARGIN/2;
					DrawUtil::FillGradient(pdis->hDC,&rc,
						RGB(0,0,0),pThis->m_PageList[pdis->itemData].crTitleColor);
					crText=RGB(255,255,255);
				} else {
					::FillRect(pdis->hDC,&pdis->rcItem,
							   reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
					crText=::GetSysColor(COLOR_WINDOWTEXT);
				}
				rc=pdis->rcItem;
				rc.left+=LIST_MARGIN;
				HDC hdcMem=::CreateCompatibleDC(pdis->hDC);
				if (hdcMem!=NULL) {
					HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,pThis->m_hbmIcons));
					::TransparentBlt(pdis->hDC,rc.left,rc.top,ICON_WIDTH,ICON_HEIGHT,
									 hdcMem,(int)pdis->itemData*ICON_WIDTH,0,ICON_WIDTH,ICON_HEIGHT,RGB(255,255,255));
					::SelectObject(hdcMem,hbmOld);
					::DeleteDC(hdcMem);
				}
				crOldText=::SetTextColor(pdis->hDC,crText);
				OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
				rc.left+=ICON_WIDTH+ICON_TEXT_MARGIN;
				::DrawText(pdis->hDC,pThis->m_PageList[pdis->itemData].pszTitle,-1,
						   &rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				::SetTextColor(pdis->hDC,crOldText);
				::SetBkMode(pdis->hDC,OldBkMode);
				if ((pdis->itemState&ODS_FOCUS)!=0) {
					rc=pdis->rcItem;
					rc.left+=LIST_MARGIN+ICON_WIDTH+ICON_TEXT_MARGIN/2;
					::DrawFocusRect(pdis->hDC,&rc);
				}
			} else if (wParam==IDC_OPTIONS_TITLE) {
				HFONT hfontOld;
				COLORREF crOldText;
				int OldBkMode;
				RECT rc;

				DrawUtil::FillGradient(pdis->hDC,&pdis->rcItem,
					RGB(0,0,0),pThis->m_PageList[pThis->m_CurrentPage].crTitleColor);
				hfontOld=SelectFont(pdis->hDC,pThis->m_TitleFont.GetHandle());
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
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_LIST:
			if (HIWORD(wParam)==LBN_SELCHANGE) {
				COptionDialog *pThis=GetThis(hDlg);
				int NewPage=(int)SendDlgItemMessage(hDlg,IDC_OPTIONS_LIST,LB_GETCURSEL,0,0);

				if (NewPage>=0)
					pThis->SetPage(NewPage);
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
				pThis->m_fSettingError=false;
				for (i=0;i<NUM_PAGES;i++) {
					if (pThis->m_hDlgList[i]!=NULL) {
						::SendMessage(pThis->m_hDlgList[i],WM_NOTIFY,0,(LPARAM)&nmh);
						if (pThis->m_fSettingError)
							return TRUE;
					}
				}
				::SetCursor(hcurOld);
				::EndDialog(hDlg,LOWORD(wParam));
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		{
			COptionDialog *pThis=GetThis(hDlg);

			if (pThis->m_hbmIcons!=NULL) {
				::DeleteObject(pThis->m_hbmIcons);
				pThis->m_hbmIcons=NULL;
			}
			pThis->m_hDlg=NULL;
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


bool COptionDialog::ShowDialog(HWND hwndOwner,int StartPage)
{
	if (m_hDlg!=NULL)
		return false;
	COptions::SetFrame(this);
	m_StartPage=StartPage;
	if (::DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_OPTIONS),hwndOwner,
						 DlgProc,reinterpret_cast<LPARAM>(this))!=IDOK) {
		return false;
	}
	MainWindow.Update();
	for (int i=0;i<NUM_PAGES;i++) {
		DWORD Flags=m_PageList[i].pOptions->GetUpdateFlags();

		if (Flags!=0)
			m_PageList[i].pOptions->Apply(Flags);
	}
	if ((COptions::GetGeneralUpdateFlags()&COptions::UPDATE_GENERAL_BUILDMEDIAVIEWER)!=0) {
		CUICore *pUICore=AppMain.GetUICore();

		if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
				|| pUICore->IsViewerInitializeError()) {
			bool fOldError=pUICore->IsViewerInitializeError();
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			bool fResult=pUICore->InitializeViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			// エラーで再生オフになっていた場合はオンにする
			if (fResult && fOldError && !pUICore->IsViewerEnabled())
				pUICore->EnableViewer(true);
		}
	}
	if ((COptions::GetGeneralUpdateFlags()&COptions::UPDATE_GENERAL_EVENTINFOFONT)!=0) {
		ApplyEventInfoFont();
	}
	if (NetworkRemoconOptions.GetUpdateFlags()!=0) {
		AppMain.InitializeChannel();
		if (pNetworkRemocon!=NULL)
			pNetworkRemocon->GetChannel(&GetChannelReceiver);
	}
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	AppMain.SaveSettings();
	PluginList.SendSettingsChangeEvent();
	return true;
}


void COptionDialog::OnSettingError(COptions *pOptions)
{
	for (int i=0;i<NUM_PAGES;i++) {
		if (m_PageList[i].pOptions==pOptions) {
			SetPage(i);
			m_fSettingError=true;
			break;
		}
	}
}


static COptionDialog OptionDialog;




class CMyPanelEventHandler : public CPanelFrame::CEventHandler
{
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
	CMyPanelEventHandler();
// CPanelFrame::CEventHandler
	bool OnClose();
	bool OnMoving(RECT *pRect);
	bool OnEnterSizeMove();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
	bool OnMouseWheel(WPARAM wParam,LPARAM lParam);
	void OnVisibleChange(bool fVisible);
	bool OnFloatingChange(bool fFloating);
// CMyPanelEventHandler
	bool OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect);
	bool IsAttached();
};

CMyPanelEventHandler::CMyPanelEventHandler()
	: m_SnapEdge(EDGE_NONE)
{
}

bool CMyPanelEventHandler::OnClose()
{
	MainWindow.SendCommand(CM_PANEL);
	return false;
}

bool CMyPanelEventHandler::OnMoving(RECT *pRect)
{
	if (!PanelFrame.GetFloating())
		return false;

	POINT pt;
	RECT rc;

	::GetCursorPos(&pt);
	pt.x=m_ptStartPos.x+(pt.x-m_ptDragStartCursorPos.x);
	pt.y=m_ptStartPos.y+(pt.y-m_ptDragStartCursorPos.y);
	::OffsetRect(pRect,pt.x-pRect->left,pt.y-pRect->top);
	if (PanelOptions.GetSnapAtMainWindow()) {
		// メインウィンドウにスナップさせる
		int SnapMargin=PanelOptions.GetSnapMargin();
		int XOffset,YOffset;
		bool fSnap;

		MainWindow.GetPosition(&rc);
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
		::OffsetRect(pRect,XOffset,YOffset);
	}
	return true;
}

bool CMyPanelEventHandler::OnEnterSizeMove()
{
	if (!PanelFrame.GetFloating())
		return false;

	::GetCursorPos(&m_ptDragStartCursorPos);
	int x,y;
	PanelFrame.GetPosition(&x,&y,NULL,NULL);
	m_ptStartPos.x=x;
	m_ptStartPos.y=y;
	return true;
}

bool CMyPanelEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}

bool CMyPanelEventHandler::OnMouseWheel(WPARAM wParam,LPARAM lParam)
{
	SendMessage(MainWindow.GetVideoHostWindow(),WM_MOUSEWHEEL,wParam,lParam);
	return true;
}

void CMyPanelEventHandler::OnVisibleChange(bool fVisible)
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

bool CMyPanelEventHandler::OnFloatingChange(bool fFloating)
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	int Size;
	RECT rc;

	PanelPaneIndex=pSplitter->IDToIndex(CONTAINER_ID_PANEL);
	if (fFloating)
		pSplitter->GetPane(PanelPaneIndex)->SetVisible(false);
	MainWindow.GetPosition(&rc);
	Size=PanelFrame.GetDockingWidth()+pSplitter->GetBarWidth();
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

bool CMyPanelEventHandler::OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect)
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
			::OffsetRect(&rc,XOffset,YOffset);
			PanelFrame.SetPosition(&rc);
			PanelFrame.MoveToMonitorInside();
		}
		return true;
	}
	return false;
}

bool CMyPanelEventHandler::IsAttached()
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


class CMyPanelFormEventHandler : public CPanelForm::CEventHandler
{
	void OnSelChange()
	{
		MainWindow.UpdatePanel();
	}

	void OnRButtonDown()
	{
		AppMain.GetUICore()->PopupMenu();
	}

	void OnTabRButtonDown(int x,int y)
	{
		HMENU hmenu=::CreatePopupMenu();

		int Cur=-1;
		int VisibleCount=0;
		for (int i=0;i<PanelForm.NumPages();i++) {
			CPanelForm::TabInfo TabInfo;

			PanelForm.GetTabInfo(i,&TabInfo);
			if (TabInfo.fVisible) {
				TCHAR szText[64];
				::LoadString(hInst,CM_PANEL_FIRST+TabInfo.ID,szText,lengthof(szText));
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_PANEL_FIRST+TabInfo.ID,szText);
				if (TabInfo.ID==PanelForm.GetCurPageID())
					Cur=VisibleCount;
				VisibleCount++;
			}
		}
		if (VisibleCount==0)
			return;
		if (Cur>=0)
			::CheckMenuRadioItem(hmenu,0,VisibleCount-1,Cur,MF_BYPOSITION);
		POINT pt;
		pt.x=x;
		pt.y=y;
		::ClientToScreen(PanelForm.GetHandle(),&pt);
		::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON,pt.x,pt.y,0,MainWindow.GetHandle(),NULL);
		::DestroyMenu(hmenu);
	}

	bool OnKeyDown(UINT KeyCode,UINT Flags)
	{
		MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
		return true;
	}
};


class CMyInformationPanelEventHandler : public CInformationPanel::CEventHandler {
	bool OnProgramInfoUpdate(bool fNext) {
		MainWindow.UpdateProgramInfo();
		return true;
	}
};


class CMyChannelPanelEventHandler : public CChannelPanel::CEventHandler
{
	void OnChannelClick(const CChannelInfo *pChannelInfo)
	{
		const CChannelList *pList=ChannelManager.GetCurrentChannelList();

		if (pList!=NULL) {
			if (pNetworkRemocon!=NULL) {
				MainWindow.PostCommand(CM_CHANNELNO_FIRST+pChannelInfo->GetChannelNo()-1);
			} else {
				int Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex(),
									  pChannelInfo->GetServiceID());
				if (Index<0 && pChannelInfo->GetServiceID()>0)
					Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex());
				if (Index>=0)
					MainWindow.PostCommand(CM_CHANNEL_FIRST+Index);
			}
		}
	}

	void OnRButtonDown()
	{
		CPopupMenu Menu(hInst,IDM_CHANNELPANEL);

		Menu.CheckItem(CM_CHANNELPANEL_DETAILPOPUP,ChannelPanel.GetDetailToolTip());
		Menu.CheckRadioItem(CM_CHANNELPANEL_EVENTS_1,CM_CHANNELPANEL_EVENTS_4,
							CM_CHANNELPANEL_EVENTS_1+ChannelPanel.GetEventsPerChannel()-1);
		Menu.Popup(MainWindow.GetHandle());
	}
};


class CMyEpgLoadEventHandler : public CEpgOptions::CEpgFileLoadEventHandler
							 , public CEpgOptions::CEDCBDataLoadEventHandler
{
	volatile bool m_fEpgFileLoading;

// CEpgFileLoadEventHandler
	void OnBeginLoad() {
		m_fEpgFileLoading=true;
	}

	void OnEndLoad(bool fSuccess) {
		m_fEpgFileLoading=false;
		if (fSuccess)
			MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
	}

// CEDCBDataLoadEventHandler
	void OnEnd(bool fSuccess,CEventManager *pEventManager) {
		TRACE(TEXT("EDCB data loaded\n"));
		if (fSuccess) {
			CEventManager::ServiceList ServiceList;

			if (pEventManager->GetServiceList(&ServiceList)) {
				for (size_t i=0;i<ServiceList.size();i++)
					EpgProgramList.UpdateService(pEventManager,&ServiceList[i]);
				MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
			}
		}
	}

public:
	CMyEpgLoadEventHandler() : m_fEpgFileLoading(false) {
	}

	bool IsEpgFileLoading() const {
		return m_fEpgFileLoading;
	}
};


static CMyPanelEventHandler PanelEventHandler;
static CMyPanelFormEventHandler PanelFormEventHandler;
static CMyInformationPanelEventHandler InformationPanelEventHandler;
static CMyChannelPanelEventHandler ChannelPanelEventHandler;
static CMyEpgLoadEventHandler EpgLoadEventHandler;




class CMyProgramGuideEventHandler : public CProgramGuide::CEventHandler
{
	bool OnClose()
	{
		fShowProgramGuide=false;
		MainMenu.CheckItem(CM_PROGRAMGUIDE,false);
		return true;
	}

	void OnDestroy()
	{
		m_pProgramGuide->Clear();
	}

	void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo)
	{
		if (!AppMain.SetDriver(pszDriverFileName))
			return;

		const CChannelList *pList=ChannelManager.GetCurrentChannelList();

		if (pList!=NULL) {
			int Index=pList->FindServiceID(pServiceInfo->m_ServiceID);

			if (Index>=0) {
				const CChannelInfo *pChInfo=pList->GetChannelInfo(Index);

				if (pChInfo!=NULL) {
					if (pNetworkRemocon==NULL) {
						MainWindow.PostCommand(CM_CHANNEL_FIRST+Index);
					} else {
						MainWindow.PostCommand(CM_CHANNELNO_FIRST+pChInfo->GetChannelNo()-1);
					}
				}
			}
		}
	}

	bool OnBeginUpdate()
	{
		if (CmdLineParser.m_fNoEpg) {
			MainWindow.ShowMessage(TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (CoreEngine.IsNetworkDriver()) {
			MainWindow.ShowMessage(TEXT("UDP/TCPでは番組表の取得はできません。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (RecordManager.IsRecording()) {
			MainWindow.ShowMessage(TEXT("録画を停止させてから番組表を取得してください。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		return MainWindow.BeginProgramGuideUpdate();
	}

	void OnEndUpdate()
	{
		MainWindow.OnProgramGuideUpdateEnd();
	}

	bool OnRefresh()
	{
		m_pProgramGuide->UpdateChannelList();
		return true;
	}

	bool OnKeyDown(UINT KeyCode,UINT Flags)
	{
		MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
		return true;
	}
};


class CMyProgramGuideDisplayEventHandler : public CProgramGuideDisplay::CEventHandler
{
// CProgramGuideDisplay::CEventHandler
	bool OnHide()
	{
		m_pProgramGuideDisplay->Destroy();
		fShowProgramGuide=false;
		MainMenu.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		return true;
	}

	bool SetAlwaysOnTop(bool fTop)
	{
		return AppMain.GetUICore()->SetAlwaysOnTop(fTop);
	}

	bool GetAlwaysOnTop() const {
		return AppMain.GetUICore()->GetAlwaysOnTop();
	}

	void OnRButtonDown(int x,int y)
	{
		RelayMouseMessage(WM_RBUTTONDOWN,x,y);
	}

	void OnLButtonDoubleClick(int x,int y)
	{
		RelayMouseMessage(WM_LBUTTONDBLCLK,x,y);
	}

// CMyProgramGuideDisplayEventHandler
	void RelayMouseMessage(UINT Message,int x,int y)
	{
		HWND hwndParent=m_pProgramGuideDisplay->GetParent();
		POINT pt={x,y};
		::MapWindowPoints(m_pProgramGuideDisplay->GetHandle(),hwndParent,&pt,1);
		::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
	}
};


static CMyProgramGuideEventHandler ProgramGuideEventHandler;
static CMyProgramGuideDisplayEventHandler ProgramGuideDisplayEventHandler;


class CStreamInfoEventHandler : public CStreamInfo::CEventHandler
{
	bool OnClose()
	{
		MainMenu.CheckItem(CM_STREAMINFO,false);
		return true;
	}
};


static CStreamInfoEventHandler StreamInfoEventHandler;




class CBarLayout
{
public:
	CBarLayout() {}
	virtual ~CBarLayout() {}
	virtual void Layout(RECT *pArea,RECT *pBarRect)=0;

	bool IsSpot(const RECT *pArea,const POINT *pPos)
	{
		RECT rcArea=*pArea,rcBar;

		Layout(&rcArea,&rcBar);
		return ::PtInRect(&rcBar,*pPos)!=FALSE;
	}

	void AdjustArea(RECT *pArea)
	{
		RECT rcBar;
		Layout(pArea,&rcBar);
	}

	void ReserveArea(RECT *pArea,bool fNoMove)
	{
		RECT rc;

		rc=*pArea;
		AdjustArea(&rc);
		if (fNoMove) {
			pArea->right+=(pArea->right-pArea->left)-(rc.right-rc.left);
			pArea->bottom+=(pArea->bottom-pArea->top)-(rc.bottom-rc.top);
		} else {
			pArea->left-=rc.left-pArea->left;
			pArea->top-=rc.top-pArea->top;
			pArea->right+=pArea->right-rc.right;
			pArea->bottom+=pArea->bottom-rc.bottom;
		}
	}
};


class CMyStatusViewEventHandler : public CStatusView::CEventHandler
{
	void OnMouseLeave()
	{
		if (!AppMain.GetUICore()->GetFullscreen()
				&& !MainWindow.GetStatusBarVisible())
			m_pStatusView->SetVisible(false);
	}

	void OnHeightChanged(int Height)
	{
		Layout::CWindowContainer *pContainer=
			dynamic_cast<Layout::CWindowContainer*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_STATUS));

		if (pContainer!=NULL)
			pContainer->SetMinSize(0,Height);
	}
};


class CTitleBarUtil : public CTitleBar::CEventHandler, public CBarLayout
{
	bool m_fMainWindow;
	bool m_fFixed;
	void ShowSystemMenu(int x,int y);

public:
	CTitleBarUtil(bool fMainWindow);
// CTitleBar::CEventHandler
	bool OnClose();
	bool OnMinimize();
	bool OnMaximize();
	bool OnFullscreen();
	void OnMouseLeave();
	void OnLabelLButtonDown(int x,int y);
	void OnLabelLButtonDoubleClick(int x,int y);
	void OnLabelRButtonDown(int x,int y);
	void OnIconLButtonDown(int x,int y);
	void OnIconLButtonDoubleClick(int x,int y);
// CBarLayout
	void Layout(RECT *pArea,RECT *pBarRect);
// CTitleBarUtil
	void EndDrag();
};

CTitleBarUtil::CTitleBarUtil(bool fMainWindow)
	: m_fMainWindow(fMainWindow)
	, m_fFixed(false)
{
}

bool CTitleBarUtil::OnClose()
{
	MainWindow.PostCommand(CM_CLOSE);
	return true;
}

bool CTitleBarUtil::OnMinimize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,SC_MINIMIZE,0);
	return true;
}

bool CTitleBarUtil::OnMaximize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,
						   MainWindow.GetMaximize()?SC_RESTORE:SC_MAXIMIZE,0);
	return true;
}

bool CTitleBarUtil::OnFullscreen()
{
	AppMain.GetUICore()->ToggleFullscreen();
	return true;
}

void CTitleBarUtil::OnMouseLeave()
{
	if (m_fMainWindow && !m_fFixed && !MainWindow.GetTitleBarVisible())
		m_pTitleBar->SetVisible(false);
}

void CTitleBarUtil::OnLabelLButtonDown(int x,int y)
{
	if (m_fMainWindow) {
		POINT pt;

		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
		MainWindow.SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
		m_fFixed=true;
	}
}

void CTitleBarUtil::OnLabelLButtonDoubleClick(int x,int y)
{
	if (m_fMainWindow)
		OnMaximize();
	else
		OnFullscreen();
}

void CTitleBarUtil::OnLabelRButtonDown(int x,int y)
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
	ShowSystemMenu(pt.x,pt.y);
}

void CTitleBarUtil::OnIconLButtonDown(int x,int y)
{
	RECT rc;

	m_pTitleBar->GetScreenPosition(&rc);
	ShowSystemMenu(rc.left,rc.bottom);
}

void CTitleBarUtil::OnIconLButtonDoubleClick(int x,int y)
{
	MainWindow.PostCommand(CM_CLOSE);
}

void CTitleBarUtil::Layout(RECT *pArea,RECT *pBarRect)
{
	pBarRect->left=pArea->left;
	pBarRect->top=pArea->top;
	pBarRect->right=pArea->right;
	pBarRect->bottom=pArea->top+m_pTitleBar->GetHeight();
	pArea->top+=m_pTitleBar->GetHeight();
}

void CTitleBarUtil::EndDrag()
{
	m_fFixed=false;
}

void CTitleBarUtil::ShowSystemMenu(int x,int y)
{
	m_fFixed=true;
	MainWindow.SendMessage(0x0313,0,MAKELPARAM(x,y));
	m_fFixed=false;

	RECT rc;
	POINT pt;
	m_pTitleBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc,pt))
		OnMouseLeave();
}


class CSideBarUtil : public CSideBar::CEventHandler, public CBarLayout
{
	bool m_fFixed;

public:
	CSideBarUtil()
		: m_fFixed(false)
	{
	}

// CSideBar::CEventHandler
	void OnCommand(int Command)
	{
		MainWindow.SendCommand(Command);
	}

	void OnRButtonDown(int x,int y)
	{
		CPopupMenu Menu(hInst,IDM_SIDEBAR);
		POINT pt;

		Menu.CheckItem(CM_SIDEBAR,MainWindow.GetSideBarVisible());
		Menu.EnableItem(CM_SIDEBAR,!AppMain.GetUICore()->GetFullscreen());
		Menu.CheckRadioItem(CM_SIDEBAR_PLACE_FIRST,CM_SIDEBAR_PLACE_LAST,
							CM_SIDEBAR_PLACE_FIRST+(int)SideBarOptions.GetPlace());
		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pSideBar->GetHandle(),&pt);
		m_fFixed=true;
		Menu.Popup(MainWindow.GetHandle(),&pt);
		m_fFixed=false;

		RECT rc;
		m_pSideBar->GetScreenPosition(&rc);
		::GetCursorPos(&pt);
		if (!::PtInRect(&rc,pt))
			OnMouseLeave();
	}

	void OnMouseLeave()
	{
		if (!m_fFixed && !AppMain.GetUICore()->GetFullscreen()
				&& !MainWindow.GetSideBarVisible())
			m_pSideBar->SetVisible(false);
	}

// CBarLayout
	void Layout(RECT *pArea,RECT *pBarRect)
	{
		const int BarWidth=SideBar.GetBarWidth();

		switch (SideBarOptions.GetPlace()) {
		case CSideBarOptions::PLACE_LEFT:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->top;
			pBarRect->right=pBarRect->left+BarWidth;
			pBarRect->bottom=pArea->bottom;
			pArea->left+=BarWidth;
			break;
		case CSideBarOptions::PLACE_RIGHT:
			pBarRect->left=pArea->right-BarWidth;
			pBarRect->top=pArea->top;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->bottom;
			pArea->right-=BarWidth;
			break;
		case CSideBarOptions::PLACE_TOP:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->top;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->top+BarWidth;
			pArea->top+=BarWidth;
			break;
		case CSideBarOptions::PLACE_BOTTOM:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->bottom-BarWidth;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->bottom;
			pArea->bottom-=BarWidth;
			break;
		}
	}
};


static CMyStatusViewEventHandler StatusViewEventHandler;
static CTitleBarUtil TitleBarUtil(true);
static CTitleBarUtil FullscreenTitleBarUtil(false);
static CSideBarUtil SideBarUtil;


class CMyVideoContainerEventHandler : public CVideoContainerWindow::CEventHandler
{
	void OnSizeChanged(int Width,int Height)
	{
		if (NotificationBar.GetVisible()) {
			RECT rc,rcView;

			NotificationBar.GetPosition(&rc);
			::GetClientRect(NotificationBar.GetParent(),&rcView);
			rc.left=rcView.left;
			rc.right=rcView.right;
			NotificationBar.SetPosition(&rc);
		}
		OSDManager.HideVolumeOSD();
	}
};

class CMyViewWindowEventHandler : public CViewWindow::CEventHandler
{
	CTitleBar *m_pTitleBar;
	CStatusView *m_pStatusView;
	CSideBar *m_pSideBar;

	void OnSizeChanged(int Width,int Height)
	{
		// 一時的に表示されているバーのサイズを合わせる
		RECT rcView,rc;

		m_pView->GetPosition(&rcView);
		if (!MainWindow.GetTitleBarVisible() && m_pTitleBar->GetVisible()) {
			TitleBarUtil.Layout(&rcView,&rc);
			m_pTitleBar->SetPosition(&rc);
		}
		if (!MainWindow.GetStatusBarVisible() && m_pStatusView->GetVisible()
				&& m_pStatusView->GetParent()==m_pView->GetParent()) {
			rc=rcView;
			rc.top=rc.bottom-m_pStatusView->GetHeight();
			rcView.bottom-=m_pStatusView->GetHeight();
			m_pStatusView->SetPosition(&rc);
		}
		if (!MainWindow.GetSideBarVisible() && m_pSideBar->GetVisible()) {
			SideBarUtil.Layout(&rcView,&rc);
			m_pSideBar->SetPosition(&rc);
		}
	}

public:
	void Initialize(CTitleBar *pTitleBar,CStatusView *pStatusView,CSideBar *pSideBar)
	{
		m_pTitleBar=pTitleBar;
		m_pStatusView=pStatusView;
		m_pSideBar=pSideBar;
	}
};

static CMyVideoContainerEventHandler VideoContainerEventHandler;
static CMyViewWindowEventHandler ViewWindowEventHandler;


class CMyChannelDisplayMenuEventHandler : public CChannelDisplayMenu::CEventHandler
{
	void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace)
	{
		if (CoreEngine.IsDriverOpen()
				&& ::lstrcmpi(CoreEngine.GetDriverFileName(),pszDriverFileName)==0) {
			MainWindow.SendCommand(CM_CHANNELDISPLAYMENU);
		} else {
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(MainWindow.GetVideoHostWindow()))
					return;
			}
			if (AppMain.SetDriver(pszDriverFileName)) {
				if (TuningSpace!=SPACE_NOTSPECIFIED) {
					MainWindow.SendCommand(CM_SPACE_FIRST+TuningSpace);
					if (TuningSpace==SPACE_ALL
							|| TuningSpace==RestoreChannelInfo.Space)
						AppMain.RestoreChannel();
				} else {
					AppMain.RestoreChannel();
				}
			}
			MainWindow.SendCommand(CM_CHANNELDISPLAYMENU);
		}
	}

	void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
	{
		if (RecordManager.IsRecording()) {
			if (!RecordOptions.ConfirmChannelChange(MainWindow.GetVideoHostWindow()))
				return;
		}
		if (AppMain.SetDriver(pszDriverFileName)) {
			int Space;
			if (RestoreChannelInfo.fAllChannels)
				Space=CChannelManager::SPACE_ALL;
			else
				Space=pChannelInfo->GetSpace();
			const CChannelList *pList=ChannelManager.GetChannelList(Space);
			if (pList!=NULL) {
				int Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex(),
									  pChannelInfo->GetServiceID());

				if (Index<0 && Space==CChannelManager::SPACE_ALL) {
					Space=pChannelInfo->GetSpace();
					pList=ChannelManager.GetChannelList(Space);
					if (pList!=NULL)
						Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),
										  pChannelInfo->GetServiceID());
				}
				if (Index>=0)
					AppMain.SetChannel(Space,Index);
			}
			MainWindow.SendCommand(CM_CHANNELDISPLAYMENU);
		}
	}

	void OnClose()
	{
		ChannelDisplayMenu.SetVisible(false);
	}

	void OnRButtonDown(int x,int y)
	{
		RelayMouseMessage(WM_RBUTTONDOWN,x,y);
	}

	void OnLButtonDoubleClick(int x,int y)
	{
		RelayMouseMessage(WM_LBUTTONDBLCLK,x,y);
	}

	void RelayMouseMessage(UINT Message,int x,int y)
	{
		HWND hwndParent=m_pChannelDisplayMenu->GetParent();
		POINT pt={x,y};
		::MapWindowPoints(m_pChannelDisplayMenu->GetHandle(),hwndParent,&pt,1);
		::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
	}
};

static CMyChannelDisplayMenuEventHandler ChannelDisplayMenuEventHandler;




CBasicViewer::CBasicViewer(CDtvEngine *pDtvEngine)
	: m_pDtvEngine(pDtvEngine)
	, m_fEnabled(false)
{
}


bool CBasicViewer::Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage)
{
	m_ViewWindow.Create(hwndParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ViewID);
	m_ViewWindow.SetMessageWindow(hwndMessage);
	const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
	Theme::BorderInfo Border;
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
	if (!ViewOptions.GetClientEdge())
		Border.Type=Theme::BORDER_NONE;
	m_ViewWindow.SetBorder(&Border);
	m_VideoContainer.Create(m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ContainerID,m_pDtvEngine);
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);

	m_DisplayBase.SetParent(&m_VideoContainer);
	m_VideoContainer.SetDisplayBase(&m_DisplayBase);

	return true;
}


bool CBasicViewer::EnableViewer(bool fEnable)
{
	if (m_fEnabled!=fEnable) {
		if (fEnable && !m_pDtvEngine->m_MediaViewer.IsOpen()) {
			bool fOK=BuildViewer();
			if (!fOK)
				return false;
		}
		if (fEnable || (!fEnable && !m_DisplayBase.IsVisible()))
			m_VideoContainer.SetVisible(fEnable);
		m_pDtvEngine->m_MediaViewer.SetVisible(fEnable);
		if (!CoreEngine.EnablePreview(fEnable))
			return false;
		if (PlaybackOptions.GetMinTimerResolution())
			CoreEngine.SetMinTimerResolution(fEnable);
		m_fEnabled=fEnable;
		PluginList.SendPreviewChangeEvent(fEnable);
	}
	return true;
}


bool CBasicViewer::BuildViewer()
{
	if (m_fEnabled)
		EnableViewer(false);

	m_pDtvEngine->m_MediaViewer.SetAudioFilter(PlaybackOptions.GetAudioFilterName());
	bool fOK=CoreEngine.BuildMediaViewer(m_VideoContainer.GetHandle(),
										 m_VideoContainer.GetHandle(),
										 GeneralOptions.GetVideoRendererType(),
										 GeneralOptions.GetMpeg2DecoderName(),
										 PlaybackOptions.GetAudioDeviceName());
	if (!fOK) {
		AppMain.OnError(&CoreEngine,TEXT("DirectShowの初期化ができません。"));
	}

	return fOK;
}


bool CBasicViewer::CloseViewer()
{
	EnableViewer(false);
	CoreEngine.CloseMediaViewer();
	return true;
}




bool CFullscreen::Initialize()
{
	WNDCLASS wc;

	wc.style=CS_DBLCLKS;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
	wc.lpszMenuName=NULL;
	wc.lpszClassName=FULLSCREEN_WINDOW_CLASS;
	return RegisterClass(&wc)!=0;
}


CFullscreen::CFullscreen()
	: m_pViewer(NULL)
	, m_PanelWidth(-1)
{
}


CFullscreen::~CFullscreen()
{
	Destroy();
}


LRESULT CALLBACK CFullscreen::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg==WM_CREATE) {
		CFullscreen *pThis=static_cast<CFullscreen*>(CBasicWindow::OnCreate(hwnd,lParam));

		return pThis->OnCreate()?0:-1;
	}

	CFullscreen *pThis=static_cast<CFullscreen*>(GetBasicWindow(hwnd));
	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
}


LRESULT CFullscreen::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		m_LayoutBase.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		OnRButtonDown();
		return 0;

	case WM_MBUTTONDOWN:
		OnMButtonDown();
		return 0;

	case WM_LBUTTONDBLCLK:
		OnLButtonDoubleClick();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove();
		return 0;

	case WM_TIMER:
		if (!m_fMenu) {
			POINT pt;
			RECT rc;
			::GetCursorPos(&pt);
			m_ViewWindow.GetPosition(&rc);
			if (::PtInRect(&rc,pt)) {
				::SetCursor(NULL);
				ShowCursor(false);
			}
		}
		::KillTimer(hwnd,1);
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			HWND hwndCursor=reinterpret_cast<HWND>(wParam);

			if (hwndCursor==m_pViewer->GetVideoContainer().GetHandle()
					|| hwndCursor==m_ViewWindow.GetHandle()
					|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
				::SetCursor(m_fShowCursor?::LoadCursor(NULL,IDC_ARROW):NULL);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			MainWindow.OnMouseWheel(wParam,lParam,fHorz);
			return fHorz;
		}

#if 0	// これはやらない方がいい気がする
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp=reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter=HWND_TOPMOST;
		}
		return 0;
#endif

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			AppMain.GetUICore()->SetFullscreen(false);
			return 0;
		}
	case WM_COMMAND:
		return MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam&0xFFFFFFF0) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;
		}
		break;

	case WM_APPCOMMAND:
		{
			int Command=Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				MainWindow.SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_SETFOCUS:
		if (m_pViewer->GetDisplayBase().IsVisible())
			m_pViewer->GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		m_TitleBar.SetLabel(reinterpret_cast<LPCTSTR>(lParam));
		break;

	case WM_DESTROY:
		{
			SIZE sz;

			m_pViewer->GetViewWindow().GetClientSize(&sz);
			m_pViewer->GetVideoContainer().SetParent(&m_pViewer->GetViewWindow());
			m_pViewer->GetViewWindow().SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			ShowCursor(true);
			m_pViewer->GetDisplayBase().AdjustPosition();
			m_TitleBar.Destroy();
			OSDManager.Reset();
			ShowStatusView(false);
			ShowSideBar(false);
			ShowPanel(false);
			OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CFullscreen::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 FULLSCREEN_WINDOW_CLASS,NULL,hInst);
}


bool CFullscreen::Create(HWND hwndOwner,CBasicViewer *pViewer)
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
#ifdef _DEBUG
	// デバッグし易いように小さく表示
	/*
	Width/=2;
	Height/=2;
	*/
#endif
	SetPosition(x,y,Width,Height);
	m_pViewer=pViewer;
	return Create(hwndOwner,WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


bool CFullscreen::OnCreate()
{
	m_LayoutBase.Create(m_hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_ViewWindow.Create(m_LayoutBase.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,0,IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_pViewer->GetVideoContainer().SetParent(m_ViewWindow.GetHandle());
	m_ViewWindow.SetVideoContainer(&m_pViewer->GetVideoContainer());

	m_Panel.Create(m_LayoutBase.GetHandle(),
				   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Panel.ShowTitle(true);
	m_Panel.EnableFloating(false);
	m_Panel.SetEventHandler(&m_PanelEventHandler);

	Layout::CSplitter *pSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pSplitter->SetVisible(true);
	Layout::CWindowContainer *pViewContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pViewContainer->SetWindow(&m_ViewWindow);
	pViewContainer->SetMinSize(32,32);
	pViewContainer->SetVisible(true);
	pSplitter->SetPane(0,pViewContainer);
	Layout::CWindowContainer *pPanelContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetWindow(&m_Panel);
	pPanelContainer->SetMinSize(64,32);
	pSplitter->SetPane(1,pPanelContainer);
	pSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	m_LayoutBase.SetTopContainer(pSplitter);

	RECT rc;
	m_pViewer->GetDisplayBase().GetParent()->GetClientRect(&rc);
	m_pViewer->GetDisplayBase().SetPosition(&rc);

	m_TitleBar.Create(m_ViewWindow.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS,0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&FullscreenTitleBarUtil);
	m_TitleBar.SetIcon(::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));

	OSDManager.Reset();

	CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
									ViewOptions.GetFullscreenStretchMode());

	m_fShowCursor=true;
	m_fMenu=false;
	m_fShowStatusView=false;
	m_fShowTitleBar=false;
	m_fShowSideBar=false;
	m_fShowPanel=false;
	m_LastCursorMovePos.x=-4;
	m_LastCursorMovePos.y=-4;
	::SetTimer(m_hwnd,1,1000,NULL);

	return true;
}


void CFullscreen::ShowCursor(bool fShow)
{
	CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(!fShow);
	m_ViewWindow.ShowCursor(fShow);
	m_fShowCursor=fShow;
}


void CFullscreen::ShowPanel(bool fShow)
{
	if (m_fShowPanel!=fShow) {
		Layout::CSplitter *pSplitter=
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

		if (fShow) {
			if (m_Panel.GetWindow()==NULL) {
				if (m_PanelWidth<0)
					m_PanelWidth=PanelFrame.GetDockingWidth();
				PanelFrame.SetPanelVisible(false);
				PanelFrame.GetPanel()->SetWindow(NULL,NULL);
				m_Panel.SetWindow(&PanelForm,TEXT("パネル"));
				pSplitter->SetPaneSize(CONTAINER_ID_PANEL,m_PanelWidth);
			}
			m_Panel.SendSizeMessage();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,true);
			MainWindow.UpdatePanel();
		} else {
			m_PanelWidth=m_Panel.GetWidth();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,false);
			m_Panel.SetWindow(NULL,NULL);
			CPanel *pPanel=PanelFrame.GetPanel();
			pPanel->SetWindow(&PanelForm,TEXT("パネル"));
			pPanel->SendSizeMessage();
			if (fShowPanelWindow) {
				PanelFrame.SetPanelVisible(true);
			}
		}
		m_fShowPanel=fShow;
	}
}


void CFullscreen::OnMouseCommand(int Command)
{
	if (Command==0)
		return;
	// メニュー表示中はカーソルを消さない
	KillTimer(m_hwnd,1);
	ShowCursor(true);
	::SetCursor(LoadCursor(NULL,IDC_ARROW));
	m_fMenu=true;
	MainWindow.SendMessage(WM_COMMAND,MAKEWPARAM(Command,CMainWindow::COMMAND_FROM_MOUSE),0);
	m_fMenu=false;
	if (m_hwnd!=NULL)
		::SetTimer(m_hwnd,1,1000,NULL);
}


void CFullscreen::OnRButtonDown()
{
	OnMouseCommand(OperationOptions.GetRightClickCommand());
}


void CFullscreen::OnMButtonDown()
{
	OnMouseCommand(OperationOptions.GetMiddleClickCommand());
}


void CFullscreen::OnLButtonDoubleClick()
{
	OnMouseCommand(OperationOptions.GetLeftDoubleClickCommand());
}


void CFullscreen::OnMouseMove()
{
	POINT pt;
	RECT rcClient,rc;

	if (m_fMenu)
		return;
	GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	m_ViewWindow.GetClientRect(&rcClient);
	rc=rcClient;
	rc.top=rc.bottom-StatusView.GetHeight();
	if (::PtInRect(&rc,pt)) {
		if (!m_fShowStatusView)
			ShowStatusView(true);
		::KillTimer(m_hwnd,1);
		return;
	} else if (m_fShowStatusView)
		ShowStatusView(false);
	if (FullscreenTitleBarUtil.IsSpot(&rcClient,&pt)) {
		if (!m_fShowTitleBar)
			ShowTitleBar(true);
		::KillTimer(m_hwnd,1);
		return;
	} else if (m_fShowTitleBar)
		ShowTitleBar(false);
	if (SideBarOptions.ShowPopup()) {
		if (SideBarUtil.IsSpot(&rcClient,&pt)) {
			if (!m_fShowSideBar)
				ShowSideBar(true);
			::KillTimer(m_hwnd,1);
			return;
		} else if (m_fShowSideBar)
			ShowSideBar(false);
	}
	if (abs(m_LastCursorMovePos.x-pt.x)>=4 || abs(m_LastCursorMovePos.y-pt.y)>=4) {
		m_LastCursorMovePos=pt;
		if (!m_fShowCursor) {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			ShowCursor(true);
		}
	}
	::SetTimer(m_hwnd,1,1000,NULL);
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;

	Layout::CLayoutBase &LayoutBase=MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rc;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,false);
		StatusView.SetParent(&m_ViewWindow);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
		::BringWindowToTop(StatusView.GetHandle());
	} else {
		StatusView.SetVisible(false);
		StatusView.SetParent(&LayoutBase);
		if (MainWindow.GetStatusBarVisible()) {
			/*
			LayoutBase.Adjust();
			StatusView.SetVisible(true);
			*/
			LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,true);
		}
	}

	m_fShowStatusView=fShow;
}


void CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow==m_fShowTitleBar)
		return;

	if (fShow) {
		RECT rc,rcBar;
		const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
		Theme::GradientInfo Gradient1,Gradient2;
		Theme::BorderInfo Border;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		FullscreenTitleBarUtil.Layout(&rc,&rcBar);
		m_TitleBar.SetPosition(&rcBar);
		m_TitleBar.SetLabel(MainWindow.GetTitleBar().GetLabel());
		CTitleBar::ThemeInfo TitleBarTheme;
		MainWindow.GetTitleBar().GetTheme(&TitleBarTheme);
		m_TitleBar.SetTheme(&TitleBarTheme);
		m_TitleBar.SetVisible(true);
		::BringWindowToTop(m_TitleBar.GetHandle());
	} else {
		m_TitleBar.SetVisible(false);
	}

	m_fShowTitleBar=fShow;
}


void CFullscreen::ShowSideBar(bool fShow)
{
	if (fShow==m_fShowSideBar)
		return;

	Layout::CLayoutBase &LayoutBase=MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rcClient,rcBar;

		ShowStatusView(false);
		ShowTitleBar(false);
		m_ViewWindow.GetClientRect(&rcClient);
		SideBarUtil.Layout(&rcClient,&rcBar);
		SideBar.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,false);
		SideBar.SetParent(&m_ViewWindow);
		SideBar.SetPosition(&rcBar);
		SideBar.SetVisible(true);
		::BringWindowToTop(SideBar.GetHandle());
	} else {
		SideBar.SetVisible(false);
		SideBar.SetParent(&LayoutBase);
		if (MainWindow.GetSideBarVisible()) {
			/*
			LayoutBase.Adjust();
			SideBar.SetVisible(true);
			*/
			LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
		}
	}

	m_fShowSideBar=fShow;
}


bool CFullscreen::CPanelEventHandler::OnClose()
{
	MainWindow.SendCommand(CM_PANEL);
	return true;
}




class CServiceUpdateInfo {
public:
	struct ServiceInfo {
		WORD ServiceID;
		TCHAR szServiceName[256];
		WORD LogoID;
	};
	ServiceInfo *m_pServiceList;
	int m_NumServices;
	int m_CurService;
	WORD m_NetworkID;
	WORD m_TransportStreamID;
	bool m_fStreamChanged;
	bool m_fServiceListEmpty;
	CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer);
	~CServiceUpdateInfo();
};

CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer)
{
	CTsAnalyzer::ServiceList ServiceList;

	pTsAnalyzer->GetViewableServiceList(&ServiceList);
	m_NumServices=(int)ServiceList.size();
	m_CurService=-1;
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
			m_pServiceList[i].ServiceID=pServiceInfo->ServiceID;
			::lstrcpy(m_pServiceList[i].szServiceName,pServiceInfo->szServiceName);
			m_pServiceList[i].LogoID=pServiceInfo->LogoID;
		}
		WORD ServiceID;
		if (pEngine->GetServiceID(&ServiceID)) {
			for (int i=0;i<m_NumServices;i++) {
				if (m_pServiceList[i].ServiceID==ServiceID) {
					m_CurService=i;
					break;
				}
			}
		}
	} else {
		m_pServiceList=NULL;
	}
	m_NetworkID=pTsAnalyzer->GetNetworkID();
	m_TransportStreamID=pTsAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty=pTsAnalyzer->GetServiceNum()==0;
}

CServiceUpdateInfo::~CServiceUpdateInfo()
{
	delete [] m_pServiceList;
}


class CMyDtvEngineHandler : public CDtvEngine::CEventHandler
{
// CEventHandler
	void OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer,bool bStreamChanged);
	void OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer);
	void OnFileWriteError(CBufferedFileWriter *pFileWriter);
	void OnVideoSizeChanged(CMediaViewer *pMediaViewer);
	void OnEmmProcessed(const BYTE *pEmmData);
	void OnEcmError(LPCTSTR pszText);
// CMyDtvEngineHandler
	void OnServiceUpdated(CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged);
};

void CMyDtvEngineHandler::OnServiceUpdated(CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged)
{
	CServiceUpdateInfo *pInfo=new CServiceUpdateInfo(m_pDtvEngine,pTsAnalyzer);

	pInfo->m_fStreamChanged=fStreamChanged;
	MainWindow.PostMessage(WM_APP_SERVICEUPDATE,fListUpdated,
						   reinterpret_cast<LPARAM>(pInfo));
}

void CMyDtvEngineHandler::OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer,bool bStreamChanged)
{
	OnServiceUpdated(pTsAnalyzer,true,bStreamChanged);
}

void CMyDtvEngineHandler::OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer)
{
	OnServiceUpdated(pTsAnalyzer,false,false);
}

void CMyDtvEngineHandler::OnFileWriteError(CBufferedFileWriter *pFileWriter)
{
	MainWindow.PostMessage(WM_APP_FILEWRITEERROR,0,0);
}

void CMyDtvEngineHandler::OnVideoSizeChanged(CMediaViewer *pMediaViewer)
{
	/*
		この通知が送られた段階ではまだレンダラの映像サイズは変わっていないため、
		後でパンスキャンの設定を行う必要がある
	*/
	MainWindow.PostMessage(WM_APP_VIDEOSIZECHANGED,0,0);
}

void CMyDtvEngineHandler::OnEmmProcessed(const BYTE *pEmmData)
{
	MainWindow.PostMessage(WM_APP_EMMPROCESSED,pEmmData!=NULL,0);
}

void CMyDtvEngineHandler::OnEcmError(LPCTSTR pszText)
{
	MainWindow.PostMessage(WM_APP_ECMERROR,0,(LPARAM)DuplicateString(pszText));
}


static CMyDtvEngineHandler DtvEngineHandler;




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




int CMainWindow::m_ThinFrameWidth=2;
bool CMainWindow::m_fThinFrameCreate=false;


bool CMainWindow::Initialize()
{
	WNDCLASS wc;

	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=MAIN_WINDOW_CLASS;
	return ::RegisterClass(&wc)!=0 && CFullscreen::Initialize();
}


CMainWindow::CMainWindow()
	: m_Viewer(&CoreEngine.m_DtvEngine)
	, m_fShowStatusBar(true)
	, m_fShowTitleBar(true)
	, m_fCustomTitleBar(true)
	, m_fShowSideBar(false)
	, m_fThinFrame(false)
	, m_fStandbyInit(false)
	, m_fMinimizeInit(false)
	, m_fSrcFilterReleased(false)
	, m_fRestorePreview(false)
	, m_fProgramGuideUpdating(false)
	, m_fExitOnRecordingStop(false)
	, m_fClosing(false)
	, m_WheelCount(0)
	, m_PrevWheelMode(COperationOptions::WHEEL_NONE)
	, m_PrevWheelTime(0)
	, m_AspectRatioType(ASPECTRATIO_DEFAULT)
	, m_AspectRatioResetTime(0)
	, m_fFrameCut(false)
	, m_ProgramListUpdateTimerCount(0)
	, m_CurEventStereoMode(-1)
	, m_fAlertedLowFreeSpace(false)
	, m_ResetErrorCountTimer(TIMER_ID_RESETERRORCOUNT)
	, m_DisplayBaseEventHandler(this)
{
	// 適当にデフォルトサイズを設定
#ifndef TVH264_FOR_1SEG
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
#else
	m_WindowPosition.Width=400;
	m_WindowPosition.Height=320;
#endif
	m_WindowPosition.Left=
		(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=
		(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
}


bool CMainWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	m_fThinFrameCreate=m_fThinFrame;
	if (m_pCore->GetAlwaysOnTop())
		ExStyle|=WS_EX_TOPMOST;
	if (!CreateBasicWindow(NULL,Style,ExStyle,ID,MAIN_WINDOW_CLASS,MAIN_TITLE_TEXT,hInst))
		return false;
	return true;
}


bool CMainWindow::Show(int CmdShow)
{
	return ::ShowWindow(m_hwnd,m_WindowPosition.fMaximized?SW_SHOWMAXIMIZED:CmdShow)!=FALSE;
}


bool CMainWindow::InitializeViewer()
{
	const bool fEnableViewer=IsViewerEnabled();

	if (m_Viewer.BuildViewer()) {
		TCHAR szText[256];

		if (CoreEngine.m_DtvEngine.GetVideoDecoderName(szText,lengthof(szText)))
			InfoPanel.SetVideoDecoderName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererName(szText,lengthof(szText)))
			InfoPanel.SetVideoRendererName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioRendererName(szText,lengthof(szText)))
			InfoPanel.SetAudioDeviceName(szText);
		if (fEnableViewer)
			m_pCore->EnableViewer(true);
	} else {
		InfoPanel.SetVideoDecoderName(NULL);
		InfoPanel.SetVideoRendererName(NULL);
		InfoPanel.SetAudioDeviceName(NULL);
		MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	}

	return true;
}


bool CMainWindow::FinalizeViewer()
{
	m_Viewer.CloseViewer();
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	return true;
}


bool CMainWindow::OnFullscreenChange(bool fFullscreen)
{
	if (fFullscreen) {
		if (::IsIconic(m_hwnd))
			::ShowWindow(m_hwnd,SW_RESTORE);
		if (!m_Fullscreen.Create(m_hwnd,&m_Viewer))
			return false;
	} else {
		ForegroundWindow(m_hwnd);
		m_Fullscreen.Destroy();
		CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
							m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
										CMediaViewer::STRETCH_KEEPASPECTRATIO);
	}
	StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
	MainMenu.CheckItem(CM_FULLSCREEN,fFullscreen);
	return true;
}


HWND CMainWindow::GetVideoHostWindow() const
{
	if (m_pCore->GetStandby())
		return NULL;
	if (m_pCore->GetFullscreen())
		return m_Fullscreen.GetHandle();
	return m_hwnd;
}


void CMainWindow::ShowNotificationBar(LPCTSTR pszText,
									  CNotificationBar::MessageType Type,DWORD Duration)
{
	NotificationBar.SetFont(OSDOptions.GetNotificationBarFont());
	NotificationBar.SetText(pszText,Type);
	NotificationBar.Show(max((DWORD)OSDOptions.GetNotificationBarDuration(),Duration));
}


void CMainWindow::AdjustWindowSize(int Width,int Height)
{
	if (IsZoomed(m_hwnd))
		return;

	RECT rcOld,rc;
	GetPosition(&rcOld);

	HMONITOR hMonitor=::MonitorFromRect(&rcOld,MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize=sizeof(mi);
	::GetMonitorInfo(hMonitor,&mi);

	::SetRect(&rc,0,0,Width,Height);
	m_Viewer.GetViewWindow().CalcWindowRect(&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;
	m_LayoutBase.GetScreenPosition(&rc);
	rc.right=rc.left+Width;
	rc.bottom=rc.top+Height;
	if (m_fShowStatusBar)
		rc.bottom+=StatusView.GetHeight();
	if (m_fShowTitleBar && m_fCustomTitleBar)
		TitleBarUtil.ReserveArea(&rc,true);
	if (m_fShowSideBar)
		SideBarUtil.ReserveArea(&rc,true);
	if (fShowPanelWindow && !PanelFrame.GetFloating()) {
		Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		rc.right+=pSplitter->GetBarWidth()+pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
	}
	if (m_fThinFrame) {
		rc.left-=m_ThinFrameWidth;
		rc.right+=m_ThinFrameWidth;
		rc.top-=m_ThinFrameWidth;
		rc.bottom+=m_ThinFrameWidth;
	} else {
		CalcPositionFromClientRect(&rc);
	}
	if (ViewOptions.GetNearCornerResizeOrigin()) {
		if (abs(rcOld.left-mi.rcWork.left)>abs(rcOld.right-mi.rcWork.right)) {
			rc.left=rcOld.right-(rc.right-rc.left);
			rc.right=rcOld.right;
		}
		if (abs(rcOld.top-mi.rcWork.top)>abs(rcOld.bottom-mi.rcWork.bottom)) {
			rc.top=rcOld.bottom-(rc.bottom-rc.top);
			rc.bottom=rcOld.bottom;
		}
	}

	// ウィンドウがモニタの外に出ないようにする
	if (rcOld.left>=mi.rcWork.left && rcOld.top>=mi.rcWork.top
			&& rcOld.right<=mi.rcWork.right && rcOld.bottom<=mi.rcWork.bottom) {
		if (rc.right>mi.rcWork.right && rc.left>mi.rcWork.left)
			::OffsetRect(&rc,max(mi.rcWork.right-rc.right,mi.rcWork.left-rc.left),0);
		if (rc.bottom>mi.rcWork.bottom && rc.top>mi.rcWork.top)
			::OffsetRect(&rc,0,max(mi.rcWork.bottom-rc.bottom,mi.rcWork.top-rc.top));
	}

	SetPosition(&rc);
	PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,&rc);
}


bool CMainWindow::ReadSettings(CSettings *pSettings)
{
	int Left,Top,Width,Height;
	bool f;

	GetPosition(&Left,&Top,&Width,&Height);
	pSettings->Read(TEXT("WindowLeft"),&Left);
	pSettings->Read(TEXT("WindowTop"),&Top);
	pSettings->Read(TEXT("WindowWidth"),&Width);
	pSettings->Read(TEXT("WindowHeight"),&Height);
	SetPosition(Left,Top,Width,Height);
	MoveToMonitorInside();
	if (pSettings->Read(TEXT("WindowMaximize"),&f))
		SetMaximize(f);
	if (pSettings->Read(TEXT("AlwaysOnTop"),&f))
		m_pCore->SetAlwaysOnTop(f);
	if (pSettings->Read(TEXT("ShowStatusBar"),&f))
		SetStatusBarVisible(f);
	if (pSettings->Read(TEXT("ShowTitleBar"),&f))
		SetTitleBarVisible(f);
	if (pSettings->Read(TEXT("ThinFrame"),&f))
		SetThinFrame(f);
	if (!m_fThinFrame && pSettings->Read(TEXT("CustomTitleBar"),&f))
		SetCustomTitleBar(f);
	if (pSettings->Read(TEXT("ShowSideBar"),&f))
		SetSideBarVisible(f);
	pSettings->Read(TEXT("FrameCut"),&m_fFrameCut);
	return true;
}


bool CMainWindow::WriteSettings(CSettings *pSettings)
{
	int Left,Top,Width,Height;

	GetPosition(&Left,&Top,&Width,&Height);
	pSettings->Write(TEXT("WindowLeft"),Left);
	pSettings->Write(TEXT("WindowTop"),Top);
	pSettings->Write(TEXT("WindowWidth"),Width);
	pSettings->Write(TEXT("WindowHeight"),Height);
	pSettings->Write(TEXT("WindowMaximize"),m_WindowPosition.fMaximized);
	pSettings->Write(TEXT("AlwaysOnTop"),m_pCore->GetAlwaysOnTop());
	pSettings->Write(TEXT("ShowStatusBar"),m_fShowStatusBar);
	pSettings->Write(TEXT("ShowTitleBar"),m_fShowTitleBar);
	pSettings->Write(TEXT("ThinFrame"),m_fThinFrame);
	pSettings->Write(TEXT("CustomTitleBar"),m_fCustomTitleBar);
	pSettings->Write(TEXT("ShowSideBar"),m_fShowSideBar);
	pSettings->Write(TEXT("FrameCut"),m_fFrameCut);
	return true;
}


bool CMainWindow::SetAlwaysOnTop(bool fTop)
{
	if (m_hwnd!=NULL) {
		::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,
					   SWP_NOMOVE | SWP_NOSIZE);
		MainMenu.CheckItem(CM_ALWAYSONTOP,fTop);
	}
	return true;
}


void CMainWindow::SetStatusBarVisible(bool fVisible)
{
	if (m_fShowStatusBar!=fVisible) {
		if (!m_pCore->GetFullscreen()) {
			m_fShowStatusBar=fVisible;
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,fVisible);
			if (!GetMaximize()) {
				RECT rc;

				GetPosition(&rc);
				if (fVisible)
					rc.bottom+=StatusView.GetHeight();
				else
					rc.bottom-=StatusView.GetHeight();
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
			if (!m_fCustomTitleBar)
				SetStyle(GetStyle()^WS_CAPTION,fMaximize);
			else if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
			if (!fMaximize) {
				int CaptionHeight;

				if (!m_fCustomTitleBar)
					CaptionHeight=::GetSystemMetrics(SM_CYCAPTION);
				else
					CaptionHeight=m_TitleBar.GetHeight();
				if (fVisible)
					rc.top-=CaptionHeight;
				else
					rc.top+=CaptionHeight;
				::SetWindowPos(m_hwnd,NULL,rc.left,rc.top,
							   rc.right-rc.left,rc.bottom-rc.top,
							   SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			}
			if (m_fCustomTitleBar && fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);
			MainMenu.CheckItem(CM_TITLEBAR,fVisible);
		}
	}
}


void CMainWindow::SetCustomTitleBar(bool fCustom)
{
	if (m_fCustomTitleBar!=fCustom) {
		if (!fCustom && m_fThinFrame)
			SetThinFrame(false);
		m_fCustomTitleBar=fCustom;
		if (m_hwnd!=NULL) {
			if (m_fShowTitleBar) {
				if (!fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
				SetStyle(GetStyle()^WS_CAPTION,true);
				if (fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);
			}
			MainMenu.CheckItem(CM_CUSTOMTITLEBAR,fCustom);
		}
	}
}


void CMainWindow::SetThinFrame(bool fThinFrame)
{
	if (m_fThinFrame!=fThinFrame) {
		if (fThinFrame && !m_fCustomTitleBar)
			SetCustomTitleBar(true);
		m_fThinFrame=fThinFrame;
		if (m_hwnd!=NULL) {
			::SetWindowPos(m_hwnd,NULL,0,0,0,0,
						   SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
			CAeroGlass Aero;
			Aero.EnableNcRendering(m_hwnd,!fThinFrame);
			MainMenu.CheckItem(CM_THINFRAME,fThinFrame);
			MainMenu.EnableItem(CM_CUSTOMTITLEBAR,!fThinFrame);
		}
	}
}


void CMainWindow::SetSideBarVisible(bool fVisible)
{
	if (m_fShowSideBar!=fVisible) {
		m_fShowSideBar=fVisible;
		if (m_hwnd!=NULL) {
			RECT rc;

			if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,false);
			if (!GetMaximize()) {
				GetPosition(&rc);
				RECT rcArea=rc;
				if (fVisible)
					SideBarUtil.ReserveArea(&rcArea,true);
				else
					SideBarUtil.AdjustArea(&rcArea);
				rc.right=rc.left+(rcArea.right-rcArea.left);
				rc.bottom=rc.top+(rcArea.bottom-rcArea.top);
				SetPosition(&rc);
			}
			if (fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
			MainMenu.CheckItem(CM_SIDEBAR,fVisible);
		}
	}
}


LRESULT CMainWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	HANDLE_MSG(hwnd,WM_COMMAND,OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,OnTimer);

	case WM_SIZE:
		OnSizeChanged((UINT)wParam,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_SIZING:
		if (OnSizeChanging((UINT)wParam,reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pmmi=reinterpret_cast<LPMINMAXINFO>(lParam);
			SIZE sz;
			RECT rc;

			m_LayoutBase.GetMinSize(&sz);
			::SetRect(&rc,0,0,sz.cx,sz.cy);
			CalcPositionFromClientRect(&rc);
			pmmi->ptMinTrackSize.x=rc.right-rc.left;
			pmmi->ptMinTrackSize.y=rc.bottom-rc.top;
		}
		return 0;

	case WM_MOVE:
		OSDManager.OnParentMove();
		return 0;

	case WM_RBUTTONDOWN:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnRButtonDown();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(OperationOptions.GetRightClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_MBUTTONDOWN:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnMButtonDown();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(OperationOptions.GetMiddleClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam!=HTCAPTION)
			break;
		ForegroundWindow(hwnd);
	case WM_LBUTTONDOWN:
		if (uMsg==WM_NCLBUTTONDOWN || OperationOptions.GetDisplayDragMove()) {
			/*
			m_ptDragStartPos.x=GET_X_LPARAM(lParam);
			m_ptDragStartPos.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&m_ptDragStartPos);
			*/
			::GetCursorPos(&m_ptDragStartPos);
			::GetWindowRect(hwnd,&m_rcDragStart);
			::SetCapture(hwnd);
		}
		return 0;

	case WM_NCLBUTTONUP:
	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd)
			::ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		TitleBarUtil.EndDrag();
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			POINT pt;
			RECT rcOld,rc;

			/*
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&pt);
			*/
			::GetWindowRect(hwnd,&rcOld);
			::GetCursorPos(&pt);
			rc.left=m_rcDragStart.left+(pt.x-m_ptDragStartPos.x);
			rc.top=m_rcDragStart.top+(pt.y-m_ptDragStartPos.y);
			rc.right=rc.left+(m_rcDragStart.right-m_rcDragStart.left);
			rc.bottom=rc.top+(m_rcDragStart.bottom-m_rcDragStart.top);
			bool fSnap=ViewOptions.GetSnapAtWindowEdge();
			if (::GetKeyState(VK_SHIFT)<0)
				fSnap=!fSnap;
			if (fSnap)
				SnapWindow(hwnd,&rc,
						   ViewOptions.GetSnapAtWindowEdgeMargin(),
						   PanelEventHandler.IsAttached()?NULL:PanelFrame.GetHandle());
			SetPosition(&rc);
			PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,&rc);
		} else if (!m_pCore->GetFullscreen()) {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			if (!m_fShowTitleBar) {
				m_Viewer.GetViewWindow().GetScreenPosition(&rc);
				MapWindowRect(NULL,hwnd,&rc);
				if (TitleBarUtil.IsSpot(&rc,&pt)) {
					if (!m_TitleBar.GetVisible()) {
						RECT rcBar;
						TitleBarUtil.Layout(&rc,&rcBar);
						m_TitleBar.SetPosition(&rcBar);
						m_TitleBar.SetVisible(true);
						::BringWindowToTop(m_TitleBar.GetHandle());
					}
				} else {
					if (m_TitleBar.GetVisible())
						m_TitleBar.SetVisible(false);
				}
			}
			if (!m_fShowStatusBar) {
				m_Viewer.GetViewWindow().GetScreenPosition(&rc);
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
			if (!m_fShowSideBar && SideBarOptions.ShowPopup()
					&& (m_fShowTitleBar || !m_TitleBar.GetVisible())
					&& (m_fShowStatusBar || !StatusView.GetVisible())) {
				m_Viewer.GetViewWindow().GetScreenPosition(&rc);
				MapWindowRect(NULL,hwnd,&rc);
				if (SideBarUtil.IsSpot(&rc,&pt)) {
					if (!SideBar.GetVisible()) {
						RECT rcBar;
						SideBarUtil.Layout(&rc,&rcBar);
						SideBar.SetPosition(&rcBar);
						SideBar.SetVisible(true);
						::BringWindowToTop(SideBar.GetHandle());
					}
				} else {
					if (SideBar.GetVisible())
						SideBar.SetVisible(false);
				}
			}
		} else {
			m_Fullscreen.OnMouseMove();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		::SendMessage(hwnd,WM_COMMAND,
			MAKEWPARAM(OperationOptions.GetLeftDoubleClickCommand(),COMMAND_FROM_MOUSE),0);
		return 0;

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		{
			int Command;

			if (wParam>=VK_F1 && wParam<=VK_F12) {
				if (!Accelerator.IsFunctionKeyChannelChange())
					break;
				Command=CM_CHANNELNO_FIRST+((int)wParam-VK_F1);
			} else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9) {
				if (!Accelerator.IsNumPadChannelChange())
					break;
				if (wParam==VK_NUMPAD0)
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+((int)wParam-VK_NUMPAD1);
			} else if (wParam>='0' && wParam<='9') {
				if (!Accelerator.IsDigitKeyChannelChange())
					break;
				if (wParam=='0')
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+((int)wParam-'1');
			} else if (wParam>=VK_F13 && wParam<=VK_F24
					&& !ControllerManager.IsControllerEnabled(TEXT("HDUS Remocon"))
					&& (::GetKeyState(VK_SHIFT)<0 || ::GetKeyState(VK_CONTROL)<0)) {
				ShowMessage(TEXT("リモコンを使用するためには、メニューの [プラグイン] -> [HDUSリモコン] でリモコンを有効にしてください。"),
							TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
				break;
			} else {
				break;
			}
			SendCommand(Command);
		}
		return 0;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			OnMouseWheel(wParam,lParam,fHorz);
			// WM_MOUSEHWHEEL は 1を返さないと繰り返し送られて来ないらしい
			return fHorz;
		}

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			if (pmis->itemID>=CM_ASPECTRATIO_FIRST && pmis->itemID<=CM_ASPECTRATIO_LAST) {
				if (AspectRatioIconMenu.OnMeasureItem(hwnd,wParam,lParam))
					return TRUE;
				break;
			}
			if (ChannelMenu.OnMeasureItem(hwnd,wParam,lParam))
				return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if (AspectRatioIconMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		if (ChannelMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		break;

// ウィンドウ枠を細くするためのコード
	case WM_NCACTIVATE:
		if (m_fThinFrame)
			return TRUE;
		break;

	case WM_NCCALCSIZE:
		if (m_fThinFrame) {
			if (wParam!=0) {
				NCCALCSIZE_PARAMS *pnccsp=reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

				::InflateRect(&pnccsp->rgrc[0],-m_ThinFrameWidth,-m_ThinFrameWidth);
			}
			return 0;
		}
		break;

	case WM_NCPAINT:
		if (m_fThinFrame) {
			HDC hdc=::GetWindowDC(hwnd);
			RECT rc,rcEmpty;

			::GetWindowRect(hwnd,&rc);
			::OffsetRect(&rc,-rc.left,-rc.top);
			rcEmpty=rc;
			::InflateRect(&rcEmpty,-m_ThinFrameWidth,-m_ThinFrameWidth);
			DrawUtil::FillBorder(hdc,&rc,&rcEmpty,&rc,
								 static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::ReleaseDC(hwnd,hdc);
			return 0;
		}
		break;

	case WM_NCHITTEST:
		if (m_fThinFrame) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int BorderWidth=m_ThinFrameWidth;
			RECT rc;
			int Code=HTCLIENT;

			::GetWindowRect(hwnd,&rc);
			if (x>=rc.left && x<rc.left+BorderWidth) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPLEFT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTLEFT;
					else if (y<rc.bottom)
						Code=HTBOTTOMLEFT;
				}
			} else if (x>=rc.right-BorderWidth && x<rc.right) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPRIGHT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTRIGHT;
					else if (y<rc.bottom)
						Code=HTBOTTOMRIGHT;
				}
			} else if (y>=rc.top && y<rc.top+BorderWidth) {
				Code=HTTOP;
			} else if (y>=rc.bottom-BorderWidth && y<rc.bottom) {
				Code=HTBOTTOM;
			}
			return Code;
		}
		break;
// ウィンドウ枠を細くするためのコード終わり

	case WM_INITMENUPOPUP:
		if (OnInitMenuPopup(reinterpret_cast<HMENU>(wParam)))
			return 0;
		break;

	case WM_UNINITMENUPOPUP:
		if (ChannelMenu.OnUninitMenuPopup(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_MENUSELECT:
		if (ChannelMenu.OnMenuSelect(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_SYSCOMMAND:
		switch ((wParam&0xFFFFFFF0UL)) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_ABOUT:
			{
				CAboutDialog AboutDialog;

				AboutDialog.Show(GetVideoHostWindow());
			}
			return 0;

		case SC_MINIMIZE:
		case SC_MAXIMIZE:
		case SC_RESTORE:
			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			break;

		case SC_CLOSE:
			SendCommand(CM_CLOSE);
			return 0;
		}
		break;

	case WM_APPCOMMAND:
		{
			int Command=Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_INPUT:
		return Accelerator.OnInput(hwnd,wParam,lParam);

	case WM_HOTKEY:
		{
			int Command=Accelerator.TranslateHotKey(wParam,lParam);

			if (Command>0)
				PostMessage(WM_COMMAND,Command,0);
		}
		return 0;

	case WM_SETFOCUS:
		m_Viewer.GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		{
			LPCTSTR pszText=reinterpret_cast<LPCTSTR>(lParam);

			m_TitleBar.SetLabel(pszText);
			if (m_pCore->GetFullscreen())
				::SetWindowText(m_Fullscreen.GetHandle(),pszText);
		}
		break;

	case WM_SETICON:
		if (wParam==ICON_SMALL)
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
		break;

	case WM_POWERBROADCAST:
		if (wParam==PBT_APMSUSPEND) {
			Logger.AddLog(TEXT("サスペンドへの移行メッセージを受信しました。"));
			if (m_fProgramGuideUpdating)
				EndProgramGuideUpdate();
			if (!m_fSrcFilterReleased) {
				m_RestoreChannelSpec.Store(&ChannelManager);
				AppMain.CloseTuner();
			}
			m_fRestorePreview=IsViewerEnabled();
			FinalizeViewer();
		} else if (wParam==PBT_APMRESUMESUSPEND) {
			Logger.AddLog(TEXT("サスペンドからの復帰メッセージを受信しました。"));
			if (!m_pCore->GetStandby()) {
				OpenTuner();	// 遅延させた方がいいかも
				if (m_fRestorePreview)
					InitializeViewer();
			}
		}
		break;

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
	case WM_DWMCOMPOSITIONCHANGED:
		OSDOptions.OnDwmCompositionChanged();
		return 0;

	case WM_APP_SERVICEUPDATE:
		// サービスが更新された
		{
			CServiceUpdateInfo *pInfo=reinterpret_cast<CServiceUpdateInfo*>(lParam);
			int i;

			if (pInfo->m_fStreamChanged) {
				if (m_ResetErrorCountTimer.IsEnabled())
					m_ResetErrorCountTimer.Begin(hwnd,2000);

				m_Viewer.GetDisplayBase().SetVisible(false);
			}

			if (!AppMain.IsChannelScanning()
					&& pInfo->m_NumServices>0 && pInfo->m_CurService>=0) {
				const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
				WORD ServiceID,TransportStreamID;

				TransportStreamID=pInfo->m_TransportStreamID;
				ServiceID=pInfo->m_pServiceList[pInfo->m_CurService].ServiceID;
				if (/*pInfo->m_fStreamChanged
						&& */TransportStreamID!=0 && ServiceID!=0
						&& !CoreEngine.IsNetworkDriver()
						&& (pChInfo==NULL
						|| ((pChInfo->GetTransportStreamID()!=0
						&& pChInfo->GetTransportStreamID()!=TransportStreamID)
						|| (pChInfo->GetServiceID()!=0
						&& pChInfo->GetServiceID()!=ServiceID)))) {
					// 外部からチャンネル変更されたか、
					// ドライバが開かれたときのデフォルトチャンネル
					AppMain.FollowChannelChange(TransportStreamID,ServiceID);
				}/* else if (pChInfo!=NULL && ServiceID!=0 && !CoreEngine.IsNetworkDriver()) {
					// サービスを選択する
					// チャンネル切り替え直後はまだPMTが来ていないので
					// サービスの選択ができないため
					WORD SID;

					SID=ChannelManager.GetCurrentServiceID();
					if (SID==0 && ChannelManager.GetCurrentService()<0
							&& pChInfo->GetServiceID()!=0) {
						SID=pChInfo->GetServiceID();
					}
					if (SID!=0 && SID!=ServiceID) {
						if (AppMain.SetServiceByID(SID,&pInfo->m_CurService))
							ChannelManager.SetCurrentService(pInfo->m_CurService);
					}

					if (OSDOptions.GetShowOSD() && wParam!=0)
						ShowChannelOSD();
				}*/
				if (pChInfo!=NULL && !CoreEngine.IsNetworkDriver()) {
					// チャンネルの情報を更新する
					// 古いチャンネル設定ファイルにはNIDとTSIDの情報が含まれていないため
					const WORD NetworkID=pInfo->m_NetworkID;

					if (NetworkID!=0) {
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
				}
				PluginList.SendServiceUpdateEvent();
			} else if (pInfo->m_fServiceListEmpty && pInfo->m_fStreamChanged
					&& !AppMain.IsChannelScanning()
					&& !m_fProgramGuideUpdating) {
				ShowNotificationBar(TEXT("このチャンネルは放送休止中です"),
									CNotificationBar::MESSAGE_ERROR);
			}

			delete pInfo;
			if (pNetworkRemocon!=NULL)
				pNetworkRemocon->GetChannel(&GetChannelReceiver);
		}
		return 0;

	case WM_APP_CHANNELCHANGE:
		{
			const CChannelList &List=pNetworkRemocon->GetChannelList();

			ChannelManager.SetNetworkRemoconCurrentChannel((int)wParam);
			MainMenu.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
				CM_CHANNELNO_FIRST+List.GetChannelNo(ChannelManager.GetNetworkRemoconCurrentChannel())-1);
			StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
			ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
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
				CPopupMenu Menu(hInst,IDM_TRAY);

				Menu.EnableItem(CM_SHOW,
								m_pCore->GetStandby() || IsMinimizeToTray());
				// お約束が必要な理由は以下を参照
				// http://support.microsoft.com/kb/135788/en-us
				ForegroundWindow(hwnd);				// お約束
				Menu.Popup(hwnd);
				::PostMessage(hwnd,WM_NULL,0,0);	// お約束
			}
			break;

		case WM_LBUTTONDOWN:
			SendCommand(CM_SHOW);
			break;
		}
		return 0;

	case WM_APP_EXECUTE:
		// 複数起動禁止時に複数起動された
		// (新しく起動されたプロセスから送られてくる)
		{
			ATOM atom=(ATOM)wParam;
			TCHAR szCmdLine[256];

			szCmdLine[0]='\0';
			if (atom!=0) {
				::GlobalGetAtomName(atom,szCmdLine,lengthof(szCmdLine));
				::GlobalDeleteAtom(atom);
			}
			OnExecute(szCmdLine);
		}
		return 0;

	case WM_APP_QUERYPORT:
		// 使っているポートを返す
		if (!m_fClosing && CoreEngine.IsNetworkDriver()) {
			WORD Port=ChannelManager.GetCurrentChannel()+
										(CoreEngine.IsUDPDriver()?1234:2230);
			WORD RemoconPort=pNetworkRemocon!=NULL?pNetworkRemocon->GetPort():0;
			return MAKELRESULT(Port,RemoconPort);
		}
		return 0;

	case WM_APP_FILEWRITEERROR:
		// ファイルの書き出しエラー
		ShowErrorMessage(TEXT("ファイルへの書き出しでエラーが発生しました。"));
		return 0;

	case WM_APP_VIDEOSIZECHANGED:
		// 映像サイズが変わった
		/*
			ストリームの映像サイズの変化を検知してから、それが実際に
			表示されるまでにはタイムラグがあるため、後で調整を行う
		*/
		m_VideoSizeChangedTimerCount=0;
		::SetTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED,1000,NULL);
		if (m_AspectRatioResetTime!=0
				&& !m_pCore->GetFullscreen() && !::IsZoomed(hwnd)
				&& IsViewerEnabled()
				&& DiffTime(m_AspectRatioResetTime,::GetTickCount())<6000) {
			int Width,Height;

			if (CoreEngine.GetVideoViewSize(&Width,&Height)) {
				SIZE sz;
				m_Viewer.GetVideoContainer().GetClientSize(&sz);
				if (sz.cx<Width*sz.cy/Height)
					AdjustWindowSize(Width*sz.cy/Height,sz.cy);
				m_AspectRatioResetTime=0;
			}
		}
		return 0;

	case WM_APP_EMMPROCESSED:
		// EMM 処理が行われた
		Logger.AddLog(wParam!=0?TEXT("EMM処理を行いました。"):TEXT("EMM処理でエラーが発生しました。"));
		return 0;

	case WM_APP_ECMERROR:
		// ECM 処理のエラーが発生した
		{
			LPTSTR pszText=reinterpret_cast<LPTSTR>(lParam);

			if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_ECMERROR))
				ShowNotificationBar(TEXT("スクランブル解除でエラーが発生しました"),
									CNotificationBar::MESSAGE_ERROR);
			if (pszText!=NULL) {
				TCHAR szText[256];
				::wnsprintf(szText,lengthof(szText)-1,TEXT("ECM処理でエラーが発生しました。(%s)"),pszText);
				szText[lengthof(szText)-1]='\0';
				Logger.AddLog(szText);
				delete [] pszText;
			} else {
				Logger.AddLog(TEXT("ECM処理でエラーが発生しました。"));
			}
		}
		return 0;

	case WM_APP_EPGLOADED:
		// EPGファイルが読み込まれた
		if (fShowPanelWindow
				&& (PanelForm.GetCurPageID()==PANEL_ID_PROGRAMLIST
					|| PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)) {
			UpdatePanel();
		}
		return 0;

	case WM_DISPLAYCHANGE:
		CoreEngine.m_DtvEngine.m_MediaViewer.DisplayModeChanged();
		break;

	case WM_THEMECHANGED:
		ChannelMenu.Destroy();
		return 0;

	case WM_CLOSE:
		if (!ConfirmExit())
			return 0;

		m_fClosing=true;

		::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		Logger.AddLog(TEXT("ウィンドウを閉じています..."));

		::KillTimer(hwnd,TIMER_ID_UPDATE);

		//CoreEngine.m_DtvEngine.EnablePreview(false);

		PluginList.SendCloseEvent();

		m_Fullscreen.Destroy();

		ShowFloatingWindows(false);
		break;

	case WM_DESTROY:
		HtmlHelpClass.Finalize();
		m_pCore->PreventDisplaySave(false);

#ifndef _DEBUG
		// 終了監視スレッド開始(本当はこういう事はしたくないが…)
		HANDLE hEvent,hThread;
		hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
		if (hEvent!=NULL)
			hThread=::CreateThread(NULL,0,ExitWatchThread,hEvent,0,NULL);
#endif

		SAFE_DELETE(pNetworkRemocon);
		ResidentManager.Finalize();
		ChannelMenu.Destroy();
		MainMenu.Destroy();
		Accelerator.Finalize();
		ControllerManager.DeleteAllControllers();
		TaskbarManager.Finalize();
		ProgramGuideFrame.Destroy();
		NotifyBalloonTip.Finalize();

		fEnablePlay=IsViewerEnabled();

		CoreEngine.m_DtvEngine.SetTracer(&Logger);
		CoreEngine.Close();
		CoreEngine.m_DtvEngine.SetTracer(NULL);
		CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(NULL);

		if (!CmdLineParser.m_fNoPlugin)
			PluginOptions.StorePluginOptions();
		PluginList.FreePlugins();

		// 終了時の負荷で他のプロセスの録画がドロップすることがあるらしい...
		::SetPriorityClass(::GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);

		if (!CmdLineParser.m_fNoEpg)
			EpgOptions.SaveEpgFile(&EpgProgramList);
		EpgOptions.SaveLogoFile();
		EpgOptions.Finalize();

		{
			TCHAR szLogoMapName[MAX_PATH];
			::GetModuleFileName(NULL,szLogoMapName,MAX_PATH);
			::PathRenameExtension(szLogoMapName,TEXT(".logo.ini"));
			Logger.AddLog(TEXT("ロゴ設定を保存しています..."));
			LogoManager.SaveLogoIDMap(szLogoMapName);
		}

#ifndef _DEBUG
		if (hThread!=NULL) {
			if (::SignalObjectAndWait(hEvent,hThread,5000,FALSE)!=WAIT_OBJECT_0)
				::TerminateThread(hThread,-1);
			::CloseHandle(hThread);
		}
		if (hEvent!=NULL)
			::CloseHandle(hEvent);
#endif

		// Finalize()ではエラー時にダイアログを出すことがあるので、
		// 終了監視の外に出す必要がある
		AppMain.SaveCurrentChannel();
		AppMain.Finalize();
		return 0;

	case WM_ACTIVATEAPP:
		if (ControllerManager.OnActivateApp(hwnd,wParam,lParam))
			return 0;
		break;

	default:
		/*
		if (ControllerManager.HandleMessage(hwnd,uMsg,wParam,lParam))
			return 0;
		*/
		if (ResidentManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
		if (TaskbarManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CMainWindow::OnCreate(const CREATESTRUCT *pcs)
{
	m_LayoutBase.Create(m_hwnd,
						WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_Viewer.Create(m_LayoutBase.GetHandle(),IDC_VIEW,IDC_VIDEOCONTAINER,m_hwnd);
	ViewWindowEventHandler.Initialize(&m_TitleBar,&StatusView,&SideBar);
	m_Viewer.GetViewWindow().SetEventHandler(&ViewWindowEventHandler);
	m_Viewer.GetVideoContainer().SetEventHandler(&VideoContainerEventHandler);
	m_Viewer.GetDisplayBase().SetEventHandler(&m_DisplayBaseEventHandler);

	m_TitleBar.Create(m_LayoutBase.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS | (m_fShowTitleBar && m_fCustomTitleBar?WS_VISIBLE:0),
					  0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&TitleBarUtil);
	m_TitleBar.SetLabel(pcs->lpszName);
	m_TitleBar.SetIcon(::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
	m_TitleBar.SetMaximizeMode((pcs->style&WS_MAXIMIZE)!=0);

	StatusView.Create(m_LayoutBase.GetHandle(),
		//WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,0,IDC_STATUS);
	StatusView.SetEventHandler(&StatusViewEventHandler);
	StatusView.AddItem(new CChannelStatusItem);
	StatusView.AddItem(new CVideoSizeStatusItem);
	StatusView.AddItem(new CVolumeStatusItem);
	StatusView.AddItem(new CAudioChannelStatusItem);
	CRecordStatusItem *pRecordStatusItem=new CRecordStatusItem;
	pRecordStatusItem->ShowRemainTime(RecordOptions.GetShowRemainTime());
	StatusView.AddItem(pRecordStatusItem);
	StatusView.AddItem(new CCaptureStatusItem);
	StatusView.AddItem(new CErrorStatusItem);
	StatusView.AddItem(new CSignalLevelStatusItem);
	CClockStatusItem *pClockStatusItem=new CClockStatusItem;
	pClockStatusItem->SetTOT(StatusOptions.GetShowTOTTime());
	StatusView.AddItem(pClockStatusItem);
	StatusView.AddItem(new CProgramInfoStatusItem);
	StatusView.AddItem(new CBufferingStatusItem);
	StatusView.AddItem(new CTunerStatusItem);
	StatusView.AddItem(new CMediaBitRateStatusItem);
	StatusOptions.ApplyOptions();
	if (!m_fShowStatusBar) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetPosition(&rc);
		::BringWindowToTop(StatusView.GetHandle());
	}
	StatusView.SetSingleText(TEXT("起動中..."));

	NotificationBar.Create(m_Viewer.GetVideoContainer().GetHandle(),
						   WS_CHILD | WS_CLIPSIBLINGS);

	SideBarOptions.ApplySideBarOptions();
	SideBar.SetEventHandler(&SideBarUtil);
	SideBar.Create(m_LayoutBase.GetHandle(),
				   WS_CHILD | WS_CLIPSIBLINGS | (m_fShowSideBar?WS_VISIBLE:0),
				   0,IDC_SIDEBAR);

	Layout::CWindowContainer *pWindowContainer;
	Layout::CSplitter *pSideBarSplitter=new Layout::CSplitter(CONTAINER_ID_SIDEBARSPLITTER);
	CSideBarOptions::PlaceType SideBarPlace=SideBarOptions.GetPlace();
	bool fSideBarVertical=SideBarPlace==CSideBarOptions::PLACE_LEFT
						|| SideBarPlace==CSideBarOptions::PLACE_RIGHT;
	int SideBarWidth=SideBar.GetBarWidth();
	pSideBarSplitter->SetStyle(Layout::CSplitter::STYLE_FIXED |
		(fSideBarVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT));
	pSideBarSplitter->SetVisible(true);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pWindowContainer->SetWindow(&m_Viewer.GetViewWindow());
	pWindowContainer->SetMinSize(32,32);
	pWindowContainer->SetVisible(true);
	pSideBarSplitter->SetPane(0,pWindowContainer);
	pSideBarSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_SIDEBAR);
	pWindowContainer->SetWindow(&SideBar);
	pWindowContainer->SetMinSize(SideBarWidth,SideBarWidth);
	pWindowContainer->SetVisible(m_fShowSideBar);
	pSideBarSplitter->SetPane(1,pWindowContainer);
	pSideBarSplitter->SetPaneSize(CONTAINER_ID_SIDEBAR,SideBarWidth);
	if (SideBarPlace==CSideBarOptions::PLACE_LEFT
			|| SideBarPlace==CSideBarOptions::PLACE_TOP)
		pSideBarSplitter->SwapPane();

	Layout::CSplitter *pTitleBarSplitter=new Layout::CSplitter(CONTAINER_ID_TITLEBARSPLITTER);
	pTitleBarSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pTitleBarSplitter->SetVisible(true);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_TITLEBAR);
	pWindowContainer->SetWindow(&m_TitleBar);
	pWindowContainer->SetMinSize(0,m_TitleBar.GetHeight());
	pWindowContainer->SetVisible(m_fShowTitleBar && m_fCustomTitleBar);
	pTitleBarSplitter->SetPane(0,pWindowContainer);
	pTitleBarSplitter->SetPane(1,pSideBarSplitter);
	pTitleBarSplitter->SetPaneSize(CONTAINER_ID_TITLEBAR,m_TitleBar.GetHeight());
	pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);

	Layout::CSplitter *pPanelSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pPanelSplitter->SetVisible(true);
	pPanelSplitter->SetPane(0,pTitleBarSplitter);
	pPanelSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pWindowContainer->SetMinSize(64,0);
	pPanelSplitter->SetPane(1,pWindowContainer);

	Layout::CSplitter *pStatusSplitter=new Layout::CSplitter(CONTAINER_ID_STATUSSPLITTER);
	pStatusSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pStatusSplitter->SetVisible(true);
	pStatusSplitter->SetPane(0,pPanelSplitter);
	pStatusSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_STATUS);
	pWindowContainer->SetWindow(&StatusView);
	pWindowContainer->SetMinSize(0,StatusView.GetHeight());
	pWindowContainer->SetVisible(m_fShowStatusBar);
	pStatusSplitter->SetPane(1,pWindowContainer);
	pStatusSplitter->SetPaneSize(CONTAINER_ID_STATUS,StatusView.GetHeight());

	m_LayoutBase.SetTopContainer(pStatusSplitter);

	OSDManager.SetEventHandler(this);

	if (m_fThinFrame) {
		CAeroGlass Aero;
		Aero.EnableNcRendering(m_hwnd,false);
	}

	MainMenu.Create(hInst);
	/*
	MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
							CM_ASPECTRATIO_FIRST+m_AspectRatioType);
	*/
	MainMenu.CheckItem(CM_ALWAYSONTOP,m_pCore->GetAlwaysOnTop());
	int Gain,SurroundGain;
	CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
	for (int i=0;i<lengthof(g_AudioGainList);i++) {
		if (Gain==g_AudioGainList[i])
			MainMenu.CheckRadioItem(CM_AUDIOGAIN_FIRST,CM_AUDIOGAIN_LAST,
									CM_AUDIOGAIN_FIRST+i);
		if (SurroundGain==g_AudioGainList[i])
			MainMenu.CheckRadioItem(CM_SURROUNDAUDIOGAIN_FIRST,CM_SURROUNDAUDIOGAIN_LAST,
									CM_SURROUNDAUDIOGAIN_FIRST+i);
	}
	/*
	MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+m_pCore->GetStereoMode());
	*/
	MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
							CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize());
	MainMenu.CheckItem(CM_CAPTUREPREVIEW,fShowCaptureWindow);
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	MainMenu.CheckItem(CM_PANEL,fShowPanelWindow);
	MainMenu.CheckItem(CM_STATUSBAR,m_fShowStatusBar);
	MainMenu.CheckItem(CM_TITLEBAR,m_fShowTitleBar);
	MainMenu.CheckItem(CM_SIDEBAR,m_fShowSideBar);
	MainMenu.CheckItem(CM_THINFRAME,m_fThinFrame);
	MainMenu.EnableItem(CM_CUSTOMTITLEBAR,!m_fThinFrame);
	MainMenu.CheckItem(CM_CUSTOMTITLEBAR,m_fCustomTitleBar);

	HMENU hSysMenu;
	hSysMenu=GetSystemMenu(m_hwnd,FALSE);
	AppendMenu(hSysMenu,MFT_SEPARATOR,0,NULL);
	AppendMenu(hSysMenu,MFT_STRING | MFS_ENABLED,SC_ABOUT,
												TEXT("バージョン情報(&A)..."));

	AspectRatioIconMenu.Initialize(MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO),
								   hInst,MAKEINTRESOURCE(IDB_PANSCAN),16,RGB(192,192,192),
								   CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST);
	AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_FIRST+m_AspectRatioType);

	TaskbarManager.Initialize(m_hwnd);

	NotifyBalloonTip.Initialize(m_hwnd);

	CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
		(pcs->style&WS_MAXIMIZE)!=0?ViewOptions.GetMaximizeStretchMode():
						m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
									CMediaViewer::STRETCH_KEEPASPECTRATIO);

	::SetTimer(m_hwnd,TIMER_ID_UPDATE,UPDATE_TIMER_INTERVAL,NULL);
	return true;
}


void CMainWindow::OnSizeChanged(UINT State,int Width,int Height)
{
	if (State==SIZE_MINIMIZED) {
		ResidentManager.SetStatus(CResidentManager::STATUS_MINIMIZED,
								  CResidentManager::STATUS_MINIMIZED);
		if (ViewOptions.GetDisablePreviewWhenMinimized()) {
			if (IsViewerEnabled()) {
				m_pCore->EnableViewer(false);
				m_fRestorePreview=true;
			}
		}
	} else if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
		SetWindowVisible();
	}

	if (State==SIZE_MAXIMIZED
			&& (!m_fShowTitleBar || m_fCustomTitleBar)) {
		HMONITOR hMonitor=::MonitorFromWindow(m_hwnd,MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		SetPosition(&mi.rcWork);
		SIZE sz;
		GetClientSize(&sz);
		Width=sz.cx;
		Height=sz.cy;
	}
	m_TitleBar.SetMaximizeMode(State==SIZE_MAXIMIZED);

	// ウィンドウ枠を細くしていると最小化時に変なサイズにされる
	if (State==SIZE_MINIMIZED)
		return;

	m_LayoutBase.SetPosition(0,0,Width,Height);

	if (!m_pCore->GetFullscreen()) {
		if (State==SIZE_MAXIMIZED)
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
										ViewOptions.GetMaximizeStretchMode());
		else if (State==SIZE_RESTORED)
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
							m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
										CMediaViewer::STRETCH_KEEPASPECTRATIO);
	}

	StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


bool CMainWindow::OnSizeChanging(UINT Edge,RECT *pRect)
{
	RECT rcOld;
	bool fChanged=false;

	GetPosition(&rcOld);
	bool fKeepRatio=ViewOptions.GetAdjustAspectResizing();
	if (::GetKeyState(VK_SHIFT)<0)
		fKeepRatio=!fKeepRatio;
	if (fKeepRatio) {
		BYTE XAspect,YAspect;

		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(
														&XAspect,&YAspect)) {
			RECT rcWindow,rcClient;
			int XMargin,YMargin,Width,Height;

			GetPosition(&rcWindow);
			GetClientRect(&rcClient);
			m_Viewer.GetViewWindow().CalcClientRect(&rcClient);
			if (m_fShowStatusBar)
				rcClient.bottom-=StatusView.GetHeight();
			if (m_fShowTitleBar && m_fCustomTitleBar)
				TitleBarUtil.AdjustArea(&rcClient);
			if (m_fShowSideBar)
				SideBarUtil.AdjustArea(&rcClient);
			if (fShowPanelWindow && !PanelFrame.GetFloating()) {
				Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
					m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
				rcClient.right-=pSplitter->GetPaneSize(CONTAINER_ID_PANEL)+pSplitter->GetBarWidth();
			}
			::OffsetRect(&rcClient,-rcClient.left,-rcClient.top);
			if (rcClient.right<=0 || rcClient.bottom<=0)
				goto SizingEnd;
			XMargin=(rcWindow.right-rcWindow.left)-rcClient.right;
			YMargin=(rcWindow.bottom-rcWindow.top)-rcClient.bottom;
			Width=(pRect->right-pRect->left)-XMargin;
			Height=(pRect->bottom-pRect->top)-YMargin;
			if (Width<=0 || Height<=0)
				goto SizingEnd;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_RIGHT)
				Height=Width*YAspect/XAspect;
			else if (Edge==WMSZ_TOP || Edge==WMSZ_BOTTOM)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect<Height*XAspect)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect>Height*XAspect)
				Height=Width*YAspect/XAspect;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_TOPLEFT || Edge==WMSZ_BOTTOMLEFT)
				pRect->left=pRect->right-(Width+XMargin);
			else
				pRect->right=pRect->left+Width+XMargin;
			if (Edge==WMSZ_TOP || Edge==WMSZ_TOPLEFT || Edge==WMSZ_TOPRIGHT)
				pRect->top=pRect->bottom-(Height+YMargin);
			else
				pRect->bottom=pRect->top+Height+YMargin;
			fChanged=true;
		}
	}
SizingEnd:
	PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,pRect);
	return fChanged;
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
	case CM_ZOOM_250:
	case CM_ZOOM_300:
	case CM_CUSTOMZOOM_1:
	case CM_CUSTOMZOOM_2:
	case CM_CUSTOMZOOM_3:
	case CM_CUSTOMZOOM_4:
	case CM_CUSTOMZOOM_5:
		{
			CZoomOptions::ZoomRate Zoom;

			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			if (IsZoomed(hwnd))
				::ShowWindow(hwnd,SW_RESTORE);
			if (ZoomOptions.GetZoomRateByCommand(id,&Zoom))
				SetZoomRate(Zoom.Rate,Zoom.Factor);
		}
		return;

	case CM_ZOOMOPTIONS:
		if (ZoomOptions.ShowDialog(GetVideoHostWindow()))
			SideBarOptions.SetSideBarImage();
		return;

	case CM_ASPECTRATIO:
		SendCommand(CM_ASPECTRATIO_FIRST+
			(m_AspectRatioType+1)%(CM_ASPECTRATIO_LAST-CM_ASPECTRATIO_FIRST+1));
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
			if (!m_pCore->GetFullscreen() && !::IsZoomed(hwnd)) {
				if (!ViewOptions.GetPanScanNoResizeWindow()) {
					int ZoomNum,ZoomDenom;
					int Width,Height;

					if (GetZoomRate(&ZoomNum,&ZoomDenom)
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

					m_Viewer.GetVideoContainer().GetClientSize(&sz);
					if (CoreEngine.GetVideoViewSize(&Width,&Height))
						AdjustWindowSize(Width*sz.cy/Height,sz.cy);
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
					ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
				}
			}
			m_AspectRatioType=i;
			m_AspectRatioResetTime=0;
			/*
			MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
									CM_ASPECTRATIO_FIRST+m_AspectRatioType);
			*/
			AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_FIRST+m_AspectRatioType);
		}
		return;

	case CM_FRAMECUT:
		m_fFrameCut=!m_fFrameCut;
		CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
						m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
									CMediaViewer::STRETCH_KEEPASPECTRATIO);
		return;

	case CM_FULLSCREEN:
		m_pCore->ToggleFullscreen();
		return;

	case CM_ALWAYSONTOP:
		m_pCore->SetAlwaysOnTop(!m_pCore->GetAlwaysOnTop());
		return;

	case CM_VOLUME_UP:
	case CM_VOLUME_DOWN:
		{
			const int CurVolume=m_pCore->GetVolume();
			int Volume=CurVolume;

			if (id==CM_VOLUME_UP) {
				Volume+=OperationOptions.GetVolumeStep();
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume-=OperationOptions.GetVolumeStep();
				if (Volume<0)
					Volume=0;
			}
			if (Volume!=CurVolume || m_pCore->GetMute())
				m_pCore->SetVolume(Volume);
		}
		return;

	case CM_VOLUME_MUTE:
		m_pCore->SetMute(!m_pCore->GetMute());
		return;

	case CM_AUDIOGAIN_NONE:
	case CM_AUDIOGAIN_125:
	case CM_AUDIOGAIN_150:
	case CM_AUDIOGAIN_200:
		{
			int SurroundGain;

			CoreEngine.GetAudioGainControl(NULL,&SurroundGain);
			CoreEngine.SetAudioGainControl(
				g_AudioGainList[id-CM_AUDIOGAIN_FIRST],SurroundGain);
			MainMenu.CheckRadioItem(CM_AUDIOGAIN_NONE,CM_AUDIOGAIN_LAST,id);
		}
		return;

	case CM_SURROUNDAUDIOGAIN_NONE:
	case CM_SURROUNDAUDIOGAIN_125:
	case CM_SURROUNDAUDIOGAIN_150:
	case CM_SURROUNDAUDIOGAIN_200:
		{
			int Gain;

			CoreEngine.GetAudioGainControl(&Gain,NULL);
			CoreEngine.SetAudioGainControl(
				Gain,g_AudioGainList[id-CM_SURROUNDAUDIOGAIN_FIRST]);
			MainMenu.CheckRadioItem(CM_SURROUNDAUDIOGAIN_NONE,CM_SURROUNDAUDIOGAIN_LAST,id);
		}
		return;

	case CM_STEREO_THROUGH:
	case CM_STEREO_LEFT:
	case CM_STEREO_RIGHT:
		m_pCore->SetStereoMode(id-CM_STEREO_THROUGH);
		return;

	case CM_SWITCHAUDIO:
		m_pCore->SwitchAudio();
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
				int Width,Height,OrigWidth,OrigHeight;
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
				if (OrigHeight==1088) {
					rc.top=rc.top*1080/1088;
					rc.bottom=rc.bottom*1080/1088;
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
				TCHAR szComment[512],szEventName[256];
				CaptureOptions.GetCommentText(szComment,lengthof(szComment),
					pChInfo!=NULL?pChInfo->GetName():NULL,
					CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName))>0?szEventName:NULL);
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
		PluginList.SendResetEvent();
		return;

	case CM_RESETVIEWER:
		CoreEngine.m_DtvEngine.ResetMediaViewer();
		return;

	case CM_REBUILDVIEWER:
		InitializeViewer();
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
			if (!RecordManager.IsPaused()
					&& !RecordOptions.ConfirmStop(GetVideoHostWindow()))
				return;
			AppMain.StopRecord();
		} else {
			if (RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
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
			if (RecordManager.IsRecording()) {
				if (RecordManager.RecordDialog(GetVideoHostWindow()))
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
			} else {
				if (RecordManager.GetFileName()==NULL) {
					TCHAR szFileName[MAX_PATH];

					if (RecordOptions.GetFilePath(szFileName,MAX_PATH))
						RecordManager.SetFileName(szFileName);
				}
				if (!RecordManager.IsReserved())
					RecordOptions.ApplyOptions(&RecordManager);
				if (RecordManager.RecordDialog(GetVideoHostWindow())) {
					RecordManager.SetClient(CRecordManager::CLIENT_USER);
					if (RecordManager.IsReserved()) {
						StatusView.UpdateItem(STATUS_ITEM_RECORD);
					} else {
						AppMain.StartReservedRecord();
					}
				} else {
					// 予約がキャンセルされた場合も表示を更新する
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			}
		}
		return;

	case CM_RECORDEVENT:
		if (RecordManager.IsRecording()) {
			RecordManager.SetStopOnEventEnd(!RecordManager.GetStopOnEventEnd());
		} else {
			SendCommand(CM_RECORD_START);
			if (RecordManager.IsRecording())
				RecordManager.SetStopOnEventEnd(true);
		}
		return;

	case CM_EXITONRECORDINGSTOP:
		m_fExitOnRecordingStop=!m_fExitOnRecordingStop;
		return;

	case CM_OPTIONS_RECORD:
		if (IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd,COptionDialog::PAGE_RECORD);
		return;

	case CM_TIMESHIFTRECORDING:
		if (!RecordManager.IsRecording()) {
			if (RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
			}
			AppMain.StartRecord(NULL,NULL,NULL,CRecordManager::CLIENT_USER,true);
		}
		return;

	case CM_ENABLETIMESHIFTRECORDING:
		RecordOptions.EnableTimeShiftRecording(!RecordOptions.IsTimeShiftRecordingEnabled());
		return;

	case CM_STATUSBARRECORD:
		OnCommand(hwnd,RecordOptions.GetStatusBarRecordCommand(),NULL,0);
		return;

	case CM_DISABLEVIEWER:
		m_pCore->EnableViewer(!IsViewerEnabled());
		return;

	/*
	case CM_AUDIOONLY:
		{
			static bool fAudioOnly=false;
			fAudioOnly=!fAudioOnly;
			CoreEngine.m_DtvEngine.m_MediaViewer.SetAudioOnly(fAudioOnly);
		}
		return;
	*/

	case CM_PANEL:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.ShowPanel(!m_Fullscreen.IsPanelVisible());
			return;
		}
		fShowPanelWindow=!fShowPanelWindow;
		if (fShowPanelWindow) {
			PanelFrame.SetPanelVisible(true);
		} else {
			PanelFrame.SetPanelVisible(false);
			InfoPanel.ResetStatistics();
			//ProgramListPanel.ClearProgramList();
			ChannelPanel.ClearChannelList();
		}
		if (!PanelFrame.GetFloating()) {
			// パネルの幅に合わせてウィンドウサイズを拡縮
			Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			const int Width=PanelFrame.GetDockingWidth()+pSplitter->GetBarWidth();
			RECT rc;

			GetPosition(&rc);
			if (pSplitter->GetPane(0)->GetID()==CONTAINER_ID_PANEL) {
				if (fShowPanelWindow)
					rc.left-=Width;
				else
					rc.left+=Width;
			} else {
				if (fShowPanelWindow)
					rc.right+=Width;
				else
					rc.right-=Width;
			}
			SetPosition(&rc);
			if (!fShowPanelWindow)
				::SetFocus(hwnd);
		}
		if (fShowPanelWindow)
			UpdatePanel();
		MainMenu.CheckItem(CM_PANEL,fShowPanelWindow);
		return;

	case CM_PROGRAMGUIDE:
		fShowProgramGuide=!fShowProgramGuide;
		if (fShowProgramGuide) {
			const bool fOnScreen=ProgramGuideOptions.GetOnScreen()
				|| (m_pCore->GetFullscreen() && ::GetSystemMetrics(SM_CMONITORS)==1);

			HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
			if (fOnScreen) {
				ProgramGuideDisplay.SetEventHandler(&ProgramGuideDisplayEventHandler);
				ProgramGuideDisplay.Create(m_Viewer.GetDisplayBase().GetParent()->GetHandle(),
					WS_CHILD | WS_CLIPCHILDREN);
				m_Viewer.GetDisplayBase().SetDisplayView(&ProgramGuideDisplay);
			} else {
				ProgramGuideFrame.Create(NULL,
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
						WS_THICKFRAME | WS_CLIPCHILDREN | WS_VISIBLE);
			}
			SYSTEMTIME stFirst,stLast;
			ProgramGuideOptions.GetTimeRange(&stFirst,&stLast);
			ProgramGuide.SetTimeRange(&stFirst,&stLast);
			ProgramGuide.SetViewDay(CProgramGuide::DAY_TODAY);
			if (fOnScreen)
				m_Viewer.GetDisplayBase().SetVisible(true);
			else
				ProgramGuideFrame.Update();
			const CTuningSpaceList *pList=ChannelManager.GetTuningSpaceList();
			int Space;
			if (!CoreEngine.IsNetworkDriver())
				Space=ChannelManager.GetCurrentSpace();
			else
				Space=-1;
			ProgramGuide.SetTuningSpaceList(CoreEngine.GetDriverFileName(),pList,Space);
			ProgramGuide.UpdateProgramGuide(true);
			::SetCursor(hcurOld);
		} else {
			if (ProgramGuideFrame.IsCreated()) {
				ProgramGuideFrame.Destroy();
			} else {
				m_Viewer.GetDisplayBase().SetVisible(false);
			}
		}
		MainMenu.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		return;

	case CM_STATUSBAR:
		SetStatusBarVisible(!m_fShowStatusBar);
		return;

	case CM_TITLEBAR:
		SetTitleBarVisible(!m_fShowTitleBar);
		return;

	case CM_SIDEBAR:
		SetSideBarVisible(!m_fShowSideBar);
		return;

	case CM_THINFRAME:
		SetThinFrame(!m_fThinFrame);
		return;

	case CM_CUSTOMTITLEBAR:
		SetCustomTitleBar(!m_fCustomTitleBar);
		return;

	case CM_VIDEODECODERPROPERTY:
	case CM_VIDEORENDERERPROPERTY:
	case CM_AUDIOFILTERPROPERTY:
	case CM_AUDIORENDERERPROPERTY:
	case CM_DEMULTIPLEXERPROPERTY:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==NULL || ::IsWindowEnabled(hwndOwner)) {
				for (int i=0;i<lengthof(g_DirectShowFilterPropertyList);i++) {
					if (g_DirectShowFilterPropertyList[i].Command==id) {
						CoreEngine.m_DtvEngine.m_MediaViewer.DisplayFilterProperty(
							g_DirectShowFilterPropertyList[i].Filter,hwndOwner);
						break;
					}
				}
			}
		}
		return;

	case CM_OPTIONS:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==NULL || IsWindowEnabled(hwndOwner))
				OptionDialog.ShowDialog(hwndOwner);
		}
		return;

	case CM_STREAMINFO:
		if (!StreamInfo.IsCreated()) {
			StreamInfo.Create(hwnd);
		} else {
			StreamInfo.Destroy();
		}
		MainMenu.CheckItem(CM_STREAMINFO,StreamInfo.IsCreated());
		return;

	case CM_CLOSE:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else if (ResidentManager.GetResident()) {
			m_pCore->SetStandby(true);
		} else {
			PostMessage(WM_CLOSE,0,0);
		}
		return;

	case CM_EXIT:
		PostMessage(WM_CLOSE,0,0);
		return;

	case CM_SHOW:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
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

				if (pList->HasRemoteControlKeyID() && pInfo->GetChannelNo()!=0)
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNEL_FIRST+pInfo->GetChannelIndex());
			} else {
				SendCommand(CM_CHANNEL_FIRST);
			}
		}
		return;

	case CM_CHANNEL_BACKWARD:
	case CM_CHANNEL_FORWARD:
		{
			const CChannelHistory::CChannel *pChannel;

			if (id==CM_CHANNEL_BACKWARD)
				pChannel=ChannelHistory.Backward();
			else
				pChannel=ChannelHistory.Forward();
			if (pChannel!=NULL)
				AppMain.SetDriverAndChannel(pChannel->GetDriverFileName(),pChannel);
		}
		return;

	case CM_MENU:
		{
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			m_pCore->PopupMenu(&pt);
		}
		return;

	case CM_ACTIVATE:
		{
			HWND hwndHost=GetVideoHostWindow();

			if (hwndHost!=NULL)
				ForegroundWindow(hwndHost);
		}
		return;

	case CM_MINIMIZE:
		::ShowWindow(hwnd,::IsIconic(hwnd)?SW_RESTORE:SW_MINIMIZE);
		return;

	case CM_MAXIMIZE:
		::ShowWindow(hwnd,::IsZoomed(hwnd)?SW_RESTORE:SW_MAXIMIZE);
		return;

	case CM_CHANNELDISPLAYMENU:
		if (!ChannelDisplayMenu.GetVisible()) {
			HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

			ChannelDisplayMenu.SetFont(OSDOptions.GetDisplayMenuFont(),
									   OSDOptions.IsDisplayMenuFontAutoSize());
			if (!ChannelDisplayMenu.IsCreated()) {
				ChannelDisplayMenu.SetEventHandler(&ChannelDisplayMenuEventHandler);
				ChannelDisplayMenu.Create(m_Viewer.GetDisplayBase().GetParent()->GetHandle(),
										  WS_CHILD | WS_CLIPCHILDREN);
				ChannelDisplayMenu.SetDriverManager(&DriverManager);
				ChannelDisplayMenu.SetLogoManager(&LogoManager);
			}
			m_Viewer.GetDisplayBase().SetDisplayView(&ChannelDisplayMenu);
			m_Viewer.GetDisplayBase().SetVisible(true);
			if (CoreEngine.IsDriverSpecified())
				ChannelDisplayMenu.SetSelect(CoreEngine.GetDriverFileName(),
											 ChannelManager.GetCurrentChannelInfo());
			ChannelDisplayMenu.Update();
			::SetCursor(hcurOld);
		} else {
			m_Viewer.GetDisplayBase().SetVisible(false);
		}
		return;

	case CM_ENABLEBUFFERING:
		CoreEngine.SetPacketBuffering(!CoreEngine.GetPacketBuffering());
		PlaybackOptions.SetPacketBuffering(CoreEngine.GetPacketBuffering());
		return;

	case CM_RESETBUFFER:
		CoreEngine.m_DtvEngine.ResetBuffer();
		return;

	case CM_RESETERRORCOUNT:
		CoreEngine.ResetErrorCount();
		StatusView.UpdateItem(STATUS_ITEM_ERROR);
		InfoPanel.UpdateErrorCount();
		PluginList.SendStatusResetEvent();
		return;

	case CM_SHOWRECORDREMAINTIME:
		{
			CRecordStatusItem *pItem=
				dynamic_cast<CRecordStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_RECORD));

			if (pItem!=NULL) {
				bool fRemain=!RecordOptions.GetShowRemainTime();
				RecordOptions.SetShowRemainTime(fRemain);
				pItem->ShowRemainTime(fRemain);
			}
		}
		return;

	case CM_SHOWTOTTIME:
		{
			bool fTOT=!StatusOptions.GetShowTOTTime();
			CClockStatusItem *pItem=
				dynamic_cast<CClockStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_CLOCK));

			StatusOptions.SetShowTOTTime(fTOT);
			if (pItem!=NULL)
				pItem->SetTOT(fTOT);
		}
		return;

	case CM_ADJUSTTOTTIME:
		TotTimeAdjuster.BeginAdjust();
		return;

	case CM_CHANNELMENU:
		{
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			MainMenu.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,TPM_RIGHTBUTTON,
								  pt.x,pt.y,MainWindow.GetHandle());
		}
		return;

	case CM_TUNINGSPACEMENU:
		{
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			MainMenu.PopupSubMenu(CMainMenu::SUBMENU_SPACE,
								  TPM_RIGHTBUTTON,pt.x,pt.y,hwnd);
		}
		return;

	case CM_RECENTCHANNELMENU:
		{
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			MainMenu.PopupSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY,
								  TPM_RIGHTBUTTON,pt.x,pt.y,hwnd);
		}
		return;

	case CM_SIDEBAR_PLACE_LEFT:
	case CM_SIDEBAR_PLACE_RIGHT:
	case CM_SIDEBAR_PLACE_TOP:
	case CM_SIDEBAR_PLACE_BOTTOM:
		{
			CSideBarOptions::PlaceType Place=(CSideBarOptions::PlaceType)(id-CM_SIDEBAR_PLACE_FIRST);

			if (Place!=SideBarOptions.GetPlace()) {
				bool fVertical=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_RIGHT;
				int Pane=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_TOP?0:1;

				SideBarOptions.SetPlace(Place);
				SideBar.SetVertical(fVertical);
				Layout::CSplitter *pSplitter=
					dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
				bool fSwap=pSplitter->IDToIndex(CONTAINER_ID_SIDEBAR)!=Pane;
				pSplitter->SetStyle(
					(fVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT) |
					Layout::CSplitter::STYLE_FIXED,
					!fSwap);
				if (fSwap)
					pSplitter->SwapPane();
			}
		}
		return;

	case CM_SIDEBAROPTIONS:
		if (::IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd,COptionDialog::PAGE_SIDEBAR);
		return;

	case CM_DRIVER_BROWSE:
		{
			OPENFILENAME ofn;
			TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
			CFilePath FilePath;

			FilePath.SetPath(CoreEngine.GetDriverFileName());
			if (FilePath.GetDirectory(szInitDir)) {
				::lstrcpy(szFileName,FilePath.GetFileName());
			} else {
				GetAppClass().GetAppDirectory(szInitDir);
				szFileName[0]='\0';
			}
			InitOpenFileName(&ofn);
			ofn.hwndOwner=GetVideoHostWindow();
			ofn.lpstrFilter=
				TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
				TEXT("すべてのファイル\0*.*\0");
			ofn.lpstrFile=szFileName;
			ofn.nMaxFile=lengthof(szFileName);
			ofn.lpstrInitialDir=szInitDir;
			ofn.lpstrTitle=TEXT("BonDriverの選択");
			ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
			if (::GetOpenFileName(&ofn)) {
				AppMain.SetDriver(szFileName);
			}
		}
		return;

	case CM_CHANNELHISTORY_CLEAR:
		RecentChannelList.Clear();
		return;

	case CM_PANEL_INFORMATION:
	case CM_PANEL_PROGRAMLIST:
	case CM_PANEL_CHANNEL:
	case CM_PANEL_CONTROL:
	case CM_PANEL_CAPTION:
		PanelForm.SetCurPageByID(id-CM_PANEL_FIRST);
		return;

	case CM_CHANNELPANEL_UPDATE:
		ChannelPanel.UpdateChannelList();
		return;

	case CM_CHANNELPANEL_DETAILPOPUP:
		ChannelPanel.SetDetailToolTip(!ChannelPanel.GetDetailToolTip());
		return;

	case CM_CHANNELPANEL_EVENTS_1:
	case CM_CHANNELPANEL_EVENTS_2:
	case CM_CHANNELPANEL_EVENTS_3:
	case CM_CHANNELPANEL_EVENTS_4:
		ChannelPanel.SetEventsPerChannel(id-CM_CHANNELPANEL_EVENTS_1+1);
		return;

	default:
		if (id>=CM_AUDIOSTREAM_FIRST && id<=CM_AUDIOSTREAM_LAST) {
			m_pCore->SetAudioStream(id-CM_AUDIOSTREAM_FIRST);
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
				OnChannelChanged(false);
				PluginList.SendChannelChangeEvent();
				return;
			} else {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();
				if (pList==NULL)
					return;

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
		// 上から続いているため、ここに別なコードを入れてはいけないので注意
		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			int Channel=id-CM_CHANNEL_FIRST;

			const CChannelList *pChList=ChannelManager.GetCurrentRealChannelList();
			if (pChList==NULL)
				return;
			const CChannelInfo *pChInfo=pChList->GetChannelInfo(Channel);
			if (pChInfo==NULL)
				return;
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
					return;
			}
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

			if (Space!=ChannelManager.GetCurrentSpace()) {
				const CChannelList *pChannelList=ChannelManager.GetChannelList(Space);
				if (pChannelList!=NULL) {
					for (int i=0;i<pChannelList->NumChannels();i++) {
						if (pChannelList->IsEnabled(i)) {
							AppMain.SetChannel(Space,i);
							return;
						}
					}
				}
			}
			return;
		}

		if (id>=CM_DRIVER_FIRST && id<=CM_DRIVER_LAST) {
			int Driver=id-CM_DRIVER_FIRST;

			if (Driver<DriverManager.NumDrivers()) {
				const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(Driver);

				if (!CoreEngine.IsDriverOpen()
						|| ::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())!=0) {
					if (AppMain.SetDriver(pDriverInfo->GetFileName())) {
						AppMain.RestoreChannel();
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
			m_pCore->ProcessTunerMenu(id);
			return;
		}

		if (id>=CM_CHANNELHISTORY_FIRST && id<=CM_CHANNELHISTORY_LAST) {
			const CRecentChannelList::CChannel *pChannel=
				RecentChannelList.GetChannelInfo(id-CM_CHANNELHISTORY_FIRST);

			if (pChannel!=NULL)
				AppMain.SetDriverAndChannel(pChannel->GetDriverFileName(),pChannel);
			return;
		}

		if (id>=CM_PLUGINCOMMAND_FIRST && id<=CM_PLUGINCOMMAND_LAST) {
			PluginList.OnPluginCommand(CommandList.GetCommandText(CommandList.IDToIndex(id)));
			return;
		}
	}
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
				InfoPanel.SetVideoSize(CoreEngine.GetOriginalVideoWidth(),
										CoreEngine.GetOriginalVideoHeight(),
										CoreEngine.GetDisplayVideoWidth(),
										CoreEngine.GetDisplayVideoHeight());
				ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
			}

			if ((UpdateStatus&(CCoreEngine::STATUS_AUDIOCHANNELS
							 | CCoreEngine::STATUS_AUDIOSTREAMS
							 | CCoreEngine::STATUS_AUDIOCOMPONENTTYPE))!=0) {
				TRACE(TEXT("Audio status changed.\n"));
				AutoSelectStereoMode();
				StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
				ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
			}

			if ((UpdateStatus&CCoreEngine::STATUS_EVENTID)!=0) {
				if (RecordManager.GetStopOnEventEnd())
					AppMain.StopRecord();

				SetTitleText(true);

				if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_EVENTNAME)) {
					TCHAR szText[256];

					if (CoreEngine.m_DtvEngine.GetEventName(szText,lengthof(szText))>0) {
						TCHAR szBarText[16+lengthof(szText)];
						SYSTEMTIME stStart,stEnd;

						if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,&stEnd)) {
							::wsprintf(szBarText,TEXT("%d:%02d〜%d:%02d "),
									   stStart.wHour,stStart.wMinute,
									   stEnd.wHour,stEnd.wMinute);
						} else {
							szBarText[0]='\0';
						}
						::lstrcat(szBarText,szText);
						ShowNotificationBar(szBarText);
					}
				}

				ProgramListPanel.SetCurrentEventID(CoreEngine.m_DtvEngine.GetEventID());

				//CoreEngine.UpdateEpgDataInfo();
				CProgramInfoStatusItem *pProgramInfoItem=
					dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
				if (pProgramInfoItem!=NULL) {
					pProgramInfoItem->UpdateContent();
					pProgramInfoItem->Update();
				}

				if (ViewOptions.GetResetPanScanEventChange()
						&& m_AspectRatioType!=ASPECTRATIO_DEFAULT) {
					CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(0,0);
					if (!m_pCore->GetFullscreen() && !::IsZoomed(hwnd)
							&& IsViewerEnabled()) {
						int Width,Height;

						if (CoreEngine.GetVideoViewSize(&Width,&Height)) {
							SIZE sz;
							m_Viewer.GetVideoContainer().GetClientSize(&sz);
							if (sz.cx<Width*sz.cy/Height)
								AdjustWindowSize(Width*sz.cy/Height,sz.cy);
						}
						// この時点でまだ新しい映像サイズが取得できない場合があるため、
						// WM_APP_VIDEOSIZECHANGED が来た時に調整するようにする
						m_AspectRatioResetTime=::GetTickCount();
					}
					m_AspectRatioType=ASPECTRATIO_DEFAULT;
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
					ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
					/*
					MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
											CM_ASPECTRATIO_DEFAULT);
					*/
					AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_DEFAULT);
				}

				m_CurEventStereoMode=-1;
				AutoSelectStereoMode();
			}

			if (TimerCount%(10000/UPDATE_TIMER_INTERVAL)==0) {
				CProgramInfoStatusItem *pProgramInfoItem=
					dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
				if (pProgramInfoItem!=NULL) {
					if (pProgramInfoItem->UpdateContent())
						pProgramInfoItem->Update();
				}
			}

			if (RecordManager.IsRecording()) {
				if (RecordManager.QueryStop()) {
					AppMain.StopRecord();
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

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
								 | CCoreEngine::STATISTIC_BITRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_SIGNALLEVEL);

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_STREAMREMAIN
								 | CCoreEngine::STATISTIC_PACKETBUFFERRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_BUFFERING);

			StatusView.UpdateItem(STATUS_ITEM_CLOCK);
			TotTimeAdjuster.AdjustTime();

			StatusView.UpdateItem(STATUS_ITEM_MEDIABITRATE);

			if (fShowPanelWindow && PanelForm.GetVisible()) {
				// パネルの更新
				if (PanelForm.GetCurPageID()==PANEL_ID_INFORMATION) {
					// 情報タブ更新
					BYTE AspectX,AspectY;
					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
						InfoPanel.SetAspectRatio(AspectX,AspectY);

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
										 | CCoreEngine::STATISTIC_BITRATE))!=0) {
						InfoPanel.SetBitRate(CoreEngine.GetBitRate());
						if (InfoPanel.IsSignalLevelEnabled())
							InfoPanel.SetSignalLevel(CoreEngine.GetSignalLevel());
					}

					InfoPanel.SetMediaBitRate(
						CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoBitRate(),
						CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioBitRate());

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
						InfoPanel.UpdateErrorCount();
					}

					if (RecordManager.IsRecording()) {
						const CRecordTask *pRecordTask=RecordManager.GetRecordTask();
						const LONGLONG FreeSpace=pRecordTask->GetFreeSpace();

						InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
							pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime(),
							FreeSpace<0?0:FreeSpace);
					}

					if (TimerCount%(2000/UPDATE_TIMER_INTERVAL)==0)	// 負荷軽減
						UpdateProgramInfo();
				} else if (PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
					// チャンネルタブ更新
					if (!EpgLoadEventHandler.IsEpgFileLoading()
							&& ChannelPanel.QueryUpdate())
						ChannelPanel.UpdateChannelList();
				}
			}

			// 空き容量が少ない場合の注意表示
			if (RecordOptions.GetAlertLowFreeSpace()
					&& !m_fAlertedLowFreeSpace
					&& RecordManager.IsRecording()) {
				LONGLONG FreeSpace=RecordManager.GetRecordTask()->GetFreeSpace();

				if (FreeSpace>=0
						&& (ULONGLONG)FreeSpace<=RecordOptions.GetLowFreeSpaceThresholdBytes()) {
					NotifyBalloonTip.Show(
						APP_NAME TEXT("の録画ファイルの保存先の空き容量が少なくなっています。"),
						TEXT("空き容量が少なくなっています。"),
						NULL,CBalloonTip::ICON_WARNING);
					::SetTimer(m_hwnd,TIMER_ID_HIDETOOLTIP,10000,NULL);
					ShowNotificationBar(
						TEXT("録画ファイルの保存先の空き容量が少なくなっています"),
						CNotificationBar::MESSAGE_WARNING,6000);
					m_fAlertedLowFreeSpace=true;
				}
			}

			TimerCount++;
		}
		break;

	case TIMER_ID_OSD:
		// OSD を消す
		OSDManager.ClearOSD();
		::KillTimer(hwnd,TIMER_ID_OSD);
		break;

	case TIMER_ID_DISPLAY:
		// モニタがオフにならないようにする
		::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		break;

	case TIMER_ID_WHEELCHANNELCHANGE:
		// ホイールでのチャンネル変更
		{
			const CChannelInfo *pInfo=ChannelManager.GetChangingChannelInfo();

			SetWheelChannelChanging(false);
			ChannelManager.SetChangingChannel(-1);
			if (pInfo!=NULL) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();

				if (pList->HasRemoteControlKeyID())
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelIndex());
			}
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		// サービスとロゴを関連付ける
		if (m_ProgramListUpdateTimerCount==0) {
			CTsAnalyzer *pAnalyzer=&CoreEngine.m_DtvEngine.m_TsAnalyzer;
			const WORD NetworkID=pAnalyzer->GetNetworkID();
			if (NetworkID!=0) {
				CTsAnalyzer::ServiceList ServiceList;
				if (pAnalyzer->GetServiceList(&ServiceList)) {
					for (size_t i=0;i<ServiceList.size();i++) {
						const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
						const WORD LogoID=pServiceInfo->LogoID;
						if (LogoID!=0xFFFF)
							LogoManager.AssociateLogoID(NetworkID,pServiceInfo->ServiceID,LogoID);
					}
				}
			}
		}

		// EPG情報の同期
		if (EpgLoadEventHandler.IsEpgFileLoading()
				|| EpgOptions.IsEDCBDataLoading())
			break;
		{
			const HANDLE hThread=::GetCurrentThread();
			int OldPriority=::GetThreadPriority(hThread);
			::SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);
			if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_PROGRAMLIST) {
				int ServiceID=ChannelManager.GetCurrentServiceID();

				if (ServiceID<=0) {
					WORD SID;
					if (CoreEngine.m_DtvEngine.GetServiceID(&SID))
						ServiceID=SID;
				}
				if (ServiceID>0)
					ProgramListPanel.UpdateProgramList(
						CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),
						(WORD)ServiceID);
			} else if ((fShowPanelWindow
						&& PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
					|| m_ProgramListUpdateTimerCount>=1) {
				const CChannelInfo *pChannelInfo=ChannelManager.GetCurrentChannelInfo();

				if (pChannelInfo!=NULL) {
					EpgProgramList.UpdateService(
						pChannelInfo->GetTransportStreamID(),
						pChannelInfo->GetServiceID());
					if (fShowPanelWindow
							&& PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
						ChannelPanel.UpdateChannel(ChannelManager.GetCurrentChannel());
				}
			}
			::SetThreadPriority(hThread,OldPriority);
		}
		m_ProgramListUpdateTimerCount++;
		// 更新頻度を下げる
		if (m_ProgramListUpdateTimerCount>=6 && m_ProgramListUpdateTimerCount<=10)
			::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,(m_ProgramListUpdateTimerCount-5)*(60*1000),NULL);
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		// 番組表の取得
		if (!m_pCore->GetStandby())
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
		{
			const CChannelList *pList=ChannelManager.GetCurrentRealChannelList();
			int i;

			for (i=ChannelManager.GetCurrentChannel()+1;i<pList->NumChannels();i++) {
				const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

				if (pChInfo->IsEnabled()) {
					int j;

					for (j=0;j<i;j++) {
						const CChannelInfo *pInfo=pList->GetChannelInfo(j);
						if (pInfo->IsEnabled()
								&& pInfo->GetChannelIndex()==pChInfo->GetChannelIndex())
							break;
					}
					if (j==i)
						break;
				}
			}
			if (i==pList->NumChannels()) {
				ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
			} else {
				// TODO: 現在の取得状況に応じて待ち時間を変える
				AppMain.SetChannel(ChannelManager.GetCurrentSpace(),i);
			}
		}
		break;

	case TIMER_ID_VIDEOSIZECHANGED:
		// 映像サイズの変化に合わせる
		{
			RECT rc;

			m_Viewer.GetVideoContainer().GetClientRect(&rc);
			m_Viewer.GetVideoContainer().SendMessage(WM_SIZE,0,
													 MAKELPARAM(rc.right,rc.bottom));
			StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
			ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
			if (m_VideoSizeChangedTimerCount==1)
				::KillTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED);
			else
				m_VideoSizeChangedTimerCount++;
		}
		break;

	case TIMER_ID_RESETERRORCOUNT:
		// エラーカウントをリセットする
		// (既にサービスの情報が取得されている場合のみ)
		if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceNum()>0) {
			SendCommand(CM_RESETERRORCOUNT);
			m_ResetErrorCountTimer.End();
		}
		break;

	case TIMER_ID_HIDETOOLTIP:
		// ツールチップを非表示にする
		NotifyBalloonTip.Hide();
		::KillTimer(hwnd,TIMER_ID_HIDETOOLTIP);
		break;
	}
}


bool CMainWindow::UpdateProgramInfo()
{
	const bool fNext=InfoPanel.GetProgramInfoNext();
	TCHAR szText[2048];
	int Length=0;

	if (fNext)
		Length=::wsprintf(szText,TEXT("次 : "));

	SYSTEMTIME stStart,stEnd;
	if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,NULL,fNext)) {
		Length+=::wsprintf(&szText[Length],TEXT("%d/%d/%d(%s) %d:%02d〜"),
						   stStart.wYear,
						   stStart.wMonth,
						   stStart.wDay,
						   GetDayOfWeekText(stStart.wDayOfWeek),
						   stStart.wHour,
			stStart.wMinute);
		if (CoreEngine.m_DtvEngine.GetEventTime(NULL,&stEnd,fNext))
			Length+=::wsprintf(&szText[Length],TEXT("%d:%02d\r\n"),
							   stEnd.wHour,stEnd.wMinute);
		else
			Length+=::wsprintf(&szText[Length],TEXT("(終了未定)\r\n"));
	}
	Length+=CoreEngine.m_DtvEngine.GetEventName(
								&szText[Length],lengthof(szText)-Length,fNext);
	Length+=::wsprintf(&szText[Length],TEXT("\r\n\r\n"));
	Length+=CoreEngine.m_DtvEngine.GetEventText(
								&szText[Length],lengthof(szText)-Length,fNext);
	Length+=::wsprintf(&szText[Length],TEXT("\r\n\r\n"));
	Length+=CoreEngine.m_DtvEngine.GetEventExtendedText(
								&szText[Length],lengthof(szText)-Length,fNext);

	CTsAnalyzer::EventSeriesInfo SeriesInfo;
	if (CoreEngine.m_DtvEngine.GetEventSeriesInfo(&SeriesInfo,fNext)
			&& SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0) {
		Length+=::wsprintf(&szText[Length],TEXT("\r\n\r\n(シリーズ"));
		if (SeriesInfo.RepeatLabel!=0)
			Length+=::wsprintf(&szText[Length],TEXT(" [再]"));
		if (SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0)
			Length+=::wsprintf(&szText[Length],TEXT(" 第%d回 / 全%d回"),
							   SeriesInfo.EpisodeNumber,SeriesInfo.LastEpisodeNumber);
		// expire_date は実際の最終回の日時でないので、紛らわしいため表示しない
		/*
		if (SeriesInfo.bIsExpireDateValid)
			Length+=::wsprintf(&szText[Length],TEXT(" 終了予定%d/%d/%d"),
							   SeriesInfo.ExpireDate.wYear,SeriesInfo.ExpireDate.wMonth,SeriesInfo.ExpireDate.wDay);
		*/
		::lstrcpy(&szText[Length],TEXT(")"));
	}

	InfoPanel.SetProgramInfo(szText);
	return true;
}


bool CMainWindow::OnInitMenuPopup(HMENU hmenu)
{
	if (hmenu==MainMenu.GetMenuHandle()) {
		bool fFullscreen=m_pCore->GetFullscreen();
		MainMenu.CheckItem(CM_PANEL,
						   fFullscreen?m_Fullscreen.IsPanelVisible():fShowPanelWindow);
		MainMenu.CheckItem(CM_FRAMECUT,
						   CoreEngine.m_DtvEngine.m_MediaViewer.GetViewStretchMode()==CMediaViewer::STRETCH_CUTFRAME);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_ZOOM)) {
		CZoomOptions::ZoomRate Zoom;
		GetZoomRate(&Zoom.Rate,&Zoom.Factor);
		ZoomOptions.SetMenu(hmenu,&Zoom);
		Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE)) {
		m_pCore->InitTunerMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
		PluginList.SetMenu(hmenu);
		Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
		RecentChannelList.SetMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO)) {
		if (!AspectRatioIconMenu.OnInitMenuPopup(m_hwnd,hmenu))
			return false;
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL)) {
		m_pCore->InitChannelMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE)) {
		CTsAnalyzer::ServiceList ServiceList;
		WORD CurServiceID;
		int CurService=-1;

		CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceList(&ServiceList);
		if (!CoreEngine.m_DtvEngine.GetServiceID(&CurServiceID))
			CurServiceID=0;
		ClearMenu(hmenu);
		for (size_t i=0;i<ServiceList.size();i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
			TCHAR szText[512],szEventName[256];

			if (pServiceInfo->szServiceName[0]!='\0') {
				::wsprintf(szText,TEXT("&%d: %s"),i+1,pServiceInfo->szServiceName);
			} else {
				::wsprintf(szText,TEXT("&%d: サービス%d"),i+1,i+1);
			}
			if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetEventName(
					CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(pServiceInfo->ServiceID),
					szEventName,lengthof(szEventName))>0) {
				::lstrcat(szText,TEXT(" ("));
				int Length=::lstrlen(szText);
				CopyToMenuText(szEventName,szText+Length,lengthof(szText)-1-Length);
				::lstrcat(szText,TEXT(")"));
			}
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_SERVICE_FIRST+i,szText);
			if (pServiceInfo->ServiceID==CurServiceID)
				CurService=(int)i;
		}
		if (CurService>=0)
			MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
									CM_SERVICE_FIRST+(int)ServiceList.size()-1,
									CM_SERVICE_FIRST+CurService);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_AUDIO)) {
		CTsAnalyzer::EventAudioInfo AudioInfo;

		ClearMenu(hmenu);
		if (CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)
				&& AudioInfo.ComponentType==0x02) {
			// Dual mono
			TCHAR szText[80],szAudio1[64],szAudio2[64];

			szAudio1[0]='\0';
			szAudio2[0]='\0';
			if (AudioInfo.szText[0]!='\0') {
				LPTSTR pszDelimiter=::StrChr(AudioInfo.szText,'\r');
				if (pszDelimiter!=NULL) {
					*pszDelimiter='\0';
					::lstrcpyn(szAudio1,AudioInfo.szText,lengthof(szAudio1));
					::lstrcpyn(szAudio2,pszDelimiter+1,lengthof(szAudio2));
				}
			}
			// ES multilingual flag が立っているのに両方日本語の場合がある…
			if (AudioInfo.bESMultiLingualFlag
					&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
				// 2カ国語
				LPCTSTR pszLang1=szAudio1[0]!='\0'?szAudio1:GetLanguageText(AudioInfo.LanguageCode,LANGUAGE_TEXT_LONG);
				LPCTSTR pszLang2=szAudio2[0]!='\0'?szAudio2:GetLanguageText(AudioInfo.LanguageCode2,LANGUAGE_TEXT_LONG);
				::wsprintf(szText,TEXT("%s+%s(&S)"),pszLang1,pszLang2);
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_THROUGH,szText);
				::wsprintf(szText,TEXT("%s(&L)"),pszLang1);
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_LEFT,szText);
				::wsprintf(szText,TEXT("%s(&R)"),pszLang2);
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_RIGHT,szText);
			} else {
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_THROUGH,TEXT("主+副音声(&S)"));
				if (szAudio1[0]!='\0')
					::wsprintf(szText,TEXT("主音声(%s)(&L)"),szAudio1);
				else
					::lstrcpy(szText,TEXT("主音声(&L)"));
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_LEFT,szText);
				if (szAudio2[0]!='\0')
					::wsprintf(szText,TEXT("副音声(%s)(&R)"),szAudio2);
				else
					::lstrcpy(szText,TEXT("副音声(&R)"));
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_RIGHT,szText);
			}
		} else {
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_THROUGH,TEXT("ステレオ/スルー(&S)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_LEFT,TEXT("左(主音声)(&L)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_STEREO_RIGHT,TEXT("右(副音声)(&R)"));
		}
		::CheckMenuRadioItem(hmenu,CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							 CM_STEREO_THROUGH+m_pCore->GetStereoMode(),MF_BYCOMMAND);
		::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
		const int NumAudioStreams=m_pCore->GetNumAudioStreams();
		if (NumAudioStreams>0) {
			for (int i=0;i<NumAudioStreams;i++) {
				TCHAR szText[64];
				int Length;

				Length=::wsprintf(szText,TEXT("&%d: 音声%d"),i+1,i+1);
				if (NumAudioStreams>1
						&& CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo,i)) {
					if (AudioInfo.szText[0]!='\0') {
						LPTSTR p=::StrChr(AudioInfo.szText,'\r');
						if (p!=NULL)
							*p='/';
						::wsprintf(szText+Length,TEXT(" (%s)"),AudioInfo.szText);
					} else {
						::wsprintf(szText+Length,TEXT(" (%s)"),GetLanguageText(AudioInfo.LanguageCode,LANGUAGE_TEXT_LONG));
					}
				}
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_AUDIOSTREAM_FIRST+i,szText);
			}
			MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
									CM_AUDIOSTREAM_FIRST+NumAudioStreams-1,
									CM_AUDIOSTREAM_FIRST+m_pCore->GetAudioStream());
		}
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_FILTERPROPERTY)) {
		for (int i=0;i<lengthof(g_DirectShowFilterPropertyList);i++) {
			MainMenu.EnableItem(g_DirectShowFilterPropertyList[i].Command,
				CoreEngine.m_DtvEngine.m_MediaViewer.FilterHasProperty(
									g_DirectShowFilterPropertyList[i].Filter));
		}
	} else {
		if (ChannelMenuInitPopup(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE),hmenu))
			return true;

		if (TunerSelectMenu.OnInitMenuPopup(hmenu))
			return true;

		return false;
	}
	return true;
}


void CMainWindow::OnTunerChanged()
{
	SetWheelChannelChanging(false);
	if (m_fSrcFilterReleased && CoreEngine.IsDriverOpen())
		m_fSrcFilterReleased=false;
	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(false);
	ProgramListPanel.ClearProgramList();
	InfoPanel.ResetStatistics();
	bool fNoSignalLevel=DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName());
	InfoPanel.ShowSignalLevel(!fNoSignalLevel);
	CSignalLevelStatusItem *pItem=dynamic_cast<CSignalLevelStatusItem*>(
							StatusView.GetItemByID(STATUS_ITEM_SIGNALLEVEL));
	if (pItem!=NULL)
		pItem->ShowSignalLevel(!fNoSignalLevel);
	/*
	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	else
		ChannelPanel.ClearChannelList();
	*/
	CaptionPanel.Clear();
	ProgramGuide.ClearCurrentService();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	SetTitleText(false);
	m_ResetErrorCountTimer.End();
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
}


void CMainWindow::OnTunerOpened()
{
	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(false);
	m_fSrcFilterReleased=false;
}


void CMainWindow::OnTunerClosed()
{
	m_fSrcFilterReleased=true;
}


void CMainWindow::OnChannelListChanged()
{
	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	else
		ChannelPanel.ClearChannelList();
}


void CMainWindow::OnChannelChanged(bool fSpaceChanged)
{
	SetWheelChannelChanging(false);
	SetTitleText(false);
	MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
							CM_SPACE_FIRST+ChannelManager.GetCurrentSpace());
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	if (OSDOptions.GetShowOSD())
		ShowChannelOSD();
	ProgramListPanel.ClearProgramList();
	::SetTimer(m_hwnd,TIMER_ID_PROGRAMLISTUPDATE,10000,NULL);
	m_ProgramListUpdateTimerCount=0;
	InfoPanel.ResetStatistics();
	if (fSpaceChanged) {
		if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
			ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
		else
			ChannelPanel.ClearChannelList();
	} else {
		if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
			ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	}
	const CChannelInfo *pInfo=ChannelManager.GetCurrentChannelInfo();
	if (pInfo!=NULL) {
		ProgramGuide.SetCurrentService(pInfo->GetNetworkID(),
									   pInfo->GetTransportStreamID(),
									   pInfo->GetServiceID());
	} else {
		ProgramGuide.ClearCurrentService();
	}
	CaptionPanel.Clear();
	UpdateControlPanelStatus();

	LPCTSTR pszDriverFileName=CoreEngine.GetDriverFileName();
	pInfo=ChannelManager.GetCurrentRealChannelInfo();
	RecentChannelList.Add(pszDriverFileName,pInfo);
	ChannelHistory.SetCurrentChannel(pszDriverFileName,pInfo);
	if (DriverOptions.IsResetChannelChangeErrorCount(pszDriverFileName))
		m_ResetErrorCountTimer.Begin(m_hwnd,5000);
	else
		m_ResetErrorCountTimer.End();
	/*
	m_pCore->SetStereoMode(0);
	m_CurEventStereoMode=-1;
	*/
}


void CMainWindow::ShowChannelOSD()
{
	const CChannelInfo *pInfo;

	if (m_fWheelChannelChanging)
		pInfo=ChannelManager.GetChangingChannelInfo();
	else
		pInfo=ChannelManager.GetCurrentChannelInfo();
	if (pInfo!=NULL)
		OSDManager.ShowChannelOSD(pInfo,m_fWheelChannelChanging);
}


void CMainWindow::OnServiceChanged()
{
	int CurService=0;
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		CurService=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
	MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
							CM_SERVICE_FIRST+CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceNum()-1,
							CM_SERVICE_FIRST+CurService);
	if (PanelForm.GetCurPageID()==PANEL_ID_INFORMATION)
		UpdateProgramInfo();
}


void CMainWindow::OnRecordingStarted()
{
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	StatusView.UpdateItem(STATUS_ITEM_ERROR);
	//MainMenu.EnableItem(CM_RECORDOPTION,false);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
	TaskbarManager.SetRecordingStatus(true);
	SetTitleText(true);
	m_ResetErrorCountTimer.End();
	m_fAlertedLowFreeSpace=false;
}


void CMainWindow::OnRecordingStopped()
{
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	InfoPanel.SetRecordStatus(false);
	//MainMenu.EnableItem(CM_RECORDOPTION,true);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,false);
	TaskbarManager.SetRecordingStatus(false);
	SetTitleText(true);
	RecordManager.SetStopOnEventEnd(false);
	if (m_pCore->GetStandby())
		AppMain.CloseTuner();
	if (m_fExitOnRecordingStop)
		PostCommand(CM_EXIT);
}


void CMainWindow::OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz)
{
	POINT pt;
	pt.x=GET_X_LPARAM(lParam);
	pt.y=GET_Y_LPARAM(lParam);

	if (m_Viewer.GetDisplayBase().IsVisible()) {
		RECT rc;

		m_Viewer.GetDisplayBase().GetParent()->GetScreenPosition(&rc);
		if (::PtInRect(&rc,pt)) {
			m_Viewer.GetDisplayBase().GetDisplayView()->SendMessage(
							fHorz?WM_MOUSEHWHEEL:WM_MOUSEWHEEL,wParam,lParam);
			return;
		}
	}

	int Mode;
	const DWORD CurTime=::GetTickCount();
	bool fProcessed=true;

	if (fHorz) {
		Mode=OperationOptions.GetWheelTiltMode();
	} else {
		if ((wParam&MK_SHIFT)!=0)
			Mode=OperationOptions.GetWheelShiftMode();
		else if ((wParam&MK_CONTROL)!=0)
			Mode=OperationOptions.GetWheelCtrlMode();
		else
			Mode=OperationOptions.GetWheelMode();
	}
	if (StatusView.GetVisible()) {
		RECT rc;

		StatusView.GetScreenPosition(&rc);
		if (::PtInRect(&rc,pt)) {
			switch (StatusView.GetCurItem()) {
			case STATUS_ITEM_CHANNEL:
				Mode=COperationOptions::WHEEL_CHANNEL;
				break;
			/*
			case STATUS_ITEM_VIDEOSIZE:
				Mode=COperationOptions::WHEEL_ZOOM;
				break;
			*/
			case STATUS_ITEM_VOLUME:
				Mode=COperationOptions::WHEEL_VOLUME;
				break;
			case STATUS_ITEM_AUDIOCHANNEL:
				Mode=COperationOptions::WHEEL_AUDIO;
				break;
			}
		}
	}
	if (Mode!=m_PrevWheelMode)
		m_WheelCount=0;
	else
		m_WheelCount++;
	switch (Mode) {
	case COperationOptions::WHEEL_VOLUME:
		SendCommand(GET_WHEEL_DELTA_WPARAM(wParam)>=0?CM_VOLUME_UP:CM_VOLUME_DOWN);
		break;

	case COperationOptions::WHEEL_CHANNEL:
		{
			bool fUp;
			const CChannelInfo *pInfo;

			if (fHorz || OperationOptions.GetWheelChannelReverse())
				fUp=GET_WHEEL_DELTA_WPARAM(wParam)>0;
			else
				fUp=GET_WHEEL_DELTA_WPARAM(wParam)<0;
			pInfo=ChannelManager.GetNextChannelInfo(fUp);
			if (pInfo!=NULL) {
				if (m_fWheelChannelChanging) {
					if (m_WheelCount<5 && DiffTime(m_PrevWheelTime,CurTime)<(5UL-m_WheelCount)*100UL) {
						fProcessed=false;
						break;
					}
				}
				SetWheelChannelChanging(true,OperationOptions.GetWheelChannelDelay());
				ChannelManager.SetChangingChannel(ChannelManager.FindChannelInfo(pInfo));
				StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (OSDOptions.GetShowOSD())
					ShowChannelOSD();
			}
		}
		break;

	case COperationOptions::WHEEL_AUDIO:
		if (Mode==m_PrevWheelMode && DiffTime(m_PrevWheelTime,CurTime)<300) {
			fProcessed=false;
			break;
		}
		SendCommand(CM_SWITCHAUDIO);
		break;

	case COperationOptions::WHEEL_ZOOM:
		if (Mode==m_PrevWheelMode && DiffTime(m_PrevWheelTime,CurTime)<500) {
			fProcessed=false;
			break;
		}
		if (!IsZoomed(m_hwnd) && !m_pCore->GetFullscreen()) {
			int Zoom;

			Zoom=GetZoomPercentage();
			if (GET_WHEEL_DELTA_WPARAM(wParam)>=0)
				Zoom+=OperationOptions.GetWheelZoomStep();
			else
				Zoom-=OperationOptions.GetWheelZoomStep();
			SetZoomRate(Zoom,100);
		}
		break;

	case COperationOptions::WHEEL_ASPECTRATIO:
		if (Mode==m_PrevWheelMode && DiffTime(m_PrevWheelTime,CurTime)<300) {
			fProcessed=false;
			break;
		}
		SendCommand(CM_ASPECTRATIO);
		break;
	}

	m_PrevWheelMode=Mode;
	if (fProcessed)
		m_PrevWheelTime=CurTime;
}


bool CMainWindow::EnableViewer(bool fEnable)
{
	if (fEnable && !CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
		CoreEngine.m_DtvEngine.SetTracer(&StatusView);
		bool fOK=InitializeViewer();
		CoreEngine.m_DtvEngine.SetTracer(NULL);
		StatusView.SetSingleText(NULL);
		if (!fOK)
			return false;
	}
	if (!m_Viewer.EnableViewer(fEnable))
		return false;
	MainMenu.CheckItem(CM_DISABLEVIEWER,!fEnable);
	m_pCore->PreventDisplaySave(fEnable);
	return true;
}


bool CMainWindow::IsViewerEnabled() const
{
	return m_Viewer.IsViewerEnabled();
}


void CMainWindow::OnVolumeChanged(bool fOSD)
{
	const int Volume=m_pCore->GetVolume();

	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,false);
	if (fOSD && OSDOptions.GetShowOSD())
		OSDManager.ShowVolumeOSD(Volume);
}


void CMainWindow::OnMuteChanged()
{
	const bool fMute=m_pCore->GetMute();

	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,fMute);
}


void CMainWindow::OnStereoModeChanged()
{
	const int StereoMode=m_pCore->GetStereoMode();

	m_CurEventStereoMode=StereoMode;
	/*
	MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+StereoMode);
	*/
	StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnAudioStreamChanged()
{
	const int Stream=m_pCore->GetAudioStream();

	MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
							CM_AUDIOSTREAM_FIRST+m_pCore->GetNumAudioStreams()-1,
							CM_AUDIOSTREAM_FIRST+Stream);
	StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::AutoSelectStereoMode()
{
	/*
		Dual Mono 時に音声を自動で選択する
		一つの番組で本編Dual Mono/CMステレオのような場合
		  Aパート -> CM       -> Bパート
		  副音声  -> ステレオ -> 副音声
		のように、ユーザーの選択を記憶しておく必要がある
	*/
	const bool fDualMono=CoreEngine.m_DtvEngine.GetAudioChannelNum()==
										CMediaViewer::AUDIO_CHANNEL_DUALMONO
					/*|| CoreEngine.m_DtvEngine.GetAudioComponentType()==0x02*/;

	if (m_CurEventStereoMode<0) {
		m_pCore->SetStereoMode(fDualMono?1:0);
		m_CurEventStereoMode=-1;
	} else if (m_CurEventStereoMode>0) {
		int OldStereoMode=m_CurEventStereoMode;
		m_pCore->SetStereoMode(fDualMono?m_CurEventStereoMode:0);
		m_CurEventStereoMode=OldStereoMode;
	}
}


bool CMainWindow::SetZoomRate(int Rate,int Factor)
{
	int Width,Height;

	if (Rate<1 || Factor<1)
		return false;
	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0)
		AdjustWindowSize((Width*Rate+Factor/2)/Factor,(Height*Rate+Factor/2)/Factor);
	return true;
}


bool CMainWindow::GetZoomRate(int *pRate,int *pFactor)
{
	bool fOK=false;
	int Width,Height;
	int Rate=0,Factor=1;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		/*
		SIZE sz;

		m_Viewer.GetVideoContainer().GetClientSize(&sz);
		Rate=sz.cy;
		Factor=Height;
		*/
		WORD DstWidth,DstHeight;
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight)) {
			Rate=DstHeight;
			Factor=Height;
		}
		fOK=true;
	}
	if (pRate)
		*pRate=Rate;
	if (pFactor)
		*pFactor=Factor;
	return fOK;
}


int CMainWindow::GetZoomPercentage()
{
	int Rate,Factor;

	if (!GetZoomRate(&Rate,&Factor) || Factor==0)
		return 0;
	return (Rate*100+Factor/2)/Factor;
}


DWORD WINAPI CMainWindow::ExitWatchThread(LPVOID lpParameter)
{
	HANDLE hEvent=lpParameter;

	if (::WaitForSingleObject(hEvent,60000)!=WAIT_OBJECT_0) {
		Logger.AddLog(TEXT("終了処理がタイムアウトしました。プロセスを強制的に終了させます。"));
		::ExitProcess(-1);
	}
	return 0;
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	if (uMsg==WM_CREATE) {
		CMainWindow *pThis=static_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd,lParam));

		if (!pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam)))
			return -1;
		return 0;
	}

	CMainWindow *pThis=static_cast<CMainWindow*>(GetBasicWindow(hwnd));
	if (pThis==NULL) {
		if (uMsg==WM_NCCALCSIZE && m_fThinFrameCreate) {
			if (wParam!=0) {
				NCCALCSIZE_PARAMS *pnccsp=reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

				::InflateRect(&pnccsp->rgrc[0],-m_ThinFrameWidth,-m_ThinFrameWidth);
			}
			return 0;
		}

		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	LRESULT Result=0;
	if (PluginList.OnMessage(hwnd,uMsg,wParam,lParam,&Result))
		return Result;

	if (uMsg==WM_DESTROY) {
		pThis->OnMessage(hwnd,uMsg,wParam,lParam);
		pThis->OnDestroy();
		::PostQuitMessage(0);
		return 0;
	}

	return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
}


void CMainWindow::SetTitleText(bool fEvent)
{
	const CChannelInfo *pInfo;
	TCHAR szText[256];
	LPCTSTR pszText;

	if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		int Length=::wsprintf(szText,MAIN_TITLE_TEXT TEXT(" %s %s"),
							  RecordManager.IsRecording()?TEXT("●"):TEXT("-"),
							  pInfo->GetName());
		if (fEvent) {
			TCHAR szEvent[256];

			if (CoreEngine.m_DtvEngine.GetEventName(szEvent,lengthof(szEvent))>0) {
				Length+=::wnsprintf(&szText[Length],lengthof(szText)-1-Length,
									TEXT(" / %s"),szEvent);
				szText[Length]='\0';
			}
		}
		pszText=szText;
	} else {
		pszText=MAIN_TITLE_TEXT;
	}
	::SetWindowText(m_hwnd,pszText);
	ResidentManager.SetTipText(pszText);
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
	if (fRestore && !m_pCore->GetStandby()) {
		if (m_fRestorePreview)
			m_pCore->EnableViewer(true);
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
		ProgramGuideFrame.SetVisible(fShow);
	if (fShowCaptureWindow)
		CaptureWindow.SetVisible(fShow);
	if (StreamInfo.IsCreated())
		StreamInfo.SetVisible(fShow);
}


bool CMainWindow::OnStandbyChange(bool fStandby)
{
	if (fStandby) {
		if (m_fStandbyInit)
			return true;
		m_fRestorePreview=IsViewerEnabled();
		if (m_fRestorePreview)
			m_pCore->EnableViewer(false);
		m_fRestoreFullscreen=m_pCore->GetFullscreen();
		if (m_pCore->GetFullscreen())
			m_pCore->SetFullscreen(false);
		ShowFloatingWindows(false);
		SetVisible(false);
		PluginList.SendStandbyEvent(true);
		m_RestoreChannelSpec.Store(&ChannelManager);
		if (EpgOptions.GetUpdateWhenStandby()
				&& CoreEngine.m_DtvEngine.IsSrcFilterOpen()
				&& !RecordManager.IsRecording()
				&& !CoreEngine.IsNetworkDriver()
				&& !CmdLineParser.m_fNoEpg)
			BeginProgramGuideUpdate(true);
		if (!RecordManager.IsRecording() && !m_fProgramGuideUpdating)
			AppMain.CloseTuner();
	} else {
		SetWindowVisible();
		::SetCursor(LoadCursor(NULL,IDC_WAIT));
		if (m_fStandbyInit) {
			OpenTuner();
			AppMain.InitializeChannel();
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			InitializeViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			m_fStandbyInit=false;
		}
		if (m_fRestoreFullscreen)
			m_pCore->SetFullscreen(true);
		ShowFloatingWindows(true);
		ForegroundWindow(m_hwnd);
		PluginList.SendStandbyEvent(false);
		OpenTuner();
		if (m_fRestorePreview)
			m_pCore->EnableViewer(true);
		::SetCursor(LoadCursor(NULL,IDC_ARROW));
	}
	return true;
}


bool CMainWindow::InitStandby()
{
	m_fRestorePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
					&& (!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay);
	m_fRestoreFullscreen=CmdLineParser.m_fFullscreen;
	if (CoreEngine.GetDriverFileName()[0]!='\0')
		m_fSrcFilterReleased=true;
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		int Space=RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:RestoreChannelInfo.Space;
		const CChannelList *pList=ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(Space);
				m_RestoreChannelSpec.SetChannel(Index);
				m_RestoreChannelSpec.SetServiceID(RestoreChannelInfo.ServiceID);
			}
		}
	}
	ResidentManager.SetResident(true);
	m_fStandbyInit=true;
	m_pCore->SetStandby(true);
	return true;
}


bool CMainWindow::InitMinimize()
{
	m_fRestorePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
					&& (!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay);
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		int Space=RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:RestoreChannelInfo.Space;
		const CChannelList *pList=ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(Space);
				m_RestoreChannelSpec.SetChannel(Index);
				m_RestoreChannelSpec.SetServiceID(RestoreChannelInfo.ServiceID);
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


bool CMainWindow::OpenTuner()
{
	bool fRestoreCh=m_fSrcFilterReleased || m_fProgramGuideUpdating;

	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(false);
	if (m_fSrcFilterReleased) {
		if (!AppMain.OpenTuner())
			return false;
		m_fSrcFilterReleased=false;
	}
	if (fRestoreCh) {
		AppMain.SetChannel(m_RestoreChannelSpec.GetSpace(),
						   m_RestoreChannelSpec.GetChannel(),
						   m_RestoreChannelSpec.GetServiceID());
	}
	return true;
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
	return AppMain.StartRecord(
		pszFileName!=NULL && pszFileName[0]!='\0'?pszFileName:NULL,
		&StartTime,&StopTime,
		CRecordManager::CLIENT_COMMANDLINE);
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

	SendCommand(CM_SHOW);
	PluginList.SendExecuteEvent(pszCmdLine);
	CmdLine.Parse(pszCmdLine);
	if (CmdLine.m_fSilent)
		AppMain.SetSilent(true);
	if (CmdLine.m_fSaveLog)
		CmdLineParser.m_fSaveLog=true;
	if (CmdLine.m_fFullscreen)
		m_pCore->SetFullscreen(true);
	if (!CmdLine.m_DriverName.IsEmpty())
		AppMain.SetDriver(CmdLine.m_DriverName.Get());
	if (CmdLine.IsChannelSpecified())
		SetCommandLineChannel(&CmdLine);
	if (CmdLine.m_fRecord) {
		if (CmdLine.m_fRecordCurServiceOnly)
			CmdLineParser.m_fRecordCurServiceOnly=true;
		CommandLineRecord(CmdLine.m_RecordFileName.Get(),
						  CmdLine.m_RecordDelay,CmdLine.m_RecordDuration);
	} else if (CmdLine.m_fRecordStop)
		AppMain.StopRecord();
	return true;
}


bool CMainWindow::BeginProgramGuideUpdate(bool fStandby)
{
	if (!m_fProgramGuideUpdating) {
		if (CmdLineParser.m_fNoEpg) {
			::MessageBox(ProgramGuideFrame.GetHandle(),
						 TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
						 TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (RecordManager.IsRecording()) {
			if (::MessageBox(ProgramGuideFrame.GetHandle(),
							 TEXT("録画中です。\n番組表の取得を開始してもいいですか?"),TEXT("確認"),
							 MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK)
				return false;
		}
		const CChannelList *pList=ChannelManager.GetCurrentRealChannelList();
		if (pList==NULL)
			return false;
		const CChannelInfo *pChInfo;
		int i;
		for (i=0;i<pList->NumChannels();i++) {
			pChInfo=pList->GetChannelInfo(i);
			if (pChInfo->IsEnabled())
				break;
		}
		if (i==pList->NumChannels())
			return false;
		Logger.AddLog(TEXT("番組表の取得開始"));
		if (m_pCore->GetStandby() && m_fSrcFilterReleased) {
			if (!OpenTuner())
				return false;
		}
		m_fProgramGuideUpdating=true;
		m_ProgramGuideUpdateStartChannel=ChannelManager.GetCurrentChannel();
		if (!fStandby) {
			m_fRestorePreview=IsViewerEnabled();
			m_pCore->EnableViewer(false);
		}
		AppMain.SetChannel(ChannelManager.GetCurrentSpace(),i);
		::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,
				   pChInfo->GetNetworkID()>=6 && pChInfo->GetNetworkID()<=10?
				   (m_pCore->GetStandby()?120000:60000):(m_pCore->GetStandby()?90000:40000),NULL);
	}
	return true;
}


void CMainWindow::OnProgramGuideUpdateEnd(bool fRelease/*=true*/)
{
	if (m_fProgramGuideUpdating) {
		HANDLE hThread;
		int OldPriority;

		Logger.AddLog(TEXT("番組表の取得終了"));
		::KillTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE);
		m_fProgramGuideUpdating=false;
		if (m_pCore->GetStandby()) {
			hThread=::GetCurrentThread();
			OldPriority=::GetThreadPriority(hThread);
			::SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		}
		EpgProgramList.UpdateProgramList();
		EpgOptions.SaveEpgFile(&EpgProgramList);
		if (m_pCore->GetStandby()) {
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			::SetThreadPriority(hThread,OldPriority);
			if (fRelease)
				AppMain.CloseTuner();
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			AppMain.SetChannel(ChannelManager.GetCurrentSpace(),
							   m_ProgramGuideUpdateStartChannel);
			if (m_fRestorePreview)
				m_pCore->EnableViewer(true);
			if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
				ChannelPanel.UpdateChannelList();
		}
	}
}


void CMainWindow::EndProgramGuideUpdate(bool fRelease/*=true*/)
{
	if (ProgramGuide.GetVisible()) {
		ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
	} else {
		OnProgramGuideUpdateEnd(fRelease);
	}
}


void CMainWindow::UpdatePanel()
{
	switch (PanelForm.GetCurPageID()) {
	case PANEL_ID_INFORMATION:
		{
			BYTE AspectX,AspectY;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
				InfoPanel.SetAspectRatio(AspectX,AspectY);
			if (InfoPanel.IsSignalLevelEnabled())
				InfoPanel.SetSignalLevel(CoreEngine.GetSignalLevel());
			InfoPanel.SetBitRate(CoreEngine.GetBitRate());
			InfoPanel.UpdateErrorCount();
			if (RecordManager.IsRecording()) {
				const CRecordTask *pRecordTask=RecordManager.GetRecordTask();
				InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
					pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime());
			}
			UpdateProgramInfo();
		}
		break;

	case PANEL_ID_PROGRAMLIST:
		if (m_ProgramListUpdateTimerCount>0) {
			int ServiceID=ChannelManager.GetCurrentServiceID();

			if (ServiceID<=0) {
				WORD SID;
				if (CoreEngine.m_DtvEngine.GetServiceID(&SID))
					ServiceID=SID;
			}
			if (ServiceID>0)
				ProgramListPanel.UpdateProgramList(CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),(WORD)ServiceID);
		}
		break;

	case PANEL_ID_CHANNEL:
		RefreshChannelPanel();
		break;
	}
}


void CMainWindow::RefreshChannelPanel()
{
	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	if (ChannelPanel.IsChannelListEmpty())
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	else
		ChannelPanel.UpdateChannelList();
	ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	::SetCursor(hcurOld);
}


void CMainWindow::ApplyColorScheme(const CColorScheme *pColorScheme)
{
	Theme::BorderInfo Border;

	m_LayoutBase.SetBackColor(pColorScheme->GetColor(CColorScheme::COLOR_SPLITTER));
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
	if (!ViewOptions.GetClientEdge())
		Border.Type=Theme::BORDER_NONE;
	m_Viewer.GetViewWindow().SetBorder(&Border);

	CTitleBar::ThemeInfo TitleBarTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARCAPTION,
						   &TitleBarTheme.CaptionStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARICON,
						   &TitleBarTheme.IconStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARHIGHLIGHTITEM,
						   &TitleBarTheme.HighlightIconStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_TITLEBAR,
								&TitleBarTheme.Border);
	m_TitleBar.SetTheme(&TitleBarTheme);
}


bool CMainWindow::SetLogo(LPCTSTR pszFileName)
{
	if (pszFileName==NULL || pszFileName[0]=='\0')
		return m_Viewer.GetViewWindow().SetLogo(NULL);

	TCHAR szFileName[MAX_PATH];

	if (::PathIsRelative(pszFileName)) {
		TCHAR szTemp[MAX_PATH];
		AppMain.GetAppDirectory(szTemp);
		::PathAppend(szTemp,pszFileName);
		::PathCanonicalize(szFileName,szTemp);
	} else {
		::lstrcpy(szFileName,pszFileName);
	}
	HBITMAP hbm=static_cast<HBITMAP>(::LoadImage(NULL,szFileName,IMAGE_BITMAP,
								0,0,LR_LOADFROMFILE | LR_CREATEDIBSECTION));
	if (hbm==NULL)
		return false;
	return m_Viewer.GetViewWindow().SetLogo(hbm);
}


bool CMainWindow::SetViewWindowEdge(bool fEdge)
{
	const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
	Theme::BorderInfo Border;

	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
	if (!fEdge)
		Border.Type=Theme::BORDER_NONE;
	m_Viewer.GetViewWindow().SetBorder(&Border);
	return true;
}


bool CMainWindow::GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD)
{
	if (m_Viewer.GetVideoContainer().GetVisible()) {
		*phwndParent=m_Viewer.GetVideoContainer().GetHandle();
	} else {
		*phwndParent=m_Viewer.GetViewWindow().GetHandle();
		*pfForcePseudoOSD=true;
	}
	::GetClientRect(*phwndParent,pRect);
	pRect->top+=NotificationBar.GetBarHeight();
	pRect->bottom-=StatusView.GetHeight();
	return true;
}


bool CMainWindow::SetOSDHideTimer(DWORD Delay)
{
	return ::SetTimer(m_hwnd,TIMER_ID_OSD,Delay,NULL)!=0;
}


CStatusView *CMainWindow::GetStatusView() const
{
	return &StatusView;
}




CMainWindow::CDisplayBaseEventHandler::CDisplayBaseEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

bool CMainWindow::CDisplayBaseEventHandler::OnVisibleChange(bool fVisible)
{
	if (!m_pMainWindow->IsViewerEnabled()) {
		m_pMainWindow->m_Viewer.GetVideoContainer().SetVisible(fVisible);
	}
	return true;
}




static bool IsNoAcceleratorMessage(const MSG *pmsg)
{
	HWND hwnd=::GetFocus();

	if (hwnd!=NULL && ::IsWindowVisible(hwnd)) {
		if (hwnd==ChannelDisplayMenu.GetHandle()) {
			return ChannelDisplayMenu.IsMessageNeed(pmsg);
		} else {
			TCHAR szClass[64];

			if (::GetClassName(hwnd,szClass,lengthof(szClass))>0) {
				if (::lstrcmpi(szClass,TEXT("EDIT"))==0
						|| ::StrCmpNI(szClass,TEXT("RICHEDIT"),8)==0) {
					if ((GetWindowStyle(hwnd)&ES_READONLY)==0)
						return true;
					if (pmsg->message==WM_KEYDOWN || pmsg->message==WM_KEYUP) {
						switch (pmsg->wParam) {
						case VK_LEFT:
						case VK_RIGHT:
						case VK_UP:
						case VK_DOWN:
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


#ifndef _DEBUG
#include "DebugHelper.h"
static CDebugHelper DebugHelper;
#endif

static int ApplicationMain(HINSTANCE hInstance,LPCTSTR pszCmdLine,int nCmdShow)
{
	hInst=hInstance;

	// コマンドラインの解析
	if (pszCmdLine[0]!='\0') {
		CmdLineParser.Parse(pszCmdLine);
		// 暫定措置
		if (CmdLineParser.m_TvRockDID>=0)
			CmdLineParser.m_fSilent=true;
		if (CmdLineParser.m_fSilent) {
			AppMain.SetSilent(true);
#ifndef _DEBUG
			DebugHelper.SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_NONE);
#endif
		}
		if (CmdLineParser.m_fMaximize && !CmdLineParser.m_fMinimize)
			MainWindow.SetMaximize(true);
	}

//#ifdef TVH264_FOR_1SEG
	PanelFrame.SetFloating(false);
//#endif

	AppMain.Initialize();

	//CAppMutex Mutex(fKeepSingleTask || CmdLineParser.m_fSingleTask);
	CAppMutex Mutex(true);

	// 複数起動のチェック
	if (Mutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineParser.m_fSingleTask)) {
		Logger.AddLog(TEXT("複数起動が禁止されています。"));
		CTVTestWindowFinder Finder;
		HWND hwnd=Finder.FindCommandLineTarget();
		if (::IsWindow(hwnd)) {
			ATOM atom;

			if (pszCmdLine[0]!='\0')
				atom=::GlobalAddAtom(pszCmdLine);
			else
				atom=0;
			// ATOM だと256文字までしか渡せないので、WM_COPYDATA 辺りの方がいいかも
			/*
			DWORD_PTR Result;
			::SendMessageTimeout(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0,
								 SMTO_NORMAL,10000,&Result);
			*/
			::PostMessage(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0);
			return 0;
		} else if (!CmdLineParser.m_fSingleTask) {
			// 固まった場合でも WinMain は抜けるのでこれは無意味...
			if (!AppMain.IsSilent())
				MessageBox(NULL,
					APP_NAME TEXT(" は既に起動しています。\n")
					TEXT("ウィンドウが見当たらない場合はタスクマネージャに隠れていますので\n")
					TEXT("強制終了させてください。"),MAIN_TITLE_TEXT,
					MB_OK | MB_ICONEXCLAMATION);
			return 0;
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
		CoreEngine.GetDriverDirectory(szDirectory);
		DriverManager.Find(szDirectory);
	}
	DriverOptions.Initialize(&DriverManager);

	TCHAR szDriverFileName[MAX_PATH];

	if (CmdLineParser.m_fInitialSettings
			|| (AppMain.IsFirstExecute() && CmdLineParser.m_DriverName.IsEmpty())) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.ShowDialog(NULL))
			return 0;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		GeneralOptions.SetDefaultDriverName(szDriverFileName);
		GeneralOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		GeneralOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		GeneralOptions.SetCardReaderType(InitialSettings.GetCardReader());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	} else if (!CmdLineParser.m_DriverName.IsEmpty()) {
		::lstrcpy(szDriverFileName,CmdLineParser.m_DriverName.Get());
	} else {
		GeneralOptions.GetFirstDriverName(szDriverFileName);
	}

	GeneralOptions.SetTemporaryNoDescramble(CmdLineParser.m_fNoDescramble);

	ColorSchemeOptions.SetApplyCallback(ColorSchemeApplyProc);
	ColorSchemeOptions.ApplyColorScheme();

	CMainWindow::Initialize();
	CViewWindow::Initialize(hInst);
	CVideoContainerWindow::Initialize(hInst);
	CStatusView::Initialize(hInst);
	CSideBar::Initialize(hInst);
	//CSplitter::Initialize(hInst);
	Layout::CLayoutBase::Initialize(hInst);
	CTitleBar::Initialize(hInst);
	CPanelFrame::Initialize(hInst);
	CPanelForm::Initialize(hInst);
	CInformationPanel::Initialize(hInst);
	CProgramListPanel::Initialize(hInst);
	CChannelPanel::Initialize(hInst);
	CControlPanel::Initialize(hInst);
	CCaptionPanel::Initialize(hInst);
	CProgramGuide::Initialize(hInst);
	CProgramGuideFrame::Initialize(hInst);
	CProgramGuideDisplay::Initialize(hInst);
	CCaptureWindow::Initialize(hInst);
	CPseudoOSD::Initialize(hInst);
	CNotificationBar::Initialize(hInst);
	CEventInfoPopup::Initialize(hInst);
	CDropDownMenu::Initialize(hInst);
	CChannelDisplayMenu::Initialize(hInst);

	StreamInfo.SetEventHandler(&StreamInfoEventHandler);

	if (!MainWindow.Create(NULL,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		Logger.AddLog(TEXT("ウィンドウが作成できません。"));
		if (!AppMain.IsSilent())
			MessageBox(NULL,TEXT("ウィンドウが作成できません。"),NULL,MB_OK | MB_ICONSTOP);
		return 0;
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
	if (!MainWindow.GetTitleBarVisible() || MainWindow.GetCustomTitleBar()) {
		// この段階でスタイルを変えないとおかしくなる
		// 最初からこのスタイルにしてもキャプションが表示される
		// ShowWindow の前に入れると、タイトルバーを表示させてもアイコンが出ない
		MainWindow.SetStyle(MainWindow.GetStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	ResidentManager.SetResident(GeneralOptions.GetResident());
	ResidentManager.Initialize(MainWindow.GetHandle(),WM_APP_TRAYICON);
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	if (CmdLineParser.m_fMinimize)
		MainWindow.InitMinimize();

	ViewOptions.Apply(COptions::UPDATE_ALL);

	CoreEngine.SetDriverFileName(szDriverFileName);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	if (!CmdLineParser.m_fNoDriver && !CmdLineParser.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("ドライバの読み込み中..."));
			if (CoreEngine.LoadDriver()) {
				Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
				AppMain.GetUICore()->OnTunerChanged();
			} else {
				AppMain.OnError(&CoreEngine);
			}
		} else {
			Logger.AddLog(TEXT("BonDriverが設定されていません。"));
			/*
			if (!AppMain.IsSilent())
				MainWindow.ShowMessage(
						TEXT("設定で使用するドライバを指定してください。"),
						TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
			*/
		}
	}

	CoreEngine.SetMinTimerResolution(PlaybackOptions.GetMinTimerResolution());
	CoreEngine.SetDescramble(!CmdLineParser.m_fNoDescramble);
	CoreEngine.SetCardReaderType(GeneralOptions.GetCardReaderType());
	CoreEngine.SetNoEpg(CmdLineParser.m_fNoEpg);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(GeneralOptions.GetDescrambleCurServiceOnly());
	CoreEngine.m_DtvEngine.m_TsDescrambler.EnableSSE2(GeneralOptions.GetDescrambleUseSSE2());
	CoreEngine.m_DtvEngine.m_TsDescrambler.EnableEmmProcess(GeneralOptions.GetEnableEmmProcess());
	PlaybackOptions.Apply(COptions::UPDATE_ALL);
	CoreEngine.m_DtvEngine.m_LogoDownloader.SetLogoHandler(&LogoManager);
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	CoreEngine.BuildDtvEngine(&DtvEngineHandler);
	RecordOptions.Apply(COptions::UPDATE_ALL);

	if (CoreEngine.IsDriverLoaded()
			&& !DriverOptions.IsDescrambleDriver(CoreEngine.GetDriverFileName())) {
		AppMain.OpenBcasCard(!AppMain.IsSilent());
	}

	if (!CmdLineParser.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];
		std::vector<LPCTSTR> ExcludePlugins;

		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		AppMain.GetAppDirectory(szPluginDir);
		if (!CmdLineParser.m_PluginsDirectory.IsEmpty()) {
			LPCTSTR pszDir=CmdLineParser.m_PluginsDirectory.Get();
			if (::PathIsRelative(pszDir)) {
				TCHAR szTemp[MAX_PATH];
				::PathCombine(szTemp,szPluginDir,pszDir);
				::PathCanonicalize(szPluginDir,szTemp);
			} else {
				::lstrcpy(szPluginDir,pszDir);
			}
		} else {
			::PathAppend(szPluginDir,
#ifndef TVH264
						 TEXT("Plugins")
#else
						 TEXT("H264Plugins")
#endif
						 );
		}
		Logger.AddLog(TEXT("プラグインを \"%s\" から読み込みます..."),szPluginDir);
		if (CmdLineParser.m_NoLoadPlugins.size()>0) {
			for (size_t i=0;i<CmdLineParser.m_NoLoadPlugins.size();i++)
				ExcludePlugins.push_back(CmdLineParser.m_NoLoadPlugins[i].Get());
		}
		PluginList.LoadPlugins(szPluginDir,&ExcludePlugins);
	}

	CommandList.Initialize(&DriverManager,&PluginList,&ZoomOptions);

	if (!CmdLineParser.m_fNoPlugin)
		PluginOptions.RestorePluginOptions();

	CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(PlaybackOptions.GetUseAudioRendererClock());
	CoreEngine.SetDownMixSurround(PlaybackOptions.GetDownMixSurround());
	if (!CmdLineParser.m_fStandby && !CmdLineParser.m_fNoDirectShow)
		AppMain.GetUICore()->InitializeViewer();

	if (!CmdLineParser.m_fNoEpg) {
		EpgOptions.SetEDCBDataLoadEventHandler(&EpgLoadEventHandler);
		EpgOptions.AsyncLoadEDCBData();
	}
	EpgOptions.LoadLogoFile();

	{
		TCHAR szLogoMapName[MAX_PATH];

		::GetModuleFileName(NULL,szLogoMapName,MAX_PATH);
		::PathRenameExtension(szLogoMapName,TEXT(".logo.ini"));
		if (::PathFileExists(szLogoMapName)) {
			StatusView.SetSingleText(TEXT("ロゴ設定を読み込んでいます..."));
			LogoManager.LoadLogoIDMap(szLogoMapName);
		}
	}

	{
		TCHAR szDRCSMapName[MAX_PATH];

		AppMain.GetAppDirectory(szDRCSMapName);
		::PathAppend(szDRCSMapName,TEXT("DRCSMap.ini"));
		if (::PathFileExists(szDRCSMapName)) {
			StatusView.SetSingleText(TEXT("DRCSマップを読み込んでいます..."));
			CaptionPanel.LoadDRCSMap(szDRCSMapName);
		}
	}

	if (CoreEngine.IsDriverLoaded()) {
		CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(
			!DriverOptions.IsIgnoreInitialStream(CoreEngine.GetDriverFileName()));
		if (CoreEngine.OpenDriver()) {
			CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetPurgeStreamOnChannelChange(
				DriverOptions.IsPurgeStreamOnChannelChange(CoreEngine.GetDriverFileName()));
		} else {
			AppMain.OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
		}
	}

	if (PlaybackOptions.GetRestoreMute() && fMuteStatus)
		AppMain.GetUICore()->SetMute(true);
	if ((!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay)
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!CmdLineParser.m_fNoView && !CmdLineParser.m_fMinimize)
			AppMain.GetUICore()->EnableViewer(true);
	}

	if (CoreEngine.IsNetworkDriver()) {
		if (fIncrementUDPPort) {
			CPortQuery PortQuery;
			WORD UDPPort=CmdLineParser.m_UDPPort>0?(WORD)CmdLineParser.m_UDPPort:
											CoreEngine.IsUDPDriver()?1234:2230;
			WORD RemoconPort=NetworkRemoconOptions.GetPort();

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(MainWindow.GetHandle(),&UDPPort,CoreEngine.IsUDPDriver()?1243:2239,&RemoconPort);
			CmdLineParser.m_UDPPort=UDPPort;
			NetworkRemoconOptions.SetTempPort(RemoconPort);
		}
		if (CmdLineParser.m_fUseNetworkRemocon)
			NetworkRemoconOptions.SetTempEnable(true);
	}

	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	AppMain.InitializeChannel();

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	if (!MainWindow.GetStatusBarVisible())
		StatusView.SetVisible(false);
	StatusView.SetSingleText(NULL);

	PanelForm.SetEventHandler(&PanelFormEventHandler);
	PanelForm.Create(MainWindow.GetHandle(),WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	PanelForm.SetTabFont(PanelOptions.GetFont());

	InfoPanel.SetEventHandler(&InformationPanelEventHandler);
	InfoPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	InfoPanel.ShowSignalLevel(!DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName()));
	PanelForm.AddWindow(&InfoPanel,PANEL_ID_INFORMATION,TEXT("情報"));

	ProgramListPanel.SetEpgProgramList(&EpgProgramList);
	ProgramListPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_VSCROLL);
	PanelForm.AddWindow(&ProgramListPanel,PANEL_ID_PROGRAMLIST,TEXT("番組表"));

	ChannelPanel.SetEpgProgramList(&EpgProgramList);
	ChannelPanel.SetEventHandler(&ChannelPanelEventHandler);
	ChannelPanel.SetLogoManager(&LogoManager);
	PanelOptions.ApplyChannelPanelOptions(&ChannelPanel);
	ChannelPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_VSCROLL);
	PanelForm.AddWindow(&ChannelPanel,PANEL_ID_CHANNEL,TEXT("チャンネル"));

	ControlPanel.SetSendMessageWindow(MainWindow.GetHandle());
	InitControlPanel();
	ControlPanel.Create(PanelForm.GetHandle(),WS_CHILD);
	PanelForm.AddWindow(&ControlPanel,PANEL_ID_CONTROL,TEXT("操作"));

	CaptionPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	PanelForm.AddWindow(&CaptionPanel,PANEL_ID_CAPTION,TEXT("字幕"));

	PanelOptions.InitializePanelForm(&PanelForm);
	PanelFrame.Create(MainWindow.GetHandle(),
					  dynamic_cast<Layout::CSplitter*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_PANELSPLITTER)),
					  CONTAINER_ID_PANEL,&PanelForm,TEXT("パネル"));
	PanelFrame.SetEventHandler(&PanelEventHandler);
	if (fShowPanelWindow
			&& (!PanelFrame.GetFloating()
				|| (!CmdLineParser.m_fStandby && !CmdLineParser.m_fMinimize))) {
		PanelFrame.SetPanelVisible(true,true);
		PanelFrame.Update();
	}

	if (!CmdLineParser.m_fNoEpg) {
		EpgOptions.AsyncLoadEpgFile(&EpgProgramList,&EpgLoadEventHandler);
	}

	ProgramGuide.SetEpgProgramList(&EpgProgramList);
	ProgramGuide.SetEventHandler(&ProgramGuideEventHandler);
	ProgramGuide.SetDriverList(&DriverManager);
	ProgramGuide.SetLogoManager(&LogoManager);

	CaptureWindow.SetEventHandler(&CaptureWindowEventHandler);

	ApplyEventInfoFont();

	Accelerator.Initialize(MainWindow.GetHandle(),&MainMenu,
						   AppMain.GetIniFileName(),&CommandList);
	OperationOptions.Initialize(AppMain.GetIniFileName(),&CommandList);

	if (CoreEngine.m_DtvEngine.IsSrcFilterOpen()) {
		if (CoreEngine.IsBuildComplete()) {
			if (CmdLineParser.m_fFullscreen)
				AppMain.GetUICore()->SetFullscreen(true);
		}

		if (CoreEngine.IsNetworkDriver()) {
			const int FirstPort=CoreEngine.IsUDPDriver()?1234:2230;
			int Port=FirstPort;
			if ((int)CmdLineParser.m_UDPPort>=FirstPort && (int)CmdLineParser.m_UDPPort<FirstPort+10)
				Port=CmdLineParser.m_UDPPort;
			else if (RestoreChannelInfo.Channel>=0 && RestoreChannelInfo.Channel<10)
				Port=FirstPort+RestoreChannelInfo.Channel;
			//if (Port!=FirstPort)
				AppMain.SetChannel(0,Port-FirstPort);
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
		} else if (RestoreChannelInfo.Space>=0
				&& RestoreChannelInfo.Channel>=0) {
			AppMain.RestoreChannel();
		} else {
			// 初期チャンネルに設定する
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i=pList->Find(
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace(),
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel());

			if (i>=0)
				MainWindow.SendCommand(CM_CHANNEL_FIRST+i);
		}

		if (CmdLineParser.m_fRecord)
			MainWindow.CommandLineRecord(CmdLineParser.m_RecordFileName.Get(),
				CmdLineParser.m_RecordDelay,CmdLineParser.m_RecordDuration);
	}
	if (CmdLineParser.m_fStandby)
		MainWindow.InitStandby();
	if (CmdLineParser.m_fExitOnRecordEnd)
		MainWindow.SendCommand(CM_EXITONRECORDINGSTOP);
	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),false);

	SetFocus(MainWindow.GetHandle());

	MSG msg;

	while (GetMessage(&msg,NULL,0,0)>0) {
		if (HtmlHelpClass.PreTranslateMessage(&msg)
				|| StreamInfo.ProcessMessage(&msg))
			continue;
		if ((IsNoAcceleratorMessage(&msg)
				|| !Accelerator.TranslateMessage(MainWindow.GetHandle(),&msg))
				&& !ControllerManager.TranslateMessage(MainWindow.GetHandle(),&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
												LPTSTR pszCmdLine,int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	DebugHelper.SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif

	Logger.AddLog(TEXT("******** 起動 ********"));

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

	const int Result=ApplicationMain(hInstance,pszCmdLine,nCmdShow);

	CoUninitialize();

	Logger.AddLog(TEXT("******** 終了 ********"));
	if (CmdLineParser.m_fSaveLog && !Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		Logger.GetDefaultLogFileName(szFileName);
		Logger.SaveToFile(szFileName,true);
	}

	return Result;
}

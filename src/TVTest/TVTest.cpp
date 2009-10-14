#include "stdafx.h"
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
#include "RecordOptions.h"
#include "Capture.h"
#include "Plugin.h"
#include "GeneralOptions.h"
#include "ViewOptions.h"
#include "StatusOptions.h"
#include "SideBarOptions.h"
#include "PanelOptions.h"
#include "OperationOptions.h"
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
#include "MessageDialog.h"
#include "Image.h"
#include "StreamInfo.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "PseudoOSD.h"
#include "NotificationBar.h"
#include "IconMenu.h"
#include "Taskbar.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#define MAIN_WINDOW_CLASS				APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS			APP_NAME TEXT(" Fullscreen")

#define TITLE_TEXT APP_NAME

enum {
	PANE_ID_VIEW=1,
	PANE_ID_PANEL
};


static HINSTANCE hInst;
static CAppMain AppMain;
static CCoreEngine CoreEngine;
static CMainMenu MainMenu;
static CCommandList CommandList;
static CEpgProgramList EpgProgramList;
static CMainWindow MainWindow;
static CStatusView StatusView;
static CTitleBar TitleBar;
static CSideBar SideBar(&CommandList);
static CSplitter Splitter;
static CNotificationBar NotificationBar;
static CMessageDialog MessageDialog;
static CHtmlHelp HtmlHelpClass;
static CPseudoOSD ChannelOSD;
static CPseudoOSD VolumeOSD;
static CIconMenu AspectRatioIconMenu;
static CTaskbarManager TaskbarManager;

static TCHAR szDriverFileName[MAX_PATH];
static bool fIncrementUDPPort=true;

static CCommandLineParser CmdLineParser;

static CChannelManager ChannelManager;
static CNetworkRemocon *pNetworkRemocon=NULL;
static CResidentManager ResidentManager;
static CDriverManager DriverManager;

static bool fShowPanelWindow=false;
static CPanelFrame PanelFrame;
static int PanelPaneIndex=1;
static CInfoPanel InfoWindow;

static CInformationPanel InfoPanel;
static CProgramListView ProgramListPanel;
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

static CGeneralOptions GeneralOptions;
static CViewOptions ViewOptions;
static COSDOptions OSDOptions;
static CStatusOptions StatusOptions(&StatusView);
static CSideBarOptions SideBarOptions(&SideBar);
static CPanelOptions PanelOptions(&PanelFrame);
static CColorSchemeOptions ColorSchemeOptions;
static COperationOptions OperationOptions;
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

static struct {
	int Space;
	int Channel;
	int ServiceID;
	TCHAR szDriverName[MAX_PATH];
} RestoreChannelInfo;

static bool fEnablePlay=true;
static bool fMuteStatus=false;

static CImageCodec ImageCodec;
static CCaptureWindow CaptureWindow;
static bool fShowCaptureWindow=false;

#if defined(TVH264) && defined(USE_TBS_FILTER)
//#define TBS_FILTER_INTERFACE
#endif

#ifdef TBS_FILTER_INTERFACE
#include <vector>
class CTBSFilterTSIDList {
	std::vector<WORD> m_TSIDList;
public:
	void Add(WORD TSID) {
		for (size_t i=0;i<m_TSIDList.size();i++) {
			if (m_TSIDList[i]==TSID)
				return;
		}
		m_TSIDList.push_back(TSID);
	}
	void Remove(WORD TSID) {
		std::vector<WORD>::iterator itr;

		for (itr=m_TSIDList.begin();itr!=m_TSIDList.end();itr++) {
			if (*itr==TSID) {
				m_TSIDList.erase(itr);
				return;
			}
		}
	}
	bool HasTSID(WORD TSID) const {
		for (size_t i=0;i<m_TSIDList.size();i++) {
			if (m_TSIDList[i]==TSID)
				return true;
		}
		return false;
	}
	bool Load(LPCTSTR pszFileName) {
		CSettings Settings;

		if (!Settings.Open(pszFileName,TEXT("TBSFilter"),CSettings::OPEN_READ))
			return false;
		int Count;
		if (Settings.Read(TEXT("TSIDCount"),&Count) && Count>0) {
			m_TSIDList.clear();
			for (int i=0;i<Count;i++) {
				TCHAR szName[16];
				int TSID;

				::wsprintf(szName,TEXT("TSID%d"),i);
				if (!Settings.Read(szName,&TSID))
					break;
				m_TSIDList.push_back(TSID);
			}
		}
		Settings.Close();
		return true;
	}
	bool Save(LPCTSTR pszFileName) const {
		CSettings Settings;

		if (!Settings.Open(pszFileName,TEXT("TBSFilter"),CSettings::OPEN_WRITE))
			return false;
		Settings.Clear();
		Settings.Write(TEXT("TSIDCount"),(unsigned int)m_TSIDList.size());
		for (size_t i=0;i<m_TSIDList.size();i++) {
			TCHAR szName[16];

			::wsprintf(szName,TEXT("TSID%d"),i);
			Settings.Write(szName,(int)m_TSIDList[i]);
		}
		Settings.Close();
		return true;
	}
};
static CTBSFilterTSIDList TBSFilterTSIDList;
#endif	// TBS_FILTER_INTERFACE


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


void CAppMain::OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	if (pErrorHandler==NULL)
		return;
	Logger.AddLog(pErrorHandler->GetLastErrorText());
	if (!CmdLineParser.m_fSilent)
		MainWindow.ShowErrorMessage(pErrorHandler,pszTitle);
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
		if (!ChannelFilePath.HasDirectory()) {
			TCHAR szDir[MAX_PATH];

			GetAppDirectory(szDir);
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

	RestoreChannelInfo.Space=-1;
	RestoreChannelInfo.Channel=-1;
	RestoreChannelInfo.ServiceID=-1;
	CDriverOptions::InitialChannelInfo InitChInfo;
	if (DriverOptions.GetInitialChannel(CoreEngine.GetDriverFileName(),&InitChInfo)) {
		RestoreChannelInfo.Space=InitChInfo.Space;
		RestoreChannelInfo.Channel=InitChInfo.Channel;
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
			/*
			::wsprintf(szName,TEXT("Service_%s"),szDriverName);
			Setting.Read(szName,&RestoreChannelInfo.Service);
			*/
			::wsprintf(szName,TEXT("ServiceID_%s"),szDriverName);
			Setting.Read(szName,&RestoreChannelInfo.ServiceID);
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
		RestoreChannelInfo.Space>=0?RestoreChannelInfo.Space:0,-1);
	ChannelManager.SetCurrentServiceID(0);
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	MainWindow.OnChannelListUpdated();
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
	ChannelManager.SetCurrentServiceID(0);
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		FollowChannelChange(CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),ServiceID);
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
	MainWindow.OnChannelListUpdated();
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
		if (ServiceID<=0 && pChInfo->GetService()>0 && pChInfo->GetServiceID()>0)
			ServiceID=pChInfo->GetServiceID();

		LPCTSTR pszTuningSpace=ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetSpace());
		AddLog(TEXT("BonDriverにチャンネル変更を要求しました。(チューニング空間 %d[%s] / Ch %d / Sv %d)"),
			   pChInfo->GetSpace(),pszTuningSpace!=NULL?pszTuningSpace:TEXT("???"),
			   pChInfo->GetChannelIndex(),ServiceID);

		if (!CoreEngine.m_DtvEngine.SetChannel(
							pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
							ServiceID>0?ServiceID:CDtvEngine::SID_INVALID)) {
			AddLog(CoreEngine.m_DtvEngine.GetLastErrorText());
			ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
			return false;
		}
#ifdef TVH264
		// 予めTSIDを設定して表示を早くする
		if ((pChInfo->GetTransportStreamID() & 0xFF00) == 0x7F00) {
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
	MainWindow.OnChannelChanged();
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
	Logger.AddLog(TEXT("チャンネル変更を検知しました。(TSID %d / SID %d)"),
				  TransportStreamID,ServiceID);
	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	ChannelManager.SetCurrentServiceID(0);
	MainWindow.OnChannelChanged();
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
	MainWindow.OnServiceChanged();
	PluginList.SendServiceChangeEvent();
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
			&& DriverOptions.IsDescrambleDriver(pszFileName)) {
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
		fOK=CoreEngine.OpenDriver();
		if (fOK) {
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
	MainWindow.OnDriverChanged();
	return fOK;
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
					MainWindow.OnTunerOpened();
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
		MainWindow.OnTunerClosed();
	}
	return true;
}


bool CAppMain::OpenBcasCard(bool fRetry)
{
	if (!CoreEngine.IsBcasCardOpen()) {
		if (!CoreEngine.OpenBcasCard()) {
			if (fRetry) {
				TCHAR szText[1024];
				int Length;

				Length=::wsprintf(szText,TEXT("%s\n"),CoreEngine.GetLastErrorText());
				if (CoreEngine.GetLastErrorSystemMessage()!=NULL)
					Length+=::wsprintf(szText+Length,TEXT("(%s)\n"),CoreEngine.GetLastErrorSystemMessage());
				::lstrcpy(szText+Length,TEXT("利用可能なカードリーダを検索しますか?"));
				if (MainWindow.ShowMessage(szText,NULL,MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2)==IDYES) {
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
				if (!CmdLineParser.m_fSilent)
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
		if (Setting.Read(TEXT("VolumeNormalizeLevel"),&Value))
			CoreEngine.SetVolumeNormalizeLevel(Value);
		Setting.Read(TEXT("ShowInfoWindow"),&fShowPanelWindow);
		Setting.Read(TEXT("Driver"),szDriverFileName,
												lengthof(szDriverFileName));
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
		ProgramGuide.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Read(TEXT("ProgramGuideLeft"),&Left);
		Setting.Read(TEXT("ProgramGuideTop"),&Top);
		Setting.Read(TEXT("ProgramGuideWidth"),&Width);
		Setting.Read(TEXT("ProgramGuideHeight"),&Height);
		ProgramGuide.SetPosition(Left,Top,Width,Height);
		ProgramGuide.MoveToMonitorInside();
		if (Setting.Read(TEXT("ProgramGuideMaximized"),&f) && f)
			ProgramGuide.SetMaximize(f);
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
	SideBarOptions.Load(m_szIniFileName);
	ColorSchemeOptions.Load(m_szIniFileName);
	//Accelerator.Load(m_szIniFileName);
	//HDUSController.Load(m_szIniFileName);
	DriverOptions.Load(m_szIniFileName);
	ProgramGuideOptions.Load(m_szIniFileName);
	PluginOptions.Load(m_szIniFileName);
	ChannelHistory.Load(m_szIniFileName);
	InfoPanel.Load(m_szIniFileName);
#ifdef TBS_FILTER_INTERFACE
	TBSFilterTSIDList.Load(m_szIniFileName);
#endif
	return true;
}


bool CAppMain::SaveSettings()
{
	CSettings Setting;

	if (Setting.Open(m_szIniFileName,TEXT("Settings"),CSettings::OPEN_WRITE)) {
		int Left,Top,Width,Height;

		Setting.Write(TEXT("Driver"),CoreEngine.GetDriverFileName());
		Setting.Write(TEXT("Volume"),CoreEngine.GetVolume());
		Setting.Write(TEXT("VolumeNormalizeLevel"),CoreEngine.GetVolumeNormalizeLevel());
		Setting.Write(TEXT("ShowInfoWindow"),fShowPanelWindow);
		Setting.Write(TEXT("EnablePlay"),MainWindow.IsPreview());
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
		Setting.Write(TEXT("InfoCurTab"),InfoWindow.GetCurTab());
		ProgramGuide.GetPosition(&Left,&Top,&Width,&Height);
		Setting.Write(TEXT("ProgramGuideLeft"),Left);
		Setting.Write(TEXT("ProgramGuideTop"),Top);
		Setting.Write(TEXT("ProgramGuideWidth"),Width);
		Setting.Write(TEXT("ProgramGuideHeight"),Height);
		Setting.Write(TEXT("ProgramGuideMaximized"),ProgramGuide.GetMaximizeStatus());
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
	SideBarOptions.Save(m_szIniFileName);
	ColorSchemeOptions.Save(m_szIniFileName);
	Accelerator.Save(m_szIniFileName);
	HDUSController.Save(m_szIniFileName);
	DriverOptions.Save(m_szIniFileName);
	ProgramGuideOptions.Save(m_szIniFileName);
	PluginOptions.Save(m_szIniFileName);
	ChannelHistory.Save(m_szIniFileName);
	InfoPanel.Save(m_szIniFileName);
#ifdef TBS_FILTER_INTERFACE
	TBSFilterTSIDList.Save(m_szIniFileName);
#endif
	return true;
}


bool CAppMain::SaveCurrentChannel()
{
	if (*CoreEngine.GetDriverFileName()!='\0') {
		const CChannelInfo *pInfo=ChannelManager.GetCurrentRealChannelInfo();
		int Space,Channel,ServiceID;
		CSettings Setting;

		if (pInfo!=NULL) {
			Space=pInfo->GetSpace();
			Channel=pInfo->GetChannelIndex();
			ServiceID=pInfo->GetServiceID();
		} else {
			Space=-1;
			Channel=-1;
			ServiceID=0;
		}
		if (Setting.Open(m_szIniFileName,TEXT("LastChannel"),CSettings::OPEN_WRITE)) {
			TCHAR szDriverName[MAX_PATH],szName[32+MAX_PATH];

			::lstrcpy(szDriverName,::PathFindFileName(CoreEngine.GetDriverFileName()));
			*::PathFindExtension(szDriverName)='\0';
			::wsprintf(szName,TEXT("Space_%s"),szDriverName);
			Setting.Write(szName,Space);
			::wsprintf(szName,TEXT("ChannelIndex_%s"),szDriverName);
			Setting.Write(szName,Channel);
			::wsprintf(szName,TEXT("ServiceID_%s"),szDriverName);
			Setting.Write(szName,ServiceID);
			Setting.Close();
		}
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
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
		MainWindow.ShowErrorMessage(&RecordManager);
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	MainWindow.OnRecordingStart();
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
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
		RecordManager.CancelReserve();
		MainWindow.ShowErrorMessage(&RecordManager);
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	MainWindow.OnRecordingStart();
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


bool CAppMain::StopRecord()
{
	if (!RecordManager.IsRecording())
		return false;

	TCHAR szText[MAX_PATH+64],szFileName[MAX_PATH],szSize[32];
	::lstrcpy(szFileName,RecordManager.GetRecordTask()->GetFileName());

	RecordManager.StopRecord();
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(
						GeneralOptions.GetDescrambleCurServiceOnly());

	UInt64ToString(CoreEngine.m_DtvEngine.m_FileWriter.GetWriteSize(),
				   szSize,lengthof(szSize));
	::wsprintf(szText,TEXT("録画停止 %s (書き出しサイズ %s Bytes)"),
			   szFileName,szSize);
	Logger.AddLog(szText);

	ResidentManager.SetStatus(0,CResidentManager::STATUS_RECORDING);
	MainWindow.OnRecordingStop();
	PluginList.SendRecordStatusChangeEvent();
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


const CDriverManager *CAppMain::GetDriverManager() const
{
	return &DriverManager;
}


CAppMain &GetAppClass()
{
	return AppMain;
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

	if (MainWindow.IsWheelChannelChanging()) {
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
	UINT Flags;
	const CChannelList *pList;

	GetMenuPos(&pt,&Flags);
	Flags|=TPM_RIGHTBUTTON;
	if (!CoreEngine.IsNetworkDriver()
			&& (pList=ChannelManager.GetCurrentChannelList())!=NULL
			&& pList->NumEnableChannels()<=20) {
		ChannelMenu.Create(pList);
		ChannelMenu.Popup(Flags,pt.x,pt.y,MainWindow.GetHandle());
		ChannelMenu.Destroy();
	} else {
		MainMenu.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,Flags,pt.x,pt.y,
							  MainWindow.GetHandle());
	}
}

void CChannelStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_SERVICE,Flags | TPM_RIGHTBUTTON,
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
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_ZOOM,Flags | TPM_RIGHTBUTTON,
						  pt.x,pt.y,MainWindow.GetHandle());
}

void CVideoSizeStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO,Flags | TPM_RIGHTBUTTON,
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
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_VOLUME,Flags | TPM_RIGHTBUTTON,
						  pt.x,pt.y,MainWindow.GetHandle());
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
		LPTSTR p=szText;

		if (CoreEngine.m_DtvEngine.GetAudioStreamNum()>1)
			p+=wsprintf(szText,TEXT("#%d: "),CoreEngine.m_DtvEngine.GetAudioStream()+1);
		switch (NumChannels) {
		case 1:
			lstrcpy(p,TEXT("Mono"));
			break;
		case 2:
			{
				const int StereoMode=CoreEngine.GetStereoMode();

				if (CoreEngine.m_DtvEngine.GetAudioComponentType()==0x02) {
					wsprintf(p,TEXT("Dual (%s)"),
							 StereoMode==0?TEXT("主+副"):StereoMode==1?TEXT("主"):TEXT("副"));
				} else {
					lstrcpy(p,TEXT("Stereo"));
					if (StereoMode!=0)
						lstrcat(p,StereoMode==1?TEXT("(L)"):TEXT("(R)"));
				}
			}
			break;
		case 6:
			lstrcpy(p,TEXT("5.1ch"));
			break;
		default:
			wsprintf(p,TEXT("%dch"),NumChannels);
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
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_STEREOMODE,Flags | TPM_RIGHTBUTTON,
						  pt.x,pt.y,MainWindow.GetHandle());
}


class CRecordStatusItem : public CStatusItem {
	bool m_fRemain;

public:
	CRecordStatusItem();
// CStatusItem
	LPCTSTR GetName() const { return TEXT("録画"); }
	void Draw(HDC hdc,const RECT *pRect);
	void DrawPreview(HDC hdc,const RECT *pRect);
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
// CRecordStatusItem
	void ShowRemainTime(bool fRemain);
};

CRecordStatusItem::CRecordStatusItem() : CStatusItem(STATUS_ITEM_RECORD,64)
{
	m_fRemain=false;
}

void CRecordStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	int FontHeight=m_pStatus->GetFontHeight();
	RECT rc;
	TCHAR szText[32],*pszText;

	rc=*pRect;
	if (RecordManager.IsRecording()) {
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
		bool fRemain=m_fRemain && RecordManager.IsStopTimeSpecified();
		int RecordSec;
		if (fRemain) {
			RecordSec=(int)RecordManager.GetRemainTime()/1000;
			if (RecordSec<0)
				RecordSec=0;
		} else {
			RecordSec=(int)(RecordManager.GetRecordTime()/1000);
		}
		::wsprintf(szText,TEXT("%s%d:%02d:%02d"),
				   fRemain?TEXT("-"):TEXT(""),
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
	if (RecordManager.IsRecording() && !RecordManager.IsPaused()) {
		if (!RecordOptions.ConfirmStatusBarStop(MainWindow.GetVideoHostWindow()))
			return;
	}
	MainWindow.SendCommand(RecordManager.IsReserved()?CM_RECORDOPTION:CM_RECORD);
}

void CRecordStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;
	UINT Flags;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_RECORD));
	CheckMenuItem(hmenu,CM_RECORDEVENT,
		MF_BYCOMMAND | (MainWindow.GetRecordingStopOnEventEnd()?MFS_CHECKED:MFS_UNCHECKED));
	EnableMenuItem(hmenu,CM_RECORD_PAUSE,
		MF_BYCOMMAND | (RecordManager.IsRecording()?MFS_ENABLED:MFS_GRAYED));
	CheckMenuItem(hmenu,CM_RECORD_PAUSE,
		MF_BYCOMMAND | (RecordManager.IsPaused()?MFS_CHECKED:MFS_UNCHECKED));
	/*
	EnableMenuItem(hmenu,CM_RECORDSTOPTIME,
		MF_BYCOMMAND | (RecordManager.IsRecording()?MFS_ENABLED:MFS_GRAYED));
	*/
	EnableMenuItem(hmenu,CM_SHOWRECORDREMAINTIME,
		MF_BYCOMMAND | (RecordManager.IsRecording() && RecordManager.IsStopTimeSpecified()?MFS_ENABLED:MFS_GRAYED));
	CheckMenuItem(hmenu,CM_SHOWRECORDREMAINTIME,
		MF_BYCOMMAND | (m_fRemain?MFS_CHECKED:MFS_UNCHECKED));
	CheckMenuItem(hmenu,CM_EXITONRECORDINGSTOP,
		MF_BYCOMMAND | (MainWindow.GetExitOnRecordingStop()?MFS_CHECKED:MFS_UNCHECKED));
	CheckMenuItem(hmenu,CM_DISABLEVIEWER,
		MF_BYCOMMAND | (MainWindow.IsPreview()?MFS_UNCHECKED:MFS_CHECKED));
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt,&Flags);
	TrackPopupMenu(GetSubMenu(hmenu,0),Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
				   MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
}

void CRecordStatusItem::ShowRemainTime(bool fRemain)
{
	m_fRemain=fRemain;
	Update();
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
	UINT Flags;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_CAPTURE));
	CheckMenuRadioItem(hmenu,CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize(),MF_BYCOMMAND);
	if (fShowCaptureWindow)
		CheckMenuItem(hmenu,CM_CAPTUREPREVIEW,MF_BYCOMMAND | MFS_CHECKED);
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt,&Flags);
	TrackPopupMenu(GetSubMenu(hmenu,0),Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
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
	void OnRButtonDown(int x,int y);
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
	MainWindow.SendCommand(CM_RESETERRORCOUNT);
}

void CErrorStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;
	UINT Flags;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_ERROR));
	GetMenuPos(&pt,&Flags);
	TrackPopupMenu(GetSubMenu(hmenu,0),Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
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
			|| DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName())) {
		// ビットレートのみ
		wsprintf(szText,TEXT("%d.%02d Mbps"),BitRate/100,BitRate%100);
	} else {
		int Level=(int)(CoreEngine.GetSignalLevel()*100.0f);

		wsprintf(szText,TEXT("%d.%02d dB / %d.%02d Mbps"),
							Level/100,abs(Level)%100,BitRate/100,BitRate%100);
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
	void OnLButtonDown(int x,int y);
	void OnRButtonDown(int x,int y);
};

CClockStatusItem::CClockStatusItem() : CStatusItem(STATUS_ITEM_CLOCK,48)
{
}

void CClockStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	SYSTEMTIME st;
	TCHAR szText[64];

	if (StatusOptions.GetShowTOTTime()) {
		if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
			return;
		::wsprintf(szText,TEXT("TOT: %d/%d/%d %d:%02d:%02d"),
				   st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	} else {
		::GetLocalTime(&st);
		::wsprintf(szText,TEXT("%d:%02d:%02d"),st.wHour,st.wMinute,st.wSecond);
	}
	DrawText(hdc,pRect,szText);
}

void CClockStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	if (StatusOptions.GetShowTOTTime()) {
		SYSTEMTIME st;
		TCHAR szText[64];

		::GetLocalTime(&st);
		::wsprintf(szText,TEXT("TOT: %d/%d/%d 13:25:30"),st.wYear,st.wMonth,st.wDay);
		DrawText(hdc,pRect,szText);
	} else {
		DrawText(hdc,pRect,TEXT("13:25:30"));
	}
}

void CClockStatusItem::OnLButtonDown(int x,int y)
{
	MainWindow.SendCommand(CM_SHOWTOTTIME);
}

void CClockStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;
	UINT Flags;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_TIME));
	CheckMenuItem(hmenu,CM_SHOWTOTTIME,
		MF_BYCOMMAND | (StatusOptions.GetShowTOTTime()?MFS_CHECKED:MFS_UNCHECKED));
	GetMenuPos(&pt,&Flags);
	TrackPopupMenu(GetSubMenu(hmenu,0),Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
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
#if 0
	// EpgDataCap使用
	const CEpgDataInfo *pInfo=CoreEngine.GetEpgDataInfo(m_fNext);

	if (pInfo!=NULL) {
		TCHAR szText[256];
		SYSTEMTIME stStart,stEnd;

		pInfo->GetStartTime(&stStart);
		pInfo->GetEndTime(&stEnd);
		::wnsprintfW(szText,lengthof(szText),L"%s%d:%02d～%d:%02d %s",
					m_fNext?L"次: ":L"",
					stStart.wHour,
					stStart.wMinute,
					stEnd.wHour,
					stEnd.wMinute,
					pInfo->GetEventName());
		DrawText(hdc,pRect,szText);
	}
#else
	TCHAR szText[256];
	SYSTEMTIME stStart,stEnd;

	if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,&stEnd,m_fNext)) {
		int Length;

		Length=::wsprintf(szText,TEXT("%s%d:%02d～%d:%02d "),
						  m_fNext?TEXT("次: "):TEXT(""),
						  stStart.wHour,stStart.wMinute,
						  stEnd.wHour,stEnd.wMinute);
		if (!CoreEngine.m_DtvEngine.GetEventName(&szText[Length],lengthof(szText)-Length,m_fNext))
			szText[Length]='\0';
		DrawText(hdc,pRect,szText);
	}
#endif
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
	UINT Flags;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_BUFFERING));
	CheckMenuItem(hmenu,CM_ENABLEBUFFERING,
		MF_BYCOMMAND | (CoreEngine.GetPacketBuffering()?MFS_CHECKED:MFS_UNCHECKED));
	EnableMenuItem(hmenu,CM_RESETBUFFER,
		MF_BYCOMMAND | (CoreEngine.GetPacketBuffering()?MFS_ENABLED:MFS_GRAYED));
	GetMenuPos(&pt,&Flags);
	TrackPopupMenu(GetSubMenu(hmenu,0),Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
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
			ChannelManager.GetTuningSpaceName(
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
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	MainMenu.PopupSubMenu(CMainMenu::SUBMENU_SPACE,Flags | TPM_RIGHTBUTTON,
						  pt.x,pt.y,MainWindow.GetHandle());
}

void CTunerStatusItem::OnRButtonDown(int x,int y)
{
	HMENU hmenu=MainWindow.CreateTunerSelectMenu();
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	::TrackPopupMenu(hmenu,Flags | TPM_RIGHTBUTTON,pt.x,pt.y,0,
					 MainWindow.GetHandle(),NULL);
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
	Theme::GradientInfo Gradient1,Gradient2,Gradient3,Gradient4;

	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_STATUSBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_STATUSHIGHLIGHTBACK,&Gradient2);
	StatusView.SetColor(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
	StatusView.SetBorderType(pColorScheme->GetBorderType(CColorScheme::BORDER_STATUS));
	CaptureWindow.SetStatusColor(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSTEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_STATUSHIGHLIGHTTEXT));
	CaptureWindow.SetStatusBorderType(pColorScheme->GetBorderType(CColorScheme::BORDER_STATUS));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_TITLEBARBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_TITLEBARHIGHLIGHTBACK,&Gradient2);
	TitleBar.SetColor(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_TITLEBARTEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_TITLEBARHIGHLIGHTICON));
	TitleBar.SetBorderType(pColorScheme->GetBorderType(CColorScheme::BORDER_TITLEBAR));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_SIDEBARBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_SIDEBARHIGHLIGHTBACK,&Gradient2);
	SideBar.SetColor(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_SIDEBARICON),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_SIDEBARHIGHLIGHTICON));
	SideBar.SetBorderType(pColorScheme->GetBorderType(CColorScheme::BORDER_SIDEBAR));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PANELTITLEBACK,&Gradient1);
	PanelFrame.SetTitleColor(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTITLETEXT));
	InfoWindow.SetBackColors(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABMARGIN),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PANELTABBACK,&Gradient1);
	InfoWindow.SetTabColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABBORDER));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PANELCURTABBACK,&Gradient1);
	InfoWindow.SetCurTabColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_PANELCURTABTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELCURTABBORDER));
	InfoPanel.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT));
	InfoPanel.SetProgramInfoColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOTEXT));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMLISTBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMLISTCURBACK,&Gradient2);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMLISTTITLEBACK,&Gradient3);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMLISTCURTITLEBACK,&Gradient4);
	ProgramListPanel.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTTEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTCURTEXT),
		&Gradient3,
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTTITLETEXT),
		&Gradient4,
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMLISTCURTITLETEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CHANNELPANELCHANNELNAMEBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CHANNELPANELCURCHANNELNAMEBACK,&Gradient2);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CHANNELPANELEVENTNAMEBACK,&Gradient3);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CHANNELPANELCUREVENTNAMEBACK,&Gradient4);
	ChannelPanel.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELCHANNELNAMETEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELCURCHANNELNAMETEXT),
		&Gradient3,
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELEVENTNAMETEXT),
		&Gradient4,
		pColorScheme->GetColor(CColorScheme::COLOR_CHANNELPANELCUREVENTNAMETEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CONTROLPANELBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_CONTROLPANELHIGHLIGHTBACK,&Gradient2);
	ControlPanel.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELTEXT),
		&Gradient2,
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELHIGHLIGHTTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELMARGIN));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_NOTIFICATIONBARBACK,&Gradient1);
	NotificationBar.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARERRORTEXT));
	static const struct {
		int From,To;
	} ProgramGuideColorMap[] = {
		{CColorScheme::COLOR_PROGRAMGUIDEBACK,			CProgramGuide::COLOR_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDETEXT,			CProgramGuide::COLOR_TEXT},
		{CColorScheme::COLOR_PROGRAMGUIDECHANNELTEXT,	CProgramGuide::COLOR_CHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDECURCHANNELTEXT,	CProgramGuide::COLOR_CURCHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDETIMETEXT,		CProgramGuide::COLOR_TIMETEXT},
	};
	for (int i=0;i<lengthof(ProgramGuideColorMap);i++)
		ProgramGuide.SetColor(ProgramGuideColorMap[i].To,
							  pColorScheme->GetColor(ProgramGuideColorMap[i].From));
	for (int i=CProgramGuide::COLOR_CONTENT_FIRST,j=0;i<=CProgramGuide::COLOR_CONTENT_LAST;i++,j++)
		ProgramGuide.SetColor(i,pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_CONTENT_FIRST+j));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECHANNELBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECURCHANNELBACK,&Gradient2);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDETIMEBACK,&Gradient3);
	ProgramGuide.SetBackColor(&Gradient1,&Gradient2,&Gradient3);
	PluginList.SendColorChangeEvent();
	return true;
}




class COptionDialog : public COptionFrame {
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
	bool m_fSettingError;
	void CreatePage(int Page);
	void SetPage(int Page);
	static COptionDialog *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
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


BOOL CALLBACK COptionDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COptionDialog *pThis=reinterpret_cast<COptionDialog*>(lParam);
			int i;

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;
			COptions::ClearGeneralUpdateFlags();
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
		/*
		if (m_UpdateFlags&COptions::UPDATE_PREVIEW) {
			//CoreEngine.m_DtvEngine.SetChannel(0,0);
			if (MainWindow.IsPreview())
				CoreEngine.EnablePreview(true);
		}
		*/
		return false;
	}
	MainWindow.Update();
	for (int i=0;i<NUM_PAGES;i++) {
		DWORD Flags=m_PageList[i].pOptions->GetUpdateFlags();

		if (Flags!=0)
			m_PageList[i].pOptions->Apply(Flags);
	}
	if ((COptions::GetGeneralUpdateFlags()&COptions::UPDATE_GENERAL_BUILDMEDIAVIEWER)!=0) {
		if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
				|| MainWindow.IsMediaViewerBuildError()) {
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			MainWindow.BuildMediaViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
		}
	}
	/*
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
			pNetworkRemocon->GetChannel(&GetChannelReceiver);
	}
	if (m_UpdateFlags&COptions::UPDATE_PREVIEW) {
		if (MainWindow.IsPreview())
			CoreEngine.EnablePreview(true);
	}
	*/
	if (NetworkRemoconOptions.GetUpdateFlags()!=0) {
		AppMain.InitializeChannel();
		if (pNetworkRemocon!=NULL)
			pNetworkRemocon->GetChannel(&GetChannelReceiver);
	}
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	AppMain.SaveSettings();
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




class CMyPanelEventHandler : public CPanelFrameEventHandler {
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
	// CPanelFrameEventHandler
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
{
	m_SnapEdge=EDGE_NONE;
}

bool CMyPanelEventHandler::OnClose()
{
	MainWindow.SendCommand(CM_INFORMATION);
	return false;
}

bool CMyPanelEventHandler::OnMoving(RECT *pRect)
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

bool CMyPanelEventHandler::OnEnterSizeMove()
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
	int Size;
	RECT rc;

	PanelPaneIndex=Splitter.IDToIndex(PANE_ID_PANEL);
	if (fFloating)
		Splitter.SetPaneVisible(PANE_ID_PANEL,false);
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
			OffsetRect(&rc,XOffset,YOffset);
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


class CMyInfoPanelEventHandler : public CInfoPanelEventHandler {
public:
	void OnSelChange();
	void OnRButtonDown();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
};

void CMyInfoPanelEventHandler::OnSelChange()
{
	MainWindow.UpdatePanel();
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


class CMyInformationEventHandler : public CInformationPanel::CEventHandler {
public:
	bool OnProgramInfoUpdate(bool fNext) {
		MainWindow.UpdateProgramInfo();
		return true;
	}
};


class CMyChannelPanelEventHandler : public CChannelPanel::CEventHandler {
public:
	void OnChannelClick(const CChannelInfo *pChannelInfo);
	void OnRButtonDown();
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
								  pChannelInfo->GetServiceID());
			if (Index<0 && pChannelInfo->GetServiceID()>0)
				Index=pList->Find(pChannelInfo->GetSpace(),
								  pChannelInfo->GetChannelIndex());
			if (Index>=0)
				MainWindow.PostCommand(CM_CHANNEL_FIRST+Index);
		}
	}
}

void CMyChannelPanelEventHandler::OnRButtonDown()
{
	POINT pt;
	HMENU hmenu=::LoadMenu(hInst,MAKEINTRESOURCE(IDM_CHANNELPANEL));

	::GetCursorPos(&pt);
	::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	::DestroyMenu(hmenu);
}


class CMyEpgLoadEventHandler : public CEpgOptions::CEpgLoadEventHandler {
public:
	void OnEnd(bool fSuccess) {
		MainWindow.PostMessage(WM_APP_EPGFILELOADED,fSuccess,0);
	}
};


static CMyPanelEventHandler PanelEventHandler;
static CMyInfoPanelEventHandler InfoPanelEventHandler;
static CMyInformationEventHandler InformationEventHandler;
static CMyChannelPanelEventHandler ChannelPanelEventHandler;
static CMyEpgLoadEventHandler EpgLoadEventHandler;




class CMyProgramGuideEventHandler : public CProgramGuideEventHandler {
public:
	bool OnClose();
	void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo);
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

void CMyProgramGuideEventHandler::OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo)
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

bool CMyProgramGuideEventHandler::OnBeginUpdate()
{
	if (CoreEngine.IsNetworkDriver()) {
		MainWindow.ShowMessage(TEXT("UDP/TCPでは番組表の取得はできません。"),
							   TEXT("ごめん"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	if (!CoreEngine.m_DtvEngine.m_TsPacketParser.IsEpgDataCapLoaded()) {
		MainWindow.ShowMessage(TEXT("EpgDataCap2.dllが読み込まれていません。\n番組表の取得にはEpgDataCap2.dllが必要です。"),NULL,MB_OK | MB_ICONINFORMATION);
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
	MainWindow.OnProgramGuideUpdateEnd();
}

bool CMyProgramGuideEventHandler::OnRefresh()
{
	HCURSOR hcurOld;

	hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	EpgProgramList.UpdateProgramList();
	m_pProgramGuide->UpdateChannelList();
	m_pProgramGuide->UpdateProgramGuide();
	::SetCursor(hcurOld);
	return true;
}

bool CMyProgramGuideEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


static CMyProgramGuideEventHandler ProgramGuideEventHandler;


class CStreamInfoEventHandler : public CStreamInfo::CEventHandler {
	bool OnClose();
};

bool CStreamInfoEventHandler::OnClose()
{
	MainMenu.CheckItem(CM_STREAMINFO,false);
	return true;
}


static CStreamInfoEventHandler StreamInfoEventHandler;




class CBarLayout {
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


class CMyStatusViewEventHandler : public CStatusView::CEventHandler {
public:
	void OnMouseLeave()
	{
		if (!MainWindow.GetFullscreen() && !MainWindow.GetStatusBarVisible())
			m_pStatusView->SetVisible(false);
	}
};


class CTitleBarUtil : public CTitleBarEventHandler, public CBarLayout {
	bool m_fFixed;
	void ShowSystemMenu(int x,int y);
	bool IsMainWindowTitleBar() const
	{
		return m_pTitleBar->GetParent()==Splitter.GetHandle();
	}

public:
	CTitleBarUtil();
// CTitleBarEventHandler
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

CTitleBarUtil::CTitleBarUtil()
{
	m_fFixed=false;
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
	MainWindow.SetFullscreen(!MainWindow.GetFullscreen());
	return true;
}

void CTitleBarUtil::OnMouseLeave()
{
	if (!m_fFixed && !MainWindow.GetTitleBarVisible()
			&& IsMainWindowTitleBar())
		m_pTitleBar->SetVisible(false);
}

void CTitleBarUtil::OnLabelLButtonDown(int x,int y)
{
	if (IsMainWindowTitleBar()) {
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
	if (IsMainWindowTitleBar())
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


class CSideBarUtil : public CSideBar::CEventHandler, public CBarLayout {
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
		HMENU hmenu=::LoadMenu(hInst,MAKEINTRESOURCE(IDM_SIDEBAR));
		POINT pt;

		::CheckMenuItem(hmenu,CM_SIDEBAR,MF_BYCOMMAND | (MainWindow.GetSideBarVisible()?MFS_CHECKED:MFS_UNCHECKED));
		::CheckMenuRadioItem(hmenu,CM_SIDEBAR_PLACE_FIRST,CM_SIDEBAR_PLACE_LAST,
							   CM_SIDEBAR_PLACE_FIRST+(int)SideBarOptions.GetPlace(),MF_BYCOMMAND);
		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pSideBar->GetHandle(),&pt);
		m_fFixed=true;
		::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,MainWindow.GetHandle(),NULL);
		::DestroyMenu(hmenu);
		m_fFixed=false;
		RECT rc;
		m_pSideBar->GetScreenPosition(&rc);
		::GetCursorPos(&pt);
		if (!::PtInRect(&rc,pt))
			OnMouseLeave();
	}

	void OnMouseLeave()
	{
		if (!m_fFixed && !MainWindow.GetFullscreen()
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
static CTitleBarUtil TitleBarUtil;
static CTitleBarUtil FullscreenTitleBarUtil;
static CSideBarUtil SideBarUtil;


class CMySplitterEventHandler : public CSplitter::CEventHandler {
public:
	void OnResizePane(int Index,RECT *pRect)
	{
		if (m_pSplitter->GetPaneID(Index)==PANE_ID_VIEW) {
			RECT rcBar;

			if (MainWindow.GetTitleBarVisible() && MainWindow.GetCustomTitleBar()) {
				TitleBarUtil.Layout(pRect,&rcBar);
				TitleBar.SetPosition(&rcBar);
			}
			if (MainWindow.GetSideBarVisible()) {
				SideBarUtil.Layout(pRect,&rcBar);
				SideBar.SetPosition(&rcBar);
			}
			if (pRect->right<pRect->left)
				pRect->right=pRect->left;
			if (pRect->bottom<pRect->top)
				pRect->bottom=pRect->top;
		}
	}
};

static CMySplitterEventHandler SplitterEventHandler;




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


CFullscreen::CFullscreen()
	: m_pVideoContainer(NULL)
	, m_pViewWindow(NULL)
{
}


CFullscreen::~CFullscreen()
{
	Destroy();
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
			pThis->m_TitleBar.Create(pThis->m_pVideoContainer->GetHandle(),
									 WS_CHILD | WS_CLIPSIBLINGS,0,IDC_TITLEBAR);
			pThis->m_TitleBar.SetEventHandler(&FullscreenTitleBarUtil);
			pThis->m_TitleBar.SetIcon(::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			pThis->m_fShowCursor=true;
			pThis->m_fMenu=false;
			pThis->m_fShowStatusView=false;
			pThis->m_fShowTitleBar=false;
			pThis->m_fShowSideBar=false;
			pThis->m_LastCursorMovePos.x=-2;
			pThis->m_LastCursorMovePos.y=-2;
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

	case WM_MBUTTONDOWN:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnMButtonDown();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnLButtonDoubleClick();
		}
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
	case WM_MOUSEHWHEEL:
		{
			CFullscreen *pThis=GetThis(hwnd);

			MainWindow.OnMouseWheel(wParam,lParam,uMsg==WM_MOUSEHWHEEL,pThis->m_fShowStatusView);
		}
		return 0;

#if 0	// これはやらない方がいい気がする
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp=reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter=HWND_TOPMOST;
		}
		return 0;
#endif

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			MainWindow.SendCommand(CM_FULLSCREEN);
			return 0;
		}
	case WM_COMMAND:
		return MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam&0xFFFFFFF0) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver())
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
			pThis->m_TitleBar.Destroy();
			pThis->ShowStatusView(false);
			pThis->ShowSideBar(false);
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


void CFullscreen::OnMouseCommand(int Command)
{
	if (Command==0)
		return;
	// メニュー表示中はカーソルを消さない
	KillTimer(m_hwnd,1);
	m_fShowCursor=true;
	CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
	::SetCursor(LoadCursor(NULL,IDC_ARROW));
	m_fMenu=true;
	//MainWindow.PopupMenu();
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
	if (abs(m_LastCursorMovePos.x-pt.x)<2 && abs(m_LastCursorMovePos.y-pt.y)<2)
		return;
	m_LastCursorMovePos=pt;
	if (!m_fShowCursor) {
		::SetCursor(LoadCursor(NULL,IDC_ARROW));
		CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
		m_fShowCursor=true;
	}
	GetClientRect(&rcClient);
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
	::SetTimer(m_hwnd,1,1000,NULL);
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;
	if (fShow) {
		RECT rc;

		ShowSideBar(false);
		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetVisible(false);
		StatusView.SetParent(m_pVideoContainer);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
		::BringWindowToTop(StatusView.GetHandle());
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
		RECT rc,rcBar;
		const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
		Theme::GradientInfo Gradient1,Gradient2;

		ShowSideBar(false);
		GetClientRect(&rc);
		FullscreenTitleBarUtil.Layout(&rc,&rcBar);
		m_TitleBar.SetPosition(&rcBar);
		m_TitleBar.SetLabel(TitleBar.GetLabel());
		pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_TITLEBARBACK,&Gradient1);
		pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_TITLEBARHIGHLIGHTBACK,&Gradient2);
		m_TitleBar.SetColor(
			&Gradient1,
			pColorScheme->GetColor(CColorScheme::COLOR_TITLEBARTEXT),
			&Gradient2,
			pColorScheme->GetColor(CColorScheme::COLOR_TITLEBARHIGHLIGHTICON));
		m_TitleBar.SetBorderType(pColorScheme->GetBorderType(CColorScheme::BORDER_TITLEBAR));
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
	if (fShow) {
		RECT rcClient,rcBar;

		ShowStatusView(false);
		ShowTitleBar(false);
		GetClientRect(&rcClient);
		SideBarUtil.Layout(&rcClient,&rcBar);
		SideBar.SetVisible(false);
		SideBar.SetParent(m_pVideoContainer);
		SideBar.SetPosition(&rcBar);
		SideBar.SetVisible(true);
		::BringWindowToTop(SideBar.GetHandle());
	} else {
		SideBar.SetVisible(false);
		SideBar.SetParent(&Splitter);
		if (MainWindow.GetSideBarVisible()) {
			SIZE sz;

			Splitter.GetClientSize(&sz);
			Splitter.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			SideBar.SetVisible(true);
		}
	}
	m_fShowSideBar=fShow;
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
	bool m_fStreamChanged;
	CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer);
	~CServiceUpdateInfo();
};

CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer)
{
	CTsAnalyzer::CServiceList ServiceList;

	pTsAnalyzer->GetViewableServiceList(&ServiceList);
	m_NumServices=ServiceList.NumServices();
	m_CurService=-1;
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=ServiceList.GetServiceInfo(i);
			m_pServiceList[i].ServiceID=pServiceInfo->ServiceID;
			::lstrcpy(m_pServiceList[i].szServiceName,pServiceInfo->szServiceName);
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
	void OnFileWriteError(CFileWriter *pFileWriter);
	void OnVideoSizeChanged(CMediaViewer *pMediaViewer);
	void OnEmmProcessed(const BYTE *pEmmData);
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

#ifdef TBS_FILTER_INTERFACE
	bool fEnable=TBSFilterTSIDList.HasTSID(pProgManager->GetTransportStreamID());

	CoreEngine.m_DtvEngine.m_MediaViewer.EnableTBSFilter(fEnable);
	MainMenu.CheckItem(CM_TBSFILTER,fEnable);
#endif
}

void CMyDtvEngineHandler::OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer)
{
	OnServiceUpdated(pTsAnalyzer,false,false);
}

void CMyDtvEngineHandler::OnFileWriteError(CFileWriter *pFileWriter)
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




const BYTE CMainWindow::VolumeNormalizeLevelList[] = {100, 125, 150, 200};
const CMainWindow::ZoomRateInfo CMainWindow::ZoomRateList[] = {
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
	: m_PreviewManager(&m_VideoContainer)
	, m_ChannelPanelTimer(TIMER_ID_CHANNELPANELUPDATE)
{
	// 適当にデフォルト位置を設定
#ifndef TVH264
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
#else
	m_WindowPosition.Width=400;
	m_WindowPosition.Height=320;
#endif
	m_WindowPosition.Left=(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
	m_fFullscreen=false;
	m_fMaximize=false;
	m_fAlwaysOnTop=false;
	m_fShowStatusBar=true;
	m_fShowTitleBar=true;
	m_fCustomTitleBar=true;
	m_fShowSideBar=false;
	m_fStandby=false;
	m_fStandbyInit=false;
	m_fMinimizeInit=false;
	m_fSrcFilterReleased=false;
	m_fRestorePreview=false;
	m_fProgramGuideUpdating=false;
	m_fRecordingStopOnEventEnd=false;
	m_fExitOnRecordingStop=false;
	m_fClosing=false;
	m_fWheelChannelChanging=false;
	m_fScreenSaverActive=FALSE;
	m_fLowPowerActiveOriginal=FALSE;
	m_fPowerOffActiveOriginal=FALSE;
	m_AspectRatioType=0;
	m_fShowRecordRemainTime=false;
	m_ProgramListUpdateTimerCount=0;
	m_fViewerBuildError=false;
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
	const bool fOldPreview=IsPreview();

	if (m_PreviewManager.BuildMediaViewer()) {
		TCHAR szText[256];

		if (CoreEngine.m_DtvEngine.GetVideoDecoderName(szText,lengthof(szText)))
			InfoPanel.SetVideoDecoderName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererName(szText,lengthof(szText)))
			InfoPanel.SetVideoRendererName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioRendererName(szText,lengthof(szText)))
			InfoPanel.SetAudioDeviceName(szText);
		if (fOldPreview)
			m_PreviewManager.EnablePreview(true);
		m_fViewerBuildError=false;
	} else {
		InfoPanel.SetVideoDecoderName(NULL);
		InfoPanel.SetVideoRendererName(NULL);
		InfoPanel.SetAudioDeviceName(NULL);
		m_fViewerBuildError=true;
	}
	MainMenu.CheckItem(CM_DISABLEVIEWER,!IsPreview());

	return true;
}


bool CMainWindow::CloseMediaViewer()
{
	m_PreviewManager.CloseMediaViewer();
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	return true;
}


bool CMainWindow::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (fFullscreen) {
			if (::IsIconic(m_hwnd))
				::ShowWindow(m_hwnd,SW_RESTORE);
			if (!m_Fullscreen.Create(m_hwnd,&m_VideoContainer,&m_ViewWindow))
				return false;
		} else {
			ForegroundWindow(m_hwnd);
			m_Fullscreen.Destroy();
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
		return m_Fullscreen.GetHandle();
	return m_hwnd;
}


int CMainWindow::ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption,UINT Type) const
{
	return ::MessageBox(GetVideoHostWindow(),pszText,pszCaption,Type);
}


void CMainWindow::ShowErrorMessage(LPCTSTR pszText)
{
	MessageDialog.Show(GetVideoHostWindow(),CMessageDialog::TYPE_WARNING,pszText);
}


void CMainWindow::ShowErrorMessage(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	TCHAR szText[1024];

	pErrorHandler->FormatLastErrorText(szText,lengthof(szText));
	MessageDialog.Show(GetVideoHostWindow(),CMessageDialog::TYPE_WARNING,szText,
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
#if 0
		rc.right+=GetSystemMetrics(SM_CXEDGE)*2;
		rc.bottom+=GetSystemMetrics(SM_CYEDGE)*2;
#else
		rc.right+=m_ViewWindow.GetVerticalEdgeWidth()*2;
		rc.bottom+=m_ViewWindow.GetHorizontalEdgeHeight()*2;
#endif
	}
	if (m_fShowStatusBar)
		rc.bottom+=StatusView.GetHeight();
	if (m_fShowTitleBar && m_fCustomTitleBar)
		TitleBarUtil.ReserveArea(&rc,true);
	if (m_fShowSideBar)
		SideBarUtil.ReserveArea(&rc,true);
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
		SetMaximizeStatus(f);
	if (pSettings->Read(TEXT("AlwaysOnTop"),&f))
		SetAlwaysOnTop(f);
	if (pSettings->Read(TEXT("ShowStatusBar"),&f))
		SetStatusBarVisible(f);
	if (pSettings->Read(TEXT("ShowTitleBar"),&f))
		SetTitleBarVisible(f);
	if (pSettings->Read(TEXT("CustomTitleBar"),&f))
		SetCustomTitleBar(f);
	if (pSettings->Read(TEXT("ShowSideBar"),&f))
		SetSideBarVisible(f);
	pSettings->Read(TEXT("ShowRecordRemainTime"),&m_fShowRecordRemainTime);
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
	pSettings->Write(TEXT("WindowMaximize"),m_fMaximize);
	pSettings->Write(TEXT("AlwaysOnTop"),m_fAlwaysOnTop);
	pSettings->Write(TEXT("ShowStatusBar"),m_fShowStatusBar);
	pSettings->Write(TEXT("ShowTitleBar"),m_fShowTitleBar);
	pSettings->Write(TEXT("CustomTitleBar"),m_fCustomTitleBar);
	pSettings->Write(TEXT("ShowSideBar"),m_fShowSideBar);
	pSettings->Write(TEXT("ShowRecordRemainTime"),m_fShowRecordRemainTime);
	return true;
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
			if (!m_fCustomTitleBar)
				SetStyle(GetStyle()^WS_CAPTION,fMaximize);
			else if (!fVisible)
				TitleBar.SetVisible(false);
			if (!fMaximize) {
				int CaptionHeight;

				if (!m_fCustomTitleBar)
					CaptionHeight=::GetSystemMetrics(SM_CYCAPTION);
				else
					CaptionHeight=TitleBar.GetHeight();

				if (fVisible)
					rc.top-=CaptionHeight;
				else
					rc.top+=CaptionHeight;
				::SetWindowPos(m_hwnd,NULL,rc.left,rc.top,
							rc.right-rc.left,rc.bottom-rc.top,
							SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			} else if (m_fCustomTitleBar) {
				Splitter.GetClientRect(&rc);
				Splitter.SendMessage(WM_SIZE,0,MAKELONG(rc.right,rc.bottom));
			}
			if (m_fCustomTitleBar && fVisible)
				TitleBar.SetVisible(true);
			::MainMenu.CheckItem(CM_TITLEBAR,fVisible);
		}
	}
}


void CMainWindow::SetCustomTitleBar(bool fCustom)
{
	if (m_fCustomTitleBar!=fCustom) {
		m_fCustomTitleBar=fCustom;
		if (m_hwnd!=NULL) {
			if (m_fShowTitleBar) {
				if (!fCustom)
					TitleBar.SetVisible(false);
				SetStyle(GetStyle()^WS_CAPTION,true);
				if (fCustom) {
					/*
					RECT rc;
					GetClientRect(&rc);
					SendMessage(WM_SIZE,0,MAKELONG(rc.right,rc.bottom));
					*/
					TitleBar.SetVisible(true);
				}
			}
			MainMenu.CheckItem(CM_CUSTOMTITLEBAR,fCustom);
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
				SideBar.SetVisible(false);
			if (::IsZoomed(m_hwnd)) {
				GetClientRect(&rc);
				SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
			} else {
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
				SideBar.SetVisible(true);
			MainMenu.CheckItem(CM_SIDEBAR,fVisible);
		}
	}
}


bool CMainWindow::OnCreate(const CREATESTRUCT *pcs)
{
	Splitter.Create(m_hwnd,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CSplitter::STYLE_VERT);

	m_ViewWindow.Create(Splitter.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,
		/*ViewOptions.GetClientEdge()?WS_EX_CLIENTEDGE:0*/0,IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_ViewWindow.SetEdge(ViewOptions.GetClientEdge());
	m_VideoContainer.Create(m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN,0,IDC_VIDEOCONTAINER,&CoreEngine.m_DtvEngine);
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);

	TitleBar.Create(Splitter.GetHandle(),
					WS_CHILD | WS_CLIPSIBLINGS | (m_fShowTitleBar && m_fCustomTitleBar?WS_VISIBLE:0),
					/*WS_EX_STATICEDGE*/0,IDC_TITLEBAR);
	TitleBar.SetEventHandler(&TitleBarUtil);
	TitleBar.SetLabel(pcs->lpszName);
	TitleBar.SetIcon(::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
	TitleBar.SetMaximizeMode((pcs->style&WS_MAXIMIZE)!=0);

	Splitter.SetEventHandler(&SplitterEventHandler);
	Splitter.SetPane(!PanelPaneIndex,&m_ViewWindow,PANE_ID_VIEW);
	Splitter.SetMinSize(PANE_ID_VIEW,32);
	Splitter.SetPaneVisible(PANE_ID_VIEW,true);

	StatusView.Create(m_hwnd,
		//WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		/*WS_EX_STATICEDGE*/0,IDC_STATUS);
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

	NotificationBar.Create(m_VideoContainer.GetHandle(),WS_CHILD | WS_CLIPSIBLINGS);

	SideBarOptions.ApplySideBarOptions();
	SideBar.SetEventHandler(&SideBarUtil);
	SideBar.Create(Splitter.GetHandle(),WS_CHILD | WS_CLIPSIBLINGS | (m_fShowSideBar?WS_VISIBLE:0),0,IDC_SIDEBAR);

	MainMenu.Create(hInst);
	/*
	MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
							CM_ASPECTRATIO_FIRST+m_AspectRatioType);
	*/
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
	MainMenu.CheckItem(CM_SIDEBAR,m_fShowSideBar);
	MainMenu.CheckItem(CM_CUSTOMTITLEBAR,m_fCustomTitleBar);

	HMENU hSysMenu;
	hSysMenu=GetSystemMenu(m_hwnd,FALSE);
	AppendMenu(hSysMenu,MFT_SEPARATOR,0,NULL);
	AppendMenu(hSysMenu,MFT_STRING | MFS_ENABLED,SC_ABOUT,
												TEXT("バージョン情報(&A)..."));

	AspectRatioIconMenu.Initialize(MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO),
								   hInst,MAKEINTRESOURCE(IDB_PANSCAN),16,RGB(192,192,192));
	AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_FIRST+m_AspectRatioType);

	TaskbarManager.Initialize(m_hwnd);

	::SetTimer(m_hwnd,TIMER_ID_UPDATE,UPDATE_TIMER_INTERVAL,NULL);
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
			m_AspectRatioType=i;
			/*
			MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
									CM_ASPECTRATIO_FIRST+m_AspectRatioType);
			*/
			AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_FIRST+m_AspectRatioType);
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
				Volume=CoreEngine.GetVolume()+OperationOptions.GetVolumeStep();
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume=CoreEngine.GetVolume()-OperationOptions.GetVolumeStep();
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
		PluginList.SendResetEvent();
		return;

	case CM_RESETVIEWER:
		CoreEngine.m_DtvEngine.ResetMediaViewer();
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
						TEXT("既に設定されている録画があります。\r\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\r\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION)!=IDOK) {
					m_fRecordingStopOnEventEnd=false;
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

	/*
	case CM_RECORDSTOPTIME:
		if (RecordManager.ChangeStopTimeDialog(GetVideoHostWindow())) {
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
		}
		return;
	*/

	case CM_RECORDEVENT:
		if (RecordManager.IsRecording()) {
			m_fRecordingStopOnEventEnd=!m_fRecordingStopOnEventEnd;
		} else {
			m_fRecordingStopOnEventEnd=true;
			SendCommand(CM_RECORD_START);
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
		EnablePreview(!IsPreview());
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

	case CM_INFORMATION:
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
		if (fShowPanelWindow)
			UpdatePanel();
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
			ProgramGuide.SetViewDay(CProgramGuide::DAY_TODAY);
			const CTuningSpaceList *pList=ChannelManager.GetTuningSpaceList();
			int Space;
			if (!CoreEngine.IsNetworkDriver())
				Space=ChannelManager.GetCurrentSpace();
			else
				Space=-1;
			EpgProgramList.UpdateProgramList();
			ProgramGuide.SetTuningSpaceList(CoreEngine.GetDriverFileName(),pList,Space);
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

	case CM_SIDEBAR:
		SetSideBarVisible(!m_fShowSideBar);
		return;

	case CM_CUSTOMTITLEBAR:
		SetCustomTitleBar(!m_fCustomTitleBar);
		return;

	case CM_DECODERPROPERTY:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==NULL || IsWindowEnabled(hwndOwner))
				CoreEngine.m_DtvEngine.DisplayVideoDecoderProperty(hwndOwner);
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

				if (pList->HasRemoteControlKeyID() && pInfo->GetChannelNo()!=0)
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNEL_FIRST+pInfo->GetChannelIndex());
			} else {
				SendCommand(CM_CHANNEL_FIRST);
			}
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
				::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
			}
			PopupMenu(&pt);
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

	case CM_ENABLEBUFFERING:
		CoreEngine.SetPacketBuffering(!CoreEngine.GetPacketBuffering());
		GeneralOptions.SetPacketBuffering(CoreEngine.GetPacketBuffering());
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
			CRecordStatusItem *pItem=dynamic_cast<CRecordStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_RECORD));

			if (pItem!=NULL) {
				m_fShowRecordRemainTime=!m_fShowRecordRemainTime;
				pItem->ShowRemainTime(m_fShowRecordRemainTime);
			}
		}
		return;

	case CM_SHOWTOTTIME:
		StatusOptions.SetShowTOTTime(!StatusOptions.GetShowTOTTime());
		StatusView.UpdateItem(STATUS_ITEM_CLOCK);
		return;

	case CM_ADJUSTTOTTIME:
		TotTimeAdjuster.BeginAdjust();
		return;

	case CM_CHANNELPANEL_UPDATE:
		::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		ChannelPanel.UpdateChannelList(true);
		::SetCursor(::LoadCursor(NULL,IDC_ARROW));
		return;

	case CM_CHANNELMENU:
		{
			POINT pt;
			const CChannelList *pList;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
			}
			if (!CoreEngine.IsNetworkDriver()
					&& (pList=ChannelManager.GetCurrentChannelList())!=NULL
					&& pList->NumEnableChannels()<=20) {
				ChannelMenu.Create(pList);
				ChannelMenu.Popup(TPM_RIGHTBUTTON,pt.x,pt.y,MainWindow.GetHandle());
				ChannelMenu.Destroy();
			} else {
				MainMenu.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,TPM_RIGHTBUTTON,
									  pt.x,pt.y,MainWindow.GetHandle());
			}
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
				::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
			}
			::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
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
				::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
			}
			::ClientToScreen(m_ViewWindow.GetHandle(),&pt);
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
				SideBarOptions.SetPlace(Place);
				SideBar.SetVertical(Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_RIGHT);
				if (m_fShowSideBar)
					Splitter.SetPaneSize(PANE_ID_VIEW,Splitter.GetPaneSize(PANE_ID_VIEW));
			}
		}
		return;

	case CM_SIDEBAROPTIONS:
		if (::IsWindowEnabled(hwnd))
			OptionDialog.ShowDialog(hwnd,COptionDialog::PAGE_SIDEBAR);
		return;

	case CM_DRIVER_BROWSE:
		{
			CMainWindow *pThis=GetThis(hwnd);
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
			ofn.hwndOwner=pThis->GetVideoHostWindow();
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
		ChannelHistory.Clear();
		return;

#ifdef TBS_FILTER_INTERFACE
	case CM_TBSFILTER:
		{
			bool fEnable=!CoreEngine.m_DtvEngine.m_MediaViewer.IsTBSFilterEnabled();

			if (CoreEngine.m_DtvEngine.m_MediaViewer.EnableTBSFilter(fEnable)) {
				WORD TSID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();

				if (fEnable)
					TBSFilterTSIDList.Add(TSID);
				else
					TBSFilterTSIDList.Remove(TSID);
				MainMenu.CheckItem(CM_TBSFILTER,fEnable);
			}
		}
		return;
#endif

	default:
		if (id>=CM_AUDIOSTREAM_FIRST && id<=CM_AUDIOSTREAM_LAST) {
			if (CoreEngine.m_DtvEngine.SetAudioStream(id-CM_AUDIOSTREAM_FIRST)) {
				MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
										CM_AUDIOSTREAM_FIRST+CoreEngine.m_DtvEngine.GetAudioStreamNum()-1,
										id);
				PluginList.SendAudioStreamChangeEvent(id-CM_AUDIOSTREAM_FIRST);
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
				OnChannelChanged();
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

			if (Space!=ChannelManager.GetCurrentSpace())
				AppMain.SetChannel(Space,0);
			return;
		}
		if (id>=CM_DRIVER_FIRST && id<=CM_DRIVER_LAST) {
			int Driver=id-CM_DRIVER_FIRST;

			if (Driver<DriverManager.NumDrivers()) {
				const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(Driver);

				if (!CoreEngine.IsDriverOpen()
						|| ::lstrcmpi(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())!=0) {
					if (AppMain.SetDriver(pDriverInfo->GetFileName())) {
						if (RestoreChannelInfo.Space>=0
								&& RestoreChannelInfo.Channel>=0) {
							const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
							if (pList!=NULL) {
								int Index=pList->Find(RestoreChannelInfo.Space,
													  RestoreChannelInfo.Channel,
													  RestoreChannelInfo.ServiceID);
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
			ProcessTunerSelectMenu(id);
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
				}
				const CChannelList *pList=ChannelManager.GetChannelList(pChannelInfo->GetSpace());
				if (pList!=NULL) {
					int Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),
										  pChannelInfo->GetServiceID());

					if (Index>=0)
						AppMain.SetChannel(pChannelInfo->GetSpace(),Index);
				}
			}
			return;
		}
		if (id>=CM_PLUGINCOMMAND_FIRST && id<=CM_PLUGINCOMMAND_LAST) {
			PluginList.OnPluginCommand(CommandList.GetCommandText(CommandList.IDToIndex(id)));
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
				CheckZoomMenu();
			}

			if ((UpdateStatus&(CCoreEngine::STATUS_AUDIOCHANNELS
							 | CCoreEngine::STATUS_AUDIOSTREAMS
							 | CCoreEngine::STATUS_AUDIOCOMPONENTTYPE))!=0) {
				SetStereoMode(CoreEngine.m_DtvEngine.GetAudioComponentType()==0x02?1:0);
				StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
			}

			if ((UpdateStatus&CCoreEngine::STATUS_EVENTID)!=0) {
				TCHAR szText[256],szTitle[256+64];

				if (m_fRecordingStopOnEventEnd)
					AppMain.StopRecord();

				::lstrcpy(szTitle,TITLE_TEXT);
				if (pChInfo!=NULL) {
					::lstrcat(szTitle,TEXT(" - "));
					::lstrcat(szTitle,pChInfo->GetName());
				}
				if (CoreEngine.m_DtvEngine.GetEventName(szText,lengthof(szText))>0) {
					if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_EVENTNAME)) {
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
						NotificationBar.SetFont(OSDOptions.GetNotificationBarFont());
						NotificationBar.SetText(szBarText);
						NotificationBar.Show(OSDOptions.GetNotificationBarDuration());
					}
					::lstrcat(szTitle,TEXT(" / "));
					::lstrcat(szTitle,szText);
				}
				::SetWindowText(m_hwnd,szTitle);

				ProgramListPanel.SetCurrentEventID(CoreEngine.m_DtvEngine.GetEventID());

				CoreEngine.UpdateEpgDataInfo();
				StatusView.UpdateItem(STATUS_ITEM_PROGRAMINFO);

				if (ViewOptions.GetResetPanScanEventChange()
						&& m_AspectRatioType!=0) {
					CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(0,0);
					if (!m_fFullscreen && !::IsZoomed(hwnd)) {
						SIZE sz;
						int Width,Height;

						m_VideoContainer.GetClientSize(&sz);
						if (CoreEngine.GetVideoViewSize(&Width,&Height))
							AdjustWindowSize(Width*sz.cy/Height,sz.cy);
					}
					m_AspectRatioType=0;
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
					//MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,CM_ASPECTRATIO_DEFAULT);
					AspectRatioIconMenu.SetCheckItem(CM_ASPECTRATIO_DEFAULT);
					CheckZoomMenu();
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

			if (fShowPanelWindow && InfoWindow.GetVisible()) {
				// パネルの更新
				if (InfoWindow.GetCurTab()==PANEL_TAB_INFORMATION) {
					// 情報タブ更新
					BYTE AspectX,AspectY;
					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
						InfoPanel.SetAspectRatio(AspectX,AspectY);

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
										 | CCoreEngine::STATISTIC_BITRATE))!=0) {
						InfoPanel.SetBitRate(CoreEngine.GetBitRateFloat());
						if (InfoPanel.IsSignalLevelEnabled())
							InfoPanel.SetSignalLevel(CoreEngine.GetSignalLevel());
					}

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
						InfoPanel.UpdateErrorCount();
					}

					if (RecordManager.IsRecording()) {
						const CRecordTask *pRecordTask=RecordManager.GetRecordTask();

						InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
							pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime());
					}

					if (TimerCount%(2000/UPDATE_TIMER_INTERVAL)==0)	// 負荷軽減
						UpdateProgramInfo();
				} else if (InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL) {
					// チャンネルタブ更新
					if (!m_ChannelPanelTimer.IsEnabled()
							&& (TimerCount+1)%(60*1000/UPDATE_TIMER_INTERVAL)==0)
						ChannelPanel.UpdateChannelList(false);
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

			m_fWheelChannelChanging=false;
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
		if (EpgOptions.IsEpgDataLoading())
			break;
		if (fShowPanelWindow && InfoWindow.GetCurTab()==PANEL_TAB_PROGRAMLIST) {
			int ServiceID=ChannelManager.GetCurrentServiceID();

			if (ServiceID<=0) {
				WORD SID;
				if (CoreEngine.m_DtvEngine.GetServiceID(&SID))
					ServiceID=SID;
			}
			if (ServiceID>0)
				ProgramListPanel.UpdateProgramList(CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),(WORD)ServiceID);
		} else if ((fShowPanelWindow
					&& InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
				|| m_ProgramListUpdateTimerCount>8
				|| (m_ProgramListUpdateTimerCount>=2
					&& m_ProgramListUpdateTimerCount%2==0)) {
			const CChannelInfo *pChannelInfo=ChannelManager.GetCurrentChannelInfo();

			if (pChannelInfo!=NULL) {
				EpgProgramList.UpdateProgramList(
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID());
				if (fShowPanelWindow
						&& InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
					ChannelPanel.UpdateChannel(ChannelManager.GetCurrentChannel(),false);
			}
		}
		m_ProgramListUpdateTimerCount++;
		// 更新頻度を下げる
		if (m_ProgramListUpdateTimerCount==8)
			::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,60*1000,NULL);
		else if (m_ProgramListUpdateTimerCount==10)
			::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,3*60*1000,NULL);
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		// 番組表の取得
		if (!m_fStandby)
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
			if (i==pList->NumChannels())
				ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
			else
				AppMain.SetChannel(ChannelManager.GetCurrentSpace(),i);
		}
		break;

	case TIMER_ID_CHANNELPANELUPDATE:
		if (EpgOptions.IsEpgDataLoading())
			break;
		if (fShowPanelWindow && InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
			RefreshChannelPanel();
		m_ChannelPanelTimer.End();
		break;

	case TIMER_ID_VIDEOSIZECHANGED:
		{
			RECT rc;

			m_VideoContainer.GetClientRect(&rc);
			m_VideoContainer.SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
			if (m_VideoSizeChangedTimerCount==1)
				::KillTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED);
			else
				m_VideoSizeChangedTimerCount++;
		}
		break;
	}
}


bool CMainWindow::UpdateProgramInfo()
{
	bool fOK=false;

	if (CoreEngine.m_DtvEngine.m_TsPacketParser.IsEpgDataCapLoaded()) {
		// EpgDataCap利用可
		CoreEngine.UpdateEpgDataInfo();

		const CEpgDataInfo *pInfo=CoreEngine.GetEpgDataInfo(InfoPanel.GetProgramInfoNext());

		if (pInfo!=NULL) {
			WCHAR szText[2048];

			SYSTEMTIME stStart,stEnd;
			int Length;

			pInfo->GetStartTime(&stStart);
			pInfo->GetEndTime(&stEnd);
			Length=::wnsprintfW(szText,lengthof(szText)-1,
				L"%d/%d/%d(%s) %d:%02d～%d:%02d\r\n%s\r\n\r\n%s\r\n\r\n%s",
				stStart.wYear,
				stStart.wMonth,
				stStart.wDay,
				L"日\0月\0火\0水\0木\0金\0土"+stStart.wDayOfWeek*2,
				stStart.wHour,
				stStart.wMinute,
				stEnd.wHour,
				stEnd.wMinute,
				NullToEmptyString(pInfo->GetEventName()),
				NullToEmptyString(pInfo->GetEventText()),
				NullToEmptyString(pInfo->GetEventExtText()));
			szText[Length]='\0';
			InfoPanel.SetProgramInfo(szText);
			fOK=true;
		}
	}
	if (!fOK) {
		TCHAR szText[1024];
		SYSTEMTIME stStart,stEnd;

		if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,&stEnd,
											InfoPanel.GetProgramInfoNext())) {
			int Length;

			Length=::wsprintf(szText,TEXT("%d/%d/%d(%s) %d:%02d～%d:%02d\r\n"),
				stStart.wYear,
				stStart.wMonth,
				stStart.wDay,
				GetDayOfWeekText(stStart.wDayOfWeek),
				stStart.wHour,
				stStart.wMinute,
				stEnd.wHour,
				stEnd.wMinute);
			Length+=CoreEngine.m_DtvEngine.GetEventName(
					&szText[Length],lengthof(szText)-Length,
					InfoPanel.GetProgramInfoNext());
			Length+=::wsprintf(&szText[Length],TEXT("\r\n\r\n"));
			CoreEngine.m_DtvEngine.GetEventText(
					&szText[Length],lengthof(szText)-Length,
					InfoPanel.GetProgramInfoNext());
			InfoPanel.SetProgramInfo(szText);
		} else {
			InfoPanel.SetProgramInfo(NULL);
		}
	}
	return fOK;
}


void CMainWindow::OnChannelListUpdated()
{
	//SetTuningSpaceMenu();
	SetChannelMenu();
}


void CMainWindow::OnChannelChanged()
{
	if (m_fWheelChannelChanging) {
		::KillTimer(m_hwnd,TIMER_ID_WHEELCHANNELCHANGE);
		m_fWheelChannelChanging=false;
	}
	SetTitleText();
	MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
							CM_SPACE_FIRST+ChannelManager.GetCurrentSpace());
	SetChannelMenu();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	if (OSDOptions.GetShowOSD())
		ShowChannelOSD();
	ProgramListPanel.ClearProgramList();
	BeginProgramListUpdateTimer();
	InfoPanel.ResetStatistics();
	if (fShowPanelWindow && InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
		ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	const CChannelInfo *pInfo=ChannelManager.GetCurrentChannelInfo();
	if (pInfo!=NULL) {
		ControlPanel.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
								CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
		ProgramGuide.SetCurrentService(pInfo->GetTransportStreamID(),pInfo->GetServiceID());
	} else {
		ProgramGuide.ClearCurrentService();
	}
	ChannelHistory.Add(CoreEngine.GetDriverFileName(),
					   ChannelManager.GetCurrentRealChannelInfo());
}


void CMainWindow::ShowChannelOSD()
{
	const CChannelInfo *pInfo;

	if (m_fWheelChannelChanging)
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
				lf.lfWidth=0;
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
			ChannelOSD.SetTextHeight(
#ifndef TVH264
				32
#else
				20
#endif
				);
			ChannelOSD.SetText(szText);
			ChannelOSD.CalcTextSize(&sz);
			ChannelOSD.SetPosition(8,24,sz.cx+8,sz.cy+8);
			if (m_fWheelChannelChanging)
				cr=MixColor(OSDOptions.GetTextColor(),RGB(0,0,0),160);
			else
				cr=OSDOptions.GetTextColor();
			ChannelOSD.SetTextColor(cr);
			ChannelOSD.Show(OSDOptions.GetFadeTime(),
							!m_fWheelChannelChanging && !ChannelOSD.IsVisible());
		}
	}
}


void CMainWindow::OnDriverChanged()
{
	if (m_fWheelChannelChanging) {
		::KillTimer(m_hwnd,TIMER_ID_WHEELCHANNELCHANGE);
		m_fWheelChannelChanging=false;
	}
	if (m_fSrcFilterReleased && CoreEngine.IsDriverOpen())
		m_fSrcFilterReleased=false;
	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(false);
	ProgramListPanel.ClearProgramList();
	InfoPanel.ResetStatistics();
	InfoPanel.ShowSignalLevel(!CoreEngine.IsNetworkDriver()
		&& !DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName()));
	if (fShowPanelWindow && InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	else
		ChannelPanel.ClearChannelList();
	ProgramGuide.ClearCurrentService();
	//SetTuningSpaceMenu();
	SetChannelMenu();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	SetTitleText();
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
}


void CMainWindow::OnRecordingStart()
{
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	StatusView.UpdateItem(STATUS_ITEM_ERROR);
	//MainMenu.EnableItem(CM_RECORDOPTION,false);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
}


void CMainWindow::OnRecordingStop()
{
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	InfoPanel.SetRecordStatus(false);
	//MainMenu.EnableItem(CM_RECORDOPTION,true);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,false);
	m_fRecordingStopOnEventEnd=false;
	if (m_fStandby)
		AppMain.CloseTuner();
	if (m_fExitOnRecordingStop)
		PostCommand(CM_EXIT);
}


void CMainWindow::OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz,bool fStatus)
{
	int Mode;

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
	if (fStatus && StatusView.GetVisible()) {
		POINT pt;
		RECT rc;

		pt.x=GET_X_LPARAM(lParam);
		pt.y=GET_Y_LPARAM(lParam);
		StatusView.GetScreenPosition(&rc);
		if (PtInRect(&rc,pt)) {
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
				m_fWheelChannelChanging=true;
				ChannelManager.SetChangingChannel(ChannelManager.FindChannelInfo(pInfo));
				StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (OSDOptions.GetShowOSD())
					ShowChannelOSD();
				SetTimer(m_hwnd,TIMER_ID_WHEELCHANNELCHANGE,
						 OperationOptions.GetWheelChannelDelay(),NULL);
			}
		}
		break;

	case COperationOptions::WHEEL_AUDIO:
		SendCommand(CM_SWITCHAUDIO);
		break;

	case COperationOptions::WHEEL_ZOOM:
		if (!IsZoomed(m_hwnd) && !m_fFullscreen) {
			int Zoom;

			Zoom=CalcZoomRate();
			if (GET_WHEEL_DELTA_WPARAM(wParam)>=0)
				Zoom+=OperationOptions.GetWheelZoomStep();
			else
				Zoom-=OperationOptions.GetWheelZoomStep();
			SetZoomRate(Zoom);
		}
		break;

	case COperationOptions::WHEEL_ASPECTRATIO:
		SendCommand(CM_ASPECTRATIO);
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
	if (!m_PreviewManager.EnablePreview(fEnable))
		return false;
	MainMenu.CheckItem(CM_DISABLEVIEWER,!IsPreview());
	return true;
}


bool CMainWindow::IsPreview() const
{
	return m_PreviewManager.IsPreviewEnabled();
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
	int NumStreams=CoreEngine.m_DtvEngine.GetAudioStreamNum();

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


DWORD WINAPI CMainWindow::ExitWatchThread(LPVOID lpParameter)
{
	HANDLE hEvent=lpParameter;

	if (::WaitForSingleObject(hEvent,30000)!=WAIT_OBJECT_0) {
		Logger.AddLog(TEXT("終了処理がタイムアウトしました。プロセスを強制的に終了させます。"));
		::ExitProcess(-1);
	}
	return 0;
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

			if (!pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam)))
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
					if (pThis->IsPreview()) {
						pThis->EnablePreview(false);
						pThis->m_fRestorePreview=true;
					}
				}
			} else if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
				pThis->SetWindowVisible();
			}

			if (wParam==SIZE_MAXIMIZED
					&& (!pThis->m_fShowTitleBar || pThis->m_fCustomTitleBar)) {
				HMONITOR hMonitor=::MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi;

				mi.cbSize=sizeof(MONITORINFO);
				::GetMonitorInfo(hMonitor,&mi);
				pThis->SetPosition(&mi.rcWork);
				SIZE sz;
				pThis->GetClientSize(&sz);
				lParam=MAKELPARAM(sz.cx,sz.cy);
			}
			TitleBar.SetMaximizeMode(wParam==SIZE_MAXIMIZED/*pThis->GetMaximize()*/);

			int Width=LOWORD(lParam),Height=HIWORD(lParam);
			//bool fShowSideBar=pThis->m_fShowSideBar && SideBar.GetParent()==Splitter.GetHandle();

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
			if (!pThis->m_fShowTitleBar && TitleBar.GetVisible()) {
				RECT rc,rcBar;

				pThis->m_ViewWindow.GetScreenPosition(&rc);
				MapWindowRect(NULL,Splitter.GetHandle(),&rc);
				TitleBarUtil.Layout(&rc,&rcBar);
				/*
				if (fShowSideBar)
					rcBar.left+=SideBar.GetBarWidth();
				*/
				TitleBar.SetPosition(&rcBar);
			}
			/*
			if (fShowSideBar) {
				RECT rc;

				rc.left=0;
				rc.top=0;
				rc.right=SideBar.GetBarWidth();
				rc.bottom=Height;
				SideBar.SetPosition(&rc);
			}
			*/
			if (NotificationBar.GetVisible()) {
				RECT rc,rcView;

				NotificationBar.GetPosition(&rc);
				pThis->m_ViewWindow.GetClientRect(&rcView);
				rc.left=rcView.left;
				/*
				if (fShowSideBar)
					rc.left+=SideBar.GetBarWidth();
				*/
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
#if 0
						rcClient.right-=GetSystemMetrics(SM_CXEDGE)*2;
						rcClient.bottom-=GetSystemMetrics(SM_CYEDGE)*2;
#else
						rcClient.right-=pThis->m_ViewWindow.GetVerticalEdgeWidth()*2;
						rcClient.bottom-=pThis->m_ViewWindow.GetHorizontalEdgeHeight()*2;
#endif
					}
					if (pThis->m_fShowStatusBar)
						rcClient.bottom-=StatusView.GetHeight();
					if (pThis->m_fShowTitleBar && pThis->m_fCustomTitleBar)
						TitleBarUtil.AdjustArea(&rcClient);
					if (pThis->m_fShowSideBar)
						SideBarUtil.AdjustArea(&rcClient);
					if (fShowPanelWindow && !PanelFrame.GetFloating())
						rcClient.right-=Splitter.GetBarWidth()+Splitter.GetPaneSize(PANE_ID_PANEL);
					::OffsetRect(&rcClient,-rcClient.left,-rcClient.top);
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
			PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,prc);
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
#if 0
				rc.right+=GetSystemMetrics(SM_CXEDGE)*2;
				rc.bottom+=GetSystemMetrics(SM_CYEDGE)*2;
#else
				rc.right+=pThis->m_ViewWindow.GetVerticalEdgeWidth()*2;
				rc.bottom+=pThis->m_ViewWindow.GetHorizontalEdgeHeight()*2;
#endif
			}
			if (pThis->m_fShowStatusBar)
				rc.bottom+=StatusView.GetHeight();
			if (pThis->m_fShowTitleBar && pThis->m_fCustomTitleBar)
				TitleBarUtil.ReserveArea(&rc,true);
			if (pThis->m_fShowSideBar)
				SideBarUtil.ReserveArea(&rc,true);
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
				pThis->m_Fullscreen.OnRButtonDown();
			} else {
#if 0
				/*
				POINT pt;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				::ClientToScreen(hwnd,&pt);
				pThis->PopupMenu(&pt);
				*/
				pThis->PopupMenu();
#else
				::SendMessage(hwnd,WM_COMMAND,
							  MAKEWPARAM(OperationOptions.GetRightClickCommand(),COMMAND_FROM_MOUSE),0);
#endif
			}
		}
		return 0;

	case WM_MBUTTONDOWN:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pThis->m_Fullscreen.OnMButtonDown();
			} else {
				::SendMessage(hwnd,WM_COMMAND,
							  MAKEWPARAM(OperationOptions.GetMiddleClickCommand(),COMMAND_FROM_MOUSE),0);
			}
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam!=HTCAPTION || !OperationOptions.GetDisplayDragMove())
			break;
		ForegroundWindow(hwnd);
	case WM_LBUTTONDOWN:
		if (OperationOptions.GetDisplayDragMove()) {
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
			TitleBarUtil.EndDrag();
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CMainWindow *pThis=GetThis(hwnd);

			if (pThis->m_fFullscreen) {
				pThis->m_Fullscreen.OnMouseMove();
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
					SnapWindow(hwnd,&rc,
							   ViewOptions.GetSnapAtWindowEdgeMargin(),
							   PanelEventHandler.IsAttached()?NULL:PanelFrame.GetHandle());
				pThis->SetPosition(&rc);
				PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,&rc);
			} else if (!pThis->m_fFullscreen) {
				POINT pt;
				RECT rc;

				::GetCursorPos(&pt);
				::ScreenToClient(hwnd,&pt);
				if (!pThis->m_fShowTitleBar) {
					pThis->m_ViewWindow.GetScreenPosition(&rc);
					MapWindowRect(NULL,hwnd,&rc);
					if (TitleBarUtil.IsSpot(&rc,&pt)) {
						if (!TitleBar.GetVisible()) {
							RECT rcBar;
							TitleBarUtil.Layout(&rc,&rcBar);
							TitleBar.SetPosition(&rcBar);
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
				if (!pThis->m_fShowSideBar && SideBarOptions.ShowPopup()
						&& (pThis->m_fShowTitleBar || !TitleBar.GetVisible())
						&& (pThis->m_fShowStatusBar || !StatusView.GetVisible())) {
					pThis->m_ViewWindow.GetScreenPosition(&rc);
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
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		::SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(OperationOptions.GetLeftDoubleClickCommand(),COMMAND_FROM_MOUSE),0);
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
			} else if (wParam>=VK_F13 && wParam<=VK_F24
					&& !HDUSController.IsEnabled()
					&& (::GetKeyState(VK_SHIFT)<0 || ::GetKeyState(VK_CONTROL)<0)) {
				pThis->ShowMessage(TEXT("リモコンを使用するためには、設定でリモコンを有効にしてください。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
				break;
			} else {
				break;
			}
			pThis->SendCommand(Command);
		}
		return 0;

	case WM_MOUSEWHEEL:
		GetThis(hwnd)->OnMouseWheel(wParam,lParam,false,StatusView.GetParent()==hwnd);
		return 0;

	case WM_MOUSEHWHEEL:
		GetThis(hwnd)->OnMouseWheel(wParam,lParam,true,StatusView.GetParent()==hwnd);
		return 1;	// 1を返さないと繰り返し送られて来ないらしい

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

			if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE)) {
				GetThis(hwnd)->SetTuningSpaceMenu();
			} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
				PluginList.SetMenu(hmenu);
				Accelerator.SetMenuAccel(hmenu);
				return 0;
			} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
				ChannelHistory.SetMenu(hmenu);
				//Accelerator.SetMenuAccel(hmenu);
				return 0;
			} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO)) {
				if (AspectRatioIconMenu.OnInitMenuPopup(hwnd,wParam,lParam))
					return 0;
			} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE)) {
				CTsAnalyzer::CServiceList ServiceList;
				WORD CurServiceID;
				int CurService=-1;

				CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceList(&ServiceList);
				if (!CoreEngine.m_DtvEngine.GetServiceID(&CurServiceID))
					CurServiceID=0;
				ClearMenu(hmenu);
				for (int i=0;i<ServiceList.NumServices();i++) {
					const CTsAnalyzer::ServiceInfo *pServiceInfo=ServiceList.GetServiceInfo(i);
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
						CurService=i;
				}
				if (CurService>=0)
					MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
											CM_SERVICE_FIRST+ServiceList.NumServices()-1,
											CM_SERVICE_FIRST+CurService);
			}
		}
		break;

	case WM_SYSCOMMAND:
		switch ((wParam&0xFFFFFFF0UL)) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver())
				return 0;
			break;

		case SC_ABOUT:
			{
				CMainWindow *pThis=GetThis(hwnd);
				CAboutDialog AboutDialog;

				AboutDialog.Show(pThis->GetVideoHostWindow());
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
		{
			int Command=Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				GetThis(hwnd)->SendCommand(Command);
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
				::PostMessage(hwnd,WM_COMMAND,Command,0);
		}
		return 0;

	case WM_SETTEXT:
		TitleBar.SetLabel(reinterpret_cast<LPCTSTR>(lParam));
		break;

	case WM_POWERBROADCAST:
		if (wParam==PBT_APMSUSPEND) {
			CMainWindow *pThis=GetThis(hwnd);

			Logger.AddLog(TEXT("サスペンドへの移行メッセージを受信しました。"));
			if (pThis->m_fProgramGuideUpdating)
				pThis->EndProgramGuideUpdate();
			if (!pThis->m_fSrcFilterReleased) {
				pThis->m_RestoreChannelSpec.Store(&ChannelManager);
				AppMain.CloseTuner();
			}
			pThis->m_fRestorePreview=pThis->IsPreview();
			pThis->CloseMediaViewer();
		} else if (wParam==PBT_APMRESUMESUSPEND) {
			CMainWindow *pThis=GetThis(hwnd);

			Logger.AddLog(TEXT("サスペンドからの復帰メッセージを受信しました。"));
			if (!pThis->m_fStandby) {
				pThis->OpenTuner();
				if (pThis->m_fRestorePreview)
					pThis->BuildMediaViewer();
			}
		}
		break;

	case WM_APP_SERVICEUPDATE:
		{
			CServiceUpdateInfo *pInfo=reinterpret_cast<CServiceUpdateInfo*>(lParam);
			HMENU hmenu;
			int i;

#if 0	// メニューを開いた時に設定するように変更
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
			if (pInfo->m_CurService>=0)
				MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
										CM_SERVICE_FIRST+pInfo->m_NumServices-1,
										CM_SERVICE_FIRST+pInfo->m_CurService);
#endif

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
						GetThis(hwnd)->ShowChannelOSD();
				}*/
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

	case WM_APP_VIDEOSIZECHANGED:
		{
			CMainWindow *pThis=GetThis(hwnd);

			pThis->m_VideoSizeChangedTimerCount=0;
			::SetTimer(hwnd,CMainWindow::TIMER_ID_VIDEOSIZECHANGED,1000,NULL);
		}
		return 0;

	case WM_APP_EMMPROCESSED:
		Logger.AddLog(wParam!=0?TEXT("EMM処理を行いました。"):TEXT("EMM処理でエラーが発生しました。"));
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

			pThis->ResetDisplayStatus();

			pThis->m_Fullscreen.Destroy();
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

			TaskbarManager.Finalize();

#ifndef _DEBUG
			HANDLE hEvent,hThread;

			hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
			hThread=::CreateThread(NULL,0,ExitWatchThread,hEvent,0,NULL);
#endif

			CoreEngine.m_DtvEngine.SetTracer(&Logger);
			CoreEngine.Close();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(NULL);

			Logger.AddLog(TEXT("プラグインを開放しています..."));
			PluginOptions.StorePluginOptions();
			PluginList.FreePlugins();

			::SetPriorityClass(::GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);

			Logger.AddLog(TEXT("EPGデータを保存しています..."));
			EpgOptions.SaveEpgFile(&EpgProgramList);
			EpgOptions.Finalize();
			AppMain.Finalize();

#ifndef _DEBUG
			::SetEvent(hEvent);
			if (::WaitForSingleObject(hThread,5000)!=WAIT_OBJECT_0)
				::TerminateThread(hThread,-1);
			::CloseHandle(hThread);
			::CloseHandle(hEvent);
#endif

			pThis->OnDestroy();
			::PostQuitMessage(0);
		}
		return 0;

	HANDLE_MSG(hwnd,WM_COMMAND,GetThis(hwnd)->OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,GetThis(hwnd)->OnTimer);

	case WM_ACTIVATEAPP:
		if (HDUSController.OnActivateApp(hwnd,wParam,lParam))
			return 0;
		break;

	default:
		if (HDUSController.HandleMessage(hwnd,uMsg,wParam,lParam))
			return 0;
		if (ResidentManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
		if (TaskbarManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CMainWindow::SetTitleText()
{
	const CChannelInfo *pInfo;
	TCHAR szText[64];
	LPCTSTR pszText;

	if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		::lstrcpy(szText,TITLE_TEXT TEXT(" - "));
		::lstrcat(szText,pInfo->GetName());
		pszText=szText;
	} else {
		pszText=TITLE_TEXT;
	}
	::SetWindowText(m_hwnd,pszText);
	ResidentManager.SetTipText(pszText);
}


void CMainWindow::SetTuningSpaceMenu()
{
	HMENU hmenu=MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE);
	TCHAR szText[MAX_PATH*2];
	int Length;
	int i,j;

	ClearMenu(hmenu);
#if 0
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
#else
	const CChannelList *pChannelList;
	HMENU hmenuSpace;
	int Command=CM_SPACE_CHANNEL_FIRST;
	LPCTSTR pszName;
	pChannelList=ChannelManager.GetAllChannelList();
	if (ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		int PrevSpace=-1;
		for (i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

			if (pChannelInfo->IsEnabled()) {
				pszName=pChannelInfo->GetName();
				CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
							   szText,lengthof(szText));
				UINT Flags=MFT_STRING | MFS_ENABLED;
				if (PrevSpace>=0 && pChannelInfo->GetSpace()!=PrevSpace)
					Flags|=MFT_MENUBREAK;
				::AppendMenu(hmenuSpace,Flags,Command,szText);
				PrevSpace=pChannelInfo->GetSpace();
			}
			Command++;
		}
		if (ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL
				&& ChannelManager.GetCurrentChannel()>=0
				&& pChannelList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenuSpace,CM_SPACE_CHANNEL_FIRST,Command-1,
								 CM_SPACE_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
								 MF_BYCOMMAND);
		}
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&A: すべて"));
	} else {
		Command+=pChannelList->NumChannels();
	}
	for (i=0;i<ChannelManager.NumSpaces();i++) {
		hmenuSpace=::CreatePopupMenu();
		pChannelList=ChannelManager.GetChannelList(i);
		bool fHasControlKeyID=pChannelList->HasRemoteControlKeyID();
		int FirstCommand=Command;
		for (j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

			if (pChannelInfo->IsEnabled()) {
				Length=::wsprintf(szText,TEXT("%d : "),
								  fHasControlKeyID?pChannelInfo->GetChannelNo():j+1);
				pszName=pChannelInfo->GetName();
				CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
							   szText+Length,lengthof(szText)-Length);
				::AppendMenu(hmenuSpace,MFT_STRING | MFS_ENABLED,Command,szText);
			}
			Command++;
		}
		if (ChannelManager.GetCurrentSpace()==i
				&& ChannelManager.GetCurrentChannel()>=0
				&& pChannelList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenuSpace,FirstCommand,Command-1,
								 FirstCommand+ChannelManager.GetCurrentChannel(),
								 MF_BYCOMMAND);
		}
		Length=::wsprintf(szText,TEXT("&%d: "),i);
		pszName=ChannelManager.GetTuningSpaceName(i);
		CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),
					   szText+Length,lengthof(szText)-Length);
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
	}
#endif
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
	::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_DRIVER_BROWSE,TEXT("参照..."));
	::CheckMenuRadioItem(hmenu,CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
						 CM_DRIVER_FIRST+CurDriver,MF_BYCOMMAND);
	Accelerator.SetMenuAccel(hmenu);
}


void CMainWindow::SetChannelMenu()
{
	HMENU hmenu=MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL);

	if (pNetworkRemocon!=NULL) {
		SetNetworkRemoconChannelMenu(hmenu);
		return;
	}

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();

	ClearMenu(hmenu);
	if (pList==NULL)
		return;
	bool fControlKeyID=pList->HasRemoteControlKeyID();
	for (int i=0,j=0;i<pList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pList->GetChannelInfo(i);
		TCHAR szText[MAX_CHANNEL_NAME+4];

		if (pChInfo->IsEnabled()) {
			wsprintf(szText,TEXT("%d: %s"),
				fControlKeyID?pChInfo->GetChannelNo():i+1,pChInfo->GetName());
			AppendMenu(hmenu,MFT_STRING | MFS_ENABLED
				| (j!=0 && j%12==0?MF_MENUBREAK:0),CM_CHANNEL_FIRST+i,szText);
			j++;
		}
	}
	if (ChannelManager.GetCurrentChannel()>=0
			&& pList->IsEnabled(ChannelManager.GetCurrentChannel()))
		MainMenu.CheckRadioItem(CM_CHANNEL_FIRST,
			CM_CHANNEL_FIRST+pList->NumChannels()-1,
			CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel());
}


void CMainWindow::SetNetworkRemoconChannelMenu(HMENU hmenu)
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


HMENU CMainWindow::CreateTunerSelectMenu()
{
	HMENU hmenu=::CreatePopupMenu(),hmenuSpace;
	const CChannelList *pChannelList;
	int Command,FirstCommand;
	int i,j;
	LPCTSTR pszName;
	TCHAR szText[MAX_PATH*2];
	int Length;

	Command=CM_SPACE_CHANNEL_FIRST;
	/*
	if ((!CoreEngine.IsNetworkDriver() || pNetworkRemocon==NULL)
			&& ChannelManager.GetAllChannelList()->NumChannels()>0) {
		hmenuSpace=::CreatePopupMenu();
		pChannelList=ChannelManager.GetAllChannelList();
		for (i=0;i<pChannelList->NumChannels();i++) {
			pszName=pChannelList->GetName(i);
			CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),szText,lengthof(szText));
			::AppendMenu(hmenuSpace,MFT_STRING | MFS_ENABLED,Command++,szText);
		}
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&A: すべて"));
	}
	*/
	pChannelList=ChannelManager.GetAllChannelList();
	if (ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		int PrevSpace=-1;
		for (i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);
	
			if (pChannelInfo->IsEnabled()) {
				pszName=pChannelInfo->GetName();
				CopyToMenuText(pszName!=NULL?pszName:TEXT("???"),szText,lengthof(szText));
				UINT Flags=MFT_STRING | MFS_ENABLED;
				if (PrevSpace>=0 && pChannelInfo->GetSpace()!=PrevSpace)
					Flags|=MFT_MENUBREAK;
				::AppendMenu(hmenuSpace,Flags,Command,szText);
				PrevSpace=pChannelInfo->GetSpace();
			}
			Command++;
		}
		if (ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL
				&& ChannelManager.GetCurrentChannel()>=0
				&& pChannelList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenuSpace,CM_SPACE_CHANNEL_FIRST,Command-1,
								 CM_SPACE_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
								 MF_BYCOMMAND);
		}
		::AppendMenu(hmenu,MF_POPUP | MFS_ENABLED,
					 reinterpret_cast<UINT_PTR>(hmenuSpace),TEXT("&A: すべて"));
	} else {
		Command+=pChannelList->NumChannels();
	}
	for (i=0;i<ChannelManager.NumSpaces();i++) {
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
				::AppendMenu(hmenuSpace,MFT_STRING | MFS_ENABLED,Command,szText);
			}
			Command++;
		}
		if (ChannelManager.GetCurrentSpace()==i
				&& ChannelManager.GetCurrentChannel()>=0
				&& pChannelList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenuSpace,FirstCommand,Command-1,
								 FirstCommand+ChannelManager.GetCurrentChannel(),
								 MF_BYCOMMAND);
		}
		Length=::wsprintf(szText,TEXT("&%d: "),i);
		pszName=ChannelManager.GetTuningSpaceName(i);
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
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetTuningSpaceList();

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


bool CMainWindow::ProcessTunerSelectMenu(int Command)
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
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetTuningSpaceList();
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
		if (m_fRestorePreview)
			EnablePreview(true);
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
	if (StreamInfo.IsCreated())
		StreamInfo.SetVisible(fShow);
}


bool CMainWindow::SetResident(bool fResident)
{
	return ResidentManager.SetResident(fResident);
}


bool CMainWindow::GetResident() const
{
	return ResidentManager.GetResident();
}


bool CMainWindow::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (fStandby) {
			m_fRestorePreview=IsPreview();
			if (m_fRestorePreview)
				EnablePreview(false);
			m_fRestoreFullscreen=m_fFullscreen;
			if (m_fFullscreen)
				SetFullscreen(false);
			ShowFloatingWindows(false);
			SetVisible(false);
			PluginList.SendStandbyEvent(true);
			ResetDisplayStatus();
			m_RestoreChannelSpec.Store(&ChannelManager);
			if (EpgOptions.GetUpdateWhenStandby()
					&& CoreEngine.m_DtvEngine.IsSrcFilterOpen()
					&& !RecordManager.IsRecording()
					&& !CoreEngine.IsNetworkDriver())
				BeginProgramGuideUpdate();
			if (!RecordManager.IsRecording() && !m_fProgramGuideUpdating)
				AppMain.CloseTuner();
		} else {
			SetWindowVisible();
			::SetCursor(LoadCursor(NULL,IDC_WAIT));
			if (m_fStandbyInit) {
				OpenTuner();
				AppMain.InitializeChannel();
				CoreEngine.m_DtvEngine.SetTracer(&StatusView);
				BuildMediaViewer();
				CoreEngine.m_DtvEngine.SetTracer(NULL);
				StatusView.SetSingleText(NULL);
				m_fStandbyInit=false;
			}
			if (m_fRestoreFullscreen)
				SetFullscreen(true);
			ShowFloatingWindows(true);
			ForegroundWindow(m_hwnd);
			PluginList.SendStandbyEvent(false);
			OpenTuner();
			if (m_fRestorePreview)
				EnablePreview(true);
			SetDisplayStatus();
			::SetCursor(LoadCursor(NULL,IDC_ARROW));
		}
		m_fStandby=fStandby;
	}
	return true;
}


bool CMainWindow::InitStandby()
{
	m_fRestorePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
						&& (!ViewOptions.GetRestorePlayStatus() || fEnablePlay);
	m_fRestoreFullscreen=CmdLineParser.m_fFullscreen;
	if (CoreEngine.GetDriverFileName()[0]!='\0')
		m_fSrcFilterReleased=true;
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(RestoreChannelInfo.Space);
				m_RestoreChannelSpec.SetChannel(Index);
				m_RestoreChannelSpec.SetServiceID(RestoreChannelInfo.ServiceID);
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
	m_fRestorePreview=!CmdLineParser.m_fNoDirectShow && !CmdLineParser.m_fNoView
						&& (!ViewOptions.GetRestorePlayStatus() || fEnablePlay);
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_RestoreChannelSpec.SetSpace(RestoreChannelInfo.Space);
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


bool CMainWindow::ConfirmExit()
{
	return RecordOptions.ConfirmExit(GetVideoHostWindow(),&RecordManager);
}


bool CMainWindow::DoCommand(LPCWSTR pszCommand)
{
	int Command=CommandList.ParseText(pszCommand);

	if (Command==0)
		return false;
	OnCommand(m_hwnd,Command,NULL,0);
	return true;
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
		AppMain.StopRecord();
	PluginList.SendExecuteEvent(pszCmdLine);
	return true;
}


bool CMainWindow::BeginProgramGuideUpdate()
{
	if (!m_fProgramGuideUpdating) {
		if (RecordManager.IsRecording()) {
			if (::MessageBox(ProgramGuide.GetHandle(),
							 TEXT("録画中です。\n番組表の取得を開始してもいいですか?"),TEXT("確認"),
							 MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK)
				return false;
		}
		const CChannelList *pList=ChannelManager.GetCurrentRealChannelList();
		if (pList==NULL)
			return false;
		int i;
		for (i=0;i<pList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

			if (pChInfo->IsEnabled())
				break;
		}
		if (i==pList->NumChannels())
			return false;
		Logger.AddLog(TEXT("番組表の取得開始"));
		if (m_fStandby && m_fSrcFilterReleased) {
			if (!OpenTuner())
				return false;
		}
		m_fProgramGuideUpdating=true;
		m_ProgramGuideUpdateStartChannel=ChannelManager.GetCurrentChannel();
		m_fRestorePreview=IsPreview();
		EnablePreview(false);
		AppMain.SetChannel(ChannelManager.GetCurrentSpace(),i);
		::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,m_fStandby?40000:20000,NULL);
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
		if (m_fStandby) {
			hThread=::GetCurrentThread();
			OldPriority=::GetThreadPriority(hThread);
			::SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		}
		EpgProgramList.UpdateProgramList();
		EpgOptions.SaveEpgFile(&EpgProgramList);
		if (m_fStandby) {
			ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			::SetThreadPriority(hThread,OldPriority);
			if (fRelease)
				AppMain.CloseTuner();
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			AppMain.SetChannel(ChannelManager.GetCurrentSpace(),
							   m_ProgramGuideUpdateStartChannel);
			if (m_fRestorePreview)
				EnablePreview(true);
			if (fShowPanelWindow && InfoWindow.GetCurTab()==PANEL_TAB_CHANNEL)
				ChannelPanel.UpdateChannelList(false);
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


void CMainWindow::BeginProgramListUpdateTimer()
{
	SetTimer(m_hwnd,TIMER_ID_PROGRAMLISTUPDATE,5000,NULL);
	m_ProgramListUpdateTimerCount=0;
}


void CMainWindow::UpdatePanel()
{
	switch (InfoWindow.GetCurTab()) {
	case PANEL_TAB_INFORMATION:
		{
			BYTE AspectX,AspectY;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
				InfoPanel.SetAspectRatio(AspectX,AspectY);
			if (InfoPanel.IsSignalLevelEnabled())
				InfoPanel.SetSignalLevel(CoreEngine.GetSignalLevel());
			InfoPanel.SetBitRate(CoreEngine.GetBitRateFloat());
			InfoPanel.UpdateErrorCount();
			if (RecordManager.IsRecording()) {
				const CRecordTask *pRecordTask=RecordManager.GetRecordTask();
				InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
					pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime());
			}
			UpdateProgramInfo();
		}
		break;

	case PANEL_TAB_PROGRAMLIST:
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

	case PANEL_TAB_CHANNEL:
		if (!m_ChannelPanelTimer.IsEnabled())
			RefreshChannelPanel();
		break;
	}
}


void CMainWindow::BeginChannelPanelUpdateTimer()
{
	m_ChannelPanelTimer.Begin(m_hwnd,3000);
}


void CMainWindow::RefreshChannelPanel()
{
	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	if (ChannelPanel.IsChannelListEmpty())
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
	else
		ChannelPanel.UpdateChannelList(true);
	ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	::SetCursor(hcurOld);
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
#if 0
	DWORD ExStyle;

	ExStyle=m_ViewWindow.GetExStyle();
	if (((ExStyle&WS_EX_CLIENTEDGE)!=0)!=fEdge)
		m_ViewWindow.SetExStyle(ExStyle^WS_EX_CLIENTEDGE,true);
#else
	m_ViewWindow.SetEdge(fEdge);
#endif
	return true;
}


void CMainWindow::SetDisplayStatus()
{
	bool fNoScreenSaver=ViewOptions.GetNoScreenSaver();
	bool fNoMonitorLowPower=ViewOptions.GetNoMonitorLowPower();
	bool fNoMonitorLowPowerActiveOnly=ViewOptions.GetNoMonitorLowPowerActiveOnly();

	if (!fNoScreenSaver && m_fScreenSaverActive) {
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
		m_fScreenSaverActive=FALSE;
	}
	if (!fNoMonitorLowPower) {
#if 1
		::KillTimer(m_hwnd,TIMER_ID_DISPLAY);
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
	if (fNoScreenSaver && !m_fScreenSaverActive) {
		if (!SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,&m_fScreenSaverActive,0))
			m_fScreenSaverActive=FALSE;
		if (m_fScreenSaverActive)
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,NULL,
								 0/*SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE*/);
	}
	if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
#if 1
		// SetThreadExecutionState() を呼ぶタイマー
		::SetTimer(m_hwnd,TIMER_ID_DISPLAY,10000,NULL);
#else
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
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
		}
#endif
	}
}


void CMainWindow::ResetDisplayStatus()
{
	::KillTimer(m_hwnd,TIMER_ID_DISPLAY);
	if (m_fScreenSaverActive) {
		::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
		m_fScreenSaverActive=FALSE;
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


CStatusView *CMainWindow::GetStatusView() const
{
	return &StatusView;
}




CMainWindow::CPreviewManager::CPreviewManager(CVideoContainerWindow *pVideoContainer)
	: m_pVideoContainer(pVideoContainer)
	, m_fPreview(false)
{
}


bool CMainWindow::CPreviewManager::EnablePreview(bool fEnable)
{
	if (m_fPreview!=fEnable) {
		if (fEnable && !CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			bool fOK=BuildMediaViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			if (!fOK)
				return false;
		}
		m_pVideoContainer->SetVisible(fEnable);
		CoreEngine.m_DtvEngine.m_MediaViewer.SetVisible(fEnable);
		if (CoreEngine.EnablePreview(fEnable)) {
			if (AudioOptions.GetMinTimerResolution())
				CoreEngine.SetMinTimerResolution(fEnable);
			m_fPreview=fEnable;
			PluginList.SendPreviewChangeEvent(fEnable);
		}
	}
	return true;
}


bool CMainWindow::CPreviewManager::BuildMediaViewer()
{
	if (m_fPreview)
		EnablePreview(false);

	bool fOK=CoreEngine.BuildMediaViewer(m_pVideoContainer->GetHandle(),
										 MainWindow.GetHandle(),
										 GeneralOptions.GetVideoRendererType(),
										 GeneralOptions.GetMpeg2DecoderName(),
										 AudioOptions.GetAudioDeviceName());
	if (!fOK) {
		AppMain.OnError(&CoreEngine,TEXT("DirectShowの初期化ができません。"));
	}

	return fOK;
}


bool CMainWindow::CPreviewManager::CloseMediaViewer()
{
	EnablePreview(false);
	CoreEngine.CloseMediaViewer();
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


#include <psapi.h>
#pragma comment(lib,"psapi.lib")

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

		DWORD ProcessId;
		HANDLE hProcess;
		::GetWindowThreadProcessId(hwnd,&ProcessId);
		hProcess=::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE,ProcessId);
		if (hProcess!=NULL
				&& ::GetModuleFileNameEx(hProcess,NULL,szFileName,lengthof(szFileName))>0
				&& ::lstrcmpi(szFileName,pInfo->pszFileName)==0) {
			pInfo->hwndFind=hwnd;
			::CloseHandle(hProcess);
			return FALSE;
		}
		::CloseHandle(hProcess);
	}
	return TRUE;
}


#ifndef _DEBUG
#include "DebugHelper.h"
static CDebugHelper DebugHelper;
#endif

int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
												LPTSTR pszCmdLine,int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	DebugHelper.SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif

	Logger.AddLog(TEXT("******** 起動 ********"));

	hInst=hInstance;

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

	// コマンドラインの解析
	if (pszCmdLine[0]!='\0') {
		CmdLineParser.Parse(pszCmdLine);
		// 暫定措置
		if (CmdLineParser.m_TvRockDID>=0)
			CmdLineParser.m_fSilent=true;
#ifndef _DEBUG
		if (CmdLineParser.m_fSilent)
			DebugHelper.SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_NONE);
#endif
		if (CmdLineParser.m_fMaximize && !CmdLineParser.m_fMinimize)
			MainWindow.SetMaximizeStatus(true);
	}

#ifdef TVH264
	PanelFrame.SetFloating(false);
#endif

	AppMain.Initialize();

	//CAppMutex Mutex(fKeepSingleTask || CmdLineParser.m_fSingleTask);
	CAppMutex Mutex(true);

	// 複数起動のチェック
	if (Mutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineParser.m_fSingleTask)) {
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
		} else if (!CmdLineParser.m_fSingleTask) {
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
		GeneralOptions.SetDefaultDriverName(szDriverFileName);
		GeneralOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		GeneralOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		GeneralOptions.SetCardReaderType(InitialSettings.GetCardReader());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	} else if (CmdLineParser.m_szDriverName[0]!='\0') {
		::lstrcpy(szDriverFileName,CmdLineParser.m_szDriverName);
	} else {
		switch (GeneralOptions.GetDefaultDriverType()) {
		case CGeneralOptions::DEFAULT_DRIVER_NONE:
			szDriverFileName[0]='\0';
			break;
		case CGeneralOptions::DEFAULT_DRIVER_CUSTOM:
			::lstrcpy(szDriverFileName,GeneralOptions.GetDefaultDriverName());
			break;
		}
	}

	GeneralOptions.SetTemporaryNoDescramble(CmdLineParser.m_fNoDescramble);

	ColorSchemeOptions.SetApplyCallback(ColorSchemeApplyProc);
	ColorSchemeOptions.ApplyColorScheme();

	CMainWindow::Initialize();
	CViewWindow::Initialize(hInst);
	CVideoContainerWindow::Initialize(hInst);
	CStatusView::Initialize(hInst);
	CSideBar::Initialize(hInst);
	CSplitter::Initialize(hInst);
	CTitleBar::Initialize(hInst);
	CPanelFrame::Initialize(hInst);
	CInfoPanel::Initialize(hInst);
	CInformationPanel::Initialize(hInst);
	CProgramListView::Initialize(hInst);
	CChannelPanel::Initialize(hInst);
	CControlPanel::Initialize(hInst);
	CProgramGuide::Initialize(hInst);
	CCaptureWindow::Initialize(hInst);
	CPseudoOSD::Initialize(hInst);
	CNotificationBar::Initialize(hInst);

	StreamInfo.SetEventHandler(&StreamInfoEventHandler);

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
	if (!MainWindow.GetTitleBarVisible() || MainWindow.GetCustomTitleBar()) {
		// この段階でスタイルを変えないとおかしくなる
		// 最初からこのスタイルにしてもキャプションが表示される
		// ShowWindow の前に入れると、タイトルバーを表示させてもアイコンが出ない
		MainWindow.SetStyle(MainWindow.GetStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	ResidentManager.SetResident(GeneralOptions.GetResident());
	ResidentManager.Initialize(MainWindow.GetHandle());
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
	if (CmdLineParser.m_fMinimize)
		MainWindow.InitMinimize();

	if (ViewOptions.GetShowLogo() && ViewOptions.GetLogoFileName()[0]!='\0')
		MainWindow.SetLogo(ViewOptions.GetLogoFileName());

	CoreEngine.SetDriverFileName(szDriverFileName);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	if (!CmdLineParser.m_fNoDriver && !CmdLineParser.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("ドライバの読み込み中..."));
			if (CoreEngine.LoadDriver()) {
				Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
			} else {
				AppMain.OnError(&CoreEngine);
			}
		} else {
			Logger.AddLog(TEXT("BonDriverが設定されていません。"));
			/*
			if (!CmdLineParser.m_fSilent)
				MainWindow.ShowMessage(
						TEXT("設定で使用するドライバを指定してください。"),
						TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
			*/
		}
	}

	CoreEngine.SetMinTimerResolution(AudioOptions.GetMinTimerResolution());
	CoreEngine.SetDescramble(!CmdLineParser.m_fNoDescramble);
	CoreEngine.SetCardReaderType(GeneralOptions.GetCardReaderType());
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(GeneralOptions.GetDescrambleCurServiceOnly());
	CoreEngine.m_DtvEngine.m_TsDescrambler.EnableSSE2(GeneralOptions.GetDescrambleUseSSE2());
	CoreEngine.m_DtvEngine.m_TsDescrambler.EnableEmmProcess(GeneralOptions.GetEnableEmmProcess());
	CoreEngine.SetPacketBufferLength(GeneralOptions.GetPacketBufferLength());
	CoreEngine.SetPacketBufferPoolPercentage(GeneralOptions.GetPacketBufferPoolPercentage());
	CoreEngine.SetPacketBuffering(GeneralOptions.GetPacketBuffering());
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	CoreEngine.BuildDtvEngine(&DtvEngineHandler);

	if (CoreEngine.IsDriverLoaded()
			&& !DriverOptions.IsDescrambleDriver(CoreEngine.GetDriverFileName())) {
		AppMain.OpenBcasCard(!CmdLineParser.m_fSilent);
	}

	if (!CmdLineParser.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];

		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		AppMain.GetAppDirectory(szPluginDir);
		::PathAppend(szPluginDir,TEXT("Plugins"));
		PluginList.LoadPlugins(szPluginDir);
		PluginOptions.RestorePluginOptions();
	}

	CoreEngine.m_DtvEngine.m_MediaViewer.SetIgnoreDisplayExtension(ViewOptions.GetIgnoreDisplayExtension());
	CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(AudioOptions.GetUseAudioRendererClock());
	CoreEngine.m_DtvEngine.m_MediaViewer.SetAdjustAudioStreamTime(AudioOptions.GetAdjustAudioStreamTime());
	CoreEngine.SetDownMixSurround(AudioOptions.GetDownMixSurround());
	if (!CmdLineParser.m_fStandby && !CmdLineParser.m_fNoDirectShow) {
		MainWindow.BuildMediaViewer();
	}

	EpgOptions.InitializeEpgDataCap();
	EpgOptions.AsyncLoadEpgData(&EpgLoadEventHandler);

	if (CoreEngine.IsDriverLoaded() && !CoreEngine.OpenDriver()) {
		AppMain.OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
	}

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
		MainWindow.SetDisplayStatus();

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	if (!MainWindow.GetStatusBarVisible())
		StatusView.SetVisible(false);
	StatusView.SetSingleText(NULL);

	EpgProgramList.SetEpgDataCapDllUtil(
			CoreEngine.m_DtvEngine.m_TsPacketParser.GetEpgDataCapDllUtil());

	InfoWindow.SetEventHandler(&InfoPanelEventHandler);
	InfoWindow.Create(MainWindow.GetHandle(),WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	InfoWindow.SetTabFont(PanelOptions.GetFont());

	InfoPanel.SetEventHandler(&InformationEventHandler);
	InfoPanel.Create(InfoWindow.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	InfoPanel.ShowSignalLevel(!CoreEngine.IsNetworkDriver()
		&& !DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName()));
	InfoWindow.AddWindow(&InfoPanel,TEXT("情報"));

	ProgramListPanel.SetEpgProgramList(&EpgProgramList);
	ProgramListPanel.Create(InfoWindow.GetHandle(),WS_CHILD | WS_VSCROLL);
	InfoWindow.AddWindow(&ProgramListPanel,TEXT("番組表"));

	ChannelPanel.SetEpgProgramList(&EpgProgramList);
	ChannelPanel.SetEventHandler(&ChannelPanelEventHandler);
	ChannelPanel.Create(InfoWindow.GetHandle(),WS_CHILD | WS_VSCROLL);
	InfoWindow.AddWindow(&ChannelPanel,TEXT("チャンネル"));

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
	ControlPanel.Create(InfoWindow.GetHandle(),WS_CHILD);
	InfoWindow.AddWindow(&ControlPanel,TEXT("操作"));

	InfoWindow.SetPageFont(PanelOptions.GetFont());
	InfoWindow.SetCurTab(PanelOptions.GetFirstTab());
	Splitter.SetPane(PanelPaneIndex,NULL,PANE_ID_PANEL);
	Splitter.SetMinSize(PANE_ID_PANEL,64);
	Splitter.SetFixedPane(PANE_ID_PANEL);
	PanelFrame.Create(MainWindow.GetHandle(),&Splitter,PANE_ID_PANEL,&InfoWindow,TEXT("パネル"));
	PanelFrame.SetEventHandler(&PanelEventHandler);
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
	ProgramGuide.SetDriverList(&DriverManager);

	CaptureWindow.SetEventHandler(&CaptureWindowEventHandler);

	CommandList.Initialize(&DriverManager,&PluginList);
	Accelerator.Initialize(MainWindow.GetHandle(),&MainMenu,
						   AppMain.GetIniFileName(),&CommandList);
	HDUSController.Initialize(MainWindow.GetHandle(),
							  AppMain.GetIniFileName(),&CommandList);
	OperationOptions.Initialize(AppMain.GetIniFileName(),&CommandList);

	if (CoreEngine.m_DtvEngine.IsSrcFilterOpen()) {
		if (CoreEngine.IsBuildComplete()) {
			if (CmdLineParser.m_fFullscreen)
				MainWindow.SetFullscreen(true);
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
		} else if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
			const CChannelList *pList=ChannelManager.GetChannelList(RestoreChannelInfo.Space);
			if (pList!=NULL) {
				int Index=pList->Find(RestoreChannelInfo.Space,
									  RestoreChannelInfo.Channel,
									  RestoreChannelInfo.ServiceID);
				if (Index<0 && RestoreChannelInfo.ServiceID>0)
					Index=pList->Find(RestoreChannelInfo.Space,
									  RestoreChannelInfo.Channel);
				if (Index>=0)
					AppMain.SetChannel(RestoreChannelInfo.Space,Index);
			}
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
			MainWindow.CommandLineRecord(CmdLineParser.m_szRecordFileName,
				CmdLineParser.m_RecordDelay,CmdLineParser.m_RecordDuration);
	}
	if (CmdLineParser.m_fStandby)
		MainWindow.InitStandby();
	if (CmdLineParser.m_fExitOnRecordEnd)
		MainWindow.SendCommand(CM_EXITONRECORDINGSTOP);
	MainWindow.BeginChannelPanelUpdateTimer();

	MSG msg;

	while (GetMessage(&msg,NULL,0,0)>0) {
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

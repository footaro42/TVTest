#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Plugin.h"
#include "Image.h"
#include "DialogUtil.h"
#include "Command.h"
#include "LogoManager.h"
#include "Controller.h"
#include "BonTsEngine/TsEncode.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CControllerPlugin : public CController
{
	CPlugin *m_pPlugin;
	DWORD m_Flags;
	CDynamicString m_Name;
	CDynamicString m_Text;
	int m_NumButtons;
	CController::ButtonInfo *m_pButtonList;
	LPTSTR m_pButtonNameList;
	CDynamicString m_IniFileName;
	CDynamicString m_SectionName;
	UINT m_ControllerImageID;
	UINT m_SelButtonsImageID;
	TVTest::ControllerInfo::TranslateMessageCallback m_pTranslateMessage;
	void *m_pClientData;

public:
	CControllerPlugin(CPlugin *pPlugin,const TVTest::ControllerInfo *pInfo);
	~CControllerPlugin();
	LPCTSTR GetName() const { return m_Name.Get(); }
	LPCTSTR GetText() const { return m_Text.Get(); }
	int NumButtons() const { return m_NumButtons; }
	bool GetButtonInfo(int Index,ButtonInfo *pInfo) const;
	bool Enable(bool fEnable) { return m_pPlugin->Enable(fEnable); }
	bool IsEnabled() const { return m_pPlugin->IsEnabled(); }
	bool IsActiveOnly() const { return (m_Flags&TVTest::CONTROLLER_FLAG_ACTIVEONLY)!=0; }
	bool SetTargetWindow(HWND hwnd) {
		return m_pPlugin->SendEvent(TVTest::EVENT_CONTROLLERFOCUS,(LPARAM)hwnd)!=0;
	}
	bool TranslateMessage(HWND hwnd,MSG *pMessage);
	bool GetIniFileName(LPTSTR pszFileName,int MaxLength) const;
	LPCTSTR GetIniFileSection() const;
	HBITMAP GetImage(ImageType Type) const;
};


CControllerPlugin::CControllerPlugin(CPlugin *pPlugin,const TVTest::ControllerInfo *pInfo)
	: m_pPlugin(pPlugin)
	, m_Flags(pInfo->Flags)
	, m_Name(pInfo->pszName)
	, m_Text(pInfo->pszText)
	, m_NumButtons(pInfo->NumButtons)
	, m_IniFileName(pInfo->pszIniFileName)
	, m_SectionName(pInfo->pszSectionName)
	, m_ControllerImageID(pInfo->ControllerImageID)
	, m_SelButtonsImageID(pInfo->SelButtonsImageID)
	, m_pTranslateMessage(pInfo->pTranslateMessage)
	, m_pClientData(pInfo->pClientData)
{
	const CCommandList *pCommandList=GetAppClass().GetCommandList();

	int Length=m_NumButtons+1;
	for (int i=0;i<m_NumButtons;i++)
		Length+=::lstrlen(pInfo->pButtonList[i].pszName);
	m_pButtonNameList=new TCHAR[Length];
	LPTSTR pszName=m_pButtonNameList;

	m_pButtonList=new CController::ButtonInfo[m_NumButtons];
	for (int i=0;i<m_NumButtons;i++) {
		const TVTest::ControllerButtonInfo &ButtonInfo=pInfo->pButtonList[i];

		::lstrcpy(pszName,ButtonInfo.pszName);
		m_pButtonList[i].pszName=pszName;
		m_pButtonList[i].DefaultCommand=ButtonInfo.pszDefaultCommand!=NULL?
					pCommandList->ParseText(ButtonInfo.pszDefaultCommand):0;
		m_pButtonList[i].ImageButtonRect.Left=ButtonInfo.ButtonRect.Left;
		m_pButtonList[i].ImageButtonRect.Top=ButtonInfo.ButtonRect.Top;
		m_pButtonList[i].ImageButtonRect.Width=ButtonInfo.ButtonRect.Width;
		m_pButtonList[i].ImageButtonRect.Height=ButtonInfo.ButtonRect.Height;
		m_pButtonList[i].ImageSelButtonPos.Left=ButtonInfo.SelButtonPos.Left;
		m_pButtonList[i].ImageSelButtonPos.Top=ButtonInfo.SelButtonPos.Top;
		pszName+=::lstrlen(pszName)+1;
	}
}


CControllerPlugin::~CControllerPlugin()
{
	delete [] m_pButtonList;
	delete [] m_pButtonNameList;
}


bool CControllerPlugin::GetButtonInfo(int Index,ButtonInfo *pInfo) const
{
	if (Index<0 || Index>=m_NumButtons)
		return false;
	*pInfo=m_pButtonList[Index];
	return true;
}


bool CControllerPlugin::TranslateMessage(HWND hwnd,MSG *pMessage)
{
	if (m_pTranslateMessage==NULL)
		return false;
	return m_pTranslateMessage(hwnd,pMessage,m_pClientData)!=FALSE;
}


bool CControllerPlugin::GetIniFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (m_IniFileName.IsEmpty())
		return CController::GetIniFileName(pszFileName,MaxLength);
	if (m_IniFileName.Length()>=MaxLength)
		return false;
	::lstrcpy(pszFileName,m_IniFileName.Get());
	return true;
}


LPCTSTR CControllerPlugin::GetIniFileSection() const
{
	if (m_SectionName.IsEmpty())
		return m_Name.Get();
	return m_SectionName.Get();
}


HBITMAP CControllerPlugin::GetImage(ImageType Type) const
{
	UINT ID;

	switch (Type) {
	case IMAGE_CONTROLLER:	ID=m_ControllerImageID;	break;
	case IMAGE_SELBUTTONS:	ID=m_SelButtonsImageID;	break;
	default:	return NULL;
	}
	if (ID==0)
		return NULL;
	return static_cast<HBITMAP>(::LoadImage(m_pPlugin->GetModuleHandle(),
											MAKEINTRESOURCE(ID),
											IMAGE_BITMAP,0,0,
											LR_CREATEDIBSECTION));
}




//DWORD CPlugin::m_FinalizeTimeout=10000;

bool CPlugin::m_fSetGrabber=false;
CPointerVector<CPlugin::CMediaGrabberInfo> CPlugin::m_GrabberList;
CCriticalLock CPlugin::m_GrabberLock;

CPointerVector<CPlugin::CAudioStreamCallbackInfo> CPlugin::m_AudioStreamCallbackList;
CCriticalLock CPlugin::m_AudioStreamLock;


CPlugin::CPlugin()
	: m_hLib(NULL)
	, m_Version(0)
	, m_Type(0)
	, m_Flags(0)
	, m_fEnabled(false)
	, m_fSetting(false)
	, m_Command(0)
	, m_pEventCallback(NULL)
	, m_pMessageCallback(NULL)
{
}


CPlugin::~CPlugin()
{
	Free();
}


bool CPlugin::Load(LPCTSTR pszFileName)
{
	HMODULE hLib;

	if (m_hLib!=NULL)
		Free();
	hLib=::LoadLibrary(pszFileName);
	if (hLib==NULL) {
		SetError(TEXT("DLLがロードできません。"));
		if (::GetLastError()==ERROR_SXS_CANT_GEN_ACTCTX)
			SetErrorAdvise(TEXT("必要なランタイムがインストールされていない可能性があります。"));
		return false;
	}
	TVTest::GetVersionFunc pGetVersion=
		reinterpret_cast<TVTest::GetVersionFunc>(::GetProcAddress(hLib,"TVTGetVersion"));
	if (pGetVersion==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTGetVersion()関数のアドレスを取得できません。"));
		return false;
	}
	m_Version=pGetVersion();
	if (TVTest::GetMajorVersion(m_Version)!=TVTest::GetMajorVersion(TVTEST_PLUGIN_VERSION)
		|| TVTest::GetMinorVersion(m_Version)!=TVTest::GetMinorVersion(TVTEST_PLUGIN_VERSION)) {
		::FreeLibrary(hLib);
		SetError(TEXT("対応していないバージョンです。"));
		return false;
	}
	TVTest::GetPluginInfoFunc pGetPluginInfo=
		reinterpret_cast<TVTest::GetPluginInfoFunc>(::GetProcAddress(hLib,"TVTGetPluginInfo"));
	if (pGetPluginInfo==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTGetPluginInfo()関数のアドレスを取得できません。"));
		return false;
	}
	TVTest::PluginInfo PluginInfo;
	::ZeroMemory(&PluginInfo,sizeof(PluginInfo));
	if (!pGetPluginInfo(&PluginInfo)) {
		::FreeLibrary(hLib);
		SetError(TEXT("プラグインの情報を取得できません。"));
		return false;
	}
	TVTest::InitializeFunc pInitialize=
		reinterpret_cast<TVTest::InitializeFunc>(::GetProcAddress(hLib,"TVTInitialize"));
	if (pInitialize==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTInitialize()関数のアドレスを取得できません。"));
		return false;
	}
	m_FileName.Set(pszFileName);
	m_PluginName.Set(PluginInfo.pszPluginName);
	m_Copyright.Set(PluginInfo.pszCopyright);
	m_Description.Set(PluginInfo.pszDescription);
	m_Type=PluginInfo.Type;
	m_Flags=PluginInfo.Flags;
	m_fEnabled=(m_Flags&TVTest::PLUGIN_FLAG_ENABLEDEFAULT)!=0;
	m_PluginParam.Callback=Callback;
	m_PluginParam.hwndApp=GetAppClass().GetUICore()->GetMainWindow();
	m_PluginParam.pClientData=NULL;
	m_PluginParam.pInternalData=this;
	if (!pInitialize(&m_PluginParam)) {
		Free();
		::FreeLibrary(hLib);
		SetError(TEXT("プラグインの初期化でエラー発生しました。"));
		return false;
	}
	m_hLib=hLib;
	ClearError();
	return true;
}


void CPlugin::Free()
{
	CAppMain &App=GetAppClass();

	m_pEventCallback=NULL;
	m_pMessageCallback=NULL;

	m_GrabberLock.Lock();
	if (m_fSetGrabber) {
		for (int i=m_GrabberList.Length()-1;i>=0;i--) {
			if (m_GrabberList[i]->m_pPlugin==this)
				m_GrabberList.Delete(i);
		}
		if (m_GrabberList.Length()==0) {
			App.GetCoreEngine()->m_DtvEngine.m_MediaGrabber.SetMediaGrabCallback(NULL);
			m_fSetGrabber=false;
		}
	}
	m_GrabberLock.Unlock();

	m_AudioStreamLock.Lock();
	for (int i=m_AudioStreamCallbackList.Length()-1;i>=0;i--) {
		if (m_AudioStreamCallbackList[i]->m_pPlugin==this) {
			m_AudioStreamCallbackList.Delete(i);
			break;
		}
	}
	m_AudioStreamLock.Unlock();

	m_CommandList.DeleteAll();

	for (size_t i=0;i<m_ControllerList.size();i++)
		App.GetControllerManager()->DeleteController(m_ControllerList[i].Get());
	m_ControllerList.clear();

	if (m_hLib!=NULL) {
		LPCTSTR pszFileName=::PathFindFileName(m_FileName.Get());

		App.AddLog(TEXT("%s の終了処理を行っています..."),pszFileName);

		TVTest::FinalizeFunc pFinalize=
			reinterpret_cast<TVTest::FinalizeFunc>(::GetProcAddress(m_hLib,"TVTFinalize"));
		if (pFinalize==NULL) {
			App.AddLog(TEXT("%s のTVTFinalize()関数のアドレスを取得できません。"),
					   pszFileName);
		} else {
			// 別スレッドからFinalizeを呼ぶと不正な処理が起こるプラグインがある
			/*
			HANDLE hThread=::CreateThread(NULL,0,FinalizeThread,pFinalize,0,NULL);

			if (hThread==NULL) {
				pFinalize();
			} else {
				if (::WaitForSingleObject(hThread,m_FinalizeTimeout)==WAIT_TIMEOUT) {
					GetAppClass().AddLog(TEXT("プラグイン \"%s\" の終了処理がタイムアウトしました。"),
										 ::PathFindFileName(m_FileName.Get()));
					::TerminateThread(hThread,-1);
				}
				::CloseHandle(hThread);
			}
			*/
			pFinalize();
		}
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
		App.AddLog(TEXT("%s を解放しました。"),pszFileName);
	}

	m_FileName.Clear();
	m_PluginName.Clear();
	m_Copyright.Clear();
	m_Description.Clear();
	m_fEnabled=false;
	m_fSetting=false;
	m_PluginParam.pInternalData=NULL;
}


/*
DWORD WINAPI CPlugin::FinalizeThread(LPVOID lpParameter)
{
	TVTest::FinalizeFunc pFinalize=static_cast<TVTest::FinalizeFunc>(lpParameter);

	pFinalize();
	return 0;
}
*/


bool CPlugin::Enable(bool fEnable)
{
	if (m_fEnabled!=fEnable) {
		if (m_fSetting)
			return false;
		m_fSetting=true;
		bool fResult=SendEvent(TVTest::EVENT_PLUGINENABLE,(LPARAM)fEnable)!=0;
		m_fSetting=false;
		if (!fResult)
			return false;
		m_fEnabled=fEnable;
		if (fEnable) {
			for (size_t i=0;i<m_ControllerList.size();i++)
				GetAppClass().GetControllerManager()->LoadControllerSettings(m_ControllerList[i].Get());
		}
	}
	return true;
}


bool CPlugin::SetCommand(int Command)
{
	if (Command<CM_PLUGIN_FIRST || Command>CM_PLUGIN_LAST)
		return false;
	m_Command=Command;
	return true;
}


int CPlugin::NumPluginCommands() const
{
	return m_CommandList.Length();
}


bool CPlugin::GetPluginCommandInfo(int Index,TVTest::CommandInfo *pInfo) const
{
	if (Index<0 || Index>=m_CommandList.Length())
		return false;
	pInfo->ID=m_CommandList[Index]->GetID();
	pInfo->pszText=m_CommandList[Index]->GetText();
	pInfo->pszName=m_CommandList[Index]->GetName();
	return true;
}


bool CPlugin::NotifyCommand(LPCWSTR pszCommand)
{
	for (int i=0;i<m_CommandList.Length();i++) {
		if (::lstrcmpi(m_CommandList[i]->GetText(),pszCommand)==0) {
			SendEvent(TVTest::EVENT_COMMAND,m_CommandList[i]->GetID(),0);
			return true;
		}
	}
	return false;
}


bool CPlugin::IsDisableOnStart() const
{
	return (m_Flags&TVTest::PLUGIN_FLAG_DISABLEONSTART)!=0;
}


/*
bool CPlugin::SetFinalizeTimeout(DWORD Timeout)
{
	if (Timeout<5000)
		return false;
	m_FinalizeTimeout=Timeout;
	return true;
}
*/


LRESULT CPlugin::SendEvent(UINT Event,LPARAM lParam1,LPARAM lParam2)
{
	if (m_pEventCallback!=NULL)
		return m_pEventCallback(Event,lParam1,lParam2,m_pEventCallbackClientData);
	return 0;
}


bool CPlugin::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult)
{
	if (m_pMessageCallback!=NULL)
		return m_pMessageCallback(hwnd,uMsg,wParam,lParam,pResult,m_pMessageCallbackClientData)!=FALSE;
	return false;
}


bool CPlugin::Settings(HWND hwndOwner)
{
	if ((m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)==0)
		return false;
	if (m_fSetting)
		return true;
	m_fSetting=true;
	bool fResult=SendEvent(TVTest::EVENT_PLUGINSETTINGS,(LPARAM)hwndOwner)!=0;
	m_fSetting=false;
	return fResult;
}


inline CPlugin *ThisFromParam(TVTest::PluginParam *pParam)
{
	return static_cast<CPlugin*>(pParam->pInternalData);
}

LRESULT CALLBACK CPlugin::Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2)
{
	switch (Message) {
	case TVTest::MESSAGE_GETVERSION:
		return TVTest::MakeVersion(VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD);

	case TVTest::MESSAGE_QUERYMESSAGE:
		if (lParam1>=0 && lParam1<TVTest::MESSAGE_TRAILER)
			return TRUE;
		return FALSE;

	case TVTest::MESSAGE_MEMORYALLOC:
		{
			void *pData=reinterpret_cast<void*>(lParam1);
			SIZE_T Size=lParam2;

			if (Size>0) {
				return (LRESULT)realloc(pData,Size);
			} else if (pData!=NULL) {
				free(pData);
			}
		}
		return (LRESULT)(void*)0;

	case TVTest::MESSAGE_SETEVENTCALLBACK:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			pThis->m_pEventCallback=reinterpret_cast<TVTest::EventCallbackFunc>(lParam1);
			pThis->m_pEventCallbackClientData=reinterpret_cast<void*>(lParam2);
		}
		return TRUE;

	case TVTest::MESSAGE_GETCURRENTCHANNELINFO:
		{
			TVTest::ChannelInfo *pChannelInfo=reinterpret_cast<TVTest::ChannelInfo*>(lParam1);

			if (pChannelInfo==NULL
					|| (pChannelInfo->Size!=sizeof(TVTest::ChannelInfo)
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V1))
				return FALSE;
			const CChannelInfo *pChInfo=GetAppClass().GetCurrentChannelInfo();
			if (pChInfo==NULL)
				return FALSE;
			CTsAnalyzer *pTsAnalyzer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer;
			pChannelInfo->Space=pChInfo->GetSpace();
			pChannelInfo->Channel=pChInfo->GetChannelIndex();
			pChannelInfo->RemoteControlKeyID=pChInfo->GetChannelNo();
			pChannelInfo->NetworkID=pTsAnalyzer->GetNetworkID();
			if (!pTsAnalyzer->GetNetworkName(pChannelInfo->szNetworkName,
										lengthof(pChannelInfo->szNetworkName)))
				pChannelInfo->szNetworkName[0]='\0';
			pChannelInfo->TransportStreamID=pTsAnalyzer->GetTransportStreamID();
			if (!pTsAnalyzer->GetTsName(pChannelInfo->szTransportStreamName,
								lengthof(pChannelInfo->szTransportStreamName)))
				pChannelInfo->szTransportStreamName[0]='\0';
			::lstrcpy(pChannelInfo->szChannelName,pChInfo->GetName());
			if (pChannelInfo->Size==sizeof(TVTest::ChannelInfo)) {
				pChannelInfo->PhysicalChannel=pChInfo->GetChannel();
				pChannelInfo->ServiceIndex=pChInfo->GetService();
				pChannelInfo->ServiceID=pChInfo->GetServiceID();
			}
		}
		return TRUE;

	case TVTest::MESSAGE_SETCHANNEL:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			CAppMain &AppMain=GetAppClass();

			AppMain.OpenTuner();
			if (pThis->m_Version<TVTEST_PLUGIN_VERSION_(0,0,8))
				return AppMain.SetChannel((int)lParam1,(int)lParam2);

			int Channel=(SHORT)LOWORD(lParam2);
			WORD ServiceID=HIWORD(lParam2);
			return AppMain.SetChannel((int)lParam1,Channel,ServiceID!=0?(int)ServiceID:-1);
		}

	case TVTest::MESSAGE_GETSERVICE:
		{
			CDtvEngine *pDtvEngine=&GetAppClass().GetCoreEngine()->m_DtvEngine;
			int *pNumServices=reinterpret_cast<int*>(lParam1);
			WORD ServiceID;

			if (pNumServices)
				*pNumServices=pDtvEngine->m_TsAnalyzer.GetViewableServiceNum();
			if (!pDtvEngine->GetServiceID(&ServiceID))
				return -1;
			return pDtvEngine->m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);;
		}

	case TVTest::MESSAGE_SETSERVICE:
		{
			if (lParam2==0)
				return GetAppClass().SetServiceByIndex((int)lParam1);
			return GetAppClass().SetServiceByID((WORD)lParam1);
		}

	case TVTest::MESSAGE_GETTUNINGSPACENAME:
		{
			LPWSTR pszName=reinterpret_cast<LPWSTR>(lParam1);
			int Index=LOWORD(lParam2);
			int MaxLength=HIWORD(lParam2);
			const CTuningSpaceList *pTuningSpaceList=GetAppClass().GetChannelManager()->GetDriverTuningSpaceList();
			LPCTSTR pszTuningSpaceName=pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName==NULL)
				return 0;
			if (pszName!=NULL)
				::lstrcpyn(pszName,pszTuningSpaceName,MaxLength);
			return ::lstrlen(pszTuningSpaceName);
		}

	case TVTest::MESSAGE_GETCHANNELINFO:
		{
			TVTest::ChannelInfo *pChannelInfo=reinterpret_cast<TVTest::ChannelInfo*>(lParam1);
			int Space=LOWORD(lParam2);
			int Channel=HIWORD(lParam2);

			if (pChannelInfo==NULL
					|| (pChannelInfo->Size!=sizeof(TVTest::ChannelInfo)
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V1)
					|| Space<0 || Channel<0)
				return FALSE;

			const CChannelManager *pChannelManager=GetAppClass().GetChannelManager();
			const CChannelList *pChannelList=pChannelManager->GetChannelList(Space);
			const CChannelInfo *pChInfo;

			if (pChannelList==NULL)
				return FALSE;
			pChInfo=pChannelList->GetChannelInfo(Channel);
			if (pChInfo==NULL)
				return FALSE;
			pChannelInfo->Space=pChInfo->GetSpace();
			pChannelInfo->Channel=pChInfo->GetChannelIndex();
			pChannelInfo->RemoteControlKeyID=pChInfo->GetChannelNo();
			pChannelInfo->NetworkID=pChInfo->GetNetworkID();
			pChannelInfo->szNetworkName[0]='\0';
			pChannelInfo->TransportStreamID=pChInfo->GetTransportStreamID();
			pChannelInfo->szTransportStreamName[0]='\0';
			::lstrcpy(pChannelInfo->szChannelName,pChInfo->GetName());
			if (pChannelInfo->Size==sizeof(TVTest::ChannelInfo)) {
				pChannelInfo->PhysicalChannel=pChInfo->GetChannel();
				pChannelInfo->ServiceIndex=pChInfo->GetService();
				pChannelInfo->ServiceID=pChInfo->GetServiceID();
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETSERVICEINFO:
		{
			int Index=(int)lParam1;
			TVTest::ServiceInfo *pServiceInfo=reinterpret_cast<TVTest::ServiceInfo*>(lParam2);

			if (Index<0 || pServiceInfo==NULL
					|| (pServiceInfo->Size!=sizeof(TVTest::ServiceInfo)
						&& pServiceInfo->Size!=TVTest::SERVICEINFO_SIZE_V1))
				return FALSE;
			CTsAnalyzer *pTsAnalyzer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer;
			WORD ServiceID;
			CTsAnalyzer::ServiceInfo Info;
			if (!pTsAnalyzer->GetViewableServiceID(Index,&ServiceID)
					|| !pTsAnalyzer->GetServiceInfo(pTsAnalyzer->GetServiceIndexByID(ServiceID),&Info))
				return FALSE;
			pServiceInfo->ServiceID=ServiceID;
			pServiceInfo->VideoPID=Info.VideoEs.PID;
			pServiceInfo->NumAudioPIDs=(int)Info.AudioEsList.size();
			for (size_t i=0;i<Info.AudioEsList.size();i++)
				pServiceInfo->AudioPID[i]=Info.AudioEsList[i].PID;
			::lstrcpyn(pServiceInfo->szServiceName,Info.szServiceName,32);
			if (pServiceInfo->Size==sizeof(TVTest::ServiceInfo)) {
				int ServiceIndex=pTsAnalyzer->GetServiceIndexByID(ServiceID);
				for (size_t i=0;i<Info.AudioEsList.size();i++) {
					pServiceInfo->AudioComponentType[i]=
						pTsAnalyzer->GetAudioComponentType(ServiceIndex,(int)i);
				}
				if (Info.CaptionEsList.size()>0)
					pServiceInfo->SubtitlePID=Info.CaptionEsList[0].PID;
				else
					pServiceInfo->SubtitlePID=0;
				pServiceInfo->Reserved=0;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERNAME:
		{
			LPWSTR pszName=reinterpret_cast<LPWSTR>(lParam1);
			int MaxLength=(int)lParam2;
			LPCTSTR pszDriverName=GetAppClass().GetCoreEngine()->GetDriverFileName();

			if (pszName!=NULL && MaxLength>0)
				::lstrcpyn(pszName,pszDriverName,MaxLength);
			return ::lstrlen(pszDriverName);
		}

	case TVTest::MESSAGE_SETDRIVERNAME:
		{
			LPCWSTR pszDriverName=reinterpret_cast<LPCWSTR>(lParam1);

			if (pszDriverName==NULL)
				return FALSE;
			return GetAppClass().SetDriver(pszDriverName);
		}

	case TVTest::MESSAGE_STARTRECORD:
		{
			CAppMain &App=GetAppClass();
			TVTest::RecordInfo *pInfo=reinterpret_cast<TVTest::RecordInfo*>(lParam1);
			CRecordManager::TimeSpecInfo StartTime,StopTime;

			if (pInfo==NULL)
				return App.StartRecord(NULL,NULL,NULL,CRecordManager::CLIENT_PLUGIN);
			if (pInfo->Size!=sizeof(TVTest::RecordInfo))
				return FALSE;
			StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
			if ((pInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				switch (pInfo->StartTimeSpec) {
				case TVTest::RECORD_START_NOTSPECIFIED:
					break;
				case TVTest::RECORD_START_TIME:
					StartTime.Type=CRecordManager::TIME_DATETIME;
					StartTime.Time.DateTime=pInfo->StartTime.Time;
					break;
				case TVTest::RECORD_START_DELAY:
					StartTime.Type=CRecordManager::TIME_DURATION;
					StartTime.Time.Duration=pInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
			if ((pInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				switch (pInfo->StopTimeSpec) {
				case TVTest::RECORD_STOP_NOTSPECIFIED:
					break;
				case TVTest::RECORD_STOP_TIME:
					StopTime.Type=CRecordManager::TIME_DATETIME;
					StopTime.Time.DateTime=pInfo->StopTime.Time;
					break;
				case TVTest::RECORD_STOP_DURATION:
					StopTime.Type=CRecordManager::TIME_DURATION;
					StopTime.Time.Duration=pInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.StartRecord(
				(pInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0?
													pInfo->pszFileName:NULL,
				&StartTime,&StopTime,
				CRecordManager::CLIENT_PLUGIN);
		}

	case TVTest::MESSAGE_STOPRECORD:
		return GetAppClass().StopRecord();

	case TVTest::MESSAGE_PAUSERECORD:
		{
			CAppMain &App=GetAppClass();
			const CRecordManager *pRecordManager=App.GetRecordManager();
			bool fPause=lParam1!=0;

			if (!pRecordManager->IsRecording())
				return FALSE;
			if (fPause==pRecordManager->IsPaused())
				return FALSE;
			App.GetUICore()->DoCommand(CM_RECORD_PAUSE);
		}
		return TRUE;

	case TVTest::MESSAGE_GETRECORD:
		{
			TVTest::RecordInfo *pInfo=reinterpret_cast<TVTest::RecordInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::RecordInfo))
				return FALSE;
			const CRecordManager *pRecordManager=GetAppClass().GetRecordManager();
			if ((pInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0
					&& pInfo->pszFileName!=NULL && pInfo->MaxFileName>0) {
				if (pRecordManager->GetFileName()!=NULL)
					::lstrcpyn(pInfo->pszFileName,pRecordManager->GetFileName(),
							   pInfo->MaxFileName);
				else
					pInfo->pszFileName[0]='\0';
			}
			pRecordManager->GetReserveTime(&pInfo->ReserveTime);
			if ((pInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				CRecordManager::TimeSpecInfo StartTime;

				if (!pRecordManager->GetStartTimeSpec(&StartTime))
					StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
				switch (StartTime.Type) {
				case CRecordManager::TIME_NOTSPECIFIED:
					pInfo->StartTimeSpec=TVTest::RECORD_START_NOTSPECIFIED;
					break;
				case CRecordManager::TIME_DATETIME:
					pInfo->StartTimeSpec=TVTest::RECORD_START_TIME;
					pInfo->StartTime.Time=StartTime.Time.DateTime;
					break;
				case CRecordManager::TIME_DURATION:
					pInfo->StartTimeSpec=TVTest::RECORD_START_DELAY;
					pInfo->StartTime.Delay=StartTime.Time.Duration;
					break;
				}
			}
			if ((pInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				CRecordManager::TimeSpecInfo StopTime;

				if (!pRecordManager->GetStopTimeSpec(&StopTime))
					StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
				switch (StopTime.Type) {
				case CRecordManager::TIME_NOTSPECIFIED:
					pInfo->StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
					break;
				case CRecordManager::TIME_DATETIME:
					pInfo->StopTimeSpec=TVTest::RECORD_STOP_TIME;
					pInfo->StopTime.Time=StopTime.Time.DateTime;
					break;
				case CRecordManager::TIME_DURATION:
					pInfo->StopTimeSpec=TVTest::RECORD_STOP_DURATION;
					pInfo->StopTime.Duration=StopTime.Time.Duration;
					break;
				}
			}
		}
		return TRUE;

	case TVTest::MESSAGE_MODIFYRECORD:
		{
			CAppMain &App=GetAppClass();
			TVTest::RecordInfo *pInfo=reinterpret_cast<TVTest::RecordInfo*>(lParam1);
			CRecordManager::TimeSpecInfo StartTime,StopTime;

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::RecordInfo))
				return false;
			if ((pInfo->Mask&TVTest::RECORD_MASK_FLAGS)!=0) {
				if ((pInfo->Flags&TVTest::RECORD_FLAG_CANCEL)!=0)
					return App.CancelReservedRecord();
			}
			if ((pInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				switch (pInfo->StartTimeSpec) {
				case TVTest::RECORD_START_NOTSPECIFIED:
					StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
					break;
				case TVTest::RECORD_START_TIME:
					StartTime.Type=CRecordManager::TIME_DATETIME;
					StartTime.Time.DateTime=pInfo->StartTime.Time;
					break;
				case TVTest::RECORD_START_DELAY:
					StartTime.Type=CRecordManager::TIME_DURATION;
					StartTime.Time.Duration=pInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			if ((pInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				switch (pInfo->StopTimeSpec) {
				case TVTest::RECORD_STOP_NOTSPECIFIED:
					StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
					break;
				case TVTest::RECORD_STOP_TIME:
					StopTime.Type=CRecordManager::TIME_DATETIME;
					StopTime.Time.DateTime=pInfo->StopTime.Time;
					break;
				case TVTest::RECORD_STOP_DURATION:
					StopTime.Type=CRecordManager::TIME_DURATION;
					StopTime.Time.Duration=pInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.ModifyRecord(
				(pInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0?pInfo->pszFileName:NULL,
				(pInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0?&StartTime:NULL,
				(pInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0?&StopTime:NULL,
				CRecordManager::CLIENT_PLUGIN);

		}

	case TVTest::MESSAGE_GETZOOM:
		return GetAppClass().GetUICore()->GetZoomPercentage();

	case TVTest::MESSAGE_SETZOOM:
		return GetAppClass().GetUICore()->SetZoomRate((int)lParam1,(int)lParam2);

	case TVTest::MESSAGE_GETPANSCAN:
		{
			TVTest::PanScanInfo *pInfo=reinterpret_cast<TVTest::PanScanInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanScanInfo))
				return FALSE;
			const CMediaViewer *pMediaViewer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer;
			pMediaViewer->GetForceAspectRatio(&pInfo->XAspect,&pInfo->YAspect);
			BYTE PanScan=pMediaViewer->GetPanAndScan();
			switch (PanScan) {
			case CMediaViewer::PANANDSCAN_HORZ_CUT | CMediaViewer::PANANDSCAN_VERT_NONE:
				pInfo->Type=TVTest::PANSCAN_SIDECUT;
				break;
			case CMediaViewer::PANANDSCAN_HORZ_NONE | CMediaViewer::PANANDSCAN_VERT_CUT:
				pInfo->Type=TVTest::PANSCAN_LETTERBOX;
				break;
			case CMediaViewer::PANANDSCAN_HORZ_CUT | CMediaViewer::PANANDSCAN_VERT_CUT:
				pInfo->Type=TVTest::PANSCAN_SUPERFRAME;
				break;
			default:
				pInfo->Type=TVTest::PANSCAN_NONE;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_SETPANSCAN:
		{
			const TVTest::PanScanInfo *pInfo=reinterpret_cast<const TVTest::PanScanInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanScanInfo))
				return FALSE;
			const CMediaViewer *pMediaViewer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer;
			int Command;
			switch (pInfo->Type) {
			case TVTest::PANSCAN_NONE:
				if (pInfo->XAspect==0 && pInfo->YAspect==0)
					Command=CM_ASPECTRATIO_DEFAULT;
				else if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_16x9;
				else if (pInfo->XAspect==4 && pInfo->YAspect==3)
					Command=CM_ASPECTRATIO_4x3;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_LETTERBOX:
				if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_LETTERBOX;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_SIDECUT:
				if (pInfo->XAspect==4 && pInfo->YAspect==3)
					Command=CM_ASPECTRATIO_SIDECUT;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_SUPERFRAME:
				if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_LETTERBOX;
				else
					return FALSE;
				break;
			default:
				return FALSE;
			}
			GetAppClass().GetUICore()->DoCommand(Command);
		}
		return TRUE;

	case TVTest::MESSAGE_GETSTATUS:
		{
			TVTest::StatusInfo *pInfo=reinterpret_cast<TVTest::StatusInfo*>(lParam1);

			if (pInfo==NULL || (pInfo->Size!=sizeof(TVTest::StatusInfo)
								&& pInfo->Size!=TVTest::STATUSINFO_SIZE_V1))
				return FALSE;
			const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
			DWORD DropCount=pCoreEngine->GetContinuityErrorPacketCount();
			pInfo->SignalLevel=pCoreEngine->GetSignalLevel();
			pInfo->BitRate=pCoreEngine->GetBitRate();
			pInfo->ErrorPacketCount=pCoreEngine->GetErrorPacketCount()+DropCount;
			pInfo->ScramblePacketCount=pCoreEngine->GetScramblePacketCount();
			if (pInfo->Size==sizeof(TVTest::StatusInfo)) {
				pInfo->DropPacketCount=DropCount;
				if (pCoreEngine->GetDescramble()
					&& pCoreEngine->GetCardReaderType()!=CCoreEngine::CARDREADER_NONE) {
					if (pCoreEngine->m_DtvEngine.m_TsDescrambler.IsBcasCardOpen()) {
						pInfo->BcasCardStatus=TVTest::BCAS_STATUS_OK;
					} else {
						// 取りあえず...
						pInfo->BcasCardStatus=TVTest::BCAS_STATUS_OPENERROR;
					}
				} else {
					pInfo->BcasCardStatus=TVTest::BCAS_STATUS_NOTOPEN;
				}
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETRECORDSTATUS:
		{
			TVTest::RecordStatusInfo *pInfo=reinterpret_cast<TVTest::RecordStatusInfo*>(lParam1);
			const CRecordManager *pRecordManager=GetAppClass().GetRecordManager();

			if (pInfo==NULL
					|| (pInfo->Size!=sizeof(TVTest::RecordStatusInfo)
						&& pInfo->Size!=TVTest::RECORDSTATUSINFO_SIZE_V1))
				return FALSE;
			pInfo->Status=pRecordManager->IsRecording()?
				(pRecordManager->IsPaused()?TVTest::RECORD_STATUS_PAUSED:
											TVTest::RECORD_STATUS_RECORDING):
											TVTest::RECORD_STATUS_NOTRECORDING;
			if (pInfo->Status!=TVTest::RECORD_STATUS_NOTRECORDING) {
				pRecordManager->GetStartTime(&pInfo->StartTime);
				CRecordManager::TimeSpecInfo StopTimeInfo;
				pRecordManager->GetStopTimeSpec(&StopTimeInfo);
				pInfo->StopTimeSpec=(DWORD)StopTimeInfo.Type;
				if (StopTimeInfo.Type==CRecordManager::TIME_DATETIME)
					pInfo->StopTime.Time=StopTimeInfo.Time.DateTime;
				else
					pInfo->StopTime.Duration=StopTimeInfo.Time.Duration;
			} else {
				pInfo->StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
			}
			pInfo->RecordTime=pRecordManager->GetRecordTime();
			pInfo->PauseTime=pRecordManager->GetPauseTime();
			if (pInfo->Size>TVTest::RECORDSTATUSINFO_SIZE_V1) {
				if (pInfo->pszFileName!=NULL && pInfo->MaxFileName>0) {
					if (pRecordManager->IsRecording()) {
						::lstrcpyn(pInfo->pszFileName,
								   pRecordManager->GetRecordTask()->GetFileName(),
								   pInfo->MaxFileName);
					} else {
						pInfo->pszFileName[0]='\0';
					}
				}
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETVIDEOINFO:
		{
			TVTest::VideoInfo *pVideoInfo=reinterpret_cast<TVTest::VideoInfo*>(lParam1);
			CMediaViewer *pMediaViewer;
			WORD VideoWidth,VideoHeight;
			BYTE XAspect,YAspect;

			if (pVideoInfo==NULL || pVideoInfo->Size!=sizeof(TVTest::VideoInfo))
				return FALSE;
			pMediaViewer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer;
			if (pMediaViewer->GetOriginalVideoSize(&VideoWidth,&VideoHeight)
					&& pMediaViewer->GetVideoAspectRatio(&XAspect,&YAspect)
					&& pMediaViewer->GetSourceRect(&pVideoInfo->SourceRect)) {
				pVideoInfo->Width=VideoWidth;
				pVideoInfo->Height=VideoHeight;
				pVideoInfo->XAspect=XAspect;
				pVideoInfo->YAspect=YAspect;
				return TRUE;
			}
		}
		return FALSE;

	case TVTest::MESSAGE_GETVOLUME:
		{
			const CUICore *pUICore=GetAppClass().GetUICore();
			int Volume=pUICore->GetVolume();
			bool fMute=pUICore->GetMute();

			return MAKELONG(Volume,fMute);
		}

	case TVTest::MESSAGE_SETVOLUME:
		{
			CUICore *pUICore=GetAppClass().GetUICore();
			int Volume=(int)lParam1;

			if (Volume<0)
				return pUICore->SetMute(lParam2!=0);
			return pUICore->SetVolume(Volume,true);
		}

	case TVTest::MESSAGE_GETSTEREOMODE:
		return GetAppClass().GetUICore()->GetStereoMode();

	case TVTest::MESSAGE_SETSTEREOMODE:
		return GetAppClass().GetUICore()->SetStereoMode((int)lParam1);

	case TVTest::MESSAGE_GETFULLSCREEN:
		return GetAppClass().GetUICore()->GetFullscreen();

	case TVTest::MESSAGE_SETFULLSCREEN:
		return GetAppClass().GetUICore()->SetFullscreen(lParam1!=0);

	case TVTest::MESSAGE_GETPREVIEW:
		return GetAppClass().GetUICore()->IsViewerEnabled();

	case TVTest::MESSAGE_SETPREVIEW:
		return GetAppClass().GetUICore()->EnableViewer(lParam1!=0);

	case TVTest::MESSAGE_GETSTANDBY:
		return GetAppClass().GetUICore()->GetStandby();

	case TVTest::MESSAGE_SETSTANDBY:
		return GetAppClass().GetUICore()->SetStandby(lParam1!=0);

	case TVTest::MESSAGE_GETALWAYSONTOP:
		return GetAppClass().GetUICore()->GetAlwaysOnTop();

	case TVTest::MESSAGE_SETALWAYSONTOP:
		GetAppClass().GetUICore()->SetAlwaysOnTop(lParam1!=0);
		return TRUE;

	case TVTest::MESSAGE_CAPTUREIMAGE:
		{
			void *pBuffer=GetAppClass().GetCoreEngine()->GetCurrentImage();

			if (pBuffer!=NULL) {
				SIZE_T Size=CalcDIBSize(static_cast<BITMAPINFOHEADER*>(pBuffer));
				void *pDib;

				pDib=malloc(Size);
				if (pDib!=NULL)
					::CopyMemory(pDib,pBuffer,Size);
				::CoTaskMemFree(pBuffer);
				return (LRESULT)pDib;
			}
		}
		return (LRESULT)(LPVOID)NULL;

	case TVTest::MESSAGE_SAVEIMAGE:
		GetAppClass().GetUICore()->DoCommand(CM_SAVEIMAGE);
		return TRUE;

	case TVTest::MESSAGE_RESET:
		{
			DWORD Flags=(DWORD)lParam1;

			if (Flags==TVTest::RESET_ALL)
				GetAppClass().GetUICore()->DoCommand(CM_RESET);
			else if (Flags==TVTest::RESET_VIEWER)
				GetAppClass().GetUICore()->DoCommand(CM_RESETVIEWER);
			else
				return FALSE;
		}
		return TRUE;

	case TVTest::MESSAGE_CLOSE:
		::PostMessage(GetAppClass().GetUICore()->GetMainWindow(),
					  WM_COMMAND,
					  (lParam1&TVTest::CLOSE_EXIT)!=0?CM_EXIT:CM_CLOSE,0);
		return TRUE;

	case TVTest::MESSAGE_SETSTREAMCALLBACK:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			TVTest::StreamCallbackInfo *pInfo=reinterpret_cast<TVTest::StreamCallbackInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::StreamCallbackInfo))
				return FALSE;

			CBlockLock Lock(&m_GrabberLock);

			if ((pInfo->Flags&TVTest::STREAM_CALLBACK_REMOVE)==0) {
				if (!m_fSetGrabber) {
					CMediaGrabber &MediaGrabber=GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaGrabber;

					MediaGrabber.SetMediaGrabCallback(GrabMediaCallback,&m_GrabberList);
					m_fSetGrabber=true;
				}
				m_GrabberList.Add(new CMediaGrabberInfo(pThis,pInfo));
			} else {
				int i;

				for (i=m_GrabberList.Length()-1;i>=0;i--) {
					if (m_GrabberList[i]->m_pPlugin==pThis
							&& m_GrabberList[i]->m_CallbackInfo.Callback==pInfo->Callback) {
						m_GrabberList.Delete(i);
						break;
					}
				}
				if (i<0)
					return FALSE;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ENABLEPLUGIN:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			return pThis->Enable(lParam1!=0);
		}

	case TVTest::MESSAGE_GETCOLOR:
		{
			LPCWSTR pszName=reinterpret_cast<LPCWSTR>(lParam1);

			if (pszName==NULL)
				return CLR_INVALID;
			return GetAppClass().GetColor(pszName);
		}

	case TVTest::MESSAGE_DECODEARIBSTRING:
		{
			TVTest::ARIBStringDecodeInfo *pInfo=reinterpret_cast<TVTest::ARIBStringDecodeInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::ARIBStringDecodeInfo)
					|| pInfo->pSrcData==NULL
					|| pInfo->pszDest==NULL || pInfo->DestLength==0)
				return FALSE;
			if (CAribString::AribToString(pInfo->pszDest,pInfo->DestLength,
					static_cast<const BYTE*>(pInfo->pSrcData),pInfo->SrcLength)==0) {
				pInfo->pszDest[0]='\0';
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETCURRENTPROGRAMINFO:
		{
			TVTest::ProgramInfo *pProgramInfo=reinterpret_cast<TVTest::ProgramInfo*>(lParam1);

			if (pProgramInfo==NULL
					|| pProgramInfo->Size!=sizeof(TVTest::ProgramInfo))
				return FALSE;

			const bool fNext=lParam2!=0;
			CDtvEngine *pDtvEngine=&GetAppClass().GetCoreEngine()->m_DtvEngine;

			pProgramInfo->ServiceID=0;
			pDtvEngine->GetServiceID(&pProgramInfo->ServiceID);
			pProgramInfo->EventID=pDtvEngine->GetEventID(fNext);
			if (pProgramInfo->pszEventName!=NULL && pProgramInfo->MaxEventName>0) {
				pProgramInfo->pszEventName[0]='\0';
				pDtvEngine->GetEventName(pProgramInfo->pszEventName,
										 pProgramInfo->MaxEventName,fNext);
			}
			if (pProgramInfo->pszEventText!=NULL && pProgramInfo->MaxEventText>0) {
				pProgramInfo->pszEventText[0]='\0';
				pDtvEngine->GetEventText(pProgramInfo->pszEventText,
										 pProgramInfo->MaxEventText,fNext);
			}
			if (pProgramInfo->pszEventExtText!=NULL && pProgramInfo->MaxEventExtText>0) {
				pProgramInfo->pszEventExtText[0]='\0';
				pDtvEngine->GetEventExtendedText(pProgramInfo->pszEventExtText,
												 pProgramInfo->MaxEventExtText,
												 fNext);
			}
			pDtvEngine->GetEventTime(&pProgramInfo->StartTime,NULL,fNext);
			pProgramInfo->Duration=pDtvEngine->GetEventDuration(fNext);
		}
		return TRUE;

	case TVTest::MESSAGE_QUERYEVENT:
		switch (lParam1) {
		case TVTest::EVENT_PLUGINENABLE:
		case TVTest::EVENT_PLUGINSETTINGS:
		case TVTest::EVENT_CHANNELCHANGE:
		case TVTest::EVENT_SERVICECHANGE:
		case TVTest::EVENT_DRIVERCHANGE:
		case TVTest::EVENT_SERVICEUPDATE:
		case TVTest::EVENT_RECORDSTATUSCHANGE:
		case TVTest::EVENT_FULLSCREENCHANGE:
		case TVTest::EVENT_PREVIEWCHANGE:
		case TVTest::EVENT_VOLUMECHANGE:
		case TVTest::EVENT_STEREOMODECHANGE:
		case TVTest::EVENT_COLORCHANGE:
		case TVTest::EVENT_STANDBY:
		case TVTest::EVENT_EXECUTE:
		case TVTest::EVENT_RESET:
		case TVTest::EVENT_STATUSRESET:
		case TVTest::EVENT_AUDIOSTREAMCHANGE:
			return TRUE;
		}
		return FALSE;

	case TVTest::MESSAGE_GETTUNINGSPACE:
		{
			const CChannelManager *pChannelManager=GetAppClass().GetChannelManager();
			int *pNumSpaces=reinterpret_cast<int*>(lParam1);
			int CurSpace;

			if (pNumSpaces!=NULL)
				*pNumSpaces=pChannelManager->GetDriverTuningSpaceList()->NumSpaces();
			CurSpace=pChannelManager->GetCurrentSpace();
			if (CurSpace<0) {
				const CChannelInfo *pChannelInfo;

				if (CurSpace==CChannelManager::SPACE_ALL
						&& (pChannelInfo=pChannelManager->GetCurrentChannelInfo())!=NULL) {
					CurSpace=pChannelInfo->GetSpace();
				} else {
					CurSpace=-1;
				}
			}
			return CurSpace;
		}

	case TVTest::MESSAGE_GETTUNINGSPACEINFO:
		{
			int Index=(int)lParam1;
			TVTest::TuningSpaceInfo *pInfo=reinterpret_cast<TVTest::TuningSpaceInfo*>(lParam2);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::TuningSpaceInfo))
				return FALSE;

			const CTuningSpaceList *pTuningSpaceList=GetAppClass().GetChannelManager()->GetDriverTuningSpaceList();
			LPCTSTR pszTuningSpaceName=pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName==NULL)
				return FALSE;
			::lstrcpyn(pInfo->szName,pszTuningSpaceName,lengthof(pInfo->szName));
			pInfo->Space=(int)pTuningSpaceList->GetTuningSpaceType(Index);
		}
		return TRUE;

	case TVTest::MESSAGE_SETNEXTCHANNEL:
		{
			bool fNext=lParam1&1;

			GetAppClass().GetUICore()->DoCommand(fNext?CM_CHANNEL_UP:CM_CHANNEL_DOWN);
		}
		return TRUE;

	case TVTest::MESSAGE_GETAUDIOSTREAM:
		return GetAppClass().GetUICore()->GetAudioStream();

	case TVTest::MESSAGE_SETAUDIOSTREAM:
		{
			CUICore *pUICore=GetAppClass().GetUICore();
			int Index=(int)lParam1;

			if (Index<0 || Index>=pUICore->GetNumAudioStreams()
					|| !pUICore->SetAudioStream(Index))
				return FALSE;
		}
		return TRUE;

	case TVTest::MESSAGE_ISPLUGINENABLED:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			return pThis->IsEnabled();
		}

	case TVTest::MESSAGE_REGISTERCOMMAND:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			const TVTest::CommandInfo *pCommandList=reinterpret_cast<TVTest::CommandInfo*>(lParam1);
			int NumCommands=(int)lParam2;

			if (pCommandList==NULL || NumCommands<=0)
				return FALSE;
			for (int i=0;i<NumCommands;i++) {
				pThis->m_CommandList.Add(new CPluginCommandInfo(pCommandList[i]));
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ADDLOG:
		{
			LPCWSTR pszText=reinterpret_cast<LPCWSTR>(lParam1);
			if (pszText==NULL)
				return FALSE;

			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			LPCTSTR pszFileName=::PathFindFileName(pThis->m_FileName.Get());
			GetAppClass().AddLog(TEXT("%s : %s"),pszFileName,pszText);
		}
		return TRUE;

	case TVTest::MESSAGE_RESETSTATUS:
		GetAppClass().GetUICore()->DoCommand(CM_RESETERRORCOUNT);
		return TRUE;

	case TVTest::MESSAGE_SETAUDIOCALLBACK:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			TVTest::AudioCallbackFunc pCallback=reinterpret_cast<TVTest::AudioCallbackFunc>(lParam1);

			m_AudioStreamLock.Lock();
			if (pCallback!=NULL) {
				if (m_AudioStreamCallbackList.Length()==0) {
					GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetAudioStreamCallback(AudioStreamCallback);
				} else {
					for (int i=m_AudioStreamCallbackList.Length()-1;i>=0;i--) {
						if (m_AudioStreamCallbackList[i]->m_pPlugin==pThis) {
							m_AudioStreamCallbackList.Delete(i);
							break;
						}
					}
				}
				m_AudioStreamCallbackList.Add(new CAudioStreamCallbackInfo(pThis,pCallback,reinterpret_cast<void*>(lParam2)));
				m_AudioStreamLock.Unlock();
			} else {
				int i;

				for (i=m_AudioStreamCallbackList.Length()-1;i>=0;i--) {
					if (m_AudioStreamCallbackList[i]->m_pPlugin==pThis) {
						m_AudioStreamCallbackList.Delete(i);
						break;
					}
				}
				m_AudioStreamLock.Unlock();
				if (i<0)
					return FALSE;
				if (m_AudioStreamCallbackList.Length()==0)
					GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetAudioStreamCallback(NULL);
			}
		}
		return TRUE;

	case TVTest::MESSAGE_DOCOMMAND:
		{
			LPCWSTR pszCommand=reinterpret_cast<LPCWSTR>(lParam1);

			if (pszCommand==NULL || pszCommand[0]=='\0')
				return FALSE;
			return GetAppClass().GetUICore()->DoCommand(pszCommand);
		}

	case TVTest::MESSAGE_GETBCASINFO:
		{
			TVTest::BCasInfo *pBCasInfo=reinterpret_cast<TVTest::BCasInfo*>(lParam1);

			if (pBCasInfo==NULL || pBCasInfo->Size!=sizeof(TVTest::BCasInfo))
				return FALSE;

			CTsDescrambler *pDescrambler=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsDescrambler;
			CBcasCard::BcasCardInfo CardInfo;
			if (!pDescrambler->GetBcasCardInfo(&CardInfo))
				return FALSE;
			pBCasInfo->CASystemID=CardInfo.CASystemID;
			::CopyMemory(pBCasInfo->CardID,CardInfo.BcasCardID,6);
			pBCasInfo->CardType=CardInfo.CardType;
			pBCasInfo->MessagePartitionLength=CardInfo.MessagePartitionLength;
			::CopyMemory(pBCasInfo->SystemKey,CardInfo.SystemKey,32);
			::CopyMemory(pBCasInfo->InitialCBC,CardInfo.InitialCbc,8);
			pBCasInfo->CardManufacturerID=CardInfo.CardManufacturerID;
			pBCasInfo->CheckCode=CardInfo.CheckCode;
#ifdef UNICODE
			WCHAR szCardID[25];
			pDescrambler->FormatBcasCardID(szCardID,lengthof(szCardID));
			::WideCharToMultiByte(CP_ACP,0,szCardID,-1,
								  pBCasInfo->szFormatCardID,lengthof(pBCasInfo->szFormatCardID),NULL,NULL);
#else
			pDescrambler->FormatBcasCardID(pBCasInfo->szFormatCardID,lengthof(pBCasInfo->szFormatCardID));
#endif

		}
		return TRUE;

	case TVTest::MESSAGE_SENDBCASCOMMAND:
		{
			TVTest::BCasCommandInfo *pBCasCommand=reinterpret_cast<TVTest::BCasCommandInfo*>(lParam1);

			if (pBCasCommand==NULL
					|| pBCasCommand->pSendData==NULL || pBCasCommand->SendSize==0
					|| pBCasCommand->pReceiveData==NULL)
				return FALSE;

			return GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsDescrambler.SendBcasCommand(
				pBCasCommand->pSendData, pBCasCommand->SendSize,
				pBCasCommand->pReceiveData, &pBCasCommand->ReceiveSize);
		}

	case TVTest::MESSAGE_GETHOSTINFO:
		{
			TVTest::HostInfo *pHostInfo=reinterpret_cast<TVTest::HostInfo*>(lParam1);

			if (pHostInfo==NULL || pHostInfo->Size!=sizeof(TVTest::HostInfo))
				return FALSE;
			pHostInfo->pszAppName=APP_NAME_W;
			pHostInfo->Version.Major=VERSION_MAJOR;
			pHostInfo->Version.Minor=VERSION_MINOR;
			pHostInfo->Version.Build=VERSION_BUILD;
			pHostInfo->pszVersionText=VERSION_TEXT_W;
			pHostInfo->SupportedPluginVersion=TVTEST_PLUGIN_VERSION;
		}
		return TRUE;

	case TVTest::MESSAGE_GETSETTING:
		{
			TVTest::SettingInfo *pSetting=reinterpret_cast<TVTest::SettingInfo*>(lParam1);

			if (pSetting==NULL || pSetting->pszName==NULL)
				return FALSE;
			if (::lstrcmpi(pSetting->pszName,TEXT("DriverDirectory"))==0) {
				if (pSetting->Type!=TVTest::SETTING_TYPE_STRING)
					return FALSE;
				TCHAR szDirectory[MAX_PATH];
				GetAppClass().GetDriverDirectory(szDirectory);
				if (pSetting->Value.pszString!=NULL)
					::lstrcpyn(pSetting->Value.pszString,szDirectory,pSetting->ValueSize);
				else
					pSetting->ValueSize=(::lstrlen(szDirectory)+1)*sizeof(WCHAR);
			} else if (::lstrcmpi(pSetting->pszName,TEXT("IniFilePath"))==0) {
				if (pSetting->Type!=TVTest::SETTING_TYPE_STRING)
					return FALSE;
				LPCTSTR pszPath=GetAppClass().GetIniFileName();
				if (pSetting->Value.pszString!=NULL)
					::lstrcpyn(pSetting->Value.pszString,pszPath,pSetting->ValueSize);
				else
					pSetting->ValueSize=(::lstrlen(pszPath)+1)*sizeof(WCHAR);
			} else if (::lstrcmpi(pSetting->pszName,TEXT("RecordFolder"))==0) {
				if (pSetting->Type!=TVTest::SETTING_TYPE_STRING)
					return FALSE;
				LPCTSTR pszFolder=GetAppClass().GetDefaultRecordFolder();
				if (pSetting->Value.pszString!=NULL)
					::lstrcpyn(pSetting->Value.pszString,pszFolder,pSetting->ValueSize);
				else
					pSetting->ValueSize=(::lstrlen(pszFolder)+1)*sizeof(WCHAR);
			} else {
				return FALSE;
			}
			if (pSetting->Type==TVTest::SETTING_TYPE_STRING
					&& pSetting->Value.pszString!=NULL)
				pSetting->ValueSize=(::lstrlen(pSetting->Value.pszString)+1)*sizeof(WCHAR);
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERFULLPATHNAME:
		{
			LPWSTR pszPath=reinterpret_cast<LPWSTR>(lParam1);
			int MaxLength=(int)lParam2;
			TCHAR szFileName[MAX_PATH];

			if (!GetAppClass().GetCoreEngine()->GetDriverPath(szFileName))
				return 0;
			if (pszPath!=NULL && MaxLength>0)
				::lstrcpyn(pszPath,szFileName,MaxLength);
			return ::lstrlen(szFileName);
		}

	case TVTest::MESSAGE_GETLOGO:
		{
			const WORD NetworkID=LOWORD(lParam1),ServiceID=HIWORD(lParam1);
			const BYTE LogoType=(BYTE)(lParam2&0xFF);
			CLogoManager *pLogoManager=GetAppClass().GetLogoManager();
			HBITMAP hbm=pLogoManager->GetAssociatedLogoBitmap(NetworkID,ServiceID,LogoType);
			if (hbm!=NULL) {
				return reinterpret_cast<LRESULT>(::CopyImage(hbm,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION));
			}
		}
		return reinterpret_cast<LRESULT>((HBITMAP)NULL);

	case TVTest::MESSAGE_GETAVAILABLELOGOTYPE:
		{
			const WORD NetworkID=LOWORD(lParam1),ServiceID=HIWORD(lParam1);
			CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

			return pLogoManager->GetAvailableLogoType(NetworkID,ServiceID);
		}

	case TVTest::MESSAGE_RELAYRECORD:
		{
			LPCWSTR pszFileName=reinterpret_cast<LPCWSTR>(lParam1);

			return GetAppClass().RelayRecord(pszFileName);
		}

	case TVTest::MESSAGE_SILENTMODE:
		{
			if (lParam1==0) {
				return GetAppClass().IsSilent();
			} else if (lParam1==1) {
				GetAppClass().SetSilent(lParam2!=0);
				return TRUE;
			}
		}
		return FALSE;

	case TVTest::MESSAGE_SETWINDOWMESSAGECALLBACK:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			pThis->m_pMessageCallback=reinterpret_cast<TVTest::WindowMessageCallbackFunc>(lParam1);
			pThis->m_pMessageCallbackClientData=reinterpret_cast<void*>(lParam2);
		}
		return TRUE;

	case TVTest::MESSAGE_REGISTERCONTROLLER:
		{
			CPlugin *pThis=ThisFromParam(pParam);
			if (pThis==NULL)
				return FALSE;

			const TVTest::ControllerInfo *pInfo=reinterpret_cast<const TVTest::ControllerInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::ControllerInfo)
					|| pInfo->pszName==NULL || pInfo->pszName[0]=='\0'
					|| pInfo->pszText==NULL || pInfo->pszText[0]=='\0'
					|| pInfo->NumButtons<1
					|| pInfo->pButtonList==NULL)
				return FALSE;
			CControllerPlugin *pController=new CControllerPlugin(pThis,pInfo);
			CControllerManager *pControllerManager=GetAppClass().GetControllerManager();
			if (!pControllerManager->AddController(pController)) {
				delete pController;
				return FALSE;
			}
			pThis->m_ControllerList.push_back(CDynamicString(pInfo->pszName));
			if (pThis->m_fEnabled)
				pControllerManager->LoadControllerSettings(pInfo->pszName);
		}
		return TRUE;

	case TVTest::MESSAGE_ONCONTROLLERBUTTONDOWN:
		return GetAppClass().GetControllerManager()->OnButtonDown(
			reinterpret_cast<LPCWSTR>(lParam1),(int)lParam2);

	case TVTest::MESSAGE_GETCONTROLLERSETTINGS:
		{
			static const DWORD ValidMask=TVTest::CONTROLLER_SETTINGS_MASK_FLAGS;
			LPCWSTR pszName=reinterpret_cast<LPCWSTR>(lParam1);
			TVTest::ControllerSettings *pSettings=reinterpret_cast<TVTest::ControllerSettings*>(lParam2);

			if (pSettings==NULL
					|| (pSettings->Mask|ValidMask)!=ValidMask)
				return FALSE;
			const CControllerManager::ControllerSettings *pControllerSettings=
				GetAppClass().GetControllerManager()->GetControllerSettings(pszName);
			if (pControllerSettings==NULL)
				return FALSE;
			if ((pSettings->Mask&TVTest::CONTROLLER_SETTINGS_MASK_FLAGS)!=0) {
				pSettings->Flags=0;
				if (pControllerSettings->fActiveOnly)
					pSettings->Flags|=TVTest::CONTROLLER_SETTINGS_FLAG_ACTIVEONLY;
			}
		}
		return TRUE;
	}
	return 0;
}


bool CALLBACK CPlugin::GrabMediaCallback(const CMediaData *pMediaData, const PVOID pParam)
{
	CBlockLock Lock(&m_GrabberLock);
	BYTE *pData=const_cast<BYTE*>(pMediaData->GetData());

	for (int i=0;i<m_GrabberList.Length();i++) {
		CMediaGrabberInfo *pInfo=m_GrabberList[i];

		if (!pInfo->m_CallbackInfo.Callback(pData,pInfo->m_CallbackInfo.pClientData))
			return false;
	}
	return true;
}


void CALLBACK CPlugin::AudioStreamCallback(short *pData,DWORD Samples,int Channels,void *pParam)
{
	CBlockLock Lock(&m_AudioStreamLock);

	for (int i=0;i<m_AudioStreamCallbackList.Length();i++) {
		CAudioStreamCallbackInfo *pInfo=m_AudioStreamCallbackList[i];

		(*pInfo->m_pCallback)(pData,Samples,Channels,pInfo->m_pClientData);
	}
}




CPlugin::CPluginCommandInfo::CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName)
	: m_ID(ID)
	, m_pszText(DuplicateString(pszText))
	, m_pszName(DuplicateString(pszName))
{
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const TVTest::CommandInfo &Info)
	: m_ID(Info.ID)
	, m_pszText(DuplicateString(Info.pszText))
	, m_pszName(DuplicateString(Info.pszName))
{
}


CPlugin::CPluginCommandInfo::~CPluginCommandInfo()
{
	delete [] m_pszText;
	delete [] m_pszName;
}


CPlugin::CPluginCommandInfo &CPlugin::CPluginCommandInfo::operator=(const CPluginCommandInfo &Info)
{
	if (&Info!=this) {
		m_ID=Info.m_ID;
		ReplaceString(&m_pszText,Info.m_pszText);
		ReplaceString(&m_pszName,Info.m_pszName);
	}
	return *this;
}




CPluginList::CPluginList()
{
}


CPluginList::~CPluginList()
{
	FreePlugins();
}


void CPluginList::SortPluginsByName()
{
	m_PluginList.Sort(CompareName);
}


int CPluginList::CompareName(const CPlugin *pPlugin1,const CPlugin *pPlugin2,void *pParam)
{
	int Cmp;

	Cmp=::lstrcmpi(pPlugin1->GetPluginName(),pPlugin2->GetPluginName());
	if (Cmp!=0)
		return Cmp;
	return ::lstrcmpi(pPlugin1->GetFileName(),pPlugin2->GetFileName());
}


bool CPluginList::LoadPlugins(LPCTSTR pszDirectory,const std::vector<LPCTSTR> *pExcludePlugins)
{
	TCHAR szFileName[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	FreePlugins();
	::PathCombine(szFileName,pszDirectory,TEXT("*.tvtp"));
	hFind=::FindFirstFile(szFileName,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if (pExcludePlugins!=NULL) {
				bool fExclude=false;
				for (size_t i=0;i<pExcludePlugins->size();i++) {
					if (::lstrcmpi((*pExcludePlugins)[i],wfd.cFileName)==0) {
						fExclude=true;
						break;
					}
				}
				if (fExclude) {
					GetAppClass().AddLog(TEXT("%s は除外指定されているため読み込まれません。"),wfd.cFileName);
					continue;
				}
			}

			CPlugin *pPlugin=new CPlugin;

			::PathCombine(szFileName,pszDirectory,wfd.cFileName);
			if (pPlugin->Load(szFileName)) {
				GetAppClass().AddLog(TEXT("%s を読み込みました。"),wfd.cFileName);
				m_PluginList.Add(pPlugin);
			} else {
				if (pPlugin->GetLastErrorText()!=NULL) {
					GetAppClass().AddLog(TEXT("%s : %s"),
										 wfd.cFileName,pPlugin->GetLastErrorText());
				}
				delete pPlugin;
			}
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}
	SortPluginsByName();
	for (int i=0;i<m_PluginList.Length();i++)
		m_PluginList[i]->SetCommand(CM_PLUGIN_FIRST+i);
	return true;
}


void CPluginList::FreePlugins()
{
	m_PluginList.DeleteAll();
	m_PluginList.Clear();
}


CPlugin *CPluginList::GetPlugin(int Index)
{
	return m_PluginList.Get(Index);
}


const CPlugin *CPluginList::GetPlugin(int Index) const
{
	return m_PluginList.Get(Index);
}


bool CPluginList::EnablePlugins(bool fEnable)
{
	for (int i=0;i<NumPlugins();i++) {
		m_PluginList[i]->Enable(fEnable);
	}
	return true;
}


int CPluginList::FindPlugin(const CPlugin *pPlugin) const
{
	for (int i=0;i<NumPlugins();i++) {
		if (m_PluginList[i]==pPlugin)
			return i;
	}
	return -1;
}


int CPluginList::FindPluginByCommand(int Command) const
{
	for (int i=0;i<NumPlugins();i++) {
		if (m_PluginList[i]->GetCommand()==Command)
			return i;
	}
	return -1;
}


bool CPluginList::DeletePlugin(int Index)
{
	if (Index<0 || Index>=NumPlugins())
		return false;
	return m_PluginList.Delete(Index);
}


bool CPluginList::SetMenu(HMENU hmenu) const
{
	ClearMenu(hmenu);
	if (NumPlugins()>0) {
		for (int i=0;i<NumPlugins();i++) {
			const CPlugin *pPlugin=m_PluginList[i];

			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED |
						 (pPlugin->IsEnabled()?MFS_CHECKED:MFS_UNCHECKED),
						 pPlugin->GetCommand(),pPlugin->GetPluginName());
		}
	} else {
		::AppendMenu(hmenu,MFT_STRING | MFS_GRAYED,0,TEXT("なし"));
	}
	return true;
}


bool CPluginList::OnPluginCommand(LPCTSTR pszCommand)
{
	if (pszCommand==NULL)
		return false;

	TCHAR szFileName[MAX_PATH];
	int Length;

	for (Length=0;pszCommand[Length]!=':';Length++)
		szFileName[Length]=pszCommand[Length];
	szFileName[Length]='\0';
	for (int i=0;i<NumPlugins();i++) {
		CPlugin *pPlugin=GetPlugin(i);

		if (::lstrcmpi(::PathFindFileName(pPlugin->GetFileName()),szFileName)==0)
			return pPlugin->NotifyCommand(&pszCommand[Length+1]);
	}
	return false;
}


bool CPluginList::SendEvent(UINT Event,LPARAM lParam1,LPARAM lParam2)
{
	const int Plugins=NumPlugins();

	for (int i=0;i<Plugins;i++) {
		m_PluginList[i]->SendEvent(Event,lParam1,lParam2);
	}
	return true;
}


bool CPluginList::SendChannelChangeEvent()
{
	return SendEvent(TVTest::EVENT_CHANNELCHANGE);
}


bool CPluginList::SendServiceChangeEvent()
{
	return SendEvent(TVTest::EVENT_SERVICECHANGE);
}


bool CPluginList::SendDriverChangeEvent()
{
	return SendEvent(TVTest::EVENT_DRIVERCHANGE);
}


bool CPluginList::SendServiceUpdateEvent()
{
	return SendEvent(TVTest::EVENT_SERVICEUPDATE);
}


bool CPluginList::SendRecordStatusChangeEvent()
{
	const CRecordManager *pRecordManager=GetAppClass().GetRecordManager();
	int Status;

	if (pRecordManager->IsRecording()) {
		if (pRecordManager->IsPaused())
			Status=TVTest::RECORD_STATUS_PAUSED;
		else
			Status=TVTest::RECORD_STATUS_RECORDING;
	} else {
		Status=TVTest::RECORD_STATUS_NOTRECORDING;
	}
	return SendEvent(TVTest::EVENT_RECORDSTATUSCHANGE,Status);
}


bool CPluginList::SendFullscreenChangeEvent(bool fFullscreen)
{
	return SendEvent(TVTest::EVENT_FULLSCREENCHANGE,fFullscreen);
}


bool CPluginList::SendPreviewChangeEvent(bool fPreview)
{
	return SendEvent(TVTest::EVENT_PREVIEWCHANGE,fPreview);
}


bool CPluginList::SendVolumeChangeEvent(int Volume,bool fMute)
{
	return SendEvent(TVTest::EVENT_VOLUMECHANGE,Volume,fMute);
}


bool CPluginList::SendStereoModeChangeEvent(int StereoMode)
{
	return SendEvent(TVTest::EVENT_STEREOMODECHANGE,StereoMode);
}


bool CPluginList::SendColorChangeEvent()
{
	return SendEvent(TVTest::EVENT_COLORCHANGE);
}


bool CPluginList::SendStandbyEvent(bool fStandby)
{
	return SendEvent(TVTest::EVENT_STANDBY,fStandby);
}


bool CPluginList::SendExecuteEvent(LPCTSTR pszCommandLine)
{
	return SendEvent(TVTest::EVENT_EXECUTE,reinterpret_cast<LPARAM>(pszCommandLine));
}


bool CPluginList::SendResetEvent()
{
	return SendEvent(TVTest::EVENT_RESET);
}


bool CPluginList::SendStatusResetEvent()
{
	return SendEvent(TVTest::EVENT_STATUSRESET);
}


bool CPluginList::SendAudioStreamChangeEvent(int Stream)
{
	return SendEvent(TVTest::EVENT_AUDIOSTREAMCHANGE,Stream);
}


bool CPluginList::SendSettingsChangeEvent()
{
	return SendEvent(TVTest::EVENT_SETTINGSCHANGE);
}


bool CPluginList::SendCloseEvent()
{
	return SendEvent(TVTest::EVENT_CLOSE);
}


bool CPluginList::SendStartRecordEvent(const CRecordManager *pRecordManager,LPTSTR pszFileName,int MaxFileName)
{
	TVTest::StartRecordInfo Info;

	Info.Size=sizeof(TVTest::StartRecordInfo);
	Info.Flags=0;
	Info.Modified=0;
	Info.Client=(DWORD)pRecordManager->GetClient();
	Info.pszFileName=pszFileName;
	Info.MaxFileName=MaxFileName;
	CRecordManager::TimeSpecInfo TimeSpec;
	pRecordManager->GetStartTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TIME_NOTSPECIFIED:
		Info.StartTimeSpec=TVTest::RECORD_START_NOTSPECIFIED;
		break;
	case CRecordManager::TIME_DATETIME:
		Info.StartTimeSpec=TVTest::RECORD_START_TIME;
		break;
	case CRecordManager::TIME_DURATION:
		Info.StartTimeSpec=TVTest::RECORD_START_DELAY;
		break;
	}
	if (TimeSpec.Type!=CRecordManager::TIME_NOTSPECIFIED)
		pRecordManager->GetReservedStartTime(&Info.StartTime);
	pRecordManager->GetStopTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TIME_NOTSPECIFIED:
		Info.StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
		break;
	case CRecordManager::TIME_DATETIME:
		Info.StopTimeSpec=TVTest::RECORD_STOP_TIME;
		Info.StopTime.Time=TimeSpec.Time.DateTime;
		break;
	case CRecordManager::TIME_DURATION:
		Info.StopTimeSpec=TVTest::RECORD_STOP_DURATION;
		Info.StopTime.Duration=TimeSpec.Time.Duration;
		break;
	}
	return SendEvent(TVTest::EVENT_STARTRECORD,reinterpret_cast<LPARAM>(&Info));
}


bool CPluginList::SendRelayRecordEvent(LPCTSTR pszFileName)
{
	return SendEvent(TVTest::EVENT_RELAYRECORD,reinterpret_cast<LPARAM>(pszFileName));
}


bool CPluginList::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult)
{
	const int Plugins=NumPlugins();
	bool fNoDefault=false;

	for (int i=0;i<Plugins;i++) {
		if (m_PluginList[i]->OnMessage(hwnd,uMsg,wParam,lParam,pResult))
			fNoDefault=true;
	}
	return fNoDefault;
}




CPluginOptions::CPluginOptions(CPluginList *pPluginList)
	: m_pPluginList(pPluginList)
{
}


CPluginOptions::~CPluginOptions()
{
	ClearList();
}


bool CPluginOptions::Read(CSettings *pSettings)
{
	/*
	unsigned int Timeout;
	if (pSettings->Read(TEXT("PluginFinalizeTimeout"),&Timeout))
		CPlugin::SetFinalizeTimeout(Timeout);
	*/
	return true;
}


bool CPluginOptions::Write(CSettings *pSettings) const
{
	/*
	pSettings->Write(TEXT("PluginFinalizeTimeout"),
					 (unsigned int)CPlugin::GetFinalizeTimeout());
	*/
	return true;
}


bool CPluginOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("PluginList"),CSettings::OPEN_READ)) {
		int Count;

		if (Settings.Read(TEXT("PluginCount"),&Count) && Count>0) {
			for (int i=0;i<Count;i++) {
				TCHAR szName[32],szFileName[MAX_PATH];

				::wsprintf(szName,TEXT("Plugin%d_Name"),i);
				if (!Settings.Read(szName,szFileName,lengthof(szFileName)))
					break;
				if (szFileName[0]!='\0') {
					bool fEnable;

					::wsprintf(szName,TEXT("Plugin%d_Enable"),i);
					if (Settings.Read(szName,&fEnable) && fEnable) {
						m_EnablePluginList.push_back(DuplicateString(szFileName));
					}
				}
			}
		}
	}
	return true;
}


bool CPluginOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("PluginList"),CSettings::OPEN_WRITE)) {
		Settings.Clear();
		Settings.Write(TEXT("PluginCount"),(unsigned int)m_EnablePluginList.size());
		for (size_t i=0;i<m_EnablePluginList.size();i++) {
			TCHAR szName[32];

			::wsprintf(szName,TEXT("Plugin%d_Name"),i);
			Settings.Write(szName,m_EnablePluginList[i]);
			::wsprintf(szName,TEXT("Plugin%d_Enable"),i);
			Settings.Write(szName,true);
		}
	}
	return true;
}


bool CPluginOptions::RestorePluginOptions()
{
	for (size_t i=0;i<m_EnablePluginList.size();i++) {
		for (int j=0;j<m_pPluginList->NumPlugins();j++) {
			CPlugin *pPlugin=m_pPluginList->GetPlugin(j);

			if (!pPlugin->IsDisableOnStart()
					&& ::lstrcmpi(m_EnablePluginList[i],::PathFindFileName(pPlugin->GetFileName()))==0)
				pPlugin->Enable(true);
		}
	}
	return true;
}


bool CPluginOptions::StorePluginOptions()
{
	ClearList();
	for (int i=0;i<m_pPluginList->NumPlugins();i++) {
		const CPlugin *pPlugin=m_pPluginList->GetPlugin(i);

		if (pPlugin->IsEnabled()) {
			m_EnablePluginList.push_back(DuplicateString(::PathFindFileName(pPlugin->GetFileName())));
		}
	}
	return true;
}


void CPluginOptions::ClearList()
{
	for (size_t i=0;i<m_EnablePluginList.size();i++) {
		delete [] m_EnablePluginList[i];
	}
	m_EnablePluginList.clear();
}


CPluginOptions *CPluginOptions::GetThis(HWND hDlg)
{
	return static_cast<CPluginOptions*>(GetOptions(hDlg));
}


INT_PTR CALLBACK CPluginOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CPluginOptions *pThis=static_cast<CPluginOptions*>(OnInitDialog(hDlg,lParam));
			HWND hwndList=GetDlgItem(hDlg,IDC_PLUGIN_LIST);
			LV_COLUMN lvc;
			int i;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=120;
			lvc.pszText=TEXT("ファイル名");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("プラグイン名");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.pszText=TEXT("説明");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.pszText=TEXT("著作権");
			ListView_InsertColumn(hwndList,3,&lvc);
			for (i=0;i<pThis->m_pPluginList->NumPlugins();i++) {
				const CPlugin *pPlugin=pThis->m_pPluginList->GetPlugin(i);
				LV_ITEM lvi;

				lvi.mask=LVIF_TEXT | LVIF_PARAM;
				lvi.iItem=i;
				lvi.iSubItem=0;
				lvi.pszText=::PathFindFileName(pPlugin->GetFileName());
				lvi.lParam=reinterpret_cast<LPARAM>(pPlugin);
				ListView_InsertItem(hwndList,&lvi);
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetPluginName());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=2;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetDescription());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=3;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetCopyright());
				ListView_SetItem(hwndList,&lvi);
			}
			for (i=0;i<4;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PLUGIN_SETTINGS:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_PLUGIN_LIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case IDC_PLUGIN_UNLOAD:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_PLUGIN_LIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi)) {
						CPluginOptions *pThis=GetThis(hDlg);
						int Index=pThis->m_pPluginList->FindPlugin(reinterpret_cast<CPlugin*>(lvi.lParam));

						if (pThis->m_pPluginList->DeletePlugin(Index))
							ListView_DeleteItem(hwndList,Sel);
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);
				int Sel=ListView_GetNextItem(pnmlv->hdr.hwndFrom,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi)) {
						CPlugin *pPlugin=reinterpret_cast<CPlugin*>(lvi.lParam);
						EnableDlgItem(hDlg,IDC_PLUGIN_SETTINGS,pPlugin->HasSettings());
					}
				} else {
					EnableDlgItem(hDlg,IDC_PLUGIN_SETTINGS,false);
				}
			}
			return TRUE;

		case NM_DBLCLK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=pnmia->iItem;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmia->hdr.hwndFrom,&lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				int Sel=ListView_GetNextItem(pnmh->hwndFrom,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmh->hwndFrom,&lvi)) {
						HMENU hmenu=
							::LoadMenu(GetAppClass().GetResourceInstance(),
										MAKEINTRESOURCE(IDM_PLUGIN));
						CPlugin *pPlugin=reinterpret_cast<CPlugin*>(lvi.lParam);
						POINT pt;

						::EnableMenuItem(hmenu,IDC_PLUGIN_SETTINGS,
								pPlugin->HasSettings()?MFS_ENABLED:MFS_GRAYED);
						::EnableMenuItem(hmenu,IDC_PLUGIN_UNLOAD,
								pPlugin->CanUnload()?MFS_ENABLED:MFS_GRAYED);
						::GetCursorPos(&pt);
						::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,
										 pt.x,pt.y,0,hDlg,NULL);
						::DestroyMenu(hmenu);
					}
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		GetThis(hDlg)->OnDestroy();
		return TRUE;
	}
	return FALSE;
}

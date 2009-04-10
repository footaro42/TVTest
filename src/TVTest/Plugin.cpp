#include "stdafx.h"
#include <commctrl.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Plugin.h"
#include "Image.h"
#include "Version.h"
#include "TsEncode.h"
#include "resource.h"




//DWORD CPlugin::m_FinalizeTimeout=10000;

bool CPlugin::m_fSetGrabber=false;
CPointerVector<CPlugin::CMediaGrabberInfo> CPlugin::m_GrabberList;
CCriticalLock CPlugin::m_GrabberLock;


CPlugin::CPlugin()
	: m_hLib(NULL)
	, m_pszFileName(NULL)
	, m_Type(0)
	, m_Flags(0)
	, m_pszPluginName(NULL)
	, m_pszCopyright(NULL)
	, m_pszDescription(NULL)
	, m_fEnabled(false)
	, m_Command(0)
	, m_pEventCallback(NULL)
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
	if (hLib==NULL)
		return false;
	TVTest::GetVersionFunc pGetVersion=
		reinterpret_cast<TVTest::GetVersionFunc>(::GetProcAddress(hLib,"TVTGetVersion"));
	if (pGetVersion==NULL) {
		::FreeLibrary(hLib);
		return false;
	}
	DWORD Version=pGetVersion();
	if (TVTest::GetMajorVersion(Version)!=TVTest::GetMajorVersion(TVTEST_PLUGIN_VERSION)
		|| TVTest::GetMinorVersion(Version)!=TVTest::GetMinorVersion(TVTEST_PLUGIN_VERSION)) {
		::FreeLibrary(hLib);
		return false;
	}
	TVTest::GetPluginInfoFunc pGetPluginInfo=
		reinterpret_cast<TVTest::GetPluginInfoFunc>(::GetProcAddress(hLib,"TVTGetPluginInfo"));
	if (pGetPluginInfo==NULL) {
		::FreeLibrary(hLib);
		return false;
	}
	TVTest::PluginInfo PluginInfo;
	::ZeroMemory(&PluginInfo,sizeof(PluginInfo));
	if (!pGetPluginInfo(&PluginInfo)) {
		::FreeLibrary(hLib);
		return false;
	}
	TVTest::InitializeFunc pInitialize=
		reinterpret_cast<TVTest::InitializeFunc>(::GetProcAddress(hLib,"TVTInitialize"));
	if (pInitialize==NULL) {
		::FreeLibrary(hLib);
		return false;
	}
	m_PluginParam.Callback=Callback;
	m_PluginParam.hwndApp=GetAppClass().GetMainWindow()->GetHandle();
	m_PluginParam.pClientData=NULL;
	m_PluginParam.pInternalData=this;
	if (!pInitialize(&m_PluginParam)) {
		::FreeLibrary(hLib);
		return false;
	}
	m_hLib=hLib;
	m_pszFileName=DuplicateString(pszFileName);
	m_Type=PluginInfo.Type;
	m_Flags=PluginInfo.Flags;
	m_pszPluginName=DuplicateString(PluginInfo.pszPluginName);
	m_pszCopyright=DuplicateString(PluginInfo.pszCopyright);
	m_pszDescription=DuplicateString(PluginInfo.pszDescription);
	m_fEnabled=(m_Flags&TVTest::PLUGIN_FLAG_ENABLEDEFAULT)!=0;
	return true;
}


void CPlugin::Free()
{
	if (m_hLib!=NULL) {
		m_GrabberLock.Lock();
		if (m_fSetGrabber) {
			for (int i=m_GrabberList.Length()-1;i>=0;i--) {
				if (m_GrabberList[i]->m_pPlugin==this)
					m_GrabberList.Delete(i);
			}
			if (m_GrabberList.Length()==0) {
				CMediaGrabber &MediaGrabber=GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaGrabber;

				MediaGrabber.SetMediaGrabCallback(NULL);
				m_fSetGrabber=false;
			}
		}
		m_GrabberLock.Unlock();

		m_pEventCallback=NULL;

		TVTest::FinalizeFunc pFinalize=
			static_cast<TVTest::FinalizeFunc>(::GetProcAddress(m_hLib,"TVTFinalize"));
		if (pFinalize!=NULL) {
			/*
			HANDLE hThread=::CreateThread(NULL,0,FinalizeThread,pFinalize,0,NULL);

			if (hThread==NULL) {
				pFinalize();
			} else {
				if (::WaitForSingleObject(hThread,m_FinalizeTimeout)==WAIT_TIMEOUT) {
					GetAppClass().AddLog(TEXT("プラグイン \"%s\" の終了処理がタイムアウトしました。"),::PathFindFileName(m_pszFileName));
					::TerminateThread(hThread,-1);
				}
				::CloseHandle(hThread);
			}
			*/
			pFinalize();
		}
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
	}
	SAFE_DELETE(m_pszFileName);
	SAFE_DELETE(m_pszPluginName);
	SAFE_DELETE(m_pszCopyright);
	SAFE_DELETE(m_pszDescription);
	m_fEnabled=false;
	m_CommandList.DeleteAll();
}


DWORD WINAPI CPlugin::FinalizeThread(LPVOID lpParameter)
{
	TVTest::FinalizeFunc pFinalize=static_cast<TVTest::FinalizeFunc>(lpParameter);

	pFinalize();
	return 0;
}


bool CPlugin::Enable(bool fEnable)
{
	if (m_fEnabled!=fEnable) {
		if (!SendEvent(TVTest::EVENT_PLUGINENABLE,(LPARAM)fEnable))
			return false;
		m_fEnabled=fEnable;
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


bool CPlugin::Settings(HWND hwndOwner)
{
	if ((m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)==0)
		return false;
	return SendEvent(TVTest::EVENT_PLUGINSETTINGS,(LPARAM)hwndOwner)!=0;
}


LRESULT CALLBACK CPlugin::Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2)
{
	switch (Message) {
	case TVTest::MESSAGE_GETVERSION:
		return TVTest::MakeVersion(VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD);

	case TVTest::MESSAGE_QUERYMESSAGE:
		switch (lParam1) {
		case TVTest::MESSAGE_GETVERSION:
		case TVTest::MESSAGE_QUERYMESSAGE:
		case TVTest::MESSAGE_MEMORYALLOC:
		case TVTest::MESSAGE_SETEVENTCALLBACK:
		case TVTest::MESSAGE_GETCURRENTCHANNELINFO:
		case TVTest::MESSAGE_SETCHANNEL:
		case TVTest::MESSAGE_GETSERVICE:
		case TVTest::MESSAGE_SETSERVICE:
		case TVTest::MESSAGE_GETTUNINGSPACENAME:
		case TVTest::MESSAGE_GETCHANNELINFO:
		case TVTest::MESSAGE_GETSERVICEINFO:
		case TVTest::MESSAGE_GETDRIVERNAME:
		case TVTest::MESSAGE_SETDRIVERNAME:
		case TVTest::MESSAGE_STARTRECORD:
		case TVTest::MESSAGE_STOPRECORD:
		case TVTest::MESSAGE_PAUSERECORD:
		case TVTest::MESSAGE_GETRECORD:
		case TVTest::MESSAGE_MODIFYRECORD:
		case TVTest::MESSAGE_GETZOOM:
		case TVTest::MESSAGE_SETZOOM:
		case TVTest::MESSAGE_GETPANSCAN:
		case TVTest::MESSAGE_SETPANSCAN:
		case TVTest::MESSAGE_GETSTATUS:
		case TVTest::MESSAGE_GETRECORDSTATUS:
		case TVTest::MESSAGE_GETVIDEOINFO:
		case TVTest::MESSAGE_GETVOLUME:
		case TVTest::MESSAGE_SETVOLUME:
		case TVTest::MESSAGE_GETSTEREOMODE:
		case TVTest::MESSAGE_SETSTEREOMODE:
		case TVTest::MESSAGE_GETFULLSCREEN:
		case TVTest::MESSAGE_SETFULLSCREEN:
		case TVTest::MESSAGE_GETPREVIEW:
		case TVTest::MESSAGE_SETPREVIEW:
		case TVTest::MESSAGE_GETSTANDBY:
		case TVTest::MESSAGE_SETSTANDBY:
		case TVTest::MESSAGE_GETALWAYSONTOP:
		case TVTest::MESSAGE_SETALWAYSONTOP:
		case TVTest::MESSAGE_CAPTUREIMAGE:
		case TVTest::MESSAGE_SAVEIMAGE:
		case TVTest::MESSAGE_RESET:
		case TVTest::MESSAGE_CLOSE:
		case TVTest::MESSAGE_SETSTREAMCALLBACK:
		case TVTest::MESSAGE_ENABLEPLUGIN:
		case TVTest::MESSAGE_GETCOLOR:
		case TVTest::MESSAGE_DECODEARIBSTRING:
		case TVTest::MESSAGE_GETCURRENTPROGRAMINFO:
		case TVTest::MESSAGE_QUERYEVENT:
		case TVTest::MESSAGE_GETTUNINGSPACE:
		case TVTest::MESSAGE_GETTUNINGSPACEINFO:
		case TVTest::MESSAGE_SETNEXTCHANNEL:
		case TVTest::MESSAGE_GETAUDIOSTREAM:
		case TVTest::MESSAGE_SETAUDIOSTREAM:
		case TVTest::MESSAGE_ISPLUGINENABLED:
		case TVTest::MESSAGE_REGISTERCOMMAND:
		case TVTest::MESSAGE_ADDLOG:
			return TRUE;
		}
		return FALSE;

	case TVTest::MESSAGE_MEMORYALLOC:
		{
			void *pData=reinterpret_cast<void*>(lParam1);
			DWORD Size=lParam2;

			if (Size>0) {
				return (LRESULT)realloc(pData,Size);
			} else if (pData!=NULL) {
				free(pData);
			}
		}
		return 0;

	case TVTest::MESSAGE_SETEVENTCALLBACK:
		{
			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);

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
			CProgManager *pProgManager=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_ProgManager;
			pChannelInfo->Space=pChInfo->GetSpace();
			pChannelInfo->Channel=pChInfo->GetChannelIndex();
			pChannelInfo->RemoteControlKeyID=pChInfo->GetChannelNo();
			pChannelInfo->NetworkID=pProgManager->GetNetworkID();
			if (!pProgManager->GetNetworkName(pChannelInfo->szNetworkName,
										lengthof(pChannelInfo->szNetworkName)))
				pChannelInfo->szNetworkName[0]='\0';
			pChannelInfo->TransportStreamID=pProgManager->GetTransportStreamID();
			if (!pProgManager->GetTSName(pChannelInfo->szTransportStreamName,
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
			CAppMain &AppMain=GetAppClass();

			AppMain.GetMainWindow()->OpenTuner();
			return AppMain.SetChannel((int)lParam1,(int)lParam2);
		}

	case TVTest::MESSAGE_GETSERVICE:
		{
			CDtvEngine *pDtvEngine=&GetAppClass().GetCoreEngine()->m_DtvEngine;
			WORD Service=pDtvEngine->GetService();
			int *pNumServices=reinterpret_cast<int*>(lParam1);

			if (pNumServices)
				*pNumServices=pDtvEngine->m_ProgManager.GetServiceNum();
			if (Service==0xFFFF)
				return -1;
			return Service;
		}

	case TVTest::MESSAGE_SETSERVICE:
		{
			if (lParam2==0)
				return GetAppClass().SetServiceByIndex(lParam1);
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
			int Index=lParam1;
			TVTest::ServiceInfo *pServiceInfo=reinterpret_cast<TVTest::ServiceInfo*>(lParam2);
			CProgManager *pProgManager;

			if (Index<0 || pServiceInfo==NULL
					|| (pServiceInfo->Size!=sizeof(TVTest::ServiceInfo)
						&& pServiceInfo->Size!=TVTest::SERVICEINFO_SIZE_V1))
				return FALSE;
			pProgManager=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_ProgManager;
			if (!pProgManager->GetServiceID(&pServiceInfo->ServiceID,Index)
					|| !pProgManager->GetServiceName(
											pServiceInfo->szServiceName,Index)
					|| !pProgManager->GetVideoEsPID(&pServiceInfo->VideoPID,Index))
				return FALSE;
			pServiceInfo->NumAudioPIDs=pProgManager->GetAudioEsNum(Index);
			for (int i=0;i<pServiceInfo->NumAudioPIDs;i++) {
				pProgManager->GetAudioEsPID(&pServiceInfo->AudioPID[i],i,Index);
			}
			if (pServiceInfo->Size==sizeof(TVTest::ServiceInfo)) {
				for (int i=0;i<pServiceInfo->NumAudioPIDs;i++) {
					pServiceInfo->AudioComponentType[i]=
						pProgManager->GetAudioComponentType(i,Index);
				}
				if (!pProgManager->GetSubtitleEsPID(&pServiceInfo->SubtitlePID,Index))
					pServiceInfo->SubtitlePID=0;
				pServiceInfo->Reserved=0;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERNAME:
		{
			LPWSTR pszName=reinterpret_cast<LPWSTR>(lParam1);
			int MaxLength=lParam2;
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
				return App.StartRecord();
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
				&StartTime,&StopTime);
		}

	case TVTest::MESSAGE_STOPRECORD:
		{
			CAppMain &App=GetAppClass();

			if (!App.GetRecordManager()->IsRecording())
				return FALSE;
			App.GetMainWindow()->SendCommand(CM_RECORD_STOP);
		}
		return TRUE;

	case TVTest::MESSAGE_PAUSERECORD:
		{
			CAppMain &App=GetAppClass();
			const CRecordManager *pRecordManager=App.GetRecordManager();
			bool fPause=lParam1!=0;

			if (!pRecordManager->IsRecording())
				return FALSE;
			if (fPause==pRecordManager->IsPaused())
				return FALSE;
			App.GetMainWindow()->SendCommand(CM_RECORD_PAUSE);
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
				(pInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0?&StopTime:NULL);

		}

	case TVTest::MESSAGE_GETZOOM:
		return GetAppClass().GetMainWindow()->CalcZoomRate();

	case TVTest::MESSAGE_SETZOOM:
		return GetAppClass().GetMainWindow()->SetZoomRate((int)lParam1,(int)lParam2);

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
			GetAppClass().GetMainWindow()->PostCommand(Command);
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
					&& pCoreEngine->GetCardReaderType()!=CCardReader::READER_NONE) {
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

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::RecordStatusInfo))
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
			const CMainWindow *pMainWindow=GetAppClass().GetMainWindow();
			int Volume=pMainWindow->GetVolume();
			bool fMute=pMainWindow->GetMute();

			return MAKELRESULT(Volume,fMute);
		}

	case TVTest::MESSAGE_SETVOLUME:
		{
			CMainWindow *pMainWindow=GetAppClass().GetMainWindow();
			int Volume=lParam1;

			if (Volume<0)
				return pMainWindow->SetMute(lParam2!=0);
			return pMainWindow->SetVolume(Volume,true);
		}

	case TVTest::MESSAGE_GETSTEREOMODE:
		return GetAppClass().GetMainWindow()->GetStereoMode();

	case TVTest::MESSAGE_SETSTEREOMODE:
		return GetAppClass().GetMainWindow()->SetStereoMode((int)lParam1);

	case TVTest::MESSAGE_GETFULLSCREEN:
		return GetAppClass().GetMainWindow()->GetFullscreen();

	case TVTest::MESSAGE_SETFULLSCREEN:
		return GetAppClass().GetMainWindow()->SetFullscreen(lParam1!=0);

	case TVTest::MESSAGE_GETPREVIEW:
		return GetAppClass().GetMainWindow()->IsPreview();

	case TVTest::MESSAGE_SETPREVIEW:
		return GetAppClass().GetMainWindow()->EnablePreview(lParam1!=0);

	case TVTest::MESSAGE_GETSTANDBY:
		return GetAppClass().GetMainWindow()->GetStandby();

	case TVTest::MESSAGE_SETSTANDBY:
		return GetAppClass().GetMainWindow()->SetStandby(lParam1!=0);

	case TVTest::MESSAGE_GETALWAYSONTOP:
		return GetAppClass().GetMainWindow()->GetAlwaysOnTop();

	case TVTest::MESSAGE_SETALWAYSONTOP:
		GetAppClass().GetMainWindow()->SetAlwaysOnTop(lParam1!=0);
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
		GetAppClass().GetMainWindow()->PostCommand(CM_SAVEIMAGE);
		return TRUE;

	case TVTest::MESSAGE_RESET:
		GetAppClass().GetMainWindow()->PostCommand(CM_RESET);
		return TRUE;

	case TVTest::MESSAGE_CLOSE:
		GetAppClass().GetMainWindow()->PostCommand(
							(lParam1&TVTest::CLOSE_EXIT)!=0?CM_EXIT:CM_CLOSE);
		return TRUE;

	case TVTest::MESSAGE_SETSTREAMCALLBACK:
		{
			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);
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
			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);

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

			const CEpgDataInfo *pEpgInfo=GetAppClass().GetCoreEngine()->GetEpgDataInfo(lParam2!=0);

			if (pEpgInfo==NULL)
				return FALSE;
			pProgramInfo->ServiceID=pEpgInfo->GetServiceID();
			pProgramInfo->EventID=pEpgInfo->GetEventID();
			if (pProgramInfo->pszEventName!=NULL && pProgramInfo->MaxEventName>0) {
				if (pEpgInfo->GetEventName()!=NULL)
					::lstrcpyn(pProgramInfo->pszEventName,pEpgInfo->GetEventName(),
							   pProgramInfo->MaxEventName);
				else
					pProgramInfo->pszEventName[0]='\0';
			}
			if (pProgramInfo->pszEventText!=NULL && pProgramInfo->MaxEventText>0) {
				if (pEpgInfo->GetEventText()!=NULL)
					::lstrcpyn(pProgramInfo->pszEventText,pEpgInfo->GetEventText(),
							   pProgramInfo->MaxEventText);
				else
					pProgramInfo->pszEventText[0]='\0';
			}
			if (pProgramInfo->pszEventExtText!=NULL && pProgramInfo->MaxEventExtText>0) {
				if (pEpgInfo->GetEventExtText()!=NULL)
					::lstrcpyn(pProgramInfo->pszEventExtText,pEpgInfo->GetEventExtText(),
							   pProgramInfo->MaxEventExtText);
				else
					pProgramInfo->pszEventExtText[0]='\0';
			}
			pProgramInfo->StartTime=pEpgInfo->GetStartTime();
			pProgramInfo->Duration=pEpgInfo->GetDuration();
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
			int Index=lParam1;
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

			GetAppClass().GetMainWindow()->PostCommand(fNext?CM_CHANNEL_UP:CM_CHANNEL_DOWN);
		}
		return TRUE;

	case TVTest::MESSAGE_GETAUDIOSTREAM:
		return GetAppClass().GetCoreEngine()->m_DtvEngine.GetAudioStream();

	case TVTest::MESSAGE_SETAUDIOSTREAM:
		{
			int Index=lParam1;

			if (Index<0 || Index>=GetAppClass().GetCoreEngine()->m_DtvEngine.GetAudioStreamNum())
				return FALSE;
			GetAppClass().GetMainWindow()->PostCommand(CM_AUDIOSTREAM_FIRST+Index);
		}
		return TRUE;

	case TVTest::MESSAGE_ISPLUGINENABLED:
		{
			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);

			return pThis->IsEnabled();
		}

	case TVTest::MESSAGE_REGISTERCOMMAND:
		{
			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);
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

			CPlugin *pThis=static_cast<CPlugin*>(pParam->pInternalData);
			LPCTSTR pszFileName=::PathFindFileName(pThis->m_pszFileName);
			LPWSTR pszLog=new TCHAR[::lstrlen(pszFileName)+3+::lstrlen(pszText)+1];
			::wsprintf(pszLog,TEXT("%s : %s"),pszFileName,pszText);
			GetAppClass().AddLog(pszLog);
			delete [] pszLog;
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




CPlugin::CPluginCommandInfo::CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName)
{
	m_ID=ID;
	m_pszText=DuplicateString(pszText);
	m_pszName=DuplicateString(pszName);
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const TVTest::CommandInfo &Info)
{
	m_ID=Info.ID;
	m_pszText=DuplicateString(Info.pszText);
	m_pszName=DuplicateString(Info.pszName);
}


CPlugin::CPluginCommandInfo::~CPluginCommandInfo()
{
	delete [] m_pszText;
	delete [] m_pszName;
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


bool CPluginList::LoadPlugins(LPCTSTR pszDirectory)
{
	TCHAR szFileName[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	FreePlugins();
	::PathCombine(szFileName,pszDirectory,TEXT("*.tvtp"));
	hFind=::FindFirstFile(szFileName,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			CPlugin *pPlugin=new CPlugin;

			::PathCombine(szFileName,pszDirectory,wfd.cFileName);
			if (pPlugin->Load(szFileName)) {
				m_PluginList.Add(pPlugin);
			} else {
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
	for (int i=0;i<NumPlugins();i++) {
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
				if (Settings.Read(szName,szFileName,lengthof(szFileName))
						&& szFileName[0]!='\0') {
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

			if (::lstrcmpi(m_EnablePluginList[i],::PathFindFileName(pPlugin->GetFileName()))==0)
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


BOOL CALLBACK CPluginOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
			lvc.pszText=TEXT("著作権");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.pszText=TEXT("説明");
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
				lvi.pszText=const_cast<LPWSTR>(pPlugin->GetPluginName());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=2;
				lvi.pszText=const_cast<LPWSTR>(pPlugin->GetCopyright());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=3;
				lvi.pszText=const_cast<LPWSTR>(pPlugin->GetDescription());
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

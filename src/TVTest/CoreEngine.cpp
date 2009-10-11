#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "CoreEngine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CCoreEngine::CCoreEngine()
{
	m_fFileMode=false;
	m_szDriverFileName[0]='\0';
	m_hDriverLib=NULL;
	m_DriverType=DRIVER_UNKNOWN;
	m_fDescramble=true;
	m_CardReaderType=CCardReader::READER_SCARD;
	m_fPacketBuffering=false;
	m_PacketBufferLength=m_DtvEngine.m_MediaBuffer.GetBufferLength();
	m_PacketBufferPoolPercentage=m_DtvEngine.m_MediaBuffer.GetPoolPercentage();
	m_OriginalVideoWidth=0;
	m_OriginalVideoHeight=0;
	m_DisplayVideoWidth=0;
	m_DisplayVideoHeight=0;
	m_NumAudioChannels=0;
	m_NumAudioStreams=0;
	m_AudioComponentType=0;
	m_fMute=false;
	m_Volume=50;
	m_VolumeNormalizeLevel=100;
	m_StereoMode=0;
	m_fDownMixSurround=true;
	m_EventID=0;
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
	m_ScramblePacketCount=0;
	m_SignalLevel=0.0;
	m_BitRate=0;
	m_PacketBufferUsedCount=0;
	m_StreamRemain=0;
	m_pEpgDataInfo=NULL;
	m_pEpgDataInfoNext=NULL;
	m_TimerResolution=0;
}


CCoreEngine::~CCoreEngine()
{
	Close();
	delete m_pEpgDataInfo;
	delete m_pEpgDataInfoNext;
	if (m_TimerResolution!=0)
		::timeEndPeriod(m_TimerResolution);
}


void CCoreEngine::Close()
{
	//m_DtvEngine.EnablePreview(false);
	//m_DtvEngine.ReleaseSrcFilter();
	m_DtvEngine.CloseEngine();
	UnloadDriver();
}


bool CCoreEngine::BuildDtvEngine(CDtvEngine::CEventHandler *pEventHandler)
{
	if (!m_DtvEngine.BuildEngine(pEventHandler,
				m_fDescramble?m_CardReaderType!=CCardReader::READER_NONE:false,
				true/*m_fPacketBuffering*/)) {
		return false;
	}
	return true;
}


bool CCoreEngine::SetDriverFileName(LPCTSTR pszFileName)
{
	if (pszFileName==NULL || ::lstrlen(pszFileName)>=MAX_PATH)
		return false;
	::lstrcpy(m_szDriverFileName,pszFileName);
	return true;
}


bool CCoreEngine::LoadDriver()
{
	UnloadDriver();
	m_hDriverLib=::LoadLibrary(m_szDriverFileName);
	if (m_hDriverLib==NULL) {
		int ErrorCode=::GetLastError();
		TCHAR szText[MAX_PATH+64];

		::wsprintf(szText,TEXT("\"%s\" が読み込めません。"),m_szDriverFileName);
		SetError(szText);
		switch (ErrorCode) {
		case ERROR_MOD_NOT_FOUND:
			SetErrorAdvise(TEXT("ファイルが見つかりません。"));
			break;
		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(TEXT("このBonDriverに必要なランタイムがインストールされていない可能性があります。"));
			break;
		default:
			::wsprintf(szText,TEXT("エラーコード: %d"),ErrorCode);
			SetErrorAdvise(szText);
		}
		if (GetErrorText(ErrorCode,szText,lengthof(szText)))
			SetErrorSystemMessage(szText);
		return false;
	}
	return true;
}


bool CCoreEngine::UnloadDriver()
{
	if (m_hDriverLib) {
		CloseDriver();
		::FreeLibrary(m_hDriverLib);
		m_hDriverLib=NULL;
	}
	return true;
}


bool CCoreEngine::OpenDriver()
{
	if (m_hDriverLib==NULL)
		return false;
	if (m_DtvEngine.IsSrcFilterOpen())
		return false;
	m_fFileMode=false;
	if (!m_DtvEngine.OpenSrcFilter_BonDriver(m_hDriverLib)) {
		SetError(m_DtvEngine.GetLastErrorException());
		return false;
	}
	LPCTSTR pszName=m_DtvEngine.m_BonSrcDecoder.GetTunerName();
	m_DriverType=DRIVER_UNKNOWN;
	if (pszName!=NULL) {
		if (::lstrcmpi(pszName,TEXT("HDUS"))==0)
			m_DriverType=DRIVER_HDUS;
		else if (::_tcsncmp(pszName,TEXT("UDP/"),4)==0)
			m_DriverType=DRIVER_UDP;
		else if (::_tcsncmp(pszName,TEXT("TCP"),3)==0)
			m_DriverType=DRIVER_TCP;
	}
	return true;
}


bool CCoreEngine::CloseDriver()
{
	return m_DtvEngine.ReleaseSrcFilter();
}


bool CCoreEngine::IsDriverOpen() const
{
	return m_DtvEngine.IsSrcFilterOpen();
}


/*
bool CCoreEngine::OpenFile(LPCTSTR pszFileName)
{
	m_DtvEngine.ReleaseSrcFilter();
	m_fFileMode=true;
	if (!m_DtvEngine.OpenSrcFilter_File(pszFileName))
		return false;
	return true;
}
*/


bool CCoreEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	if (!m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!m_DtvEngine.BuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
										  pszMpeg2Decoder,pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	} else {
		if (!m_DtvEngine.RebuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
											pszMpeg2Decoder,pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	}
	m_DtvEngine.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	m_DtvEngine.m_MediaViewer.SetAudioNormalize(m_VolumeNormalizeLevel!=100,
										(float)m_VolumeNormalizeLevel/100.0f);
	m_DtvEngine.SetStereoMode(m_StereoMode);
	m_DtvEngine.m_MediaViewer.SetDownMixSurround(m_fDownMixSurround);
	return true;
}


bool CCoreEngine::CloseMediaViewer()
{
	return m_DtvEngine.CloseMediaViewer();
}


bool CCoreEngine::OpenBcasCard()
{
	if (m_fDescramble) {
		if (!m_DtvEngine.OpenBcasCard(m_CardReaderType)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	}
	return true;
}


bool CCoreEngine::CloseBcasCard()
{
	return m_DtvEngine.CloseBcasCard();
}


bool CCoreEngine::IsBcasCardOpen() const
{
	return m_DtvEngine.m_TsDescrambler.IsBcasCardOpen();
}


bool CCoreEngine::IsBuildComplete() const
{
	return m_DtvEngine.IsBuildComplete();
}


bool CCoreEngine::EnablePreview(bool fPreview)
{
	if (!m_DtvEngine.EnablePreview(fPreview))
		return false;
	if (fPreview)
		m_DtvEngine.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	return true;
}


bool CCoreEngine::SetDescramble(bool fDescramble)
{
	if (m_fDescramble!=fDescramble) {
		if (m_DtvEngine.IsEngineBuild()) {
			if (!m_DtvEngine.SetDescramble(fDescramble)) {
				SetError(m_DtvEngine.GetLastErrorException());
				return false;
			}
		}
		m_fDescramble=fDescramble;
	}
	return true;
}


bool CCoreEngine::SetCardReaderType(CCardReader::ReaderType Type)
{
	if (Type<CCardReader::READER_NONE || Type>CCardReader::READER_LAST)
		return false;
	if (m_CardReaderType!=Type) {
		if (m_DtvEngine.IsEngineBuild()) {
			if (!SetDescramble(Type!=CCardReader::READER_NONE))
				return false;
			if (!m_DtvEngine.OpenBcasCard(Type)) {
				SetError(m_DtvEngine.GetLastErrorException());
				return false;
			}
		}
		m_CardReaderType=Type;
	}
	return true;
}


bool CCoreEngine::IsNetworkDriverFileName(LPCTSTR pszFileName)
{
	LPCTSTR pszName=::PathFindFileName(pszFileName);

	if (::lstrcmpi(pszName,TEXT("BonDriver_UDP.dll"))==0
			|| ::lstrcmpi(pszName,TEXT("BonDriver_TCP.dll"))==0)
		return true;
	return false;
}


bool CCoreEngine::SetPacketBuffering(bool fBuffering)
{
	if (!m_DtvEngine.m_MediaBuffer.EnableBuffering(fBuffering))
		return false;
	m_fPacketBuffering=fBuffering;
	return true;
}


bool CCoreEngine::SetPacketBufferLength(DWORD BufferLength)
{
	if (!m_DtvEngine.m_MediaBuffer.SetBufferLength(BufferLength))
		return false;
	m_PacketBufferLength=BufferLength;
	return true;
}


bool CCoreEngine::SetPacketBufferPoolPercentage(int Percentage)
{
	if (!m_DtvEngine.m_MediaBuffer.SetPoolPercentage(Percentage))
		return false;
	m_PacketBufferPoolPercentage=Percentage;
	return true;
}


bool CCoreEngine::GetVideoViewSize(int *pWidth,int *pHeight)
{
	WORD Width,Height;

	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)
			&& Width>0 && Height>0) {
		BYTE XAspect,YAspect;

		if (m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&XAspect,&YAspect)) {
			Width=Height*XAspect/YAspect;
		}
		if (pWidth)
			*pWidth=Width;
		if (pHeight)
			*pHeight=Height;
		return true;
	}
	return false;
}


bool CCoreEngine::SetVolume(int Volume)
{
	if (Volume<0 || Volume>MAX_VOLUME)
		return false;
	m_DtvEngine.SetVolume(LevelToDeciBel(Volume));
	m_Volume=Volume;
	m_fMute=false;
	return true;
}


bool CCoreEngine::SetMute(bool fMute)
{
	if (fMute!=m_fMute) {
		m_DtvEngine.SetVolume(fMute?-100.0f:LevelToDeciBel(m_Volume));
		m_fMute=fMute;
	}
	return true;
}


bool CCoreEngine::SetVolumeNormalizeLevel(int Level)
{
	if (Level<0)
		return false;
	if (Level!=m_VolumeNormalizeLevel) {
		m_DtvEngine.m_MediaViewer.SetAudioNormalize(Level!=100,(float)Level/100.0f);
		m_VolumeNormalizeLevel=Level;
	}
	return true;
}


bool CCoreEngine::SetStereoMode(int Mode)
{
	if (Mode<0 || Mode>2)
		return false;
	if (Mode!=m_StereoMode) {
		m_DtvEngine.SetStereoMode(Mode);
		m_StereoMode=Mode;
	}
	return true;
}


bool CCoreEngine::SetDownMixSurround(bool fDownMix)
{
	if (fDownMix!=m_fDownMixSurround) {
		m_DtvEngine.m_MediaViewer.SetDownMixSurround(fDownMix);
		m_fDownMixSurround=fDownMix;
	}
	return true;
}


DWORD CCoreEngine::UpdateAsyncStatus()
{
	DWORD Updated=0;

	WORD Width,Height;
	if (m_DtvEngine.m_MediaViewer.GetOriginalVideoSize(&Width,&Height)) {
		if (Width!=m_OriginalVideoWidth || Height!=m_OriginalVideoHeight) {
			m_OriginalVideoWidth=Width;
			m_OriginalVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}
	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)) {
		if (Width!=m_DisplayVideoWidth || Height!=m_DisplayVideoHeight) {
			m_DisplayVideoWidth=Width;
			m_DisplayVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}
	int NumAudioChannels=m_DtvEngine.GetAudioChannelNum();
	if (NumAudioChannels!=m_NumAudioChannels) {
		m_NumAudioChannels=NumAudioChannels;
		Updated|=STATUS_AUDIOCHANNELS;
	}
	int NumAudioStreams=m_DtvEngine.GetAudioStreamNum();
	if (NumAudioStreams!=m_NumAudioStreams) {
		m_NumAudioStreams=NumAudioStreams;
		Updated|=STATUS_AUDIOSTREAMS;
	}
	BYTE AudioComponentType=m_DtvEngine.GetAudioComponentType();
	if (AudioComponentType!=m_AudioComponentType) {
		m_AudioComponentType=AudioComponentType;
		Updated|=STATUS_AUDIOCOMPONENTTYPE;
		TRACE(TEXT("AudioComponentType = %x\n"),AudioComponentType);
	}
	WORD EventID=m_DtvEngine.GetEventID();
	if (EventID!=m_EventID) {
		m_EventID=EventID;
		Updated|=STATUS_EVENTID;
		TRACE(TEXT("EventID = %d\n"),EventID);
	}
	return Updated;
}


DWORD CCoreEngine::UpdateStatistics()
{
	DWORD Updated=0;

	DWORD ErrorCount=m_DtvEngine.m_TsPacketParser.GetErrorPacketCount();
	if (ErrorCount!=m_ErrorPacketCount) {
		m_ErrorPacketCount=ErrorCount;
		Updated|=STATISTIC_ERRORPACKETCOUNT;
	}
	DWORD ContinuityErrorCount=m_DtvEngine.m_TsPacketParser.GetContinuityErrorPacketCount();
	if (ContinuityErrorCount!=m_ContinuityErrorPacketCount) {
		m_ContinuityErrorPacketCount=ContinuityErrorCount;
		Updated|=STATISTIC_CONTINUITYERRORPACKETCOUNT;
	}
	DWORD ScrambleCount=m_DtvEngine.m_TsDescrambler.GetScramblePacketCount();
	if (ScrambleCount!=m_ScramblePacketCount) {
		m_ScramblePacketCount=ScrambleCount;
		Updated|=STATISTIC_SCRAMBLEPACKETCOUNT;
	}
	float SignalLevel;
	DWORD BitRate;
	DWORD StreamRemain;
	if (!m_fFileMode) {
		SignalLevel=m_DtvEngine.m_BonSrcDecoder.GetSignalLevel();
		BitRate=m_DtvEngine.m_BonSrcDecoder.GetBitRate();
		StreamRemain=m_DtvEngine.m_BonSrcDecoder.GetStreamRemain();
	} else {
		SignalLevel=0.0;
		BitRate=0;
		StreamRemain=0;
	}
	if (SignalLevel!=m_SignalLevel) {
		m_SignalLevel=SignalLevel;
		Updated|=STATISTIC_SIGNALLEVEL;
	}
	if (BitRate!=m_BitRate) {
		m_BitRate=BitRate;
		Updated|=STATISTIC_BITRATE;
	}
	if (StreamRemain!=m_StreamRemain) {
		m_StreamRemain=StreamRemain;
		Updated|=STATISTIC_STREAMREMAIN;
	}
	DWORD BufferUsedCount=m_DtvEngine.m_MediaBuffer.GetUsedBufferCount();
	if (BufferUsedCount!=m_PacketBufferUsedCount) {
		m_PacketBufferUsedCount=BufferUsedCount;
		Updated|=STATISTIC_PACKETBUFFERRATE;
	}
	return Updated;
}


void CCoreEngine::ResetErrorCount()
{
	m_DtvEngine.m_TsPacketParser.ResetErrorPacketCount();
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
	m_DtvEngine.m_TsDescrambler.ResetScramblePacketCount();
	m_ScramblePacketCount=0;
}


int CCoreEngine::GetPacketBufferUsedPercentage()
{
	return m_DtvEngine.m_MediaBuffer.GetUsedBufferCount()*100/
								m_DtvEngine.m_MediaBuffer.GetBufferLength();
}


bool CCoreEngine::UpdateEpgDataInfo()
{
	WORD ServiceID;

	SAFE_DELETE(m_pEpgDataInfo);
	SAFE_DELETE(m_pEpgDataInfoNext);
	if (!m_DtvEngine.GetServiceID(&ServiceID))
		return false;
	m_pEpgDataInfo=m_DtvEngine.GetEpgDataInfo(ServiceID,false);
	m_pEpgDataInfoNext=m_DtvEngine.GetEpgDataInfo(ServiceID,true);
	return true;
}


const CEpgDataInfo *CCoreEngine::GetEpgDataInfo(bool fNext) const
{
	if (fNext)
		return m_pEpgDataInfoNext;
	return m_pEpgDataInfo;
}


void *CCoreEngine::GetCurrentImage()
{
	BYTE *pDib;

#if 0
	if (m_DtvEngine.m_MediaViewer.GetGrabber()) {
		pDib=static_cast<BYTE*>(m_DtvEngine.m_MediaViewer.DoCapture(1000));
		return pDib;
	}
#endif
	bool fPause=m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_DEFAULT;

	if (fPause)
		m_DtvEngine.m_MediaViewer.Pause();
	if (!m_DtvEngine.m_MediaViewer.GetCurrentImage(&pDib))
		pDib=NULL;
	if (fPause)
		m_DtvEngine.m_MediaViewer.Play();
	return pDib;
}


bool CCoreEngine::SetMinTimerResolution(bool fMin)
{
	if ((m_TimerResolution!=0)!=fMin) {
		if (fMin) {
			TIMECAPS tc;

			if (::timeGetDevCaps(&tc,sizeof(tc))!=TIMERR_NOERROR)
				tc.wPeriodMin=1;
			if (::timeBeginPeriod(tc.wPeriodMin)!=TIMERR_NOERROR)
				return false;
			m_TimerResolution=tc.wPeriodMin;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Set %u\n"),m_TimerResolution);
		} else {
			::timeEndPeriod(m_TimerResolution);
			m_TimerResolution=0;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Reset\n"));
		}
	}
	return true;
}

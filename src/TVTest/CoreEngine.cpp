#include "stdafx.h"
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
	m_pDtvEngineHandler=NULL;
	m_fPacketBuffering=false;
	m_PacketBufferLength=m_DtvEngine.m_MediaBuffer.GetBufferLength();
	m_PacketBufferPoolPercentage=m_DtvEngine.m_MediaBuffer.GetPoolPercentage();
	m_VideoWidth=0;
	m_VideoHeight=0;
	m_NumAudioChannels=0;
	m_fMute=false;
	m_Volume=100;
	m_VolumeNormalizeLevel=100;
	m_StereoMode=0;
	m_ErrorPacketCount=0;
	m_ScramblePacketCount=0;
	m_SignalLevel=0.0;
	m_BitRate=0;
	m_PacketBufferUsedCount=0;
	m_StreamRemain=0;
	m_pEpgDataInfo=NULL;
}


CCoreEngine::~CCoreEngine()
{
	Close();
	delete m_pEpgDataInfo;
}


void CCoreEngine::Close()
{
	//m_DtvEngine.EnablePreview(false);
	//m_DtvEngine.ReleaseSrcFilter();
	m_DtvEngine.CloseEngine();
	if (m_hDriverLib) {
		::FreeLibrary(m_hDriverLib);
		m_hDriverLib=NULL;
	}
}


bool CCoreEngine::BuildDtvEngine(CDtvEngineHandler *pDtvEngineHandler)
{
	if (!m_DtvEngine.BuildEngine(pDtvEngineHandler,
				m_fDescramble?m_CardReaderType!=CCardReader::READER_NONE:false,
				true/*m_fPacketBuffering*/)) {
		return false;
	}
	m_pDtvEngineHandler=pDtvEngineHandler;
	return true;
}


bool CCoreEngine::SetDriverFileName(LPCTSTR pszFileName)
{
	if (::lstrlen(pszFileName)>=MAX_PATH)
		return false;
	::lstrcpy(m_szDriverFileName,pszFileName);
	return true;
}


bool CCoreEngine::LoadDriver()
{
	if (m_hDriverLib) {
		m_DtvEngine.ReleaseSrcFilter();
		::FreeLibrary(m_hDriverLib);
	}
	m_hDriverLib=::LoadLibrary(m_szDriverFileName);
	return m_hDriverLib!=NULL;
}


bool CCoreEngine::OpenDriver()
{
	if (m_hDriverLib==NULL)
		return false;
	m_DtvEngine.ReleaseSrcFilter();
	m_fFileMode=false;
	if (!m_DtvEngine.OpenSrcFilter_BonDriver(m_hDriverLib))
		return false;
	LPCTSTR pszName=m_DtvEngine.m_BonSrcDecoder.GetTunerName();
	m_DriverType=DRIVER_UNKNOWN;
	if (pszName!=NULL) {
		if (::lstrcmpi(pszName,TEXT("HDUS"))==0)
			m_DriverType=DRIVER_HDUS;
		else if (::_tcsncmp(pszName,TEXT("UDP/"),4)==0)
			m_DriverType=DRIVER_UDP;
	}
	return true;
}


bool CCoreEngine::OpenFile(LPCTSTR pszFileName)
{
	m_DtvEngine.ReleaseSrcFilter();
	m_fFileMode=true;
	if (!m_DtvEngine.OpenSrcFilter_File(pszFileName))
		return false;
	return true;
}


bool CCoreEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder)
{
	if (!m_DtvEngine.BuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,pszMpeg2Decoder)) {
		SetError(m_DtvEngine.GetLastErrorException());
		return false;
	}
	m_DtvEngine.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	m_DtvEngine.m_MediaViewer.SetAudioNormalize(m_VolumeNormalizeLevel!=100,
										(float)m_VolumeNormalizeLevel/100.0f);
	m_DtvEngine.SetStereoMode(m_StereoMode);
	return true;
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


bool CCoreEngine::IsBuildComplete() const
{
	return m_DtvEngine.IsBuildComplete();
}


bool CCoreEngine::EnablePreview(bool fPreview)
{
	if (!m_DtvEngine.EnablePreview(fPreview))
		return false;
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


DWORD CCoreEngine::UpdateAsyncStatus()
{
	DWORD Updated=0;

	WORD Width,Height;
	if (m_DtvEngine.GetVideoSize(&Width,&Height)) {
		if (Width!=m_VideoWidth || Height!=m_VideoHeight) {
			m_VideoWidth=Width;
			m_VideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}
	int NumAudioChannels=m_DtvEngine.GetAudioChannelNum();
	if (NumAudioChannels!=m_NumAudioChannels) {
		m_NumAudioChannels=NumAudioChannels;
		Updated|=STATUS_AUDIOCHANNELS;
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


int CCoreEngine::GetPacketBufferUsedPercentage()
{
	return m_DtvEngine.m_MediaBuffer.GetUsedBufferCount()*100/
								m_DtvEngine.m_MediaBuffer.GetBufferLength();
}


bool CCoreEngine::UpdateEpgDataInfo()
{
	delete m_pEpgDataInfo;
	m_pEpgDataInfo=m_DtvEngine.GetEpgDataInfo(m_DtvEngine.GetService(),false);
	return true;
}


void *CCoreEngine::GetCurrentImage()
{
	BYTE *pDib;

	if (m_DtvEngine.m_MediaViewer.GetGrabber()) {
		pDib=static_cast<BYTE*>(m_DtvEngine.m_MediaViewer.DoCapture(1000));
	} else {
		bool fPause=m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_DEFAULT;

		if (fPause)
			m_DtvEngine.m_MediaViewer.Pause();
		if (!m_DtvEngine.m_MediaViewer.GetCurrentImage(&pDib))
			pDib=NULL;
		if (fPause)
			m_DtvEngine.m_MediaViewer.Play();
	}
	return pDib;
}

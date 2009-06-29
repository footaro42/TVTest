#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H


#include "DtvEngine.h"


class CCoreEngine : public CBonErrorHandler {
public:
	enum DriverType {
		DRIVER_UNKNOWN,
		DRIVER_UDP,
		DRIVER_TCP,
		DRIVER_HDUS
	};
	enum { MAX_VOLUME=100 };

private:
	bool m_fFileMode;
	TCHAR m_szDriverFileName[MAX_PATH];
	HMODULE m_hDriverLib;
	DriverType m_DriverType;
	bool m_fDescramble;
	CCardReader::ReaderType m_CardReaderType;
	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;
	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_NumAudioChannels;
	BYTE m_AudioComponentType;
	bool m_fMute;
	int m_Volume;
	int m_VolumeNormalizeLevel;
	int m_StereoMode;
	bool m_fDownMixSurround;
	WORD m_EventID;
	DWORD m_ErrorPacketCount;
	DWORD m_ContinuityErrorPacketCount;
	DWORD m_ScramblePacketCount;
	float m_SignalLevel;
	DWORD m_BitRate;
	DWORD m_StreamRemain;
	DWORD m_PacketBufferUsedCount;
	CEpgDataInfo *m_pEpgDataInfo;
	CEpgDataInfo *m_pEpgDataInfoNext;
	UINT m_TimerResolution;

public:
	CCoreEngine();
	~CCoreEngine();
	void Close();
	bool BuildDtvEngine(CDtvEngine::CEventHandler *pEventHandler);
	bool SetDriverFileName(LPCTSTR pszFileName);
	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool IsDriverSpecified() const { return m_szDriverFileName[0]!='\0'; }
	bool LoadDriver();
	bool UnloadDriver();
	bool IsDriverLoaded() const { return m_hDriverLib!=NULL; }
	bool OpenDriver();
	bool CloseDriver();
	bool IsDriverOpen() const;
	//bool OpenFile(LPCTSTR pszFileName);
	bool IsFileMode() const { return m_fFileMode; }
	bool BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer=CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszMpeg2Decoder=NULL,LPCWSTR pszAudioDevice=NULL);
	bool CloseMediaViewer();
	bool OpenBcasCard();
	bool CloseBcasCard();
	bool IsBcasCardOpen() const;
	bool IsBuildComplete() const;
	bool EnablePreview(bool fPreview);
	bool SetDescramble(bool fDescramble);
	bool GetDescramble() const { return m_fDescramble; }
	bool SetCardReaderType(CCardReader::ReaderType Type);
	CCardReader::ReaderType GetCardReaderType() const { return m_CardReaderType; }
	DriverType GetDriverType() const { return m_DriverType; }
	bool IsUDPDriver() const { return m_DriverType==DRIVER_UDP; }
	bool IsTCPDriver() const { return m_DriverType==DRIVER_TCP; }
	bool IsNetworkDriver() const { return IsUDPDriver() || IsTCPDriver(); }
	static bool IsNetworkDriverFileName(LPCTSTR pszFileName);
	bool SetPacketBuffering(bool fBuffering);
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	bool SetPacketBufferLength(DWORD BufferLength);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	bool SetPacketBufferPoolPercentage(int Percentage);
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	bool GetVideoViewSize(int *pWidth,int *pHeight);
	int GetOriginalVideoWidth() const { return m_OriginalVideoWidth; }
	int GetOriginalVideoHeight() const { return m_OriginalVideoHeight; }
	int GetDisplayVideoWidth() const { return m_DisplayVideoWidth; }
	int GetDisplayVideoHeight() const { return m_DisplayVideoHeight; }
	bool SetVolume(int Volume);
	int GetVolume() const { return m_Volume; }
	bool SetMute(bool fMute);
	bool GetMute() const { return m_fMute; }
	bool SetVolumeNormalizeLevel(int Level);
	int GetVolumeNormalizeLevel() const { return m_VolumeNormalizeLevel; }
	bool SetStereoMode(int Mode);
	int GetStereoMode() const { return m_StereoMode; }
	bool SetDownMixSurround(bool fDownMix);
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	enum {
		STATUS_VIDEOSIZE			=0x00000001UL,
		STATUS_AUDIOCHANNELS		=0x00000002UL,
		STATUS_AUDIOCOMPONENTTYPE	=0x00000004UL,
		STATUS_EVENTID				=0x00000008UL
	};
	DWORD UpdateAsyncStatus();
	enum {
		STATISTIC_ERRORPACKETCOUNT				=0x00000001UL,
		STATISTIC_CONTINUITYERRORPACKETCOUNT	=0x00000002UL,
		STATISTIC_SCRAMBLEPACKETCOUNT			=0x00000004UL,
		STATISTIC_SIGNALLEVEL					=0x00000008UL,
		STATISTIC_BITRATE						=0x00000010UL,
		STATISTIC_STREAMREMAIN					=0x00000020UL,
		STATISTIC_PACKETBUFFERRATE				=0x00000040UL
	};
	DWORD UpdateStatistics();
	DWORD GetErrorPacketCount() const { return m_ErrorPacketCount; }
	DWORD GetContinuityErrorPacketCount() const { return m_ContinuityErrorPacketCount; }
	DWORD GetScramblePacketCount() const { return m_ScramblePacketCount; }
	void ResetErrorCount();
	float GetSignalLevel() const { return m_SignalLevel; }
	DWORD GetBitRate() const { return m_BitRate; }
	float GetBitRateFloat() const { return (float)m_BitRate/(float)(1024*1024); }
	DWORD GetStreamRemain() const { return m_StreamRemain; }
	int GetPacketBufferUsedPercentage();
	bool UpdateEpgDataInfo();
	const CEpgDataInfo *GetEpgDataInfo(bool fNext=false) const;
	void *GetCurrentImage();
	bool SetMinTimerResolution(bool fMin);
//private:
	CDtvEngine m_DtvEngine;
};


#endif

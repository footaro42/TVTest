#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H


#include "DtvEngine.h"


class CCoreEngine : public CBonErrorHandler {
public:
	enum DriverType {
		DRIVER_UNKNOWN,
		DRIVER_UDP,
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
	CDtvEngineHandler *m_pDtvEngineHandler;
	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;
	int m_VideoWidth;
	int m_VideoHeight;
	int m_NumAudioChannels;
	bool m_fMute;
	int m_Volume;
	int m_VolumeNormalizeLevel;
	int m_StereoMode;
	DWORD m_ErrorPacketCount;
	DWORD m_ScramblePacketCount;
	float m_SignalLevel;
	DWORD m_BitRate;
	DWORD m_StreamRemain;
	DWORD m_PacketBufferUsedCount;
	CEpgDataInfo *m_pEpgDataInfo;
public:
	CCoreEngine();
	~CCoreEngine();
	void Close();
	bool BuildDtvEngine(CDtvEngineHandler *pDtvEngineHandler);
	bool SetDriverFileName(LPCTSTR pszFileName);
	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool LoadDriver();
	bool IsDriverLoaded() const { return m_hDriverLib!=NULL; }
	bool OpenDriver();
	bool OpenFile(LPCTSTR pszFileName);
	bool IsFileMode() const { return m_fFileMode; }
	bool BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer=CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszMpeg2Decoder=NULL);
	bool OpenBcasCard();
	bool IsBuildComplete() const;
	bool EnablePreview(bool fPreview);
	bool SetDescramble(bool fDescramble);
	bool GetDescramble() const { return m_fDescramble; }
	bool SetCardReaderType(CCardReader::ReaderType Type);
	CCardReader::ReaderType GetCardReaderType() const { return m_CardReaderType; }
	DriverType GetDriverType() const { return m_DriverType; }
	bool IsUDPDriver() const { return m_DriverType==DRIVER_UDP; }
	bool SetPacketBuffering(bool fBuffering);
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	bool SetPacketBufferLength(DWORD BufferLength);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	bool SetPacketBufferPoolPercentage(int Percentage);
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	bool GetVideoViewSize(int *pWidth,int *pHeight);
	bool SetVolume(int Volume);
	int GetVolume() const { return m_Volume; }
	bool SetMute(bool fMute);
	bool GetMute() const { return m_fMute; }
	bool SetVolumeNormalizeLevel(int Level);
	int GetVolumeNormalizeLevel() const { return m_VolumeNormalizeLevel; }
	bool SetStereoMode(int Mode);
	int GetStereoMode() const { return m_StereoMode; }
	enum {
		STATUS_VIDEOSIZE		=0x00000001UL,
		STATUS_AUDIOCHANNELS	=0x00000002UL
	};
	DWORD UpdateAsyncStatus();
	enum {
		STATISTIC_ERRORPACKETCOUNT		=0x00000001UL,
		STATISTIC_SCRAMBLEPACKETCOUNT	=0x00000002UL,
		STATISTIC_SIGNALLEVEL			=0x00000004UL,
		STATISTIC_BITRATE				=0x00000008UL,
		STATISTIC_STREAMREMAIN			=0x00000010UL,
		STATISTIC_PACKETBUFFERRATE		=0x00000020UL
	};
	DWORD UpdateStatistics();
	DWORD GetErrorPacketCount() const { return m_ErrorPacketCount; }
	DWORD GetScramblePacketCount() const { return m_ScramblePacketCount; }
	float GetSignalLevel() const { return m_SignalLevel; }
	DWORD GetBitRate() const { return m_BitRate; }
	float GetBitRateFloat() const { return (float)(m_BitRate*8)/(float)(1024*1024); }
	DWORD GetStreamRemain() const { return m_StreamRemain; }
	int GetPacketBufferUsedPercentage();
	bool UpdateEpgDataInfo();
	const CEpgDataInfo *GetEpgDataInfo() const { return m_pEpgDataInfo; }
	void *GetCurrentImage();
//private:
	CDtvEngine m_DtvEngine;
};


#endif

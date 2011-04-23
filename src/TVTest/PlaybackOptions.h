#ifndef PLAYBACK_OPTIONS_H
#define PLAYBACK_OPTIONS_H


#include "Options.h"
#include "BonTsEngine/MediaViewer.h"


class CPlaybackOptions : public COptions
{
	enum {
		UPDATE_ADJUSTAUDIOSTREAMTIME	= 0x00000001UL,
		UPDATE_PTSSYNC					= 0x00000002UL,
		UPDATE_PACKETBUFFERING			= 0x00000004UL,
		UPDATE_STREAMTHREADPRIORITY		= 0x00000008UL
#ifdef TVH264
		, UPDATE_ADJUSTFRAMERATE		= 0x00000010UL
#endif
	};
	enum {
		MAX_AUDIO_DEVICE_NAME = 128,
		MAX_AUDIO_FILTER_NAME = 128
	};
	enum {
		MAX_PACKET_BUFFER_LENGTH = 0x00100000UL
	};

	TCHAR m_szAudioDeviceName[MAX_AUDIO_DEVICE_NAME];
	TCHAR m_szAudioFilterName[MAX_AUDIO_FILTER_NAME];

	CAacDecFilter::SpdifOptions m_SpdifOptions;
	bool m_fDownMixSurround;
	bool m_fRestoreMute;
	bool m_fRestorePlayStatus;

	bool m_fUseAudioRendererClock;
	bool m_fEnablePTSSync;
	bool m_fAdjustAudioStreamTime;
	bool m_fMinTimerResolution;

	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;
	int m_StreamThreadPriority;

#ifdef TVH264
	bool m_fAdjustFrameRate;
#endif

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CPlaybackOptions();
	~CPlaybackOptions();
// COptions
	bool Apply(DWORD Flags);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CBasicDialog
	bool Create(HWND hwndOwner);
// CPlaybackOptions
	LPCTSTR GetAudioDeviceName() const { return m_szAudioDeviceName; }
	LPCTSTR GetAudioFilterName() const { return m_szAudioFilterName; }
	const CAacDecFilter::SpdifOptions &GetSpdifOptions() const { return m_SpdifOptions; }
	bool SetSpdifOptions(const CAacDecFilter::SpdifOptions &Options);
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	bool GetRestoreMute() const { return m_fRestoreMute; }
	bool GetRestorePlayStatus() const { return m_fRestorePlayStatus; }
	bool GetUseAudioRendererClock() const { return m_fUseAudioRendererClock; }
	bool GetAdjustAudioStreamTime() const { return m_fAdjustAudioStreamTime; }
	bool GetMinTimerResolution() const { return m_fMinTimerResolution; }
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	void SetPacketBuffering(bool fBuffering);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	int GetStreamThreadPriority() const { return m_StreamThreadPriority; }
};


#endif

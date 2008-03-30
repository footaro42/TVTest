// TsDemuxer.h: CTsDemuxer クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsTable.h"
#include "TsMedia.h"


/////////////////////////////////////////////////////////////////////////////
// TS分離デコーダ(PIDによるルーティング及びPAT/PMTを処理する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket			TSパケット
// Output	#0	: CMpeg2Sequence	MPEG2-ESシーケンス
// Output	#1	: CAdtsFrame		ADTSフレーム
/////////////////////////////////////////////////////////////////////////////

class CTsDemuxer :	public CMediaDecoder,
					protected CPesParser::IPacketHandler,
					protected CMpeg2Parser::ISequenceHandler,
					protected CAdtsParser::IFrameHandler
{
public:
	enum {OUTPUT_VIDEO, OUTPUT_AUDIO};

	CTsDemuxer(IEventHandler *pEventHandler = NULL);
	virtual ~CTsDemuxer();

// IMediaDecoder
	virtual void Reset(void);
	virtual const DWORD GetInputNum(void) const;
	virtual const DWORD GetOutputNum(void) const;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsDemuxer
	const bool SetVideoPID(const WORD wPID);
	const bool SetAudioPID(const WORD wPID);
	void EnableLipSync(const bool bEnable = true);

protected:
// CPesParser::IPacketHandler、CMpeg2Parser::ISequenceHandler、CAdtsParser::IFrameHandler
	virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket);
	virtual void OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence);
	virtual void OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame);

	// CTsClockRef m_TsClockRef; 音ずれ補正のときは使うかも(PCRでリップシンクする)
	CPesParser m_VideoPesParser;
	CPesParser m_AudioPesParser;

	CMpeg2Parser m_Mpeg2Parser;
	CAdtsParser m_AdtsParser;

	WORD m_wVideoPID;
	WORD m_wAudioPID;

	bool m_bLipSyncEnable;
	bool m_bWaitingForVideo;
};

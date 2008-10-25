// AacDecoder.cpp: CAacDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AacDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
#pragma comment(lib, "LibFaad.lib")


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CAacDecoder::CAacDecoder(IPcmHandler *pPcmHandler)
	: m_pPcmHandler(pPcmHandler)
	, m_hDecoder(NULL)
	, m_bInitRequest(false)
	, m_byLastChannelConfig(0U)
{

}


CAacDecoder::~CAacDecoder()
{
	CloseDecoder();
}


const bool CAacDecoder::OpenDecoder(void)
{
	CloseDecoder();

	// FAAD2オープン
	m_hDecoder = ::NeAACDecOpen();
	if (m_hDecoder==NULL)
		return false;

	// デフォルト設定取得
	NeAACDecConfigurationPtr pDecodeConfig = ::NeAACDecGetCurrentConfiguration(m_hDecoder);

	/*
	// Openが成功すればNULLにはならない
	if (pDecodeConfig==NULL) {
		CloseDecoder();
		return false;
	}
	*/

	// デコーダ設定
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;

	if (!::NeAACDecSetConfiguration(m_hDecoder, pDecodeConfig)) {
		CloseDecoder();
		return false;
	}

	m_bInitRequest = true;
	m_byLastChannelConfig = 0xFFU;

	return true;
}


void CAacDecoder::CloseDecoder()
{
	// FAAD2クローズ
	if (m_hDecoder) {
		::NeAACDecClose(m_hDecoder);
		m_hDecoder = NULL;
	}
}


const bool CAacDecoder::ResetDecoder(void)
{
	if (m_hDecoder==NULL)
		return false;
	return OpenDecoder();
}


const bool CAacDecoder::Decode(const CAdtsFrame *pFrame)
{
	if (m_hDecoder==NULL)
		return false;

	// デコード

	// チャンネル設定解析
	if (pFrame->GetChannelConfig() != m_byLastChannelConfig) {
		// チャンネル設定が変化した、デコーダリセット
		ResetDecoder();
		m_byLastChannelConfig = pFrame->GetChannelConfig();
	}

	// 初回フレーム解析
	if (m_bInitRequest) {
		unsigned long SampleRate;
		unsigned char Channels;

		if (::NeAACDecInit(m_hDecoder, pFrame->GetData(), pFrame->GetSize(), &SampleRate, &Channels) < 0) {
			return false;
		}
		m_bInitRequest = false;
	}
	// デコード
	NeAACDecFrameInfo FrameInfo;
	//::ZeroMemory(&FrameInfo, sizeof(FrameInfo));

	BYTE *pPcmBuffer = (BYTE *)::NeAACDecDecode(m_hDecoder, &FrameInfo, pFrame->GetData(), pFrame->GetSize());

	if (FrameInfo.error==0) {
		if (FrameInfo.samples>0) {
			// L-PCMハンドラに通知
			if (m_pPcmHandler)
				m_pPcmHandler->OnPcmFrame(this, pPcmBuffer, FrameInfo.samples / FrameInfo.channels, FrameInfo.channels);
		}
	} else {
		// エラー発生
#ifdef _DEBUG
		::OutputDebugString(TEXT("CAacDecoder::Decode error - "));
		::OutputDebugStringA(NeAACDecGetErrorMessage(FrameInfo.error));
		::OutputDebugString(TEXT("\n"));
#endif
		// 何回も初期化するとメモリリークする
		//m_bInitRequest = true;
	}

	return true;
}


void CAacDecoder::OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame)
{
	// CAdtsParser::IFrameHandlerの実装
	Decode(pFrame);
}

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
	, m_InitRequest(false)
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
	if(!(m_hDecoder = ::faacDecOpen()))return false;

	// デフォルト設定取得
	faacDecConfigurationPtr pDecodeConfig = ::faacDecGetCurrentConfiguration(m_hDecoder);
	
	if(!pDecodeConfig){
		CloseDecoder();
		return false;
		}

	// デコーダ設定
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;

	if(!::faacDecSetConfiguration(m_hDecoder, pDecodeConfig)){
		CloseDecoder();
		return false;
		}
	
	m_InitRequest = true;
	m_byLastChannelConfig = 0xFFU;

	return true;
}

void CAacDecoder::CloseDecoder()
{
	// FAAD2クローズ
	if(m_hDecoder){
		::faacDecClose(m_hDecoder);
		m_hDecoder = NULL;
		}
}

const bool CAacDecoder::ResetDecoder(void)
{
	if(!m_hDecoder)return false;
	
	// FAAD2クローズ
	::faacDecClose(m_hDecoder);

	// FAAD2オープン
	if(!(m_hDecoder = ::faacDecOpen()))return false;

	// デフォルト設定取得
	faacDecConfigurationPtr pDecodeConfig = ::faacDecGetCurrentConfiguration(m_hDecoder);
	
	if(!pDecodeConfig){
		CloseDecoder();
		return false;
		}

	// デコーダ設定
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;

	if(!::faacDecSetConfiguration(m_hDecoder, pDecodeConfig)){
		CloseDecoder();
		return false;
		}
	
	m_InitRequest = true;
	m_byLastChannelConfig = 0xFFU;

	return true;
}

const bool CAacDecoder::Decode(const CAdtsFrame *pFrame)
{
	if(!m_hDecoder)return false;

	// デコード
	DWORD dwSamples = 0UL;
	BYTE byChannels = 0U;
	
	// チャンネル設定解析
	if(pFrame->GetChannelConfig() != m_byLastChannelConfig){
		// チャンネル設定が変化した、デコーダリセット
		ResetDecoder();
		m_byLastChannelConfig = pFrame->GetChannelConfig();
		}	
	
	// 初回フレーム解析
	if(m_InitRequest){
		if(::faacDecInit(m_hDecoder, pFrame->GetData(), pFrame->GetSize(), &dwSamples, &byChannels) < 0){
			return false;
			}
		
		m_InitRequest = false;
		}
	
	// デコード
	faacDecFrameInfo FrameInfo;
	::ZeroMemory(&FrameInfo, sizeof(FrameInfo));
	
	BYTE *pPcmBuffer = (BYTE *)::faacDecDecode(m_hDecoder, &FrameInfo, pFrame->GetData(), pFrame->GetSize());

	if((!FrameInfo.error) && (FrameInfo.samples > 0L)){
		// L-PCMハンドラに通知
		if(m_pPcmHandler)m_pPcmHandler->OnPcmFrame(this, pPcmBuffer, FrameInfo.samples / (DWORD)FrameInfo.channels, FrameInfo.channels);
		}
	else{
		// エラー発生
		m_InitRequest = true;
		}

	return true;
}

void CAacDecoder::OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame)
{
	// CAdtsParser::IFrameHandlerの実装
	Decode(pFrame);
}

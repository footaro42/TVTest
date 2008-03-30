// MediaViewer.h: CMediaViewer クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsUtilClass.h"
#include "BonSrcFilter.h"
#include "AacDecFilter.h"
#include <Bdaiface.h>


/////////////////////////////////////////////////////////////////////////////
// メディアビューア(映像及び音声を再生する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket		入力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaViewer : public CMediaDecoder  
{
public:
	CMediaViewer(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaViewer();

// IMediaDecoder
	virtual void Reset(void);

	virtual const DWORD GetInputNum(void) const;
	virtual const DWORD GetOutputNum(void) const;

	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaViewer
	const bool OpenViewer(HWND hOwnerHwnd = NULL);
	void CloseViewer(void);

	const bool Play(void);
	const bool Stop(void);
	
	const bool SetVideoPID(const WORD wPID);
	const bool SetAudioPID(const WORD wPID);

protected:
	CCriticalLock m_CriticalLock;

	// DirectShowインタフェース
	IGraphBuilder *m_pFilterGraph;
	IMediaControl *m_pMediaControl;
	
	// DirectShowフィルタ
	CBonSrcFilter *m_pBonSrcFilter;
	CAacDecFilter *m_pAacDecFilter;
	IBaseFilter *m_pMp2DemuxFilter;

	// MPEG2Demultiplexerインタフェース
	IMpeg2Demultiplexer *m_pMp2DemuxInterface;
	IReferenceClock *m_pMp2DemuxRefClock;

	// DirectShowピン
	IPin *m_pBonSrcOutputPin;

	IPin *m_pMp2DemuxInputPin;
	IPin *m_pMp2DemuxVideoPin;
	IPin *m_pMp2DemuxAudioPin;

	IPin *m_pAacDecInputPin;
	IPin *m_pAacDecOutputPin;
	
	// ビデオPIDマップ
	IMPEG2PIDMap *m_pMp2DemuxVideoMap;
	IMPEG2PIDMap *m_pMp2DemuxAudioMap;
	
	// ビデオウィンドウ
	IVideoWindow *m_pVideoWindow;
	
	// Elementary StreamのPID
	WORD m_wVideoEsPID;
	WORD m_wAudioEsPID;
	
private:
	HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const;
	void RemoveFromRot(const DWORD pdwRegister) const;

	DWORD m_dwRegister;
};

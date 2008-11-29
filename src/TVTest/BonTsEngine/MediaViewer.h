// MediaViewer.h: CMediaViewer クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsUtilClass.h"
#include "BonSrcFilter.h"
#include "AacDecFilter.h"
#include "Mpeg2SequenceFilter.h"
#include "PcmSelectFilter.h"
#include <Bdaiface.h>
#include "DirectShowUtil.h"

// Append by HDUSTestの中の人
#include "VideoRenderer.h"
#include "../ImageMixer.h"
#include "../Tracer.h"


/////////////////////////////////////////////////////////////////////////////
// メディアビューア(映像及び音声を再生する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket		入力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaViewer : public CMediaDecoder, public CBonErrorHandler
{
public:
	enum EVENTID {
		EID_FILTER_GRAPH_FLUSH
	};
	enum {
		PID_INVALID=0xFFFF
	};

	CMediaViewer(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaViewer();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaViewer
	const bool OpenViewer(HWND hOwnerHwnd = NULL,HWND hMessageDrainHwnd = NULL,
		CVideoRenderer::RendererType RendererType = CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszMpeg2Decoder = NULL);
	void CloseViewer(void);
	const bool IsOpen() const;

	const bool Play(void);
	const bool Stop(void);

	const bool SetVideoPID(const WORD wPID);
	const bool SetAudioPID(const WORD wPID);

	// Append by Meru
	const bool ResizeVideoWindow();
	const bool SetViewSize(const int x,const int y);
	const bool SetVolume(const float fVolume);
	const bool GetVideoSize(WORD *pwWidth,WORD *pwHeight) const;
	const bool GetVideoAspectRatio(BYTE *pbyAspectRatioX,BYTE *pbyAspectRatioY) const;
	const BYTE GetAudioChannelNum();
	const bool SetStereoMode(const int iMode);
	const int GetStereoMode() const;
	const bool GetVideoDecoderName(LPWSTR lpName,int iBufLen);
	const bool DisplayVideoDecoderProperty(HWND hWndParent);
	const bool DisplayVideoRandererProperty(HWND hWndParent);

	// Append by HDUSTestの中の人
	const bool Pause();
	const bool Flush();
	const bool ForceAspectRatio(int AspectX,int AspectY);
	const bool GetForceAspectRatio(int *pAspectX,int *pAspectY) const;
	const bool GetEffectiveAspectRatio(BYTE *pAspectX,BYTE *pAspectY);
	enum { PANANDSCAN_HORZ=1, PANANDSCAN_VERT=2 };
	const bool SetPanAndScan(BYTE bFlags);
	BYTE GetPanAndScan() const { return m_PanAndScan; }
	enum ViewStretchMode {
		STRETCH_KEEPASPECTRATIO,
		STRETCH_CUTFRAME,
		STRETCH_FIT
	};
	const bool SetViewStretchMode(ViewStretchMode Mode);
	const bool GetOriginalVideoSize(WORD *pWidth,WORD *pHeight) const;
	const bool GetDestRect(RECT *pRect);
	const bool GetDestSize(WORD *pWidth,WORD *pHeight);
	const bool GetCroppedVideoSize(WORD *pWidth,WORD *pHeight);
	const bool GetSourceRect(RECT *pRect);
	bool SetVisible(bool fVisible);
	const void HideCursor(bool bHide);
	const bool GetCurrentImage(BYTE **ppDib);
#ifdef USE_GABBER_FILTER
	bool SetGrabber(bool bGrabber);
	bool GetGrabber() const { return m_pGrabber!=NULL; }
	void *DoCapture(DWORD WaitTime);
#endif
	bool SetAudioNormalize(bool bNormalize,float Level=1.0f);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	const bool RepaintVideo(HWND hwnd,HDC hdc);
	const bool DisplayModeChanged();
	const bool DrawText(LPCTSTR pszText,HFONT hfont,COLORREF crColor,
												int Opacity,RECT *pDestRect);
	const bool ClearOSD();
	bool SetTracer(CTracer *pTracer);
	void Trace(LPCTSTR pszOutput, ...);
	bool CheckHangUp(DWORD TimeOut);
protected:
	//CCriticalLock m_CriticalLock;

	// DirectShowインタフェース
	bool m_bInit;
	IMediaControl *m_pMediaControl;

	// DirectShowフィルタ
	IGraphBuilder *m_pFilterGraph;
	IBaseFilter *m_pMpeg2DecFilter;
	// Source
	IBaseFilter *m_pSrcFilter;
	CBonSrcFilter *m_pBonSrcFilterClass;
	// Mpeg2-Sequence
	IBaseFilter *m_pMpeg2SeqFilter;
	CMpeg2SequenceFilter *m_pMpeg2SeqClass;
	// AAC
	IBaseFilter *m_pAacDecFilter;
	CAacDecFilter *m_pAacDecClass;
	/*
	// Pcm Select
	IBaseFilter *m_pPcmSelFilter;
	CPcmSelectFilter *m_pPcmSelClass;
	*/
	// Renderer
	CVideoRenderer *m_pVideoRenderer;

	LPWSTR m_pszMpeg2DecoderName;

	// MPEG2Demultiplexerインタフェース
	IBaseFilter *m_pMp2DemuxFilter;

	// PIDマップ
	IMPEG2PIDMap *m_pMp2DemuxVideoMap;
	IMPEG2PIDMap *m_pMp2DemuxAudioMap;

	// Elementary StreamのPID
	WORD m_wVideoEsPID;
	WORD m_wAudioEsPID;

	static void CALLBACK OnMpeg2VideoInfo(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	WORD m_wVideoWindowX;
	WORD m_wVideoWindowY;
	CMpeg2VideoInfo m_VideoInfo;
	HWND m_hOwnerWnd;

	// Append by HDUSTestの中の人
	CVideoRenderer::RendererType m_VideoRendererType;
	int m_ForceAspectX,m_ForceAspectY;
	BYTE m_PanAndScan;
	ViewStretchMode m_ViewStretchMode;
	CImageMixer *m_pImageMixer;
#ifdef USE_GABBER_FILTER
	bool m_bGrabber;
	class CGrabber *m_pGrabber;
#endif
	CTracer *m_pTracer;
	HANDLE m_hFlushThread;
	HANDLE m_hFlushEvent;
	HANDLE m_hFlushResumeEvent;
	DWORD m_LastFlushTime;
	volatile enum {
		FLUSH_ABORT,
		FLUSH_WAIT,
		FLUSH_RESET
	} m_FlushEventType;
	static DWORD WINAPI FlushThread(LPVOID lpParameter);
	const bool CalcSourcePosition(long *pLeft,long *pTop,long *pWidth,long *pHeight) const;

#ifdef DEBUG
private:
	HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const;
	void RemoveFromRot(const DWORD pdwRegister) const;
	DWORD m_dwRegister;
#endif
};

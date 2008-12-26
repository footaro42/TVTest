#ifndef EVRENDERER_H
#define EVRENDERER_H


#include "VideoRenderer.h"


class CVideoRenderer_EVR : public CVideoRenderer
{
	HMODULE m_hMFPlatLib;
public:
	CVideoRenderer_EVR();
	~CVideoRenderer_EVR();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect);
	bool GetDestPosition(RECT *pRect);
	bool GetCurrentImage(void **ppBuffer);
	bool RepaintVideo(HWND hwnd,HDC hdc);
	bool DisplayModeChanged();
	bool SetVisible(bool fVisible);
};


#endif

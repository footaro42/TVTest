#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H


#include "Exception.h"


class CVideoRenderer : public CBonErrorHandler {
protected:
	IBaseFilter *m_pRenderer;
	IGraphBuilder *m_pFilterGraph;
	HWND m_hwndRender;
public:
	enum RendererType {
		RENDERER_UNDEFINED=-1,
		RENDERER_DEFAULT,
		RENDERER_VMR7,
		RENDERER_VMR9
	};
	CVideoRenderer();
	virtual ~CVideoRenderer();
	virtual bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)=0;
	virtual bool Finalize() { return true; }
	virtual bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
							const RECT *pDestRect,const RECT *pWindowRect)=0;
	virtual bool GetDestPosition(RECT *pRect)=0;
	virtual bool GetCurrentImage(void **ppBuffer) { return false; }
	virtual bool ShowCursor(bool fShow) { return true; }
	virtual bool RepaintVideo(HWND hwnd,HDC hdc) { return true; }
	virtual bool DisplayModeChanged() { return true; }
	virtual bool SetVisible(bool fVisible) { return true; }
	virtual bool ShowProperty(HWND hwndOwner);
	IBaseFilter *GetRendererFilter() const { return m_pRenderer; }
	static bool CreateRenderer(RendererType Type,CVideoRenderer **ppRenderer);
	static LPCTSTR EnumRendererName(int Index);
	static RendererType ParseName(LPCTSTR pszName);
};


#endif

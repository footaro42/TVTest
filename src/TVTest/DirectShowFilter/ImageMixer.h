#ifndef IMAGE_MIXER_H
#define IMAGE_MIXER_H


#include "VideoRenderer.h"


class CImageMixer {
protected:
	IBaseFilter *m_pRenderer;
public:
	CImageMixer(IBaseFilter *pRenderer);
	virtual ~CImageMixer();
	virtual void Clear()=0;
	virtual bool SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect)=0;
	virtual bool SetText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF Color,int Opacity)=0;
	virtual bool GetMapSize(int *pWidth,int *pHeight)=0;
	static CImageMixer *CreateImageMixer(CVideoRenderer::RendererType RendererType,
										 IBaseFilter *pRendererFilter);
	static bool IsSupported(CVideoRenderer::RendererType RendererType);
};

class CImageMixer_VMR7 : public CImageMixer {
	HDC m_hdc;
	HBITMAP m_hbm;
	HBITMAP m_hbmOld;
	bool CreateMemDC();
public:
	CImageMixer_VMR7(IBaseFilter *pRenderer);
	~CImageMixer_VMR7();
	void Clear();
	bool SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect);
	bool SetText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF Color,int Opacity);
	bool GetMapSize(int *pWidth,int *pHeight);
};

class CImageMixer_VMR9 : public CImageMixer {
	HDC m_hdc;
	HBITMAP m_hbm;
	HBITMAP m_hbmOld;
	bool CreateMemDC();
public:
	CImageMixer_VMR9(IBaseFilter *pRenderer);
	~CImageMixer_VMR9();
	void Clear();
	bool SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect);
	bool SetText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF Color,int Opacity);
	bool GetMapSize(int *pWidth,int *pHeight);
};


#endif

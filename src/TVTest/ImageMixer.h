#ifndef IMAGE_MIXER_H
#define IMAGE_MIXER_H


class CImageMixer {
	IBaseFilter *m_pVMR9;
	HDC m_hdc;
	HBITMAP m_hbm;
	HBITMAP m_hbmOld;
	bool CreateMemDC();
public:
	CImageMixer(IBaseFilter *pVMR9);
	~CImageMixer();
	void Clear();
	bool SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect);
	bool SetText(LPCTSTR pszText,HFONT hfont,COLORREF Color,int Opacity,
															RECT *pDestRect);
};


#endif

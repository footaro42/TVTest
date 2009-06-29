#ifndef VIRTUAL_SCREEN_H
#define VIRTUAL_SCREEN_H


class CVirtualScreen {
	HDC m_hdc;
	HBITMAP m_hbm;
	HBITMAP m_hbmOld;
	int m_Width;
	int m_Height;

public:
	CVirtualScreen();
	~CVirtualScreen();
	bool Create(int Width,int Height,HDC hdc=NULL);
	void Destroy();
	bool IsCreated() const { return m_hdc!=NULL; }
	HDC GetDC() const { return m_hdc; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
};


#endif

#ifndef AERO_H
#define AERO_H


class CAeroGlass {
	HMODULE m_hDwmLib;
	bool LoadDwmLib();

public:
	CAeroGlass();
	~CAeroGlass();
	bool ApplyAeroGlass(HWND hwnd,const RECT *pRect);
	bool EnableNcRendering(HWND hwnd,bool fEnable);
};


#endif

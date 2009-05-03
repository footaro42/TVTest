#ifndef AERO_H
#define AERO_H


class CAeroGlass {
	HMODULE m_hDwmLib;
public:
	CAeroGlass();
	~CAeroGlass();
	bool ApplyAeroGlass(HWND hwnd,const RECT *pRect);
};


#endif

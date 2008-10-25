#ifndef POPUP_INFO_H
#define POPUP_INFO_H


#include "CBasicWindow.h"


class CPopupInfo : public CBasicWindow {
public:
	CPopupInfo();
	~CPopupInfo();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
};


#endif

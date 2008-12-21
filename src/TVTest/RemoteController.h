#ifndef REMOTE_CONTROLLER_H
#define REMOTE_CONTROLLER_H


class CRemoteController {
	HMODULE m_hLib;
	bool m_fHook;
	UINT m_Message;
	HWND m_hwnd;
public:
	CRemoteController(HWND hwnd);
	~CRemoteController();
	bool BeginHook(bool fLocal);
	bool EndHook();
	bool TranslateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

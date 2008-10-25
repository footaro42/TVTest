#ifndef RESIDENT_MANAGER_H
#define RESIDENT_MANAGER_H


class CResidentManager {
	bool m_fResident;
	UINT m_Status;
	HWND m_hwnd;
	HINSTANCE m_hinst;
	UINT m_TaskbarCreatedMessage;
	bool AddTrayIcon();
	bool RemoveTrayIcon();
	bool ChangeTrayIcon();
public:
	enum {
		STATUS_RECORDING=0x00000001
	};
	CResidentManager();
	~CResidentManager();
	bool Initialize(HWND hwnd,HINSTANCE hinst);
	void Finalize();
	bool SetResident(bool fResident);
	bool GetResident() const { return m_fResident; }
	bool SetStatus(UINT Status,UINT Mask=(UINT)-1);
	UINT GetStatus() const { return m_Status; }
	bool HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam);
};


#endif

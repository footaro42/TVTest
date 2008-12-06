#ifndef RESIDENT_MANAGER_H
#define RESIDENT_MANAGER_H


class CResidentManager {
	HWND m_hwnd;
	bool m_fResident;
	bool m_fMinimizeToTray;
	UINT m_Status;
	UINT m_TaskbarCreatedMessage;
	bool AddTrayIcon();
	bool RemoveTrayIcon();
	bool ChangeTrayIcon();
	bool IsTrayIconVisible() const;
public:
	enum {
		STATUS_RECORDING	=0x00000001,
		STATUS_MINIMIZED	=0x00000002
	};
	CResidentManager();
	~CResidentManager();
	bool Initialize(HWND hwnd);
	void Finalize();
	bool SetResident(bool fResident);
	bool GetResident() const { return m_fResident; }
	bool SetMinimizeToTray(bool fMinimizeToTray);
	bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
	bool SetStatus(UINT Status,UINT Mask=(UINT)-1);
	UINT GetStatus() const { return m_Status; }
	bool HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam);
};


#endif

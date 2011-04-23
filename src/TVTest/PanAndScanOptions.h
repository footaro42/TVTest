#ifndef PAN_AND_SCAN_OPTIONS_H
#define PAN_AND_SCAN_OPTIONS_H


#include <vector>
#include "Settings.h"
#include "Dialog.h"
#include "UICore.h"


class CPanAndScanOptions : public CBasicDialog
{
public:
	enum { MAX_NAME=64 };
	struct PanAndScanInfo
	{
		CUICore::PanAndScanInfo Info;
		TCHAR szName[MAX_NAME];
		UINT ID;
	};

	CPanAndScanOptions();
	~CPanAndScanOptions();
	bool Show(HWND hwndOwner) override;
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	size_t GetPresetCount() const;
	bool GetPreset(size_t Index,PanAndScanInfo *pInfo) const;
	bool GetPresetByID(UINT ID,PanAndScanInfo *pInfo) const;
	UINT GetPresetID(size_t Index) const;
	int FindPresetByID(UINT ID) const;

private:
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetItemStatus() const;
	bool GetSettings(PanAndScanInfo *pInfo) const;
	bool GetPanAndScanSettings(CUICore::PanAndScanInfo *pInfo) const;
	bool IsSettingsValid() const;

	std::vector<PanAndScanInfo> m_PresetList;
	UINT m_PresetID;
	bool m_fStateChanging;
	bool m_fTested;
	CUICore::PanAndScanInfo m_OldPanAndScanInfo;
};


#endif

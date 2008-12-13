#ifndef PLUGIN_H
#define PLUGIN_H


#include <vector>
#include "TVTestPlugin.h"
#include "PointerArray.h"
#include "Options.h"


class CPlugin {
	HMODULE m_hLib;
	LPTSTR m_pszFileName;
	TVTest::PluginParam m_PluginParam;
	DWORD m_Type;
	DWORD m_Flags;
	LPWSTR m_pszPluginName;
	LPWSTR m_pszCopyright;
	LPWSTR m_pszDescription;
	bool m_fEnabled;
	TVTest::EventCallbackFunc m_pEventCallback;
	void *m_pEventCallbackClientData;
	class CMediaGrabberInfo {
	public:
		CPlugin *m_pPlugin;
		TVTest::StreamCallbackInfo m_CallbackInfo;
		CMediaGrabberInfo(CPlugin *pPlugin,TVTest::StreamCallbackInfo *pCallbackInfo) {
			m_pPlugin=pPlugin;
			m_CallbackInfo=*pCallbackInfo;
		}
	};
	static bool m_fSetGrabber;
	static CPointerVector<CMediaGrabberInfo> m_GrabberList;
	static CCriticalLock m_GrabberLock;
	static bool CALLBACK GrabMediaCallback(const CMediaData *pMediaData,const PVOID pParam);
	static LRESULT CALLBACK Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);
public:
	CPlugin();
	~CPlugin();
	bool Load(LPCTSTR pszFileName);
	void Free();
	bool IsLoaded() const { return m_hLib!=NULL; }
	bool IsEnabled() const { return m_fEnabled; }
	bool Enable(bool fEnable);
	LPCTSTR GetFileName() const { return m_pszFileName; }
	LPCWSTR GetPluginName() const { return m_pszPluginName; }
	LPCWSTR GetCopyright() const { return m_pszCopyright; }
	LPCWSTR GetDescription() const { return m_pszDescription; }
	LRESULT SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);
	bool Settings(HWND hwndOwner);
	bool HasSettings() const { return (m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)!=0; }
};

class CPluginList {
	CPointerVector<CPlugin> m_PluginList;
	void SortPluginsByName();
	static int CompareName(const CPlugin *pPlugin1,const CPlugin *pPlugin2,void *pParam);
	bool SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);
public:
	CPluginList();
	~CPluginList();
	bool LoadPlugins(LPCTSTR pszDirectory);
	void FreePlugins();
	int NumPlugins() const { return m_PluginList.Length(); }
	CPlugin *GetPlugin(int Index);
	const CPlugin *GetPlugin(int Index) const;
	bool EnablePlugins(bool fEnable=true);
	int FindPlugin(const CPlugin *pPlugin) const;
	bool DeletePlugin(int Index);
	bool SetMenu(HMENU hmenu) const;
	bool SendChannelChangeEvent();
	bool SendServiceChangeEvent();
	bool SendDriverChangeEvent();
	bool SendServiceUpdateEvent();
	bool SendRecordStatusChangeEvent();
	bool SendFullscreenChangeEvent(bool fFullscreen);
	bool SendPreviewChangeEvent(bool fPreview);
	bool SendVolumeChangeEvent(int Volume,bool fMute);
	bool SendStereoModeChangeEvent(int StereoMode);
	bool SendColorChangeEvent();
};

class CPluginOptions : public COptions {
	CPluginList *m_pPluginList;
	std::vector<LPTSTR> m_EnablePluginList;
	void ClearList();
	static CPluginOptions *GetThis(HWND hDlg);
public:
	CPluginOptions(CPluginList *pPluginList);
	~CPluginOptions();
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool RestorePluginOptions();
	bool StorePluginOptions();
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

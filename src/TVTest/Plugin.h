#ifndef PLUGIN_H
#define PLUGIN_H


#include <vector>
#include "TVTestPlugin.h"
#include "PointerArray.h"
#include "Options.h"
#include "MediaData.h"
#include "BonTsEngine/Exception.h"
#include "BonTsEngine/TsUtilClass.h"


class CPlugin : public CBonErrorHandler
{
	//static DWORD m_FinalizeTimeout;
	HMODULE m_hLib;
	CDynamicString m_FileName;
	TVTest::PluginParam m_PluginParam;
	DWORD m_Version;
	DWORD m_Type;
	DWORD m_Flags;
	CDynamicString m_PluginName;
	CDynamicString m_Copyright;
	CDynamicString m_Description;
	bool m_fEnabled;
	bool m_fSetting;
	int m_Command;
	TVTest::EventCallbackFunc m_pEventCallback;
	void *m_pEventCallbackClientData;
	TVTest::WindowMessageCallbackFunc m_pMessageCallback;
	void *m_pMessageCallbackClientData;
	class CPluginCommandInfo {
		int m_ID;
		LPWSTR m_pszText;
		LPWSTR m_pszName;
	public:
		CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName);
		CPluginCommandInfo(const TVTest::CommandInfo &Info);
		~CPluginCommandInfo();
		CPluginCommandInfo &operator=(const CPluginCommandInfo &Info);
		int GetID() const { return m_ID; }
		LPCWSTR GetText() const { return m_pszText; }
		LPCWSTR GetName() const { return m_pszName; }
	};
	CPointerVector<CPluginCommandInfo> m_CommandList;
	std::vector<CDynamicString> m_ControllerList;
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
	class CAudioStreamCallbackInfo {
	public:
		CPlugin *m_pPlugin;
		TVTest::AudioCallbackFunc m_pCallback;
		void *m_pClientData;
		CAudioStreamCallbackInfo(CPlugin *pPlugin,TVTest::AudioCallbackFunc pCallback,void *pClientData) {
			m_pPlugin=pPlugin;
			m_pCallback=pCallback;
			m_pClientData=pClientData;
		}
	};
	static CCriticalLock m_AudioStreamLock;
	static CPointerVector<CAudioStreamCallbackInfo> m_AudioStreamCallbackList;
	static void CALLBACK AudioStreamCallback(short *pData,DWORD Samples,int Channels,void *pParam);
	static LRESULT CALLBACK Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);
	//static DWORD WINAPI FinalizeThread(LPVOID lpParameter);

public:
	CPlugin();
	~CPlugin();
	bool Load(LPCTSTR pszFileName);
	void Free();
	bool IsLoaded() const { return m_hLib!=NULL; }
	bool IsEnabled() const { return m_fEnabled; }
	bool Enable(bool fEnable);
	HMODULE GetModuleHandle() { return m_hLib; }
	LPCTSTR GetFileName() const { return m_FileName.Get(); }
	LPCTSTR GetPluginName() const { return m_PluginName.GetSafe(); }
	LPCTSTR GetCopyright() const { return m_Copyright.GetSafe(); }
	LPCTSTR GetDescription() const { return m_Description.GetSafe(); }
	LRESULT SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);
	bool OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult);
	bool Settings(HWND hwndOwner);
	bool HasSettings() const { return (m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)!=0; }
	bool CanUnload() const { return (m_Flags&TVTest::PLUGIN_FLAG_NOUNLOAD)==0; }
	int GetCommand() const { return m_Command; }
	bool SetCommand(int Command);
	int NumPluginCommands() const;
	bool GetPluginCommandInfo(int Index,TVTest::CommandInfo *pInfo) const;
	bool NotifyCommand(LPCWSTR pszCommand);
	bool IsDisableOnStart() const;
	/*
	static bool SetFinalizeTimeout(DWORD Timeout);
	static DWORD GetFinalizeTimeout() { return m_FinalizeTimeout; }
	*/
};

class CPluginList
{
	CPointerVector<CPlugin> m_PluginList;
	void SortPluginsByName();
	static int CompareName(const CPlugin *pPlugin1,const CPlugin *pPlugin2,void *pParam);
	bool SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);

public:
	CPluginList();
	~CPluginList();
	bool LoadPlugins(LPCTSTR pszDirectory,const std::vector<LPCTSTR> *pExcludePlugins=NULL);
	void FreePlugins();
	int NumPlugins() const { return m_PluginList.Length(); }
	CPlugin *GetPlugin(int Index);
	const CPlugin *GetPlugin(int Index) const;
	bool EnablePlugins(bool fEnable=true);
	int FindPlugin(const CPlugin *pPlugin) const;
	int FindPluginByCommand(int Command) const;
	bool DeletePlugin(int Index);
	bool SetMenu(HMENU hmenu) const;
	bool OnPluginCommand(LPCTSTR pszCommand);
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
	bool SendStandbyEvent(bool fStandby);
	bool SendExecuteEvent(LPCTSTR pszCommandLine);
	bool SendResetEvent();
	bool SendStatusResetEvent();
	bool SendAudioStreamChangeEvent(int Stream);
	bool SendSettingsChangeEvent();
	bool SendCloseEvent();
	bool SendStartRecordEvent(const class CRecordManager *pRecordManager,LPTSTR pszFileName,int MaxFileName);
	bool SendRelayRecordEvent(LPCTSTR pszFileName);
	bool OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult);
};

class CPluginOptions : public COptions
{
	CPluginList *m_pPluginList;
	std::vector<LPTSTR> m_EnablePluginList;
	void ClearList();
	static CPluginOptions *GetThis(HWND hDlg);

public:
	CPluginOptions(CPluginList *pPluginList);
	~CPluginOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool RestorePluginOptions();
	bool StorePluginOptions();
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif

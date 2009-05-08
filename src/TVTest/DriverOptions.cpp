#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "DriverOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CDriverSettings {
	LPTSTR m_pszFileName;
	int m_InitialChannelType;
	int m_InitialSpace;
	int m_InitialChannel;
	bool m_fNoSignalLevel;
public:
	CDriverSettings();
	CDriverSettings(const CDriverSettings &Settings);
	~CDriverSettings();
	CDriverSettings &operator=(const CDriverSettings &Settings);
	LPCTSTR GetFileName() const { return m_pszFileName; }
	bool SetFileName(LPCTSTR pszFileName);
	enum {
		INITIALCHANNEL_NONE,
		INITIALCHANNEL_LAST,
		INITIALCHANNEL_CUSTOM
	};
	int GetInitialChannelType() const { return m_InitialChannelType; }
	bool SetInitialChannelType(int Type);
	int GetInitialSpace() const { return m_InitialSpace; }
	bool SetInitialSpace(int Space);
	int GetInitialChannel() const { return m_InitialChannel; }
	bool SetInitialChannel(int Channel);
	bool GetNoSignalLevel() const { return m_fNoSignalLevel; }
	void SetNoSignalLevel(bool fNoSignalLevel) { m_fNoSignalLevel=fNoSignalLevel; }
};


CDriverSettings::CDriverSettings()
	: m_pszFileName(NULL)
	, m_InitialChannelType(INITIALCHANNEL_LAST)
	, m_InitialSpace(0)
	, m_InitialChannel(0)
	, m_fNoSignalLevel(false)
{
}


CDriverSettings::CDriverSettings(const CDriverSettings &Settings)
{
	m_pszFileName=DuplicateString(Settings.m_pszFileName);
	m_InitialChannelType=Settings.m_InitialChannelType;
	m_InitialSpace=Settings.m_InitialSpace;
	m_InitialChannel=Settings.m_InitialChannel;
	m_fNoSignalLevel=Settings.m_fNoSignalLevel;
}


CDriverSettings::~CDriverSettings()
{
	delete [] m_pszFileName;
}


CDriverSettings &CDriverSettings::operator=(const CDriverSettings &Settings)
{
	if (&Settings!=this) {
		ReplaceString(&m_pszFileName,Settings.m_pszFileName);
		m_InitialChannelType=Settings.m_InitialChannelType;
		m_InitialSpace=Settings.m_InitialSpace;
		m_InitialChannel=Settings.m_InitialChannel;
		m_fNoSignalLevel=Settings.m_fNoSignalLevel;
	}
	return *this;
}


bool CDriverSettings::SetFileName(LPCTSTR pszFileName)
{
	return ReplaceString(&m_pszFileName,pszFileName);
}


bool CDriverSettings::SetInitialChannelType(int Type)
{
	if (Type<INITIALCHANNEL_NONE || Type>INITIALCHANNEL_CUSTOM)
		return false;
	m_InitialChannelType=Type;
	return true;
}


bool CDriverSettings::SetInitialSpace(int Space)
{
	m_InitialSpace=Space;
	return true;
}


bool CDriverSettings::SetInitialChannel(int Channel)
{
	m_InitialChannel=Channel;
	return true;
}




CDriverSettingList::CDriverSettingList()
{
}


CDriverSettingList::~CDriverSettingList()
{
	Clear();
}


CDriverSettingList &CDriverSettingList::operator=(const CDriverSettingList &List)
{
	if (&List!=this) {
		int NumDrivers=List.NumDrivers();

		m_SettingList.DeleteAll();
		m_SettingList.Reserve(NumDrivers);
		for (int i=0;i<NumDrivers;i++)
			m_SettingList.Add(new CDriverSettings(*List.m_SettingList[i]));
	}
	return *this;
}


void CDriverSettingList::Clear()
{
	m_SettingList.DeleteAll();
	m_SettingList.Clear();
}


bool CDriverSettingList::Add(CDriverSettings *pSettings)
{
	return m_SettingList.Add(pSettings);
}


CDriverSettings *CDriverSettingList::GetDriverSettings(int Index)
{
	return m_SettingList.Get(Index);
}


const CDriverSettings *CDriverSettingList::GetDriverSettings(int Index) const
{
	return m_SettingList.Get(Index);
}


int CDriverSettingList::Find(LPCTSTR pszFileName) const
{
	if (pszFileName==NULL)
		return -1;
	for (int i=0;i<m_SettingList.Length();i++) {
		if (::lstrcmpi(m_SettingList[i]->GetFileName(),pszFileName)==0)
			return i;
	}
	return -1;
}




CDriverOptions::CDriverOptions()
	: m_pDriverManager(NULL)
{
}


CDriverOptions::~CDriverOptions()
{
}


bool CDriverOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("DriverSettings"),CSettings::OPEN_READ)) {
		int NumDrivers;

		if (Settings.Read(TEXT("DriverCount"),&NumDrivers) && NumDrivers>0) {
			for (int i=0;i<NumDrivers;i++) {
				TCHAR szName[32],szFileName[MAX_PATH];

				::wsprintf(szName,TEXT("Driver%d_FileName"),i);
				if (Settings.Read(szName,szFileName,lengthof(szFileName))
						&& szFileName[0]!='\0') {
					CDriverSettings *pSettings=new CDriverSettings;
					int Value;

					pSettings->SetFileName(szFileName);
					::wsprintf(szName,TEXT("Driver%d_InitChannelType"),i);
					if (Settings.Read(szName,&Value))
						pSettings->SetInitialChannelType(Value);
					::wsprintf(szName,TEXT("Driver%d_InitSpace"),i);
					if (Settings.Read(szName,&Value))
						pSettings->SetInitialSpace(Value);
					::wsprintf(szName,TEXT("Driver%d_InitChannel"),i);
					if (Settings.Read(szName,&Value))
						pSettings->SetInitialChannel(Value);
					::wsprintf(szName,TEXT("Driver%d_Options"),i);
					if (Settings.Read(szName,&Value))
						pSettings->SetNoSignalLevel((Value&1)!=0);
					m_SettingList.Add(pSettings);
				}
			}
		}
	}
	return true;
}


bool CDriverOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("DriverSettings"),CSettings::OPEN_WRITE)) {
		int NumDrivers=m_SettingList.NumDrivers();

		Settings.Write(TEXT("DriverCount"),NumDrivers);
		for (int i=0;i<NumDrivers;i++) {
			const CDriverSettings *pSettings=m_SettingList.GetDriverSettings(i);
			TCHAR szName[32];

			::wsprintf(szName,TEXT("Driver%d_FileName"),i);
			Settings.Write(szName,pSettings->GetFileName());
			::wsprintf(szName,TEXT("Driver%d_InitChannelType"),i);
			Settings.Write(szName,pSettings->GetInitialChannelType());
			::wsprintf(szName,TEXT("Driver%d_InitSpace"),i);
			Settings.Write(szName,pSettings->GetInitialSpace());
			::wsprintf(szName,TEXT("Driver%d_InitChannel"),i);
			Settings.Write(szName,pSettings->GetInitialChannel());
			::wsprintf(szName,TEXT("Driver%d_Options"),i);
			Settings.Write(szName,pSettings->GetNoSignalLevel()?1:0);
		}
	}
	return true;
}


bool CDriverOptions::Initialize(CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	return true;
}


bool CDriverOptions::GetInitialChannel(LPCTSTR pszFileName,InitialChannelInfo *pChannelInfo) const
{
	int Index=m_SettingList.Find(pszFileName);

	if (Index>=0) {
		const CDriverSettings *pSettings=m_SettingList.GetDriverSettings(Index);

		switch (pSettings->GetInitialChannelType()) {
		case CDriverSettings::INITIALCHANNEL_NONE:
			pChannelInfo->Space=-1;
			pChannelInfo->Channel=-1;
			return true;
		case CDriverSettings::INITIALCHANNEL_CUSTOM:
			pChannelInfo->Space=pSettings->GetInitialSpace();
			pChannelInfo->Channel=pSettings->GetInitialChannel();
			return true;
		}
	}
	return false;
}


bool CDriverOptions::IsNoSignalLevel(LPCTSTR pszFileName) const
{
	int Index=m_SettingList.Find(pszFileName);

	if (Index<0)
		return false;
	return m_SettingList.GetDriverSettings(Index)->GetNoSignalLevel();
}


void CDriverOptions::InitDlgItem(int Driver)
{
	EnableDlgItems(m_hDlg,IDC_DRIVEROPTIONS_FIRST,IDC_DRIVEROPTIONS_LAST,Driver>=0);
	DlgComboBox_Clear(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE);
	DlgComboBox_Clear(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
	if (Driver>=0) {
		CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(Driver);
		LPCTSTR pszFileName=pDriverInfo->GetFileName();
		CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(DlgComboBox_GetItemData(m_hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Driver));

		int InitChannelType=pSettings->GetInitialChannelType();
		::CheckRadioButton(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_NONE,
								  IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM,
						IDC_DRIVEROPTIONS_INITCHANNEL_NONE+InitChannelType);
		EnableDlgItems(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
							  IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
					InitChannelType==CDriverSettings::INITIALCHANNEL_CUSTOM);
		bool fCur=::lstrcmpi(pszFileName,
				::PathFindFileName(GetAppClass().GetCoreEngine()->GetDriverFileName()))==0;
		if (fCur || pDriverInfo->LoadTuningSpaceList(
				!::PathMatchSpec(pszFileName,TEXT("BonDriver_Spinel*.dll")))) {
			const CTuningSpaceList *pTuningSpaceList;
			int i;

			if (fCur)
				pTuningSpaceList=GetAppClass().GetChannelManager()->GetDriverTuningSpaceList();
			else
				pTuningSpaceList=pDriverInfo->GetTuningSpaceList();
			for (i=0;i<pTuningSpaceList->NumSpaces();i++) {
				LPCTSTR pszName=pTuningSpaceList->GetTuningSpaceName(i);
				TCHAR szName[16];

				if (pszName==NULL) {
					::wsprintf(szName,TEXT("Space %d"),i+1);
					pszName=szName;
				}
				DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,pszName);
			}
			i=pSettings->GetInitialSpace();
			DlgComboBox_SetCurSel(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,i);
			if (i>=0) {
				const CChannelList *pChannelList;

				if (fCur)
					pChannelList=GetAppClass().GetChannelManager()->GetChannelList(i);
				else
					pChannelList=pTuningSpaceList->GetChannelList(i);
				if (pChannelList!=NULL) {
					int Sel=-1;

					DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,TEXT("Žw’è‚È‚µ"));
					for (i=0;i<pChannelList->NumChannels();i++) {
						DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
							pChannelList->GetName(i));
						DlgComboBox_SetItemData(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,i+1,pChannelList->GetChannelIndex(i));
						if (pChannelList->GetChannelIndex(i)==pSettings->GetInitialChannel())
							Sel=i;
					}
					DlgComboBox_SetCurSel(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,Sel+1);
				}
			}
		}
		bool fNetwork=GetAppClass().GetCoreEngine()->IsNetworkDriverFileName(pszFileName);
		::EnableDlgItem(m_hDlg,IDC_DRIVEROPTIONS_NOSIGNALLEVEL,!fNetwork);
		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_NOSIGNALLEVEL,
						  fNetwork?true:pSettings->GetNoSignalLevel());
	}
}


CDriverOptions *CDriverOptions::GetThis(HWND hDlg)
{
	return static_cast<CDriverOptions*>(GetOptions(hDlg));
}


BOOL CALLBACK CDriverOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CDriverOptions *pThis=static_cast<CDriverOptions*>(OnInitDialog(hDlg,lParam));

			if (pThis->m_pDriverManager!=NULL
					&& pThis->m_pDriverManager->NumDrivers()>0) {
				int CurDriver=0;

				pThis->m_CurSettingList=pThis->m_SettingList;
				for (int i=0;i<pThis->m_pDriverManager->NumDrivers();i++) {
					LPCTSTR pszFileName=pThis->m_pDriverManager->GetDriverInfo(i)->GetFileName();
					CDriverSettings *pSettings;

					DlgComboBox_AddString(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,pszFileName);
					int Index=pThis->m_CurSettingList.Find(pszFileName);
					if (Index<0) {
						pSettings=new CDriverSettings;
						pSettings->SetFileName(pszFileName);
						pThis->m_CurSettingList.Add(pSettings);
					} else {
						pSettings=pThis->m_CurSettingList.GetDriverSettings(Index);
					}
					DlgComboBox_SetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,i,(LPARAM)pSettings);
				}
				LPCTSTR pszCurDriverName=GetAppClass().GetCoreEngine()->GetDriverFileName();
				if (pszCurDriverName[0]!='\0') {
					CurDriver=DlgComboBox_FindStringExact(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,
						-1,::PathFindFileName(pszCurDriverName));
					if (CurDriver<0)
						CurDriver=0;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,CurDriver);
				pThis->InitDlgItem(CurDriver);
			} else {
				EnableDlgItems(hDlg,IDC_DRIVEROPTIONS_FIRST,IDC_DRIVEROPTIONS_LAST,false);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DRIVEROPTIONS_DRIVERLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CDriverOptions *pThis=GetThis(hDlg);

				pThis->InitDlgItem(DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST));
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_NONE:
		case IDC_DRIVEROPTIONS_INITCHANNEL_LAST:
		case IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM:
			{
				CDriverOptions *pThis=GetThis(hDlg);
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

				if (Sel>=0) {
					CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));

					pSettings->SetInitialChannelType(LOWORD(wParam)-IDC_DRIVEROPTIONS_INITCHANNEL_NONE);
					EnableDlgItems(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
						IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
						pSettings->GetInitialChannelType()==CDriverSettings::INITIALCHANNEL_CUSTOM);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_SPACE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CDriverOptions *pThis=GetThis(hDlg);
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

				if (Sel>=0) {
					int Space=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE);
					CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));
					const CChannelList *pChannelList;

					pSettings->SetInitialSpace(Space);
					if (::lstrcmpi(pSettings->GetFileName(),
							::PathFindFileName(GetAppClass().GetCoreEngine()->GetDriverFileName()))==0) {
						pChannelList=GetAppClass().GetChannelManager()->GetChannelList(Space);
					} else {
						const CTuningSpaceList *pTuningSpaceList=pThis->m_pDriverManager->GetDriverInfo(Sel)->GetTuningSpaceList();
						pChannelList=pTuningSpaceList->GetChannelList(Space);
					}
					DlgComboBox_Clear(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
					if (pChannelList!=NULL) {
						DlgComboBox_AddString(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,TEXT("Žw’è‚È‚µ"));
						for (int i=0;i<pChannelList->NumChannels();i++) {
							DlgComboBox_AddString(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,pChannelList->GetName(i));
						}
						DlgComboBox_SetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,0);
					}
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CDriverOptions *pThis=GetThis(hDlg);
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

				if (Sel>=0) {
					CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));
					int Channel=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL)-1;

					if (Channel>=0)
						Channel=DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,Channel+1);
					pSettings->SetInitialChannel(Channel);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_NOSIGNALLEVEL:
			{
				CDriverOptions *pThis=GetThis(hDlg);
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

				if (Sel>=0) {
					CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));

					pSettings->SetNoSignalLevel(DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_NOSIGNALLEVEL));
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CDriverOptions *pThis=GetThis(hDlg);

				pThis->m_SettingList=pThis->m_CurSettingList;
			}
			break;

		case PSN_RESET:
			{
				CDriverOptions *pThis=GetThis(hDlg);

				pThis->m_CurSettingList.Clear();
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CDriverOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}

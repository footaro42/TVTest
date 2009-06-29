#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AudioOptions.h"
#include "DirectShowUtil.h"
#include "DialogUtil.h"
#include "resource.h"




CAudioOptions::CAudioOptions()
{
	m_szAudioDeviceName[0]='\0';
	m_fDownMixSurround=true;
	m_fRestoreMute=false;
	m_fUseAudioRendererClock=true;
	m_fAdjustAudioStreamTime=false;
	m_fMinTimerResolution=true;
}


CAudioOptions::~CAudioOptions()
{
}


bool CAudioOptions::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("AudioDevice"),m_szAudioDeviceName,MAX_AUDIO_DEVICE_NAME);
	pSettings->Read(TEXT("DownMixSurround"),&m_fDownMixSurround);
	pSettings->Read(TEXT("RestoreMute"),&m_fRestoreMute);
	pSettings->Read(TEXT("UseAudioRendererClock"),&m_fUseAudioRendererClock);
	pSettings->Read(TEXT("AdjustAudioStreamTime"),&m_fAdjustAudioStreamTime);
	pSettings->Read(TEXT("MinTimerResolution"),&m_fMinTimerResolution);
	return true;
}


bool CAudioOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("AudioDevice"),m_szAudioDeviceName);
	pSettings->Write(TEXT("DownMixSurround"),m_fDownMixSurround);
	pSettings->Write(TEXT("RestoreMute"),m_fRestoreMute);
	pSettings->Write(TEXT("UseAudioRendererClock"),m_fUseAudioRendererClock);
	pSettings->Write(TEXT("AdjustAudioStreamTime"),m_fAdjustAudioStreamTime);
	pSettings->Write(TEXT("MinTimerResolution"),m_fMinTimerResolution);
	return true;
}


CAudioOptions *CAudioOptions::GetThis(HWND hDlg)
{
	return static_cast<CAudioOptions*>(GetOptions(hDlg));
}


BOOL CALLBACK CAudioOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAudioOptions *pThis=static_cast<CAudioOptions*>(OnInitDialog(hDlg,lParam));

			int Sel=0;
			DlgComboBox_AddString(hDlg,IDC_AUDIOOPTIONS_DEVICE,TEXT("デフォルト"));
			CDirectShowDeviceEnumerator DevEnum;
			if (DevEnum.EnumDevice(CLSID_AudioRendererCategory)) {
				for (int i=0;i<DevEnum.GetDeviceCount();i++) {
					LPCTSTR pszName=DevEnum.GetDeviceFriendlyName(i);
					DlgComboBox_AddString(hDlg,IDC_AUDIOOPTIONS_DEVICE,pszName);
					if (::lstrcmpi(pszName,pThis->m_szAudioDeviceName)==0)
						Sel=i+1;
				}
			}
			DlgComboBox_SetCurSel(hDlg,IDC_AUDIOOPTIONS_DEVICE,Sel);

			DlgCheckBox_Check(hDlg,IDC_AUDIOOPTIONS_DOWNMIXSURROUND,pThis->m_fDownMixSurround);
			DlgCheckBox_Check(hDlg,IDC_AUDIOOPTIONS_RESTOREMUTE,pThis->m_fRestoreMute);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINTIMERRESOLUTION,pThis->m_fMinTimerResolution);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ADJUSTAUDIOSTREAMTIME,pThis->m_fAdjustAudioStreamTime);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_USEDEMUXERCLOCK,!pThis->m_fUseAudioRendererClock);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CAudioOptions *pThis=GetThis(hDlg);

				TCHAR szAudioDevice[MAX_AUDIO_DEVICE_NAME];
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_AUDIOOPTIONS_DEVICE);
				if (Sel<=0)
					szAudioDevice[0]='\0';
				else
					DlgComboBox_GetLBString(hDlg,IDC_AUDIOOPTIONS_DEVICE,Sel,szAudioDevice);
				if (::lstrcmpi(pThis->m_szAudioDeviceName,szAudioDevice)!=0) {
					::lstrcpy(pThis->m_szAudioDeviceName,szAudioDevice);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				pThis->m_fDownMixSurround=DlgCheckBox_IsChecked(hDlg,IDC_AUDIOOPTIONS_DOWNMIXSURROUND);
				GetAppClass().GetCoreEngine()->SetDownMixSurround(pThis->m_fDownMixSurround);
				pThis->m_fRestoreMute=DlgCheckBox_IsChecked(hDlg,IDC_AUDIOOPTIONS_RESTOREMUTE);

				bool f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINTIMERRESOLUTION);
				if (f!=pThis->m_fMinTimerResolution) {
					pThis->m_fMinTimerResolution=f;
					if (GetAppClass().GetMainWindow()->IsPreview())
						GetAppClass().GetCoreEngine()->SetMinTimerResolution(f);
				}

				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ADJUSTAUDIOSTREAMTIME);
				if (f!=pThis->m_fAdjustAudioStreamTime) {
					pThis->m_fAdjustAudioStreamTime=f;
					GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetAdjustAudioStreamTime(f);
				}

				f=!DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEDEMUXERCLOCK);
				if (f!=pThis->m_fUseAudioRendererClock) {
					pThis->m_fUseAudioRendererClock=f;
					GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(f);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CAudioOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}

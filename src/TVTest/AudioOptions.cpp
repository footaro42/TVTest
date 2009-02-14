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
}


CAudioOptions::~CAudioOptions()
{
}


bool CAudioOptions::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("AudioDevice"),m_szAudioDeviceName,MAX_AUDIO_DEVICE_NAME);
	pSettings->Read(TEXT("DownMixSurround"),&m_fDownMixSurround);
	pSettings->Read(TEXT("RestoreMute"),&m_fRestoreMute);
	return true;
}


bool CAudioOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("AudioDevice"),m_szAudioDeviceName);
	pSettings->Write(TEXT("DownMixSurround"),m_fDownMixSurround);
	pSettings->Write(TEXT("RestoreMute"),m_fRestoreMute);
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
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CAudioOptions *pThis=GetThis(hDlg);

				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_AUDIOOPTIONS_DEVICE);
				if (Sel<=0)
					pThis->m_szAudioDeviceName[0]='\0';
				else
					DlgComboBox_GetLBString(hDlg,IDC_AUDIOOPTIONS_DEVICE,Sel,
											pThis->m_szAudioDeviceName);

				pThis->m_fDownMixSurround=DlgCheckBox_IsChecked(hDlg,IDC_AUDIOOPTIONS_DOWNMIXSURROUND);
				GetAppClass().GetCoreEngine()->SetDownMixSurround(pThis->m_fDownMixSurround);
				pThis->m_fRestoreMute=DlgCheckBox_IsChecked(hDlg,IDC_AUDIOOPTIONS_RESTOREMUTE);
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

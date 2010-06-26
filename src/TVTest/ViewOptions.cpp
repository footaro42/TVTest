#include "stdafx.h"
#include "TVTest.h"
#include "MainWindow.h"
#include "ViewOptions.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CViewOptions::CViewOptions()
	: m_fAdjustAspectResizing(false)
	, m_fSnapAtWindowEdge(false)
	, m_SnapAtWindowEdgeMargin(8)
	, m_fNearCornerResizeOrigin(false)
	, m_fPanScanNoResizeWindow(true)
	, m_fResetPanScanEventChange(true)
	, m_fNoMaskSideCut(true)
	, m_FullscreenStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_MaximizeStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_fClientEdge(true)
	, m_fMinimizeToTray(false)
	, m_fDisablePreviewWhenMinimized(false)
	, m_fRestorePlayStatus(false)
	, m_fIgnoreDisplayExtension(false)
	, m_fNoScreenSaver(false)
	, m_fNoMonitorLowPower(false)
	, m_fNoMonitorLowPowerActiveOnly(false)
	, m_fShowLogo(true)
{
	::lstrcpy(m_szLogoFileName,APP_NAME TEXT("_Logo.bmp"));
}


bool CViewOptions::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();

	if ((Flags&UPDATE_MASKCUTAREA)!=0) {
		AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetNoMaskSideCut(m_fNoMaskSideCut);
	}

	if ((Flags&UPDATE_IGNOREDISPLAYEXTENSION)!=0) {
		AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetIgnoreDisplayExtension(m_fIgnoreDisplayExtension);
	}

	if ((Flags&UPDATE_LOGO)!=0) {
		AppMain.GetMainWindow()->SetLogo(m_fShowLogo?m_szLogoFileName:NULL);
	}

	return true;
}


bool CViewOptions::Read(CSettings *pSettings)
{
	int Value;

	pSettings->Read(TEXT("AdjustAspectResizing"),&m_fAdjustAspectResizing);
	pSettings->Read(TEXT("SnapToWindowEdge"),&m_fSnapAtWindowEdge);
	pSettings->Read(TEXT("NearCornerResizeOrigin"),&m_fNearCornerResizeOrigin);
	pSettings->Read(TEXT("PanScanNoResizeWindow"),&m_fPanScanNoResizeWindow);
	pSettings->Read(TEXT("ResetPanScanEventChange"),&m_fResetPanScanEventChange);
	pSettings->Read(TEXT("NoMaskSideCut"),&m_fNoMaskSideCut);
	if (pSettings->Read(TEXT("FullscreenStretchMode"),&Value))
		m_FullscreenStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
										 CMediaViewer::STRETCH_KEEPASPECTRATIO;
	if (pSettings->Read(TEXT("MaximizeStretchMode"),&Value))
		m_MaximizeStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
									   CMediaViewer::STRETCH_KEEPASPECTRATIO;
	pSettings->Read(TEXT("ClientEdge"),&m_fClientEdge);
	pSettings->Read(TEXT("MinimizeToTray"),&m_fMinimizeToTray);
	pSettings->Read(TEXT("DisablePreviewWhenMinimized"),&m_fDisablePreviewWhenMinimized);
	pSettings->Read(TEXT("RestorePlayStatus"),&m_fRestorePlayStatus);
	pSettings->Read(TEXT("IgnoreDisplayExtension"),&m_fIgnoreDisplayExtension);
	pSettings->Read(TEXT("NoScreenSaver"),&m_fNoScreenSaver);
	pSettings->Read(TEXT("NoMonitorLowPower"),&m_fNoMonitorLowPower);
	pSettings->Read(TEXT("NoMonitorLowPowerActiveOnly"),&m_fNoMonitorLowPowerActiveOnly);
	pSettings->Read(TEXT("ShowLogo"),&m_fShowLogo);
	pSettings->Read(TEXT("LogoFileName"),m_szLogoFileName,lengthof(m_szLogoFileName));
	return true;
}


bool CViewOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("AdjustAspectResizing"),m_fAdjustAspectResizing);
	pSettings->Write(TEXT("SnapToWindowEdge"),m_fSnapAtWindowEdge);
	pSettings->Write(TEXT("NearCornerResizeOrigin"),m_fNearCornerResizeOrigin);
	pSettings->Write(TEXT("PanScanNoResizeWindow"),m_fPanScanNoResizeWindow);
	pSettings->Write(TEXT("ResetPanScanEventChange"),m_fResetPanScanEventChange);
	pSettings->Write(TEXT("NoMaskSideCut"),m_fNoMaskSideCut);
	pSettings->Write(TEXT("FullscreenStretchMode"),(int)m_FullscreenStretchMode);
	pSettings->Write(TEXT("MaximizeStretchMode"),(int)m_MaximizeStretchMode);
	pSettings->Write(TEXT("ClientEdge"),m_fClientEdge);
	pSettings->Write(TEXT("MinimizeToTray"),m_fMinimizeToTray);
	pSettings->Write(TEXT("DisablePreviewWhenMinimized"),m_fDisablePreviewWhenMinimized);
	pSettings->Write(TEXT("RestorePlayStatus"),m_fRestorePlayStatus);
	pSettings->Write(TEXT("IgnoreDisplayExtension"),m_fIgnoreDisplayExtension);
	pSettings->Write(TEXT("NoScreenSaver"),m_fNoScreenSaver);
	pSettings->Write(TEXT("NoMonitorLowPower"),m_fNoMonitorLowPower);
	pSettings->Write(TEXT("NoMonitorLowPowerActiveOnly"),m_fNoMonitorLowPowerActiveOnly);
	pSettings->Write(TEXT("ShowLogo"),m_fShowLogo);
	pSettings->Write(TEXT("LogoFileName"),m_szLogoFileName);
	return true;
}


CViewOptions *CViewOptions::GetThis(HWND hDlg)
{
	return static_cast<CViewOptions*>(GetOptions(hDlg));
}


INT_PTR CALLBACK CViewOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CViewOptions *pThis=static_cast<CViewOptions*>(OnInitDialog(hDlg,lParam));

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ADJUSTASPECTRESIZING,
							  pThis->m_fAdjustAspectResizing);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SNAPATWINDOWEDGE,
							  pThis->m_fSnapAtWindowEdge);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NEARCORNERRESIZEORIGIN,
							  pThis->m_fNearCornerResizeOrigin);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_PANSCANNORESIZEWINDOW,
							  pThis->m_fPanScanNoResizeWindow);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE,
							  pThis->m_fResetPanScanEventChange);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMASKSIDECUT,
							  pThis->m_fNoMaskSideCut);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME,
				pThis->m_FullscreenStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME,
				pThis->m_MaximizeStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_CLIENTEDGE,pThis->m_fClientEdge);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINIMIZETOTRAY,pThis->m_fMinimizeToTray);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINIMIZEDISABLEPREVIEW,
							  pThis->m_fDisablePreviewWhenMinimized);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESTOREPLAYSTATUS,
							  pThis->m_fRestorePlayStatus);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE,
							  pThis->m_fIgnoreDisplayExtension);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOSCREENSAVER,pThis->m_fNoScreenSaver);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER,pThis->m_fNoMonitorLowPower);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
							  pThis->m_fNoMonitorLowPowerActiveOnly);
			EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,pThis->m_fNoMonitorLowPower);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SHOWLOGO,pThis->m_fShowLogo);
			::SetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,pThis->m_szLogoFileName);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_LOGOFILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::EnableDlgItems(hDlg,IDC_OPTIONS_LOGOFILENAME,IDC_OPTIONS_LOGOFILENAME_BROWSE,
							 pThis->m_fShowLogo);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_NOMONITORLOWPOWER:
			EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
				DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER));
			return TRUE;

		case IDC_OPTIONS_SHOWLOGO:
			::EnableDlgItems(hDlg,IDC_OPTIONS_LOGOFILENAME,IDC_OPTIONS_LOGOFILENAME_BROWSE,
				DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SHOWLOGO));
			return TRUE;

		case IDC_OPTIONS_LOGOFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BMPファイル(*.bmp)\0*.bmp\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("ロゴ画像の選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CViewOptions *pThis=GetThis(hDlg);
				CAppMain &AppMain=GetAppClass();
				bool f;

				pThis->m_fAdjustAspectResizing=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ADJUSTASPECTRESIZING);
				pThis->m_fSnapAtWindowEdge=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SNAPATWINDOWEDGE);
				pThis->m_fNearCornerResizeOrigin=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NEARCORNERRESIZEORIGIN);
				pThis->m_fPanScanNoResizeWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_PANSCANNORESIZEWINDOW);
				pThis->m_fResetPanScanEventChange=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMASKSIDECUT);
				if (pThis->m_fNoMaskSideCut!=f) {
					pThis->m_fNoMaskSideCut=f;
					pThis->SetUpdateFlag(UPDATE_MASKCUTAREA);
				}
				pThis->m_FullscreenStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME)?
					CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (AppMain.GetUICore()->GetFullscreen())
					AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetViewStretchMode(pThis->m_FullscreenStretchMode);
				pThis->m_MaximizeStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME)?
					CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (AppMain.GetMainWindow()->GetMaximize())
					AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetViewStretchMode(pThis->m_MaximizeStretchMode);
				{
					bool fEdge=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_CLIENTEDGE);
					if (fEdge!=pThis->m_fClientEdge) {
						pThis->m_fClientEdge=fEdge;
						AppMain.GetMainWindow()->SetViewWindowEdge(fEdge);
					}
				}
				pThis->m_fMinimizeToTray=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINIMIZETOTRAY);
				pThis->m_fDisablePreviewWhenMinimized=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINIMIZEDISABLEPREVIEW);
				pThis->m_fRestorePlayStatus=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESTOREPLAYSTATUS);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE);
				if (pThis->m_fIgnoreDisplayExtension!=f) {
					pThis->m_fIgnoreDisplayExtension=f;
					pThis->SetUpdateFlag(UPDATE_IGNOREDISPLAYEXTENSION);
				}
				pThis->m_fNoScreenSaver=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOSCREENSAVER);
				pThis->m_fNoMonitorLowPower=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER);
				pThis->m_fNoMonitorLowPowerActiveOnly=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY);
				AppMain.GetUICore()->PreventDisplaySave(true);
				{
					bool fLogo=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SHOWLOGO);
					TCHAR szFileName[MAX_PATH];

					::GetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName,MAX_PATH);
					if (fLogo!=pThis->m_fShowLogo
							|| ::lstrcmp(szFileName,pThis->m_szLogoFileName)!=0) {
						pThis->m_fShowLogo=fLogo;
						::lstrcpy(pThis->m_szLogoFileName,szFileName);
						pThis->SetUpdateFlag(UPDATE_LOGO);
					}
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CViewOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}

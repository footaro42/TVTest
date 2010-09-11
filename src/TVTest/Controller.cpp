#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Controller.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CController::CController()
	: m_pEventHandler(NULL)
{
}


CController::~CController()
{
}


bool CController::TranslateMessage(HWND hwnd,MSG *pMessage)
{
	return false;
}


HBITMAP CController::GetImage(ImageType Type) const
{
	return NULL;
}


bool CController::GetIniFileName(LPTSTR pszFileName,int MaxLength) const
{
	LPCTSTR pszIniFileName=GetAppClass().GetIniFileName();

	if (::lstrlen(pszIniFileName)>=MaxLength)
		return false;
	::lstrcpy(pszFileName,pszIniFileName);
	return true;
}


LPCTSTR CController::GetIniFileSection() const
{
	return GetName();
}


void CController::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CController::OnButtonDown(int Index)
{
	if (m_pEventHandler==NULL || Index<0 || Index>=NumButtons())
		return false;
	return m_pEventHandler->OnButtonDown(this,Index);
}




CControllerManager::CControllerManager()
	: m_fFocus(false)
	, m_fActive(false)
	, m_hbmController(NULL)
	, m_hbmSelButtons(NULL)
{
}


CControllerManager::~CControllerManager()
{
	DeleteAllControllers();
}


bool CControllerManager::Read(CSettings *pSettings)
{
	TCHAR szText[256];

	if (pSettings->Read(TEXT("CurController"),szText,lengthof(szText)))
		m_CurController.Set(szText);
	return true;
}


bool CControllerManager::Write(CSettings *pSettings) const
{
	if (!m_CurController.IsEmpty())
		pSettings->Write(TEXT("CurController"),m_CurController.Get());
	return true;
}


bool CControllerManager::AddController(CController *pController)
{
	if (pController==NULL || FindController(pController->GetName())>=0)
		return false;
	m_ControllerList.push_back(ControllerInfo(pController));
	pController->SetEventHandler(this);
	ControllerInfo &Info=m_ControllerList[m_ControllerList.size()-1];
	int NumButtons=pController->NumButtons();
	Info.Settings.AssignList.resize(NumButtons);
	Info.Settings.fActiveOnly=pController->IsActiveOnly();
	for (int i=0;i<NumButtons;i++) {
		CController::ButtonInfo Button;

		if (pController->GetButtonInfo(i,&Button)) {
			Info.Settings.AssignList[i]=(WORD)Button.DefaultCommand;
		}
	}
	return true;
}


bool CControllerManager::DeleteController(LPCTSTR pszName)
{
	if (pszName==NULL)
		return false;
	for (std::vector<ControllerInfo>::iterator itr=m_ControllerList.begin();
		 	itr!=m_ControllerList.end();itr++) {
		if (::lstrcmpi(itr->pController->GetName(),pszName)==0) {
			if (itr->fSettingsLoaded)
				SaveControllerSettings(pszName);
			delete itr->pController;
			m_ControllerList.erase(itr);
			return true;
		}
	}
	return false;
}


void CControllerManager::DeleteAllControllers()
{
	for (size_t i=0;i<m_ControllerList.size();i++) {
		if (m_ControllerList[i].fSettingsLoaded)
			SaveControllerSettings(m_ControllerList[i].pController->GetName());
	}
	for (size_t i=0;i<m_ControllerList.size();i++)
		delete m_ControllerList[i].pController;
	m_ControllerList.clear();
}


bool CControllerManager::IsControllerEnabled(LPCTSTR pszName) const
{
	int Index=FindController(pszName);
	if (Index<0)
		return false;
	return m_ControllerList[Index].pController->IsEnabled();
}


bool CControllerManager::LoadControllerSettings(LPCTSTR pszName)
{
	int Index=FindController(pszName);
	if (Index<0)
		return false;

	ControllerInfo &Info=m_ControllerList[Index];

	if (Info.fSettingsLoaded)
		return true;

	CSettings Settings;
	TCHAR szFileName[MAX_PATH];

	if (!Info.pController->GetIniFileName(szFileName,lengthof(szFileName)))
		return false;
	if (Settings.Open(szFileName,Info.pController->GetIniFileSection(),CSettings::OPEN_READ)) {
		const int NumButtons=Info.pController->NumButtons();
		const CCommandList *pCommandList=GetAppClass().GetCommandList();

		for (int i=0;i<NumButtons;i++) {
			TCHAR szName[64],szCommand[CCommandList::MAX_COMMAND_TEXT];

			::wsprintf(szName,TEXT("Button%d_Command"),i);
			if (Settings.Read(szName,szCommand,lengthof(szCommand))
					&& szCommand[0]!='\0') {
				Info.Settings.AssignList[i]=pCommandList->ParseText(szCommand);
			}
		}
		if (!Info.pController->IsActiveOnly())
			Settings.Read(TEXT("ActiveOnly"),&Info.Settings.fActiveOnly);
		Settings.Close();
		Info.fSettingsLoaded=true;
	}
	return true;
}


bool CControllerManager::SaveControllerSettings(LPCTSTR pszName) const
{
	int Index=FindController(pszName);
	if (Index<0)
		return false;

	const ControllerInfo &Info=m_ControllerList[Index];
	if (!Info.fSettingsLoaded)
		return true;

	CSettings Settings;
	TCHAR szFileName[MAX_PATH];

	if (!Info.pController->GetIniFileName(szFileName,lengthof(szFileName)))
		return false;
	if (Settings.Open(szFileName,Info.pController->GetIniFileSection(),CSettings::OPEN_WRITE)) {
		const int NumButtons=Info.pController->NumButtons();
		const CCommandList *pCommandList=GetAppClass().GetCommandList();

		for (int i=0;i<NumButtons;i++) {
			TCHAR szName[64];
			LPCTSTR pszText=NULL;

			::wsprintf(szName,TEXT("Button%d_Command"),i);
			if (Info.Settings.AssignList[i]!=0)
				pszText=pCommandList->GetCommandTextByID(Info.Settings.AssignList[i]);
			Settings.Write(szName,pszText!=NULL?pszText:TEXT(""));
		}
		if (!Info.pController->IsActiveOnly())
			Settings.Write(TEXT("ActiveOnly"),Info.Settings.fActiveOnly);
		Settings.Close();
	}
	return true;
}


bool CControllerManager::TranslateMessage(HWND hwnd,MSG *pMessage)
{
	for (size_t i=0;i<m_ControllerList.size();i++) {
		if (m_ControllerList[i].pController->IsEnabled()) {
			if (m_ControllerList[i].pController->TranslateMessage(hwnd,pMessage))
				return true;
		}
	}
	return false;
}


bool CControllerManager::OnActiveChange(HWND hwnd,bool fActive)
{
	m_fActive=fActive;
	if (fActive)
		OnFocusChange(hwnd,true);
	return true;
}


bool CControllerManager::OnFocusChange(HWND hwnd,bool fFocus)
{
	m_fFocus=fFocus;
	if (fFocus) {
		for (size_t i=0;i<m_ControllerList.size();i++)
			m_ControllerList[i].pController->SetTargetWindow(hwnd);
	}
	return true;
}


bool CControllerManager::OnButtonDown(LPCTSTR pszName,int Button) const
{
	if (pszName==NULL || Button<0)
		return false;

	int Index=FindController(pszName);
	if (Index<0)
		return false;
	const ControllerInfo &Info=m_ControllerList[Index];
	if (!m_fActive && Info.Settings.fActiveOnly)
		return false;
	if (Button>=(int)Info.Settings.AssignList.size())
		return false;
	const WORD Command=Info.Settings.AssignList[Button];
	if (Command!=0)
		::PostMessage(GetAppClass().GetUICore()->GetMainWindow(),
					  WM_COMMAND,Command,0);
	return true;
}


bool CControllerManager::OnButtonDown(CController *pController,int Index)
{
	if (Index<0)
		return false;

	for (size_t i=0;i<m_ControllerList.size();i++) {
		const ControllerInfo &Info=m_ControllerList[Index];

		if (Info.pController==pController) {
			if (Index>=(int)Info.Settings.AssignList.size())
				return false;
			const WORD Command=Info.Settings.AssignList[Index];
			if (Command!=0)
				::PostMessage(GetAppClass().GetUICore()->GetMainWindow(),
							  WM_COMMAND,Command,0);
			return true;
		}
	}
	return false;
}


const CControllerManager::ControllerSettings *CControllerManager::GetControllerSettings(LPCTSTR pszName) const
{
	int Index=FindController(pszName);

	if (Index<0)
		return NULL;
	return &m_ControllerList[Index].Settings;
}


int CControllerManager::FindController(LPCTSTR pszName) const
{
	if (pszName==NULL)
		return -1;
	for (size_t i=0;i<m_ControllerList.size();i++) {
		if (::lstrcmpi(m_ControllerList[i].pController->GetName(),pszName)==0)
			return (int)i;
	}
	return -1;
}


void CControllerManager::InitDlgItems()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CONTROLLER_ASSIGN);
	ListView_DeleteAllItems(hwndList);

	if (m_hbmController!=NULL) {
		::DeleteObject(m_hbmController);
		m_hbmController=NULL;
	}
	if (m_hbmSelButtons!=NULL) {
		::DeleteObject(m_hbmSelButtons);
		m_hbmSelButtons=NULL;
	}
	m_Tooltip.DeleteAllTools();

	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_CONTROLLER_LIST);
	if (Sel>=0) {
		const CCommandList *pCommandList=GetAppClass().GetCommandList();
		const ControllerInfo &Info=m_ControllerList[Sel];
		const CController *pController=Info.pController;
		const int NumButtons=pController->NumButtons();

		if (!Info.fSettingsLoaded) {
			LoadControllerSettings(pController->GetName());
			m_CurSettingsList[Sel]=Info.Settings;
		}

		bool fActiveOnly=pController->IsActiveOnly();
		EnableDlgItem(m_hDlg,IDC_CONTROLLER_ACTIVEONLY,!fActiveOnly);
		DlgCheckBox_Check(m_hDlg,IDC_CONTROLLER_ACTIVEONLY,
						  fActiveOnly || m_CurSettingsList[Sel].fActiveOnly);

		for (int i=0;i<NumButtons;i++) {
			CController::ButtonInfo Button;
			int Command=m_CurSettingsList[Sel].AssignList[i];
			LV_ITEM lvi;

			pController->GetButtonInfo(i,&Button);
			lvi.mask=LVIF_TEXT | LVIF_PARAM;
			lvi.iItem=i;
			lvi.iSubItem=0;
			lvi.pszText=const_cast<LPTSTR>(Button.pszName);
			lvi.lParam=Command;
			lvi.iItem=ListView_InsertItem(hwndList,&lvi);
			if (Command>0) {
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];

				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				pCommandList->GetCommandNameByID(Command,szText,lengthof(szText));
				lvi.pszText=szText;
				ListView_SetItem(hwndList,&lvi);
			}
		}
		for (int i=0;i<2;i++)
			ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);

		m_hbmController=pController->GetImage(CController::IMAGE_CONTROLLER);
		if (m_hbmController!=NULL) {
			RECT rc;

			BITMAP bm;
			::GetObject(m_hbmController,sizeof(BITMAP),&bm);
			::SetRect(&rc,172,8,232,216);
			::MapDialogRect(m_hDlg,&rc);
			m_ImageRect.left=rc.left+((rc.right-rc.left)-bm.bmWidth)/2;
			m_ImageRect.top=rc.top+((rc.bottom-rc.top)-bm.bmHeight)/2;
			m_ImageRect.right=m_ImageRect.left+bm.bmWidth;
			m_ImageRect.bottom=m_ImageRect.top+bm.bmHeight;

			m_hbmSelButtons=pController->GetImage(CController::IMAGE_SELBUTTONS);

			if (m_hbmSelButtons!=NULL) {
				for (int i=0;i<NumButtons;i++) {
					CController::ButtonInfo Button;

					pController->GetButtonInfo(i,&Button);
					rc.left=m_ImageRect.left+Button.ImageButtonRect.Left;
					rc.top=m_ImageRect.top+Button.ImageButtonRect.Top;
					rc.right=rc.left+Button.ImageButtonRect.Width;
					rc.bottom=rc.top+Button.ImageButtonRect.Height;
					m_Tooltip.AddTool(i,rc,const_cast<LPTSTR>(Button.pszName));
				}
			}
		}
	}
	EnableDlgItems(m_hDlg,IDC_CONTROLLER_ACTIVEONLY,IDC_CONTROLLER_DEFAULT,Sel>=0);
	DlgComboBox_SetCurSel(m_hDlg,IDC_CONTROLLER_COMMAND,0);
	EnableDlgItem(m_hDlg,IDC_CONTROLLER_COMMAND,false);
	::InvalidateRect(m_hDlg,NULL,TRUE);
}


void CControllerManager::SetButtonCommand(HWND hwndList,int Index,int Command)
{
	int CurController=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_CONTROLLER_LIST);
	if (CurController<0)
		return;

	LV_ITEM lvi;
	TCHAR szText[CCommandList::MAX_COMMAND_NAME];

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.lParam=Command;
	ListView_SetItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	if (Command>0) {
		GetAppClass().GetCommandList()->GetCommandNameByID(Command,szText,lengthof(szText));
		lvi.pszText=szText;
	} else {
		lvi.pszText=TEXT("");
	}
	ListView_SetItem(hwndList,&lvi);
	m_CurSettingsList[CurController].AssignList[Index]=(WORD)Command;
}


void CControllerManager::SetDlgItemStatus()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CONTROLLER_ASSIGN);
	int Sel;

	Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (Sel>=0) {
		LV_ITEM lvi;

		lvi.mask=LVIF_PARAM;
		lvi.iItem=Sel;
		lvi.iSubItem=0;
		ListView_GetItem(hwndList,&lvi);
		if (lvi.lParam==0) {
			DlgComboBox_SetCurSel(m_hDlg,IDC_CONTROLLER_COMMAND,0);
		} else {
			DlgComboBox_SetCurSel(m_hDlg,IDC_CONTROLLER_COMMAND,
								  GetAppClass().GetCommandList()->IDToIndex((int)lvi.lParam)+1);
		}
	} else {
		DlgComboBox_SetCurSel(m_hDlg,IDC_CONTROLLER_COMMAND,0);
	}
	EnableDlgItem(m_hDlg,IDC_CONTROLLER_COMMAND,Sel>=0);
}


CController *CControllerManager::GetCurController() const
{
	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_CONTROLLER_LIST);

	if (Sel<0 || Sel>=(int)m_ControllerList.size())
		return NULL;
	return m_ControllerList[Sel].pController;
}


CControllerManager *CControllerManager::GetThis(HWND hDlg)
{
	return static_cast<CControllerManager*>(GetOptions(hDlg));
}


INT_PTR CALLBACK CControllerManager::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CControllerManager *pThis=static_cast<CControllerManager*>(OnInitDialog(hDlg,lParam));

			const size_t NumControllers=pThis->m_ControllerList.size();
			if (NumControllers>0) {
				pThis->m_CurSettingsList.resize(NumControllers);
				int Sel=0;
				for (size_t i=0;i<NumControllers;i++) {
					const ControllerInfo &Info=pThis->m_ControllerList[i];

					DlgComboBox_AddString(hDlg,IDC_CONTROLLER_LIST,Info.pController->GetText());
					if (!pThis->m_CurController.IsEmpty()
							&& ::lstrcmpi(pThis->m_CurController.Get(),Info.pController->GetName())==0)
						Sel=(int)i;
					if (Info.fSettingsLoaded)
						pThis->m_CurSettingsList[i]=Info.Settings;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_CONTROLLER_LIST,Sel);
			}
			EnableDlgItem(hDlg,IDC_CONTROLLER_LIST,NumControllers>0);

			HWND hwndList=::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
			LV_COLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=120;
			lvc.pszText=TEXT("ボタン");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("コマンド");
			ListView_InsertColumn(hwndList,1,&lvc);

			const CCommandList *pCommandList=GetAppClass().GetCommandList();
			TCHAR szText[CCommandList::MAX_COMMAND_NAME];
			DlgComboBox_AddString(hDlg,IDC_CONTROLLER_COMMAND,TEXT("なし"));
			for (int i=0;i<pCommandList->NumCommands();i++) {
				pCommandList->GetCommandName(i,szText,lengthof(szText));
				DlgComboBox_AddString(hDlg,IDC_CONTROLLER_COMMAND,szText);
			}

			pThis->m_Tooltip.Create(hDlg);

			pThis->InitDlgItems();
			pThis->SetDlgItemStatus();
		}
		return TRUE;

	case WM_PAINT:
		{
			CControllerManager *pThis=GetThis(hDlg);

			if (pThis->m_hbmController==NULL)
				break;

			const CController *pController=pThis->GetCurController();
			if (pController==NULL)
				break;

			int CurButton=ListView_GetNextItem(::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN),-1,LVNI_SELECTED);

			PAINTSTRUCT ps;
			BITMAP bm;
			RECT rc;
			HDC hdcMem;
			HBITMAP hbmOld;

			::BeginPaint(hDlg,&ps);
			::GetObject(pThis->m_hbmController,sizeof(BITMAP),&bm);
			::SetRect(&rc,172,8,232,216);
			::MapDialogRect(hDlg,&rc);
			::FillRect(ps.hdc,&rc,static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
			hdcMem=::CreateCompatibleDC(ps.hdc);
			hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,pThis->m_hbmController));
			::BitBlt(ps.hdc,pThis->m_ImageRect.left,pThis->m_ImageRect.top,
							bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
			if (CurButton>=0 && pThis->m_hbmSelButtons!=NULL) {
				CController::ButtonInfo Button;

				pController->GetButtonInfo(CurButton,&Button);
				::SelectObject(hdcMem,pThis->m_hbmSelButtons);
				::TransparentBlt(ps.hdc,
					pThis->m_ImageRect.left+Button.ImageButtonRect.Left,
					pThis->m_ImageRect.top+Button.ImageButtonRect.Top,
					Button.ImageButtonRect.Width,
					Button.ImageButtonRect.Height,
					hdcMem,
					Button.ImageSelButtonPos.Left,
					Button.ImageSelButtonPos.Top,
					Button.ImageButtonRect.Width,
					Button.ImageButtonRect.Height,
					RGB(255,0,255));
			}
			::SelectObject(hdcMem,hbmOld);
			::DeleteDC(hdcMem);
			::EndPaint(hDlg,&ps);
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		{
			CControllerManager *pThis=GetThis(hDlg);
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (pThis->m_hbmSelButtons!=NULL
					&& ::PtInRect(&pThis->m_ImageRect,pt)) {
				const CController *pController=pThis->GetCurController();
				if (pController==NULL)
					return TRUE;

				const int NumButtons=pController->NumButtons();
				for (int i=0;i<NumButtons;i++) {
					CController::ButtonInfo Button;
					RECT rc;

					pController->GetButtonInfo(i,&Button);
					rc.left=pThis->m_ImageRect.left+Button.ImageButtonRect.Left;
					rc.top=pThis->m_ImageRect.top+Button.ImageButtonRect.Top;
					rc.right=rc.left+Button.ImageButtonRect.Width;
					rc.bottom=rc.top+Button.ImageButtonRect.Height;
					if (::PtInRect(&rc,pt)) {
						HWND hwndList=::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN);

						ListView_SetItemState(hwndList,i,
											  LVIS_FOCUSED | LVIS_SELECTED,
											  LVIS_FOCUSED | LVIS_SELECTED);
						ListView_EnsureVisible(hwndList,i,FALSE);
						::SetFocus(hwndList);
						break;
					}
				}
			}
		}
		return TRUE;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CControllerManager *pThis=GetThis(hDlg);

			if (pThis->m_hbmSelButtons!=NULL) {
				POINT pt;

				::GetCursorPos(&pt);
				::ScreenToClient(hDlg,&pt);
				if (::PtInRect(&pThis->m_ImageRect,pt)) {
					const CController *pController=pThis->GetCurController();
					if (pController==NULL)
						break;

					const int NumButtons=pController->NumButtons();
					for (int i=0;i<NumButtons;i++) {
						CController::ButtonInfo Button;
						RECT rc;

						pController->GetButtonInfo(i,&Button);
						rc.left=pThis->m_ImageRect.left+Button.ImageButtonRect.Left;
						rc.top=pThis->m_ImageRect.top+Button.ImageButtonRect.Top;
						rc.right=rc.left+Button.ImageButtonRect.Width;
						rc.bottom=rc.top+Button.ImageButtonRect.Height;
						if (::PtInRect(&rc,pt)) {
							::SetCursor(::LoadCursor(NULL,IDC_HAND));
							::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
							return TRUE;
						}
					}
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONTROLLER_LIST:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CControllerManager *pThis=GetThis(hDlg);
				const CController *pCurController=pThis->GetCurController();

				pThis->InitDlgItems();
				if (pCurController!=NULL)
					pThis->m_CurController.Set(pCurController->GetName());
			}
			return TRUE;

		case IDC_CONTROLLER_ACTIVEONLY:
			{
				int CurController=(int)DlgComboBox_GetCurSel(hDlg,IDC_CONTROLLER_LIST);

				if (CurController>=0) {
					CControllerManager *pThis=GetThis(hDlg);

					pThis->m_CurSettingsList[CurController].fActiveOnly=
						DlgCheckBox_IsChecked(hDlg,IDC_CONTROLLER_ACTIVEONLY);
				}
			}
			return TRUE;

		case IDC_CONTROLLER_COMMAND:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				HWND hwndList=::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					CControllerManager *pThis=GetThis(hDlg);
					int Command=(int)DlgComboBox_GetCurSel(hDlg,IDC_CONTROLLER_COMMAND);

					pThis->SetButtonCommand(hwndList,Sel,
						Command<=0?0:GetAppClass().GetCommandList()->GetCommandID(Command-1));
				}
			}
			return TRUE;

		case IDC_CONTROLLER_DEFAULT:
			{
				CControllerManager *pThis=GetThis(hDlg);
				const CController *pController=pThis->GetCurController();
				if (pController==NULL)
					return TRUE;

				HWND hwndList=::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN);
				const int NumButtons=pController->NumButtons();
				for (int i=0;i<NumButtons;i++) {
					CController::ButtonInfo Button;

					pController->GetButtonInfo(i,&Button);
					pThis->SetButtonCommand(hwndList,i,Button.DefaultCommand);
				}
				pThis->SetDlgItemStatus();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				CControllerManager *pThis=GetThis(hDlg);

				pThis->SetDlgItemStatus();
				::InvalidateRect(hDlg,&pThis->m_ImageRect,FALSE);
			}
			break;

		case LVN_KEYDOWN:
			{
				LPNMLVKEYDOWN pnmlvk=reinterpret_cast<LPNMLVKEYDOWN>(lParam);

				if (pnmlvk->wVKey==VK_BACK || pnmlvk->wVKey==VK_DELETE) {
					CControllerManager *pThis=GetThis(hDlg);
					HWND hwndList=::GetDlgItem(hDlg,IDC_CONTROLLER_ASSIGN);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0)
						pThis->SetButtonCommand(hwndList,Sel,0);
				}
			}
			break;

		case PSN_APPLY:
			{
				CControllerManager *pThis=GetThis(hDlg);

				for (size_t i=0;i<pThis->m_ControllerList.size();i++) {
					ControllerInfo &Info=pThis->m_ControllerList[i];
					ControllerSettings &CurSettings=pThis->m_CurSettingsList[i];

					if (Info.pController->IsEnabled()) {
						if (CurSettings.fActiveOnly!=Info.Settings.fActiveOnly) {
							Info.pController->Enable(false);
							Info.Settings.fActiveOnly=CurSettings.fActiveOnly;
							Info.pController->Enable(true);
						}
					}
					Info.Settings=CurSettings;
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CControllerManager *pThis=GetThis(hDlg);

			if (pThis->m_hbmController!=NULL) {
				::DeleteObject(pThis->m_hbmController);
				pThis->m_hbmController=NULL;
			}
			if (pThis->m_hbmSelButtons!=NULL) {
				::DeleteObject(pThis->m_hbmSelButtons);
				pThis->m_hbmSelButtons=NULL;
			}
			pThis->m_CurSettingsList.clear();
			pThis->m_Tooltip.Destroy();
			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}

#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "RemoteController.h"
#include "TVTest_KeyHook/TVTest_KeyHook.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"




class CRemoteController {
	HMODULE m_hLib;
	bool m_fHook;
	UINT m_Message;
	HWND m_hwnd;
public:
	CRemoteController(HWND hwnd);
	~CRemoteController();
	bool BeginHook(bool fLocal);
	bool EndHook();
	bool HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam,WORD *pKeyCode,WORD *pModifiers);
};


CRemoteController::CRemoteController(HWND hwnd)
{
	m_hLib=NULL;
	m_fHook=false;
	m_Message=WM_NULL;
	m_hwnd=hwnd;
}


CRemoteController::~CRemoteController()
{
	if (m_hLib!=NULL) {
		EndHook();
		FreeLibrary(m_hLib);
	}
}


bool CRemoteController::BeginHook(bool fLocal)
{
	BeginHookFunc pBeginHook;

	if (m_hLib==NULL) {
		m_hLib=LoadLibrary(TEXT("TVTest_KeyHook.dll"));
		if (m_hLib==NULL)
			return false;
	}
	pBeginHook=(BeginHookFunc)GetProcAddress(m_hLib,"BeginHook");
	if (pBeginHook==NULL || !pBeginHook(m_hwnd,fLocal))
		return false;
	m_fHook=true;
	m_Message=RegisterWindowMessage(KEYHOOK_MESSAGE);

	// Vista で管理者権限で実行された時用の対策
#ifndef MSGFLT_ADD
#define MSGFLT_ADD 1
#endif
	typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT,DWORD);
	HMODULE hLib=LoadLibrary(TEXT("user32.dll"));
	ChangeWindowMessageFilterFunc pChangeFilter=(ChangeWindowMessageFilterFunc)
							GetProcAddress(hLib,"ChangeWindowMessageFilter");

	if (pChangeFilter!=NULL)
		pChangeFilter(m_Message,MSGFLT_ADD);
	FreeLibrary(hLib);
	return true;
}


bool CRemoteController::EndHook()
{
	if (m_fHook) {
		EndHookFunc pEndHook;

		pEndHook=(EndHookFunc)GetProcAddress(m_hLib,"EndHook");
		if (pEndHook==NULL)
			return false;
		pEndHook();
		m_fHook=false;
	}
	return true;
}


bool CRemoteController::HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam,WORD *pKeyCode,WORD *pModifiers)
{
	if (m_Message==WM_NULL || uMsg!=m_Message
			|| KEYHOOK_GET_REPEATCOUNT(lParam)>1)
		return false;
	WORD Modifiers=0;
	if (KEYHOOK_GET_CONTROL(lParam))
		Modifiers|=MK_CONTROL;
	if (KEYHOOK_GET_SHIFT(lParam))
		Modifiers|=MK_SHIFT;
	if (pKeyCode)
		*pKeyCode=(WORD)wParam;
	if (pModifiers)
		*pModifiers=Modifiers;
	return true;
}




static const struct {
	LPCTSTR pszName;
	BYTE KeyCode;
	BYTE Modifiers;
	WORD DefaultCommand;
	struct {
		BYTE Left,Top,Width,Height;
	} ButtonRect;
	struct {
		BYTE Left,Top;
	} SelButtonPos;
} ButtonList[] = {
	{TEXT("画面表示"),		VK_F13,	MK_SHIFT,				CM_ASPECTRATIO,
		{8,14,16,11},{0,0}},
	{TEXT("電源"),			VK_F14,	MK_SHIFT,				CM_CLOSE,
		{32,11,16,16},{0,22}},
	{TEXT("消音"),			VK_F15,	MK_SHIFT,				CM_VOLUME_MUTE,
		{56,14,16,11},{16,0}},
	{TEXT("チャンネル1"),	VK_F17,	MK_SHIFT,				CM_CHANNELNO_1,
		{8,28,16,11},{32,0}},
	{TEXT("チャンネル2"),	VK_F18,	MK_SHIFT,				CM_CHANNELNO_2,
		{24,28,16,11},{48,0}},
	{TEXT("チャンネル3"),	VK_F19,	MK_SHIFT,				CM_CHANNELNO_3,
		{40,28,16,11},{64,0}},
	{TEXT("チャンネル4"),	VK_F20,	MK_SHIFT,				CM_CHANNELNO_4,
		{8,43,16,11},{80,0}},
	{TEXT("チャンネル5"),	VK_F21,	MK_SHIFT,				CM_CHANNELNO_5,
		{24,43,16,11},{96,0}},
	{TEXT("チャンネル6"),	VK_F22,	MK_SHIFT,				CM_CHANNELNO_6,
		{40,43,16,11},{112,0}},
	{TEXT("チャンネル7"),	VK_F23,	MK_SHIFT,				CM_CHANNELNO_7,
		{8,57,16,11},{128,0}},
	{TEXT("チャンネル8"),	VK_F24,	MK_SHIFT,				CM_CHANNELNO_8,
		{24,57,16,11},{144,0}},
	{TEXT("チャンネル9"),	VK_F13,	MK_CONTROL,				CM_CHANNELNO_9,
		{40,57,16,11},{160,0}},
	{TEXT("チャンネル10"),	VK_F16,	MK_SHIFT,				CM_CHANNELNO_10,
		{8,71,16,11},{176,0}},
	{TEXT("チャンネル11"),	VK_F14,	MK_CONTROL,				CM_CHANNELNO_11,
		{24,71,16,11},{192,0}},
	{TEXT("チャンネル12"),	VK_F15,	MK_CONTROL,				CM_CHANNELNO_12,
		{40,71,16,11},{208,0}},
	{TEXT("メニュー"),		VK_F16,	MK_CONTROL,				CM_MENU,
		{11,85,28,11},{32,11}},
	{TEXT("全画面表示"),	VK_F17,	MK_CONTROL,				CM_FULLSCREEN,
		{41,85,28,11},{60,11}},
	{TEXT("字幕"),			VK_F18,	MK_CONTROL,				0,
		{9,102,24,25},{122,22}},
	{TEXT("音声切替"),		VK_F19,	MK_CONTROL			,	CM_SWITCHAUDIO,
		{47,102,24,25},{146,22,}},
	{TEXT("EPG"),			VK_F20,	MK_CONTROL,				CM_PROGRAMGUIDE,
		{9,140,24,25},{170,22}},
	{TEXT("戻る"),			VK_F21,	MK_CONTROL,				0,
		{47,140,24,25},{194,22}},
	{TEXT("録画"),			VK_F22,	MK_CONTROL,				CM_RECORD_START,
		{20,170,16,11},{88,11}},
	{TEXT("メモ"),			VK_F23,	MK_CONTROL,				CM_SAVEIMAGE,
		{44,170,16,11},{104,11}},
	{TEXT("停止"),			VK_F24,	MK_CONTROL,				CM_RECORD_STOP,
		{8,186,16,11},{120,11}},
	{TEXT("再生"),			VK_F13,	MK_CONTROL | MK_SHIFT,	0,
		{26,184,28,14},{16,22}},
	{TEXT("一時停止"),		VK_F14,	MK_CONTROL | MK_SHIFT,	CM_RECORD_PAUSE,
		{56,187,16,11},{136,11}},
	{TEXT("|<<"),			VK_F15,	MK_CONTROL | MK_SHIFT,	0,
		{8,201,16,11},{152,11}},
	{TEXT("<<"),			VK_F16,	MK_CONTROL | MK_SHIFT,	0,
		{24,201,16,11},{168,11}},
	{TEXT(">>"),			VK_F17,	MK_CONTROL | MK_SHIFT,	0,
		{40,201,16,11},{184,11}},
	{TEXT(">>|"),			VK_F18,	MK_CONTROL | MK_SHIFT,	0,
		{56,201,16,11},{200,11}},
	{TEXT("しおり"),		VK_F19,	MK_CONTROL | MK_SHIFT,	0,
		{24,215,16,11},{216,11}},
	{TEXT("ジャンプ"),		VK_F20,	MK_CONTROL | MK_SHIFT,	0,
		{40,215,16,11},{232,11}},
	{TEXT("A (青)"),		VK_F21,	MK_CONTROL | MK_SHIFT,	0,
		{8,230,16,11},{44,22}},
	{TEXT("B (赤)"),		VK_F22,	MK_CONTROL | MK_SHIFT,	0,
		{24,230,16,11},{60,22}},
	{TEXT("C (緑)"),		VK_F23,	MK_CONTROL | MK_SHIFT,	0,
		{40,230,16,11},{76,22}},
	{TEXT("D (黄)"),		VK_F24,	MK_CONTROL | MK_SHIFT,	0,
		{56,230,16,11},{92,22}},
	/*
	{TEXT("音量 +"),		VK_UP,		MK_SHIFT,			CM_VOLUME_UP,
		{56,28,16,11},{0,11}},
	{TEXT("音量 -"),		VK_DOWN,	MK_SHIFT,			CM_VOLUME_DOWN,
		{56,43,16,11},{16,11}},
	{TEXT("チャンネル +"),	VK_UP,		MK_CONTROL,			CM_CHANNEL_UP,
		{57,57,14,13},{108,22}},
	{TEXT("チャンネル -"),	VK_DOWN,	MK_CONTROL,			CM_CHANNEL_DOWN,
		{57,70,14,13},{108,35}},
	*/
};


CHDUSController::CHDUSController()
{
	m_fUseHDUSController=false;
	m_fActiveOnly=false;
	for (int i=0;i<NUM_BUTTONS;i++)
		m_AssignList[i]=ButtonList[i].DefaultCommand;
	m_pRemoteController=NULL;
	m_hAccel=NULL;
	m_pCommandList=NULL;
	m_hbmController=NULL;
	m_hbmSelButtons=NULL;
	m_hwndToolTip=NULL;
}


CHDUSController::~CHDUSController()
{
	Finalize();
}


bool CHDUSController::Read(CSettings *pSettings)
{
	bool f;

	if (pSettings->Read(TEXT("NoRemoteController"),&f))
		m_fUseHDUSController=!f;
	pSettings->Read(TEXT("NoKeyHook"),&m_fActiveOnly);
	return true;
}


bool CHDUSController::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("NoRemoteController"),!m_fUseHDUSController);
	pSettings->Write(TEXT("NoKeyHook"),m_fActiveOnly);
	return true;
}


bool CHDUSController::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("HDUSController"),CSettings::OPEN_READ)) {
		for (int i=0;i<lengthof(ButtonList);i++) {
			TCHAR szName[64],szCommand[64];

			::wsprintf(szName,TEXT("Button%d_Command"),i);
			if (Settings.Read(szName,szCommand,lengthof(szCommand))
					&& szCommand[0]!='\0') {
				m_AssignList[i]=m_pCommandList->ParseText(szCommand);
			}
		}
		Settings.Close();
	}
	return true;
}


bool CHDUSController::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (m_pCommandList==NULL)
		return true;
	if (Settings.Open(pszFileName,TEXT("HDUSController"),CSettings::OPEN_WRITE)) {
		for (int i=0;i<lengthof(ButtonList);i++) {
			TCHAR szName[64];

			::wsprintf(szName,TEXT("Button%d_Command"),i);
			Settings.Write(szName,m_AssignList[i]>0?
				m_pCommandList->GetCommandText(m_pCommandList->IDToIndex(m_AssignList[i])):TEXT(""));
		}
		Settings.Close();
	}
	return true;
}


bool CHDUSController::Initialize(HWND hwnd,LPCTSTR pszSettingFileName,const CCommandList *pCommandList)
{
	m_pCommandList=pCommandList;
	Load(pszSettingFileName);
	if (m_fUseHDUSController) {
		m_pRemoteController=new CRemoteController(hwnd);
		m_pRemoteController->BeginHook(m_fActiveOnly);
		m_hAccel=::LoadAccelerators(GetAppClass().GetResourceInstance(),
									MAKEINTRESOURCE(IDA_ACCEL));
	}
	return true;
}


void CHDUSController::Finalize()
{
	SAFE_DELETE(m_pRemoteController);
	m_hAccel=NULL;
}


bool CHDUSController::TranslateMessage(HWND hwnd,LPMSG pmsg)
{
	return m_hAccel!=NULL && ::TranslateAccelerator(hwnd,m_hAccel,pmsg);
}


bool CHDUSController::HandleMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WORD KeyCode,Modifiers;

	if (m_pRemoteController==NULL)
		return false;
	if (!m_pRemoteController->HandleMessage(uMsg,wParam,lParam,&KeyCode,&Modifiers))
		return false;
	for (int i=0;i<lengthof(ButtonList);i++) {
		if (ButtonList[i].KeyCode==KeyCode && ButtonList[i].Modifiers==Modifiers) {
			if (m_AssignList[i]>0)
				::PostMessage(hwnd,WM_COMMAND,m_AssignList[i],0);
			break;
		}
	}
	return true;
}


void CHDUSController::SetButtonCommand(HWND hwndList,int Index,int Command)
{
	LV_ITEM lvi;
	TCHAR szText[64];

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.lParam=Command;
	ListView_SetItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	if (Command>0) {
		m_pCommandList->GetCommandName(m_pCommandList->IDToIndex(Command),szText,lengthof(szText));
		lvi.pszText=szText;
	} else {
		lvi.pszText=TEXT("");
	}
	ListView_SetItem(hwndList,&lvi);
}


void CHDUSController::SetDlgItemStatus()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_HDUSCONTROLLER_ASSIGN);
	int Sel,Index;

	Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (Sel>=0) {
		LV_ITEM lvi;

		lvi.mask=LVIF_PARAM;
		lvi.iItem=Sel;
		lvi.iSubItem=0;
		ListView_GetItem(hwndList,&lvi);
		for (int i=0;i<m_pCommandList->NumCommands();i++) {
			if (m_pCommandList->GetCommandID(i)==(int)lvi.lParam) {
				DlgComboBox_SetCurSel(m_hDlg,IDC_HDUSCONTROLLER_COMMAND,i+1);
				break;
			}
		}
	} else {
		DlgComboBox_SetCurSel(m_hDlg,IDC_HDUSCONTROLLER_COMMAND,0);
	}
	EnableDlgItem(m_hDlg,IDC_HDUSCONTROLLER_COMMAND,Sel>=0);
}


CHDUSController *CHDUSController::GetThis(HWND hDlg)
{
	return static_cast<CHDUSController*>(GetOptions(hDlg));
}


BOOL CALLBACK CHDUSController::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CHDUSController *pThis=static_cast<CHDUSController*>(OnInitDialog(hDlg,lParam));

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_USEREMOTECONTROLLER,
							  pThis->m_fUseHDUSController);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
							  pThis->m_fActiveOnly);
			EnableDlgItem(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
						  pThis->m_fUseHDUSController);

			HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);
			LV_COLUMN lvc;
			TCHAR szText[CCommandList::MAX_COMMAND_NAME];

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=120;
			lvc.pszText=TEXT("ボタン");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("コマンド");
			ListView_InsertColumn(hwndList,1,&lvc);
			for (int i=0;i<lengthof(ButtonList);i++) {
				LV_ITEM lvi;

				lvi.mask=LVIF_TEXT | LVIF_PARAM;
				lvi.iItem=i;
				lvi.iSubItem=0;
				lvi.pszText=const_cast<LPTSTR>(ButtonList[i].pszName);
				lvi.lParam=pThis->m_AssignList[i];
				lvi.iItem=ListView_InsertItem(hwndList,&lvi);
				if (pThis->m_AssignList[i]>0) {
					lvi.mask=LVIF_TEXT;
					lvi.iSubItem=1;
					pThis->m_pCommandList->GetCommandName(
						pThis->m_pCommandList->IDToIndex(pThis->m_AssignList[i]),
						szText,lengthof(szText));
					lvi.pszText=szText;
					ListView_SetItem(hwndList,&lvi);
				}
			}
			for (int i=0;i<2;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
			DlgComboBox_AddString(hDlg,IDC_HDUSCONTROLLER_COMMAND,TEXT("なし"));
			for (int i=0;i<pThis->m_pCommandList->NumCommands();i++) {
				pThis->m_pCommandList->GetCommandName(i,szText,lengthof(szText));
				DlgComboBox_AddString(hDlg,IDC_HDUSCONTROLLER_COMMAND,szText);
			}

			HINSTANCE hinst=GetAppClass().GetResourceInstance();
			pThis->m_hbmController=::LoadBitmap(hinst,MAKEINTRESOURCE(IDB_HDUSCONTROLLER));
			pThis->m_hbmSelButtons=::LoadBitmap(hinst,MAKEINTRESOURCE(IDB_HDUSCONTROLLER_SEL));
			BITMAP bm;
			RECT rc;
			::GetObject(pThis->m_hbmController,sizeof(BITMAP),&bm);
			::SetRect(&rc,172,8,232,216);
			::MapDialogRect(hDlg,&rc);
			pThis->m_ImageRect.left=rc.left+((rc.right-rc.left)-bm.bmWidth)/2;
			pThis->m_ImageRect.top=rc.top+((rc.bottom-rc.top)-bm.bmHeight)/2;
			pThis->m_ImageRect.right=pThis->m_ImageRect.left+bm.bmWidth;
			pThis->m_ImageRect.bottom=pThis->m_ImageRect.top+bm.bmHeight;

			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_ALWAYSTIP,0,0,0,0,hDlg,NULL,GetAppClass().GetInstance(),NULL);
			TOOLINFO ti;
			ti.cbSize=TTTOOLINFO_V1_SIZE;
			ti.uFlags=TTF_SUBCLASS;
			ti.hwnd=hDlg;
			ti.hinst=NULL;
			for (int i=0;i<lengthof(ButtonList);i++) {
				ti.uId=i;
				ti.lpszText=const_cast<LPTSTR>(ButtonList[i].pszName);
				ti.rect.left=pThis->m_ImageRect.left+ButtonList[i].ButtonRect.Left;
				ti.rect.top=pThis->m_ImageRect.top+ButtonList[i].ButtonRect.Top;
				ti.rect.right=ti.rect.left+ButtonList[i].ButtonRect.Width;
				ti.rect.bottom=ti.rect.top+ButtonList[i].ButtonRect.Height;
				::SendMessage(pThis->m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}

			pThis->SetDlgItemStatus();
		}
		return TRUE;

	case WM_PAINT:
		{
			CHDUSController *pThis=GetThis(hDlg);
			PAINTSTRUCT ps;
			BITMAP bm;
			RECT rc;
			HDC hdcMem;
			HBITMAP hbmOld;
			int CurButton=ListView_GetNextItem(::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN),-1,LVNI_SELECTED);

			::BeginPaint(hDlg,&ps);
			::GetObject(pThis->m_hbmController,sizeof(BITMAP),&bm);
			::SetRect(&rc,172,8,232,216);
			::MapDialogRect(hDlg,&rc);
			::FillRect(ps.hdc,&rc,static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
			hdcMem=::CreateCompatibleDC(ps.hdc);
			hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,pThis->m_hbmController));
			::BitBlt(ps.hdc,pThis->m_ImageRect.left,pThis->m_ImageRect.top,
							bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
			if (CurButton>=0) {
				::SelectObject(hdcMem,pThis->m_hbmSelButtons);
				::TransparentBlt(ps.hdc,
					pThis->m_ImageRect.left+ButtonList[CurButton].ButtonRect.Left,
					pThis->m_ImageRect.top+ButtonList[CurButton].ButtonRect.Top,
					ButtonList[CurButton].ButtonRect.Width,
					ButtonList[CurButton].ButtonRect.Height,
					hdcMem,
					ButtonList[CurButton].SelButtonPos.Left,
					ButtonList[CurButton].SelButtonPos.Top,
					ButtonList[CurButton].ButtonRect.Width,
					ButtonList[CurButton].ButtonRect.Height,
					RGB(255,0,255));
			}
			::SelectObject(hdcMem,hbmOld);
			::DeleteDC(hdcMem);
			::EndPaint(hDlg,&ps);
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		{
			CHDUSController *pThis=GetThis(hDlg);
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (::PtInRect(&pThis->m_ImageRect,pt)) {
				for (int i=0;i<lengthof(ButtonList);i++) {
					RECT rc;

					rc.left=pThis->m_ImageRect.left+ButtonList[i].ButtonRect.Left;
					rc.top=pThis->m_ImageRect.top+ButtonList[i].ButtonRect.Top;
					rc.right=rc.left+ButtonList[i].ButtonRect.Width;
					rc.bottom=rc.top+ButtonList[i].ButtonRect.Height;
					if (::PtInRect(&rc,pt)) {
						HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);

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
			CHDUSController *pThis=GetThis(hDlg);
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hDlg,&pt);
			if (::PtInRect(&pThis->m_ImageRect,pt)) {
				for (int i=0;i<lengthof(ButtonList);i++) {
					RECT rc;

					rc.left=pThis->m_ImageRect.left+ButtonList[i].ButtonRect.Left;
					rc.top=pThis->m_ImageRect.top+ButtonList[i].ButtonRect.Top;
					rc.right=rc.left+ButtonList[i].ButtonRect.Width;
					rc.bottom=rc.top+ButtonList[i].ButtonRect.Height;
					if (::PtInRect(&rc,pt)) {
						::SetCursor(::LoadCursor(NULL,IDC_HAND));
						::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
						return TRUE;
					}
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_USEREMOTECONTROLLER:
			EnableDlgItem(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY,
				DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEREMOTECONTROLLER));
			return TRUE;

		case IDC_HDUSCONTROLLER_COMMAND:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CHDUSController *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel<0)
					return TRUE;
				pThis->SetButtonCommand(hwndList,Sel,
					pThis->m_pCommandList->GetCommandID(DlgComboBox_GetCurSel(hDlg,IDC_HDUSCONTROLLER_COMMAND)-1));
			}
			return TRUE;

		case IDC_HDUSCONTROLLER_DEFAULT:
			{
				CHDUSController *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);

				for (int i=0;i<lengthof(ButtonList);i++)
					pThis->SetButtonCommand(hwndList,i,ButtonList[i].DefaultCommand);
				pThis->SetDlgItemStatus();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				CHDUSController *pThis=GetThis(hDlg);

				pThis->SetDlgItemStatus();
				::InvalidateRect(hDlg,&pThis->m_ImageRect,FALSE);
			}
			break;

		case LVN_KEYDOWN:
			{
				LPNMLVKEYDOWN pnmlvk=reinterpret_cast<LPNMLVKEYDOWN>(lParam);

				if (pnmlvk->wVKey==VK_BACK || pnmlvk->wVKey==VK_DELETE) {
					CHDUSController *pThis=GetThis(hDlg);
					HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0)
						pThis->SetButtonCommand(hwndList,Sel,0);
				}
			}
			break;

		case PSN_APPLY:
			{
				CHDUSController *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_HDUSCONTROLLER_ASSIGN);
				LV_ITEM lvi;

				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (int i=0;i<lengthof(ButtonList);i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					pThis->m_AssignList[i]=(WORD)lvi.lParam;
				}

				bool fUse,fActiveOnly;
				fUse=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEREMOTECONTROLLER);
				fActiveOnly=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_REMOTECONTROLLERACTIVEONLY);
				if (fUse!=pThis->m_fUseHDUSController
						|| fActiveOnly!=pThis->m_fActiveOnly) {
					SAFE_DELETE(pThis->m_pRemoteController);
					if (fUse) {
						pThis->m_pRemoteController=new CRemoteController(GetAppClass().GetMainWindow()->GetHandle());
						pThis->m_pRemoteController->BeginHook(fActiveOnly);
						pThis->m_hAccel=::LoadAccelerators(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDA_ACCEL));
					} else {
						pThis->m_hAccel=NULL;
					}
					pThis->m_fUseHDUSController=fUse;
					pThis->m_fActiveOnly=fActiveOnly;
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CHDUSController *pThis=GetThis(hDlg);

			if (pThis->m_hbmController!=NULL) {
				::DeleteObject(pThis->m_hbmController);
				pThis->m_hbmController=NULL;
			}
			if (pThis->m_hbmSelButtons!=NULL) {
				::DeleteObject(pThis->m_hbmSelButtons);
				pThis->m_hbmSelButtons=NULL;
			}
			pThis->OnDestroy();
			pThis->m_hwndToolTip=NULL;
		}
		return TRUE;
	}
	return FALSE;
}

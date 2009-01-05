#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Accelerator.h"
#include "Command.h"
#include "Menu.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAKE_ACCEL_PARAM(key,mod,global) (((key)<<16)|((mod)<<8)|((global)?1:0))
#define GET_ACCEL_KEY(param)	((WORD)((param)>>16))
#define GET_ACCEL_MOD(param)	((BYTE)(((param)>>8)&0xFF))
#define GET_ACCEL_GLOBAL(param)	(((param)&0xFF)!=0)


static const struct {
	unsigned int KeyCode;
	LPCTSTR pszText;
} AccelKeyList[] = {
	{VK_BACK,		TEXT("BS")},
	{VK_TAB,		TEXT("Tab")},
	{VK_CLEAR,		TEXT("Clear")},
	{VK_RETURN,		TEXT("Enter")},
	{VK_PAUSE,		TEXT("Pause")},
	{VK_ESCAPE,		TEXT("Esc")},
	{VK_SPACE,		TEXT("Space")},
	{VK_PRIOR,		TEXT("PgUp")},
	{VK_NEXT,		TEXT("PgDown")},
	{VK_END,		TEXT("End")},
	{VK_HOME,		TEXT("Home")},
	{VK_LEFT,		TEXT("←")},
	{VK_UP,			TEXT("↑")},
	{VK_RIGHT,		TEXT("→")},
	{VK_DOWN,		TEXT("↓")},
	{VK_SELECT,		TEXT("Select")},
	{VK_PRINT,		TEXT("Print")},
	{VK_EXECUTE,	TEXT("Execute")},
	{VK_INSERT,		TEXT("Ins")},
	{VK_DELETE,		TEXT("Del")},
	{VK_HELP,		TEXT("Help")},
	{'0',			TEXT("0")},
	{'1',			TEXT("1")},
	{'2',			TEXT("2")},
	{'3',			TEXT("3")},
	{'4',			TEXT("4")},
	{'5',			TEXT("5")},
	{'6',			TEXT("6")},
	{'7',			TEXT("7")},
	{'8',			TEXT("8")},
	{'9',			TEXT("9")},
	{'A',			TEXT("A")},
	{'B',			TEXT("B")},
	{'C',			TEXT("C")},
	{'D',			TEXT("D")},
	{'E',			TEXT("E")},
	{'F',			TEXT("F")},
	{'G',			TEXT("G")},
	{'H',			TEXT("H")},
	{'I',			TEXT("I")},
	{'J',			TEXT("J")},
	{'K',			TEXT("K")},
	{'L',			TEXT("L")},
	{'M',			TEXT("M")},
	{'N',			TEXT("N")},
	{'O',			TEXT("O")},
	{'P',			TEXT("P")},
	{'Q',			TEXT("Q")},
	{'R',			TEXT("R")},
	{'S',			TEXT("S")},
	{'T',			TEXT("T")},
	{'U',			TEXT("U")},
	{'V',			TEXT("V")},
	{'W',			TEXT("W")},
	{'X',			TEXT("X")},
	{'Y',			TEXT("Y")},
	{'Z',			TEXT("Z")},
	{VK_NUMPAD0,	TEXT("Num0")},
	{VK_NUMPAD1,	TEXT("Num1")},
	{VK_NUMPAD2,	TEXT("Num2")},
	{VK_NUMPAD3,	TEXT("Num3")},
	{VK_NUMPAD4,	TEXT("Num4")},
	{VK_NUMPAD5,	TEXT("Num5")},
	{VK_NUMPAD6,	TEXT("Num6")},
	{VK_NUMPAD7,	TEXT("Num7")},
	{VK_NUMPAD8,	TEXT("Num8")},
	{VK_NUMPAD9,	TEXT("Num9")},
	{VK_MULTIPLY,	TEXT("Num*")},
	{VK_ADD,		TEXT("Num+")},
	{VK_SUBTRACT,	TEXT("Num-")},
	{VK_DECIMAL,	TEXT("Num.")},
	{VK_DIVIDE,		TEXT("Num/")},
	{VK_F1,			TEXT("F1")},
	{VK_F2,			TEXT("F2")},
	{VK_F3,			TEXT("F3")},
	{VK_F4,			TEXT("F4")},
	{VK_F5,			TEXT("F5")},
	{VK_F6,			TEXT("F6")},
	{VK_F7,			TEXT("F7")},
	{VK_F8,			TEXT("F8")},
	{VK_F9,			TEXT("F9")},
	{VK_F10,		TEXT("F10")},
	{VK_F11,		TEXT("F11")},
	{VK_F12,		TEXT("F12")},
};

const CAccelerator::KeyInfo CAccelerator::m_DefaultAccelList[] = {
	{CM_ZOOM_100,		VK_HOME,		0,			false},
	{CM_ASPECTRATIO,	'A',			0,			false},
	{CM_FULLSCREEN,		VK_RETURN,		MOD_ALT,	false},
	{CM_ALWAYSONTOP,	'T',			0,			false},
	{CM_VOLUME_MUTE,	'M',			0,			false},
	{CM_VOLUME_UP,		VK_UP,			0,			false},
	{CM_VOLUME_DOWN,	VK_DOWN,		0,			false},
	{CM_SWITCHAUDIO,	'S',			0,			false},
	{CM_CHANNEL_DOWN,	VK_LEFT,		0,			false},
	{CM_CHANNEL_UP,		VK_RIGHT,		0,			false},
	{CM_RECORD,			'R',			0,			false},
	{CM_COPY,			'C',			0,			false},
	{CM_SAVEIMAGE,		'V',			0,			false},
	{CM_INFORMATION,	'I',			0,			false},
	{CM_PROGRAMGUIDE,	'E',			0,			false},
};


CAccelerator::CAccelerator()
{
	m_hAccel=NULL;
	for (size_t i=0;i<lengthof(m_DefaultAccelList);i++)
		m_KeyList.push_back(m_DefaultAccelList[i]);
	m_hwndHotKey=NULL;
	m_pMainMenu=NULL;
	m_pCommandList=NULL;
	m_fRegisterHotKey=false;
	m_fFunctionKeyChangeChannel=true;
	m_fDigitKeyChangeChannel=true;
	m_fNumPadChangeChannel=true;
}


CAccelerator::~CAccelerator()
{
	Finalize();
}


void CAccelerator::FormatAccelText(LPTSTR pszText,int Key,int Modifiers)
{
	pszText[0]='\0';
	if ((Modifiers&MOD_SHIFT)!=0)
		::lstrcat(pszText,TEXT("Shift+"));
	if ((Modifiers&MOD_CONTROL)!=0)
		::lstrcat(pszText,TEXT("Ctrl+"));
	if ((Modifiers&MOD_ALT)!=0)
		::lstrcat(pszText,TEXT("Alt+"));
	for (int i=0;i<lengthof(AccelKeyList);i++) {
		if (Key==AccelKeyList[i].KeyCode) {
			::lstrcat(pszText,AccelKeyList[i].pszText);
			break;
		}
	}
}


void CAccelerator::SetMenuAccelText(HMENU hmenu,int Command)
{
	TCHAR szText[128],*p;
	size_t i;
	const KeyInfo *pKey=NULL;

	GetMenuString(hmenu,Command,szText,lengthof(szText),MF_BYCOMMAND);
	for (i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].Command==Command) {
			pKey=&m_KeyList[i];
			break;
		}
	}
	p=szText;
	while (*p!='\t') {
		if (*p=='\0') {
			if (pKey==NULL)
				return;
			break;
		}
		p++;
	}
	if (pKey!=NULL) {
		p[0]='\t';
		FormatAccelText(p+1,pKey->KeyCode,pKey->Modifiers);
	} else
		*p='\0';
	ModifyMenu(hmenu,Command,
		MF_BYCOMMAND | MFT_STRING | GetMenuState(hmenu,Command,MF_BYCOMMAND),
		Command,szText);
}


HACCEL CAccelerator::CreateAccel()
{
	if (m_KeyList.size()==0)
		return NULL;

	LPACCEL paccl;
	int j;

	paccl=new ACCEL[m_KeyList.size()];
	j=0;
	for (size_t i=0;i<m_KeyList.size();i++) {
		if (!m_KeyList[i].fGlobal) {
			paccl[j].fVirt=FVIRTKEY;
			if ((m_KeyList[i].Modifiers&MOD_SHIFT)!=0)
				paccl[j].fVirt|=FSHIFT;
			if ((m_KeyList[i].Modifiers&MOD_CONTROL)!=0)
				paccl[j].fVirt|=FCONTROL;
			if ((m_KeyList[i].Modifiers&MOD_ALT)!=0)
				paccl[j].fVirt|=FALT;
			paccl[j].key=m_KeyList[i].KeyCode;
			paccl[j].cmd=m_KeyList[i].Command;
			j++;
		}
	}
	HACCEL hAccel=NULL;
	if (j>0)
		hAccel=::CreateAcceleratorTable(paccl,j);
	delete [] paccl;
	return hAccel;
}


bool CAccelerator::RegisterHotKey()
{
	if (m_fRegisterHotKey)
		return false;

	bool fOK=true;

	for (size_t i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].fGlobal) {
			if (!::RegisterHotKey(m_hwndHotKey,
					(m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode,
					m_KeyList[i].Modifiers,m_KeyList[i].KeyCode)) {
				fOK=false;
			}
		}
	}
	m_fRegisterHotKey=true;
	return fOK;
}


bool CAccelerator::UnregisterHotKey()
{
	if (!m_fRegisterHotKey)
		return true;

	bool fOK=true;

	for (size_t i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].fGlobal) {
			if (!::UnregisterHotKey(m_hwndHotKey,
					(m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode)) {
				fOK=false;
			}
		}
	}
	m_fRegisterHotKey=false;
	return fOK;
}


bool CAccelerator::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("FuncKeyChangeChannel"),&m_fFunctionKeyChangeChannel);
	pSettings->Read(TEXT("DigitKeyChangeChannel"),&m_fDigitKeyChangeChannel);
	pSettings->Read(TEXT("NumPadChangeChannel"),&m_fNumPadChangeChannel);
	return true;
}


bool CAccelerator::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("FuncKeyChangeChannel"),m_fFunctionKeyChangeChannel);
	pSettings->Write(TEXT("DigitKeyChangeChannel"),m_fDigitKeyChangeChannel);
	pSettings->Write(TEXT("NumPadChangeChannel"),m_fNumPadChangeChannel);
	return true;
}


bool CAccelerator::Load(LPCTSTR pszFileName)
{
	CSettings Settings;
	int i,j;

	if (Settings.Open(pszFileName,TEXT("Accelerator"),CSettings::OPEN_READ)) {
		int NumAccel;

		if (Settings.Read(TEXT("AccelCount"),&NumAccel) && NumAccel>=0) {
			m_KeyList.clear();
			for (i=0;i<NumAccel;i++) {
				TCHAR szName[64],szCommand[CCommandList::MAX_COMMAND_TEXT];

				::wsprintf(szName,TEXT("Accel%d_Command"),i);
				if (Settings.Read(szName,szCommand,lengthof(szCommand))
						&& szCommand[0]!='\0') {
					unsigned int Key,Modifiers;

					::wsprintf(szName,TEXT("Accel%d_Key"),i);
					if (Settings.Read(szName,&Key)) {
						KeyInfo Info;

						Info.Command=m_pCommandList->ParseText(szCommand);
						if (Info.Command==0)
							continue;
						Info.KeyCode=Key;
						Modifiers=0;
						::wsprintf(szName,TEXT("Accel%d_Mod"),i);
						Settings.Read(szName,&Modifiers);
						Info.Modifiers=Modifiers&0x7F;
						Info.fGlobal=(Modifiers&0x80)!=0;
						m_KeyList.push_back(Info);
					}
				}
			}
		}
		Settings.Close();
	}
	return true;
}


bool CAccelerator::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (m_pCommandList==NULL)
		return true;
	if (Settings.Open(pszFileName,TEXT("Accelerator"),CSettings::OPEN_WRITE)) {
		bool fDefault=false;
		int i,j;

		if (m_KeyList.size()==lengthof(m_DefaultAccelList)) {
			for (i=0;i<lengthof(m_DefaultAccelList);i++) {
				for (j=0;j<lengthof(m_DefaultAccelList);j++) {
					if (m_DefaultAccelList[i]==m_KeyList[j])
						break;
				}
				if (j==lengthof(m_DefaultAccelList))
					break;
			}
			if (i==lengthof(m_DefaultAccelList))
				fDefault=true;
		}
		Settings.Clear();
		if (!fDefault) {
			Settings.Write(TEXT("AccelCount"),(int)m_KeyList.size());
			for (i=0;i<(int)m_KeyList.size();i++) {
				TCHAR szName[64];

				::wsprintf(szName,TEXT("Accel%d_Command"),i);
				Settings.Write(szName,
					m_pCommandList->GetCommandText(m_pCommandList->IDToIndex(m_KeyList[i].Command)));
				::wsprintf(szName,TEXT("Accel%d_Key"),i);
				Settings.Write(szName,(int)m_KeyList[i].KeyCode);
				::wsprintf(szName,TEXT("Accel%d_Mod"),i);
				Settings.Write(szName,(int)(m_KeyList[i].Modifiers | (m_KeyList[i].fGlobal?0x80:0x00)));
			}
		} else
			Settings.Write(TEXT("AccelCount"),-1);
		Settings.Close();
	}
	return true;
}


bool CAccelerator::Initialize(HWND hwndHotKey,CMainMenu *pMainMenu,
				LPCTSTR pszSettingFileName,const CCommandList *pCommandList)
{
	m_pCommandList=pCommandList;
	Load(pszSettingFileName);
	if (m_hAccel==NULL)
		m_hAccel=CreateAccel();
	m_pMainMenu=pMainMenu;
	m_pMainMenu->SetAccelerator(this);
	m_hwndHotKey=hwndHotKey;
	RegisterHotKey();
	return true;
}


void CAccelerator::Finalize()
{
	if (m_hAccel) {
		::DestroyAcceleratorTable(m_hAccel);
		m_hAccel=NULL;
	}
	if (m_fRegisterHotKey)
		UnregisterHotKey();
}


bool CAccelerator::TranslateMessage(HWND hwnd,LPMSG pmsg)
{
	if (m_hAccel!=NULL)
		return ::TranslateAccelerator(hwnd,m_hAccel,pmsg)!=FALSE;
	return false;
}


int CAccelerator::TranslateHotKey(WPARAM wParam,LPARAM lParam)
{
	for (size_t i=0;i<m_KeyList.size();i++) {
		if (((m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode)==wParam) {
			return m_KeyList[i].Command;
		}
	}
	return -1;
}


void CAccelerator::SetMenuAccel(HMENU hmenu)
{
	int Count,i;
	unsigned int State;

	Count=::GetMenuItemCount(hmenu);
	for (i=0;i<Count;i++) {
		State=::GetMenuState(hmenu,i,MF_BYPOSITION);
		if ((State&MF_POPUP)!=0) {
			SetMenuAccel(::GetSubMenu(hmenu,i));
		} else if ((State&MF_SEPARATOR)==0) {
			SetMenuAccelText(hmenu,(int)::GetMenuItemID(hmenu,i));
		}
	}
}


int CAccelerator::CheckAccelKey(HWND hwndList,BYTE Mod,WORD Key)
{
	LV_ITEM lvi;
	int i,Count;

	Count=ListView_GetItemCount(hwndList);
	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	for (i=0;i<Count;i++) {
		lvi.iItem=i;
		ListView_GetItem(hwndList,&lvi);
		if (GET_ACCEL_KEY(lvi.lParam)==Key && GET_ACCEL_MOD(lvi.lParam)==Mod)
			return i;
	}
	return -1;
}


void CAccelerator::SetAccelItem(HWND hwndList,int Index,BYTE Mod,WORD Key,bool fGlobal)
{
	LV_ITEM lvi;
	TCHAR szText[64];

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.lParam=MAKE_ACCEL_PARAM(Key,Mod,fGlobal);
	ListView_SetItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	if (Key!=0) {
		FormatAccelText(szText,Key,Mod);
		lvi.pszText=szText;
	} else {
		lvi.pszText=TEXT("");
	}
	ListView_SetItem(hwndList,&lvi);
}


void CAccelerator::SetDlgItemStatus(HWND hDlg)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
	int Sel,Index;
	WORD Key;
	BYTE Mod;

	Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (Sel>=0) {
		LV_ITEM lvi;

		lvi.mask=LVIF_PARAM;
		lvi.iItem=Sel;
		lvi.iSubItem=0;
		ListView_GetItem(hwndList,&lvi);
		Key=GET_ACCEL_KEY(lvi.lParam);
		Mod=GET_ACCEL_MOD(lvi.lParam);
		for (int i=0;i<lengthof(AccelKeyList);i++) {
			if (AccelKeyList[i].KeyCode==(unsigned)Key) {
				::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_SETCURSEL,i+1,0);
				break;
			}
		}
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_SHIFT,(Mod&MOD_SHIFT)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_CONTROL,(Mod&MOD_CONTROL)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_ALT,(Mod&MOD_ALT)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_GLOBAL,GET_ACCEL_GLOBAL(lvi.lParam));
	} else {
		::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_SETCURSEL,0,0);
		for (int i=IDC_ACCELERATOR_SHIFT;i<=IDC_ACCELERATOR_ALT;i++)
			DlgCheckBox_Check(hDlg,i,false);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_GLOBAL,false);
	}
	EnableDlgItems(hDlg,IDC_ACCELERATOR_KEY,IDC_ACCELERATOR_GLOBAL,Sel>=0);
}


CAccelerator *CAccelerator::GetThis(HWND hDlg)
{
	return static_cast<CAccelerator*>(GetOptions(hDlg));
}


BOOL CALLBACK CAccelerator::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAccelerator *pThis=static_cast<CAccelerator*>(OnInitDialog(hDlg,lParam));
			HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
			LV_COLUMN lvc;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=120;
			lvc.pszText=TEXT("コマンド");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("キー");
			ListView_InsertColumn(hwndList,1,&lvc);
			for (int i=0;i<pThis->m_pCommandList->NumCommands();i++) {
				const KeyInfo *pKey=NULL;
				LV_ITEM lvi;
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];

				for (size_t j=0;j<pThis->m_KeyList.size();j++) {
					if (pThis->m_KeyList[j].Command==pThis->m_pCommandList->GetCommandID(i)) {
						pKey=&pThis->m_KeyList[j];
						break;
					}
				}
				pThis->m_pCommandList->GetCommandName(i,szText,lengthof(szText));
				lvi.mask=LVIF_TEXT | LVIF_PARAM;
				lvi.iItem=i;
				lvi.iSubItem=0;
				lvi.pszText=szText;
				if (pKey!=NULL)
					lvi.lParam=MAKE_ACCEL_PARAM(pKey->KeyCode,pKey->Modifiers,pKey->fGlobal);
				else
					lvi.lParam=MAKE_ACCEL_PARAM(0,0,false);
				lvi.iItem=ListView_InsertItem(hwndList,&lvi);
				if (pKey!=NULL) {
					lvi.mask=LVIF_TEXT;
					lvi.iSubItem=1;
					FormatAccelText(szText,pKey->KeyCode,pKey->Modifiers);
					ListView_SetItem(hwndList,&lvi);
				}
			}
			for (int i=0;i<2;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
			::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_ADDSTRING,0,
														(LPARAM)TEXT("なし"));
			for (int i=0;i<lengthof(AccelKeyList);i++)
				::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_ADDSTRING,0,
											(LPARAM)AccelKeyList[i].pszText);
			SetDlgItemStatus(hDlg);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_CHANGECH_FUNCTION,
							  pThis->m_fFunctionKeyChangeChannel);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_CHANGECH_DIGIT,
							  pThis->m_fDigitKeyChangeChannel);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_CHANGECH_NUMPAD,
							  pThis->m_fNumPadChangeChannel);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ACCELERATOR_KEY:
			if (HIWORD(wParam)!=CBN_SELCHANGE)
				break;
		case IDC_ACCELERATOR_SHIFT:
		case IDC_ACCELERATOR_CONTROL:
		case IDC_ACCELERATOR_ALT:
		case IDC_ACCELERATOR_GLOBAL:
			{
				CAccelerator *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
				LV_ITEM lvi;
				int Key;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem<0)
					return TRUE;
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				ListView_GetItem(hwndList,&lvi);
				Key=SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_GETCURSEL,0,0);
				if (Key>0) {
					BYTE Mod;
					int i;

					Mod=0;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_SHIFT))
						Mod|=MOD_SHIFT;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_CONTROL))
						Mod|=MOD_CONTROL;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_ALT))
						Mod|=MOD_ALT;
					i=CheckAccelKey(hwndList,Mod,AccelKeyList[Key-1].KeyCode);
					if (i>=0) {
						if (i!=lvi.iItem) {
							TCHAR szCommand[CCommandList::MAX_COMMAND_NAME],szText[CCommandList::MAX_COMMAND_NAME+128];

							pThis->m_pCommandList->GetCommandName(i,szCommand,lengthof(szCommand));
							::wsprintf(szText,TEXT("既に [%s] に割り当てられています。\r\n割り当て直しますか?"),szCommand);
							if (::MessageBox(hDlg,szText,NULL,
											MB_YESNO | MB_ICONQUESTION)!=IDYES)
								return TRUE;
							SetAccelItem(hwndList,i,0,0,false);
						}
					}
					lvi.lParam=MAKE_ACCEL_PARAM(AccelKeyList[Key-1].KeyCode,Mod,
						DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_GLOBAL));
				} else {
					lvi.lParam=MAKE_ACCEL_PARAM(0,0,false);
				}
				ListView_SetItem(hwndList,&lvi);
				TCHAR szText[64];
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				if (Key>0) {
					FormatAccelText(szText,GET_ACCEL_KEY(lvi.lParam),GET_ACCEL_MOD(lvi.lParam));
					lvi.pszText=szText;
				} else {
					lvi.pszText=TEXT("");
				}
				ListView_SetItem(hwndList,&lvi);
			}
			return TRUE;

		case IDC_ACCELERATOR_DEFAULT:
			{
				CAccelerator *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
				LV_ITEM lvi;
				int NumCommands,i,j;

				NumCommands=pThis->m_pCommandList->NumCommands();
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (i=0;i<NumCommands;i++) {
					WORD Key=0;
					BYTE Mod=0;

					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					for (j=0;j<lengthof(m_DefaultAccelList);j++) {
						if (m_DefaultAccelList[j].Command==pThis->m_pCommandList->GetCommandID(i)) {
							Key=m_DefaultAccelList[j].KeyCode;
							Mod=m_DefaultAccelList[j].Modifiers;
							break;
						}
					}
					if (GET_ACCEL_KEY(lvi.lParam)!=Key
							|| GET_ACCEL_MOD(lvi.lParam)!=Mod)
						SetAccelItem(hwndList,i,Mod,Key,false);
				}
				SetDlgItemStatus(hDlg);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			SetDlgItemStatus(hDlg);
			break;

		case LVN_KEYDOWN:
			{
				LPNMLVKEYDOWN pnmlvk=reinterpret_cast<LPNMLVKEYDOWN>(lParam);

				if (pnmlvk->wVKey==VK_BACK || pnmlvk->wVKey==VK_DELETE) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0)
						SetAccelItem(hwndList,Sel,0,0,false);
				}
			}
			break;

		case PSN_APPLY:
			{
				CAccelerator *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST);
				LV_ITEM lvi;
				int Count,i;

				pThis->UnregisterHotKey();
				pThis->m_KeyList.clear();
				Count=pThis->m_pCommandList->NumCommands();
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (i=0;i<Count;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					if (GET_ACCEL_KEY(lvi.lParam)!=0) {
						KeyInfo Info;

						Info.Command=pThis->m_pCommandList->GetCommandID(i);
						Info.KeyCode=GET_ACCEL_KEY(lvi.lParam);
						Info.Modifiers=GET_ACCEL_MOD(lvi.lParam);
						Info.fGlobal=GET_ACCEL_GLOBAL(lvi.lParam);
						pThis->m_KeyList.push_back(Info);
					}
				}
				HACCEL hAccel=pThis->CreateAccel();
				if (pThis->m_hAccel!=NULL)
					::DestroyAcceleratorTable(pThis->m_hAccel);
				pThis->m_hAccel=hAccel;
				pThis->m_pMainMenu->SetAccelerator(pThis);
				pThis->RegisterHotKey();

				pThis->m_fFunctionKeyChangeChannel=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_CHANGECH_FUNCTION);
				pThis->m_fDigitKeyChangeChannel=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_CHANGECH_DIGIT);
				pThis->m_fNumPadChangeChannel=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_CHANGECH_NUMPAD);
			}
			break;
		}
		break;
	}
	return FALSE;
}

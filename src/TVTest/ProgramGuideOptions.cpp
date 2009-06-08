#include "stdafx.h"
#include <commctrl.h>
#include "TVTest.h"
#include "ProgramGuideOptions.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "resource.h"




CProgramGuideOptions::CProgramGuideOptions(CProgramGuide *pProgramGuide)
{
	m_pProgramGuide=pProgramGuide;
	m_ViewHours=26;
	m_ItemWidth=m_pProgramGuide->GetItemWidth();
	m_LinesPerHour=m_pProgramGuide->GetLinesPerHour();
	m_WheelScrollLines=m_pProgramGuide->GetWheelScrollLines();
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&m_Font);
}


CProgramGuideOptions::~CProgramGuideOptions()
{
}


bool CProgramGuideOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("ProgramGuide"),CSettings::OPEN_READ)) {
		int Value;

		if (Settings.Read(TEXT("ViewHours"),&Value)
				&& Value>=MIN_VIEW_HOURS && Value<=MAX_VIEW_HOURS)
			m_ViewHours=Value;
		if (Settings.Read(TEXT("ItemWidth"),&Value)
				&& Value>=CProgramGuide::MIN_ITEM_WIDTH
				&& Value<=CProgramGuide::MAX_ITEM_WIDTH)
			m_ItemWidth=Value;
		if (Settings.Read(TEXT("LinesPerHour"),&Value)
				&& Value>=CProgramGuide::MIN_LINES_PER_HOUR
				&& Value<=CProgramGuide::MAX_LINES_PER_HOUR)
			m_LinesPerHour=Value;
		m_pProgramGuide->SetUIOptions(m_LinesPerHour,m_ItemWidth);

		if (Settings.Read(TEXT("WheelScrollLines"),&Value))
			m_WheelScrollLines=Value;
		m_pProgramGuide->SetWheelScrollLines(m_WheelScrollLines);

		// Font
		TCHAR szFont[LF_FACESIZE];
		if (Settings.Read(TEXT("FontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
			lstrcpy(m_Font.lfFaceName,szFont);
			m_Font.lfEscapement=0;
			m_Font.lfOrientation=0;
			m_Font.lfUnderline=0;
			m_Font.lfStrikeOut=0;
			m_Font.lfCharSet=DEFAULT_CHARSET;
			m_Font.lfOutPrecision=OUT_DEFAULT_PRECIS;
			m_Font.lfClipPrecision=CLIP_DEFAULT_PRECIS;
			m_Font.lfQuality=DRAFT_QUALITY;
			m_Font.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
		}
		if (Settings.Read(TEXT("FontSize"),&Value)) {
			m_Font.lfHeight=Value;
			m_Font.lfWidth=0;
		}
		if (Settings.Read(TEXT("FontWeight"),&Value))
			m_Font.lfWeight=Value;
		if (Settings.Read(TEXT("FontItalic"),&Value))
			m_Font.lfItalic=Value;
		m_pProgramGuide->SetFont(&m_Font);

		Settings.Close();
	}

	if (Settings.Open(pszFileName,TEXT("ProgramGuideTools"),CSettings::OPEN_READ)) {
		int NumTools;

		if (Settings.Read(TEXT("ToolCount"),&NumTools) && NumTools>0) {
			CProgramGuideToolList *pToolList=m_pProgramGuide->GetToolList();

			for (int i=0;i<NumTools;i++) {
				TCHAR szName[16];
				TCHAR szToolName[CProgramGuideTool::MAX_NAME];
				TCHAR szCommand[CProgramGuideTool::MAX_COMMAND];

				wsprintf(szName,TEXT("Tool%d_Name"),i);
				if (!Settings.Read(szName,szToolName,lengthof(szToolName))
						|| szToolName[0]=='\0')
					break;
				wsprintf(szName,TEXT("Tool%d_Command"),i);
				if (!Settings.Read(szName,szCommand,lengthof(szCommand))
						|| szCommand[0]=='\0')
					break;
				pToolList->Add(new CProgramGuideTool(szToolName,szCommand));
			}
		}
		Settings.Close();
	}
	return true;
}


bool CProgramGuideOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("ProgramGuide"),CSettings::OPEN_WRITE)) {
		Settings.Write(TEXT("ViewHours"),m_ViewHours);
		Settings.Write(TEXT("ItemWidth"),m_ItemWidth);
		Settings.Write(TEXT("LinesPerHour"),m_LinesPerHour);
		Settings.Write(TEXT("WheelScrollLines"),m_WheelScrollLines);

		// Font
		Settings.Write(TEXT("FontName"),m_Font.lfFaceName);
		Settings.Write(TEXT("FontSize"),(int)m_Font.lfHeight);
		Settings.Write(TEXT("FontWeight"),(int)m_Font.lfWeight);
		Settings.Write(TEXT("FontItalic"),(int)m_Font.lfItalic);
		Settings.Close();
	}

	if (Settings.Open(pszFileName,TEXT("ProgramGuideTools"),CSettings::OPEN_WRITE)) {
		const CProgramGuideToolList *pToolList=m_pProgramGuide->GetToolList();

		Settings.Clear();
		Settings.Write(TEXT("ToolCount"),pToolList->NumTools());
		for (int i=0;i<pToolList->NumTools();i++) {
			const CProgramGuideTool *pTool=pToolList->GetTool(i);
			TCHAR szName[16];

			::wsprintf(szName,TEXT("Tool%d_Name"),i);
			Settings.Write(szName,pTool->GetName());
			::wsprintf(szName,TEXT("Tool%d_Command"),i);
			Settings.Write(szName,pTool->GetCommand());
		}
		Settings.Close();
	}
	return true;
}


bool CProgramGuideOptions::GetTimeRange(SYSTEMTIME *pstFirst,SYSTEMTIME *pstLast)
{
	SYSTEMTIME st;

	::GetLocalTime(&st);
	st.wMinute=0;
	st.wSecond=0;
	st.wMilliseconds=0;
	*pstFirst=st;
	OffsetSystemTime(&st,(LONGLONG)m_ViewHours*(60*60*1000));
	*pstLast=st;
	return true;
}


CProgramGuideOptions *CProgramGuideOptions::GetThis(HWND hDlg)
{
	return static_cast<CProgramGuideOptions*>(GetOptions(hDlg));
}


static void SetFontInfo(HWND hDlg,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,IDC_PROGRAMGUIDEOPTIONS_FONTINFO,szText);
	ReleaseDC(hDlg,hdc);
}


BOOL CALLBACK CProgramGuideOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static LOGFONT lfCurFont;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CProgramGuideOptions *pThis=static_cast<CProgramGuideOptions*>(OnInitDialog(hDlg,lParam));

			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS,pThis->m_ViewHours,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS_UD,
								 UDM_SETRANGE32,MIN_VIEW_HOURS,MAX_VIEW_HOURS);
			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH,pThis->m_ItemWidth,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH_UD,
				UDM_SETRANGE32,CProgramGuide::MIN_ITEM_WIDTH,CProgramGuide::MAX_ITEM_WIDTH);
			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR,pThis->m_LinesPerHour,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR_UD,
				UDM_SETRANGE32,CProgramGuide::MIN_LINES_PER_HOUR,CProgramGuide::MAX_LINES_PER_HOUR);
			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES,pThis->m_WheelScrollLines,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES_UD,
				UDM_SETRANGE,0,MAKELONG(UD_MAXVAL,UD_MINVAL));

			lfCurFont=pThis->m_Font;
			SetFontInfo(hDlg,&lfCurFont);

			CProgramGuideToolList *pToolList=pThis->m_pProgramGuide->GetToolList();
			HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
			RECT rc;
			LV_COLUMN lvc;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=80;
			lvc.pszText=TEXT("名前");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.fmt=LVCFMT_LEFT;
			GetClientRect(hwndList,&rc);
			rc.right-=GetSystemMetrics(SM_CXVSCROLL);
			lvc.cx=rc.right-lvc.cx;
			lvc.pszText=TEXT("コマンド");
			ListView_InsertColumn(hwndList,1,&lvc);
			if (pToolList->NumTools()>0) {
				ListView_SetItemCount(hwndList,pToolList->NumTools());
				for (int i=0;i<pToolList->NumTools();i++) {
					CProgramGuideTool *pTool=new CProgramGuideTool(*pToolList->GetTool(i));
					LV_ITEM lvi;

					lvi.mask=LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=i;
					lvi.iSubItem=0;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetName());
					lvi.lParam=reinterpret_cast<LPARAM>(pTool);
					ListView_InsertItem(hwndList,&lvi);
					lvi.mask=LVIF_TEXT;
					lvi.iSubItem=1;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList,&lvi);
				}
				ListView_SetColumnWidth(hwndList,0,LVSCW_AUTOSIZE_USEHEADER);
				int Width=ListView_GetColumnWidth(hwndList,0);
				ListView_SetColumnWidth(hwndList,1,LVSCW_AUTOSIZE_USEHEADER);
				if (ListView_GetColumnWidth(hwndList,1)<rc.right-Width)
					ListView_SetColumnWidth(hwndList,1,rc.right-Width);
				pThis->SetDlgItemState();
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDEOPTIONS_CHOOSEFONT:
			if (ChooseFontDialog(hDlg,&lfCurFont))
				SetFontInfo(hDlg,&lfCurFont);
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_ADD:
			{
				CProgramGuideTool *pTool;

				pTool=new CProgramGuideTool;
				if (pTool->ShowDialog(hDlg)) {
					HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
					LV_ITEM lvi;

					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=ListView_GetItemCount(hwndList);
					lvi.iSubItem=0;
					lvi.state=LVIS_FOCUSED | LVIS_SELECTED;
					lvi.stateMask=lvi.state;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetName());
					lvi.lParam=reinterpret_cast<LPARAM>(pTool);
					ListView_InsertItem(hwndList,&lvi);
					lvi.mask=LVIF_TEXT;
					lvi.iSubItem=1;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList,&lvi);
					ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
				} else {
					delete pTool;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_EDIT:
			{
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask=LVIF_PARAM;
				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				lvi.iSubItem=0;
				if (!ListView_GetItem(hwndList,&lvi))
					return TRUE;
				pTool=reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				if (pTool->ShowDialog(hDlg)) {
					lvi.mask=LVIF_TEXT;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetName());
					ListView_SetItem(hwndList,&lvi);
					lvi.iSubItem=1;
					lvi.pszText=const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList,&lvi);
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_UP:
			{
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask=LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem<=0)
					return TRUE;
				lvi.iSubItem=0;
				ListView_GetItem(hwndList,&lvi);
				pTool=reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				ListView_DeleteItem(hwndList,lvi.iItem);
				lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem--;
				lvi.state=LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask=lvi.state;
				lvi.pszText=const_cast<LPTSTR>(pTool->GetName());
				ListView_InsertItem(hwndList,&lvi);
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				lvi.pszText=const_cast<LPTSTR>(pTool->GetCommand());
				ListView_SetItem(hwndList,&lvi);
				ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_DOWN:
			{
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask=LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem<0
						|| lvi.iItem+1==ListView_GetItemCount(hwndList))
					return TRUE;
				lvi.iSubItem=0;
				ListView_GetItem(hwndList,&lvi);
				pTool=reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				ListView_DeleteItem(hwndList,lvi.iItem);
				lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem++;
				lvi.state=LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask=lvi.state;
				lvi.pszText=const_cast<LPTSTR>(pTool->GetName());
				ListView_InsertItem(hwndList,&lvi);
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				lvi.pszText=const_cast<LPTSTR>(pTool->GetCommand());
				ListView_SetItem(hwndList,&lvi);
				ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_REMOVE:
			{
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;

				lvi.mask=LVIF_PARAM;
				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				lvi.iSubItem=0;
				if (ListView_GetItem(hwndList,&lvi)) {
					CProgramGuideTool *pTool;

					ListView_DeleteItem(hwndList,lvi.iItem);
					pTool=reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
					delete pTool;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_REMOVEALL:
			{
				CProgramGuideOptions *pThis=GetThis(hDlg);
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);

				pThis->DeleteAllTools();
				ListView_DeleteAllItems(hwndList);
				pThis->SetDlgItemState();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			GetThis(hDlg)->SetDlgItemState();
			return TRUE;

		case NM_RCLICK:
			{
				LPNMITEMACTIVATE pnmia=(LPNMITEMACTIVATE)lParam;
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);

				if (pnmia->hdr.hwndFrom==hwndList
						&& ListView_GetNextItem(hwndList,-1,LVNI_SELECTED)>=0) {
					static const int MenuIDs[] = {
						IDC_PROGRAMGUIDETOOL_EDIT,
						0,
						IDC_PROGRAMGUIDETOOL_UP,
						IDC_PROGRAMGUIDETOOL_DOWN,
						0,
						IDC_PROGRAMGUIDETOOL_REMOVE
					};

					PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				CProgramGuideOptions *pThis=GetThis(hDlg);
				int Value;

				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS,NULL,TRUE);
				Value=LimitRange(Value,(int)MIN_VIEW_HOURS,(int)MAX_VIEW_HOURS);
				if (pThis->m_ViewHours!=Value) {
					SYSTEMTIME stFirst,stLast;
					FILETIME ft;

					pThis->m_ViewHours=Value;
					pThis->m_pProgramGuide->GetTimeRange(&stFirst,NULL);
					::SystemTimeToFileTime(&stFirst,&ft);
					ft+=(LONGLONG)pThis->m_ViewHours*(FILETIME_SECOND*60*60);
					pThis->m_pProgramGuide->SetTimeRange(&stFirst,&stLast);
					pThis->m_pProgramGuide->UpdateProgramGuide();
				}
				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH,NULL,TRUE);
				pThis->m_ItemWidth=LimitRange(Value,
					(int)CProgramGuide::MIN_ITEM_WIDTH,(int)CProgramGuide::MAX_ITEM_WIDTH);
				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR,NULL,TRUE);
				pThis->m_LinesPerHour=LimitRange(Value,
					(int)CProgramGuide::MIN_LINES_PER_HOUR,(int)CProgramGuide::MAX_LINES_PER_HOUR);
				pThis->m_pProgramGuide->SetUIOptions(pThis->m_LinesPerHour,pThis->m_ItemWidth);

				pThis->m_WheelScrollLines=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES,NULL,TRUE);
				pThis->m_pProgramGuide->SetWheelScrollLines(pThis->m_WheelScrollLines);

				if (!CompareLogFont(&pThis->m_Font,&lfCurFont)) {
					pThis->m_Font=lfCurFont;
					pThis->m_pProgramGuide->SetFont(&lfCurFont);
				}

				CProgramGuideToolList *pToolList=pThis->m_pProgramGuide->GetToolList();
				HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_LIST);
				int Items,i;

				pToolList->Clear();
				Items=ListView_GetItemCount(hwndList);
				if (Items>0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					for (i=0;i<Items;i++) {
						lvi.iItem=i;
						ListView_GetItem(hwndList,&lvi);
						pToolList->Add(reinterpret_cast<CProgramGuideTool*>(lvi.lParam));
					}
				}
			}
			break;

		case PSN_RESET:
			GetThis(hDlg)->DeleteAllTools();
			break;
		}
		break;

	case WM_DESTROY:
		{
			CProgramGuideOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


void CProgramGuideOptions::SetDlgItemState()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_LIST);
	int Items,Sel;

	Items=ListView_GetItemCount(hwndList);
	Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_EDIT,Sel>=0);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_UP,Sel>0);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_DOWN,Sel>=0 && Sel+1<Items);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_REMOVE,Sel>=0);
	EnableDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_REMOVEALL,Items>0);
}


void CProgramGuideOptions::DeleteAllTools()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_PROGRAMGUIDETOOL_LIST);
	int Items;

	Items=ListView_GetItemCount(hwndList);
	if (Items>0) {
		LV_ITEM lvi;
		int i;
		CProgramGuideTool *pTool;

		lvi.mask=LVIF_PARAM;
		lvi.iSubItem=0;
		for (i=Items-1;i>=0;i--) {
			lvi.iItem=i;
			ListView_GetItem(hwndList,&lvi);
			pTool=reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
			delete pTool;
		}
	}
}

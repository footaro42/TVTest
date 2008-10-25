#include "stdafx.h"
#include "TVTest.h"
#include "StatusOptions.h"
#include "Settings.h"
#include "resource.h"


#define ITEM_MARGIN	2
#define CHECK_WIDTH	14

#define TIMER_ID_UP		1
#define TIMER_ID_DOWN	2




CStatusOptions::CStatusOptions(CStatusView *pStatusView)
{
	m_pStatusView=pStatusView;
	SetDefaultItemList();
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&m_lfItemFont);
}


CStatusOptions::~CStatusOptions()
{
}


bool CStatusOptions::Load(LPCTSTR pszFileName)
{
	CSettings Setting;

	if (Setting.Open(pszFileName,TEXT("Status"),CSettings::OPEN_READ)) {
		int NumItems,i,j;
		StatusItemInfo ItemList[NUM_STATUS_ITEMS];

		if (!Setting.Read(TEXT("NumItems"),&NumItems) || NumItems<1 || NumItems>NUM_STATUS_ITEMS)
			return false;
		for (i=0;i<NumItems;i++) {
			TCHAR szKey[32];

			wsprintf(szKey,TEXT("Item%d_ID"),i);
			if (!Setting.Read(szKey,&ItemList[i].ID))
				return false;
			for (j=0;j<i;j++) {
				if (ItemList[i].ID==ItemList[j].ID)
					return false;
			}
			for (j=0;j<NUM_STATUS_ITEMS;j++) {
				if (ItemList[i].ID==m_ItemList[j].ID)
					break;
			}
			if (j==NUM_STATUS_ITEMS)
				return false;
			wsprintf(szKey,TEXT("Item%d_Visible"),i);
			Setting.Read(szKey,&ItemList[i].fVisible);
			wsprintf(szKey,TEXT("Item%d_Width"),i);
			if (!Setting.Read(szKey,&ItemList[i].Width) || ItemList[i].Width<1)
				ItemList[i].Width=-1;
		}
		if (NumItems<NUM_STATUS_ITEMS) {
			int k;

			k=NumItems;
			for (i=0;i<NUM_STATUS_ITEMS;i++) {
				for (j=0;j<NumItems;j++) {
					if (ItemList[j].ID==m_ItemList[i].ID)
						break;
				}
				if (j==NumItems)
					m_ItemList[k++]=m_ItemList[i];
			}
		}
		for (i=0;i<NumItems;i++)
			m_ItemList[i]=ItemList[i];

		// Font
		TCHAR szFont[LF_FACESIZE];
		int Value;

		if (Setting.Read(TEXT("FontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
			lstrcpy(m_lfItemFont.lfFaceName,szFont);
			m_lfItemFont.lfEscapement=0;
			m_lfItemFont.lfOrientation=0;
			m_lfItemFont.lfUnderline=0;
			m_lfItemFont.lfStrikeOut=0;
			m_lfItemFont.lfCharSet=DEFAULT_CHARSET;
			m_lfItemFont.lfOutPrecision=OUT_DEFAULT_PRECIS;
			m_lfItemFont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
			m_lfItemFont.lfQuality=DRAFT_QUALITY;
			m_lfItemFont.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
		}
		if (Setting.Read(TEXT("FontSize"),&Value)) {
			m_lfItemFont.lfHeight=Value;
			m_lfItemFont.lfWidth=0;
		}
		if (Setting.Read(TEXT("FontWeight"),&Value))
			m_lfItemFont.lfWeight=Value;
		if (Setting.Read(TEXT("FontItalic"),&Value))
			m_lfItemFont.lfItalic=Value;
	} else
		return false;
	return true;
}


bool CStatusOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Setting;

	if (Setting.Open(pszFileName,TEXT("Status"),CSettings::OPEN_WRITE)) {
		int i;

		if (!Setting.Write(TEXT("NumItems"),NUM_STATUS_ITEMS))
			return false;
		for (i=0;i<NUM_STATUS_ITEMS;i++) {
			TCHAR szKey[32];

			wsprintf(szKey,TEXT("Item%d_ID"),i);
			Setting.Write(szKey,m_ItemList[i].ID);
			wsprintf(szKey,TEXT("Item%d_Visible"),i);
			Setting.Write(szKey,m_ItemList[i].fVisible);
			wsprintf(szKey,TEXT("Item%d_Width"),i);
			Setting.Write(szKey,m_ItemList[i].Width);
		}

		// Font
		Setting.Write(TEXT("FontName"),m_lfItemFont.lfFaceName);
		Setting.Write(TEXT("FontSize"),(int)m_lfItemFont.lfHeight);
		Setting.Write(TEXT("FontWeight"),(int)m_lfItemFont.lfWeight);
		Setting.Write(TEXT("FontItalic"),(int)m_lfItemFont.lfItalic);
	} else
		return false;
	return true;
}


void CStatusOptions::SetDefaultItemList()
{
	for (int i=0;i<NUM_STATUS_ITEMS;i++) {
		m_ItemList[i].ID=i;
		m_ItemList[i].fVisible=i<=STATUS_ITEM_ERROR;
		m_ItemList[i].Width=-1;
	}
}


void CStatusOptions::InitListBox(HWND hDlg)
{
	int i;

	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		m_ItemListCur[i]=m_ItemList[i];
		SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_ITEMLIST,LB_ADDSTRING,
								0,reinterpret_cast<LPARAM>(&m_ItemListCur[i]));
	}
}


static void SetFontInfo(HWND hDlg,const LOGFONT *plf)
{
	HDC hdc;
	HFONT hfont,hfontOld;

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	hfont=CreateFontIndirect(plf);
	if (hfont!=NULL) {
		TEXTMETRIC tm;
		TCHAR szText[LF_FACESIZE+16];

		hfontOld=(HFONT)SelectObject(hdc,hfont);
		GetTextMetrics(hdc,&tm);
		wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,
			(tm.tmHeight-tm.tmInternalLeading)*72/GetDeviceCaps(hdc,LOGPIXELSY));
		SetDlgItemText(hDlg,IDC_STATUSOPTIONS_FONTINFO,szText);
		SelectObject(hdc,hfontOld);
		DeleteObject(hfont);
	}
	ReleaseDC(hDlg,hdc);
}


CStatusOptions *CStatusOptions::GetThis(HWND hDlg)
{
	return static_cast<CStatusOptions*>(GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CStatusOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static LOGFONT lfCurFont;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CStatusOptions *pThis=dynamic_cast<CStatusOptions*>(OnInitDialog(hDlg,lParam));

			pThis->m_DropInsertPos=-1;
			pThis->m_DragTimerID=0;
			pThis->InitListBox(hDlg);
			pThis->m_pOldListProc=SubclassWindow(GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST),ItemListProc);
			pThis->m_ItemHeight=pThis->m_pStatusView->GetItemHeight()+1*2+ITEM_MARGIN*2;
			SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_ITEMLIST,LB_SETITEMHEIGHT,
														0,pThis->m_ItemHeight);
			pThis->CalcTextWidth(hDlg);
			pThis->SetListHExtent(hDlg);
			lfCurFont=pThis->m_lfItemFont;
			SetFontInfo(hDlg,&lfCurFont);
		}
		return TRUE;

	/*
	// WM_INITDIALOG‚æ‚è‘O‚É—ˆ‚Ä‚µ‚Ü‚¤‚½‚ß•s‰Â
	case WM_MEASUREITEM:
		{
			CStatusOptions *pThis=GetThis(hDlg);
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			pmis->itemHeight=pThis->m_pStatusView->GetItemHeight()+ITEM_MARGIN*2;
		}
		return TRUE;
	*/

	case WM_DRAWITEM:
		{
			CStatusOptions *pThis=GetThis(hDlg);
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			switch (pdis->itemAction) {
			case ODA_DRAWENTIRE:
			case ODA_SELECT:
				if ((INT)pdis->itemID>=0) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(pdis->itemData);
					CStatusItem *pItem=pThis->m_pStatusView->GetItemByID(pItemInfo->ID);
					int TextColor,BackColor;
					int OldBkMode;
					COLORREF crTextColor,crOldTextColor;
					RECT rc;

					if ((pdis->itemState&ODS_SELECTED)==0) {
						TextColor=COLOR_WINDOWTEXT;
						BackColor=COLOR_WINDOW;
					} else {
						TextColor=COLOR_HIGHLIGHTTEXT;
						BackColor=COLOR_HIGHLIGHT;
					}
					FillRect(pdis->hDC,&pdis->rcItem,(HBRUSH)(BackColor+1));
					OldBkMode=SetBkMode(pdis->hDC,TRANSPARENT);
					crTextColor=GetSysColor(TextColor);
					if (!pItemInfo->fVisible) {
						COLORREF crBack=GetSysColor(BackColor);

						crTextColor=
							RGB((GetRValue(crTextColor)+GetRValue(crBack))/2,
								(GetGValue(crTextColor)+GetGValue(crBack))/2,
								(GetBValue(crTextColor)+GetBValue(crBack))/2);
					}
					crOldTextColor=SetTextColor(pdis->hDC,crTextColor);
					rc.left=pdis->rcItem.left+ITEM_MARGIN;
					rc.top=pdis->rcItem.top+ITEM_MARGIN;
					rc.right=rc.left+CHECK_WIDTH;
					rc.bottom=pdis->rcItem.bottom-ITEM_MARGIN;
					DrawFrameControl(pdis->hDC,&rc,DFC_BUTTON,
						DFCS_BUTTONCHECK | (pItemInfo->fVisible?DFCS_CHECKED:0));
					rc.left=pdis->rcItem.left+ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN;
					rc.top=pdis->rcItem.top+ITEM_MARGIN;
					rc.right=rc.left+pThis->m_TextWidth;
					rc.bottom=pdis->rcItem.bottom-ITEM_MARGIN;
					DrawText(pdis->hDC,pItem->GetName(),-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					rc.left=rc.right+ITEM_MARGIN+1;
					rc.right=rc.left+(pItemInfo->Width>=0?
									pItemInfo->Width:pItem->GetDefaultWidth());
					rc.top=pdis->rcItem.top+((pdis->rcItem.bottom-pdis->rcItem.top)-pThis->m_pStatusView->GetItemHeight())/2;
					rc.bottom=rc.top+pThis->m_pStatusView->GetItemHeight();
					HPEN hpen,hpenOld;
					HBRUSH hbrOld;
					hpen=CreatePen(PS_SOLID,1,crTextColor);
					hpenOld=SelectPen(pdis->hDC,hpen);
					hbrOld=SelectBrush(pdis->hDC,static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)));
					Rectangle(pdis->hDC,rc.left-1,rc.top-1,rc.right+1,rc.bottom+1);
					SelectBrush(pdis->hDC,hbrOld);
					SelectPen(pdis->hDC,hpen);
					pThis->m_pStatusView->DrawItemPreview(pItem,pdis->hDC,&rc);
					SetBkMode(pdis->hDC,OldBkMode);
					SetTextColor(pdis->hDC,crOldTextColor);
					if ((int)pdis->itemID==pThis->m_DropInsertPos
								|| (int)pdis->itemID+1==pThis->m_DropInsertPos)
						PatBlt(pdis->hDC,pdis->rcItem.left,
							(int)pdis->itemID==pThis->m_DropInsertPos?
										pdis->rcItem.top:pdis->rcItem.bottom-1,
							pdis->rcItem.right-pdis->rcItem.left,1,DSTINVERT);
					if ((pdis->itemState&ODS_FOCUS)==0)
						break;
				}
			case ODA_FOCUS:
				DrawFocusRect(pdis->hDC,&pdis->rcItem);
				break;
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STATUSOPTIONS_DEFAULT:
			{
				CStatusOptions *pThis=GetThis(hDlg);

				pThis->SetDefaultItemList();
				SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_ITEMLIST,LB_RESETCONTENT,0,0);
				pThis->InitListBox(hDlg);
				pThis->SetListHExtent(hDlg);
			}
			return TRUE;

		case IDC_STATUSOPTIONS_CHOOSEFONT:
			if (ChooseFontDialog(hDlg,&lfCurFont))
				SetFontInfo(hDlg,&lfCurFont);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CStatusOptions *pThis=GetThis(hDlg);
				StatusItemInfo NewList[NUM_STATUS_ITEMS];
				int i;

				for (i=0;i<NUM_STATUS_ITEMS;i++)
					pThis->m_ItemList[i]=*reinterpret_cast<StatusItemInfo*>(
						SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_ITEMLIST,LB_GETITEMDATA,i,0));
				pThis->ApplyItemList();

				// Font
				if (memcmp(&pThis->m_lfItemFont,&lfCurFont,28/*offsetof(LOGFONT,lfFaceName)*/)!=0
						|| lstrcmp(pThis->m_lfItemFont.lfFaceName,lfCurFont.lfFaceName)!=0) {
					int OldHeight=pThis->m_pStatusView->Height(),NewHeight;

					pThis->m_lfItemFont=lfCurFont;
					pThis->m_pStatusView->SetFont(CreateFontIndirect(&lfCurFont));
					NewHeight=pThis->m_pStatusView->Height();
					if (NewHeight!=OldHeight) {
						HWND hwnd=GetParent(pThis->m_pStatusView->GetHandle());
						RECT rc;

						GetClientRect(hwnd,&rc);
						if (IsZoomed(hwnd)) {
							SendMessage(hwnd,WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
						} else {
							RECT rcWindow;

							GetWindowRect(hwnd,&rcWindow);
							SetWindowPos(hwnd,NULL,0,0,rc.right-rc.left,
								(rcWindow.bottom-rcWindow.top)+(NewHeight-OldHeight),
								SWP_NOZORDER | SWP_NOMOVE);
						}
					}
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CStatusOptions *pThis=GetThis(hDlg);

			SubclassWindow(GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST),
													pThis->m_pOldListProc);
			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


void CStatusOptions::CalcTextWidth(HWND hDlg)
{
	HWND hwndList;
	HDC hdc;
	HFONT hfontOld;
	int Count,i;
	int MaxWidth;
	CStatusItem *pItem;
	SIZE sz;

	hwndList=GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
	hdc=GetDC(hwndList);
	if (hdc==NULL)
		return;
	hfontOld=SelectFont(hdc,GetWindowFont(hwndList));
	Count=ListBox_GetCount(hwndList);
	MaxWidth=0;
	for (i=0;i<Count;i++) {
		pItem=m_pStatusView->GetItemByID(m_ItemList[i].ID);
		GetTextExtentPoint32(hdc,pItem->GetName(),lstrlen(pItem->GetName()),&sz);
		if (sz.cx>MaxWidth)
			MaxWidth=sz.cx;
	}
	SelectFont(hdc,hfontOld);
	ReleaseDC(hwndList,hdc);
	m_TextWidth=MaxWidth;
}


void CStatusOptions::SetListHExtent(HWND hDlg)
{
	HWND hwndList;
	int MaxWidth,Width;
	int i;

	hwndList=GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
	MaxWidth=0;
	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		Width=m_ItemListCur[i].Width>=0?m_ItemListCur[i].Width:
			m_pStatusView->GetItem(m_pStatusView->IDToIndex(m_ItemList[i].ID))->GetWidth();
		if (Width>MaxWidth)
			MaxWidth=Width;
	}
	ListBox_SetHorizontalExtent(hwndList,
		ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN+m_TextWidth+ITEM_MARGIN+MaxWidth+2+ITEM_MARGIN);
}


static int ListBox_GetHitItem(HWND hwndList,int x,int y)
{
	int Index;

	Index=ListBox_GetTopIndex(hwndList)+y/ListBox_GetItemHeight(hwndList,0);
	if (Index<0 || Index>=ListBox_GetCount(hwndList))
		return -1;
	return Index;
}


static void ListBox_MoveItem(HWND hwndList,int From,int To)
{
	int Top=ListBox_GetTopIndex(hwndList);
	LPARAM lData;

	lData=ListBox_GetItemData(hwndList,From);
	ListBox_DeleteString(hwndList,From);
	ListBox_InsertItemData(hwndList,To,lData);
	ListBox_SetCurSel(hwndList,To);
	ListBox_SetTopIndex(hwndList,Top);
}


static void ListBox_EnsureVisible(HWND hwndList,int Index)
{
	int Top;

	Top=ListBox_GetTopIndex(hwndList);
	if (Index<Top) {
		ListBox_SetTopIndex(hwndList,Index);
	} else if (Index>Top) {
		RECT rc;
		int Rows;

		GetClientRect(hwndList,&rc);
		Rows=rc.bottom/ListBox_GetItemHeight(hwndList,0);
		if (Rows==0)
			Rows=1;
		if (Index>=Top+Rows)
			ListBox_SetTopIndex(hwndList,Index-Rows+1);
	}
}


void CStatusOptions::DrawInsertMark(HWND hwndList,int Pos)
{
	HDC hdc;
	RECT rc;

	hdc=GetDC(hwndList);
	GetClientRect(hwndList,&rc);
	rc.top=(Pos-ListBox_GetTopIndex(hwndList))*m_ItemHeight-1;
	PatBlt(hdc,0,rc.top,rc.right-rc.left,2,DSTINVERT);
	ReleaseDC(hwndList,hdc);
}


bool CStatusOptions::GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect)
{
	StatusItemInfo *pItemInfo;
	RECT rc;

	if (Index<0)
		return false;
	pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwndList,Index));
	ListBox_GetItemRect(hwndList,Index,&rc);
	OffsetRect(&rc,-GetScrollPos(hwndList,SB_HORZ),0);
	rc.left+=ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN+m_TextWidth+ITEM_MARGIN+1;
	rc.right=rc.left+(pItemInfo->Width>=0?pItemInfo->Width:
				m_pStatusView->GetItemByID(pItemInfo->ID)->GetDefaultWidth());
	rc.top+=ITEM_MARGIN+1;
	rc.bottom-=ITEM_MARGIN+1;
	*pRect=rc;
	return true;
}


bool CStatusOptions::IsCursorResize(HWND hwndList,int x,int y)
{
	RECT rc;

	if (!GetItemPreviewRect(hwndList,ListBox_GetHitItem(hwndList,x,y),&rc))
		return false;
	return x>=rc.right-2 && x<=rc.right+2;
}


LRESULT CALLBACK CStatusOptions::ItemListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CStatusOptions *pThis=GetThis(GetParent(hwnd));

	switch (uMsg) {
	case WM_LBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int Sel;

			SetFocus(hwnd);
			Sel=ListBox_GetHitItem(hwnd,x,y);
			if (Sel>=0) {
				RECT rc;

				ListBox_GetItemRect(hwnd,Sel,&rc);
				OffsetRect(&rc,-GetScrollPos(hwnd,SB_HORZ),0);
				if (x>=rc.left+ITEM_MARGIN && x<rc.left+ITEM_MARGIN+CHECK_WIDTH) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));
					RECT rc;

					pItemInfo->fVisible=!pItemInfo->fVisible;
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
				} else {
					if (ListBox_GetCurSel(hwnd)!=Sel)
						ListBox_SetCurSel(hwnd,Sel);
					SetCapture(hwnd);
					pThis->m_fDragResize=pThis->IsCursorResize(hwnd,x,y);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			ReleaseCapture();
			if (pThis->m_DragTimerID!=0) {
				KillTimer(hwnd,pThis->m_DragTimerID);
				pThis->m_DragTimerID=0;
			}
			if (pThis->m_DropInsertPos>=0) {
				int From=ListBox_GetCurSel(hwnd),To;

				pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				To=pThis->m_DropInsertPos;
				if (To>From)
					To--;
				SetWindowRedraw(hwnd,FALSE);
				ListBox_MoveItem(hwnd,From,To);
				SetWindowRedraw(hwnd,TRUE);
				pThis->m_DropInsertPos=-1;
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			int y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetClientRect(hwnd,&rc);
			if (pThis->m_fDragResize) {
				int x=GET_X_LPARAM(lParam);
				int Sel=ListBox_GetCurSel(hwnd);
				RECT rc;

				if (pThis->GetItemPreviewRect(hwnd,Sel,&rc)) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));

					pItemInfo->Width=max(x-(int)rc.left,
						pThis->m_pStatusView->GetItemByID(pItemInfo->ID)->GetMinWidth());
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
					pThis->SetListHExtent(GetParent(hwnd));
				}
			} else if (y>=0 && y<rc.bottom) {
				int Insert,Count,Sel;

				if (pThis->m_DragTimerID!=0) {
					KillTimer(hwnd,pThis->m_DragTimerID);
					pThis->m_DragTimerID=0;
				}
				Insert=ListBox_GetTopIndex(hwnd)+
								(y+pThis->m_ItemHeight/2)/pThis->m_ItemHeight;
				Count=ListBox_GetCount(hwnd);
				if (Insert>Count) {
					Insert=Count;
				} else {
					Sel=ListBox_GetCurSel(hwnd);
					if (Insert==Sel || Insert==Sel+1)
						Insert=-1;
				}
				if (pThis->m_DropInsertPos>=0)
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				pThis->m_DropInsertPos=Insert;
				if (pThis->m_DropInsertPos>=0)
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			} else {
				UINT TimerID;

				if (pThis->m_DropInsertPos>=0) {
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
					pThis->m_DropInsertPos=-1;
				}
				if (y<0)
					TimerID=TIMER_ID_UP;
				else
					TimerID=TIMER_ID_DOWN;
				if (TimerID!=pThis->m_DragTimerID) {
					if (pThis->m_DragTimerID!=0)
						KillTimer(hwnd,pThis->m_DragTimerID);
					pThis->m_DragTimerID=SetTimer(hwnd,TimerID,100,NULL);
				}
				SetCursor(LoadCursor(NULL,IDC_NO));
			}
		} else {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			SetCursor(LoadCursor(NULL,pThis->IsCursorResize(hwnd,x,y)?IDC_SIZEWE:IDC_ARROW));
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetCapture()==hwnd) {
			ReleaseCapture();
			if (pThis->m_DragTimerID!=0) {
				KillTimer(hwnd,pThis->m_DragTimerID);
				pThis->m_DragTimerID=0;
			}
			if (pThis->m_DropInsertPos>=0) {
				pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				pThis->m_DropInsertPos=-1;
			}
		}
		return 0;

	case WM_TIMER:
		{
			int Pos;

			Pos=ListBox_GetTopIndex(hwnd);
			if (wParam==TIMER_ID_UP) {
				if (Pos>0)
					Pos--;
			} else
				Pos++;
			ListBox_SetTopIndex(hwnd,Pos);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT)
			return TRUE;
		break;
	}
	return CallWindowProc(pThis->m_pOldListProc,hwnd,uMsg,wParam,lParam);
}


bool CStatusOptions::ApplyItemList()
{
	int i;
	int ItemOrder[NUM_STATUS_ITEMS];
	CStatusItem *pItem;

	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		ItemOrder[i]=m_ItemList[i].ID;
		pItem=m_pStatusView->GetItemByID(ItemOrder[i]);
		pItem->SetVisible(m_ItemList[i].fVisible);
		if (m_ItemList[i].Width>=0)
			pItem->SetWidth(m_ItemList[i].Width);
		else
			pItem->SetWidth(pItem->GetDefaultWidth());
	}
	return m_pStatusView->SetItemOrder(ItemOrder);
}


bool CStatusOptions::ApplyOptions()
{
	ApplyItemList();
	m_pStatusView->SetFont(CreateFontIndirect(&m_lfItemFont));
	return true;
}

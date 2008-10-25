// TsProgGuide.cpp: TS EPGクラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsEncode.h"
#include "TsProgGuide.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




/////////////////////////////////////////////////////////////////////////////
// EITセクションクラス
/////////////////////////////////////////////////////////////////////////////

CEpgItem::CEpgItem()
{
	Reset();
}

CEpgItem::CEpgItem(const CEpgItem &Operand)
{
	*this = Operand;
}

CEpgItem & CEpgItem::operator = (const CEpgItem &Operand)
{
	// インスタンスのコピー
	m_EitItem = Operand.m_EitItem;
	
	return *this;
}

void CEpgItem::Reset(void)
{
	m_EitItem.wServiceID		= 0xFFFFU;
	m_EitItem.wTsID				= 0xFFFFU;
	m_EitItem.wOnID				= 0xFFFFU;
	m_EitItem.wEventID			= 0xFFFFU;
	m_EitItem.dwDuration		= 0UL;
	m_EitItem.byRunningStatus	= 0U;
	m_EitItem.bIsCaService		= false;
	m_EitItem.DescBlock.Reset();

	::ZeroMemory(&m_EitItem.StartTime, sizeof(m_EitItem.StartTime));
}

const WORD CEpgItem::ParseSection(const CPsiSection *pSection, const BYTE *pHexData, const WORD wHexSize)
{
	if(!pSection || !pHexData || !wHexSize)return 0U;

	const BYTE *pPayload = pSection->GetPayloadData();

	// セクション部分を解析する
	m_EitItem.byTableID			= pSection->GetTableID();								// Table ID
	m_EitItem.bySectionNo		= pSection->GetSectionNumber();							// Section Number
	m_EitItem.byLastSectionNo	= pSection->GetLastSectionNumber();						// Last Section Number
	m_EitItem.wServiceID		= pSection->GetTableIdExtension();						// Service ID
	m_EitItem.wTsID				= ((WORD)pPayload[0] << 8) | (WORD)pPayload[1];			// Transport Stream ID
	m_EitItem.wOnID				= ((WORD)pPayload[2] << 8) | (WORD)pPayload[3];			// Original Network ID

	// EITのRepeat部分を解析する
	m_EitItem.wEventID			= ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];			// +0 - +1		EventID
	CAribTime::AribToSystemTime(&pHexData[2], &m_EitItem.StartTime);					// +2 - +6		Start Time
	m_EitItem.dwDuration		= CAribTime::AribBcdToSecond(&pHexData[7]);				// +7 - +9		Duration
	m_EitItem.byRunningStatus	= pHexData[10] >> 5;									// +10 bit7-5	Running Status
	m_EitItem.bIsCaService		= (pHexData[10] & 0x10U)? true : false;					// +10 bit4		Free CA Mode
	const WORD wDescLength		= ((WORD)(pHexData[10] & 0x0FU) << 8) | pHexData[11];	// +10 - +11	Descriptors Loop Length
	m_EitItem.DescBlock.ParseBlock(&pHexData[12], wDescLength);							// +12			Descriptors Field
	
	return wDescLength + 12U;
}

const BYTE CEpgItem::GetTableID(void) const
{
	// Table IDを返す
	return m_EitItem.byTableID;
}

const BYTE CEpgItem::GetSectionNo(void) const
{
	// Section Numberを返す
	return m_EitItem.bySectionNo;
}

const BYTE CEpgItem::GetLastSectionNo(void) const
{
	// Last Section Numberを返す
	return m_EitItem.byLastSectionNo;
}

const DWORD CEpgItem::GetServiceID(void) const
{
	// Service IDを返す
	return m_EitItem.wServiceID;
}

const DWORD CEpgItem::GetTsID(void) const
{
	// Transport Stream IDを返す
	return m_EitItem.wTsID;
}

const DWORD CEpgItem::GetOnID(void) const
{
	// Original Network IDを返す
	return m_EitItem.wOnID;
}

const WORD CEpgItem::GetEventID(void) const
{
	// Event IDを返す
	return m_EitItem.wEventID;
}

const SYSTEMTIME & CEpgItem::GetStartTime(void) const
{
	// Start Timeを返す
	return m_EitItem.StartTime;
}

const DWORD CEpgItem::GetDuration(void) const
{
	// Durationを返す
	return m_EitItem.dwDuration;
}

const BYTE CEpgItem::GetRunningStatus(void) const
{
	// Running Statusを返す
	return m_EitItem.byRunningStatus;
}

const bool CEpgItem::GetFreeCaMode(void) const
{
	// Free CA Modeを返す
	return m_EitItem.bIsCaService;
}

const CDescBlock * CEpgItem::GetDescBlock(void) const
{
	// Descriptor Fieldを返す
	return &m_EitItem.DescBlock;
}

const DWORD CEpgItem::GetEventName(LPTSTR lpszDst) const
{
	// Event Nameを返す
	const CShortEventDesc *pShortEventDesc = dynamic_cast<const CShortEventDesc *>(m_EitItem.DescBlock.GetDescByTag(CShortEventDesc::DESC_TAG));
	return (pShortEventDesc)? pShortEventDesc->GetEventName(lpszDst) : 0UL;
}

const DWORD CEpgItem::GetEventDesc(LPTSTR lpszDst) const
{
	// Event Descriptionを返す
	const CShortEventDesc *pShortEventDesc = dynamic_cast<const CShortEventDesc *>(m_EitItem.DescBlock.GetDescByTag(CShortEventDesc::DESC_TAG));
	return (pShortEventDesc)? pShortEventDesc->GetEventDesc(lpszDst) : 0UL;
}


/////////////////////////////////////////////////////////////////////////////
// EITセクションパーサクラス
/////////////////////////////////////////////////////////////////////////////

CEitParser::CEitParser(IEitHandler *pEitHandler)
	: m_pEitHandler(pEitHandler)
{

}

CEitParser::CEitParser(const CEitParser &Operand)
{
	*this = Operand;
}

CEitParser & CEitParser::operator = (const CEitParser &Operand)
{
	CPsiSingleTable::operator = (Operand);

	m_pEitHandler = Operand.m_pEitHandler;
	m_EpgItem = Operand.m_EpgItem;

	return *this;
}

void CEitParser::Reset(void)
{
	// 状態をクリアする
	CPsiSingleTable::Reset();
	m_EpgItem.Reset();
}

void CEitParser::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	const BYTE *pHexData = pSection->GetPayloadData();
	const WORD wHexSize = pSection->GetPayloadSize();

	WORD wPos = 6U;
	
	// Repeat部分を解析する
	while(wPos < wHexSize){
		const WORD wSize = m_EpgItem.ParseSection(pSection, &pHexData[wPos], wHexSize - wPos);
		
		if(wSize)OnEpgItem(&m_EpgItem);
		else break;

		wPos += wSize;
		}
}

void CEitParser::OnEpgItem(const CEpgItem *pEpgItem)
{
	// ハンドラを呼び出す
	if(m_pEitHandler)m_pEitHandler->OnEpgItem(this, pEpgItem);

//	if(pEpgItem->GetTableID() != 0x4EU)return;
//	if(pEpgItem->GetSectionNo() != 0x00U)return;

	const SYSTEMTIME &StartTime = pEpgItem->GetStartTime();
	const DWORD dwDuration = pEpgItem->GetDuration();
	
	TRACE(TEXT("\n------- EIT Item -------\nSID: %04X, TSID: %04X, ONID: %04X, EVENTID: %04X\n"), pEpgItem->GetServiceID(), pEpgItem->GetTsID(), pEpgItem->GetOnID(), pEpgItem->GetEventID());
	TRACE(TEXT("StartTime: %04u/%02u/%02u %02u:%02u:%02u\n"), StartTime.wYear, StartTime.wMonth, StartTime.wDay, StartTime.wHour, StartTime.wMinute, StartTime.wSecond);
	TRACE(TEXT("Duration : %02u:%02u:%02u\n"), dwDuration / 3600U, (dwDuration % 3600U) / 60U,  (dwDuration % 3600U) % 60U);
	TRACE(TEXT("wDescNum = %u\n"), pEpgItem->GetDescBlock()->GetDescNum());

	TCHAR szEventname[256] = {TEXT('\0')};
	pEpgItem->GetEventName(szEventname);
	TRACE(TEXT("Event Name = %s\n"), szEventname);
	pEpgItem->GetEventDesc(szEventname);
	TRACE(TEXT("Event Desc = %s\n"), szEventname);
}

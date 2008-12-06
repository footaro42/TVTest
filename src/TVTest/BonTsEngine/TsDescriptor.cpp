// TsDescriptor.h: 記述子ラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsEncode.h"
#include "TsDescriptor.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// 記述子の基底クラス
/////////////////////////////////////////////////////////////////////////////

CBaseDesc::CBaseDesc()
{
	Reset();
}

CBaseDesc::CBaseDesc(const CBaseDesc &Operand)
{
	// コピーコンストラクタ
	*this = Operand;
}

CBaseDesc::~CBaseDesc()
{

}

CBaseDesc & CBaseDesc::operator = (const CBaseDesc &Operand)
{
	// 代入演算子
	CopyDesc(&Operand);

	return *this;
}

void CBaseDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	m_byDescTag = pOperand->m_byDescTag;
	m_byDescLen = pOperand->m_byDescLen;
	m_bIsValid = pOperand->m_bIsValid;
}

const bool CBaseDesc::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	Reset();

	// 共通フォーマットをチェック
	if(!pHexData)return false;										// データが空
	else if(wDataLength < 2U)return false;							// データが最低記述子サイズ未満
	else if(wDataLength < (WORD)(pHexData[1] + 2U))return false;	// データが記述子のサイズよりも小さい

	m_byDescTag = pHexData[0];
	m_byDescLen = pHexData[1];

	// ペイロード解析
	if (StoreContents(&pHexData[2])) {
		m_bIsValid = true;
	}

	return m_bIsValid;
}

const bool CBaseDesc::IsValid(void) const
{
	// データが有効(解析済)かどうかを返す
	return m_bIsValid;
}

const BYTE CBaseDesc::GetTag(void) const
{
	// 記述子タグを返す
	return m_byDescTag;
}

const BYTE CBaseDesc::GetLength(void) const
{
	// 記述子長を返す
	return m_byDescLen;
}

void CBaseDesc::Reset(void)
{
	// 状態をクリアする
	m_byDescTag = 0x00U;
	m_byDescLen = 0U;
	m_bIsValid = false;
}

const bool CBaseDesc::StoreContents(const BYTE *pPayload)
{
	// デフォルトの実装では何もしない
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CCaMethodDesc::CCaMethodDesc()
	: CBaseDesc()
{
	Reset();
}

CCaMethodDesc::CCaMethodDesc(const CCaMethodDesc &Operand)
{
	*this = Operand;
}

CCaMethodDesc & CCaMethodDesc::operator = (const CCaMethodDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CCaMethodDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CCaMethodDesc *pSrcDesc = dynamic_cast<const CCaMethodDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_wCaMethodID = pSrcDesc->m_wCaMethodID;
		m_wCaPID = pSrcDesc->m_wCaPID;
		m_PrivateData = pSrcDesc->m_PrivateData;
	}
}

void CCaMethodDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_wCaMethodID = 0x0000U;		// Conditional Access Method ID
	m_wCaPID = 0xFFFFU;				// Conditional Access PID
	m_PrivateData.ClearSize();		// Private Data
}

const WORD CCaMethodDesc::GetCaMethodID(void) const
{
	// Conditional Access Method ID を返す
	return m_wCaMethodID;
}

const WORD CCaMethodDesc::GetCaPID(void) const
{
	// Conditional Access PID
	return m_wCaPID;
}

const CMediaData * CCaMethodDesc::GetPrivateData(void) const
{
	// Private Data を返す
	return &m_PrivateData;
}

const bool CCaMethodDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;							// タグが不正
	else if(m_byDescLen < 4U)return false;								// CAメソッド記述子の最小サイズは4
	else if((pPayload[2] & 0xE0U) != 0xE0U)return false;				// 固定ビットが不正

	// 記述子を解析
	m_wCaMethodID = (WORD)pPayload[0] << 8 | (WORD)pPayload[1];			// +0,1	Conditional Access Method ID
	m_wCaPID = (WORD)(pPayload[2] & 0x1FU) << 8 | (WORD)pPayload[3];	// +2,3	Conditional Access PID
	m_PrivateData.SetData(&pPayload[4], m_byDescLen - 4U);				// +4-	Private Data

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CServiceDesc::CServiceDesc()
	: CBaseDesc()
{
	Reset();
}

CServiceDesc::CServiceDesc(const CServiceDesc &Operand)
{
	*this = Operand;
}

CServiceDesc & CServiceDesc::operator = (const CServiceDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CServiceDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CServiceDesc *pSrcDesc = dynamic_cast<const CServiceDesc *>(pOperand);
	
	if (pSrcDesc && pSrcDesc != this) {
		m_byServiceType = pSrcDesc->m_byServiceType;
		::lstrcpy(m_szProviderName, pSrcDesc->m_szProviderName);
		::lstrcpy(m_szServiceName, pSrcDesc->m_szServiceName);
	}
}

void CServiceDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byServiceType = 0x00U;			// Service Type
	m_szProviderName[0] = TEXT('\0');	// Service Provider Name
	m_szServiceName[0] = TEXT('\0');	// Service Name
}

const BYTE CServiceDesc::GetServiceType(void) const
{
	// Service Typeを返す
	return m_byServiceType;
}

const DWORD CServiceDesc::GetProviderName(LPTSTR lpszDst) const
{
	// Service Provider Nameを返す
	if(lpszDst)::lstrcpy(lpszDst, m_szProviderName);

	return ::lstrlen(m_szProviderName);
}

const DWORD CServiceDesc::GetServiceName(LPTSTR lpszDst) const
{
	// Service Provider Nameを返す
	if(lpszDst)::lstrcpy(lpszDst, m_szServiceName);

	return ::lstrlen(m_szServiceName);
}

const bool CServiceDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen < 3U)return false;		// サービス記述子のサイズは最低3

	// 記述子を解析
	m_byServiceType = pPayload[0];				// +0	Service Type

	BYTE byPos = 1U;

	// Provider Name
	if(pPayload[byPos + 0]){
		CAribString::AribToString(m_szProviderName, &pPayload[byPos + 1], pPayload[byPos + 0]);
		byPos += pPayload[byPos + 0];
		}

	byPos++;

	// Service Name
	if(pPayload[byPos + 0]){
		CAribString::AribToString(m_szServiceName, &pPayload[byPos + 1], pPayload[byPos + 0]);
		byPos += pPayload[byPos + 0];
		}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CShortEventDesc::CShortEventDesc()
	: CBaseDesc()
{
	Reset();
}

CShortEventDesc::CShortEventDesc(const CShortEventDesc &Operand)
{
	*this = Operand;
}

CShortEventDesc & CShortEventDesc::operator = (const CShortEventDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CShortEventDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CShortEventDesc *pSrcDesc = dynamic_cast<const CShortEventDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_dwLanguageCode = pSrcDesc->m_dwLanguageCode;
		::lstrcpy(m_szEventName, pSrcDesc->m_szEventName);
		::lstrcpy(m_szEventDesc, pSrcDesc->m_szEventDesc);
	}
}

void CShortEventDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_dwLanguageCode = 0UL;			// ISO639  Language Code
	m_szEventName[0] = TEXT('\0');	// Event Name
	m_szEventDesc[0] = TEXT('\0');	// Event Description
}

const DWORD CShortEventDesc::GetLanguageCode(void) const
{
	// Language Codeを返す
	return m_dwLanguageCode;
}

const DWORD CShortEventDesc::GetEventName(LPTSTR lpszDst) const
{
	// Event Nameを返す
	if(lpszDst)::lstrcpy(lpszDst, m_szEventName);

	return ::lstrlen(m_szEventName);
}

const DWORD CShortEventDesc::GetEventDesc(LPTSTR lpszDst) const
{
	// Event Descriptionを返す
	if(lpszDst)::lstrcpy(lpszDst, m_szEventDesc);

	return ::lstrlen(m_szEventDesc);
}

const bool CShortEventDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen < 5U)return false;		// Short Event記述子のサイズは最低5

	// 記述子を解析
	m_dwLanguageCode = ((DWORD)pPayload[0] << 16) | ((DWORD)pPayload[1] << 8) | (DWORD)pPayload[2];		// +0 - +2	ISO639  Language Code

	BYTE byPos = 3U;

	// Event Name
	if(pPayload[byPos + 0]){
		CAribString::AribToString(m_szEventName, &pPayload[byPos + 1], pPayload[byPos + 0]);
		byPos += pPayload[byPos + 0];
		}

	byPos++;

	// Event Description
	if(pPayload[byPos + 0]){
		CAribString::AribToString(m_szEventDesc, &pPayload[byPos + 1], pPayload[byPos + 0]);
		byPos += pPayload[byPos + 0];
		}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x52] Stream Identifier 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CStreamIdDesc::CStreamIdDesc()
	: CBaseDesc()
{
	Reset();
}

CStreamIdDesc::CStreamIdDesc(const CStreamIdDesc &Operand)
{
	*this = Operand;
}

CStreamIdDesc & CStreamIdDesc::operator = (const CStreamIdDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CStreamIdDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CStreamIdDesc *pSrcDesc = dynamic_cast<const CStreamIdDesc *>(pOperand);

	if (pSrcDesc) {
		m_byComponentTag = pSrcDesc->m_byComponentTag;
	}
}

void CStreamIdDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byComponentTag = 0x00U;	// Component Tag
}

const BYTE CStreamIdDesc::GetComponentTag(void) const
{
	// Component Tag を返す
	return m_byComponentTag;
}

const bool CStreamIdDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen != 1U)return false;		// ストリームID記述子のサイズは常に1

	// 記述子を解析
	m_byComponentTag = pPayload[0];				// +0	Component Tag

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CNetworkNameDesc::CNetworkNameDesc()
	: CBaseDesc()
{
	Reset();
}

CNetworkNameDesc::CNetworkNameDesc(const CNetworkNameDesc &Operand)
{
	*this = Operand;
}

CNetworkNameDesc & CNetworkNameDesc::operator = (const CNetworkNameDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CNetworkNameDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CNetworkNameDesc *pSrcDesc = dynamic_cast<const CNetworkNameDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		::lstrcpy(m_szNetworkName, pSrcDesc->m_szNetworkName);
	}
}

void CNetworkNameDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_szNetworkName[0] = '\0';
}

const DWORD CNetworkNameDesc::GetNetworkName(LPTSTR pszName, int MaxLength) const
{
	if (pszName)
		::lstrcpyn(pszName, m_szNetworkName, MaxLength);
	return ::lstrlen(m_szNetworkName);
}

const bool CNetworkNameDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	CAribString::AribToString(m_szNetworkName, &pPayload[0], m_byDescLen);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xFE] System Management 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSystemManageDesc::CSystemManageDesc()
	: CBaseDesc()
{
	Reset();
}

CSystemManageDesc::CSystemManageDesc(const CSystemManageDesc &Operand){
	*this = Operand;
}

CSystemManageDesc & CSystemManageDesc::operator = (const CSystemManageDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CSystemManageDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CSystemManageDesc *pSrcDesc = dynamic_cast<const CSystemManageDesc *>(pOperand);

	if (pSrcDesc) {
		m_byBroadcastingFlag = pSrcDesc->m_byBroadcastingFlag;
		m_byBroadcastingID = pSrcDesc->m_byBroadcastingID;
		m_byAdditionalBroadcastingID = pSrcDesc->m_byAdditionalBroadcastingID;
	}
}

void CSystemManageDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byBroadcastingFlag = 0;
	m_byBroadcastingID = 0;
	m_byAdditionalBroadcastingID = 0;
}

const BYTE CSystemManageDesc::GetBroadcastingFlag(void) const
{
	return m_byBroadcastingFlag;
}

const BYTE CSystemManageDesc::GetBroadcastingID(void) const
{
	return m_byBroadcastingID;
}

const BYTE CSystemManageDesc::GetAdditionalBroadcastingID(void) const
{
	return m_byAdditionalBroadcastingID;
}

const bool CSystemManageDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byBroadcastingFlag = (pPayload[0]&0xC0)>>6;
	m_byBroadcastingID = (pPayload[0]&0x3F);
	m_byAdditionalBroadcastingID = pPayload[1];

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CTSInfoDesc::CTSInfoDesc()
	: CBaseDesc()
{
	Reset();
}

CTSInfoDesc::CTSInfoDesc(const CTSInfoDesc &Operand)
{
	*this = Operand;
}

CTSInfoDesc & CTSInfoDesc::operator = (const CTSInfoDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CTSInfoDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CTSInfoDesc *pSrcDesc = dynamic_cast<const CTSInfoDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_byRemoteControlKeyID = pSrcDesc->m_byRemoteControlKeyID;
		::lstrcpy(m_szTSName, pSrcDesc->m_szTSName);
	}
}

void CTSInfoDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byRemoteControlKeyID = 0;
	m_szTSName[0] = '\0';
}

const BYTE CTSInfoDesc::GetRemoteControlKeyID(void) const
{
	return m_byRemoteControlKeyID;
}

const DWORD CTSInfoDesc::GetTSName(LPTSTR pszName, int MaxLength) const
{
	if (pszName)
		::lstrcpyn(pszName, m_szTSName, MaxLength);
	return ::lstrlen(m_szTSName);
}

const bool CTSInfoDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	BYTE Length = (pPayload[1]&0xFC)>>2;
	if (m_byDescLen < 2 + Length)
		return false;

	m_byRemoteControlKeyID = pPayload[0];
	CAribString::AribToString(m_szTSName, &pPayload[2], Length);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CAudioComponentDesc::CAudioComponentDesc()
	: CBaseDesc()
{
	Reset();
}

CAudioComponentDesc::CAudioComponentDesc(const CAudioComponentDesc &Operand)
{
	*this = Operand;
}

CAudioComponentDesc & CAudioComponentDesc::operator = (const CAudioComponentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CAudioComponentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CAudioComponentDesc *pSrcDesc = dynamic_cast<const CAudioComponentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_StreamContent=pSrcDesc->m_StreamContent;
		m_ComponentType=pSrcDesc->m_ComponentType;
		m_ComponentTag=pSrcDesc->m_ComponentTag;
		m_StreamType=pSrcDesc->m_StreamType;
		m_SimulcastGroupTag=pSrcDesc->m_SimulcastGroupTag;
		m_bESMultiLingualFlag=pSrcDesc->m_bESMultiLingualFlag;
		m_bMainComponentFlag=pSrcDesc->m_bMainComponentFlag;
		m_QualityIndicator=pSrcDesc->m_QualityIndicator;
		m_SamplingRate=pSrcDesc->m_SamplingRate;
		::lstrcpy(m_szText,pSrcDesc->m_szText);
	}
}

void CAudioComponentDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_StreamContent=0;
	m_ComponentType=0;
	m_ComponentTag=0;
	m_StreamType=0;
	m_SimulcastGroupTag=0;
	m_bESMultiLingualFlag=false;
	m_bMainComponentFlag=false;
	m_QualityIndicator=0;
	m_SamplingRate=0;
	m_szText[0]='\0';
}

const BYTE CAudioComponentDesc::GetStreamContent(void) const
{
	return m_StreamContent;
}

const BYTE CAudioComponentDesc::GetComponentType(void) const
{
	return m_ComponentType;
}

const BYTE CAudioComponentDesc::GetComponentTag(void) const
{
	return m_ComponentTag;
}

const BYTE CAudioComponentDesc::GetSimulcastGroupTag(void) const
{
	return m_SimulcastGroupTag;
}

const bool CAudioComponentDesc::GetESMultiLingualFlag(void) const
{
	return m_bESMultiLingualFlag;
}

const bool CAudioComponentDesc::GetMainComponentFlag(void) const
{
	return m_bMainComponentFlag;
}

const BYTE CAudioComponentDesc::GetQualityIndicator(void) const
{
	return m_QualityIndicator;
}

const BYTE CAudioComponentDesc::GetSamplingRate(void) const
{
	return m_SamplingRate;
}

LPCTSTR CAudioComponentDesc::GetText(void) const
{
	return m_szText;
}

const bool CAudioComponentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 8)
		return false;

	m_StreamContent=pPayload[0]&0x0F;
	m_ComponentType=pPayload[1];
	m_ComponentTag=pPayload[2];
	m_StreamType=pPayload[3];
	m_SimulcastGroupTag=pPayload[4];
	m_bESMultiLingualFlag=(pPayload[5]&0x80)!=0;
	m_bMainComponentFlag=(pPayload[5]&0x40)!=0;
	m_QualityIndicator=(pPayload[5]&0x30)>>4;
	m_SamplingRate=(pPayload[5]&0x0E)>>1;
	DWORD Pos=6+3;	// ISO_639_language_code
	if (m_bESMultiLingualFlag)
		Pos+=3;	// ISO_639_language_code_2
	if ((DWORD)m_byDescLen-2 > Pos)
		CAribString::AribToString(m_szText, &pPayload[Pos], m_byDescLen - Pos);
	else
		m_szText[0]='\0';
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// 記述子ブロック抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CDescBlock::CDescBlock()
{

}

CDescBlock::CDescBlock(const CDescBlock &Operand)
{
	*this = Operand;
}

CDescBlock::~CDescBlock()
{
	Reset();
}

CDescBlock & CDescBlock::operator = (const CDescBlock &Operand)
{
	if (&Operand == this)
		return *this;

	// インスタンスのコピー
	Reset();
	m_DescArray.resize(Operand.m_DescArray.size());

	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++){
		m_DescArray[Index] = CreateDescInstance(Operand.m_DescArray[Index]->GetTag());
		m_DescArray[Index]->CopyDesc(Operand.m_DescArray[Index]);
	}

	return *this;
}

const WORD CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return 0U;

	// 状態をクリア
	Reset();

	// 指定されたブロックに含まれる記述子を解析する
	WORD wPos = 0UL;
	CBaseDesc *pNewDesc = NULL;

	while (wPos < wDataLength) {
		// ブロックを解析する
		if (!(pNewDesc = ParseDesc(&pHexData[wPos], wDataLength - wPos)))
			break;

		// リストに追加する
		m_DescArray.push_back(pNewDesc);

		// 位置更新
		wPos += (pNewDesc->GetLength() + 2U);
	}

	return m_DescArray.size();
}

const CBaseDesc * CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag)
{
	// 指定されたブロックに含まれる記述子を解析して指定されたタグの記述子を返す
	return (ParseBlock(pHexData, wDataLength))? GetDescByTag(byTag) : NULL;
}

void CDescBlock::Reset(void)
{
	// 全てのインスタンスを開放する
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++){
		delete m_DescArray[Index];
	}

	m_DescArray.clear();
}

const WORD CDescBlock::GetDescNum(void) const
{
	// 記述子の数を返す
	return m_DescArray.size();
}

const CBaseDesc * CDescBlock::GetDescByIndex(const WORD wIndex) const
{
	// インデックスで指定した記述子を返す
	return (wIndex < m_DescArray.size())? m_DescArray[wIndex] : NULL;
}

const CBaseDesc * CDescBlock::GetDescByTag(const BYTE byTag) const
{
	// 指定したタグに一致する記述子を返す
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++){
		if (m_DescArray[Index]->GetTag() == byTag)
			return m_DescArray[Index];
	}

	return NULL;
}

CBaseDesc * CDescBlock::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return NULL;

	// タグに対応したインスタンスを生成する
	CBaseDesc *pNewDesc = CreateDescInstance(pHexData[0]);

	/*
	// メモリ不足
	if(!pNewDesc)return NULL;
	*/

	// 記述子を解析する
	if (!pNewDesc->ParseDesc(pHexData, wDataLength)) {
		// エラーあり
		delete pNewDesc;
		return NULL;
	}

	return pNewDesc;
}

CBaseDesc * CDescBlock::CreateDescInstance(const BYTE byTag)
{
	// タグに対応したインスタンスを生成する
	switch (byTag) {
	case CCaMethodDesc::DESC_TAG		: return new CCaMethodDesc;
	case CServiceDesc::DESC_TAG			: return new CServiceDesc;
	case CShortEventDesc::DESC_TAG		: return new CShortEventDesc;
	case CStreamIdDesc::DESC_TAG		: return new CStreamIdDesc;
	case CNetworkNameDesc::DESC_TAG		: return new CNetworkNameDesc;
	case CSystemManageDesc::DESC_TAG	: return new CSystemManageDesc;
	case CTSInfoDesc::DESC_TAG			: return new CTSInfoDesc;
	case CAudioComponentDesc::DESC_TAG	: return new CAudioComponentDesc;
	default								: return new CBaseDesc;
	}
}

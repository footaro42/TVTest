// CasProcessor.cpp: CCasProcessor クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Common.h"
#include "CasProcessor.h"
#include "TsStream.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CCasProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CCasProcessor::CCasProcessor(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_hCasLib(NULL)
	, m_pCasManager(NULL)

	, m_bEnableDescramble(true)
	, m_bEnableContract(false)
	, m_Instruction(0)

	, m_InputPacketCount(0)
	, m_ScramblePacketCount(0)
{
}

CCasProcessor::~CCasProcessor()
{
	FreeCasLibrary();
}

void CCasProcessor::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_pCasManager != NULL)
		m_pCasManager->Reset();

	m_InputPacketCount = 0;
	m_ScramblePacketCount = 0;
}

const bool CCasProcessor::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	m_InputPacketCount++;

	CTsPacket *pPacket = static_cast<CTsPacket *>(pMediaData);

	if (m_pCasManager != NULL) {
		m_pCasManager->ProcessPacket(pPacket->GetData(), pPacket->GetSize());
		pPacket->m_Header.byTransportScramblingCtrl = pPacket->GetAt(3) >> 6;
	} else {
		if (pPacket->IsScrambled() && m_bEnableDescramble)
			m_ScramblePacketCount++;
	}

	// パケットを下流デコーダにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

bool CCasProcessor::LoadCasLibrary(LPCTSTR pszFileName)
{
	if (pszFileName == NULL || pszFileName[0] == '\0') {
		SetError(TEXT("CASライブラリのファイル名が指定されていません。"));
		return false;
	}

	CBlockLock Lock(&m_DecoderLock);

	FreeCasLibrary();

	m_hCasLib = ::LoadLibrary(pszFileName);
	if (m_hCasLib == NULL) {
		const DWORD ErrorCode = ::GetLastError();
		TCHAR szText[MAX_PATH + 32];
		StdUtil::snprintf(szText, _countof(szText),
			TEXT("CASライブラリ \"%s\" をロードできません。"), pszFileName);
		SetError(szText);
		switch (ErrorCode) {
		case ERROR_MOD_NOT_FOUND:
			SetErrorAdvise(TEXT("ファイルが見つかりません。"));
			break;
		case ERROR_BAD_EXE_FORMAT:
			SetErrorAdvise(
#ifndef _WIN64
				TEXT("32")
#else
				TEXT("64")
#endif
				TEXT("ビット用のDLLではないか、ファイルが破損している可能性があります。"));
			break;
		default:
			StdUtil::snprintf(szText, _countof(szText), TEXT("エラーコード: 0x%x"), ErrorCode);
			SetErrorAdvise(szText);
		}
		return false;
	}

	TVCAS::GetModuleInfoFunc pGetModuleInfo = TVCAS::Helper::Module::GetModuleInfo(m_hCasLib);
	if (pGetModuleInfo == NULL
			|| !pGetModuleInfo(&m_CasModuleInfo)
			|| m_CasModuleInfo.LibVersion != TVCAS::LIB_VERSION) {
		FreeCasLibrary();
		SetError(
			pGetModuleInfo == NULL ?
			TEXT("指定されたDLLがCASライブラリではありません。") :
			TEXT("CASライブラリのバージョンが非対応です。"));
		return false;
	}

	TVCAS::CreateInstanceFunc pCreateInstance = TVCAS::Helper::Module::CreateInstance(m_hCasLib);
	if (pCreateInstance == NULL) {
		FreeCasLibrary();
		SetError(TEXT("CASライブラリから必要な関数を取得できません。"));
		return false;
	}

	m_pCasManager = static_cast<TVCAS::ICasManager *>(pCreateInstance(__uuidof(TVCAS::ICasManager)));
	if (m_pCasManager == NULL) {
		FreeCasLibrary();
		SetError(TEXT("CASマネージャのインスタンスを作成できません。"));
		return false;
	}

	if (!m_pCasManager->Initialize(this)) {
		FreeCasLibrary();
		SetError(TEXT("CASマネージャの初期化ができません。"));
		return false;
	}

	m_pCasManager->EnableDescramble(m_bEnableDescramble);
	m_pCasManager->EnableContract(m_bEnableContract);
	m_pCasManager->SetInstruction(m_Instruction & m_pCasManager->GetAvailableInstructions());

	ClearError();

	return true;
}

bool CCasProcessor::FreeCasLibrary()
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_pCasManager != NULL) {
		m_pCasManager->Release();
		m_pCasManager = NULL;
	}

	if (m_hCasLib != NULL) {
		::FreeLibrary(m_hCasLib);
		m_hCasLib = NULL;
	}

	return true;
}

bool CCasProcessor::IsCasLibraryLoaded() const
{
	return m_hCasLib != NULL;
}

bool CCasProcessor::GetCasModuleInfo(CasModuleInfo *pInfo) const
{
	if (m_pCasManager == NULL || pInfo == NULL)
		return false;

	*pInfo = m_CasModuleInfo;

	return true;
}

bool CCasProcessor::EnableDescramble(bool bEnable)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_pCasManager != NULL) {
		if (!m_pCasManager->EnableDescramble(bEnable))
			return false;
	}

	m_bEnableDescramble = bEnable;

	return true;
}

bool CCasProcessor::EnableContract(bool bEnable)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_pCasManager != NULL) {
		if (!m_pCasManager->EnableContract(bEnable))
			return false;
	}

	m_bEnableContract = bEnable;

	return true;
}

int CCasProcessor::GetCasDeviceCount() const
{
	if (m_pCasManager == NULL)
		return 0;

	return m_pCasManager->GetCasDeviceCount();
}

bool CCasProcessor::GetCasDeviceInfo(int Device, CasDeviceInfo *pInfo) const
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->GetCasDeviceInfo(Device, pInfo);
}

bool CCasProcessor::GetCasDeviceCardList(int Device, StringList *pList)
{
	if (m_pCasManager == NULL || pList == NULL)
		return false;

	pList->clear();

	TVCAS::ICasDevice *pDevice = m_pCasManager->OpenCasDevice(Device);
	if (pDevice == NULL)
		return false;

	WCHAR szName[256];
	for (int i = 0; pDevice->GetCardName(i, szName, _countof(szName)); i++) {
		pList->push_back(szName);
	}

	pDevice->Release();

	return true;
}

bool CCasProcessor::IsCasDeviceAvailable(int Device)
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->IsCasDeviceAvailable(Device);
}

bool CCasProcessor::CheckCasDeviceAvailability(int Device, bool *pbAvailable, LPWSTR pszMessage, int MaxLength)
{
	if (pszMessage == NULL || MaxLength < 1)
		return false;
	if (m_pCasManager == NULL) {
		pszMessage[0] = L'\0';
		return false;
	}

	return m_pCasManager->CheckCasDeviceAvailability(Device, pbAvailable, pszMessage, MaxLength);
}

int CCasProcessor::GetDefaultCasDevice()
{
	if (m_pCasManager == NULL)
		return -1;

	return m_pCasManager->GetDefaultCasDevice();
}

int CCasProcessor::GetCasDeviceByID(DWORD DeviceID) const
{
	if (m_pCasManager == NULL)
		return -1;

	return m_pCasManager->GetCasDeviceByID(DeviceID);
}

int CCasProcessor::GetCasDeviceByName(LPCWSTR pszName) const
{
	if (m_pCasManager == NULL)
		return -1;

	return m_pCasManager->GetCasDeviceByName(pszName);
}

bool CCasProcessor::OpenCasCard(int Device, LPCWSTR pszName)
{
	if (m_pCasManager == NULL)
		return false;

	ClearError();

	if (!m_pCasManager->OpenCasCard(Device, pszName))
		return false;

	return true;
}

bool CCasProcessor::CloseCasCard()
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->CloseCasCard();
}

bool CCasProcessor::IsCasCardOpen() const
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->IsCasCardOpen();
}

int CCasProcessor::GetCasDevice() const
{
	if (m_pCasManager == NULL)
		return -1;

	return m_pCasManager->GetCasDevice();
}

int CCasProcessor::GetCasCardName(LPWSTR pszName, int MaxName) const
{
	if (m_pCasManager == NULL)
		return 0;

	return m_pCasManager->GetCasCardName(pszName, MaxName);
}

bool CCasProcessor::GetCasCardInfo(CasCardInfo *pInfo) const
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->GetCasCardInfo(pInfo);
}

bool CCasProcessor::SendCasCommand(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize)
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->SendCasCommand(pSendData, SendSize, pRecvData, pRecvSize);
}

ULONGLONG CCasProcessor::GetInputPacketCount() const
{
	return m_InputPacketCount;
}

ULONGLONG CCasProcessor::GetScramblePacketCount() const
{
	ULONGLONG Count = m_ScramblePacketCount;

	if (m_pCasManager != NULL)
		Count += m_pCasManager->GetScramblePacketCount();

	return Count;
}

void CCasProcessor::ResetScramblePacketCount()
{
	m_ScramblePacketCount = 0;

	if (m_pCasManager != NULL)
		m_pCasManager->ResetScramblePacketCount();
}

bool CCasProcessor::SetTargetServiceID(WORD ServiceID)
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->SetDescrambleServiceID(ServiceID);
}

WORD CCasProcessor::GetTargetServiceID() const
{
	if (m_pCasManager == NULL)
		return 0;

	return m_pCasManager->GetDescrambleServiceID();
}

WORD CCasProcessor::GetEcmPIDByServiceID(WORD ServiceID) const
{
	if (m_pCasManager == NULL)
		return 0xFFFF;

	return m_pCasManager->GetEcmPIDByServiceID(ServiceID);
}

CCasProcessor::ContractStatus CCasProcessor::GetContractStatus(WORD NetworkID, WORD ServiceID, const SYSTEMTIME *pTime)
{
	if (m_pCasManager == NULL)
		return CONTRACT_ERROR;

	return (ContractStatus)m_pCasManager->GetContractStatus(NetworkID, ServiceID, pTime);
}

CCasProcessor::ContractStatus CCasProcessor::GetContractPeriod(WORD NetworkID, WORD ServiceID, SYSTEMTIME *pTime)
{
	if (m_pCasManager == NULL)
		return CONTRACT_ERROR;

	return (ContractStatus)m_pCasManager->GetContractPeriod(NetworkID, ServiceID, pTime);
}

bool CCasProcessor::HasContractInfo(WORD NetworkID, WORD ServiceID) const
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->HasContractInfo(NetworkID, ServiceID);
}

int CCasProcessor::GetInstructionName(int Instruction, LPWSTR pszName, int MaxName) const
{
	if (m_pCasManager == NULL)
		return 0;

	return m_pCasManager->GetInstructionName(Instruction, pszName, MaxName);
}

UINT CCasProcessor::GetAvailableInstructions() const
{
	if (m_pCasManager == NULL)
		return 0;

	return m_pCasManager->GetAvailableInstructions();
}

bool CCasProcessor::SetInstruction(int Instruction)
{
	if (m_pCasManager != NULL)
		return m_pCasManager->SetInstruction(Instruction);

	m_Instruction = Instruction;

	return true;
}

int CCasProcessor::GetInstruction() const
{
	if (m_pCasManager != NULL)
		return m_pCasManager->GetInstruction();

	return m_Instruction;
}

bool CCasProcessor::DescrambleBenchmarkTest(int Instruction, DWORD Round, DWORD *pTime)
{
	if (m_pCasManager == NULL)
		return false;

	return m_pCasManager->DescrambleBenchmarkTest(Instruction, Round, pTime);
}

LPCWSTR CCasProcessor::GetName() const
{
	return L"CasProcessor";
}

LRESULT CCasProcessor::OnEvent(UINT Event, void *pParam)
{
	switch (Event) {
	case TVCAS::EVENT_EMM_PROCESSED:
		SendDecoderEvent(EVENT_EMM_PROCESSED);
		return 0;

	case TVCAS::EVENT_EMM_ERROR:
		SendDecoderEvent(EVENT_EMM_ERROR, pParam);
		return 0;

	case TVCAS::EVENT_ECM_ERROR:
		SendDecoderEvent(EVENT_ECM_ERROR, pParam);
		return 0;

	case TVCAS::EVENT_ECM_REFUSED:
		SendDecoderEvent(EVENT_ECM_REFUSED, pParam);
		return 0;

	case TVCAS::EVENT_CARD_READER_HUNG:
		SendDecoderEvent(EVENT_CARD_READER_HUNG);
		return 0;
	}

	return 0;
}

LRESULT CCasProcessor::OnError(const TVCAS::ErrorInfo *pInfo)
{
	SetError(pInfo->Code, pInfo->pszText, pInfo->pszAdvise, pInfo->pszSystemMessage);

	return 0;
}

void CCasProcessor::OutLog(TVCAS::LogType Type, LPCWSTR pszMessage)
{
	Trace(pszMessage);
}

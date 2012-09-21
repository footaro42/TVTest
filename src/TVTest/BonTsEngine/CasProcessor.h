// CasProcessor.h: CCasProcessor クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <string>
#include "MediaDecoder.h"
#include "TsUtilClass.h"
#include "TVCAS.h"


class CCasProcessor
	: public CMediaDecoder
	, public TVCAS::ICasClient
	, protected TVCAS::Helper::CBaseImplNoRef
{
public:
	enum {
		EVENT_EMM_PROCESSED    = 0x00000001UL,
		EVENT_EMM_ERROR        = 0x00000002UL,
		EVENT_ECM_ERROR        = 0x00000003UL,
		EVENT_ECM_REFUSED      = 0x00000004UL,
		EVENT_CARD_READER_HUNG = 0x00000005UL
	};

	typedef TVCAS::ModuleInfo CasModuleInfo;
	typedef TVCAS::EcmErrorInfo EcmErrorInfo;
	typedef TVCAS::EmmErrorInfo EmmErrorInfo;
	typedef TVCAS::CasDeviceInfo CasDeviceInfo;
	typedef TVCAS::CasCardInfo CasCardInfo;

	enum ContractStatus {
		CONTRACT_CONTRACTED,
		CONTRACT_UNCONTRACTED,
		CONTRACT_UNKNOWN,
		CONTRACT_ERROR
	};

	typedef std::vector<std::wstring> StringList;

	CCasProcessor(IEventHandler *pEventHandler = NULL);
	virtual ~CCasProcessor();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CCasProcessor
	bool LoadCasLibrary(LPCTSTR pszFileName);
	bool FreeCasLibrary();
	bool IsCasLibraryLoaded() const;
	bool GetCasModuleInfo(CasModuleInfo *pInfo) const;

	bool EnableDescramble(bool bEnable);
	bool EnableContract(bool bEnable);

	int GetCasDeviceCount() const;
	bool GetCasDeviceInfo(int Device, CasDeviceInfo *pInfo) const;
	bool GetCasDeviceCardList(int Device, StringList *pList);
	bool IsCasDeviceAvailable(int Device);
	bool CheckCasDeviceAvailability(int Device, bool *pbAvailable, LPWSTR pszMessage, int MaxLength);
	int GetDefaultCasDevice();
	int GetCasDeviceByID(DWORD DeviceID) const;
	int GetCasDeviceByName(LPCWSTR pszName) const;

	bool OpenCasCard(int Device, LPCWSTR pszName = NULL);
	bool CloseCasCard();
	bool IsCasCardOpen() const;
	int GetCasDevice() const;
	int GetCasCardName(LPWSTR pszName, int MaxName) const;
	bool GetCasCardInfo(CasCardInfo *pInfo) const;
	bool SendCasCommand(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize);

	ULONGLONG GetInputPacketCount() const;
	ULONGLONG GetScramblePacketCount() const;
	void ResetScramblePacketCount();

	bool SetTargetServiceID(WORD ServiceID = 0);
	WORD GetTargetServiceID() const;
	WORD GetEcmPIDByServiceID(WORD ServiceID) const;

	ContractStatus GetContractStatus(WORD NetworkID, WORD ServiceID, const SYSTEMTIME *pTime = NULL);
	ContractStatus GetContractPeriod(WORD NetworkID, WORD ServiceID, SYSTEMTIME *pTime);
	bool HasContractInfo(WORD NetworkID, WORD ServiceID) const;

	int GetInstructionName(int Instruction, LPWSTR pszName, int MaxName) const;
	UINT GetAvailableInstructions() const;
	bool SetInstruction(int Instruction);
	int GetInstruction() const;
	bool DescrambleBenchmarkTest(int Instruction, DWORD Round, DWORD *pTime);

protected:
// TVCAS::IBase
	TVCAS_DECLARE_BASE
	LPCWSTR GetName() const override;

// TVCAS::ICasClient
	LRESULT OnEvent(UINT Event, void *pParam) override;
	LRESULT OnError(const TVCAS::ErrorInfo *pInfo) override;
	void OutLog(TVCAS::LogType Type, LPCWSTR pszMessage) override;

	HMODULE m_hCasLib;
	TVCAS::ICasManager *m_pCasManager;
	CasModuleInfo m_CasModuleInfo;

	bool m_bEnableDescramble;
	bool m_bEnableContract;
	int m_Instruction;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_ScramblePacketCount;
};

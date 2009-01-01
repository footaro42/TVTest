// BcasCard.h: CBcasCard クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "CardReader.h"


// エラーコード
#define BCEC_NOERROR			0x00000000UL	// エラーなし
#define BCEC_INTERNALERROR		0x00000001UL	// 内部エラー
#define BCEC_NOTESTABLISHED		0x00000002UL	// コンテキスト確立失敗
#define BCEC_NOCARDREADERS		0x00000003UL	// カードリーダがない
#define BCEC_ALREADYOPEN		0x00000004UL	// 既にオープン済み
#define BCEC_CARDOPENERROR		0x00000005UL	// カードオープン失敗
#define BCEC_CARDNOTOPEN		0x00000006UL	// カード未オープン
#define BCEC_TRANSMITERROR		0x00000007UL	// 通信エラー
#define BCEC_BADARGUMENT		0x00000008UL	// 引数が不正
#define BCEC_ECMREFUSED			0x00000009UL	// ECM受付拒否


class CBcasCard
{
public:
	CBcasCard();
	~CBcasCard();

	const DWORD GetCardReaderNum(void) const;
	LPCTSTR EnumCardReader(const DWORD dwIndex) const;

	const bool OpenCard(CCardReader::ReaderType ReaderType = CCardReader::READER_SCARD ,LPCTSTR lpszReader = NULL);
	void CloseCard(void);
	const bool ReOpenCard();
	const bool IsCardOpen() const;
	LPCTSTR GetCardReaderName() const;

	const BYTE * GetBcasCardID(void);
	const BYTE * GetInitialCbc(void);
	const BYTE * GetSystemKey(void);
	const BYTE * GetKsFromEcm(const BYTE *pEcmData, const DWORD dwEcmSize);
	const int FormatCardID(LPTSTR pszText, int MaxLength) const;
	const char GetCardManufacturerID() const;
	const BYTE GetCardVersion() const;

	const DWORD GetLastError(void) const;

protected:
	const bool InitialSetting(void);

	CCardReader *m_pCardReader;

	struct TAG_BCASCARDINFO
	{
		BYTE BcasCardID[6];			// Card ID
		BYTE SystemKey[32];			// Descrambling system key
		BYTE InitialCbc[8];			// Descrambler CBC initial value
		BYTE CardManufacturerID;	// Manufacturer identifier
		BYTE CardVersion;			// Version
		WORD CheckCode;				// Check code
	} m_BcasCardInfo;

	struct TAG_ECMSTATUS
	{
		DWORD dwLastEcmSize;	// 最後に問い合わせのあったECMサイズ
		BYTE LastEcmData[256];	// 最後に問い合わせのあったECMデータ
		BYTE KsData[16];		// Ks Odd + Even	
	} m_EcmStatus;

	DWORD m_dwLastError;
};

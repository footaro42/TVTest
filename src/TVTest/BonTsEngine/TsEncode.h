// TsEncode.h: TSエンコードクラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B24 Part1文字列処理クラス
/////////////////////////////////////////////////////////////////////////////

class CAribString
{
public:
	static const DWORD AribToString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen);
	static const DWORD CaptionToString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen);

private:
	enum CODE_SET
	{
		CODE_UNKNOWN,				// 不明なグラフィックセット(非対応)
		CODE_KANJI,					// Kanji
		CODE_ALPHANUMERIC,			// Alphanumeric
		CODE_HIRAGANA,				// Hiragana
		CODE_KATAKANA,				// Katakana
		CODE_MOSAIC_A,				// Mosaic A
		CODE_MOSAIC_B,				// Mosaic B
		CODE_MOSAIC_C,				// Mosaic C
		CODE_MOSAIC_D,				// Mosaic D
		CODE_PROP_ALPHANUMERIC,		// Proportional Alphanumeric
		CODE_PROP_HIRAGANA,			// Proportional Hiragana
		CODE_PROP_KATAKANA,			// Proportional Katakana
		CODE_JIS_X0201_KATAKANA,	// JIS X 0201 Katakana
		CODE_JIS_KANJI_PLANE_1,		// JIS compatible Kanji Plane 1
		CODE_JIS_KANJI_PLANE_2,		// JIS compatible Kanji Plane 2
		CODE_ADDITIONAL_SYMBOLS,	// Additional symbols
		CODE_MACRO					// Macro
	};

	enum CHAR_SIZE {
		SIZE_SMALL,		// 小型
		SIZE_MEDIUM,	// 中型
		SIZE_NORMAL,	// 標準
		SIZE_MICRO,		// 超小型
		SIZE_HIGH_W,	// 縦倍
		SIZE_WIDTH_W,	// 横倍
		SIZE_W,			// 縦横倍
		SIZE_SPECIAL_1,	// 特殊1
		SIZE_SPECIAL_2	// 特殊2
	};

	CODE_SET m_CodeG[4];
	CODE_SET *m_pLockingGL;
	CODE_SET *m_pLockingGR;
	CODE_SET *m_pSingleGL;

	BYTE m_byEscSeqCount;
	BYTE m_byEscSeqIndex;
	bool m_bIsEscSeqDrcs;

	CHAR_SIZE m_CharSize;

	bool m_bCaption;

	const DWORD AribToStringInternal(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen, const bool bCaption = false);
	const DWORD ProcessString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen);
	inline const int ProcessCharCode(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode, const CODE_SET CodeSet);

	inline const int PutKanjiChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutAlphanumericChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutHiraganaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutJisKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutSymbolsChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutMacroChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);

	inline void ProcessEscapeSeq(const BYTE byCode);

	inline void LockingShiftGL(const BYTE byIndexG);
	inline void LockingShiftGR(const BYTE byIndexG);
	inline void SingleShiftGL(const BYTE byIndexG);

	inline const bool DesignationGSET(const BYTE byIndexG, const BYTE byCode);
	inline const bool DesignationDRCS(const BYTE byIndexG, const BYTE byCode);

	inline const bool IsSmallCharMode() const {
		return m_CharSize == SIZE_SMALL || m_CharSize == SIZE_MEDIUM || m_CharSize == SIZE_MICRO;
	}
};


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B10 Part2 Annex C MJD+JTC 処理クラス
/////////////////////////////////////////////////////////////////////////////

class CAribTime
{
public:
	static const bool AribToSystemTime(const BYTE *pHexData, SYSTEMTIME *pSysTime);
	static void SplitAribMjd(const WORD wAribMjd, WORD *pwYear, WORD *pwMonth, WORD *pwDay, WORD *pwDayOfWeek = NULL);
	static void MjdToSystemTime(const WORD wAribMjd, SYSTEMTIME *pSysTime);
	static void SplitAribBcd(const BYTE *pAribBcd, WORD *pwHour, WORD *pwMinute, WORD *pwSecond = NULL);
	static const DWORD AribBcdToSecond(const BYTE *pAribBcd);
	static const WORD BcdHMToMinute(const WORD Bcd);
};

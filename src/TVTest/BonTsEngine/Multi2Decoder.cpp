// Multi2Decoder.cpp: CMulti2Decoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Multi2Decoder.h"
#ifdef MULTI2_SSE2_ICC
#pragma comment(lib, "Multi2Decoder.lib")
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define SCRAMBLE_ROUND 4

#define USE_INTRINSIC	// 組み込み関数を展開

#ifdef USE_INTRINSIC
#pragma intrinsic(_byteswap_ulong, _lrotl)
#endif


inline void CMulti2Decoder::DATKEY::SetHexData(const BYTE *pHexData)
{
#ifndef USE_INTRINSIC
	BYTE * const pHexThis = reinterpret_cast<BYTE *>(this);

	// バイトオーダー変換
	pHexThis[ 3] = pHexData[ 0];	pHexThis[ 2] = pHexData[ 1];	pHexThis[ 1] = pHexData[ 2];	pHexThis[ 0] = pHexData[ 3];
	pHexThis[ 7] = pHexData[ 4];	pHexThis[ 6] = pHexData[ 5];	pHexThis[ 5] = pHexData[ 6];	pHexThis[ 4] = pHexData[ 7];
#else
	dwLeft  = _byteswap_ulong(*(DWORD*)(pHexData + 0));
	dwRight = _byteswap_ulong(*(DWORD*)(pHexData + 4));
#endif
}

inline void CMulti2Decoder::DATKEY::GetHexData(BYTE *pHexData) const
{
#ifndef USE_INTRINSIC
	const BYTE *pHexThis = reinterpret_cast<const BYTE *>(this);

	// バイトオーダー変換
	pHexData[ 0] = pHexThis[ 3];	pHexData[ 1] = pHexThis[ 2];	pHexData[ 2] = pHexThis[ 1];	pHexData[ 3] = pHexThis[ 0];
	pHexData[ 4] = pHexThis[ 7];	pHexData[ 5] = pHexThis[ 6];	pHexData[ 6] = pHexThis[ 5];	pHexData[ 7] = pHexThis[ 4];
#else
	*(DWORD*)(pHexData + 0) = _byteswap_ulong(dwLeft);
	*(DWORD*)(pHexData + 4) = _byteswap_ulong(dwRight);
#endif
}

inline void CMulti2Decoder::SYSKEY::SetHexData(const BYTE *pHexData)
{
#ifndef USE_INTRINSIC
	BYTE * const pHexThis = reinterpret_cast<BYTE *>(this);

	// バイトオーダー変換
	pHexThis[ 3] = pHexData[ 0];	pHexThis[ 2] = pHexData[ 1];	pHexThis[ 1] = pHexData[ 2];	pHexThis[ 0] = pHexData[ 3];
	pHexThis[ 7] = pHexData[ 4];	pHexThis[ 6] = pHexData[ 5];	pHexThis[ 5] = pHexData[ 6];	pHexThis[ 4] = pHexData[ 7];
	pHexThis[11] = pHexData[ 8];	pHexThis[10] = pHexData[ 9];	pHexThis[ 9] = pHexData[10];	pHexThis[ 8] = pHexData[11];
	pHexThis[15] = pHexData[12];	pHexThis[14] = pHexData[13];	pHexThis[13] = pHexData[14];	pHexThis[12] = pHexData[15];
	pHexThis[19] = pHexData[16];	pHexThis[18] = pHexData[17];	pHexThis[17] = pHexData[18];	pHexThis[16] = pHexData[19];
	pHexThis[23] = pHexData[20];	pHexThis[22] = pHexData[21];	pHexThis[21] = pHexData[22];	pHexThis[20] = pHexData[23];
	pHexThis[27] = pHexData[24];	pHexThis[26] = pHexData[25];	pHexThis[25] = pHexData[26];	pHexThis[24] = pHexData[27];
	pHexThis[31] = pHexData[28];	pHexThis[30] = pHexData[29];	pHexThis[29] = pHexData[30];	pHexThis[28] = pHexData[31];
#else
	dwKey1 = _byteswap_ulong(*(DWORD*)(pHexData +  0));
	dwKey2 = _byteswap_ulong(*(DWORD*)(pHexData +  4));
	dwKey3 = _byteswap_ulong(*(DWORD*)(pHexData +  8));
	dwKey4 = _byteswap_ulong(*(DWORD*)(pHexData + 12));
	dwKey5 = _byteswap_ulong(*(DWORD*)(pHexData + 16));
	dwKey6 = _byteswap_ulong(*(DWORD*)(pHexData + 20));
	dwKey7 = _byteswap_ulong(*(DWORD*)(pHexData + 24));
	dwKey8 = _byteswap_ulong(*(DWORD*)(pHexData + 28));
#endif
}

inline void CMulti2Decoder::SYSKEY::GetHexData(BYTE *pHexData) const
{
#ifndef USE_INTRINSIC
	const BYTE *pHexThis = reinterpret_cast<const BYTE *>(this);

	// バイトオーダー変換
	pHexData[ 0] = pHexThis[ 3];	pHexData[ 1] = pHexThis[ 2];	pHexData[ 2] = pHexThis[ 1];	pHexData[ 3] = pHexThis[ 0];
	pHexData[ 4] = pHexThis[ 7];	pHexData[ 5] = pHexThis[ 6];	pHexData[ 6] = pHexThis[ 5];	pHexData[ 7] = pHexThis[ 4];
	pHexData[ 8] = pHexThis[11];	pHexData[ 9] = pHexThis[10];	pHexData[10] = pHexThis[ 9];	pHexData[11] = pHexThis[ 8];
	pHexData[12] = pHexThis[15];	pHexData[13] = pHexThis[14];	pHexData[14] = pHexThis[13];	pHexData[15] = pHexThis[12];
	pHexData[16] = pHexThis[19];	pHexData[17] = pHexThis[18];	pHexData[18] = pHexThis[17];	pHexData[19] = pHexThis[16];
	pHexData[20] = pHexThis[23];	pHexData[21] = pHexThis[22];	pHexData[22] = pHexThis[21];	pHexData[23] = pHexThis[20];
	pHexData[24] = pHexThis[27];	pHexData[25] = pHexThis[26];	pHexData[26] = pHexThis[25];	pHexData[27] = pHexThis[24];
	pHexData[28] = pHexThis[31];	pHexData[29] = pHexThis[30];	pHexData[30] = pHexThis[29];	pHexData[31] = pHexThis[28];
#else
	*(DWORD*)(pHexData +  0) = _byteswap_ulong(dwKey1);
	*(DWORD*)(pHexData +  4) = _byteswap_ulong(dwKey2);
	*(DWORD*)(pHexData +  8) = _byteswap_ulong(dwKey3);
	*(DWORD*)(pHexData + 12) = _byteswap_ulong(dwKey4);
	*(DWORD*)(pHexData + 16) = _byteswap_ulong(dwKey5);
	*(DWORD*)(pHexData + 20) = _byteswap_ulong(dwKey6);
	*(DWORD*)(pHexData + 24) = _byteswap_ulong(dwKey7);
	*(DWORD*)(pHexData + 28) = _byteswap_ulong(dwKey8);
#endif
}


CMulti2Decoder::CMulti2Decoder(void)
	: m_bIsSysKeyValid(false)
	, m_bIsWorkKeyValid(false)
{

}

CMulti2Decoder::~CMulti2Decoder(void)
{

}

const bool CMulti2Decoder::Initialize(const BYTE *pSystemKey, const BYTE *pInitialCbc)
{
	if (!pSystemKey || !pInitialCbc)
		return false;

	m_bIsSysKeyValid = true;
	m_bIsWorkKeyValid = false;

	// Descrambling System Keyセット
	m_SystemKey.SetHexData(pSystemKey);

	// Descrambler CBC Initial Valueセット
	m_InitialCbc.SetHexData(pInitialCbc);
	return true;
}

const bool CMulti2Decoder::SetScrambleKey(const BYTE *pScrambleKey)
{
	if(!m_bIsSysKeyValid)return false;

	if(!pScrambleKey){
		// キーが設定されない場合はデコード不能にする(不正な復号による破損防止のため)
		m_bIsWorkKeyValid = false;
		return false;
		}

	// Scramble Key Odd/Even をセットする
	DATKEY ScrKeyOdd, ScrKeyEven;

	// バイトオーダー変換
	ScrKeyOdd.SetHexData(&pScrambleKey[0]);
	ScrKeyEven.SetHexData(&pScrambleKey[8]);

	// キースケジュール
	KeySchedule(m_WorkKeyOdd, m_SystemKey, ScrKeyOdd);
	KeySchedule(m_WorkKeyEven, m_SystemKey, ScrKeyEven);

	m_bIsWorkKeyValid = true;

	return true;
}

const bool CMulti2Decoder::Decode(BYTE *pData, const DWORD dwSize, const BYTE byScrCtrl) const
{
	if(!byScrCtrl)return true;										// スクランブルなし
	else if(!m_bIsSysKeyValid || !m_bIsWorkKeyValid)return false;	// スクランブルキー未設定
	else if((byScrCtrl != 2U) && (byScrCtrl != 3U))return false;	// スクランブル制御不正

	// ワークキー選択
	const SYSKEY &WorkKey = (byScrCtrl == 3)? m_WorkKeyOdd : m_WorkKeyEven;

	DATKEY CbcData = m_InitialCbc;
	//DWORD RemainSize = dwSize/sizeof(DATKEY)*sizeof(DATKEY);
	DWORD RemainSize = dwSize&0xFFFFFFF8UL;
	BYTE *pEnd = pData + RemainSize;
	BYTE *p = pData;

	// CBCモード
	DATKEY SrcData;
	while (p < pEnd) {
		SrcData.SetHexData(p);
		//DecryptBlock(SrcData, WorkKey);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi4(SrcData, WorkKey.dwKey8);
			RoundFuncPi3(SrcData, WorkKey.dwKey6, WorkKey.dwKey7);
			RoundFuncPi2(SrcData, WorkKey.dwKey5);
			RoundFuncPi1(SrcData);
			RoundFuncPi4(SrcData, WorkKey.dwKey4);
			RoundFuncPi3(SrcData, WorkKey.dwKey2, WorkKey.dwKey3);
			RoundFuncPi2(SrcData, WorkKey.dwKey1);
			RoundFuncPi1(SrcData);
		}
		SrcData.dwLeft ^= CbcData.dwLeft;
		SrcData.dwRight ^= CbcData.dwRight;
		CbcData.SetHexData(p);
		SrcData.GetHexData(p);
		p+=sizeof(DATKEY);
	}

	// OFBモード
	//RemainSize=dwSize%sizeof(DATKEY);
	RemainSize=dwSize&0x00000007UL;
	if (RemainSize > 0) {
		BYTE Remain[sizeof(DATKEY)];

		//EncryptBlock(CbcData, WorkKey);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi1(CbcData);
			RoundFuncPi2(CbcData, WorkKey.dwKey1);
			RoundFuncPi3(CbcData, WorkKey.dwKey2, WorkKey.dwKey3);
			RoundFuncPi4(CbcData, WorkKey.dwKey4);
			RoundFuncPi1(CbcData);
			RoundFuncPi2(CbcData, WorkKey.dwKey5);
			RoundFuncPi3(CbcData, WorkKey.dwKey6, WorkKey.dwKey7);
			RoundFuncPi4(CbcData, WorkKey.dwKey8);
		}
		CbcData.GetHexData(Remain);
		/*
		pEnd+=RemainSize;
		for (BYTE *q=Remain ; p<pEnd ; q++) {
			*p++ ^= *q;
		}
		*/
		switch (RemainSize) {
		case 7: p[6] ^= Remain[6];
		case 6: p[5] ^= Remain[5];
		case 5: p[4] ^= Remain[4];
		case 4: p[3] ^= Remain[3];
		case 3: p[2] ^= Remain[2];
		case 2: p[1] ^= Remain[1];
		case 1: p[0] ^= Remain[0];
		}
	}

	return true;
}

/*
// 実際はinline展開されない@VC2005
inline void CMulti2Decoder::DecryptBlock(DATKEY &Block, const SYSKEY &WorkKey)
{
	// Block Decryption
	for(DWORD dwRound = 0UL ; dwRound < SCRAMBLE_ROUND ; dwRound++){
		RoundFuncPi4(Block, WorkKey.dwKey8);
		RoundFuncPi3(Block, WorkKey.dwKey6, WorkKey.dwKey7);
		RoundFuncPi2(Block, WorkKey.dwKey5);
		RoundFuncPi1(Block);
		RoundFuncPi4(Block, WorkKey.dwKey4);
		RoundFuncPi3(Block, WorkKey.dwKey2, WorkKey.dwKey3);
		RoundFuncPi2(Block, WorkKey.dwKey1);
		RoundFuncPi1(Block);
		}
}

inline void CMulti2Decoder::EncryptBlock(DATKEY &Block, const SYSKEY &WorkKey)
{
	// Block Encryption
	for(DWORD dwRound = 0UL ; dwRound < SCRAMBLE_ROUND ; dwRound++){
		RoundFuncPi1(Block);
		RoundFuncPi2(Block, WorkKey.dwKey1);
		RoundFuncPi3(Block, WorkKey.dwKey2, WorkKey.dwKey3);
		RoundFuncPi4(Block, WorkKey.dwKey4);
		RoundFuncPi1(Block);
		RoundFuncPi2(Block, WorkKey.dwKey5);
		RoundFuncPi3(Block, WorkKey.dwKey6, WorkKey.dwKey7);
		RoundFuncPi4(Block, WorkKey.dwKey8);
		}
}
*/

inline void CMulti2Decoder::KeySchedule(SYSKEY &WorkKey, const SYSKEY &SysKey, DATKEY &DataKey)
{
	// Key Schedule
	RoundFuncPi1(DataKey);									// π1

	RoundFuncPi2(DataKey, SysKey.dwKey1);					// π2
	WorkKey.dwKey1 = DataKey.dwLeft;

	RoundFuncPi3(DataKey, SysKey.dwKey2, SysKey.dwKey3);	// π3
	WorkKey.dwKey2 = DataKey.dwRight;

	RoundFuncPi4(DataKey, SysKey.dwKey4);					// π4
	WorkKey.dwKey3 = DataKey.dwLeft;

	RoundFuncPi1(DataKey);									// π1
	WorkKey.dwKey4 = DataKey.dwRight;

	RoundFuncPi2(DataKey, SysKey.dwKey5);					// π2
	WorkKey.dwKey5 = DataKey.dwLeft;

	RoundFuncPi3(DataKey, SysKey.dwKey6, SysKey.dwKey7);	// π3
	WorkKey.dwKey6 = DataKey.dwRight;

	RoundFuncPi4(DataKey, SysKey.dwKey8);					// π4
	WorkKey.dwKey7 = DataKey.dwLeft;

	RoundFuncPi1(DataKey);									// π1
	WorkKey.dwKey8 = DataKey.dwRight;
}

inline void CMulti2Decoder::RoundFuncPi1(DATKEY &Block)
{
	// Elementary Encryption Function π1
	Block.dwRight ^= Block.dwLeft;
}

inline void CMulti2Decoder::RoundFuncPi2(DATKEY &Block, const DWORD dwK1)
{
	// Elementary Encryption Function π2
	const DWORD dwY = Block.dwRight + dwK1;
	const DWORD dwZ = LeftRotate(dwY, 1UL) + dwY - 1UL;
	Block.dwLeft ^= LeftRotate(dwZ, 4UL) ^ dwZ;
}

inline void CMulti2Decoder::RoundFuncPi3(DATKEY &Block, const DWORD dwK2, const DWORD dwK3)
{
	// Elementary Encryption Function π3
	const DWORD dwY = Block.dwLeft + dwK2;
	const DWORD dwZ = LeftRotate(dwY, 2UL) + dwY + 1UL;
	const DWORD dwA = LeftRotate(dwZ, 8UL) ^ dwZ;
	const DWORD dwB = dwA + dwK3;
	const DWORD dwC = LeftRotate(dwB, 1UL) - dwB;
	Block.dwRight ^= (LeftRotate(dwC, 16UL) ^ (dwC | Block.dwLeft));
}

inline void CMulti2Decoder::RoundFuncPi4(DATKEY &Block, const DWORD dwK4)
{
	// Elementary Encryption Function π4
	const DWORD dwY = Block.dwRight + dwK4;
	Block.dwLeft ^= (LeftRotate(dwY, 2UL) + dwY + 1UL);
}

inline const DWORD CMulti2Decoder::LeftRotate(const DWORD dwValue, const DWORD dwRotate)
{
#ifndef USE_INTRINSIC
	// 左ローテート
	return (dwValue << dwRotate) | (dwValue >> (32UL - dwRotate));
#else
	// 実はわざわざ書き換えなくても、上のコードでもrolが使われる
	return _lrotl(dwValue, dwRotate);
#endif
}




#ifdef MULTI2_SSE2
#ifndef MULTI2_SSE2_ICC
/*
	SSE2対応
	elick氏のコードを参考にしています。まだ最適化が可能だと思います。
*/


#define MM_SHUFFLE4(a, b, c, d) ((a << 6) | (b << 4) | (c << 2) | d)

#define MM_SHIFT_COUNT(n) (ShiftCount##n)
#define MM_ROTATE(v, l, r) LeftRotate(v, MM_SHIFT_COUNT(l), MM_SHIFT_COUNT(r))


static __m128i ShiftCount1;
static __m128i ShiftCount2;
static __m128i ShiftCount4;
static __m128i ShiftCount8;
static __m128i ShiftCount16;

static __m128i ShiftCount31;
static __m128i ShiftCount30;
static __m128i ShiftCount28;
static __m128i ShiftCount24;

static __m128i Immediate1;

static __m128i SwapMask;

class CSSE2Initializer
{
public:
	CSSE2Initializer() {
		if (IsSSE2Available()) {
			TRACE(TEXT("Multi2Decoder SSE2 available\n"));

			ShiftCount1 = _mm_set_epi32(0, 0, 0, 1);
			ShiftCount2 = _mm_set_epi32(0, 0, 0, 2);
			ShiftCount4 = _mm_set_epi32(0, 0, 0, 4);
			ShiftCount8 = _mm_set_epi32(0, 0, 0, 8);
			ShiftCount16 = _mm_set_epi32(0, 0, 0, 16);

			ShiftCount31 = _mm_set_epi32(0, 0, 0, 31);
			ShiftCount30 = _mm_set_epi32(0, 0, 0, 30);
			ShiftCount28 = _mm_set_epi32(0, 0, 0, 28);
			ShiftCount24 = _mm_set_epi32(0, 0, 0, 24);

			Immediate1 = _mm_set1_epi32(1);

			SwapMask = _mm_set1_epi32(0xFF00FF00);
		}
	}
	static bool IsSSE2Available() {
#if 1
		bool b;

		__asm {
			mov		eax, 1
			cpuid
			bt		edx, 26
			setc	b
		}

		return b;
#else
		return ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) != FALSE;
#endif
	}
};

static CSSE2Initializer SSE2Initializer;


inline void CMulti2Decoder::RoundFuncPi1SSE2(__m128i &Left, __m128i &Right)
{
	Right = _mm_xor_si128(Right, Left);
}

inline void CMulti2Decoder::RoundFuncPi2SSE2(__m128i &Left, __m128i &Right, DWORD Key1)
{
	__m128i t0,t1,t2;

	t0 = _mm_add_epi32(Right, _mm_set1_epi32(Key1));
	t1 = _mm_sub_epi32(_mm_add_epi32(MM_ROTATE(t0, 1, 31), t0), _mm_set1_epi32(1));
	t2 = _mm_xor_si128(MM_ROTATE(t1, 4, 28), t1);

	Left = _mm_xor_si128(Left, t2);
}

inline void CMulti2Decoder::RoundFuncPi3SSE2(__m128i &Left, __m128i &Right, DWORD Key2, DWORD Key3)
{
	__m128i t0,t1,t2,t3,t4,t5;

	t0 = _mm_add_epi32(Left, _mm_set1_epi32(Key2));
	t1 = _mm_add_epi32(_mm_add_epi32(MM_ROTATE(t0, 2, 30), t0), _mm_set1_epi32(1));
	t2 = _mm_xor_si128(MM_ROTATE(t1, 8, 24), t1);
	t3 = _mm_add_epi32(t2, _mm_set1_epi32(Key3));
	t4 = _mm_sub_epi32(MM_ROTATE(t3, 1, 31), t3);
	t5 = _mm_xor_si128(MM_ROTATE(t4, 16, 16), _mm_or_si128(t4, Left));

	Right = _mm_xor_si128(Right, t5);
}

inline void CMulti2Decoder::RoundFuncPi4SSE2(__m128i &Left, __m128i &Right, DWORD Key4)
{
	__m128i t0,t1;

	t0 = _mm_add_epi32(Right, _mm_set1_epi32(Key4));
	t1 = _mm_add_epi32(_mm_add_epi32(MM_ROTATE(t0, 2, 30), t0), _mm_set1_epi32(1));

	Left = _mm_xor_si128(Left, t1);
}

inline __m128i CMulti2Decoder::LeftRotate(const __m128i &Value, const __m128i &Rotate, const __m128i &InvRotate)
{
	return _mm_or_si128(_mm_sll_epi32(Value, Rotate), _mm_srl_epi32(Value, InvRotate));
}

inline __m128i CMulti2Decoder::ByteSwap128(const __m128i &Value)
{
	__m128i src = Value;
	__m128i tmp = _mm_and_si128(src, SwapMask);
	tmp = _mm_srl_epi32(tmp, MM_SHIFT_COUNT(8));
	src = _mm_sll_epi32(src, MM_SHIFT_COUNT(8));
	src = _mm_and_si128(src, SwapMask);
	src = _mm_or_si128(src, tmp);
	return MM_ROTATE(src, 16, 16);
}

const bool CMulti2Decoder::DecodeSSE2(BYTE *pData, const DWORD dwSize, const BYTE byScrCtrl) const
{
	if(!byScrCtrl)return true;										// スクランブルなし
	else if(!m_bIsSysKeyValid || !m_bIsWorkKeyValid)return false;	// スクランブルキー未設定
	else if((byScrCtrl != 2U) && (byScrCtrl != 3U))return false;	// スクランブル制御不正

	// ワークキー選択
	const SYSKEY &WorkKey = (byScrCtrl == 3) ? m_WorkKeyOdd : m_WorkKeyEven;

	DWORD RemainSize = dwSize & 0xFFFFFFE0UL;
	BYTE *pEnd = pData + RemainSize;
	BYTE *p = pData;

	// CBCモード
	__m128i Cbc = _mm_set_epi32(0, 0,
								_byteswap_ulong(m_InitialCbc.dwRight),
								_byteswap_ulong(m_InitialCbc.dwLeft));
	while (p < pEnd) {
		__m128i Src1, Src2, Left, Right;

		// r2 l2 r1 l1
		Src1 = _mm_loadu_si128((__m128i*)p);
		// r4 l4 r3 l3
		Src2 = _mm_loadu_si128((__m128i*)(p + 16));

		Left = ByteSwap128(Src1);
		Right = ByteSwap128(Src2);

		{
			// r2 r1 l2 l1
			__m128i x = _mm_shuffle_epi32(Left, MM_SHUFFLE4(3, 1, 2, 0));

			// r4 r3 l4 l3
			__m128i y = _mm_shuffle_epi32(Right, MM_SHUFFLE4(3, 1, 2, 0));

			// l4 l2 l3 l1
			Left = _mm_unpacklo_epi32(x, y);

			// r4 r2 r3 r1
			Right = _mm_unpackhi_epi32(x, y);
		}

		for (int i = 0 ; i < SCRAMBLE_ROUND ; i++) {
			RoundFuncPi4SSE2(Left, Right, WorkKey.dwKey8);
			RoundFuncPi3SSE2(Left, Right, WorkKey.dwKey6, WorkKey.dwKey7);
			RoundFuncPi2SSE2(Left, Right, WorkKey.dwKey5);
			RoundFuncPi1SSE2(Left, Right);
			RoundFuncPi4SSE2(Left, Right, WorkKey.dwKey4);
			RoundFuncPi3SSE2(Left, Right, WorkKey.dwKey2, WorkKey.dwKey3);
			RoundFuncPi2SSE2(Left, Right, WorkKey.dwKey1);
			RoundFuncPi1SSE2(Left, Right);
		}

		{
			// l4 l3 l2 l1
			__m128i a = _mm_shuffle_epi32(Left, MM_SHUFFLE4(3, 1, 2, 0));

			// r4 r3 r2 r1
			__m128i b = _mm_shuffle_epi32(Right, MM_SHUFFLE4(3, 1, 2, 0));

			// r2 l2 r1 l1
			__m128i x = _mm_unpacklo_epi32(a, b);

			// r4 l4 r3 l3
			__m128i y = _mm_unpackhi_epi32(a, b);

			x = _mm_xor_si128(ByteSwap128(x), _mm_or_si128(_mm_slli_si128(Src1, 8), Cbc));
			y = _mm_xor_si128(ByteSwap128(y), _mm_loadu_si128((__m128i*)(p + 8)));
			_mm_storeu_si128((__m128i*)p, x);
			_mm_storeu_si128((__m128i*)p + 1, y);

			Cbc = _mm_srli_si128(Src2, 8);
		}

		p += 32;
	}

	DATKEY CbcData;
	CbcData.dwLeft = _byteswap_ulong(Cbc.m128i_i32[0]);
	CbcData.dwRight = _byteswap_ulong(Cbc.m128i_i32[1]);

	RemainSize= dwSize & 0x00000018UL;
	pEnd = p + RemainSize;
	while (p < pEnd) {
		DATKEY SrcData;

		SrcData.SetHexData(p);
		//DecryptBlock(SrcData, WorkKey);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi4(SrcData, WorkKey.dwKey8);
			RoundFuncPi3(SrcData, WorkKey.dwKey6, WorkKey.dwKey7);
			RoundFuncPi2(SrcData, WorkKey.dwKey5);
			RoundFuncPi1(SrcData);
			RoundFuncPi4(SrcData, WorkKey.dwKey4);
			RoundFuncPi3(SrcData, WorkKey.dwKey2, WorkKey.dwKey3);
			RoundFuncPi2(SrcData, WorkKey.dwKey1);
			RoundFuncPi1(SrcData);
		}
		SrcData.dwLeft ^= CbcData.dwLeft;
		SrcData.dwRight ^= CbcData.dwRight;
		CbcData.SetHexData(p);
		SrcData.GetHexData(p);
		p += sizeof(DATKEY);
	}

	// OFBモード
	RemainSize = dwSize & 0x00000007UL;
	if (RemainSize > 0) {
		BYTE Remain[sizeof(DATKEY)];

		//EncryptBlock(CbcData, WorkKey);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi1(CbcData);
			RoundFuncPi2(CbcData, WorkKey.dwKey1);
			RoundFuncPi3(CbcData, WorkKey.dwKey2, WorkKey.dwKey3);
			RoundFuncPi4(CbcData, WorkKey.dwKey4);
			RoundFuncPi1(CbcData);
			RoundFuncPi2(CbcData, WorkKey.dwKey5);
			RoundFuncPi3(CbcData, WorkKey.dwKey6, WorkKey.dwKey7);
			RoundFuncPi4(CbcData, WorkKey.dwKey8);
		}
		CbcData.GetHexData(Remain);
		switch (RemainSize) {
		case 7: p[6] ^= Remain[6];
		case 6: p[5] ^= Remain[5];
		case 5: p[4] ^= Remain[4];
		case 4: p[3] ^= Remain[3];
		case 3: p[2] ^= Remain[2];
		case 2: p[1] ^= Remain[1];
		case 1: p[0] ^= Remain[0];
		}
	}

	return true;
}

bool CMulti2Decoder::IsSSE2Available()
{
	return CSSE2Initializer::IsSSE2Available();
}


#else	// MULTI2_SSE2_ICC


class CSSE2Initializer
{
public:
	CSSE2Initializer() {
		Multi2DecoderSSE2::Initialize();
	}
};

static CSSE2Initializer SSE2Initializer;


const bool CMulti2Decoder::DecodeSSE2(BYTE *pData, const DWORD dwSize, const BYTE byScrCtrl) const
{
	if(!byScrCtrl)return true;										// スクランブルなし
	else if(!m_bIsSysKeyValid || !m_bIsWorkKeyValid)return false;	// スクランブルキー未設定
	else if((byScrCtrl != 2U) && (byScrCtrl != 3U))return false;	// スクランブル制御不正

	// ワークキー選択
	return Multi2DecoderSSE2::Decode(pData, dwSize,
		reinterpret_cast<const Multi2DecoderSSE2::SYSKEY*>((byScrCtrl == 3) ? &m_WorkKeyOdd : &m_WorkKeyEven),
		m_InitialCbc.dwLeft, m_InitialCbc.dwRight);
}


bool CMulti2Decoder::IsSSE2Available()
{
	return Multi2DecoderSSE2::IsSSE2Available();
}


#endif	// MULTI2_SSE2_ICC


#endif	// MULTI2_SSE2

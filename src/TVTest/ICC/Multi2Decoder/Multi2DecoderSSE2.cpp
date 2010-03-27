#include <windows.h>
#include "Multi2DecoderSSE2.h"


#define SCRAMBLE_ROUND 4

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


bool Multi2DecoderSSE2::IsSSE2Available()
{
#ifndef WIN64
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


bool Multi2DecoderSSE2::Initialize()
{
	if (!IsSSE2Available())
		return false;

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

	return true;
}


static inline __m128i LeftRotate(const __m128i &Value, const __m128i &Rotate, const __m128i &InvRotate)
{
	return _mm_or_si128(_mm_sll_epi32(Value, Rotate), _mm_srl_epi32(Value, InvRotate));
}

static inline void RoundFuncPi1SSE2(__m128i &Left, __m128i &Right)
{
	Right = _mm_xor_si128(Right, Left);
}

static inline void RoundFuncPi2SSE2(__m128i &Left, __m128i &Right, DWORD Key1)
{
	__m128i t0,t1,t2;

	t0 = _mm_add_epi32(Right, _mm_set1_epi32(Key1));
	t1 = _mm_sub_epi32(_mm_add_epi32(MM_ROTATE(t0, 1, 31), t0), _mm_set1_epi32(1));
	t2 = _mm_xor_si128(MM_ROTATE(t1, 4, 28), t1);

	Left = _mm_xor_si128(Left, t2);
}

static inline void RoundFuncPi3SSE2(__m128i &Left, __m128i &Right, DWORD Key2, DWORD Key3)
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

static inline void RoundFuncPi4SSE2(__m128i &Left, __m128i &Right, DWORD Key4)
{
	__m128i t0,t1;

	t0 = _mm_add_epi32(Right, _mm_set1_epi32(Key4));
	t1 = _mm_add_epi32(_mm_add_epi32(MM_ROTATE(t0, 2, 30), t0), _mm_set1_epi32(1));

	Left = _mm_xor_si128(Left, t1);
}

static inline __m128i ByteSwap128(const __m128i &Value)
{
	__m128i src = Value;
	__m128i tmp = _mm_and_si128(src, SwapMask);
	tmp = _mm_srl_epi32(tmp, MM_SHIFT_COUNT(8));
	src = _mm_sll_epi32(src, MM_SHIFT_COUNT(8));
	src = _mm_and_si128(src, SwapMask);
	src = _mm_or_si128(src, tmp);
	return MM_ROTATE(src, 16, 16);
}


static inline const DWORD LeftRotate(const DWORD dwValue, const DWORD dwRotate)
{
	// 実はわざわざ書き換えなくても、上のコードでもrolが使われる
	return _lrotl(dwValue, dwRotate);
}

static inline void RoundFuncPi1(DWORD &Left, DWORD &Right)
{
	// Elementary Encryption Function π1
	Right ^= Left;
}

static inline void RoundFuncPi2(DWORD &Left, DWORD &Right, const DWORD dwK1)
{
	// Elementary Encryption Function π2
	const DWORD dwY = Right + dwK1;
	const DWORD dwZ = LeftRotate(dwY, 1UL) + dwY - 1UL;
	Left ^= LeftRotate(dwZ, 4UL) ^ dwZ;
}

static inline void RoundFuncPi3(DWORD &Left, DWORD &Right, const DWORD dwK2, const DWORD dwK3)
{
	// Elementary Encryption Function π3
	const DWORD dwY = Left + dwK2;
	const DWORD dwZ = LeftRotate(dwY, 2UL) + dwY + 1UL;
	const DWORD dwA = LeftRotate(dwZ, 8UL) ^ dwZ;
	const DWORD dwB = dwA + dwK3;
	const DWORD dwC = LeftRotate(dwB, 1UL) - dwB;
	Right ^= (LeftRotate(dwC, 16UL) ^ (dwC | Left));
}

static inline void RoundFuncPi4(DWORD &Left, DWORD &Right, const DWORD dwK4)
{
	// Elementary Encryption Function π4
	const DWORD dwY = Right + dwK4;
	Left ^= (LeftRotate(dwY, 2UL) + dwY + 1UL);
}


bool Multi2DecoderSSE2::Decode(BYTE *pData, const DWORD dwSize, const CMulti2Decoder::SYSKEY *pWorkKey,
	const DWORD InitialCbcLeft, const DWORD InitialCbcRight)
{
	DWORD RemainSize = dwSize & 0xFFFFFFE0UL;
	BYTE *pEnd = pData + RemainSize;
	BYTE *p = pData;

	// CBCモード
	__m128i Cbc = _mm_set_epi32(0, 0,
								_byteswap_ulong(InitialCbcRight),
								_byteswap_ulong(InitialCbcLeft));
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
			RoundFuncPi4SSE2(Left, Right, pWorkKey->dwKey8);
			RoundFuncPi3SSE2(Left, Right, pWorkKey->dwKey6, pWorkKey->dwKey7);
			RoundFuncPi2SSE2(Left, Right, pWorkKey->dwKey5);
			RoundFuncPi1SSE2(Left, Right);
			RoundFuncPi4SSE2(Left, Right, pWorkKey->dwKey4);
			RoundFuncPi3SSE2(Left, Right, pWorkKey->dwKey2, pWorkKey->dwKey3);
			RoundFuncPi2SSE2(Left, Right, pWorkKey->dwKey1);
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

	DWORD CbcLeft, CbcRight;
	__declspec(align(16)) DWORD Data[4];
	_mm_store_si128((__m128i*)Data, Cbc);
	CbcLeft = Data[0];
	CbcRight = Data[1];

	RemainSize= dwSize & 0x00000018UL;
	pEnd = p + RemainSize;
	while (p < pEnd) {
		DWORD Src1, Src2, Left, Right;

		Src1 = *(DWORD*)(p + 0);
		Src2 = *(DWORD*)(p + 4);
		Left  = _byteswap_ulong(Src1);
		Right = _byteswap_ulong(Src2);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi4(Left, Right, pWorkKey->dwKey8);
			RoundFuncPi3(Left, Right, pWorkKey->dwKey6, pWorkKey->dwKey7);
			RoundFuncPi2(Left, Right, pWorkKey->dwKey5);
			RoundFuncPi1(Left, Right);
			RoundFuncPi4(Left, Right, pWorkKey->dwKey4);
			RoundFuncPi3(Left, Right, pWorkKey->dwKey2, pWorkKey->dwKey3);
			RoundFuncPi2(Left, Right, pWorkKey->dwKey1);
			RoundFuncPi1(Left, Right);
		}
		*(DWORD*)(p + 0) = _byteswap_ulong(Left)  ^ CbcLeft;
		*(DWORD*)(p + 4) = _byteswap_ulong(Right) ^ CbcRight;
		CbcLeft  = Src1;
		CbcRight = Src2;

		p += 8;
	}

	// OFBモード
	RemainSize = dwSize & 0x00000007UL;
	if (RemainSize > 0) {
		CbcLeft  = _byteswap_ulong(CbcLeft);
		CbcRight = _byteswap_ulong(CbcRight);
		for (int Round = 0 ; Round < SCRAMBLE_ROUND ; Round++) {
			RoundFuncPi1(CbcLeft, CbcRight);
			RoundFuncPi2(CbcLeft, CbcRight, pWorkKey->dwKey1);
			RoundFuncPi3(CbcLeft, CbcRight, pWorkKey->dwKey2, pWorkKey->dwKey3);
			RoundFuncPi4(CbcLeft, CbcRight, pWorkKey->dwKey4);
			RoundFuncPi1(CbcLeft, CbcRight);
			RoundFuncPi2(CbcLeft, CbcRight, pWorkKey->dwKey5);
			RoundFuncPi3(CbcLeft, CbcRight, pWorkKey->dwKey6, pWorkKey->dwKey7);
			RoundFuncPi4(CbcLeft, CbcRight, pWorkKey->dwKey8);
		}
		BYTE Remain[8];
		*(DWORD*)(Remain + 0) = _byteswap_ulong(CbcLeft);
		*(DWORD*)(Remain + 4) = _byteswap_ulong(CbcRight);
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

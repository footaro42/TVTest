#ifndef MULTI2_DECODER_SSE2
#define MULTI2_DECODER_SSE2


#include <emmintrin.h>


namespace Multi2DecoderSSE2 {
	struct SYSKEY {
		DWORD dwKey1, dwKey2, dwKey3, dwKey4, dwKey5, dwKey6, dwKey7, dwKey8;
	};

	bool IsSSE2Available();
	bool Initialize();
	bool Decode(BYTE *pData, const DWORD dwSize, const SYSKEY *pWorkKey,
				const DWORD InitialCbcLeft, const DWORD InitialCbcRight);
}


#endif

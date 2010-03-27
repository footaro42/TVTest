#ifndef MULTI2_DECODER_SSE2
#define MULTI2_DECODER_SSE2


#include "../../BonTsEngine/Multi2Decoder.h"
#include <emmintrin.h>


namespace Multi2DecoderSSE2
{
	bool IsSSE2Available();
	bool Initialize();
	bool Decode(BYTE *pData, const DWORD dwSize, const CMulti2Decoder::SYSKEY *pWorkKey,
				const DWORD InitialCbcLeft, const DWORD InitialCbcRight);
}


#endif

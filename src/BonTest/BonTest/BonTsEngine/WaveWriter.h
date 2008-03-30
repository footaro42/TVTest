// WaveWriter.h: CWaveWriter クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "FileWriter.h"


/////////////////////////////////////////////////////////////////////////////
// Waveファイル出力(48KHz 16bit Streo PCMデータをWavファイルに書き出す)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		書き込みデータ
/////////////////////////////////////////////////////////////////////////////

class CWaveWriter : public CFileWriter  
{
public:
	CWaveWriter(IEventHandler *pEventHandler = NULL);
	virtual ~CWaveWriter();

// CFileWriter
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);
	const bool OpenFile(LPCTSTR lpszFileName);
	void CloseFile(void);
};

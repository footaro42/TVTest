// WaveWriter.cpp: CWaveWriter クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WaveWriter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CWaveWriter::CWaveWriter(IEventHandler *pEventHandler)
	: CFileWriter(pEventHandler)
{

}

CWaveWriter::~CWaveWriter()
{
	CloseFile();
}

const bool CWaveWriter::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex > GetInputNum())return false;

	// 4GBの制限チェック
	if((m_llWriteSize + (ULONGLONG)pMediaData->GetSize()) >= 0xFFFFFFFFULL)return true;

	// ファイル書き込み
	return CFileWriter::InputMedia(pMediaData, dwInputIndex);
}

const bool CWaveWriter::OpenFile(LPCTSTR lpszFileName)
{
	if(!CFileWriter::OpenFile(lpszFileName))return false;

	// RIFFヘッダを書き込む
	static const BYTE WaveHead[] =
	{
		'R', 'I', 'F', 'F',				// +0	RIFF
		0x00U, 0x00U, 0x00U, 0x00U,		// +4	これ以降のファイルサイズ(ファイルサイズ - 8)
		'W', 'A', 'V', 'E',				// +8	WAVE
		'f', 'm', 't', ' ',				// +12	fmt
		0x10U, 0x00U, 0x00U, 0x00U,		// +16	fmt チャンクのバイト数
		0x01U, 0x00U,					// +18	フォーマットID
		0x02U, 0x00U,					// +20	ステレオ
		0x80U, 0xBBU, 0x00U, 0x00U,		// +24	48KHz
		0x00U, 0xEEU, 0x02U, 0x00U,		// +28	192000Byte/s
		0x04U, 0x00U,					// +30	ブロックサイズ
		0x10U, 0x00U,					// +32	サンプルあたりのビット数
		'd', 'a', 't', 'a',				// +36	data
		0x00U, 0x00U, 0x00U, 0x00U		// +40	波形データのバイト数
	};

	return m_OutFile.Write(WaveHead, sizeof(WaveHead));
}

void CWaveWriter::CloseFile(void)
{
	// RIFFヘッダにサイズを書き込む
	DWORD dwLength = (DWORD)m_llWriteSize;
	m_OutFile.Write((BYTE *)&dwLength, 4UL, 40ULL);
	dwLength = (dwLength > 36)? (dwLength - 36UL) : 0UL;
	m_OutFile.Write((BYTE *)&dwLength, 4UL, 4ULL);		

	// ファイルを閉じる
	CFileWriter::CloseFile();
}

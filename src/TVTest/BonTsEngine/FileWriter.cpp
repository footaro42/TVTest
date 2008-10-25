// FileWriter.cpp: CFileWriter クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileWriter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CFileWriter::CFileWriter(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_BufferSize(DEFBUFFSIZE)
	, m_llWriteSize(0U)
	, m_llWriteCount(0U)
	, m_bWriteError(false)
	, m_bPause(false)
{

}


CFileWriter::~CFileWriter()
{
	CloseFile();
}


void CFileWriter::Reset(void)
{

}


const bool CFileWriter::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if (dwInputIndex>GetInputNum())
		return false;

	if (!m_OutFile.IsOpen() || m_bPause)
		return true;

	// ファイルに書き込み
	if (!m_OutFile.Write(pMediaData->GetData(), pMediaData->GetSize())) {
		if (!m_bWriteError) {
			SendDecoderEvent(EID_WRITE_ERROR);
			m_bWriteError=true;
		}
		return false;
	}

	m_llWriteSize += pMediaData->GetSize();
	m_llWriteCount++;
	return true;
}


const bool CFileWriter::OpenFile(LPCTSTR lpszFileName, BYTE bFlags)
{
	// 一旦閉じる
	CloseFile();

	// ファイルを開く
	if (!m_OutFile.Open(lpszFileName, CNCachedFile::CNF_WRITE | CNCachedFile::CNF_NEW | bFlags,m_BufferSize))
		return false;
	m_bWriteError=false;
	m_bPause=false;
	return true;
}


void CFileWriter::CloseFile(void)
{
	// ファイルを閉じる
	m_OutFile.Close();

	m_llWriteSize = 0U;
	m_llWriteCount = 0U;
}


const bool CFileWriter::IsOpen() const
{
	return m_OutFile.IsOpen();
}


const LONGLONG CFileWriter::GetWriteSize(void) const
{
	// 書き込み済みサイズを返す
	return m_llWriteSize;
}


const LONGLONG CFileWriter::GetWriteCount(void) const
{
	// 書き込み回数を返す
	return m_llWriteCount;
}


bool CFileWriter::SetBufferSize(DWORD Size)
{
	if (Size==0)
		return false;
	m_BufferSize=Size;
	return true;
}


bool CFileWriter::Pause()
{
	if (!m_OutFile.IsOpen() || m_bPause)
		return false;
	m_bPause=true;
	return true;
}


bool CFileWriter::Resume()
{
	if (!m_OutFile.IsOpen() || !m_bPause)
		return false;
	m_bPause=false;
	return true;
}


LPCTSTR CFileWriter::GetFileName() const
{
	return m_OutFile.GetFileName();
}

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
	: CMediaDecoder(pEventHandler)
	, m_llWriteSize(0U)
	, m_llWriteCount(0U)
{

}

CFileWriter::~CFileWriter()
{
	CloseFile();
}

void CFileWriter::Reset(void)
{

}

const DWORD CFileWriter::GetInputNum(void) const
{
	return 1UL;
}

const DWORD CFileWriter::GetOutputNum(void) const
{
	return 0UL;
}

const bool CFileWriter::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex > GetInputNum())return false;

	// ファイルに書き込み
	if(m_OutFile.Write(pMediaData->GetData(), pMediaData->GetSize())){
		m_llWriteSize += pMediaData->GetSize();
		m_llWriteCount++;
		return true;
		}

	return false;
}

const bool CFileWriter::OpenFile(LPCTSTR lpszFileName)
{
	// 一旦閉じる
	CloseFile();

	// ファイルを開く
	return (m_OutFile.Open(lpszFileName, CNCachedFile::CNF_WRITE | CNCachedFile::CNF_NEW))? true : false;
}

void CFileWriter::CloseFile(void)
{
	// ファイルを閉じる
	m_OutFile.Close();
	
	m_llWriteSize = 0U;
	m_llWriteCount = 0U;
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

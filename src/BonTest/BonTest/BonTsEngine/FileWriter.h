// FileWriter.h: CFileWriter クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "NCachedFile.h"


/////////////////////////////////////////////////////////////////////////////
// 汎用ファイル出力(CMediaDataをそのままファイルに書き出す)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		書き込みデータ
/////////////////////////////////////////////////////////////////////////////

class CFileWriter : public CMediaDecoder  
{
public:
	CFileWriter(IEventHandler *pEventHandler = NULL);
	virtual ~CFileWriter();

// IMediaDecoder
	virtual void Reset(void);

	virtual const DWORD GetInputNum(void) const;
	virtual const DWORD GetOutputNum(void) const;

	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CFileWriter
	const bool OpenFile(LPCTSTR lpszFileName);
	void CloseFile(void);

	const LONGLONG GetWriteSize(void) const;
	const LONGLONG GetWriteCount(void) const;

protected:
	CNCachedFile m_OutFile;
	
	LONGLONG m_llWriteSize;
	LONGLONG m_llWriteCount;
};

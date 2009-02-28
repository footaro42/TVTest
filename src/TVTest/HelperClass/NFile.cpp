// NFile.cpp: CNFile クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NFile.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CNFile::CNFile()
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_pszFileName(NULL)
	, m_LastError(ERROR_SUCCESS)
{
}


CNFile::~CNFile()
{
	Close();
}


const bool CNFile::Open(LPCTSTR lpszName, const BYTE bFlags)
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_BUSY;	// 「要求したリソースは使用中です。」
		return false;
	}

	if (lpszName == NULL) {
		m_LastError = ERROR_INVALID_PARAMETER;	// 「パラメータが正しくありません。」
		return false;
	}

	// ファイルアクセス属性構築
	DWORD dwAccess = 0x00000000UL;

	if (bFlags & CNF_READ)
		dwAccess |= GENERIC_READ;
	if (bFlags & CNF_WRITE)
		dwAccess |= GENERIC_WRITE;

	if (!dwAccess) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// ファイル共有属性構築
	DWORD dwShare = 0x00000000UL;

	if (bFlags & CNF_SHAREREAD)
		dwShare |= FILE_SHARE_READ;
	if (bFlags & CNF_SHAREWRITE)
		dwShare |= FILE_SHARE_WRITE;

	// ファイル作成属性構築
	DWORD dwCreate;

	if (bFlags & CNF_NEW)
		dwCreate = CREATE_ALWAYS;
	else
		dwCreate = OPEN_EXISTING;

	// ファイルオープン
	m_hFile = ::CreateFile(lpszName, dwAccess, dwShare, NULL, dwCreate,
						   FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_pszFileName = StdUtil::strdup(lpszName);

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNFile::Close(void)
{
	// ファイルクローズ

	m_LastError = ERROR_SUCCESS;

	if (m_hFile != INVALID_HANDLE_VALUE) {
		if (!::CloseHandle(m_hFile))
			m_LastError = ::GetLastError();
		m_hFile = INVALID_HANDLE_VALUE;
		delete [] m_pszFileName;
		m_pszFileName = NULL;
	}

	return m_LastError == ERROR_SUCCESS;
}


const bool CNFile::IsOpen() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}


const DWORD CNFile::Read(void *pBuff, const DWORD dwLen)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return 0;
	}

	// ファイルリード
	DWORD dwRead = 0;

	if (!::ReadFile(m_hFile, pBuff, dwLen, &dwRead, NULL)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return dwRead;
}


const DWORD CNFile::Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// ファイルリード
	if (!SetPos(llPos))
		return 0;

	return Read(pBuff, dwLen);
}


const bool CNFile::Write(const void *pBuff, const DWORD dwLen)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// ファイルライト
	DWORD dwWritten = 0UL;

	if (!::WriteFile(m_hFile, pBuff, dwLen, &dwWritten, NULL)) {
		m_LastError = ::GetLastError();
		return false;
	}
	if (dwWritten != dwLen) {
		m_LastError = ERROR_WRITE_FAULT;
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNFile::Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// ファイルシーク
	if (!SetPos(llPos))
		return false;
	return Write(pBuff, dwLen);
}


const bool CNFile::Flush(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (!::FlushFileBuffers(m_hFile)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


const ULONGLONG CNFile::GetSize(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// ファイルサイズ取得
	LARGE_INTEGER FileSize;

	if (!::GetFileSizeEx(m_hFile, &FileSize)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return FileSize.QuadPart;
}


const ULONGLONG CNFile::GetPos(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// ポジション取得
	static const LARGE_INTEGER DistanceToMove = {0, 0};
	LARGE_INTEGER NewPos;

	if (!::SetFilePointerEx(m_hFile, DistanceToMove, &NewPos, FILE_CURRENT)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return NewPos.QuadPart;
}


const bool CNFile::SetPos(const ULONGLONG llPos)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	// ファイルシーク
	LARGE_INTEGER DistanceToMove;
	DistanceToMove.QuadPart = llPos;
	if (!::SetFilePointerEx(m_hFile, DistanceToMove, NULL, FILE_BEGIN)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


LPCTSTR CNFile::GetFileName() const
{
	return m_pszFileName;
}


DWORD CNFile::GetLastError(void) const
{
	return m_LastError;
}


DWORD CNFile::GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const
{
	if (pszMessage == NULL || MaxLength == 0)
		return 0;

	DWORD Length = ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		m_LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		pszMessage, MaxLength, NULL);
	if (Length == 0)
		pszMessage[0] = '\0';
	return Length;
}

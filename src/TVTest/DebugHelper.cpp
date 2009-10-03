#include "stdafx.h"
#include <tlhelp32.h>
#include "TVTest.h"
#include "DebugHelper.h"


#ifdef UNICODE
#undef Module32First
#undef Module32Next
#undef MODULEENTRY32
#endif


#define MAX_MODULE_ENTRIES 256


#ifdef ENABLE_DEBUG_HELPER
HMODULE CDebugHelper::m_hDbgHelp=NULL;
CDebugHelper::SymInitializeFunc CDebugHelper::m_pSymInitialize=NULL;
CDebugHelper::SymCleanupFunc CDebugHelper::m_pSymCleanup=NULL;
CDebugHelper::SymFromAddrFunc CDebugHelper::m_pSymFromAddr=NULL;
CDebugHelper::SymSetOptionsFunc CDebugHelper::m_pSymSetOptions=NULL;
CDebugHelper::SymLoadModuleFunc CDebugHelper::m_pSymLoadModule=NULL;
CDebugHelper::StackWalk64Func CDebugHelper::m_pStackWalk64=NULL;
#endif
CDebugHelper::ExceptionFilterMode CDebugHelper::m_ExceptionFilterMode=EXCEPTION_FILTER_DEFAULT;


CDebugHelper::CDebugHelper()
{
#ifdef ENABLE_DEBUG_HELPER
	::SetUnhandledExceptionFilter(ExceptionFilter);

	m_hDbgHelp=::LoadLibrary(TEXT("dbghelp.dll"));
	if (m_hDbgHelp!=NULL) {
		m_pSymInitialize=reinterpret_cast<SymInitializeFunc>(::GetProcAddress(m_hDbgHelp,"SymInitialize"));
		m_pSymCleanup=reinterpret_cast<SymCleanupFunc>(::GetProcAddress(m_hDbgHelp,"SymCleanup"));
		m_pSymFromAddr=reinterpret_cast<SymFromAddrFunc>(::GetProcAddress(m_hDbgHelp,"SymFromAddr"));
		m_pSymSetOptions=reinterpret_cast<SymSetOptionsFunc>(::GetProcAddress(m_hDbgHelp,"SymSetOptions"));
		m_pSymLoadModule=reinterpret_cast<SymLoadModuleFunc>(::GetProcAddress(m_hDbgHelp,"SymLoadModule"));
		m_pStackWalk64=reinterpret_cast<StackWalk64Func>(::GetProcAddress(m_hDbgHelp,"StackWalk64"));
		if (m_pSymInitialize==NULL || m_pSymCleanup==NULL || m_pSymFromAddr==NULL
				|| m_pSymSetOptions==NULL || m_pStackWalk64==NULL) {
			::FreeLibrary(m_hDbgHelp);
			m_hDbgHelp=NULL;
		}
	}
#endif
}


CDebugHelper::~CDebugHelper()
{
#ifdef ENABLE_DEBUG_HELPER
	if (m_hDbgHelp!=NULL)
		::FreeLibrary(m_hDbgHelp);
#endif
}


bool CDebugHelper::SetExceptionFilterMode(ExceptionFilterMode Mode)
{
	m_ExceptionFilterMode=Mode;
	return true;
}


LONG WINAPI CDebugHelper::ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
	if (m_ExceptionFilterMode==EXCEPTION_FILTER_NONE)
		return EXCEPTION_EXECUTE_HANDLER;

#ifdef ENABLE_DEBUG_HELPER
	if (m_ExceptionFilterMode==EXCEPTION_FILTER_DIALOG && m_hDbgHelp!=NULL) {
		static const struct {
			DWORD Code;
			LPCSTR pszText;
		} ExceptionCodeList[] = {
			{EXCEPTION_ACCESS_VIOLATION,		"Access violation"},
			{EXCEPTION_STACK_OVERFLOW,			"Stack overflow"},
			{EXCEPTION_IN_PAGE_ERROR,			"Page error"},
			{EXCEPTION_ILLEGAL_INSTRUCTION,		"Illegal instruction"},
			{EXCEPTION_DATATYPE_MISALIGNMENT,	"Misalignment"},
			{EXCEPTION_INT_DIVIDE_BY_ZERO,		"Divide by zero"},
			{EXCEPTION_FLT_DIVIDE_BY_ZERO,		"Floating-point divide by zero"},
		};
		LPCSTR pszExceptionCode="Other";
		for (int i=0;i<lengthof(ExceptionCodeList);i++) {
			if (ExceptionCodeList[i].Code==ExceptionInfo->ExceptionRecord->ExceptionCode) {
				pszExceptionCode=ExceptionCodeList[i].pszText;
				break;
			}
		}

		LPSTR pszText=static_cast<LPSTR>(::GlobalAlloc(GMEM_FIXED,32*1024));
		if (pszText==NULL)
			return EXCEPTION_CONTINUE_SEARCH;
		int Length=::wsprintfA(pszText,APP_NAME_A "で例外が発生しました。\r\n\r\nCode %#x (%s) / Address %p\r\n"
							   "EAX %#08x / EBX %#08x / ECX %#08x / EDX %#08x / ESI %#08x / EDI %#08x\r\n"
							   "EBP %#08x / ESP %#08x / EIP %#08x\r\n\r\n",
							   ExceptionInfo->ExceptionRecord->ExceptionCode,
							   pszExceptionCode,
							   ExceptionInfo->ExceptionRecord->ExceptionAddress,
							   ExceptionInfo->ContextRecord->Eax,
							   ExceptionInfo->ContextRecord->Ebx,
							   ExceptionInfo->ContextRecord->Ecx,
							   ExceptionInfo->ContextRecord->Edx,
							   ExceptionInfo->ContextRecord->Esi,
							   ExceptionInfo->ContextRecord->Edi,
							   ExceptionInfo->ContextRecord->Ebp,
							   ExceptionInfo->ContextRecord->Esp,
							   ExceptionInfo->ContextRecord->Eip);

		m_pSymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

		HANDLE hProcess=::GetCurrentProcess();
		if (!m_pSymInitialize(hProcess,NULL,FALSE)) {
			::GlobalFree(pszText);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		HANDLE hThread=::GetCurrentThread();

		// モジュールの列挙
		MODULEENTRY32 *pModuleEntries=NULL;
		int NumModuleEntries=0;
		if (m_pSymLoadModule!=NULL) {
			HANDLE hSnap=::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,::GetCurrentProcessId());
			if (hSnap!=INVALID_HANDLE_VALUE) {
				pModuleEntries=static_cast<MODULEENTRY32*>(::GlobalAlloc(GMEM_FIXED,sizeof(MODULEENTRY32)*MAX_MODULE_ENTRIES));

				MODULEENTRY32 me;
				me.dwSize=sizeof(me);
				if (::Module32First(hSnap,&me)) {
					do {
						m_pSymLoadModule(hProcess,NULL,me.szExePath,me.szModule,
										 reinterpret_cast<DWORD>(me.modBaseAddr),
										 me.modBaseSize);
						if (pModuleEntries!=NULL
								&& NumModuleEntries<MAX_MODULE_ENTRIES) {
							CopyMemory(&pModuleEntries[NumModuleEntries],&me,sizeof(MODULEENTRY32));
							NumModuleEntries++;
						}
					} while (::Module32Next(hSnap,&me));
				}
				::CloseHandle(hSnap);
			}
		}

		STACKFRAME64 sf;
		::ZeroMemory(&sf,sizeof(sf));
		sf.AddrPC.Offset=ExceptionInfo->ContextRecord->Eip;
		sf.AddrPC.Mode=AddrModeFlat;
		sf.AddrStack.Offset=ExceptionInfo->ContextRecord->Esp;
		sf.AddrStack.Mode=AddrModeFlat;
		sf.AddrFrame.Offset=ExceptionInfo->ContextRecord->Ebp;
		sf.AddrFrame.Mode=AddrModeFlat;

		/*
		CONTEXT context;
		::ZeroMemory(&context,sizeof(context));
		context.ContextFlags=CONTEXT_FULL;
		*/

		PSYMBOL_INFO pSymbol=static_cast<PSYMBOL_INFO>(::GlobalAlloc(GMEM_FIXED,sizeof(SYMBOL_INFO)+256));
		if (pSymbol==NULL) {
			m_pSymCleanup(hProcess);
			if (pModuleEntries!=NULL)
				::GlobalFree(pModuleEntries);
			::GlobalFree(pszText);
			return EXCEPTION_CONTINUE_SEARCH;
		}
		pSymbol->SizeOfStruct=sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen=256;

		for (int i=0;i<10;i++) {
			if (!m_pStackWalk64(IMAGE_FILE_MACHINE_I386,hProcess,hThread,
								&sf,NULL,NULL,NULL,NULL,NULL)
					|| sf.AddrPC.Offset==0
					|| sf.AddrReturn.Offset==0
					|| sf.AddrPC.Offset==sf.AddrReturn.Offset)
				break;
			DWORD64 Disp;
			if (m_pSymFromAddr(hProcess,sf.AddrReturn.Offset,&Disp,pSymbol)) {
				LPCSTR pszModule="???";
				for (int j=0;j<NumModuleEntries;j++) {
					if (sf.AddrReturn.Offset>=(DWORD64)pModuleEntries[j].modBaseAddr
							&& sf.AddrReturn.Offset<(DWORD64)pModuleEntries[j].modBaseAddr+pModuleEntries[j].modBaseSize) {
						pszModule=pModuleEntries[j].szModule;
						break;
					}
				}
				Length+=::wsprintfA(pszText+Length,"%#08x %s : %s + %#08x\r\n",
					(DWORD)sf.AddrReturn.Offset,pszModule,pSymbol->Name,Disp);
			} else {
				Length+=::wsprintfA(pszText+Length,"%#08x\r\n",sf.AddrReturn.Offset);
			}
		}

		m_pSymCleanup(hProcess);
		::GlobalFree(pSymbol);

		// 継続の問い合わせ
		BOOL fContinueExecution=FALSE;
		if (ExceptionInfo->ExceptionRecord->ExceptionFlags==EXCEPTION_NONCONTINUABLE) {
			::MessageBoxA(NULL,pszText,NULL,MB_OK | MB_ICONSTOP);
		} else {
			::lstrcpyA(pszText+Length,"\n無視して処理を継続しますか?\n※正常に動作する保障はありません。\n[いいえ] を選択するとプログラムが終了します。");
			if (::MessageBoxA(NULL,pszText,NULL,MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2)==IDYES)
				fContinueExecution=TRUE;
		}

		// ログ保存
		TCHAR szFileName[MAX_PATH];
		HANDLE hFile;
		::GetModuleFileName(NULL,szFileName,MAX_PATH);
		::lstrcat(szFileName,TEXT(".exception.log"));
		hFile=::CreateFile(szFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			if (NumModuleEntries>0) {
				Length+=::wsprintfA(pszText+Length,"\r\nModules (%d Modules)\r\n",NumModuleEntries);
				for (int i=0;i<NumModuleEntries;i++) {
					Length+=::wsprintfA(pszText+Length,"%s %#08x + %#08x\r\n",
						pModuleEntries[i].szModule,
						(DWORD)pModuleEntries[i].modBaseAddr,
						pModuleEntries[i].modBaseSize);
				}
			}
			DWORD Write;
			::WriteFile(hFile,pszText,Length,&Write,NULL);
			::CloseHandle(hFile);
		}

		if (pModuleEntries!=NULL)
			::GlobalFree(pModuleEntries);
		::GlobalFree(pszText);

		if (fContinueExecution)
			return EXCEPTION_CONTINUE_EXECUTION;

		//return EXCEPTION_EXECUTE_HANDLER;
	}
#endif	// ENABLE_DEBUG_HELPER

	return EXCEPTION_CONTINUE_SEARCH;
}

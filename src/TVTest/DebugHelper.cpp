#include "stdafx.h"
#include "TVTest.h"
#include "DebugHelper.h"




#ifndef _DEBUG
HMODULE CDebugHelper::m_hDbgHelp=NULL;
CDebugHelper::SymInitializeFunc CDebugHelper::m_pSymInitialize=NULL;
CDebugHelper::SymCleanupFunc CDebugHelper::m_pSymCleanup=NULL;
CDebugHelper::SymFromAddrFunc CDebugHelper::m_pSymFromAddr=NULL;
CDebugHelper::SymSetOptionsFunc CDebugHelper::m_pSymSetOptions=NULL;
CDebugHelper::StackWalk64Func CDebugHelper::m_pStackWalk64=NULL;
#endif
CDebugHelper::ExceptionFilterMode CDebugHelper::m_ExceptionFilterMode=EXCEPTION_FILTER_DEFAULT;


CDebugHelper::CDebugHelper()
{
#ifndef _DEBUG
	::SetUnhandledExceptionFilter(ExceptionFilter);

	m_hDbgHelp=::LoadLibrary(TEXT("dbghelp.dll"));
	if (m_hDbgHelp!=NULL) {
		m_pSymInitialize=reinterpret_cast<SymInitializeFunc>(::GetProcAddress(m_hDbgHelp,"SymInitialize"));
		m_pSymCleanup=reinterpret_cast<SymCleanupFunc>(::GetProcAddress(m_hDbgHelp,"SymCleanup"));
		m_pSymFromAddr=reinterpret_cast<SymFromAddrFunc>(::GetProcAddress(m_hDbgHelp,"SymFromAddr"));
		m_pSymSetOptions=reinterpret_cast<SymSetOptionsFunc>(::GetProcAddress(m_hDbgHelp,"SymSetOptions"));
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
#ifndef _DEBUG
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

#ifndef _DEBUG
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

		m_pSymSetOptions(/*SYMOPT_UNDNAME | */SYMOPT_DEFERRED_LOADS);

		HANDLE hProcess=::GetCurrentProcess();
		if (!m_pSymInitialize(hProcess,NULL,TRUE))
			return EXCEPTION_CONTINUE_SEARCH;
		HANDLE hThread=::GetCurrentThread();

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
		pSymbol->SizeOfStruct=sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen=256;

		LPSTR pszText=static_cast<LPSTR>(::GlobalAlloc(GMEM_FIXED,32*1024));
		int Length=::wsprintfA(pszText,APP_NAME_A "Ç≈ó·äOÇ™î≠ê∂ÇµÇ‹ÇµÇΩÅB\n\nCode %#x (%s) / Address %p\n"
							   "EAX %#08x / EBX %#08x / ECX %#08x / EDX %#08x / ESI %#08x / EDI %#08x\n"
							   "EBP %#08x / ESP %#08x / EIP %#08x\n",
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

		for (int i=0;i<10;i++) {
			if (!m_pStackWalk64(IMAGE_FILE_MACHINE_I386,hProcess,hThread,
								&sf,NULL,NULL,NULL,NULL,NULL)
					|| sf.AddrPC.Offset==0
					|| sf.AddrReturn.Offset==0
					|| sf.AddrPC.Offset==sf.AddrReturn.Offset)
				break;
			DWORD64 Disp;
			if (m_pSymFromAddr(hProcess,sf.AddrReturn.Offset,&Disp,pSymbol)) {
				Length+=::wsprintfA(pszText+Length,"%#08x %s + %#08x\n",
									sf.AddrPC.Offset,pSymbol->Name,Disp);
			} else {
				Length+=::wsprintfA(pszText+Length,"%#08x\n",sf.AddrPC.Offset);
			}
		}

		m_pSymCleanup(hProcess);
		::GlobalFree(pSymbol);

		if (ExceptionInfo->ExceptionRecord->ExceptionFlags==EXCEPTION_NONCONTINUABLE) {
			::MessageBoxA(NULL,pszText,NULL,MB_OK | MB_ICONSTOP);
		} else {
			::lstrcpyA(pszText+Length,"\nñ≥éãÇµÇƒèàóùÇåpë±ÇµÇ‹Ç∑Ç©?\nÅ¶ê≥èÌÇ…ìÆçÏÇ∑ÇÈï€è·ÇÕÇ†ÇËÇ‹ÇπÇÒÅB");
			if (::MessageBoxA(NULL,pszText,NULL,MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2)==IDYES) {
				::GlobalFree(pszText);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}

		::GlobalFree(pszText);

		//return EXCEPTION_EXECUTE_HANDLER;
	}
#endif

	return EXCEPTION_CONTINUE_SEARCH;
}

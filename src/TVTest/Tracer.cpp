#include "stdafx.h"
#include "TVTest.h"
#include "Tracer.h"




void CTracer::Trace(LPCTSTR pszOutput, ...)
{
	va_list Args;

	va_start(Args,pszOutput);
	TraceV(pszOutput,Args);
	va_end(Args);
}


void CTracer::TraceV(LPCTSTR pszOutput,va_list Args)
{
	_vsntprintf(m_szBuffer,lengthof(m_szBuffer),pszOutput,Args);
	OnTrace(m_szBuffer);
}

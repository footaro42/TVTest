#ifndef TRACER_H
#define TRACER_H


class CTracer {
	TCHAR m_szBuffer[256];
public:
	virtual ~CTracer() {}
	void Trace(LPCTSTR pszOutput, ...);
	void TraceV(LPCTSTR pszOutput,va_list Args);
	virtual void OnTrace(LPCTSTR pszOutput)=0;
};


#endif

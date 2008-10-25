#include "stdafx.h"
#include "MediaOutput.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMediaOutput::CMediaOutput(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler,1,1)
{
	m_hOutputThread=NULL;
	m_hOutputEvent=::CreateEvent(NULL,TRUE,FALSE,NULL);
	m_hBreakEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hCompleteEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
}


CMediaOutput::~CMediaOutput()
{
	Stop();
	if (m_hOutputEvent)
		::CloseHandle(m_hOutputEvent);
	if (m_hBreakEvent)
		::CloseHandle(m_hBreakEvent);
	if (m_hCompleteEvent)
		::CloseHandle(m_hCompleteEvent);
}


void CMediaOutput::Reset(void)
{
	if (m_hOutputThread) {
		::ResetEvent(m_hCompleteEvent);
		m_SignalType=SIGNAL_RESET;
		::SetEvent(m_hBreakEvent);
		::WaitForSingleObject(m_hCompleteEvent,2000/*INFINITE*/);
	}
	CMediaDecoder::Reset();
}


const bool CMediaOutput::InputMedia(CMediaData *pMediaData,const DWORD dwInputIndex)
{
	if (dwInputIndex>=GetInputNum())
		return false;

	CTsPacket *pPacket=dynamic_cast<CTsPacket*>(pMediaData);

	if (::WaitForSingleObject(m_hOutputEvent,0)==WAIT_TIMEOUT) {
		m_OutputPacket=*pPacket;
		::SetEvent(m_hOutputEvent);
	}
	return true;
}


bool CMediaOutput::Play()
{
	if (m_hOutputThread)
		return false;
	m_hOutputThread=::CreateThread(NULL,0,OutputThread,this,0,NULL);
	if (m_hOutputThread==NULL)
		return false;
	return true;
}


bool CMediaOutput::Stop()
{
	if (m_hOutputThread) {
		m_SignalType=SIGNAL_KILL;
		::SetEvent(m_hBreakEvent);
		if (::WaitForSingleObject(m_hOutputThread,2000)!=WAIT_OBJECT_0) {
			TRACE(TEXT("CMediaOutput::OutputThread Terminate\n"));
			::TerminateThread(m_hOutputThread,1);
		}
		::CloseHandle(m_hOutputThread);
		m_hOutputThread=NULL;
	}
	return true;
}


DWORD WINAPI CMediaOutput::OutputThread(LPVOID lpParameter)
{
	CMediaOutput *pThis=static_cast<CMediaOutput*>(lpParameter);
	HANDLE Handles[2];

	Handles[0]=pThis->m_hOutputEvent;
	Handles[1]=pThis->m_hBreakEvent;
	while (true) {
		while (true) {
			DWORD Result=::WaitForMultipleObjects(2,Handles,FALSE,10);

			if (Result==WAIT_OBJECT_0) {
				pThis->OutputMedia(&pThis->m_OutputPacket);
				::ResetEvent(pThis->m_hOutputEvent);
			} else if (Result==WAIT_OBJECT_0+1)
				break;
		}
		::ResetEvent(pThis->m_hOutputEvent);
		if (pThis->m_SignalType==SIGNAL_KILL)
			break;
		::SetEvent(pThis->m_hCompleteEvent);
	}
	return 0;
}

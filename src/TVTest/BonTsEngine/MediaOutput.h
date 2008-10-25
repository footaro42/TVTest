#ifndef MEDIA_OUTPUT_H
#define MEDIA_OUTPUT_H


#include "MediaDecoder.h"
#include "TsStream.h"


class CMediaOutput : public CMediaDecoder {
	CTsPacket m_OutputPacket;
	HANDLE m_hOutputThread;
	HANDLE m_hOutputEvent;
	HANDLE m_hBreakEvent;
	HANDLE m_hCompleteEvent;
	volatile enum {
		SIGNAL_KILL,
		SIGNAL_RESET
	} m_SignalType;
	static DWORD WINAPI OutputThread(LPVOID lpParameter);
public:
	CMediaOutput(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaOutput();
	// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);
	// CMediaOutput
	bool Play();
	bool Stop();
};


#endif

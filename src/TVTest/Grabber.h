#ifndef GRABBER_H
#define GRABBER_H


class CGrabber {
	class CGrabberFilter *m_pGrabberFilter;
public:
	CGrabber();
	~CGrabber();
	bool Init();
	IBaseFilter *GetGrabberFilter();
	bool SetCapture(bool fCapture);
	bool WaitCapture(DWORD WaitTime);
	void *GetCaptureBitmap() const;
};


#endif

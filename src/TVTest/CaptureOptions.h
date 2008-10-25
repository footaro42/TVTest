#ifndef CAPTURE_OPTIONS_H
#define CAPTURE_OPTIONS_H


#include "Image.h"
#include "Options.h"


class CCaptureOptions : public COptions {
public:
	enum {
		SIZE_ORIGINAL,
		SIZE_VIEW,
		SIZE_RAW,
		SIZE_1920x1080,
		SIZE_1440x810,
		SIZE_1280x720,
		SIZE_1024x576,
		SIZE_960x540,
		SIZE_800x450,
		SIZE_640x360,
		SIZE_1440x1080,
		SIZE_1280x960,
		SIZE_1024x768,
		SIZE_800x600,
		SIZE_720x540,
		SIZE_640x480,
		SIZE_CUSTOM_FIRST=SIZE_1920x1080,
		SIZE_CUSTOM_LAST=SIZE_640x480,
		SIZE_LAST=SIZE_CUSTOM_LAST
	};
	CCaptureOptions();
	~CCaptureOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	int GetSaveFormat() const { return m_SaveFormat; }
	bool GetWriteComment() const { return m_fSetComment; }
	bool SetCaptureSize(int Size);
	int GetCaptureSize() const { return m_CaptureSize; }
	bool GetCustomSize(int Size,int *pWidth,int *pHeight) const;
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength) const;
	bool GetOptionText(LPTSTR pszOption,int MaxLength) const;
	bool GetCommentText(LPTSTR pszComment,int MaxComment,
						LPCTSTR pszChannelName,LPCTSTR pszEventName);
	int TranslateCommand(int Command);
	bool OpenSaveFolder() const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
private:
	TCHAR m_szSaveFolder[MAX_PATH];
	TCHAR m_szFileName[MAX_PATH];
	int m_SaveFormat;
	int m_JPEGQuality;
	int m_PNGCompressionLevel;
	bool m_fCaptureSaveToFile;
	bool m_fSetComment;
	int m_CaptureSize;
	CImageCodec m_ImageCodec;
	static const SIZE m_SizeList[SIZE_LAST+1];
	static CCaptureOptions *GetThis(HWND hDlg);
};


#endif

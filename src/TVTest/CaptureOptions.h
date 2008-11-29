#ifndef CAPTURE_OPTIONS_H
#define CAPTURE_OPTIONS_H


#include "Image.h"
#include "Options.h"
#include "Capture.h"


class CCaptureOptions : public COptions {
public:
	enum {
		SIZE_TYPE_ORIGINAL,
		SIZE_TYPE_VIEW,
		SIZE_TYPE_RAW,
		SIZE_TYPE_CUSTOM,
		SIZE_TYPE_PERCENTAGE,
		SIZE_TYPE_LAST=SIZE_TYPE_PERCENTAGE
	};
	enum {
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
		SIZE_LAST=SIZE_640x480
	};
	enum {
		PERCENTAGE_75,
		PERCENTAGE_66,
		PERCENTAGE_50,
		PERCENTAGE_33,
		PERCENTAGE_LAST=PERCENTAGE_33
	};
	CCaptureOptions();
	~CCaptureOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	int GetSaveFormat() const { return m_SaveFormat; }
	bool GetWriteComment() const { return m_fSetComment; }
	bool SetPresetCaptureSize(int Size);
	int GetPresetCaptureSize() const;
	int GetCaptureSizeType() const { return m_CaptureSizeType; }
	bool GetSizePercentage(int *pNum,int *pDenom) const;
	bool GetCustomSize(int *pWidth,int *pHeight) const;
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength,const SYSTEMTIME *pst=NULL) const;
	bool GetOptionText(LPTSTR pszOption,int MaxLength) const;
	bool GetCommentText(LPTSTR pszComment,int MaxComment,
						LPCTSTR pszChannelName,LPCTSTR pszEventName);
	bool SaveImage(CCaptureImage *pImage);
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
	int m_CaptureSizeType;
	int m_CaptureSize;
	int m_CapturePercentage;
	CImageCodec m_ImageCodec;
	static const SIZE m_SizeList[SIZE_LAST+1];
	struct PercentageType {
		BYTE Num,Denom;
	};
	static const PercentageType m_PercentageList[PERCENTAGE_LAST+1];
	static CCaptureOptions *GetThis(HWND hDlg);
};


#endif

#ifndef INITIAL_SETTINGS_H
#define INITIAL_SETTINGS_H


#include "DriverManager.h"
#include "CoreEngine.h"
#include "Aero.h"
#include "DrawUtil.h"


class CInitialSettings
{
	enum { MAX_DECODER_NAME=128 };
	const CDriverManager *m_pDriverManager;
	TCHAR m_szDriverFileName[MAX_PATH];
	TCHAR m_szMpeg2DecoderName[MAX_DECODER_NAME];
	CVideoRenderer::RendererType m_VideoRenderer;
	CCoreEngine::CardReaderType m_CardReader;
	TCHAR m_szRecordFolder[MAX_PATH];
	CAeroGlass m_AeroGlass;
	CGdiPlus m_GdiPlus;
	CGdiPlus::CImage m_LogoImage;

	static CInitialSettings *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CInitialSettings(const CDriverManager *pDriverManager);
	~CInitialSettings();
	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool GetDriverFileName(LPTSTR pszFileName,int MaxLength) const;
	LPCTSTR GetMpeg2DecoderName() const { return m_szMpeg2DecoderName; }
	bool GetMpeg2DecoderName(LPTSTR pszDecoderName,int MaxLength) const;
	CVideoRenderer::RendererType GetVideoRenderer() const { return m_VideoRenderer; }
	CCoreEngine::CardReaderType GetCardReader() const { return m_CardReader; }
	LPCTSTR GetRecordFolder() const { return m_szRecordFolder; }
	bool ShowDialog(HWND hwndOwner);
};


#endif

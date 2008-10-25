#ifndef INITIAL_SETTINGS_H
#define INITIAL_SETTINGS_H


#include "DirectShowFilter/VideoRenderer.h"
#include "BonTsEngine/CardReader.h"


class CInitialSettings {
	TCHAR m_szDriverFileName[MAX_PATH];
	TCHAR m_szMpeg2DecoderName[128];
	CVideoRenderer::RendererType m_VideoRenderer;
	CCardReader::ReaderType m_CardReader;
	static CInitialSettings *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CInitialSettings();
	~CInitialSettings();
	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool GetDriverFileName(LPTSTR pszFileName,int MaxLength) const;
	LPCTSTR GetMpeg2DecoderName() const { return m_szMpeg2DecoderName; }
	bool GetMpeg2DecoderName(LPTSTR pszDecoderName,int MaxLength) const;
	CVideoRenderer::RendererType GetVideoRenderer() const { return m_VideoRenderer; }
	CCardReader::ReaderType GetCardReader() const { return m_CardReader; }
	bool ShowDialog(HWND hwndOwner);
};


#endif

#ifndef GENERAL_OPTIONS_H
#define GENERAL_OPTIONS_H


#include "Options.h"
#include "CoreEngine.h"
#include "DirectShowFilter/VideoRenderer.h"


class CGeneralOptions : public COptions {
public:
	enum DefaultDriverType {
		DEFAULT_DRIVER_NONE,
		DEFAULT_DRIVER_LAST,
		DEFAULT_DRIVER_CUSTOM
	};
	enum {
		MAX_MPEG2_DECODER_NAME=128
	};

	CGeneralOptions();
	~CGeneralOptions();
// COptions
	bool Apply(DWORD Flags);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CGeneralOptions
	DefaultDriverType GetDefaultDriverType() const;
	LPCTSTR GetDefaultDriverName() const;
	bool SetDefaultDriverName(LPCTSTR pszDriverName);
	bool GetFirstDriverName(LPTSTR pszDriverName) const;
	LPCTSTR GetMpeg2DecoderName() const;
	bool SetMpeg2DecoderName(LPCTSTR pszDecoderName);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	bool SetVideoRendererType(CVideoRenderer::RendererType Renderer);
	CCoreEngine::CardReaderType GetCardReaderType() const;
	bool SetCardReaderType(CCoreEngine::CardReaderType CardReader);
	void SetTemporaryNoDescramble(bool fNoDescramble);
	bool GetResident() const;
	bool GetKeepSingleTask() const;
	bool GetDescrambleUseSSE2() const { return m_fDescrambleUseSSE2; }
	bool GetDescrambleCurServiceOnly() const;
	bool GetEnableEmmProcess() const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
	TCHAR m_szDriverDirectory[MAX_PATH];
	DefaultDriverType m_DefaultDriverType;
	TCHAR m_szDefaultDriverName[MAX_PATH];
	TCHAR m_szLastDriverName[MAX_PATH];
	TCHAR m_szMpeg2DecoderName[MAX_MPEG2_DECODER_NAME];
	CVideoRenderer::RendererType m_VideoRendererType;
	CCoreEngine::CardReaderType m_CardReaderType;
	bool m_fTemporaryNoDescramble;
	bool m_fResident;
	bool m_fKeepSingleTask;
	bool m_fDescrambleUseSSE2;
	bool m_fDescrambleCurServiceOnly;
	bool m_fEnableEmmProcess;
	static CGeneralOptions *GetThis(HWND hDlg);
	enum {
		UPDATE_DECODER				= 0x00000001UL,
		UPDATE_RENDERER				= 0x00000002UL,
		UPDATE_CARDREADER			= 0x00000004UL,
		UPDATE_RESIDENT				= 0x00000008UL,
		UPDATE_DESCRAMBLECURONLY	= 0x00000010UL,
		UPDATE_ENABLEEMMPROCESS		= 0x00000020UL
	};
};


#endif

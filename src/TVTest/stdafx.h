// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

// 下で指定された定義の前に対象プラットフォームを指定しなければならない場合、以下の定義を変更してください。
// 異なるプラットフォームに対応する値に関する最新情報については、MSDN を参照してください。
#ifndef WINVER				// Windows XP 以降のバージョンに固有の機能の使用を許可します。
#define WINVER 0x0501		// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_WINNT		// Windows XP 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_WINNT 0x0501	// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_WINDOWS		// Windows 98 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_WINDOWS 0x0410 // これを Windows Me またはそれ以降のバージョン向けに適切な値に変更してください。
#endif

#ifndef _WIN32_IE			// IE 6.0 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_IE 0x0600	// これを IE の他のバージョン向けに適切な値に変更してください。
#endif

// Winsock2 とヘッダが干渉しないようにする
#define _WINSOCKAPI_

#define _WIN32_DCOM	// for CoInitializeEx()

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <streams.h>	// DirectShow BaseClasses

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#undef strdup
#define strdup strdup
#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif // _DEBUG

#undef _WINSOCKAPI_

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


// 警告の無効設定
#pragma warning(disable: 4355) // warning C4355: "'this' : ベース メンバ初期化リストで使用されました。"
#pragma warning(disable: 4995) // warning C4995: 名前が避けられた #pragma として記述されています。
#pragma warning(disable: 4996) // warning C4996: "This function or variable may be unsafe."


// ライブラリのリンク
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "ComDlg32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "ShLwApi.lib")

#ifdef _DEBUG
#pragma comment(lib, "StrmBasd.lib")
#else
#pragma comment(lib, "StrmBase.lib")
#endif

// トレース出力
#ifdef _DEBUG
	#undef TRACE
	#define TRACE ::DebugTrace
	void DebugTrace(LPCTSTR szFormat, ...);
#else
	#define TRACE __noop
#endif


// BonTsEngine の設定
#ifdef TVH264
#ifndef TVH264_FOR_HD
#define BONTSENGINE_1SEG_SUPPORT	// ワンセグ対応
#define BONTSENGINE_RADIO_SUPPORT	// 音声放送対応
#endif
#define BONTSENGINE_H264_SUPPORT	// H.264 対応
#else	// TVH264
#define BONTSENGINE_MPEG2_SUPPORT	// MPEG-2 対応
#endif

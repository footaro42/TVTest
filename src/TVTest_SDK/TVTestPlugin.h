/*
	TVTest プラグインヘッダ ver.0.0.5

	このファイルは再配布・改変など自由に行って構いません。
	ただし、改変した場合はオリジナルと違う旨を記載して頂けると、混乱がなくてい
	いと思います。

	特にコンパイラに依存する記述はないはずなので、Borland などでも大丈夫です。
	また、ヘッダを移植すれば C++ 以外の言語でプラグインを作成することも可能な
	はずです(凄く面倒なので誰もやらないと思いますが)。

	プラグインの仕様はまだ暫定ですので、今後変更される可能性があります(できるだ
	け互換性は維持したいと思っていますが)。

	実際にプラグインを作成する場合、このヘッダだけを見ても恐らく意味不明だと思
	いますので、サンプルを参考にしてください。
*/


/*
	TVTest プラグインの概要

	プラグインは32ビット DLL の形式です。拡張子は .tvtp とします。
	プラグインでは、以下の関数をエクスポートします。

	DWORD WINAPI TVTGetVersion()
	BOOL WINAPI TVTGetPluginInfo(PluginInfo *pInfo)
	BOOL WINAPI TVTInitialize(PluginParam *pParam)
	BOOL WINAPI TVTFinalize()

	各関数については、このヘッダの最後の方にあるプラグインクラスでのエクスポート
	関数の実装(#ifdef TVTEST_PLUGIN_CLASS_IMPLEMENT の部分)を参照してください。

	プラグインからは、コールバック関数を通じてメッセージを送信することにより、
	TVTest の機能を利用することができます。
	このような方法になっているのは、将来的な拡張が容易であるためです。

	また、イベントコールバック関数を登録することにより、TVTest からイベントが通
	知されます。

	メッセージの送信はスレッドセーフではありませんので、別スレッドからメッセー
	ジコールバック関数を呼び出さないでください。

	TVTEST_PLUGIN_CLASS_IMPLEMENT シンボルが #define されていると、エクスポート
	関数を直接記述しなくても、クラスとしてプラグインを記述することができます。
	その場合、TVTestPlugin クラスからプラグインクラスを派生させます。

	以下は最低限の実装を行ったサンプルです。

	#include <windows.h>
	#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// プラグインをクラスとして実装
	#include "TVTestPlugin.h"

	// プラグインクラス。CTVTestPlugin から派生させる
	class CMyPlugin : public TVTest::CTVTestPlugin
	{
	public:
		virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo) {
			// プラグインの情報を返す
			pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
			pInfo->Flags          = 0;
			pInfo->pszPluginName  = L"サンプル";
			pInfo->pszCopyright   = L"Copyright(c) 2008 Taro Yamada";
			pInfo->pszDescription = L"何もしないプラグイン";
			return true;	// false を返すとプラグインのロードが失敗になる
		}
		virtual bool Initialize() {
			// ここで初期化を行う
			// 何もしないのであればオーバーライドしなくても良い
			return true;	// false を返すとプラグインのロードが失敗になる
		}
		virtual bool Finalize() {
			// ここでクリーンアップを行う
			// 何もしないのであればオーバーライドしなくても良い
			return true;
		}
	};

	// CreatePluginClass 関数で、プラグインクラスのインスタンスを生成して返す
	TVTest::CTVTestPlugin *CreatePluginClass()
	{
		return new CMyPlugin;
	}
*/


/*
	更新履歴

    ver.0.0.5 (TVTest ver.0.5.41 or later)
    ・以下のイベントを追加した
      ・EVENT_RESET
      ・EVENT_STATUSRESET
      ・EVENT_AUDIOSTREAMCHANGE
    ・MESSAGE_RESETSTATUS を追加した

	ver.0.0.4 (TVTest ver.0.5.33 or later)
	・EVENT_EXECUTE を追加した

	ver.0.0.3 (TVTest ver.0.5.27 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_ISPLUGINENABLED
	  ・MESSAGE_REGISTERCOMMAND
	  ・MESSAGE_ADDLOG
	・EVENT_STANDBY と EVENT_COMMAND を追加した

	ver.0.0.2
	・MESSAGE_GETAUDIOSTREAM と MESSAGE_SETAUDIOSTREAM を追加した
	・ServiceInfo 構造体に AudioComponentType と SubtitlePID メンバを追加した
	・StatusInfo 構造体に DropPacketCount と BcasCardStatus メンバを追加した

	ver.0.0.1
	・以下のメッセージを追加した
	  ・MESSAGE_QUERYEVENT
	  ・MESSAGE_GETTUNINGSPACE
	  ・MESSAGE_GETTUNINGSPACEINFO
	  ・MESSAGE_SETNEXTCHANNEL
	・ChannelInfo 構造体にいくつかメンバを追加した

	ver.0.0.0
	・最初のバージョン
*/


#ifndef TVTEST_PLUGIN_H
#define TVTEST_PLUGIN_H


#include <pshpack1.h>


namespace TVTest {


// プラグインのバージョン
#define TVTEST_PLUGIN_VERSION_0_0_0	0x00000000UL
#define TVTEST_PLUGIN_VERSION_0_0_1	0x00000001UL
#define TVTEST_PLUGIN_VERSION_0_0_2	0x00000002UL
#define TVTEST_PLUGIN_VERSION_0_0_3	0x00000003UL
#define TVTEST_PLUGIN_VERSION_0_0_4	0x00000004UL
#define TVTEST_PLUGIN_VERSION_0_0_5	0x00000005UL
#ifndef TVTEST_PLUGIN_VERSION
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_0_0_5
#endif

// エクスポート関数定義用
#define TVTEST_EXPORT(type) extern "C" __declspec(dllexport) type WINAPI

// offsetof
#define TVTEST_OFFSETOF(type,member) \
	((size_t)((BYTE*)&((type*)0)->member-(BYTE*)(type*)0))

// プラグインの種類
enum {
	PLUGIN_TYPE_NORMAL	// 普通
};

// プラグインのフラグ
enum {
	PLUGIN_FLAG_HASSETTINGS		=0x00000001UL,	// 設定ダイアログがある
	PLUGIN_FLAG_ENABLEDEFAULT	=0x00000002UL	// デフォルトで有効
												// 特別な理由が無い限り使わない
};

// プラグインの情報
struct PluginInfo {
	DWORD Type;				// 種類(PLUGIN_TYPE_???)
	DWORD Flags;			// フラグ(PLUGIN_FLAG_???)
	LPCWSTR pszPluginName;	// プラグイン名
	LPCWSTR pszCopyright;	// 著作権情報
	LPCWSTR pszDescription;	// 説明文
};

// メッセージ送信用コールバック関数
typedef LRESULT (CALLBACK *MessageCallbackFunc)(struct PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);

// プラグインパラメータ
struct PluginParam {
	MessageCallbackFunc Callback;	// コールバック関数
	HWND hwndApp;					// メインウィンドウのハンドル
	void *pClientData;				// プラグイン側で好きに使えるデータ
	void *pInternalData;			// TVTest側で使用するデータ。アクセス禁止
};

// エクスポート関数
typedef DWORD (WINAPI *GetVersionFunc)();
typedef BOOL (WINAPI *GetPluginInfoFunc)(PluginInfo *pInfo);
typedef BOOL (WINAPI *InitializeFunc)(PluginParam *pParam);
typedef BOOL (WINAPI *FinalizeFunc)();

// メッセージ
enum {
	MESSAGE_GETVERSION,
	MESSAGE_QUERYMESSAGE,
	MESSAGE_MEMORYALLOC,
	MESSAGE_SETEVENTCALLBACK,
	MESSAGE_GETCURRENTCHANNELINFO,
	MESSAGE_SETCHANNEL,
	MESSAGE_GETSERVICE,
	MESSAGE_SETSERVICE,
	MESSAGE_GETTUNINGSPACENAME,
	MESSAGE_GETCHANNELINFO,
	MESSAGE_GETSERVICEINFO,
	MESSAGE_GETDRIVERNAME,
	MESSAGE_SETDRIVERNAME,
	MESSAGE_STARTRECORD,
	MESSAGE_STOPRECORD,
	MESSAGE_PAUSERECORD,
	MESSAGE_GETRECORD,
	MESSAGE_MODIFYRECORD,
	MESSAGE_GETZOOM,
	MESSAGE_SETZOOM,
	MESSAGE_GETPANSCAN,
	MESSAGE_SETPANSCAN,
	MESSAGE_GETSTATUS,
	MESSAGE_GETRECORDSTATUS,
	MESSAGE_GETVIDEOINFO,
	MESSAGE_GETVOLUME,
	MESSAGE_SETVOLUME,
	MESSAGE_GETSTEREOMODE,
	MESSAGE_SETSTEREOMODE,
	MESSAGE_GETFULLSCREEN,
	MESSAGE_SETFULLSCREEN,
	MESSAGE_GETPREVIEW,
	MESSAGE_SETPREVIEW,
	MESSAGE_GETSTANDBY,
	MESSAGE_SETSTANDBY,
	MESSAGE_GETALWAYSONTOP,
	MESSAGE_SETALWAYSONTOP,
	MESSAGE_CAPTUREIMAGE,
	MESSAGE_SAVEIMAGE,
	MESSAGE_RESET,
	MESSAGE_CLOSE,
	MESSAGE_SETSTREAMCALLBACK,
	MESSAGE_ENABLEPLUGIN,
	MESSAGE_GETCOLOR,
	MESSAGE_DECODEARIBSTRING,
	MESSAGE_GETCURRENTPROGRAMINFO,
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1
	MESSAGE_QUERYEVENT,
	MESSAGE_GETTUNINGSPACE,
	MESSAGE_GETTUNINGSPACEINFO,
	MESSAGE_SETNEXTCHANNEL,
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
	MESSAGE_GETAUDIOSTREAM,
	MESSAGE_SETAUDIOSTREAM,
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3
	MESSAGE_ISPLUGINENABLED,
	MESSAGE_REGISTERCOMMAND,
	MESSAGE_ADDLOG,
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5
	MESSAGE_RESETSTATUS,
#endif
	MESSAGE_TRAILER
};

// イベント用コールバック関数
typedef LRESULT (CALLBACK *EventCallbackFunc)(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);

// イベント
// 各イベント発生時のパラメータは CTVTestEventHadler を参照してください。
enum {
	EVENT_PLUGINENABLE,			// 有効状態が変化した
	EVENT_PLUGINSETTINGS,		// 設定を行う
	EVENT_CHANNELCHANGE,		// チャンネルが変更された
	EVENT_SERVICECHANGE,		// サービスが変更された
	EVENT_DRIVERCHANGE,			// ドライバが変更された
	EVENT_SERVICEUPDATE,		// サービスの構成が変化した
	EVENT_RECORDSTATUSCHANGE,	// 録画状態が変化した
	EVENT_FULLSCREENCHANGE,		// 全画面表示状態が変化した
	EVENT_PREVIEWCHANGE,		// プレビュー表示状態が変化した
	EVENT_VOLUMECHANGE,			// 音量が変化した
	EVENT_STEREOMODECHANGE,		// ステレオモードが変化した
	EVENT_COLORCHANGE,			// 色の設定が変化した
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3
	EVENT_STANDBY,				// 待機状態が変化した
	EVENT_COMMAND,				// コマンドが選択された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_4
	EVENT_EXECUTE,				// 複数起動禁止時に複数起動された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5
	EVENT_RESET,				// リセットされた
	EVENT_STATUSRESET,			// ステータスがリセットされた
	EVENT_AUDIOSTREAMCHANGE,	// 音声ストリームが変更された
#endif
	EVENT_TRAILER
};

inline DWORD MakeVersion(BYTE Major,WORD Minor,WORD Build) {
	return ((DWORD)Major<<24) | ((DWORD)Minor<<12) | Build;
}
inline DWORD GetMajorVersion(DWORD Version) { return Version>>24; }
inline DWORD GetMinorVersion(DWORD Version) { return (Version&0x00FFF000UL)>>12; }
inline DWORD GetBuildVersion(DWORD Version) { return Version&0x00000FFFUL; }

// TVTestのバージョンを取得する
// 上位8ビットがメジャーバージョン、次の12ビットがマイナーバージョン、
// 下位12ビットがビルドナンバー
// GetMajorVersion/GetMinorVersion/GetBuildVersionを使って取得できる
inline DWORD MsgGetVersion(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETVERSION,0,0);
}

// 指定されたメッセージに対応しているか問い合わせる
inline bool MsgQueryMessage(PluginParam *pParam,UINT Message) {
	return (*pParam->Callback)(pParam,MESSAGE_QUERYMESSAGE,Message,0)!=0;
}

// メモリ再確保
// 仕様はreallocと同じ
inline void *MsgMemoryReAlloc(PluginParam *pParam,void *pData,DWORD Size) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)pData,Size);
}

// メモリ確保
// 仕様はrealloc(NULL,Size)と同じ
inline void *MsgMemoryAlloc(PluginParam *pParam,DWORD Size) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)(void*)NULL,Size);
}

// メモリ開放
// 仕様はrealloc(pData,0)と同じ
// (実際にreallocでメモリ開放しているコードは見たこと無いけど...)
inline void MsgMemoryFree(PluginParam *pParam,void *pData) {
	(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)pData,0);
}

// イベントハンドル用コールバックの設定
// pClientDataはコールバックの呼び出し時に渡される
inline bool MsgSetEventCallback(PluginParam *pParam,EventCallbackFunc Callback,void *pClientData=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_SETEVENTCALLBACK,(LPARAM)Callback,(LPARAM)pClientData)!=0;
}

// チャンネルの情報
struct ChannelInfo {
	DWORD Size;							// 構造体のサイズ
	int Space;							// チューニング空間(BonDriverのインデックス)
	int Channel;						// チャンネル(BonDriverのインデックス)
	int RemoteControlKeyID;				// リモコンID
	WORD NetworkID;						// ネットワークID
	WORD TransportStreamID;				// トランスポートストリームID
	WCHAR szNetworkName[32];			// ネットワーク名
	WCHAR szTransportStreamName[32];	// トランスポートストリーム名
	WCHAR szChannelName[64];			// チャンネル名
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1
	int PhysicalChannel;				// 物理チャンネル番号(場合によっては信用できない)
										// 不明の場合は0
	WORD ServiceIndex;					// サービスのインデックス
	WORD ServiceID;						// サービスID
	// サービスはチャンネルファイルで設定されているものが取得される
	// サービスはユーザーが切り替えられるので、実際に視聴中のサービスがこれであるとは限らない
	// 実際に視聴中のサービスは MESSAGE_GETSERVICE で取得できる
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1
enum { CHANNELINFO_SIZE_V1=TVTEST_OFFSETOF(ChannelInfo,PhysicalChannel) };
#endif

// 現在のチャンネルの情報を取得する
// 事前に ChannelInfo の Size メンバを設定しておく
inline bool MsgGetCurrentChannelInfo(PluginParam *pParam,ChannelInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCURRENTCHANNELINFO,(LPARAM)pInfo,0)!=0;
}

// チャンネルを設定する
inline bool MsgSetChannel(PluginParam *pParam,int Space,int Channel) {
	return (*pParam->Callback)(pParam,MESSAGE_SETCHANNEL,Space,Channel)!=0;
}

// 現在のサービス及びサービス数を取得する
// サービスのインデックスが返る。エラー時は-1が返る
// pNumServices が NULL でない場合は、サービスの数が返される
inline int MsgGetService(PluginParam *pParam,int *pNumServices=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSERVICE,(LPARAM)pNumServices,0);
}

// サービスを設定する
// fByID=false の場合はインデックス、fByID=trueの場合はサービスID
inline bool MsgSetService(PluginParam *pParam,int Service,bool fByID=false) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSERVICE,Service,fByID)!=0;
}

// チューニング空間名を取得する
// チューニング空間名の長さが返る。Indexが範囲外の場合は0が返る
// pszNameをNULLで呼べば長さだけを取得できる
inline int MsgGetTuningSpaceName(PluginParam *pParam,int Index,LPWSTR pszName,int MaxLength) {
	return (*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACENAME,(LPARAM)pszName,MAKELPARAM(Index,min(MaxLength,0xFFFF)));
}

// チャンネルの情報を取得する
// 事前に ChannelInfo の Size メンバを設定しておく
// szNetworkName,szTransportStreamName は MESSAGE_GETCURRENTCHANNEL でしか取得できない
// NetworkID,TransportStreamID は取得できたりできなかったり(取得できなかった場合は0)
inline bool MsgGetChannelInfo(PluginParam *pParam,int Space,int Index,ChannelInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCHANNELINFO,(LPARAM)pInfo,MAKELPARAM(Space,Index))!=0;
}

// サービスの情報
struct ServiceInfo {
	DWORD Size;					// 構造体のサイズ
	WORD ServiceID;				// サービスID
	WORD VideoPID;				// ビデオストリームのPID
	int NumAudioPIDs;			// 音声PIDの数
	WORD AudioPID[4];			// 音声ストリームのPID
	WCHAR szServiceName[32];	// サービス名
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
	BYTE AudioComponentType[4];	// 音声コンポーネントタイプ
	WORD SubtitlePID;			// 字幕ストリームのPID(無い場合は0)
	WORD Reserved;				// 予約
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
enum { SERVICEINFO_SIZE_V1=TVTEST_OFFSETOF(ServiceInfo,AudioComponentType) };
#endif

// サービスの情報を取得する
// 事前にServiceInfoのSizeメンバを設定しておく
inline bool MsgGetServiceInfo(PluginParam *pParam,int Index,ServiceInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSERVICEINFO,Index,(LPARAM)pInfo)!=0;
}

// ドライバのファイル名を取得する
// 戻り値はファイル名の長さ(ナルを除く)
// pszNameをNULLで呼べば長さだけを取得できる
inline int MsgGetDriverName(PluginParam *pParam,LPWSTR pszName,int MaxLength) {
	return (*pParam->Callback)(pParam,MESSAGE_GETDRIVERNAME,(LPARAM)pszName,MaxLength);
}

// ドライバを設定する
inline bool MsgSetDriverName(PluginParam *pParam,LPCWSTR pszName) {
	return (*pParam->Callback)(pParam,MESSAGE_SETDRIVERNAME,(LPARAM)pszName,0)!=0;
}

// 録画情報のマスク
enum {
	RECORD_MASK_FLAGS		=0x00000001UL,
	RECORD_MASK_FILENAME	=0x00000002UL,
	RECORD_MASK_STARTTIME	=0x00000004UL,
	RECORD_MASK_STOPTIME	=0x00000008UL
};

// 録画フラグ
enum {
	RECORD_FLAG_CANCEL		=0x10000000UL	// キャンセル
};

// 録画開始時間の指定方法
enum {
	RECORD_START_NOTSPECIFIED,	// 未指定
	RECORD_START_TIME,			// 時刻指定
	RECORD_START_DELAY			// 長さ指定
};

// 録画停止時間の指定方法
enum {
	RECORD_STOP_NOTSPECIFIED,	// 未指定
	RECORD_STOP_TIME,			// 時刻指定
	RECORD_STOP_DURATION		// 長さ指定
};

// 録画情報
struct RecordInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Mask;				// マスク(RECORD_MASK_???)
	DWORD Flags;			// フラグ(RECORD_FLAG_???)
	LPWSTR pszFileName;		// ファイル名(NULLでデフォルト)
	int MaxFileName;		// ファイル名の最大長(MESSAGE_GETRECORDのみで使用)
	FILETIME ReserveTime;	// 録画予約された時刻(MESSAGE_GETRECORDのみで使用)
	DWORD StartTimeSpec;	// 録画開始時間の指定方法(RECORD_START_???)
	union {
		FILETIME Time;		// 録画開始時刻(StartTimeSpec==RECORD_START_TIME)
							// ローカル時刻
		ULONGLONG Delay;	// 録画開始時間(StartTimeSpec==RECORD_START_DELAY)
							// 録画を開始するまでの時間(ms)
	} StartTime;
	DWORD StopTimeSpec;		// 録画停止時間の指定方法(RECORD_STOP_???)
	union {
		FILETIME Time;		// 録画停止時刻(StopTimeSpec==RECORD_STOP_TIME)
							// ローカル時刻
		ULONGLONG Duration;	// 録画停止時間(StopTimeSpec==RECORD_STOP_DURATION)
							// 開始時間からのミリ秒
	} StopTime;
};

// 録画を開始する
// pInfo が NULL で即時録画開始
inline bool MsgStartRecord(PluginParam *pParam,const RecordInfo *pInfo=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_STARTRECORD,(LPARAM)pInfo,0)!=0;
}

// 録画を停止する
inline bool MsgStopRecord(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_STOPRECORD,0,0)!=0;
}

// 録画を一時停止/再開する
inline bool MsgPauseRecord(PluginParam *pParam,bool fPause=true) {
	return (*pParam->Callback)(pParam,MESSAGE_PAUSERECORD,fPause,0)!=0;
}

// 録画設定を取得する
inline bool MsgGetRecord(PluginParam *pParam,RecordInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETRECORD,(LPARAM)pInfo,0)!=0;
}

// 録画設定を変更する
// 既に録画中である場合は、ファイル名と開始時間の指定は無視される
inline bool MsgModifyRecord(PluginParam *pParam,const RecordInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_MODIFYRECORD,(LPARAM)pInfo,0)!=0;
}

// 表示倍率を取得する(%単位)
inline int MsgGetZoom(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETZOOM,0,0);
}

// 表示倍率を設定する
// %単位だけではなく、Num=1/Denom=3などとして割り切れない倍率を設定することもできる
inline bool MsgSetZoom(PluginParam *pParam,int Num,int Denom=100) {
	return (*pParam->Callback)(pParam,MESSAGE_SETZOOM,Num,Denom)!=0;
}

// パンスキャンの種類
enum {
	PANSCAN_NONE,		// なし
	PANSCAN_LETTERBOX,	// レターボックス
	PANSCAN_SIDECUT,	// サイドカット
	PANSCAN_SUPERFRAME	// 超額縁
};

// パンスキャンの情報
struct PanScanInfo {
	DWORD Size;		// 構造体のサイズ
	int Type;		// 種類(PANSCAN_???)
	int XAspect;	// 水平アスペクト比
	int YAspect;	// 垂直アスペクト比
};

// パンスキャンの設定を取得する
inline bool MsgGetPanScan(PluginParam *pParam,PanScanInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETPANSCAN,(LPARAM)pInfo,0)!=0;
}

// パンスキャンを設定する
inline bool MsgSetPanScan(PluginParam *pParam,const PanScanInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPANSCAN,(LPARAM)pInfo,0)!=0;
}

// B-CAS カードの状態
enum {
	BCAS_STATUS_OK,				// エラーなし
	BCAS_STATUS_NOTOPEN,		// 開かれていない(スクランブル解除なし)
	BCAS_STATUS_NOCARDREADER,	// カードリーダが無い
	BCAS_STATUS_NOCARD,			// カードがない
	BCAS_STATUS_OPENERROR,		// オープンエラー
	BCAS_STATUS_TRANSMITERROR,	// 通信エラー
	BCAS_STATUS_ESTABLISHERROR	// コンテキスト確立失敗
};

// ステータス情報
struct StatusInfo {
	DWORD Size;							// 構造体のサイズ
	float SignalLevel;					// 信号レベル(dB)
	DWORD BitRate;						// ビットレート(Bits/Sec)
	DWORD ErrorPacketCount;				// エラーパケット数
										// DropPacketCount も含まれる
	DWORD ScramblePacketCount;			// 復号漏れパケット数
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
	DWORD DropPacketCount;				// ドロップパケット数
	DWORD BcasCardStatus;				// B-CAS カードの状態(BCAS_STATUS_???)
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
enum { STATUSINFO_SIZE_V1=TVTEST_OFFSETOF(StatusInfo,DropPacketCount) };
#endif

// ステータスを取得する
// 事前にStatusInfoのSizeメンバを設定しておく
inline bool MsgGetStatus(PluginParam *pParam,StatusInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTATUS,(LPARAM)pInfo,0)!=0;
}

// 録画の状態
enum {
	RECORD_STATUS_NOTRECORDING,	// 録画していない
	RECORD_STATUS_RECORDING,	// 録画中
	RECORD_STATUS_PAUSED		// 録画一時停止中
};

// 録画ステータス情報
struct RecordStatusInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Status;			// 状態(RECORD_STATUS_???)
	FILETIME StartTime;		// 録画開始時間(ローカル時間)
	DWORD RecordTime;		// 録画時間(ms) 一時停止中を含まない
	DWORD PauseTime;		// 一時停止時間(ms)
	DWORD StopTimeSpec;		// 録画停止時間の指定方法(RECORD_STOP_???)
	union {
		FILETIME Time;		// 録画停止時間(StopTimeSpec==RECORD_STOP_TIME)
							// ローカル時間
		ULONGLONG Duration;	// 録画停止時間(StopTimeSpec==RECORD_STOP_DURATION)
							// 開始時間(StartTime)からのミリ秒
	} StopTime;
};

// 録画ステータスを取得する
// 事前にRecordStatusInfoのSizeメンバを設定しておく
inline bool MsgGetRecordStatus(PluginParam *pParam,RecordStatusInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETRECORDSTATUS,(LPARAM)pInfo,0)!=0;
}

// 映像の情報
struct VideoInfo {
	DWORD Size;			// 構造体のサイズ
	int Width;			// 幅(ピクセル単位)
	int Height;			// 高さ(ピクセル単位)
	int XAspect;		// 水平アスペクト比
	int YAspect;		// 垂直アスペクト比
	RECT SourceRect;	// ソースの表示範囲
};

// 映像の情報を取得する
// 事前にVideoInfoのSizeメンバを設定しておく
inline bool MsgGetVideoInfo(PluginParam *pParam,VideoInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETVIDEOINFO,(LPARAM)pInfo,0)!=0;
}

// 音量を取得する(0-100)
inline int MsgGetVolume(PluginParam *pParam) {
	return LOWORD((*pParam->Callback)(pParam,MESSAGE_GETVOLUME,0,0));
}

// 音量を設定する(0-100)
inline bool MsgSetVolume(PluginParam *pParam,int Volume) {
	return (*pParam->Callback)(pParam,MESSAGE_SETVOLUME,Volume,0)!=0;
}

// 消音状態であるか取得する
inline bool MsgGetMute(PluginParam *pParam) {
	return HIWORD((*pParam->Callback)(pParam,MESSAGE_GETVOLUME,0,0))!=0;
}

// 消音状態を設定する
inline bool MsgSetMute(PluginParam *pParam,bool fMute) {
	return (*pParam->Callback)(pParam,MESSAGE_SETVOLUME,-1,fMute)!=0;
}

// ステレオモード
enum {
	STEREOMODE_STEREO,	// ステレオ
	STEREOMODE_LEFT,	// 左(主音声)
	STEREOMODE_RIGHT	// 右(副音声)
};

// ステレオモードを取得する
inline int MsgGetStereoMode(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTEREOMODE,0,0);
}

// ステレオモードを設定する
inline bool MsgSetStereoMode(PluginParam *pParam,int StereoMode) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSTEREOMODE,StereoMode,0)!=0;
}

// 全画面表示の状態を取得する
inline bool MsgGetFullscreen(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETFULLSCREEN,0,0)!=0;
}

// 全画面表示の状態を設定する
inline bool MsgSetFullscreen(PluginParam *pParam,bool fFullscreen) {
	return (*pParam->Callback)(pParam,MESSAGE_SETFULLSCREEN,fFullscreen,0)!=0;
}

// 表示が有効であるか取得する
inline bool MsgGetPreview(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETPREVIEW,0,0)!=0;
}

// 表示の有効状態を設定する
inline bool MsgSetPreview(PluginParam *pParam,bool fPreview) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPREVIEW,fPreview,0)!=0;
}

// 待機状態であるか取得する
inline bool MsgGetStandby(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTANDBY,0,0)!=0;
}

// 待機状態を設定する
inline bool MsgSetStandby(PluginParam *pParam,bool fStandby) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSTANDBY,fStandby,0)!=0;
}

// 常に最前面表示の状態を取得する
inline bool MsgGetAlwaysOnTop(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETALWAYSONTOP,0,0)!=0;
}

// 常に最前面表示の状態を設定する
inline bool MsgSetAlwaysOnTop(PluginParam *pParam,bool fAlwaysOnTop) {
	return (*pParam->Callback)(pParam,MESSAGE_SETALWAYSONTOP,fAlwaysOnTop,0)!=0;
}

// 画像をキャプチャする
// 戻り値はDIBデータへのポインタ
// 不要になった場合はMsgMemoryFreeで開放する
inline void *MsgCaptureImage(PluginParam *pParam,DWORD Flags=0) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_CAPTUREIMAGE,Flags,0);
}

// 画像を保存する
inline bool MsgSaveImage(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_SAVEIMAGE,0,0)!=0;
}

// リセットを行う
inline bool MsgReset(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_RESET,0,0)!=0;
}

// ウィンドウクローズのフラグ
enum {
	CLOSE_EXIT	=0x00000001UL	// 必ず終了させる
};

// ウィンドウを閉じる
inline bool MsgClose(PluginParam *pParam,DWORD Flags=0) {
	return (*pParam->Callback)(pParam,MESSAGE_CLOSE,Flags,0)!=0;
}

// ストリームコールバック関数
typedef BOOL (CALLBACK *StreamCallbackFunc)(BYTE *pData,void *pClientData);

// ストリームコールバックフラグ
enum {
	STREAM_CALLBACK_REMOVE	=0x00000001UL	// コールバックの削除
};

// ストリームコールバックの情報
struct StreamCallbackInfo {
	DWORD Size;						// 構造体のサイズ
	DWORD Flags;					// フラグ(STREAM_CALLBACK_???)
	StreamCallbackFunc Callback;	// コールバック関数
	void *pClientData;				// コールバック関数に渡されるデータ
};

// ストリームコールバックを設定する
// ストリームコールバック関数で処理が遅延すると全体が遅延するので、
// 時間が掛かる処理は別スレッドで行うなどしてください
inline bool MsgSetStreamCallback(PluginParam *pParam,DWORD Flags,
					StreamCallbackFunc Callback,void *pClientData=NULL) {
	StreamCallbackInfo Info;
	Info.Size=sizeof(StreamCallbackInfo);
	Info.Flags=Flags;
	Info.Callback=Callback;
	Info.pClientData=pClientData;
	return (*pParam->Callback)(pParam,MESSAGE_SETSTREAMCALLBACK,(LPARAM)&Info,0)!=0;
}

// プラグインの有効状態を設定する
inline bool MsgEnablePlugin(PluginParam *pParam,bool fEnable) {
	return (*pParam->Callback)(pParam,MESSAGE_ENABLEPLUGIN,fEnable,0)!=0;
}

// 色の設定を取得する
inline COLORREF MsgGetColor(PluginParam *pParam,LPCWSTR pszColor) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCOLOR,(LPARAM)pszColor,0);
}

// ARIB文字列のデコード情報
struct ARIBStringDecodeInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Flags;			// フラグ(現在は常に0)
	const void *pSrcData;	// 変換元データ
	DWORD SrcLength;		// 変換元サイズ(バイト単位)
	LPWSTR pszDest;			// 変換先バッファ
	DWORD DestLength;		// 変換先バッファのサイズ(文字単位)
};

// ARIB文字列をデコードする
inline bool MsgDecodeARIBString(PluginParam *pParam,const void *pSrcData,
							DWORD SrcLength,LPWSTR pszDest,DWORD DestLength) {
	ARIBStringDecodeInfo Info;
	Info.Size=sizeof(ARIBStringDecodeInfo);
	Info.Flags=0;
	Info.pSrcData=pSrcData;
	Info.SrcLength=SrcLength;
	Info.pszDest=pszDest;
	Info.DestLength=DestLength;
	return (*pParam->Callback)(pParam,MESSAGE_DECODEARIBSTRING,(LPARAM)&Info,0)!=0;
}

// 番組の情報
struct ProgramInfo {
	DWORD Size;				// 構造体のサイズ
	WORD ServiceID;			// サービスID
	WORD EventID;			// イベントID
	LPWSTR pszEventName;	// イベント名
	int MaxEventName;		// イベント名の最大長
	LPWSTR pszEventText;	// イベントテキスト
	int MaxEventText;		// イベントテキストの最大長
	LPWSTR pszEventExtText;	// 追加イベントテキスト
	int MaxEventExtText;	// 追加イベントテキストの最大長
	SYSTEMTIME StartTime;	// 開始時間(ローカル時間)
	DWORD Duration;			// 長さ(秒単位)
};

// 現在の番組の情報を取得する
inline bool MsgGetCurrentProgramInfo(PluginParam *pParam,ProgramInfo *pInfo,bool fNext=false) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCURRENTPROGRAMINFO,(LPARAM)pInfo,fNext)!=0;
}

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1

// 指定されたイベントに対応しているか取得する
inline bool MsgQueryEvent(PluginParam *pParam,UINT Event) {
	return (*pParam->Callback)(pParam,MESSAGE_QUERYEVENT,Event,0)!=0;
}

// 現在のチューニング空間及びチューニング空間数を取得する
inline int MsgGetTuningSpace(PluginParam *pParam,int *pNumSpaces=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACE,(LPARAM)pNumSpaces,0);
}

// チューニング空間の種類
enum {
	TUNINGSPACE_UNKNOWN,		// 不明
	TUNINGSPACE_TERRESTRIAL,	// 地上デジタル
	TUNINGSPACE_BS,				// BS
	TUNINGSPACE_110CS			// 110度CS
};

// チューニング空間の情報
struct TuningSpaceInfo {
	DWORD Size;			// 構造体のサイズ
	int Space;			// チューニング空間の種類(TUNINGSPACE_???)
						// 場合によっては信用できない
	WCHAR szName[64];	// チューニング空間名
};

// チューニング空間の情報を取得する
// 事前に TuningSpaceInfo の Size メンバを設定しておく
inline bool MsgGetTuningSpaceInfo(PluginParam *pParam,int Index,TuningSpaceInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACENAME,Index,(LPARAM)pInfo)!=0;
}

// チャンネルを次に設定する
inline bool MsgSetNextChannel(PluginParam *pParam,bool fNext=true)
{
	return (*pParam->Callback)(pParam,MESSAGE_SETNEXTCHANNEL,fNext,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2

// 現在の音声ストリームを取得する
// 音声ストリームの数は MESSAGE_GETSERVICEINFO で取得できる
inline int MsgGetAudioStream(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETAUDIOSTREAM,0,0);
}

// 音声ストリームを設定する
inline bool MsgSetAudioStream(PluginParam *pParam,int Index)
{
	return (*pParam->Callback)(pParam,MESSAGE_SETAUDIOSTREAM,Index,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3

// プラグインの有効状態を取得する
inline bool MsgIsPluginEnabled(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_ISPLUGINENABLED,0,0)!=0;
}

// コマンドの情報
struct CommandInfo {
	int ID;				// 識別子
	LPCWSTR pszText;	// コマンドの文字列
	LPCWSTR pszName;	// コマンドの名前
};

// コマンドを登録する
// TVTInitialize 内で呼びます。
// 以下のように使用します。
// MsgRegisterCommand(pParam, ID_MYCOMMAND, L"MyCommand", L"私のコマンド");
// コマンドが実行されると EVENT_COMMAND イベントが送られます。
// その際、パラメータとして識別子が渡されます。
inline bool MsgRegisterCommand(PluginParam *pParam,int ID,LPCWSTR pszText,LPCWSTR pszName)
{
	CommandInfo Info;
	Info.ID=ID;
	Info.pszText=pszText;
	Info.pszName=pszName;
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERCOMMAND,(LPARAM)&Info,1)!=0;
}

// コマンドを登録する
// TVTInitialize 内で呼びます。
inline bool MsgRegisterCommand(PluginParam *pParam,const CommandInfo *pCommandList,int NumCommands)
{
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERCOMMAND,(LPARAM)pCommandList,NumCommands)!=0;
}

// ログを記録する
// 設定のログの項目に表示されます。
inline bool MsgAddLog(PluginParam *pParam,LPCWSTR pszText)
{
	return (*pParam->Callback)(pParam,MESSAGE_ADDLOG,(LPARAM)pszText,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5

// ステータス(MESSAGE_GETSTATUS で取得できる内容)をリセットする
// リセットが行われると EVENT_STATUSRESET が送られる
inline bool MsgResetStatus(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_RESETSTATUS,0,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5


/*
	TVTest アプリケーションクラス

	TVTest の各種機能を呼び出すためのクラス。
	ただのラッパーなので使わなくてもいいです。
	TVTInitialize 関数が呼ばれた時に、引数の PluginParam 構造体へのポインタを
	コンストラクタに渡してインスタンスを生成します。
*/
class CTVTestApp
{
protected:
	PluginParam *m_pParam;
public:
	CTVTestApp(PluginParam *pParam) : m_pParam(pParam) {}
	virtual ~CTVTestApp() {}
	HWND GetAppWindow() {
		return m_pParam->hwndApp;
	}
	DWORD GetVersion() {
		return MsgGetVersion(m_pParam);
	}
	bool QueryMessage(UINT Message) {
		return MsgQueryMessage(m_pParam,Message);
	}
	void *MemoryReAlloc(void *pData,DWORD Size) {
		return MsgMemoryReAlloc(m_pParam,pData,Size);
	}
	void *MemoryAlloc(DWORD Size) {
		return MsgMemoryAlloc(m_pParam,Size);
	}
	void MemoryFree(void *pData) {
		MsgMemoryFree(m_pParam,pData);
	}
	bool SetEventCallback(EventCallbackFunc Callback,void *pClientData=NULL) {
		return MsgSetEventCallback(m_pParam,Callback,pClientData);
	}
	bool GetCurrentChannelInfo(ChannelInfo *pInfo) {
		pInfo->Size=sizeof(ChannelInfo);
		return MsgGetCurrentChannelInfo(m_pParam,pInfo);
	}
	bool SetChannel(int Space,int Channel) {
		return MsgSetChannel(m_pParam,Space,Channel);
	}
	int GetService(int *pNumServices=NULL) {
		return MsgGetService(m_pParam,pNumServices);
	}
	bool SetService(int Service,bool fByID=false) {
		return MsgSetService(m_pParam,Service,fByID);
	}
	int GetTuningSpaceName(int Index,LPWSTR pszName,int MaxLength) {
		return MsgGetTuningSpaceName(m_pParam,Index,pszName,MaxLength);
	}
	bool GetChannelInfo(int Space,int Index,ChannelInfo *pInfo) {
		pInfo->Size=sizeof(ChannelInfo);
		return MsgGetChannelInfo(m_pParam,Space,Index,pInfo);
	}
	bool GetServiceInfo(int Index,ServiceInfo *pInfo) {
		pInfo->Size=sizeof(ServiceInfo);
		return MsgGetServiceInfo(m_pParam,Index,pInfo);
	}
	int GetDriverName(LPWSTR pszName,int MaxLength) {
		return MsgGetDriverName(m_pParam,pszName,MaxLength);
	}
	bool SetDriverName(LPCWSTR pszName) {
		return MsgSetDriverName(m_pParam,pszName);
	}
	bool StartRecord(RecordInfo *pInfo=NULL) {
		if (pInfo!=NULL)
			pInfo->Size=sizeof(RecordInfo);
		return MsgStartRecord(m_pParam,pInfo);
	}
	bool StopRecord() {
		return MsgStopRecord(m_pParam);
	}
	bool PauseRecord(bool fPause=true) {
		return MsgPauseRecord(m_pParam,fPause);
	}
	bool GetRecord(RecordInfo *pInfo) {
		pInfo->Size=sizeof(RecordInfo);
		return MsgGetRecord(m_pParam,pInfo);
	}
	bool ModifyRecord(RecordInfo *pInfo) {
		pInfo->Size=sizeof(RecordInfo);
		return MsgModifyRecord(m_pParam,pInfo);
	}
	int GetZoom() {
		return MsgGetZoom(m_pParam);
	}
	int SetZoom(int Num,int Denom=100) {
		return MsgSetZoom(m_pParam,Num,Denom);
	}
	bool GetPanScan(PanScanInfo *pInfo) {
		pInfo->Size=sizeof(PanScanInfo);
		return MsgGetPanScan(m_pParam,pInfo);
	}
	bool SetPanScan(PanScanInfo *pInfo) {
		pInfo->Size=sizeof(PanScanInfo);
		return MsgSetPanScan(m_pParam,pInfo);
	}
	bool GetStatus(StatusInfo *pInfo) {
		pInfo->Size=sizeof(StatusInfo);
		return MsgGetStatus(m_pParam,pInfo);
	}
	bool GetRecordStatus(RecordStatusInfo *pInfo) {
		pInfo->Size=sizeof(RecordStatusInfo);
		return MsgGetRecordStatus(m_pParam,pInfo);
	}
	bool GetVideoInfo(VideoInfo *pInfo) {
		pInfo->Size=sizeof(VideoInfo);
		return MsgGetVideoInfo(m_pParam,pInfo);
	}
	int GetVolume() {
		return MsgGetVolume(m_pParam);
	}
	bool SetVolume(int Volume) {
		return MsgSetVolume(m_pParam,Volume);
	}
	bool GetMute() {
		return MsgGetMute(m_pParam);
	}
	bool SetMute(bool fMute) {
		return MsgSetMute(m_pParam,fMute);
	}
	int GetStereoMode() {
		return MsgGetStereoMode(m_pParam);
	}
	bool SetStereoMode(int StereoMode) {
		return MsgSetStereoMode(m_pParam,StereoMode);
	}
	bool GetFullscreen() {
		return MsgGetFullscreen(m_pParam);
	}
	bool SetFullscreen(bool fFullscreen) {
		return MsgSetFullscreen(m_pParam,fFullscreen);
	}
	bool GetPreview() {
		return MsgGetPreview(m_pParam);
	}
	bool SetPreview(bool fPreview) {
		return MsgSetPreview(m_pParam,fPreview);
	}
	bool GetStandby() {
		return MsgGetStandby(m_pParam);
	}
	bool SetStandby(bool fStandby) {
		return MsgSetStandby(m_pParam,fStandby);
	}
	bool GetAlwaysOnTop() {
		return MsgGetAlwaysOnTop(m_pParam);
	}
	bool SetAlwaysOnTop(bool fAlwaysOnTop) {
		return MsgSetAlwaysOnTop(m_pParam,fAlwaysOnTop);
	}
	void *CaptureImage(DWORD Flags=0) {
		return MsgCaptureImage(m_pParam,Flags);
	}
	bool SaveImage() {
		return MsgSaveImage(m_pParam);
	}
	bool Reset() {
		return MsgReset(m_pParam);
	}
	bool Close(DWORD Flags=0) {
		return MsgClose(m_pParam,Flags);
	}
	bool SetStreamCallback(DWORD Flags,StreamCallbackFunc Callback,void *pClientData=NULL) {
		return MsgSetStreamCallback(m_pParam,Flags,Callback,pClientData);
	}
	bool EnablePlugin(bool fEnable) {
		return MsgEnablePlugin(m_pParam,fEnable);
	}
	COLORREF GetColor(LPCWSTR pszColor) {
		return MsgGetColor(m_pParam,pszColor);
	}
	bool DecodeARIBString(const void *pSrcData,DWORD SrcLength,
						  LPWSTR pszDest,DWORD DestLength) {
		return MsgDecodeARIBString(m_pParam,pSrcData,SrcLength,pszDest,DestLength);
	}
	bool GetCurrentProgramInfo(ProgramInfo *pInfo,bool fNext=false) {
		pInfo->Size=sizeof(ProgramInfo);
		return MsgGetCurrentProgramInfo(m_pParam,pInfo,fNext);
	}
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_1
	bool QueryEvent(UINT Event) {
		return MsgQueryEvent(m_pParam,Event);
	}
	int GetTuningSpace(int *pNumSpaces=NULL) {
		return MsgGetTuningSpace(m_pParam,pNumSpaces);
	}
	bool GetTuningSpaceInfo(int Index,TuningSpaceInfo *pInfo) {
		pInfo->Size=sizeof(TuningSpaceInfo);
		return MsgGetTuningSpaceInfo(m_pParam,Index,pInfo);
	}
	bool SetNextChannel(bool fNext=true) {
		return MsgSetNextChannel(m_pParam,fNext);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_2
	int GetAudioStream() {
		return MsgGetAudioStream(m_pParam);
	}
	bool SetAudioStream(int Index) {
		return MsgSetAudioStream(m_pParam,Index);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3
	bool IsPluginEnabled() {
		return MsgIsPluginEnabled(m_pParam);
	}
	bool RegisterCommand(int ID,LPCWSTR pszText,LPCWSTR pszName) {
		return MsgRegisterCommand(m_pParam,ID,pszText,pszName);
	}
	bool RegisterCommand(const CommandInfo *pCommandList,int NumCommands) {
		return MsgRegisterCommand(m_pParam,pCommandList,NumCommands);
	}
	bool AddLog(LPCWSTR pszText) {
		return MsgAddLog(m_pParam,pszText);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5
	bool ResetStatus() {
		return MsgResetStatus(m_pParam);
	}
#endif
};

/*
	TVTest プラグインクラス

	プラグインをクラスとして記述するための抽象クラスです。
	このクラスを各プラグインで派生させて、プラグインの内容を記述します。
	このクラスを使わずに直接エクスポート関数を書いてもいいです。
*/
class CTVTestPlugin
{
protected:
	PluginParam *m_pPluginParam;
	CTVTestApp *m_pApp;
public:
	CTVTestPlugin() : m_pPluginParam(NULL),m_pApp(NULL) {}
	void SetPluginParam(PluginParam *pParam) {
		m_pPluginParam=pParam;
		m_pApp=new CTVTestApp(pParam);
	}
	virtual ~CTVTestPlugin() { delete m_pApp; }
	virtual DWORD GetVersion() { return TVTEST_PLUGIN_VERSION; }
	virtual bool GetPluginInfo(PluginInfo *pInfo)=0;
	virtual bool Initialize() { return true; }
	virtual bool Finalize() { return true; }
};

/*
	イベントハンドルクラス

	イベント処理用クラスです。
	このクラスを派生させてイベント処理を行うことができます。
	イベントコールバック関数として登録した関数内で HandleEvent を呼びます。
	もちろん使わなくてもいいです。

	以下は実装例です。

	class CMyEventHandler : public TVTest::CTVTestEventHandler
	{
	public:
		virtual bool OnPluginEnable(bool fEnable) {
			if (fEnable) {
				if (MessageBox(NULL, TEXT("有効にするよ"), TEXT("イベント"),
							   MB_OKCANCEL) != IDOK) {
					return false;
				}
			}
			return true;
		}
	};

	CMyEventHandler Handler;

	// この関数がイベントコールバック関数として登録されているものとします
	LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
	{
		Handler.HandleEvent(Event,lParam1,lParam2,pClientData);
	}
*/
class CTVTestEventHandler
{
protected:
	void *m_pClientData;

	// イベントハンドラは、特に記述の無いものは何か処理したら true を返します

	// 有効状態が変化した
	// 変化を拒否する場合 false を返します
	virtual bool OnPluginEnable(bool fEnable) { return false; }
	// 設定を行う
	// プラグインのフラグに PLUGIN_FLAG_HASSETTINGS が設定されている場合に呼ばれます
	// 設定が OK されたら true を返します
	virtual bool OnPluginSettings(HWND hwndOwner) { return false; }
	// チャンネルが変更された
	virtual bool OnChannelChange() { return false; }
	// サービスが変更された
	virtual bool OnServiceChange() { return false; }
	// ドライバが変更された
	virtual bool OnDriverChange() { return false; }
	// サービスの構成が変化した
	virtual bool OnServiceUpdate() { return false; }
	// 録画状態が変化した
	virtual bool OnRecordStatusChange(int Status) { return false; }
	// 全画面表示状態が変化した
	virtual bool OnFullscreenChange(bool fFullscreen) { return false; }
	// プレビュー表示状態が変化した
	virtual bool OnPreviewChange(bool fPreview) { return false; }
	// 音量が変化した
	virtual bool OnVolumeChange(int Volume,bool fMute) { return false; }
	// ステレオモードが変化した
	virtual bool OnStereoModeChange(int StereoMode) { return false; }
	// 色の設定が変化した
	virtual bool OnColorChange() { return false; }
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3
	// 待機状態が変化した
	virtual bool OnStandby(bool fStandby) { return false; }
	// コマンドが選択された
	virtual bool OnCommand(int ID) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_4
	// 複数起動禁止時に複数起動された
	virtual bool OnExecute(LPCWSTR pszCommandLine) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5
	// リセットされた
	virtual bool OnReset() { return false; }
	// ステータス(MESSAGE_GETSTUATUSで取得できる内容)がリセットされた
	virtual bool OnStatusReset() { return false; }
	// 音声ストリームが変更された
	virtual bool OnAudioStreamChange(int Stream) { return false; }
#endif
public:
	virtual ~CTVTestEventHandler() {}
	LRESULT HandleEvent(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData) {
		m_pClientData=pClientData;
		switch (Event) {
		case EVENT_PLUGINENABLE:		return OnPluginEnable(lParam1!=0);
		case EVENT_PLUGINSETTINGS:		return OnPluginSettings((HWND)lParam1);
		case EVENT_CHANNELCHANGE:		return OnChannelChange();
		case EVENT_SERVICECHANGE:		return OnServiceChange();
		case EVENT_DRIVERCHANGE:		return OnDriverChange();
		case EVENT_SERVICEUPDATE:		return OnServiceUpdate();
		case EVENT_RECORDSTATUSCHANGE:	return OnRecordStatusChange(lParam1);
		case EVENT_FULLSCREENCHANGE:	return OnFullscreenChange(lParam1!=0);
		case EVENT_PREVIEWCHANGE:		return OnPreviewChange(lParam1!=0);
		case EVENT_VOLUMECHANGE:		return OnVolumeChange(lParam1,lParam2!=0);
		case EVENT_STEREOMODECHANGE:	return OnStereoModeChange(lParam1);
		case EVENT_COLORCHANGE:			return OnColorChange();
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_3
		case EVENT_STANDBY:				return OnStandby(lParam1!=0);
		case EVENT_COMMAND:				return OnCommand(lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_4
		case EVENT_EXECUTE:				return OnExecute((LPCWSTR)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_0_0_5
		case EVENT_RESET:				return OnReset();
		case EVENT_STATUSRESET:			return OnStatusReset();
		case EVENT_AUDIOSTREAMCHANGE:	return OnAudioStreamChange(lParam1);
#endif
		}
		return 0;
	}
};


}	// namespace TVTest


#include <poppack.h>


#ifdef TVTEST_PLUGIN_CLASS_IMPLEMENT
/*
	プラグインをクラスとして記述できるようにするための、エクスポート関数の実装
	です。
	これを使えば、エクスポート関数を自分で実装する必要がなくなります。

	プラグイン側では CreatePluginClass 関数を実装して、CTVTestPlugin クラスから
	派生させたプラグインクラスのインスタンスを new で生成して返します。例えば、
	以下のように書きます。

	TVTest::CTVTestPlugin *CreatePluginClass()
	{
		return new CMyPluginClass;
	}
*/


TVTest::CTVTestPlugin *CreatePluginClass();

HINSTANCE g_hinstDLL;
TVTest::CTVTestPlugin *g_pPlugin;


BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		g_hinstDLL=hinstDLL;
		g_pPlugin=CreatePluginClass();
		if (g_pPlugin==NULL)
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		if (g_pPlugin) {
			delete g_pPlugin;
			g_pPlugin=NULL;
		}
		break;
	}
	return TRUE;
}

// プラグインの準拠するプラグイン仕様のバージョンを返す
// プラグインがロードされると最初にこの関数が呼ばれ、
// 対応していないバージョンが返された場合はすぐにアンロードされます。
TVTEST_EXPORT(DWORD) TVTGetVersion()
{
	return g_pPlugin->GetVersion();
}

// プラグインの情報を取得する
// TVTGetVersion の次に呼ばれるので、プラグインの情報を PluginInfo 構造体に設定します。
// FALSE が返された場合、すぐにアンロードされます。
TVTEST_EXPORT(BOOL) TVTGetPluginInfo(TVTest::PluginInfo *pInfo)
{
	return g_pPlugin->GetPluginInfo(pInfo);
}

// 初期化を行う
// TVTGetPluginInfo の次に呼ばれるので、初期化処理を行います。
// FALSE が返された場合、すぐにアンロードされます。
TVTEST_EXPORT(BOOL) TVTInitialize(TVTest::PluginParam *pParam)
{
	g_pPlugin->SetPluginParam(pParam);
	return g_pPlugin->Initialize();
}

// 終了処理を行う
// プラグインがアンロードされる前に呼ばれるので、終了処理を行います。
// この関数が呼ばれるのは TVTInitialize 関数が TRUE を返した場合だけです。
TVTEST_EXPORT(BOOL) TVTFinalize()
{
	bool fOK=g_pPlugin->Finalize();
	delete g_pPlugin;
	g_pPlugin=NULL;
	return fOK;
}


#endif	// TVTEST_PLUGIN_CLASS_IMPLEMENT


#endif	// TVTEST_PLUGIN_H

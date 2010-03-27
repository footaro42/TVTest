#ifndef __EPG_BON_COMMON_DEF_H__
#define __EPG_BON_COMMON_DEF_H__

#include <windows.h>
#include <TCHAR.h>
#include "Util.h"
#include "StructDef.h"

#define SAVE_FOLDER L"\\EpgTimerBon"
#define EPG_SAVE_FOLDER L"\\EpgData"
#define LOGO_SAVE_FOLDER L"\\LogoData"
#define BON_DLL_FOLDER L"\\BonDriver"

#define EPG_TIMER_SERVICE_EXE L"EpgTimerSrv.exe"

#define EPG_TIMER_BON_MUTEX L"Global\\EpgTimer_Bon2"
#define EPG_TIMER_BON_SRV_MUTEX L"Global\\EpgTimer_Bon_Service"
#define SERVICE_NAME L"EpgTimer_Bon Service"

#define ERR_FALSE FALSE //汎用エラー
#define NO_ERR TRUE //成功
#define ERR_INIT		10
#define ERR_NOT_INIT	11
#define ERR_SIZE		12
#define ERR_NEED_NEXT_PACKET	20 //次のTSパケット入れないと解析できない
#define ERR_CAN_NOT_ANALYZ		21 //本当にTSパケット？解析不可能
#define ERR_NOT_FIRST 			22 //最初のTSパケット未入力
#define ERR_INVALID_PACKET		23 //本当にTSパケット？パケット飛んで壊れてるかも
#define ERR_NO_CHAGE			30	//バージョンの変更ないため解析不要

#define NO_ERR_EPG_ALL 100 //EPG情報貯まった BasicとExtend両方
#define NO_ERR_EPG_BASIC 101 //EPG情報貯まった Basicのみ
#define NO_ERR_EPG_EXTENDED 102 //EPG情報貯まった Extendのみ


#define RECMODE_ALL 0 //全サービス
#define RECMODE_SERVICE 1 //指定サービスのみ
#define RECMODE_ALL_NOB25 2 //全サービス（B25処理なし）
#define RECMODE_SERVICE_NOB25 3 //指定サービスのみ（B25処理なし）
#define RECMODE_VIEW 4 //視聴
#define RECMODE_NO 5 //無効
#define RECMODE_EPG 0xFF //EPG取得

#define RESERVE_EXECUTE 0 //普通に予約実行
#define RESERVE_PILED_UP 1 //重なって実行できない予約あり
#define RESERVE_NO_EXECUTE 2 //重なって実行できない
#define RESERVE_NO 3 //無効

#define RECSERVICEMODE_DEF	0x00000000	//デフォルト設定
#define RECSERVICEMODE_SET	0x00000001	//設定値使用
#define RECSERVICEMODE_CAP	0x00000010	//字幕データ含む
#define RECSERVICEMODE_DATA	0x00000020	//データカルーセル含む

static wstring charContentTBL[]={
	L"ニュース・報道",
	L"スポーツ",
	L"情報・ワイドショー",
	L"ドラマ",
	L"音楽",
	L"バラエティ",
	L"映画",
	L"アニメ・漫画",
	L"ドキュメンタリー・教養",
	L"劇場・公演",
	L"趣味・教育",
	L"福祉"
};

__int64 _Create64Key( DWORD dwONID, DWORD dwTSID, DWORD dwSID );
void _SafeReleaseEpgDataInfo(EPG_DATA_INFO* pData);
void _SafeReleaseServiceInfo(SERVICE_INFO* pData);
void _SafeReleaseEpgDataInfo2(EPG_DATA_INFO2* pData);
void _SafeReleaseEpgDataInfo3(EPG_DATA_INFO3* pData);
BOOL _GetStartEndTime(RESERVE_DATA* pItem, int iDefStartMargine, int iDefEndMargine, __int64* i64Start, __int64* i64End, SYSTEMTIME* StartTime, SYSTEMTIME* EndTime );
unsigned long Crc32(int n,  BYTE c[]);

#endif

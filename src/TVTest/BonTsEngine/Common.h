// Common.h: BonTsEngine共通ヘッダ
//
//////////////////////////////////////////////////////////////////////

#ifndef BONTSENGINE_COMMON_H
#define BONTSENGINE_COMMON_H


/* PID */
#define PID_PAT		0x0000U	// PAT
#define PID_CAT		0x0001U	// CAT
#define PID_NIT		0x0010U	// NIT
#define PID_SDT		0x0011U	// SDT
#define PID_HEIT	0x0012U	// H-EIT
#define PID_TOT		0x0014U	// TOT
#define PID_LEIT	0x0027U	// L-EIT
#define PID_CDT		0x0029U	// CDT

/* stream_type */
#define STREAM_TYPE_MPEG1			0x01	// MPEG-1
#define STREAM_TYPE_MPEG2			0x02	// MPEG-2
#define STREAM_TYPE_CAPTION			0x06	// 字幕
#define STREAM_TYPE_DATACARROUSEL	0x0D	// データ放送
#define STREAM_TYPE_AAC				0x0F	// AAC
#define STREAM_TYPE_H264			0x1B	// H.264


#endif

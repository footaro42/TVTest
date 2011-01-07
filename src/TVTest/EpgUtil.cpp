#include "stdafx.h"
#include "TVTest.h"
#include "EpgUtil.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




bool CEpgIcons::Load()
{
	return CBitmap::Load(GetAppClass().GetInstance(),IDB_PROGRAMGUIDEICONS,LR_DEFAULTCOLOR);
}


bool CEpgIcons::Draw(HDC hdcDst,int DstX,int DstY,
					 HDC hdcSrc,int Icon,int Width,int Height,BYTE Opacity)
{
	if (hdcDst==NULL || hdcSrc==NULL || Icon<0 || Icon>ICON_LAST
			|| Width<=0 || Height<=0)
		return false;
	if (Opacity==255) {
		::BitBlt(hdcDst,DstX,DstY,Width,Height,
				 hdcSrc,Icon*ICON_WIDTH,0,SRCCOPY);
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::AlphaBlend(hdcDst,DstX,DstY,Width,Height,
					 hdcSrc,Icon*ICON_WIDTH,0,Width,Height,
					 bf);
	}
	return true;
}


unsigned int CEpgIcons::GetEventIcons(const CEventInfoData *pEventInfo)
{
	unsigned int ShowIcons=0;

	if (pEventInfo->m_VideoInfo.ComponentType>=0xB1
			&& pEventInfo->m_VideoInfo.ComponentType<=0xB4)
		ShowIcons|=1<<ICON_HD;
	else if (pEventInfo->m_VideoInfo.ComponentType>=0x01
			&& pEventInfo->m_VideoInfo.ComponentType<=0x04)
		ShowIcons|=1<<ICON_SD;

	if (pEventInfo->m_AudioList.size()>0) {
		const CEventInfoData::AudioInfo *pAudioInfo=pEventInfo->GetMainAudioInfo();

		if (pAudioInfo->ComponentType==0x02) {
			if (pAudioInfo->bESMultiLingualFlag
					&& pAudioInfo->LanguageCode!=pAudioInfo->LanguageCode2)
				ShowIcons|=1<<ICON_MULTILINGUAL;
			else
				ShowIcons|=1<<ICON_SUB;
		} else {
			if (pAudioInfo->ComponentType==0x09)
				ShowIcons|=1<<ICON_5_1CH;
			if (pEventInfo->m_AudioList.size()>=2
					&& pEventInfo->m_AudioList[0].LanguageCode!=0
					&& pEventInfo->m_AudioList[1].LanguageCode!=0) {
				if (pEventInfo->m_AudioList[0].LanguageCode!=
						pEventInfo->m_AudioList[1].LanguageCode)
					ShowIcons|=1<<ICON_MULTILINGUAL;
				else
					ShowIcons|=1<<ICON_SUB;
			}
		}
	}

	if (pEventInfo->m_FreeCaMode==CEventInfoData::FREE_CA_MODE_UNSCRAMBLED
			&& pEventInfo->m_NetworkID>=4 && pEventInfo->m_NetworkID<=10)
		ShowIcons|=1<<ICON_FREE;

	return ShowIcons;
}

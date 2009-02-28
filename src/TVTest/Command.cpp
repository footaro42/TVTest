#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Command.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"


static const struct {
	LPCTSTR pszText;
	WORD Command;
} CommandList[] = {
	{TEXT("Zoom20"),				CM_ZOOM_20},
	{TEXT("Zoom25"),				CM_ZOOM_25},
	{TEXT("Zoom33"),				CM_ZOOM_33},
	{TEXT("Zoom50"),				CM_ZOOM_50},
	{TEXT("Zoom66"),				CM_ZOOM_66},
	{TEXT("Zoom75"),				CM_ZOOM_75},
	{TEXT("Zoom100"),				CM_ZOOM_100},
	{TEXT("Zoom150"),				CM_ZOOM_150},
	{TEXT("Zoom200"),				CM_ZOOM_200},
	{TEXT("AspectRatio"),			CM_ASPECTRATIO},
	{TEXT("AspectDefault"),			CM_ASPECTRATIO_DEFAULT},
	{TEXT("Aspect16x9"),			CM_ASPECTRATIO_16x9},
	{TEXT("LetterBox"),				CM_ASPECTRATIO_LETTERBOX},
	{TEXT("SuperFrame"),			CM_ASPECTRATIO_SUPERFRAME},
	{TEXT("SideCut"),				CM_ASPECTRATIO_SIDECUT},
	{TEXT("Aspect4x3"),				CM_ASPECTRATIO_4x3},
	{TEXT("Fullscreen"),			CM_FULLSCREEN},
	{TEXT("AlwaysOnTop"),			CM_ALWAYSONTOP},
	{TEXT("ChannelUp"),				CM_CHANNEL_UP},
	{TEXT("ChannelDown"),			CM_CHANNEL_DOWN},
	{TEXT("Mute"),					CM_VOLUME_MUTE},
	{TEXT("VolumeUp"),				CM_VOLUME_UP},
	{TEXT("VolumeDown"),			CM_VOLUME_DOWN},
	{TEXT("VolumeNormalizeNone"),	CM_VOLUMENORMALIZE_NONE},
	{TEXT("VolumeNormalize125"),	CM_VOLUMENORMALIZE_125},
	{TEXT("VolumeNormalize150"),	CM_VOLUMENORMALIZE_150},
	{TEXT("VolumeNormalize200"),	CM_VOLUMENORMALIZE_200},
	{TEXT("SwitchAudio"),			CM_SWITCHAUDIO},
	{TEXT("Stereo"),				CM_STEREO_THROUGH},
	{TEXT("StereoLeft"),			CM_STEREO_LEFT},
	{TEXT("StereoRight"),			CM_STEREO_RIGHT},
	{TEXT("Record"),				CM_RECORD},
	{TEXT("RecordStart"),			CM_RECORD_START},
	{TEXT("RecordStop"),			CM_RECORD_STOP},
	{TEXT("RecordPause"),			CM_RECORD_PAUSE},
	{TEXT("RecordOption"),			CM_RECORDOPTION},
	{TEXT("RecordStopTime"),		CM_RECORDSTOPTIME},
	{TEXT("DisableViewer"),			CM_DISABLEVIEWER},
	{TEXT("CopyImage"),				CM_COPY},
	{TEXT("SaveImage"),				CM_SAVEIMAGE},
	{TEXT("CapturePreview"),		CM_CAPTUREPREVIEW},
	{TEXT("Reset"),					CM_RESET},
	{TEXT("Panel"),					CM_INFORMATION},
	{TEXT("ProgramGuide"),			CM_PROGRAMGUIDE},
	{TEXT("StatusBar"),				CM_STATUSBAR},
	{TEXT("TitleBar"),				CM_TITLEBAR},
	{TEXT("VideoDecoderProperty"),	CM_DECODERPROPERTY},
	{TEXT("Options"),				CM_OPTIONS},
	{TEXT("StreamInfo"),			CM_STREAMINFO},
	{TEXT("Close"),					CM_CLOSE},
	{TEXT("Menu"),					CM_MENU},
	{TEXT("Channel1"),				CM_CHANNELNO_1},
	{TEXT("Channel2"),				CM_CHANNELNO_2},
	{TEXT("Channel3"),				CM_CHANNELNO_3},
	{TEXT("Channel4"),				CM_CHANNELNO_4},
	{TEXT("Channel5"),				CM_CHANNELNO_5},
	{TEXT("Channel6"),				CM_CHANNELNO_6},
	{TEXT("Channel7"),				CM_CHANNELNO_7},
	{TEXT("Channel8"),				CM_CHANNELNO_8},
	{TEXT("Channel9"),				CM_CHANNELNO_9},
	{TEXT("Channel10"),				CM_CHANNELNO_10},
	{TEXT("Channel11"),				CM_CHANNELNO_11},
	{TEXT("Channel12"),				CM_CHANNELNO_12},
	{TEXT("Service1"),				CM_SERVICE_FIRST},
	{TEXT("Service2"),				CM_SERVICE_FIRST+1},
	{TEXT("Service3"),				CM_SERVICE_FIRST+2},
	{TEXT("Service4"),				CM_SERVICE_FIRST+3},
	{TEXT("Service5"),				CM_SERVICE_FIRST+4},
	{TEXT("Service6"),				CM_SERVICE_FIRST+5},
	{TEXT("Service7"),				CM_SERVICE_FIRST+6},
	{TEXT("Service8"),				CM_SERVICE_FIRST+7},
	{TEXT("TuningSpace1"),			CM_SPACE_FIRST},
	{TEXT("TuningSpace2"),			CM_SPACE_FIRST+1},
	{TEXT("TuningSpace3"),			CM_SPACE_FIRST+2},
	{TEXT("TuningSpace4"),			CM_SPACE_FIRST+3},
	{TEXT("TuningSpace5"),			CM_SPACE_FIRST+4},
};




CCommandList::CCommandList()
{
}


CCommandList::~CCommandList()
{
	m_DriverList.DeleteAll();
	m_PluginList.DeleteAll();
	m_PluginCommandList.DeleteAll();
}


bool CCommandList::Initialize(const CDriverManager *pDriverManager,const CPluginList *pPluginList)
{
	m_DriverList.DeleteAll();
	if (pDriverManager!=NULL) {
		for (int i=0;i<pDriverManager->NumDrivers();i++)
			m_DriverList.Add(DuplicateString(::PathFindFileName(pDriverManager->GetDriverInfo(i)->GetFileName())));
	}
	m_PluginList.DeleteAll();
	m_PluginCommandList.DeleteAll();
	if (pPluginList!=NULL) {
		for (int i=0;i<pPluginList->NumPlugins();i++) {
			const CPlugin *pPlugin=pPluginList->GetPlugin(i);
			LPCTSTR pszFileName=::PathFindFileName(pPlugin->GetFileName());

			m_PluginList.Add(DuplicateString(pszFileName));
			for (int j=0;j<pPlugin->NumPluginCommands();j++) {
				TVTest::CommandInfo Info;
				LPTSTR pszText;

				pPlugin->GetPluginCommandInfo(j,&Info);
				pszText=new TCHAR[::lstrlen(pszFileName)+1+::lstrlen(Info.pszText)+1+::lstrlen(Info.pszName)+1];
				::wsprintf(pszText,TEXT("%s:%s%c%s"),
						   pszFileName,Info.pszText,'\0',Info.pszName);
				m_PluginCommandList.Add(pszText);
			}
		}
	}
	return true;
}


int CCommandList::NumCommands() const
{
	return lengthof(CommandList)+m_DriverList.Length()+
		m_PluginList.Length()+m_PluginCommandList.Length();
}


int CCommandList::GetCommandID(int Index) const
{
	int Base;

	if (Index<0 || Index>=NumCommands())
		return 0;
	if (Index<lengthof(CommandList))
		return CommandList[Index].Command;
	Base=lengthof(CommandList);
	if (Index<Base+m_DriverList.Length())
		return CM_DRIVER_FIRST+Index-Base;
	Base+=m_DriverList.Length();
	if (Index<Base+m_PluginList.Length())
		return CM_PLUGIN_FIRST+Index-Base;
	Base+=m_PluginList.Length();
	return CM_PLUGINCOMMAND_FIRST+Index-Base;
}


LPCTSTR CCommandList::GetCommandText(int Index) const
{
	int Base;

	if (Index<0 || Index>=NumCommands())
		return NULL;
	if (Index<lengthof(CommandList))
		return CommandList[Index].pszText;
	Base=lengthof(CommandList);
	if (Index<Base+m_DriverList.Length())
		return m_DriverList[Index-Base];
	Base+=m_DriverList.Length();
	if (Index<Base+m_PluginList.Length())
		return m_PluginList[Index-Base];
	Base+=m_PluginList.Length();
	return m_PluginCommandList[Index-Base];
}


int CCommandList::GetCommandName(int Index,LPTSTR pszName,int MaxLength) const
{
	int Base;

	if (pszName==NULL || MaxLength<1)
		return 0;
	if (Index<0 || Index>=NumCommands()) {
		pszName[0]='\0';
		return 0;
	}
	if (Index<lengthof(CommandList))
		return ::LoadString(GetAppClass().GetResourceInstance(),
							CommandList[Index].Command,pszName,MaxLength);
	Base=lengthof(CommandList);
	if (Index<Base+m_DriverList.Length())
		return StdUtil::snprintf(pszName,MaxLength,TEXT("ドライバ(%s)"),
								 m_DriverList[Index-Base]);
	Base+=m_DriverList.Length();
	if (Index<Base+m_PluginList.Length())
		return StdUtil::snprintf(pszName,MaxLength,TEXT("プラグイン(%s)"),
								 m_PluginList[Index-Base]);
	Base+=m_PluginList.Length();
	LPCTSTR pszText=m_PluginCommandList[Index-Base];
	int Length;
	TCHAR szFileName[MAX_PATH];
	for (Length=0;pszText[Length]!=':';Length++)
		szFileName[Length]=pszText[Length];
	szFileName[Length]='\0';
	return StdUtil::snprintf(pszName,MaxLength,TEXT("%s(%s)"),
							 szFileName,pszText+::lstrlen(pszText)+1);
}


int CCommandList::IDToIndex(int ID) const
{
	int Base;

	for (int i=0;i<lengthof(CommandList);i++) {
		if (CommandList[i].Command==ID)
			return i;
	}
	Base=lengthof(CommandList);
	if (ID>=CM_DRIVER_FIRST && ID<CM_DRIVER_FIRST+m_DriverList.Length())
		return Base+ID-CM_DRIVER_FIRST;
	Base+=m_DriverList.Length();
	if (ID>=CM_PLUGIN_FIRST && ID<CM_PLUGIN_FIRST+m_PluginList.Length())
		return Base+ID-CM_PLUGIN_FIRST;
	Base+=m_PluginList.Length();
	if (ID>=CM_PLUGINCOMMAND_FIRST && ID<CM_PLUGINCOMMAND_LAST+m_PluginCommandList.Length())
		return Base+ID-CM_PLUGINCOMMAND_FIRST;
	return -1;
}


int CCommandList::ParseText(LPCTSTR pszText) const
{
	int i;

	if (pszText==NULL)
		return 0;
	for (i=0;i<lengthof(CommandList);i++) {
		if (::lstrcmpi(CommandList[i].pszText,pszText)==0)
			return CommandList[i].Command;
	}
	for (i=0;i<m_DriverList.Length();i++) {
		if (::lstrcmpi(m_DriverList[i],pszText)==0)
			return CM_DRIVER_FIRST+i;
	}
	for (i=0;i<m_PluginList.Length();i++) {
		if (::lstrcmpi(m_PluginList[i],pszText)==0)
			return CM_PLUGIN_FIRST+i;
	}
	for (i=0;i<m_PluginCommandList.Length();i++) {
		if (::lstrcmpi(m_PluginCommandList[i],pszText)==0)
			return CM_PLUGINCOMMAND_FIRST+i;
	}
	return 0;
}

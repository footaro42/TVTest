#include "stdafx.h"
#include "TVTest.h"
#include "ChannelList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CChannelInfo::CChannelInfo(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName)
{
	m_Space=Space;
	m_Channel=Channel;
	m_ChannelIndex=Index;
	m_ChannelNo=No;
	m_Service=Service;
	::lstrcpyn(m_szName,pszName,MAX_CHANNEL_NAME);
	m_NetworkID=0;
	m_ServiceID=0;
}


CChannelInfo::CChannelInfo(const CChannelInfo &Info)
{
	*this=Info;
}


CChannelInfo &CChannelInfo::operator=(const CChannelInfo &Info)
{
	if (&Info==this)
		return *this;
	m_Space=Info.m_Space;
	m_Channel=Info.m_Channel;
	m_ChannelIndex=Info.m_ChannelIndex;
	m_ChannelNo=Info.m_ChannelNo;
	m_Service=Info.m_Service;
	::lstrcpy(m_szName,Info.m_szName);
	m_NetworkID=Info.m_NetworkID;
	m_ServiceID=Info.m_ServiceID;
	return *this;
}


bool CChannelInfo::SetName(LPCTSTR pszName)
{
	::lstrcpyn(m_szName,pszName,MAX_CHANNEL_NAME);
	return true;
}


bool CChannelInfo::SetNetworkID(WORD NetworkID)
{
	m_NetworkID=NetworkID;
	return true;
}


bool CChannelInfo::SetServiceID(WORD ServiceID)
{
	m_ServiceID=ServiceID;
	return true;
}




CChannelList::CChannelList()
{
	m_NumChannels=0;
	m_ppList=NULL;
	m_ListLength=0;
}


CChannelList::CChannelList(const CChannelList &List)
{
	m_NumChannels=List.m_NumChannels;
	m_ListLength=m_NumChannels;
	if (m_NumChannels>0) {
		m_ppList=static_cast<CChannelInfo**>(malloc(m_ListLength*sizeof(CChannelInfo*)));
		for (int i=0;i<m_NumChannels;i++)
			m_ppList[i]=new CChannelInfo(*List.m_ppList[i]);
	} else
		m_ppList=NULL;
}


CChannelList::~CChannelList()
{
	Clear();
}


CChannelList &CChannelList::operator=(const CChannelList &List)
{
	if (&List==this)
		return *this;
	Clear();
	if (List.m_NumChannels>0) {
		m_NumChannels=List.m_NumChannels;
		m_ListLength=m_NumChannels;
		m_ppList=static_cast<CChannelInfo**>(malloc(m_ListLength*sizeof(CChannelInfo*)));
		for (int i=0;i<m_NumChannels;i++)
			m_ppList[i]=new CChannelInfo(*List.m_ppList[i]);
	}
	return *this;
}


bool CChannelList::AddChannel(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName)
{
	CChannelInfo Info(Space,Channel,Index,No,Service,pszName);

	return AddChannel(Info);
}


bool CChannelList::AddChannel(const CChannelInfo &Info)
{
	if (m_NumChannels==m_ListLength) {
		if (m_ListLength==0)
			m_ListLength=16;
		else
			m_ListLength*=2;
		m_ppList=static_cast<CChannelInfo**>(realloc(m_ppList,
										m_ListLength*sizeof(CChannelInfo*)));
	}
	m_ppList[m_NumChannels++]=new CChannelInfo(Info);
	return true;
}


CChannelInfo *CChannelList::GetChannelInfo(int Index)
{
	if (Index<0 || Index>=m_NumChannels) {
		TRACE(TEXT("CChannelList::GetChannelInfo Out of range %d\n"),Index);
		return NULL;
	}
	return m_ppList[Index];
}


const CChannelInfo *CChannelList::GetChannelInfo(int Index) const
{
	if (Index<0 || Index>=m_NumChannels) {
		TRACE(TEXT("CChannelList::GetChannelInfo Out of range %d\n"),Index);
		return NULL;
	}
	return m_ppList[Index];
}


int CChannelList::GetSpace(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return -1;
	return m_ppList[Index]->GetSpace();
}


int CChannelList::GetChannel(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return -1;
	return m_ppList[Index]->GetChannel();
}


int CChannelList::GetChannelIndex(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return -1;
	return m_ppList[Index]->GetChannelIndex();
}


int CChannelList::GetChannelNo(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return -1;
	return m_ppList[Index]->GetChannelNo();
}


LPCTSTR CChannelList::GetName(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		//return NULL;
		return TEXT("");
	return m_ppList[Index]->GetName();
}


int CChannelList::GetService(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return -1;
	return m_ppList[Index]->GetService();
}


bool CChannelList::DeleteChannel(int Index)
{
	if (Index<0 || Index>=m_NumChannels)
		return false;
	delete m_ppList[Index];
	m_NumChannels--;
	if (Index<m_NumChannels)
		::MoveMemory(&m_ppList[Index],&m_ppList[Index+1],
					 (m_NumChannels-Index)*sizeof(CChannelInfo*));
	return true;
}


void CChannelList::Clear()
{
	if (m_NumChannels>0) {
		for (int i=m_NumChannels-1;i>=0;i--)
			delete m_ppList[i];
		free(m_ppList);
		m_NumChannels=0;
		m_ppList=NULL;
		m_ListLength=0;
	}
}


int CChannelList::Find(const CChannelInfo *pInfo) const
{
	int i;

	for (i=m_NumChannels-1;i>=0;i--) {
		if (m_ppList[i]==pInfo)
			break;
	}
	return i;
}


int CChannelList::Find(int Space,int ChannelIndex,int Service) const
{
	for (int i=0;i<m_NumChannels;i++) {
		const CChannelInfo *pChInfo=m_ppList[i];

		if ((Space<0 || pChInfo->GetSpace()==Space)
				&& (ChannelIndex<0 || pChInfo->GetChannelIndex()==ChannelIndex)
				&& (Service<0 || pChInfo->GetService()==Service))
			return i;
	}
	return -1;
}


int CChannelList::FindChannel(int Channel) const
{
	for (int i=0;i<m_NumChannels;i++) {
		if (GetChannel(i)==Channel)
			return i;
	}
	return -1;
}


int CChannelList::FindChannelNo(int No) const
{
	for (int i=0;i<m_NumChannels;i++) {
		if (GetChannelNo(i)==No)
			return i;
	}
	return -1;
}


int CChannelList::FindServiceID(WORD ServiceID) const
{
	for (int i=0;i<m_NumChannels;i++) {
		if (m_ppList[i]->GetServiceID()==ServiceID)
			return i;
	}
	return -1;
}


int CChannelList::GetNextChannelNo(int ChannelNo,bool fWrap) const
{
	int i;
	int Channel,Min,No;

	Channel=1000;
	Min=1000;
	for (i=0;i<m_NumChannels;i++) {
		No=GetChannelNo(i);
		if (No>ChannelNo && No<Channel)
			Channel=No;
		if (No<Min)
			Min=No;
	}
	if (Channel==1000) {
		if (fWrap)
			return Min;
		return 0;
	}
	return Channel;
}


int CChannelList::GetPrevChannelNo(int ChannelNo,bool fWrap) const
{
	int i;
	int Channel,Max,No;

	Channel=0;
	Max=0;
	for (i=0;i<m_NumChannels;i++) {
		No=GetChannelNo(i);
		if (No<ChannelNo && No>Channel)
			Channel=No;
		if (No>Max)
			Max=No;
	}
	if (Channel==0) {
		if (fWrap)
			return Max;
		return 0;
	}
	return Channel;
}


int CChannelList::GetMaxChannelNo() const
{
	int Max,No;
	int i;

	Max=0;
	for (i=0;i<m_NumChannels;i++) {
		No=GetChannelNo(i);
		if (No>Max)
			Max=No;
	}
	return Max;
}


void CChannelList::SortSub(SortType Type,bool fDescending,int First,int Last,CChannelInfo **ppTemp)
{
	int Center,i,j,k;

	if (First>=Last)
		return;
	Center=(First+Last)/2;
	SortSub(Type,fDescending,First,Center,ppTemp);
	SortSub(Type,fDescending,Center+1,Last,ppTemp);
	for (i=First;i<=Center;i++)
		ppTemp[i]=m_ppList[i];
	for (j=Last;i<=Last;i++,j--)
		ppTemp[i]=m_ppList[j];
	i=First;
	j=Last;
	for (k=First;k<=Last;k++) {
		CChannelInfo *pCh1=ppTemp[i],*pCh2=ppTemp[j];
		int Cmp;

		switch (Type) {
		case SORT_SPACE:
			Cmp=pCh1->GetSpace()-pCh2->GetSpace();
			break;
		case SORT_CHANNEL:
			Cmp=pCh1->GetChannel()-pCh2->GetChannel();
			break;
		case SORT_CHANNELINDEX:
			Cmp=pCh1->GetChannelIndex()-pCh2->GetChannelIndex();
			break;
		case SORT_CHANNELNO:
			Cmp=pCh1->GetChannelNo()-pCh2->GetChannelNo();
			break;
		case SORT_SERVICE:
			Cmp=pCh1->GetService()-pCh2->GetService();
			break;
		case SORT_NAME:
			Cmp=::lstrcmpi(pCh1->GetName(),pCh2->GetName());
			if (Cmp==0)
				Cmp=::lstrcmp(pCh1->GetName(),pCh2->GetName());
			break;
		case SORT_NETWORKID:
			Cmp=pCh1->GetNetworkID()-pCh2->GetNetworkID();
			break;
		case SORT_SERVICEID:
			Cmp=pCh1->GetServiceID()-pCh2->GetServiceID();
			break;
		}
		if (fDescending)
			Cmp=-Cmp;
		if (Cmp<=0)
			m_ppList[k]=ppTemp[i++];
		else
			m_ppList[k]=ppTemp[j--];
	}
}


void CChannelList::Sort(SortType Type,bool fDescending)
{
	if (m_NumChannels>1) {
		CChannelInfo **ppTemp;

		ppTemp=new CChannelInfo*[m_NumChannels];
		SortSub(Type,fDescending,0,m_NumChannels-1,ppTemp);
		delete [] ppTemp;
	}
}


bool CChannelList::SetServiceID(int Space,int ChannelIndex,int Service,WORD ServiceID)
{
	int i;

	for (i=0;i<m_NumChannels;i++) {
		if (m_ppList[i]->GetSpace()==Space
				&& m_ppList[i]->GetChannelIndex()==ChannelIndex
				&& m_ppList[i]->GetService()==Service)
			m_ppList[i]->SetServiceID(ServiceID);
	}
	return true;
}




CTuningSpaceList::CTuningSpaceList()
{
	m_ppChannelList=NULL;
	m_NumSpaces=0;
}


CTuningSpaceList::CTuningSpaceList(const CTuningSpaceList &List)
	: m_AllChannelList(List.m_AllChannelList)
{
	m_ppChannelList=NULL;
	m_NumSpaces=0;
	*this=List;
}


CTuningSpaceList::~CTuningSpaceList()
{
	Clear();
}


CTuningSpaceList &CTuningSpaceList::operator=(const CTuningSpaceList &List)
{
	if (&List==this)
		return *this;
	Clear();
	if (List.m_NumSpaces>0) {
		m_ppChannelList=static_cast<CChannelList**>(malloc(List.m_NumSpaces*sizeof(CChannelList*)));
		for (int i=0;i<List.m_NumSpaces;i++)
			m_ppChannelList[i]=new CChannelList(*List.m_ppChannelList[i]);
		m_NumSpaces=List.m_NumSpaces;
	}
	m_AllChannelList=List.m_AllChannelList;
	return *this;
}


CChannelList *CTuningSpaceList::GetChannelList(int Space)
{
	if (Space<0 || Space>=m_NumSpaces) {
		TRACE(TEXT("CTuningSpaceList::GetChannelList() : Out of range %d\n"),Space);
		return NULL;
	}
	return m_ppChannelList[Space];
}


const CChannelList *CTuningSpaceList::GetChannelList(int Space) const
{
	if (Space<0 || Space>=m_NumSpaces) {
		TRACE(TEXT("CTuningSpaceList::GetChannelList() const : Out of range %d\n"),Space);
		return NULL;
	}
	return m_ppChannelList[Space];
}


bool CTuningSpaceList::MakeTuningSpaceList(const CChannelList *pList,int NumSpaces)
{
	int i;
	int Space;

	for (i=0;i<pList->NumChannels();i++) {
		Space=pList->GetSpace(i);
		if (Space+1>NumSpaces)
			NumSpaces=Space+1;
	}
	if (NumSpaces<1)
		return false;
	if (!Reserve(NumSpaces))
		return false;
	for (i=0;i<pList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

		m_ppChannelList[pChInfo->GetSpace()]->AddChannel(*pChInfo);
	}
	return true;
}


bool CTuningSpaceList::Create(const CChannelList *pList,int NumSpaces)
{
	Clear();
	if (!MakeTuningSpaceList(pList,NumSpaces))
		return false;
	m_AllChannelList=*pList;
	return true;
}


bool CTuningSpaceList::Reserve(int NumSpaces)
{
	int i;

	if (NumSpaces<0)
		return false;
	if (NumSpaces==m_NumSpaces)
		return true;
	if (NumSpaces==0) {
		Clear();
		return true;
	}
	CChannelList **ppNewList=static_cast<CChannelList**>(malloc(NumSpaces*sizeof(CChannelList*)));
	if (ppNewList==NULL)
		return false;
	if (NumSpaces<m_NumSpaces) {
		for (i=NumSpaces;i<m_NumSpaces;i++)
			delete m_ppChannelList[i];
	} else {
		for (i=m_NumSpaces;i<NumSpaces;i++)
			ppNewList[i]=new CChannelList;
	}
	for (i=0;i<min(NumSpaces,m_NumSpaces);i++)
		ppNewList[i]=m_ppChannelList[i];
	free(m_ppChannelList);
	m_ppChannelList=ppNewList;
	m_NumSpaces=NumSpaces;
	return true;
}


void CTuningSpaceList::Clear()
{
	if (m_ppChannelList!=NULL) {
		int i;

		for (i=m_NumSpaces-1;i>=0;i--)
			delete m_ppChannelList[i];
		free(m_ppChannelList);
		m_ppChannelList=NULL;
		m_NumSpaces=0;
	}
	m_AllChannelList.Clear();
}


bool CTuningSpaceList::MakeAllChannelList()
{
	m_AllChannelList.Clear();
	for (int i=0;i<m_NumSpaces;i++) {
		CChannelList *pList=m_ppChannelList[i];

		for (int j=0;j<pList->NumChannels();j++) {
			m_AllChannelList.AddChannel(*pList->GetChannelInfo(j));
		}
	}
	return true;
}


bool CTuningSpaceList::SaveToFile(LPCTSTR pszFileName) const
{
	HANDLE hFile;
	DWORD Length,Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	static const char szComment[]=
		"; " APP_NAME_A " チャンネル設定ファイル\r\n"
		"; 名称,チューニング空間,チャンネル,リモコン番号(オプション),サービス(オプション),サービスID(オプション)\r\n";
	::WriteFile(hFile,szComment,sizeof(szComment)-1,&Write,NULL);
	for (int i=0;i<m_NumSpaces;i++) {
		const CChannelList *pChannelList=m_ppChannelList[i];

		for (int j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(j);
			char szName[MAX_CHANNEL_NAME*2],szText[MAX_CHANNEL_NAME*2+32];

#ifdef UNICODE
			::WideCharToMultiByte(CP_ACP,0,pChInfo->GetName(),-1,
											szName,lengthof(szName),NULL,NULL);
#else
			::lstrcpy(szName,pChInfo->GetName());
#endif
			if (pChInfo->GetServiceID()!=0) {
				Length=::wsprintfA(szText,"%s,%d,%d,%d,%d,%d\r\n",
					szName,
					pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
					pChInfo->GetChannelNo(),pChInfo->GetService(),
					pChInfo->GetServiceID());
			} else {
				Length=::wsprintfA(szText,"%s,%d,%d,%d,%d\r\n",
					szName,
					pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
					pChInfo->GetChannelNo(),pChInfo->GetService());
			}
			if (!::WriteFile(hFile,szText,Length,&Write,NULL) || Write!=Length) {
				::CloseHandle(hFile);
				return false;
			}
		}
	}
	::CloseHandle(hFile);
	return true;
}


static void SkipSpaces(char **ppText)
{
	char *p=*ppText;

	while (*p==' ' || *p=='\t')
		p++;
	*ppText=p;
}

bool inline IsDigit(char c)
{
	return c>='0' && c<='9';
}

bool CTuningSpaceList::LoadFromFile(LPCTSTR pszFileName)
{
	HANDLE hFile;
	DWORD FileSize,Read;
	char *pszFile,*p;

	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
					   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	FileSize=::GetFileSize(hFile,NULL);
	if (FileSize==0 || FileSize==INVALID_FILE_SIZE) {
		::CloseHandle(hFile);
		return false;
	}
	pszFile=new char[FileSize+1];
	if (!::ReadFile(hFile,pszFile,FileSize,&Read,NULL) || Read!=FileSize) {
		delete [] pszFile;
		::CloseHandle(hFile);
		return false;
	}
	pszFile[FileSize]='\0';
	::CloseHandle(hFile);
	m_AllChannelList.Clear();
	p=pszFile;
	do {
		TCHAR szName[MAX_CHANNEL_NAME];
		int Space,Channel,ControlKey,Service,ServiceID;

		while (*p=='\r' || *p=='\n' || *p==' ' || *p=='\t')
			p++;
		if (*p==';') {	// コメント
			while (*p!='\r' && *p!='\n' && *p!='\0')
				p++;
			continue;
		}
		if (*p=='\0')
			break;
		// チャンネル名
		char *pszName=p;
		while (*p!=',' && *p!='\0') {
			if (::IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!='\0')
				p++;
			p++;
		}
		if (*p!=',')
			goto Next;
		*p++='\0';
#ifdef UNICODE
		::MultiByteToWideChar(CP_ACP,0,pszName,-1,szName,MAX_CHANNEL_NAME);
#else
		::lstrcpyn(szName,pszName,MAX_CHANNEL_NAME);
#endif
		SkipSpaces(&p);
		// チューニング空間
		if (!IsDigit(*p))
			goto Next;
		Space=0;
		do {
			Space=Space*10+(*p-'0');
			p++;
		} while (IsDigit(*p));
		SkipSpaces(&p);
		if (*p!=',')
			goto Next;
		p++;
		SkipSpaces(&p);
		// チャンネル
		if (!IsDigit(*p))
			goto Next;
		Channel=0;
		do {
			Channel=Channel*10+(*p-'0');
			p++;
		} while (IsDigit(*p));
		SkipSpaces(&p);
		ControlKey=0;
		Service=0;
		ServiceID=0;
		if (*p==',') {
			p++;
			SkipSpaces(&p);
			// リモコン番号(オプション)
			while (IsDigit(*p)) {
				ControlKey=ControlKey*10+(*p-'0');
				p++;
			}
			SkipSpaces(&p);
			// サービス(オプション)
			if (*p==',') {
				p++;
				SkipSpaces(&p);
				while (IsDigit(*p)) {
					Service=Service*10+(*p-'0');
					p++;
				}
				SkipSpaces(&p);
				// サービスID(オプション)
				if (*p==',') {
					p++;
					SkipSpaces(&p);
					while (IsDigit(*p)) {
						ServiceID=ServiceID*10+(*p-'0');
						p++;
					}
				}
			}
		}
		{
			CChannelInfo ChInfo(Space,0,Channel,ControlKey,Service,szName);
			if (ServiceID!=0)
				ChInfo.SetServiceID(ServiceID);
			m_AllChannelList.AddChannel(ChInfo);
		}
	Next:
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	} while (*p!='\0');
	delete [] pszFile;
	return MakeTuningSpaceList(&m_AllChannelList);
}


bool CTuningSpaceList::SetServiceID(int Space,int ChannelIndex,int Service,WORD ServiceID)
{
	for (int i=0;i<m_NumSpaces;i++)
		m_ppChannelList[i]->SetServiceID(Space,ChannelIndex,Service,ServiceID);
	m_AllChannelList.SetServiceID(Space,ChannelIndex,Service,ServiceID);
	return true;
}

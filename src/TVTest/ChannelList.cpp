#include "stdafx.h"
#include <shlwapi.h>
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
	m_TransportStreamID=0;
	m_ServiceID=0;
	m_fEnabled=true;
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
	m_TransportStreamID=Info.m_TransportStreamID;
	m_ServiceID=Info.m_ServiceID;
	m_fEnabled=Info.m_fEnabled;
	return *this;
}


bool CChannelInfo::SetSpace(int Space)
{
	m_Space=Space;
	return true;
}


bool CChannelInfo::SetChannel(int Channel)
{
	m_Channel=Channel;
	return true;
}


bool CChannelInfo::SetChannelNo(int ChannelNo)
{
	m_ChannelNo=ChannelNo;
	return true;
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


bool CChannelInfo::SetTransportStreamID(WORD TransportStreamID)
{
	m_TransportStreamID=TransportStreamID;
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


bool CChannelList::IsEnabled(int Index) const
{
	if (Index<0 || Index>=m_NumChannels)
		return false;
	return m_ppList[Index]->IsEnabled();
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


int CChannelList::Find(int Space,int ChannelIndex,int ServiceID) const
{
	for (int i=0;i<m_NumChannels;i++) {
		const CChannelInfo *pChInfo=m_ppList[i];

		if ((Space<0 || pChInfo->GetSpace()==Space)
				&& (ChannelIndex<0 || pChInfo->GetChannelIndex()==ChannelIndex)
				&& (ServiceID<=0 || pChInfo->GetServiceID()==ServiceID))
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
		const CChannelInfo *pChInfo=m_ppList[i];

		if (pChInfo->IsEnabled()) {
			No=pChInfo->GetChannelNo();
			if (No>ChannelNo && No<Channel)
				Channel=No;
			if (No<Min)
				Min=No;
		}
	}
	if (Channel==1000) {
		if (Min==1000)
			return 0;
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
		const CChannelInfo *pChInfo=m_ppList[i];

		if (pChInfo->IsEnabled()) {
			No=pChInfo->GetChannelNo();
			if (No<ChannelNo && No>Channel)
				Channel=No;
			if (No>Max)
				Max=No;
		}
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


bool CChannelList::UpdateStreamInfo(int Space,int ChannelIndex,int Service,
						WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	int i;

	for (i=0;i<m_NumChannels;i++) {
		CChannelInfo *pChannelInfo=m_ppList[i];

		if (pChannelInfo->GetSpace()==Space
				&& pChannelInfo->GetChannelIndex()==ChannelIndex
				&& pChannelInfo->GetService()==Service) {
			if (NetworkID!=0 && pChannelInfo->GetNetworkID()==0)
				pChannelInfo->SetNetworkID(NetworkID);
			if (TransportStreamID!=0 && pChannelInfo->GetTransportStreamID()==0)
				pChannelInfo->SetTransportStreamID(TransportStreamID);
			if (ServiceID!=0 && pChannelInfo->GetServiceID()==0)
				pChannelInfo->SetServiceID(ServiceID);
		}
	}
	return true;
}


bool CChannelList::HasRemoteControlKeyID() const
{
	for (int i=0;i<m_NumChannels;i++) {
		if (m_ppList[i]->GetChannelNo()!=0)
			return true;
	}
	return false;
}




CTuningSpaceInfo::CTuningSpaceInfo()
	: m_pChannelList(NULL)
	, m_pszName(NULL)
	, m_Space(SPACE_UNKNOWN)
{
}


CTuningSpaceInfo::CTuningSpaceInfo(const CTuningSpaceInfo &Info)
	: m_pChannelList(NULL)
	, m_pszName(NULL)
	, m_Space(SPACE_UNKNOWN)
{
	Create(Info.m_pChannelList,Info.m_pszName);
}


CTuningSpaceInfo::~CTuningSpaceInfo()
{
	delete m_pChannelList;
	delete [] m_pszName;
}


CTuningSpaceInfo &CTuningSpaceInfo::operator=(const CTuningSpaceInfo &Info)
{
	if (&Info!=this)
		Create(Info.m_pChannelList,Info.m_pszName);
	return *this;
}


bool CTuningSpaceInfo::Create(const CChannelList *pList,LPCTSTR pszName)
{
	delete m_pChannelList;
	if (pList!=NULL)
		m_pChannelList=new CChannelList(*pList);
	else
		m_pChannelList=new CChannelList;
	SetName(pszName);
	return true;
}


bool CTuningSpaceInfo::SetName(LPCTSTR pszName)
{
	if (!ReplaceString(&m_pszName,pszName))
		return false;
	// チューニング空間の種類を判定する
	// BonDriverから取得できないので苦肉の策
	m_Space=SPACE_UNKNOWN;
	if (pszName!=NULL) {
		if (::StrStr(pszName,TEXT("地デジ"))!=NULL
				|| ::StrStr(pszName,TEXT("地上"))!=NULL
				|| ::StrStrI(pszName,TEXT("VHF"))!=NULL
				|| ::StrStrI(pszName,TEXT("UHF"))!=NULL
				|| ::StrStrI(pszName,TEXT("CATV"))!=NULL) {
			m_Space=SPACE_TERRESTRIAL;
		} else if (::StrStrI(pszName,TEXT("BS"))!=NULL) {
			m_Space=SPACE_BS;
		} else if (::StrStrI(pszName,TEXT("CS"))!=NULL) {
			m_Space=SPACE_110CS;
		}
	}
	return true;
}




CTuningSpaceList::CTuningSpaceList()
{
}


CTuningSpaceList::CTuningSpaceList(const CTuningSpaceList &List)
	: m_AllChannelList(List.m_AllChannelList)
{
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
	if (List.NumSpaces()>0) {
		m_TuningSpaceList.Reserve(List.NumSpaces());
		for (int i=0;i<List.NumSpaces();i++)
			m_TuningSpaceList.Add(new CTuningSpaceInfo(*List.m_TuningSpaceList[i]));
	}
	m_AllChannelList=List.m_AllChannelList;
	return *this;
}


CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space)
{
	if (Space<0 || Space>=NumSpaces()) {
		TRACE(TEXT("CTuningSpaceList::GetTuningSpaceInfo() : Out of range %d\n"),Space);
		return NULL;
	}
	return m_TuningSpaceList[Space];
}


const CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space) const
{
	if (Space<0 || Space>=NumSpaces()) {
		TRACE(TEXT("CTuningSpaceList::GetTuningSpaceInfo() const : Out of range %d\n"),Space);
		return NULL;
	}
	return m_TuningSpaceList[Space];
}


CChannelList *CTuningSpaceList::GetChannelList(int Space)
{
	if (Space<0 || Space>=NumSpaces()) {
		TRACE(TEXT("CTuningSpaceList::GetChannelList() : Out of range %d\n"),Space);
		return NULL;
	}
	return m_TuningSpaceList[Space]->GetChannelList();
}


const CChannelList *CTuningSpaceList::GetChannelList(int Space) const
{
	if (Space<0 || Space>=NumSpaces()) {
		TRACE(TEXT("CTuningSpaceList::GetChannelList() const : Out of range %d\n"),Space);
		return NULL;
	}
	return m_TuningSpaceList[Space]->GetChannelList();
}


LPCTSTR CTuningSpaceList::GetTuningSpaceName(int Space) const
{
	if (Space<0 || Space>=NumSpaces()) {
		return NULL;
	}
	return m_TuningSpaceList[Space]->GetName();
}


CTuningSpaceInfo::TuningSpaceType CTuningSpaceList::GetTuningSpaceType(int Space) const
{
	if (Space<0 || Space>=NumSpaces()) {
		return CTuningSpaceInfo::SPACE_ERROR;
	}
	return m_TuningSpaceList[Space]->GetType();
}


bool CTuningSpaceList::MakeTuningSpaceList(const CChannelList *pList,int Spaces)
{
	int i;
	int Space;

	for (i=0;i<pList->NumChannels();i++) {
		Space=pList->GetSpace(i);
		if (Space+1>Spaces)
			Spaces=Space+1;
	}
	if (Spaces<1)
		return false;
	if (!Reserve(Spaces))
		return false;
	for (i=0;i<pList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

		m_TuningSpaceList[pChInfo->GetSpace()]->GetChannelList()->AddChannel(*pChInfo);
	}
	return true;
}


bool CTuningSpaceList::Create(const CChannelList *pList,int Spaces)
{
	Clear();
	if (!MakeTuningSpaceList(pList,Spaces))
		return false;
	m_AllChannelList=*pList;
	return true;
}


bool CTuningSpaceList::Reserve(int Spaces)
{
	int i;

	if (Spaces<0)
		return false;
	if (Spaces==NumSpaces())
		return true;
	if (Spaces==0) {
		Clear();
		return true;
	}
	if (Spaces<NumSpaces()) {
		for (i=NumSpaces()-1;i>=Spaces;i--)
			m_TuningSpaceList.Delete(i);
	} else {
		for (i=NumSpaces();i<Spaces;i++) {
			CTuningSpaceInfo *pInfo=new CTuningSpaceInfo;

			pInfo->Create();
			m_TuningSpaceList.Add(pInfo);
		}
	}
	return true;
}


void CTuningSpaceList::Clear()
{
	m_TuningSpaceList.DeleteAll();
	m_TuningSpaceList.Clear();
	m_AllChannelList.Clear();
}


bool CTuningSpaceList::MakeAllChannelList()
{
	m_AllChannelList.Clear();
	for (int i=0;i<NumSpaces();i++) {
		CChannelList *pList=m_TuningSpaceList[i]->GetChannelList();

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
		"; 名称,チューニング空間,チャンネル,リモコン番号,サービス,サービスID,ネットワークID,TSID,状態\r\n";
	::WriteFile(hFile,szComment,sizeof(szComment)-1,&Write,NULL);
	for (int i=0;i<NumSpaces();i++) {
		const CChannelList *pChannelList=m_TuningSpaceList[i]->GetChannelList();

		for (int j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(j);
			char szName[MAX_CHANNEL_NAME*2],szText[MAX_CHANNEL_NAME*2+64];

#ifdef UNICODE
			::WideCharToMultiByte(CP_ACP,0,pChInfo->GetName(),-1,
											szName,lengthof(szName),NULL,NULL);
#else
			::lstrcpy(szName,pChInfo->GetName());
#endif
			Length=::wsprintfA(szText,"%s,%d,%d,%d,%d,",
				szName,
				pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
				pChInfo->GetChannelNo(),pChInfo->GetService());
			if (pChInfo->GetServiceID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetServiceID());
			szText[Length++]=',';
			if (pChInfo->GetNetworkID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetNetworkID());
			szText[Length++]=',';
			if (pChInfo->GetTransportStreamID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetTransportStreamID());
			szText[Length++]=',';
			szText[Length++]=pChInfo->IsEnabled()?'1':'0';
			szText[Length++]='\r';
			szText[Length++]='\n';
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
		int Space,Channel,ControlKey,Service,ServiceID,NetworkID,TransportStreamID;
		bool fEnabled;

		while (*p=='\r' || *p=='\n' || *p==' ' || *p=='\t')
			p++;
		if (*p==';')	// コメント
			goto Next;
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
		NetworkID=0;
		TransportStreamID=0;
		fEnabled=true;
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
					SkipSpaces(&p);
					// ネットワークID(オプション)
					if (*p==',') {
						p++;
						SkipSpaces(&p);
						while (IsDigit(*p)) {
							NetworkID=ServiceID*10+(*p-'0');
							p++;
						}
						SkipSpaces(&p);
						// トランスポートストリームID(オプション)
						if (*p==',') {
							p++;
							SkipSpaces(&p);
							while (IsDigit(*p)) {
								TransportStreamID=TransportStreamID*10+(*p-'0');
								p++;
							}
							SkipSpaces(&p);
							// 有効状態(オプション)
							if (*p==',') {
								p++;
								SkipSpaces(&p);
								if (IsDigit(*p)) {
									int Value=0;
									while (IsDigit(*p)) {
										Value=Value*10+(*p-'0');
										p++;
									}
									fEnabled=(Value&1)!=0;
								}
							}
						}
					}
				}
			}
		}
		{
			CChannelInfo ChInfo(Space,0,Channel,ControlKey,Service,szName);
			if (ServiceID!=0)
				ChInfo.SetServiceID(ServiceID);
			if (NetworkID!=0)
				ChInfo.SetNetworkID(NetworkID);
			if (TransportStreamID!=0)
				ChInfo.SetTransportStreamID(TransportStreamID);
			if (!fEnabled)
				ChInfo.Enable(false);
			m_AllChannelList.AddChannel(ChInfo);
		}
	Next:
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	} while (*p!='\0');
	delete [] pszFile;
	return MakeTuningSpaceList(&m_AllChannelList);
}


bool CTuningSpaceList::UpdateStreamInfo(int Space,int ChannelIndex,int Service,
						WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	if (Space<0 || Space>=NumSpaces())
		return false;
	m_TuningSpaceList[Space]->GetChannelList()->UpdateStreamInfo(
			Space,ChannelIndex,Service,NetworkID,TransportStreamID,ServiceID);
	m_AllChannelList.UpdateStreamInfo(Space,ChannelIndex,Service,
									  NetworkID,TransportStreamID,ServiceID);
	return true;
}

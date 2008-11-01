#include "stdafx.h"
#include <new>
#include "PointerArray.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define INIT_LENGTH 16


template<typename type> inline void Swap(type &a,type &b)
{
	type tmp(a);
	a=b;
	b=tmp;
}




CPointerArray::CPointerArray()
	: m_ppList(NULL)
	, m_Length(0)
	, m_ListLength(0)
{
}


CPointerArray::CPointerArray(const CPointerArray &Array)
{
	m_ppList=NULL;
	m_Length=0;
	m_ListLength=0;
	if (Array.m_Length>0) {
		m_ppList=static_cast<void**>(malloc(Array.m_Length*sizeof(void*)));
		if (m_ppList==NULL)
			throw std::bad_alloc();
		memcpy(m_ppList,Array.m_ppList,Array.m_Length*sizeof(void*));
		m_Length=m_ListLength=Array.m_Length;
	}
}


CPointerArray::~CPointerArray()
{
	Clear();
}


CPointerArray &CPointerArray::operator=(const CPointerArray &Array)
{
	if (&Array==this)
		return *this;
	if (Array.m_Length>0) {
		if (Array.m_Length>m_ListLength) {
			if (!Reserve(Array.m_Length))
				throw std::bad_alloc();
		}
		memcpy(m_ppList,Array.m_ppList,Array.m_Length*sizeof(void*));
		m_Length=Array.m_Length;
	} else {
		Clear();
	}
	return *this;
}


void CPointerArray::Clear()
{
	if (m_ppList!=NULL) {
		free(m_ppList);
		m_ppList=NULL;
		m_Length=0;
		m_ListLength=0;
	}
}


bool CPointerArray::Reserve(int Length)
{
	if (Length<m_Length)
		return false;
	if (Length!=m_ListLength) {
		if (Length==0) {
			Clear();
		} else {
			void **ppNewList;

			ppNewList=static_cast<void**>(realloc(m_ppList,Length*sizeof(void*)));
			if (ppNewList==NULL)
				return false;
			m_ppList=ppNewList;
			m_ListLength=Length;
		}
	}
	return true;
}


bool CPointerArray::Add(void *pItem)
{
	if (m_Length==m_ListLength) {
		if (!Reserve(m_ListLength==0?INIT_LENGTH:m_ListLength*2))
			return false;
	}
	m_ppList[m_Length++]=pItem;
	return true;
}


bool CPointerArray::Insert(int Index,void *pItem)
{
	if (Index<0 || Index>m_Length)
		return false;
	if (m_Length==m_ListLength) {
		if (!Reserve(m_ListLength==0?INIT_LENGTH:m_ListLength*2))
			return false;
	}
	if (Index<m_Length)
		memmove(&m_ppList[Index+1],&m_ppList[Index],
				(m_Length-Index)*sizeof(void*));
	m_ppList[Index]=pItem;
	m_Length++;
	return true;
}


void *CPointerArray::Remove(int Index)
{
	void *pItem;

	if (Index<0 || Index>=m_Length)
		return NULL;
	pItem=m_ppList[Index];
	m_Length--;
	if (Index<m_Length)
		memmove(&m_ppList[Index],&m_ppList[Index+1],(m_Length-Index)*sizeof(void*));
	return pItem;
}


void *CPointerArray::Get(int Index)
{
	if (Index<0 || Index>=m_Length)
		return NULL;
	return m_ppList[Index];
}


const void *CPointerArray::Get(int Index) const
{
	if (Index<0 || Index>=m_Length)
		return NULL;
	return m_ppList[Index];
}


bool CPointerArray::Set(int Index,void *pItem)
{
	if (Index<0 || Index>=m_Length)
		return false;
	m_ppList[Index]=pItem;
	return true;
}


bool CPointerArray::Move(int From,int To)
{
	void *pItem;

	if (From<0 || From>=m_Length || To<0 || To>=m_Length)
		return false;
	if (From==To)
		return true;
	pItem=m_ppList[From];
	if (From<To)
		memmove(&m_ppList[From],&m_ppList[From+1],(To-From)*sizeof(void*));
	else
		memmove(&m_ppList[To+1],&m_ppList[To],(From-To)*sizeof(void*));
	m_ppList[To]=pItem;
	return true;
}


int CPointerArray::Find(const void *pItem) const
{
	int i;

	for (i=0;i<m_Length;i++)
		if (m_ppList[i]==pItem)
			return i;
	return -1;
}


static void SortSub(void **ppHead,void **ppTail,
					CPointerArray::CompareFunc pCompare)
{
	void *pKey=ppHead[(ppTail-ppHead)/2];
	void **p,**q;

	p=ppHead;
	q=ppTail;
	while (p<=q) {
		while ((*pCompare)(*p,pKey)<0)
			p++;
		while ((*pCompare)(*q,pKey)>0)
			q--;
		if (p<=q) {
			Swap(*p,*q);
			p++;
			q--;
		}
	}
	if (q>ppHead)
		SortSub(ppHead,q,pCompare);
	if (p<ppTail)
		SortSub(p,ppTail,pCompare);
}

void CPointerArray::Sort(CompareFunc pCompare,bool fDescending/*=false*/)
{
	if (m_Length>1) {
		SortSub(&m_ppList[0],&m_ppList[m_Length-1],pCompare);
		if (fDescending) {
			int i,j;

			for (i=0,j=m_Length-1;i<j;i++,j--)
				Swap(m_ppList[i],m_ppList[j]);
		}
	}
}

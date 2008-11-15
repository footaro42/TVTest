#ifndef POINTER_ARRAY_H
#define POINTER_ARRAY_H


class CPointerArray {
	void **m_ppList;
	int m_Length;
	int m_ListLength;
public:
	CPointerArray();
	CPointerArray(const CPointerArray &Array);
	virtual ~CPointerArray();
	CPointerArray &operator=(const CPointerArray &Array);
	void *operator[](int Index) { return m_ppList[Index]; }
	const void *operator[](int Index) const { return m_ppList[Index]; }
	void Clear();
	int Length() const { return m_Length; }
	int Capacity() const { return m_ListLength; }
	bool Reserve(int Length);
	bool IsEmpty() const { return m_Length==0; }
	bool Add(void *pItem);
	bool Insert(int Index,void *pItem);
	void *Remove(int Index);
	void *Get(int Index);
	const void *Get(int Index) const;
	bool Set(int Index,void *pItem);
	bool Move(int From,int To);
	int Find(const void *pItem) const;
	typedef int (*CompareFunc)(const void*,const void*,void *);
	void Sort(CompareFunc pCompare,bool fDescending=false,void *pParam=NULL);
};

template <typename Type> class CPointerVector {
	CPointerArray m_Array;
public:
	Type *operator[](int Index) { return static_cast<Type*>(m_Array[Index]);}
	const Type *operator[](int Index) const { return static_cast<const Type*>(m_Array[Index]); }
	int Length() const { return m_Array.Length(); }
	int Capacity() const { return m_Array.Capacity(); }
	bool Reserve(int Length) { return m_Array.Reserve(Length); }
	bool IsEmpty() const { return m_Array.IsEmpty(); }
	void Clear() { m_Array.Clear(); }
	void DeleteAll() {
		for (int i=Length()-1;i>=0;i--)
			delete Remove(i);
	}
	bool Add(Type *pItem) { return m_Array.Add(pItem); }
	bool Insert(int Index,Type *pItem) { return m_Array.Insert(Index,pItem); }
	Type *Remove(int Index) { return static_cast<Type*>(m_Array.Remove(Index)); }
	bool Delete(int Index) {
		Type *pItem=Remove(Index);
		if (pItem==NULL)
			return false;
		delete pItem;
		return true;
	}
	const Type *Get(int Index) const { return static_cast<const Type*>(m_Array.Get(Index)); }
	Type *Get(int Index) { return static_cast<Type*>(m_Array.Get(Index)); }
	bool Set(int Index,Type *pItem) { return m_Array.Set(Index,pItem); }
	bool Move(int From,int To) { return m_Array.Move(From,To); }
	int Find(const Type *pItem) const { return m_Array.Find(pItem); }
	void Sort(int (*pCompare)(const Type*,const Type*,void*),bool fDescending=false,void *pParam=NULL) {
		m_Array.Sort((CPointerArray::CompareFunc)pCompare,fDescending,pParam);
	}
};


#endif

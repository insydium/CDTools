// CDArray.h

#ifndef _CDArray_H_
#define _CDArray_H_

#include <vector>
using std::vector;


template <typename T>
class CDArray
{
protected:
	vector<T> data;
	typename vector<T>::iterator dIter;
	
public:
	CDArray() {}
	~CDArray() {}
	
	void Init(void) { data.resize(0); }
	void Free(void) { data.clear(); }
	
	// size functions
	int Size(void) { return data.size(); }
	bool IsEmpty(void) { return data.empty(); }
	bool InBounds(int i)
	{
		return i > -1 && i < data.size();
	}
	
	// access functions
	T* Array(void)
	{
		return ((data.size() > 0) ? &data.front() : NULL);
	}
	typename vector<T>::reference operator [](int i)
	{
		return data[i];
	}
	
	// assignment functions
	void Fill(T v) { data.assign(data.size(),v); }
	bool Alloc(int n)
	{
		return Resize(n);
	}
	
	bool Resize(size_t n)
	{
		data.resize(n);
		return data.size() == n;
	}
	
	bool Append(T a)
	{
		size_t n = data.size();
		data.push_back(a);
		return data.size() > n;
	}
	
	bool Remove(int i)
	{
		if(!InBounds(i)) return false;
		size_t n = data.size();
		dIter = data.begin() + i;
		data.erase(dIter);
		return data.size() < n;
	}
	
	bool RemoveEnd(void)
	{
		size_t n = data.size();
		data.pop_back();
		return data.size() < n;
	}

	bool Insert(int i, T d)
	{
		if(!InBounds(i)) return false;
		size_t n = data.size();
		dIter = data.begin() + i;
		data.insert(dIter,d);
		return data.size() > n;
	}
	
	// copy functions
	bool Copy(CDArray &dst)
	{
		if(!dst.Alloc(data.size())) return false;
		dst.data.assign(data.begin(),data.end());
		return dst.data.size() == data.size();
	}
	
	bool CopyTo(CDArray &dst)
	{
		if(dst.data.size() != data.size()) return false;
		dst.data.assign(data.begin(),data.end());
		return true;
	}
	
};


#endif
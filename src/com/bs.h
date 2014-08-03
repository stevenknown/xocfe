/*@
Copyright (c) 2013-2014, Su Zhenyu steven.known@gmail.com 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/
#ifndef __BITSET_H__
#define __BITSET_H__

#define BS_ZERO			0
#define BS_DUMP_BITSET	1
#define BS_DUMP_POS		2
#define BITS_PER_BYTE	8
#define BYTES_PER_UINT	4

class BITSET;
class BITSET_MGR;

extern BITSET * bs_create(BITSET_MGR & bs_mgr);
extern BITSET * bs_union(IN BITSET const& set1, IN BITSET const& set2, 
						OUT BITSET & res);
extern BITSET * bs_diff(IN BITSET const& set1, IN BITSET const& set2, 
						OUT BITSET & res);
extern BITSET * bs_intersect(IN BITSET const& set1, IN BITSET const& set2, 
						OUT BITSET & res);

class BITSET 
{
	friend BITSET * bs_union(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
	friend BITSET * bs_diff(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
	friend BITSET * bs_intersect(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
protected:
	UINT m_size;
	BYTE * m_ptr;
	
	void * realloc(IN void * src, ULONG orgsize, ULONG newsize);
public:
	BITSET(UINT init_pool_size = 1)		
	{
		m_ptr = 0;	
		init(init_pool_size);	
	}	

	//Copy constructor
	BITSET(BITSET const& bs)		
	{
		m_ptr = 0;
		init();
		copy(bs);
	}
	
	~BITSET() { destroy(); }	

	void init(UINT init_pool_size = 1)	
	{
		if (m_ptr != NULL) return;
		m_size = init_pool_size;
		if (init_pool_size == 0) return;
		m_ptr = (BYTE*)::malloc(init_pool_size);
		::memset(m_ptr, 0, m_size); 
	}
	
	void destroy()
	{
		if (m_ptr == NULL) return;
		IS_TRUE0(m_size > 0);
		::free(m_ptr);
		m_ptr = NULL;
		m_size = 0;
	}

	void alloc(UINT size);
	void bunion(BITSET const& bs);
	void bunion(INT elem);

	void copy(BITSET const& src);
	void clean();
	UINT count_mem() const;
	void complement(IN BITSET const& universal_set);	
	
	void diff(UINT elem);
	void diff(BITSET const& bs);
	void dump(CHAR const* name = NULL, bool is_del = false, 
			  UINT flag = BS_DUMP_BITSET | BS_DUMP_POS,
			  INT last_pos = -1) const;
	void dump(FILE * h, UINT flag = BS_DUMP_BITSET | BS_DUMP_POS, 
			  INT last_pos = -1) const;
	
	UINT get_elem_count() const;
	INT get_first() const;
	INT get_last() const;
	bool get(UINT elem) const;
	BITSET * get_subset_in_range(IN UINT low, 
								IN UINT high, 
								OUT BITSET & subset);
	INT get_next(UINT elem) const;
	UINT get_byte_size() const { return m_size; }

	bool has_elem_in_range(UINT low, UINT high) const;

	void intersect(BITSET const& bs);
	bool is_equal(BITSET const& bs) const;
	bool is_contain(UINT elem) const;
	bool is_contain(BITSET const& bs, bool strict = false) const;
	bool is_contained_in_range(UINT low, UINT high, bool strict) const;
	bool is_contain_range(UINT low, UINT high, bool strict) const;
	bool is_intersect(BITSET const& bs) const;
	bool is_overlapped(UINT low, UINT high) const;
	bool is_empty() const;

	BITSET const& operator = (BITSET const& src);

	void rev(UINT last_bit_pos);
};


class BITSET_MGR
{
protected:
	LIST<BITSET*> m_bs_list;
	LIST<BITSET*> m_free_list;
	bool m_is_init;
public:	
	BITSET_MGR()
	{
		m_is_init = false;
		init();
	}
	~BITSET_MGR() { destroy(); }	
	inline void init()
	{
		if (m_is_init) return;
		m_bs_list.init();
		m_is_init = true;
	}	
	void destroy()
	{
		if (!m_is_init) return;
		for (BITSET * bs = m_bs_list.get_head(); 
			 bs != NULL; bs = m_bs_list.get_next()) {
			bs->destroy();
		}
		m_bs_list.destroy();
		m_is_init = false;
	}
	BITSET * create();
	inline BITSET * copy(BITSET const& bs)
	{
		IS_TRUE(m_is_init, ("not yet init"));
		BITSET * p = create();
		p->copy(bs);
		return p;
	}

	inline void clean()
	{
		IS_TRUE(m_is_init, ("not yet init"));
		destroy();
		init();
	}	
	UINT count_mem(FILE * h = NULL);
	inline void free(IN BITSET * bs) //free bs for next use.
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_list.append_tail(bs);
	}
};


//
//START BVEC
//
template <class T> class BVEC : public SVECTOR <T> {
protected:	
	BITSET m_bs; //Record position set by 'set()'	
public:
	BVEC();
	BVEC(INT size);
	~BVEC();
	void init();
	void destroy();
	inline void copy(LIST<T> & list);	
	inline void clone(BVEC<T> & vec);
	UINT count_mem() const;
	inline void clean();
	inline void set(INT i, T elem);
	inline T & operator[](INT i);

	//Get the first index number and return the element. 
	inline T get_first(OUT INT * idx); 
	inline INT get_first() const; //Get first number of index of element.

	//Get next index number and return the next element at the same time. 
	inline T get_next(INT * curidx); 
	inline INT get_next(UINT curidx) const; //Get next index number.
	inline UINT get_elem_count() const; //Get number of elements in vector.
	inline BITSET * get_bs();

	//Clear bit of position 'i', and set new value 't' for the position.
	inline void remove(INT i, T t = (T)0); 
	void dump(CHAR const* name = NULL, bool is_del = false) const
	{ m_bs.dump(name, is_del); }
	void dump(FILE * h) const
	{ m_bs.dump(h); }
};


template <class T>
BVEC<T>::BVEC()
{
	init();
}


template <class T>
BVEC<T>::BVEC(INT size)
{
	init();
	SVECTOR<T>::grow(size);
}


template <class T>
BVEC<T>::~BVEC()
{
	destroy();
}


template <class T>
void BVEC<T>::init()
{
	if (SVECTOR<T>::m_is_init) return;
	SVECTOR<T>::init();
	m_bs.init();	
}


template <class T>
void BVEC<T>::destroy()
{
	if (!SVECTOR<T>::m_is_init) return;
	m_bs.destroy();
	SVECTOR<T>::destroy();
}


template <class T>
void BVEC<T>::copy(LIST<T> & list)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	INT count = 0;
	set(list.get_elem_count()-1, 0); //Alloc memory right away.
	for (T elem = list.get_head(); 
		 elem != (T)0; 
		 elem = list.get_next(), count++) {
		set(count, elem);
	}
}


template <class T>
void BVEC<T>::clone(BVEC<T> & vec)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::copy(vec);
	m_bs.copy(vec.m_bs);
}


template <class T>
void BVEC<T>::clean()
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::clean();
	m_bs.clean();
}


template <class T>
UINT BVEC<T>::count_mem() const
{ 
	return m_bs.count_mem() + ((SVECTOR<T>*)this)->count_mem(); 
}


template <class T>
void BVEC<T>::set(INT i, T elem)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::set(i, elem);
	m_bs.bunion(i);
}


/*
Overloaded [] for non-const array reference return.
Create an lvalue, equal to 'set()'
*/
template <class T>
T & BVEC<T>::operator[](INT i)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	if (i >= SVECTOR<T>::m_size) { 
		set(i, (T)0);
	}
	return SVECTOR<T>::m_vec[i];
}


template <class T>
INT BVEC<T>::get_first() const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_first();
}


template <class T>
T BVEC<T>::get_first(OUT INT * idx)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	INT i = m_bs.get_first(); 
	if (idx) { *idx = i; }	 
	return SVECTOR<T>::get(i);		
}


//Return next element related to current 'idx'.	
template <class T>
T BVEC<T>::get_next(OUT INT * curidx) 
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	*curidx = m_bs.get_next(*curidx);
	return SVECTOR<T>::get(*curidx);
}


template <class T>
INT BVEC<T>::get_next(UINT curidx) const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_next(curidx);
}


//Get number of elements in vector.
template <class T>
UINT BVEC<T>::get_elem_count() const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_elem_count();
}


/*
Clear bit of position 'i', and set new value 't' for the position.
Default placeholder of clear bit is NULL.
*/
template <class T>
void BVEC<T>::remove(INT i, T t)
{
	m_bs.diff(i);
	SVECTOR<T>::set(i, t);
}


template <class T>
BITSET * BVEC<T>::get_bs()
{
	return &m_bs;
}
//END BVEC


//
//START BVEC_MGR
//
template <class T> class BVEC_MGR {
protected:
	LIST<BVEC<T>*> m_bs_list;
	LIST<BVEC<T>*> m_free_list;
	bool m_is_init;
public:
	BVEC_MGR()
	{
		m_is_init = false;
		init();
	}
	~BVEC_MGR(){ destroy(); }
	
	inline void init()
	{
		if (m_is_init) return;
		m_bs_list.init();
		m_is_init = true;
	}	
	void destroy()
	{
		if (!m_is_init) return;
		for (BVEC<T> * bs = m_bs_list.get_head(); 
			 bs != NULL; bs = m_bs_list.get_next()) {
			bs->destroy();
		}
		m_bs_list.destroy();
		m_is_init = false;
	}
	BVEC<T> * create();	
	inline void clean()
	{
		IS_TRUE(m_is_init, ("not yet init"));
		destroy();
		init();
	}	
	UINT count_mem(FILE * h = NULL);
	inline void free(IN BVEC<T> * bs) //free bs for next use.
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_list.append_tail(bs);
	}
};


template <class T>
UINT BVEC_MGR<T>::count_mem(FILE * h)
{
	UINT count = 0;
	for (BVEC<T> const* bs = m_bs_list.get_head();
		 bs != NULL; bs = m_bs_list.get_next()) {
		count += bs->count_mem();
	}
	#ifdef _DEBUG_
    if (h != NULL) {
        //Dump mem usage into file.
        LIST<UINT> lst;
        for (BVEC<T> const* bs = m_bs_list.get_head();
             bs != NULL; bs = m_bs_list.get_next()) {
            UINT c = bs->count_mem();
            C<UINT> * ct;
            UINT n = lst.get_elem_count();
            lst.get_head(&ct);
            UINT i;
            for (i = 0; i < n; i++, lst.get_next(&ct)) {
                if (c >= C_val(ct)) {
                    lst.insert_before(c, ct);
                    break;
                }
            }
            if (i == n) {
                lst.append_head(c);
            }
        }
        UINT v = lst.get_head();
        fprintf(h, "\n== DUMP BITSET_MGR: total %d "
                   "bitsets, mem usage are:\n",
                   m_bs_list.get_elem_count());
        UINT b = 0;
        UINT n = lst.get_elem_count();
        for (UINT i = 0; i < n; i++, v = lst.get_next(), b++) {
            if (b == 20) {
                fprintf(h, "\n");
                b = 0;
            }
            if (v < 1024) {
                fprintf(h, "%dB,", v);
            } else if (v < 1024 * 1024) {
                fprintf(h, "%dKB,", v/1024);
            } else {
                fprintf(h, "%dMB,", v/1024/1024);
            }
        }
        fflush(h);
    }
	#endif		 
	return count;
}


template <class T>
BVEC<T> * BVEC_MGR<T>::create()
{
	IS_TRUE(m_is_init, ("not yet init"));	
	BVEC<T> * p = m_free_list.remove_head();
	if (p == NULL) {
		p = (BVEC<T>*)::malloc(sizeof(BVEC<T>));
		::memset(p, 0, sizeof(BVEC<T>));
		p->init();
		m_bs_list.append_tail(p);
	}
	return p;
}
//END BVEC_MGR
#endif


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
#define BYTES_PER_SEG	64
#define BITS_PER_SEG	(BITS_PER_BYTE * BYTES_PER_SEG)

class BitSet;
class BitSetMgr;

class BitSet
{
	friend BitSet * bs_union(IN BitSet const& set1, IN BitSet const& set2,
							OUT BitSet & res);
	friend BitSet * bs_diff(IN BitSet const& set1, IN BitSet const& set2,
							OUT BitSet & res);
	friend BitSet * bs_intersect(IN BitSet const& set1, IN BitSet const& set2,
							OUT BitSet & res);
protected:
	UINT m_size;
	BYTE * m_ptr;

	void * realloc(IN void * src, size_t orgsize, size_t newsize);
public:
	BitSet(UINT init_pool_size = 1)		
	{
		m_ptr = 0;
		init(init_pool_size);
	}

	//Copy constructor
	BitSet(BitSet const& bs)
	{
		m_ptr = 0;
		init();
		copy(bs);
	}
	BitSet const& operator = (BitSet const& src);
	~BitSet() { destroy(); }

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
		ASSERT(m_size > 0, ("bitset is invalid"));
		::free(m_ptr);
		m_ptr = NULL;
		m_size = 0;
	}

	void alloc(UINT size);
	void bunion(BitSet const& bs);
	void bunion(UINT elem);

	void copy(BitSet const& src);
	void clean();
	UINT count_mem() const { return get_byte_size() + sizeof(BitSet); }
	void complement(IN BitSet const& univers);

	void diff(UINT elem);
	void diff(BitSet const& bs);
	void dump(CHAR const* name = NULL, bool is_del = false,
			  UINT flag = BS_DUMP_BITSET | BS_DUMP_POS,
			  INT last_pos = -1) const;
	void dump(FILE * h, UINT flag, INT last_pos) const;
	void dump(FILE * h) const { dump(h, BS_DUMP_BITSET|BS_DUMP_POS, -1); }

	UINT get_elem_count() const;
	INT get_first() const;
	INT get_last() const;
	BitSet * get_subset_in_range(UINT low, UINT high, OUT BitSet & subset);
	INT get_next(UINT elem) const;
	UINT get_byte_size() const { return m_size; }

	bool has_elem_in_range(UINT low, UINT high) const;

	void intersect(BitSet const& bs);
	bool is_equal(BitSet const& bs) const;
	bool is_contain(UINT elem) const;
	bool is_contain(BitSet const& bs, bool strict = false) const;
	bool is_contained_in_range(UINT low, UINT high, bool strict) const;
	bool is_contain_range(UINT low, UINT high, bool strict) const;
	bool is_intersect(BitSet const& bs) const;
	bool is_overlapped(UINT low, UINT high) const;
	bool is_empty() const;

	void rev(UINT last_bit_pos);
};


class BitSetMgr
{
protected:
	SMemPool * m_pool;
	List<BitSet*> m_bs_list;
	List<BitSet*> m_free_list;

	inline void * xmalloc(size_t size)
	{
		ASSERT(m_pool, ("List not yet initialized."));
		void * p = smpoolMallocConstSize(size, m_pool);
		ASSERT(p, ("malloc failed"));
		memset(p, 0, size);
		return p;
	}
public:
	BitSetMgr()
	{
		m_pool = NULL;
		init();
	}
	COPY_CONSTRUCTOR(BitSetMgr);
	~BitSetMgr() { destroy(); }
	

	inline void init()
	{
		if (m_pool != NULL) { return; }
		m_pool = smpoolCreate(sizeof(BitSet) * 4, MEM_CONST_SIZE);
		m_bs_list.init();
		m_free_list.init();
	}

	void destroy()
	{
		if (m_pool == NULL) { return; }

		C<BitSet*> * ct;
		for (m_bs_list.get_head(&ct); 
			 ct != m_bs_list.end(); ct = m_bs_list.get_next(ct)) {
			BitSet * bs = ct->val();
			ASSERT0(bs);
			bs->destroy();
		}
		
		m_bs_list.destroy();
		m_free_list.destroy();
		smpoolDelete(m_pool);
		m_pool = NULL;
	}

	BitSet * create(UINT init_sz = 0)
	{
		ASSERT(m_pool, ("not yet init"));
		BitSet * p = m_free_list.remove_head();
		if (p == NULL) {
			p = (BitSet*)xmalloc(sizeof(BitSet));
			p->init(init_sz);
			m_bs_list.append_head(p);
		}
		return p;
	}

	inline BitSet * copy(BitSet const& bs)
	{
		ASSERT(m_pool, ("not yet init"));
		BitSet * p = create();
		p->copy(bs);
		return p;
	}

	inline void clean()
	{
		ASSERT(m_pool, ("not yet init"));
		destroy();
		init();
	}
	UINT count_mem(FILE * h = NULL);

	inline void free(IN BitSet * bs) //free bs for next use.
	{
		if (bs == NULL) { return; }
		
		#ifdef _DEBUG_
		C<BitSet*> * ct;
		for (m_free_list.get_head(&ct); 
			 ct != m_free_list.end(); ct = m_free_list.get_next(ct)) {
			BitSet * x = ct->val(); 	
			ASSERT(x && x != bs, ("Already have been freed."));
		}
		#endif
		bs->clean();
		m_free_list.append_head(bs);
	}
};


//
//START BSVec
//
//This class represents a Vector that supply the
//operations like BitSet. You can iterate the element
//in the Vector via get_first and get_next.
template <class T> class BSVec : public Vector<T> {
protected:
	BitSet m_bs; //Record position set by 'set()'
public:
	BSVec() { init(); }
	BSVec(INT size)
	{
		init();
		Vector<T>::grow(size);
	}		
	~BSVec() { destroy(); }

	void init()
	{
		if (Vector<T>::m_is_init) return;
		Vector<T>::init();
		m_bs.init();
	}

	void destroy()
	{
		if (!Vector<T>::m_is_init) return;
		m_bs.destroy();
		Vector<T>::destroy();
	}

	//Copy element from list.
	inline void copy(List<T> & list)
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		INT count = 0;
		
		set(list.get_elem_count()-1, 0); //Alloc memory right away.
	
		C<T> * ct;
		for (list.get_head(&ct); 
			 ct != list.end(); list.get_next(&ct), count++) {
			T elem = ct->val();
			set(count, elem);
		}
	}
	
	inline void copy(BSVec<T> & vec)
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		Vector<T>::copy(vec);
		m_bs.copy(vec.m_bs);
	}
	
	UINT count_mem() const { return m_bs.count_mem() + Vector<T>::count_mem(); }
	inline void clean()
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		Vector<T>::clean();
		m_bs.clean();
	}

	//Overloaded [] for non-const array reference return.
	//Create an lvalue, equal to 'set()'
	inline T & operator[](INT i)
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		if (i >= Vector<T>::m_size) {
			set(i, (T)0);
		}
		return Vector<T>::m_vec[i];
	}

	//Get the first index number and return the element.
	inline T get_first(OUT INT * idx)
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		INT i = m_bs.get_first();
		if (idx) { *idx = i; }
		return Vector<T>::get(i);
	}

	//Get first number of index of element.
	inline INT get_first() const	
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		return m_bs.get_first();
	}

	//Get next index number and return the next element at the same time.
	//Return next element related to current 'idx'.
	inline T get_next(INT * curidx)	
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		*curidx = m_bs.get_next(*curidx);
		return Vector<T>::get(*curidx);
	}

	//Get next index number.
	inline INT get_next(UINT curidx) const
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		return m_bs.get_next(curidx);
	}

	//Get number of elements in vector.	
	inline UINT get_elem_count() const
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		return m_bs.get_elem_count();
	}
	
	inline BitSet * get_bs() { return &m_bs; }

	inline void set(UINT i, T elem) 	
	{
		ASSERT(Vector<T>::m_is_init, ("VECTOR not yet initialized."));
		Vector<T>::set(i, elem);
		m_bs.bunion(i);
	}
	
	//Clear bit of position 'i', and set new value 't' for the position.
	//Default placeholder of clear bit is NULL.
	inline void remove(UINT i, T t = (T)0)
	{
		m_bs.diff(i);
		Vector<T>::set(i, t);
	}

	void dump(CHAR const* name = NULL, bool is_del = false) const
	{ m_bs.dump(name, is_del); }

	void dump(FILE * h) const
	{ m_bs.dump(h); }
};
//END BSVec


//
//START BSVecMgr
//
template <class T> class BSVecMgr {
protected:
	SList<BSVec<T>*> m_bs_list;
	SList<BSVec<T>*> m_free_list;
	SMemPool * m_pool;

public:
	BSVecMgr()
	{
		m_pool = NULL;
		init();
	}
	COPY_CONSTRUCTOR(BSVecMgr);
	~BSVecMgr(){ destroy(); }

	inline void init()
	{
		if (m_pool != NULL) { return; }
		m_pool = smpoolCreate(sizeof(SC<BSVec<T>*>) * 2, MEM_CONST_SIZE);
		m_bs_list.set_pool(m_pool);
		m_free_list.set_pool(m_pool);
	}
	
	void destroy()
	{
		if (m_pool == NULL) { return; }

		for (SC<BSVec<T>*> * ct = m_bs_list.get_head(); 
			 ct != m_bs_list.end(); ct = m_bs_list.get_next(ct)) {
			BSVec<T> * bs = ct->val();
			ASSERT0(bs);
			
			bs->destroy();
		}
		
		smpoolDelete(m_pool);
		m_pool = NULL;
	}
	
	void dump(FILE * h)
	{
		if (h == NULL) { return; }		
	 
		//Dump mem usage into file.
		List<UINT> lst;
		for (BSVec<T> const* bs = m_bs_list.get_head();
			 bs != m_bs_list.end(); bs = m_bs_list.get_next()) {
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
		fprintf(h, "\n== DUMP BitSetMgr: total %d "
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
	
	BSVec<T> * create()
	{
		ASSERT(m_pool, ("not yet init"));
		
		BSVec<T> * p = m_free_list.remove_head();
		if (p == NULL) {
			p = (BSVec<T>*)::malloc(sizeof(BSVec<T>));
			::memset(p, 0, sizeof(BSVec<T>));
			p->init();
			m_bs_list.append_head(p);
		}
		return p;
	}
	
	inline void clean()
	{
		ASSERT(m_pool, ("not yet init"));
		destroy();
		init();
	}
	
	UINT count_mem()
	{
		UINT count = smpoolGetPoolSize(m_pool);
		for (SC<BSVec<T>*> * ct = m_bs_list.get_head(); 
			 ct != m_bs_list.end(); ct = m_bs_list.get_next(ct)) {
			BSVec<T> * bs = ct->val();
			ASSERT0(bs);

			count += bs->count_mem();
		}
		return count;
	}
	
	inline void free(IN BSVec<T> * bs) //free bs for next use.
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_list.append_head(bs);
	}
};



//
//Sparse BitSet
//

//#define DEBUG_SEG
//Segment
class SEG {
public:
	#ifdef DEBUG_SEG
	int id;
	#endif
	UINT start;
	BitSet bs;

	SEG()
	{
		#ifdef DEBUG_SEG
		id = 0;
		#endif
		start = 0;
	}
	SEG(SEG const& src) { copy(src); }

	inline void copy(SEG const& src)
	{
		start = src.start;
		bs.copy(src.bs);
	}

	inline UINT count_mem() const
	{
		return sizeof(start) + bs.count_mem();
	}

	inline void clean()
	{
		start = 0;
		bs.clean();
	}

	inline bool is_contain(UINT elem)
	{
		if (elem < start) { return false; }
		UINT last = start + MAX(bs.get_byte_size(), BYTES_PER_UINT) *
					BITS_PER_BYTE - 1;
		if (elem <= last) {
			return true;
		}
		return false;
	}

	//Return the start position of current segment.
	inline UINT get_start() const { return start; }
	//Return the end position of current segment.
	inline UINT get_end() const { return start + BITS_PER_SEG - 1; }
};


//Segment Manager.
//This class is responsible to allocate and destroy SEG object.
class SegMgr {
#ifdef _DEBUG_
public:
	UINT seg_count;
#endif
	SList<SEG*> m_free_list;
public:
	SegMgr()
	{
		#ifdef _DEBUG_
		seg_count = 0;
		#endif
		SMemPool * p = smpoolCreate(sizeof(SC<SEG*>) * 4,
											 MEM_CONST_SIZE);
		m_free_list.set_pool(p);
	}
	COPY_CONSTRUCTOR(SegMgr);

	~SegMgr()
	{
		#ifdef _DEBUG_
		UINT n = m_free_list.get_elem_count();
		ASSERT(seg_count == n, ("MemLeak! There still are SEGs not freed"));
		#endif

		for (SC<SEG*> * sc = m_free_list.get_head(); 
			 sc != m_free_list.end(); sc = m_free_list.get_next(sc)) {
			SEG * s = sc->val();
			ASSERT0(s);
			
			delete s;
		}
			 
		ASSERT(m_free_list.get_pool(), ("miss pool"));
		
		smpoolDelete(m_free_list.get_pool());
	}

	inline void free(SEG * s)
	{
		s->clean();
		m_free_list.append_head(s);
	}

	SEG * new_seg()
	{
		SEG * s = m_free_list.remove_head();
		if (s != NULL) { return s; }

		#ifdef _DEBUG_
		seg_count++;
		#endif

		s = new SEG();

		#ifdef DEBUG_SEG
		s->id = seg_count;
		#endif

		return s;
	}

	UINT count_mem() const
	{
		UINT count = 0;
		for (SC<SEG*> * sc = m_free_list.get_head();
			 sc != m_free_list.end(); sc = m_free_list.get_next(sc)) {
			SEG * s = sc->val();
			ASSERT0(s);
			
			count += s->count_mem();
		}
			 
		count += m_free_list.count_mem();
		return count;
	}
	
	#ifdef _DEBUG_
	UINT get_seg_count() const { return seg_count; }

	//Decrease seg_count.
	void dec_seg_count() { seg_count--; }
	#endif

	SList<SEG*> const* get_free_list() const { return &m_free_list; }
};


class MiscBitSetMgr;


//Sparse BitSet Core
class SBitSetCore {
protected:
	SList2<SEG*> segs;

	void * realloc(IN void * src, size_t orgsize, size_t newsize);
public:
	SBitSetCore() {}
	COPY_CONSTRUCTOR(SBitSetCore);
	~SBitSetCore() { /*should call clean() before destruction.*/ }

	void bunion(SBitSetCore const& src, SegMgr * sm,
				SC<SEG*> ** free_list, SMemPool * pool);

	void bunion(UINT elem, SegMgr * sm,
				SC<SEG*> ** free_list, SMemPool * pool);
	inline void bunion(UINT elem, MiscBitSetMgr & m);
	inline void bunion(SBitSetCore const& src, MiscBitSetMgr &m);

	inline void clean(MiscBitSetMgr & m);
	void clean(SegMgr * sm, SC<SEG*> ** free_list);
	void copy(SBitSetCore const& src, SegMgr * sm,
			  SC<SEG*> ** free_list, SMemPool * pool);
	inline void copy(SBitSetCore const& src, MiscBitSetMgr & m);
	UINT count_mem() const;

	void destroy_seg_and_clean(SegMgr * sm, SC<SEG*> ** free_list);
	void diff(UINT elem, SegMgr * sm, SC<SEG*> ** free_list);
	inline void diff(UINT elem, MiscBitSetMgr & m);
	void diff(SBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list);
	inline void diff(SBitSetCore const& src, MiscBitSetMgr & m);
	void dump(FILE * h) const;
	void dump2(FILE * h) const;

	UINT get_elem_count() const;
	INT get_first(SC<SEG*> ** cur) const;
	INT get_last(SC<SEG*> ** cur) const;
	INT get_next(UINT elem, SC<SEG*> ** cur) const;
	
	void intersect(SBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list);
	inline void intersect(SBitSetCore const& src, MiscBitSetMgr & m);
	bool is_equal(SBitSetCore const& src) const;
	bool is_contain(UINT elem) const;
	bool is_intersect(SBitSetCore const& src) const;
	bool is_empty() const;
};



//Sparse BitSet
class SBitSet : public SBitSetCore {
protected:
	SMemPool * m_pool;
	SC<SEG*> * m_flst; //free list
	SegMgr * m_sm;
public:
	SBitSet(SegMgr * sm, UINT sz = sizeof(SC<SEG*>))
	{ 
		m_pool = NULL;
		init(sm, sz); 
	}
	COPY_CONSTRUCTOR(SBitSet);
	~SBitSet() { destroy(); }
	
	void init(SegMgr * sm, UINT sz = sizeof(SC<SEG*>))
	{
		ASSERT(sm, ("need SegMgr"));
		ASSERT(sz % sizeof(SC<SEG*>) == 0, ("pool size must be mulitple."));
		ASSERT(m_pool == NULL, ("already initialized"));
		m_pool = smpoolCreate(sz, MEM_CONST_SIZE);
		m_sm = sm;
		m_flst = NULL;
	}
	
	void destroy()
	{
		ASSERT(m_pool, ("already destroy"));
		for (SC<SEG*> * st = segs.get_head(); 
			 st != segs.end(); st = segs.get_next(st)) {
			SEG * s = st->val();
			ASSERT0(s);
			
			m_sm->free(s);
		}
		
		//Unnecessary call clean(), since free pool will free all
		//SC<SEG*> object.
		//SBitSetCore::clean(m_sm, &m_flst);
		smpoolDelete(m_pool);
		m_pool = NULL;
	}
	
	void bunion(SBitSet const& src)
	{ SBitSetCore::bunion(src, m_sm, &m_flst, m_pool);	}

	void bunion(UINT elem)
	{ SBitSetCore::bunion(elem, m_sm, &m_flst, m_pool); }

	void copy(SBitSet const& src)
	{
		//Do NOT change current m_sm.
		SBitSetCore::copy(src, m_sm, &m_flst, m_pool);
	}

	void clean()
	{ SBitSetCore::clean(m_sm, &m_flst); }

	UINT count_mem() const
	{
		UINT c = 0;
		for (SC<SEG*> * st = segs.get_head(); 
			 st != segs.end(); st = segs.get_next(st)) {
			SEG * s = st->val();
			c += s->count_mem();
		}
		c += sizeof(m_pool);
		c += sizeof(m_sm);
		c += segs.count_mem();
		c += smpoolGetPoolSize(m_pool);
		return c;
	}

	void diff(UINT elem)
	{ SBitSetCore::diff(elem, m_sm, &m_flst); }

	//Difference between current bitset and 'src', current bitset
	//will be modified.
	void diff(SBitSet const& src)
	{ SBitSetCore::diff(src, m_sm, &m_flst); }

	//Do intersection for current bitset and 'src', current bitset
	//will be modified.
	void intersect(SBitSet const& src)
	{ SBitSetCore::intersect(src, m_sm, &m_flst); }
};


//
//START DBitSetCore, Dual BitSet Core
//
//This class represent a BitSet which can be switched between sparse and
//dense bitset.
class DBitSetCore : public SBitSetCore {
protected:
	UINT m_is_sparse:1; //true if bitset is sparse.

	//Only read BitSet.
	BitSet const* read_bs() const
	{
		ASSERT(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc = segs.get_head();
		if (sc == segs.end()) {
			return NULL;
		}
		ASSERT0(sc->val());
		return &sc->val()->bs;
	}

	//Get BitSet, alloc BitSet if it not exist.
	BitSet * alloc_bs(SegMgr * sm, SC<SEG*> ** flst, SMemPool * pool)
	{
		ASSERT(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc = segs.get_head();
		if (sc == segs.end()) {
			SEG * t = sm->new_seg();
			segs.append_head(t, flst, pool);
			return &t->bs;
		}
		ASSERT0(sc->val());
		return &sc->val()->bs;
	}

	//Get BitSet and modify BitSet, do not alloc.
	BitSet * get_bs()
	{
		ASSERT(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc = segs.get_head();
		if (sc == segs.end()) {
			return NULL;
		}
		ASSERT0(sc->val());
		return &sc->val()->bs;
	}
public:
	DBitSetCore() { m_is_sparse = true; }
	COPY_CONSTRUCTOR(DBitSetCore);
	~DBitSetCore() {}

	void bunion(DBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list,
				SMemPool * pool)
	{
		ASSERT(this != &src, ("operate on same set"));
		ASSERT(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBitSetCore::bunion(src, sm, free_list, pool);
		} else {
			BitSet const* srcbs = src.read_bs();
			if (srcbs == NULL) { return; }
			BitSet * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->bunion(*srcbs);
		}
	}
	inline void bunion(DBitSetCore const& src, MiscBitSetMgr & m);

	void bunion(UINT elem, SegMgr * sm, SC<SEG*> ** free_list,
				SMemPool * pool)
	{
		if (m_is_sparse) {
			SBitSetCore::bunion(elem, sm, free_list, pool);
		} else {
			BitSet * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->bunion(elem);
		}
	}
	inline void bunion(UINT elem, MiscBitSetMgr & m);

	void copy(DBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list,
			  SMemPool * pool)
	{
		ASSERT(this != &src, ("operate on same set"));
		ASSERT(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBitSetCore::copy(src, sm, free_list, pool);
		} else {
			BitSet const* srcbs = src.read_bs();
			if (srcbs == NULL) {
				clean(sm, free_list);
				return;
			}
			BitSet * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->copy(*srcbs);
		}
	}
	inline void copy(DBitSetCore const& src, MiscBitSetMgr & m);

	UINT count_mem() const { return SBitSetCore::count_mem() + 1; }

	void diff(UINT elem, SegMgr * sm, SC<SEG*> ** free_list)
	{
		if (m_is_sparse) {
			SBitSetCore::diff(elem, sm, free_list);
		} else {
			BitSet * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->diff(elem);
		}
	}
	inline void diff(UINT elem, MiscBitSetMgr & m);

	void diff(DBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list)
	{
		ASSERT(this != &src, ("operate on same set"));
		ASSERT(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBitSetCore::diff(src, sm, free_list);
		} else {
			BitSet const* srcbs = src.read_bs();
			if (srcbs == NULL) { return; }
			BitSet * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->diff(*srcbs);
		}
	}
	inline void diff(DBitSetCore const& src, MiscBitSetMgr & m);

	void intersect(DBitSetCore const& src, SegMgr * sm, SC<SEG*> ** free_list)
	{
		ASSERT(this != &src, ("operate on same set"));
		ASSERT(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBitSetCore::intersect(src, sm, free_list);
		} else {
			BitSet const* srcbs = src.read_bs();
			if (srcbs == NULL) {
				clean(sm, free_list);
				return;
			}
			BitSet * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->intersect(*srcbs);
		}
	}
	inline void intersect(DBitSetCore const& src, MiscBitSetMgr & m);

	bool is_contain(UINT elem) const
	{
		if (m_is_sparse) {
			return SBitSetCore::is_contain(elem);
		}
		BitSet const* tgtbs = read_bs();
		if (tgtbs == NULL) { return false; }
		return tgtbs->is_contain(elem);
	}

	bool is_equal(DBitSetCore const& src) const
	{
		ASSERT(this != &src, ("operate on same set"));
		ASSERT(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			return SBitSetCore::is_equal(src);
		}
		BitSet const* srcbs = src.read_bs();
		BitSet const* tgtbs = read_bs();
		if (srcbs == NULL) {
			if (tgtbs == NULL) { return true; }
			if (tgtbs->is_empty()) { return true; }
			return false;
		}
		if (tgtbs == NULL) {
			if (srcbs->is_empty()) { return true; }
			return false;
		}
		return tgtbs->is_equal(*srcbs);
	}

	INT get_first(SC<SEG*> ** cur) const;
	INT get_last(SC<SEG*> ** cur) const;

	void set_sparse(bool is_sparse) { m_is_sparse = (UINT)is_sparse; }
};



//
//START DBitSet, Dual BitSet
//
//This class represent a BitSet which can be switched between sparse and
//dense bitset.
class DBitSet : public DBitSetCore {
protected:
	SMemPool * m_pool;
	SC<SEG*> * m_flst;
	SegMgr * m_sm;
public:
	DBitSet(SegMgr * sm, UINT sz = sizeof(SC<SEG*>))
	{
		ASSERT(sm, ("need SegMgr"));
		ASSERT(sz % sizeof(SC<SEG*>) == 0, ("pool size must be mulitple."));
		m_is_sparse = true;
		m_pool = smpoolCreate(sz, MEM_CONST_SIZE);
		m_sm = sm;
		m_flst = NULL;
	}
	COPY_CONSTRUCTOR(DBitSet);
	~DBitSet()
	{
		for (SC<SEG*> * st = segs.get_head(); 
			 st != segs.end(); st = segs.get_next(st)) {
			SEG * s = st->val();
			ASSERT0(s);
			
			m_sm->free(s);
		}

		//Unnecessary call clean(), since free pool will free all
		//SC<SEG*> object.
		//DBitSetCore::clean(m_sm, &m_flst);
		smpoolDelete(m_pool);
	}

	void bunion(DBitSet const& src)
	{ DBitSetCore::bunion(src, m_sm, &m_flst, m_pool);	}

	void bunion(UINT elem)
	{ DBitSetCore::bunion(elem, m_sm, &m_flst, m_pool);	}

	void copy(DBitSet const& src)
	{ DBitSetCore::copy(src, m_sm, &m_flst, m_pool); }

	UINT count_mem() const
	{
		UINT count = sizeof(m_pool);
		count += sizeof(m_flst);
		count += sizeof(m_sm);
		count += smpoolGetPoolSize(m_pool);
		count += DBitSetCore::count_mem();
		return count;
	}

	void clean()
	{ DBitSetCore::clean(m_sm, &m_flst); }

	void diff(UINT elem)
	{ DBitSetCore::diff(elem, m_sm, &m_flst); }

	void diff(DBitSet const& src)
	{ DBitSetCore::diff(src, m_sm, &m_flst); }

	void intersect(DBitSet const& src)
	{ DBitSetCore::intersect(src, m_sm, &m_flst); }
};


//This class represent a BitSet Manager that is response for creating
//and destory dense bitset, sparse bitset and dual bitset.
#define MiscBitSetMgr_sc_free_list(sbs)		((sbs)->scflst)
class MiscBitSetMgr
{
protected:
	SList<SBitSet*> m_sbitset_list;
	SList<DBitSet*> m_dbitset_list;
	SList<DBitSetCore*> m_dbitsetc_list;
	SList<SBitSetCore*> m_free_sbitsetc_list;	
	SList<SBitSet*> m_free_sbitset_list;
	SList<DBitSet*> m_free_dbitset_list;
	SList<DBitSetCore*> m_free_dbitsetc_list;
	
	SMemPool * m_sbitsetc_pool;
	SMemPool * m_dbitsetc_pool;
	
protected:
	SBitSetCore * xmalloc_sbitsetc()
	{
		ASSERT(m_sbitsetc_pool, ("not yet initialized."));
		SBitSetCore * p = (SBitSetCore*)smpoolMallocConstSize(sizeof(SBitSetCore), 
															m_sbitsetc_pool);
		ASSERT(p, ("malloc failed"));
		memset(p, 0, sizeof(SBitSetCore));
		return p;
	}

	DBitSetCore * xmalloc_dbitsetc()
	{
		ASSERT(m_dbitsetc_pool, ("not yet initialized."));
		DBitSetCore * p = (DBitSetCore*)smpoolMallocConstSize(sizeof(DBitSetCore), 
															m_dbitsetc_pool);
		ASSERT(p, ("malloc failed"));
		memset(p, 0, sizeof(DBitSetCore));
		return p;
	}
public:
	//SEG manager.
	SegMgr sm;

	//Free list of SC<SEG*> container. It will be allocated in ptr_pool.
	SC<SEG*> * scflst; 
	
	SMemPool * ptr_pool; //only used to allocate SC<SEG*>.

public:
	MiscBitSetMgr() { ptr_pool = NULL; init(); }
	COPY_CONSTRUCTOR(MiscBitSetMgr);
	~MiscBitSetMgr() { destroy(); }

	void init()
	{
		if (ptr_pool != NULL) { return; }

		ptr_pool = smpoolCreate(sizeof(SC<SEG*>) * 10,
										MEM_CONST_SIZE);
		m_sbitsetc_pool = smpoolCreate(sizeof(SBitSetCore) * 10,
										 MEM_CONST_SIZE);
		m_dbitsetc_pool = smpoolCreate(sizeof(DBitSetCore) * 10,
										 MEM_CONST_SIZE);
		
		m_sbitset_list.set_pool(ptr_pool);
		m_dbitset_list.set_pool(ptr_pool);
		m_dbitsetc_list.set_pool(ptr_pool);
		
		m_free_sbitsetc_list.set_pool(ptr_pool);
		m_free_sbitset_list.set_pool(ptr_pool);
		m_free_dbitset_list.set_pool(ptr_pool);
		m_free_dbitsetc_list.set_pool(ptr_pool);
		
		scflst = NULL;
	}
	
	void destroy()
	{
		if (ptr_pool == NULL) { return; }

		for (SC<SBitSet*> * st = m_sbitset_list.get_head();
			 st != m_sbitset_list.end(); st = m_sbitset_list.get_next(st)) {
			SBitSet * s = st->val();
			ASSERT0(s);
			delete s;
		}
		
		for (SC<DBitSet*> * dt = m_dbitset_list.get_head();
			 dt != m_dbitset_list.end(); dt = m_dbitset_list.get_next(dt)) {
			DBitSet * d = dt->val(); 
			ASSERT0(d);
			delete d;
		}
		
		//All DBitSetCore and SBitSetCore are allocated in the pool.
		//It is not necessary to destroy it specially.
		//SC<DBitSetCore*> * dct;
		//for (DBitSetCore * d = m_dbitsetc_list.get_head(&dct);
		//	 d != NULL; d = m_dbitsetc_list.get_next(&dct)) {
		//	delete d;
		//}

		smpoolDelete(m_sbitsetc_pool);
		smpoolDelete(m_dbitsetc_pool);
		smpoolDelete(ptr_pool);
		
		ptr_pool = NULL;
		m_sbitsetc_pool = NULL;
		m_dbitsetc_pool = NULL;
		scflst = NULL;
	}
	
	inline SBitSet * create_sbitset()
	{
		SBitSet * p = m_free_sbitset_list.remove_head();
		if (p == NULL) {
			p = new SBitSet(&sm);
			m_sbitset_list.append_head(p);
		}
		return p;
	}

	inline SBitSetCore * create_sbitsetc()
	{
		SBitSetCore * p = m_free_sbitsetc_list.remove_head();
		if (p == NULL) {
			p = xmalloc_sbitsetc();
		}
		return p;
	}

	inline DBitSet * create_dbitset()
	{
		DBitSet * p = m_free_dbitset_list.remove_head();
		if (p == NULL) {
			p = new DBitSet(&sm);
			m_dbitset_list.append_head(p);
		}
		return p;
	}

	inline DBitSetCore * create_dbitsetc()
	{
		DBitSetCore * p = m_free_dbitsetc_list.remove_head();
		if (p == NULL) {
			p = xmalloc_dbitsetc();
			p->set_sparse(true);
			m_dbitsetc_list.append_head(p);
		}
		return p;
	}

	//Note that this function does not add up the memory allocated by 
	//create_sbitsetc() and create_dbitsetc(). You should count these
	//objects additionally.
	UINT count_mem(FILE * h = NULL) const;

	//free bs for next use.
	inline void free_sbitset(SBitSet * bs)
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_sbitset_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_sbitsetc(SBitSetCore * bs)
	{
		if (bs == NULL) return;
		bs->clean(*this);
		m_free_sbitsetc_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_dbitset(DBitSet * bs)
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_dbitset_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_dbitsetc(DBitSetCore * bs)
	{
		if (bs == NULL) return;
		bs->clean(&sm, &scflst);
		m_free_dbitsetc_list.append_head(bs);
	}

	//This function destroy SEG objects and free containers back to
	//MiscBitSetMgr for next use.
	inline void destroy_seg_and_freedc(DBitSetCore * bs)
	{
		if (bs == NULL) { return; }
		bs->destroy_seg_and_clean(&sm, &scflst);

		//Recycle bs.
		m_free_dbitsetc_list.append_head(bs);
	}

	//Get SegMgr.
	inline SegMgr * get_seg_mgr() { return &sm; }
};
//END MiscBitSetMgr



//
//START SBitSetCore
//
void SBitSetCore::bunion(UINT elem, MiscBitSetMgr & m)
{
	bunion(elem, &m.sm, &m.scflst, m.ptr_pool);
}


void SBitSetCore::bunion(SBitSetCore const& src, MiscBitSetMgr &m)
{
	bunion(src, &m.sm, &m.scflst, m.ptr_pool);
}


void SBitSetCore::copy(SBitSetCore const& src, MiscBitSetMgr & m)
{
	copy(src, &m.sm, &m.scflst, m.ptr_pool);
}


void SBitSetCore::clean(MiscBitSetMgr & m)
{
	clean(&m.sm, &m.scflst);
}


void SBitSetCore::diff(UINT elem, MiscBitSetMgr & m)
{
	diff(elem, &m.sm, &m.scflst);
}


void SBitSetCore::diff(SBitSetCore const& src, MiscBitSetMgr & m)
{
	diff(src, &m.sm, &m.scflst);
}


void SBitSetCore::intersect(SBitSetCore const& src, MiscBitSetMgr & m)
{
	intersect(src, &m.sm, &m.scflst);
}
//END SBitSetCore


//
//START DBitSetCore
//
void DBitSetCore::bunion(DBitSetCore const& src, MiscBitSetMgr & m)
{
	bunion(src, &m.sm, &m.scflst, m.ptr_pool);
}


void DBitSetCore::bunion(UINT elem, MiscBitSetMgr & m)
{
	bunion(elem, &m.sm, &m.scflst, m.ptr_pool);
}


void DBitSetCore::copy(DBitSetCore const& src, MiscBitSetMgr & m)
{
	copy(src, &m.sm, &m.scflst, m.ptr_pool);
}


void DBitSetCore::diff(UINT elem, MiscBitSetMgr & m)
{
	diff(elem, &m.sm, &m.scflst);
}


void DBitSetCore::diff(DBitSetCore const& src, MiscBitSetMgr & m)
{
	diff(src, &m.sm, &m.scflst);
}


void DBitSetCore::intersect(DBitSetCore const& src, MiscBitSetMgr & m)
{
	intersect(src, &m.sm, &m.scflst);
}
//END DBitSetCore

extern BYTE const g_bit_count[];
extern inline BitSet * bs_create(BitSetMgr & bs_mgr)
{
	return bs_mgr.create();
}
extern BitSet * bs_union(IN BitSet const& set1, IN BitSet const& set2,
						OUT BitSet & res);
extern BitSet * bs_diff(IN BitSet const& set1, IN BitSet const& set2,
						OUT BitSet & res);
extern BitSet * bs_intersect(IN BitSet const& set1, IN BitSet const& set2,
						OUT BitSet & res);
#endif

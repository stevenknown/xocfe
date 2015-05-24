#ifndef __SYMTAB_H__
#define __SYMTAB_H__

/*
Record a variety of symbols such as user defined variables,
compiler internal variables, LABEL, ID, TYPE_NAME etc.
*/
#define SYM_name(sym)			((sym)->s)
class SYM {
public:
	CHAR * s;
};


class CSYM_HASH_FUNC {
public:
	UINT compute_char_sum(CHAR const* s) const
	{
		UINT v = 0 ;
		UINT cnt = 0;
		while ((*s++ != 0) && (cnt < 6)) {
			v += (UINT)(*s);
			cnt++;
		}
		return v;
	}

	UINT get_hash_value(SYM const* s, UINT bs) const
	{
		IS_TRUE0(is_power_of_2(bs));
		UINT v = compute_char_sum(SYM_name(s));
		return hash32bit(v) & (bs - 1);
	}

	//Note v must be const string pointer.
	UINT get_hash_value(OBJTY v, UINT bs) const
	{
		IS_TRUE(sizeof(OBJTY) == sizeof(CHAR const*),
				("exception will taken place in type-cast"));
		IS_TRUE0(is_power_of_2(bs));
		UINT n = compute_char_sum((CHAR const*)v);
		return hash32bit(n) & (bs - 1);
	}

	bool compare(SYM const* s1, SYM const* s2) const
	{ return strcmp(SYM_name(s1),  SYM_name(s2)) == 0; }

	bool compare(SYM const* s, OBJTY val) const
	{
		IS_TRUE(sizeof(OBJTY) == sizeof(CHAR const*),
				("exception will taken place in type-cast"));
		return (strcmp(SYM_name(s),  (CHAR const*)val) == 0);
	}
};


class SYM_HASH_FUNC {
public:
	UINT compute_char_sum(CHAR const* s) const
	{
		UINT v = 0;
		UINT cnt = 0;
		while ((*s++ != 0) && (cnt < 6)) {
			v += (UINT)(*s);
			cnt++;
		}
		return v;
	}

	UINT get_hash_value(SYM * s, UINT bs) const
	{
		IS_TRUE0(is_power_of_2(bs));
		UINT v = compute_char_sum(SYM_name(s));
		return hash32bit(v) & (bs - 1);
	}

	//Note v must be string pointer.
	UINT get_hash_value(OBJTY v, UINT bs) const
	{
		IS_TRUE(sizeof(OBJTY) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		IS_TRUE0(is_power_of_2(bs));
		UINT n = compute_char_sum((CHAR*)v);
		return hash32bit(n) & (bs - 1);
	}

	bool compare(SYM * s1, SYM * s2) const
	{ return strcmp(SYM_name(s1),  SYM_name(s2)) == 0; }

	bool compare(SYM * s, OBJTY val) const
	{
		IS_TRUE(sizeof(OBJTY) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		return (strcmp(SYM_name(s),  (CHAR*)val) == 0);
	}
};


class SYM_TAB : public SHASH<SYM*, SYM_HASH_FUNC> {
	SMEM_POOL * m_pool;
public:
	explicit SYM_TAB(UINT bsize) : SHASH<SYM*, SYM_HASH_FUNC>(bsize)
	{ m_pool = smpool_create_handle(64, MEM_COMM); }
	virtual ~SYM_TAB()
	{ smpool_free_handle(m_pool); }

	inline CHAR * strdup(CHAR const* s)
	{
		if (s == NULL) {
			return NULL;
		}
		UINT l = strlen(s);
		CHAR * ns = (CHAR*)smpool_malloc_h(l + 1, m_pool);
		memcpy(ns, s, l);
		ns[l] = 0;
		return ns;
	}

	SYM * create(OBJTY v)
	{
		SYM * sym = (SYM*)smpool_malloc_h(sizeof(SYM), m_pool);
		SYM_name(sym) = strdup((CHAR const*)v);
		return sym;
	}

	inline SYM * add(CHAR const* s)
	{
		UINT sz = SHASH<SYM*, SYM_HASH_FUNC>::get_bucket_size() * 4;
		if (sz < SHASH<SYM*, SYM_HASH_FUNC>::get_elem_count()) {
			SHASH<SYM*, SYM_HASH_FUNC>::grow(sz);
		}
		return SHASH<SYM*, SYM_HASH_FUNC>::append((OBJTY)s); 
	}

	inline SYM * get(CHAR const* s)
	{ return SHASH<SYM*, SYM_HASH_FUNC>::find((OBJTY)s); }
};
#endif
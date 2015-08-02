#ifndef __SYMTAB_H__
#define __SYMTAB_H__

//Record a variety of symbols such as user defined variables,
//compiler internal variables, LABEL, ID, TYPE_NAME etc.
#define SYM_name(sym)			((sym)->s)
class SYM {
public:
	CHAR * s;
};


class ConstSymbolHashFunc {
public:
	UINT computeCharSum(CHAR const* s) const
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
		ASSERT0(is_power_of_2(bs));
		UINT v = computeCharSum(SYM_name(s));
		return hash32bit(v) & (bs - 1);
	}

	//Note v must be const string pointer.
	UINT get_hash_value(OBJTY v, UINT bs) const
	{
		ASSERT(sizeof(OBJTY) == sizeof(CHAR const*),
				("exception will taken place in type-cast"));
		ASSERT0(is_power_of_2(bs));
		UINT n = computeCharSum((CHAR const*)v);
		return hash32bit(n) & (bs - 1);
	}

	bool compare(SYM const* s1, SYM const* s2) const
	{ return strcmp(SYM_name(s1),  SYM_name(s2)) == 0; }

	bool compare(SYM const* s, OBJTY val) const
	{
		ASSERT(sizeof(OBJTY) == sizeof(CHAR const*),
				("exception will taken place in type-cast"));
		return (strcmp(SYM_name(s),  (CHAR const*)val) == 0);
	}
};


class SymbolHashFunc {
public:
	UINT computeCharSum(CHAR const* s) const
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
		ASSERT0(is_power_of_2(bs));
		UINT v = computeCharSum(SYM_name(s));
		return hash32bit(v) & (bs - 1);
	}

	//Note v must be string pointer.
	UINT get_hash_value(OBJTY v, UINT bs) const
	{
		ASSERT(sizeof(OBJTY) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		ASSERT0(is_power_of_2(bs));
		UINT n = computeCharSum((CHAR*)v);
		return hash32bit(n) & (bs - 1);
	}

	bool compare(SYM * s1, SYM * s2) const
	{ return strcmp(SYM_name(s1),  SYM_name(s2)) == 0; }

	bool compare(SYM * s, OBJTY val) const
	{
		ASSERT(sizeof(OBJTY) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		return (strcmp(SYM_name(s),  (CHAR*)val) == 0);
	}
};


class SymTab : public SHash<SYM*, SymbolHashFunc> {
	SMemPool * m_pool;
public:
	explicit SymTab(UINT bsize) : SHash<SYM*, SymbolHashFunc>(bsize)
	{ m_pool = smpoolCreate(64, MEM_COMM); }
	virtual ~SymTab()
	{ smpoolDelete(m_pool); }

	inline CHAR * strdup(CHAR const* s)
	{
		if (s == NULL) {
			return NULL;
		}
		UINT l = strlen(s);
		CHAR * ns = (CHAR*)smpoolMalloc(l + 1, m_pool);
		memcpy(ns, s, l);
		ns[l] = 0;
		return ns;
	}

	SYM * create(OBJTY v)
	{
		SYM * sym = (SYM*)smpoolMalloc(sizeof(SYM), m_pool);
		SYM_name(sym) = strdup((CHAR const*)v);
		return sym;
	}

	inline SYM * add(CHAR const* s)
	{
		UINT sz = SHash<SYM*, SymbolHashFunc>::get_bucket_size() * 4;
		if (sz < SHash<SYM*, SymbolHashFunc>::get_elem_count()) {
			SHash<SYM*, SymbolHashFunc>::grow(sz);
		}
		return SHash<SYM*, SymbolHashFunc>::append((OBJTY)s); 
	}

	inline SYM * get(CHAR const* s)
	{ return SHash<SYM*, SymbolHashFunc>::find((OBJTY)s); }
};
#endif
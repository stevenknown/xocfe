#ifndef __SYMTAB_H__
#define __SYMTAB_H__

/*
Record a variety of symbols such as user defined variables, 
compiler internal variables, LABEL, ID, TYPE_NAME etc.
*/
class SYM {
public:
	CHAR * s;
	INT hash_val; //retrive index in hash bucket	
};
#define SYM_hash_val(sym)		(sym)->hash_val
#define SYM_name(sym)			(sym)->s


#define MAX_HASH_BUCKET_SYMTAB 217
class SYM_TAB : public SHASH<SYM*> {
	SMEM_POOL * m_pool;
public:
	explicit SYM_TAB(UINT bsize) : SHASH<SYM*>(bsize) 
	{ m_pool = smpool_create_handle(64, MEM_COMM); }
	virtual ~SYM_TAB() 
	{ smpool_free_handle(m_pool); }
	
	UINT compute_hash_value(CHAR const* s)
	{
		UINT v = 0 ;
		while (*s++) {
			v += (UINT)(*s);
		}
		v %= m_bucket_size;
		return v;
	}

	UINT get_hash_value(SYM * s)
	{ return SYM_hash_val(s); }

	UINT get_hash_value(ULONG v)
	{	
		IS_TRUE(sizeof(ULONG) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		return compute_hash_value((CHAR const*)v);
	}

	bool compare(SYM * s1, SYM * s2)
	{ return strcmp(SYM_name(s1),  SYM_name(s2)) == 0; }

	bool compare(SYM * s, ULONG val)
	{
		IS_TRUE(sizeof(ULONG) == sizeof(CHAR*),
				("exception will taken place in type-cast"));
		return (strcmp(SYM_name(s),  (CHAR const*)val) == 0); 
	}

	inline CHAR * strdup(CHAR const* s)
	{
		if (s == NULL) {
			return NULL;
		}	
		INT l = strlen(s);
		CHAR * ns = (CHAR*)_xmalloc(l + 1);
		memcpy(ns, s, l);
		ns[l] = 0;
		return ns;
	}

	SYM * create(ULONG v)
	{ 
		SYM * sym = (SYM*)smpool_malloc_h(sizeof(SYM), m_pool);
		SYM_name(sym) = strdup((CHAR const*)v);
		SYM_hash_val(sym) = compute_hash_value(SYM_name(sym));
		return sym;
	}

	inline SYM * add(CHAR const* s) { return SHASH<SYM*>::append((ULONG)s); }
	inline SYM * get(CHAR const* s) 
	{ SYM * sym = SHASH<SYM*>::find((ULONG)s); return sym; }
};
#endif


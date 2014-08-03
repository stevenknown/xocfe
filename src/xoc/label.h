#ifndef __LABEL_H__
#define __LABEL_H__

typedef enum {
	L_UNDEF = 0,
	L_CLABEL, //customer defined label
	L_ILABEL, //internal generated label
	L_PRAGMA, //pragma 
} LABEL_TYPE;

#define PREFIX_OF_LABEL()     "_L"
#define POSTFIX_OF_LABEL()    ""
#define PREFIX_OF_CLABEL()    ""
#define POSTFIX_OF_CLABEL()   ""

#define ILABEL_STR_FORMAT  "%s%d%s" //prefix label-num postfix
#define ILABEL_CONT(li) \
	PREFIX_OF_LABEL(),LABEL_INFO_num(li),POSTFIX_OF_LABEL()

#define CLABEL_STR_FORMAT  "%s%s%s" //prefix label-name postfix
#define CLABEL_CONT(li) \
	PREFIX_OF_CLABEL(), SYM_name(LABEL_INFO_name(li)), POSTFIX_OF_CLABEL()


#define LABEL_INFO_type(l)				((l)->ltype)
#define LABEL_INFO_name(l)				((l)->u1.lab_name)
#define LABEL_INFO_num(l)				((l)->u1.lab_num)
#define LABEL_INFO_pragma(l)			((l)->u1.pragma_str)
#define LABEL_INFO_is_used(l)			((l)->u2.s1.is_used)
#define LABEL_INFO_is_catch_start(l)	((l)->u2.s1.is_catch_start)
#define LABEL_INFO_is_try_start(l)		((l)->u2.s1.is_try_start)
#define LABEL_INFO_is_try_end(l)		((l)->u2.s1.is_try_end)
#define LABEL_INFO_is_pragma(l)			(LABEL_INFO_type(l) == L_PRAGMA)
#define LABEL_INFO_b1(l)				((l)->u2.b1)
class LABEL_INFO {
public:
	LABEL_TYPE ltype;
	union {
		SYM * lab_name;
		INT lab_num;
		SYM * pragma_str;
	} u1;
	union {
		struct {
			BYTE is_used:1;
			BYTE is_catch_start:1; //Set if current label is the start
								   //label of exception catch block.
			BYTE is_try_start:1;   //Set if current label is the start
								   //label of exception try block.
			BYTE is_try_end:1;	   //Set if current label is the end
								   //label of exception try block.
		} s1;
		BYTE b1;
	} u2;
	
	void copy(LABEL_INFO const& li)
	{
		ltype = li.ltype;
		u1.lab_name = li.u1.lab_name;
		u2.b1 = li.u2.b1;
	}
};


/*
Exported Functions
Simplest method to compute hash value.
*/
inline INT lab_hash_value(LABEL_INFO const* li)
{ 
	INT v = 0;
	if (LABEL_INFO_type(li) == L_CLABEL) {
		CHAR const* p = SYM_name(LABEL_INFO_name(li));
		while (*p != 0) {
			v += *p;
			p++;
		}
	} else {
		IS_TRUE0(LABEL_INFO_type(li) == L_ILABEL);
		v = LABEL_INFO_num(li);
		v = (((UINT)(-v)) >> 7) ^ 0xAC5AAC5A;
	}
	return v;
}
LABEL_INFO * new_label(SMEM_POOL * pool);
LABEL_INFO * new_ilabel(SMEM_POOL * pool);
LABEL_INFO * new_clabel(SYM * st, SMEM_POOL * pool);
bool is_same_label(LABEL_INFO const* li1, LABEL_INFO const* li2);
void dump_lab(LABEL_INFO const* li);	
#endif 


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
#ifndef __DECL_H__
#define __DECL_H__

#define ALLOW_CONST_VOLATILE 1

class SCOPE;

class EVAL_LIST {
public:
	INT val;
	SYM	* str;
	EVAL_LIST * next;
	EVAL_LIST * prev;
};
#define EVAL_LIST_val(el) (el)->val
#define EVAL_LIST_name(el) (el)->str
#define EVAL_LIST_next(el) (el)->next
#define EVAL_LIST_prev(el) (el)->prev


/*
ENUM

Record ENUM info, and field 'name' reserved its character description.
*/
#define ENUM_name(e)    	(e)->name
#define ENUM_vallist(e)     (e)->pevlist
class ENUM {
public:
	INT val;
	SYM * name;  
	EVAL_LIST * pevlist;
};


//ENUM_LIST
#define ENUM_LIST_enum(el) (el)->e
#define ENUM_LIST_next(el) (el)->next
#define ENUM_LIST_prev(el) (el)->prev
class ENUM_LIST {
public:
	ENUM_LIST * next;
	ENUM_LIST * prev;
	ENUM * e; 
};


//USER_TYPE_LIST record user defined type with 'typedef'
#define USER_TYPE_LIST_utype(utl)	(utl)->ut
#define USER_TYPE_LIST_next(utl)	(utl)->next
#define USER_TYPE_LIST_prev(utl)	(utl)->prev
class USER_TYPE_LIST {
public:
	USER_TYPE_LIST * next;
	USER_TYPE_LIST * prev;
	DECL * ut; 
};


//STRUCT
#define STRUCT_next(s)           (s)->next
#define STRUCT_prev(s)           (s)->prev
#define STRUCT_decl_list(s)      (s)->decl_list
#define STRUCT_tag(s)            (s)->tag
#define STRUCT_is_complete(s)    (s)->is_complete
#define STRUCT_align(s)          (s)->align
class STRUCT {
public:
	STRUCT * next;
	STRUCT * prev;
	DECL * decl_list;
	SYM * tag;
	bool is_complete;
	UINT align;
};


//UNION
#define UNION_next(s)			(s)->next
#define UNION_prev(s)			(s)->prev
#define UNION_decl_list(s)		(s)->decl_list
#define UNION_tag(s)			(s)->tag
#define UNION_is_complete(s)	(s)->is_complete
#define UNION_align(s)			(s)->align
class UNION {
public:
	UNION * next;
	UNION * prev;
	DECL * decl_list;
	SYM * tag;
	bool is_complete;
	UINT align;
};


//Qualifier
#define T_QUA_CONST				(0x1)
#define T_QUA_VOLATILE			(0x2)
#define T_QUA_RESTRICT			(0x4)

//Specifier
#define T_SPEC_VOID				(0x8)
#define T_SPEC_CHAR				(0x10)
#define T_SPEC_SHORT			(0x20)
#define T_SPEC_INT				(0x40)
#define T_SPEC_LONG				(0x80)
#define T_SPEC_FLOAT			(0x100)
#define T_SPEC_DOUBLE			(0x200)
#define T_SPEC_SIGNED			(0x400)
#define T_SPEC_UNSIGNED			(0x800)
#define T_SPEC_STRUCT			(0x1000)
#define T_SPEC_UNION			(0x2000)
#define T_SPEC_ENUM_TYPE		(0x4000)
#define T_SPEC_USER_TYPE		(0x8000)
#define T_SPEC_LONGLONG			(0x10000)
#define T_SPEC_BOOL				(0x20000)

//Storage type
#define T_STOR_AUTO				(0x40000)
#define T_STOR_REG				(0x80000)
#define T_STOR_STATIC			(0x100000)
#define T_STOR_EXTERN			(0x200000)
#define T_STOR_TYPEDEF			(0x400000)
#define T_STOR_INLINE			(0x800000)


#define MAX_TYPE_FLD			3
#define TYPE_des(t)				(t)->t_des
#define TYPE_parent(t)			(t)->pfld[0]
#define TYPE_next(t)			(t)->pfld[1]
#define TYPE_prev(t)			(t)->pfld[2]
#define TYPE_user_type(t)		(t)->u1.user_type //user type define 
#define TYPE_enum_type(t)		(t)->u1.enum_type //enum type define
#define TYPE_struct_type(t)		(t)->u1.struct_type //struct type define
#define TYPE_union_type(t)		(t)->u1.union_type //union type define
#define IS_CONST(t)				HAVE_FLAG(TYPE_des(t), T_QUA_CONST)
#define IS_VOLATILE(t)			HAVE_FLAG(TYPE_des(t), T_QUA_VOLATILE)
#define IS_RESTRICT(t)			HAVE_FLAG(TYPE_des(t), T_QUA_RESTRICT)
#define IS_STRUCT(t)			HAVE_FLAG(TYPE_des(t), T_SPEC_STRUCT)
#define IS_UNION(t)				HAVE_FLAG(TYPE_des(t), T_SPEC_UNION)
#define IS_TYPE(t,T)			HAVE_FLAG(TYPE_des(t), T)
#define IS_TYPED(des,T)			HAVE_FLAG((des), T) 
#define IS_REG(t)				HAVE_FLAG(TYPE_des(t), T_STOR_REG)
#define IS_STATIC(t)			HAVE_FLAG(TYPE_des(t), T_STOR_STATIC)
#define IS_EXTERN(t)			HAVE_FLAG(TYPE_des(t), T_STOR_EXTERN)
#define IS_INLINE(t)			HAVE_FLAG(TYPE_des(t), T_STOR_INLINE)
#define IS_TYPEDEF(t)			HAVE_FLAG(TYPE_des(t), T_STOR_TYPEDEF)
#define IS_ENUM_TYPE(t)			HAVE_FLAG(TYPE_des(t), T_SPEC_ENUM_TYPE)
#define IS_USER_TYPE(t)			HAVE_FLAG(TYPE_des(t), T_SPEC_USER_TYPE)
#define IS_DOUBLE(t)			HAVE_FLAG(t, T_SPEC_DOUBLE) 
#define IS_FLOAT(t)				HAVE_FLAG(t, T_SPEC_FLOAT) 
class TYPE { 
public:
	ULONG t_des; /* descripte: const volatile
				    	void char short int long longlong float double bool
						signed unsigned struct union enum-specifier typedef-name
			        	auto register static extern typedef	
				 */
	union {
		TYPE * decl_list; //record struct or union body
		DECL * user_type; //record user defined type
		ENUM * enum_type; //enumerator type
		STRUCT * struct_type; //structure type
		UNION * union_type; //union type
	} u1;
	TYPE * pfld[MAX_TYPE_FLD];

	TYPE() 
	{
		clean();
	}

	void clean()
	{
		t_des = 0;
		u1.decl_list = NULL;
		for (INT i = 0; i < MAX_TYPE_FLD; i++) {
			pfld[i] = NULL;
		}
	}

	void copy(TYPE & ty)
	{
		t_des = ty.t_des;
		u1.decl_list = ty.u1.decl_list;
		for (INT i = 0; i < MAX_TYPE_FLD; i++) {
			pfld[i] = ty.pfld[i];
		}
	}		
};



//
//START DECL
//
typedef enum {
	DCL_NULL = 0,
	DCL_ARRAY,     //array    declarator
	DCL_POINTER,   //pointer  declarator
	DCL_FUN,       //function declarator
	DCL_ID,        //identifier
	DCL_VARIABLE,  //variable length parameter, e.g: ...
	DCL_TYPE_NAME,   //if current decl is TYPE_NAME,  it descript a
					 //abstract type spec 
	DCL_DECLARATOR,  //declarator
	DCL_DECLARATION, //declaration
	DCL_ABS_DECLARATOR, //abstract declarator	
} DCL;

class DECL {
public:
	#ifdef _DEBUG_
	UINT unique_id;
	#endif
	DCL decl_type;
	DECL * prev;
	DECL * next;
	DECL * child;
	UINT lineno; //record line number of declaration.

	//record the num of fields while the base of DECL is STRUCT/UNION.
	UINT fieldno; 

	//memory alignment of declaration in the current context.
	UINT align; 
	TYPE * base_type_spec; //Record base type-spec of field.

	//1 indicates that the DECL enclosed with a pair of '(' ')'.
	BYTE is_paren:1; 
	BYTE is_bit_field:1; //descripte a bit field.

	//descripte a function's property: 1: definition, 0:declaration.
	BYTE is_fun_def:1;  
	BYTE is_init:1; //has a initializing expression.
	BYTE is_sub_field:1; //DECL is a sub field of struct/union.
	BYTE is_formal_para:1; //DECL is a formal parameter.

	struct {
		//declaration specifier
		//only locate on top of decl link list
		TYPE * specifier; 

		DECL * declarator_list; //declarator list 
		SCOPE * scope; //declaration reside in 
	} u0; //only for DCL_DECLARATION used
	TYPE * qualifier; //quanlifier for POINTER/ID		
	union {
		struct {
			union {
				ULONGLONG  index;
				TREE * index_exp;
			} u121;
			DECL * abase; 
		} u12; //only for DCL_ARRAY used

		struct {
			DECL * para_list;
			DECL * fbase;
		} u13; //only for DCL_FUN used

		union {
			/*
			Record number of bit which restricted by type-spec descriptor.
			This value was computed in compute_constant_exp().
			only be seen as a child of DCL_DECLARATOR.
			only be valid in struct-member declare.
			only was effective after 'bit_exp' has been computed 
			completely in typeck.cpp file.
			*/
			INT  bit_len;
		} u16;
		
		//Record a identifier, only used by DCL_ID.
		//TREE type should be defined relatively with specific language.		
		TREE * id;
	} u1;

	union {
		//Record a function body
		//ONLY record as a child of DCL_DECLARATION
		SCOPE * fun_body; 

		//Record an initializing tree
		//ONLY record as a child of DCL_DECLARATOR
		TREE * init; 
	} u2;
};

#ifdef _DEBUG_
#define DECL_uid(d)					(d)->unique_id
#endif
#define DECL_dt(d)					(d)->decl_type
#define DECL_next(d)				(d)->next
#define DECL_prev(d)				(d)->prev
#define DECL_is_paren(d)			(d)->is_paren
#define DECL_is_bit_field(d)		(d)->is_bit_field
#define DECL_is_sub_field(d)		(d)->is_sub_field
#define DECL_is_formal_para(d)		(d)->is_formal_para //ONLY used in DCL_DECLARATION
#define DECL_is_fun_def(d)			(d)->is_fun_def  //ONLY used in DCL_DECLARATION
#define DECL_is_init(d)				(d)->is_init     //ONLY used in DCL_DECLARATOR
#define DECL_fieldno(d)				(d)->fieldno
#define DECL_align(d)				(d)->align
#define DECL_base_type_spec(d)		(d)->base_type_spec

/*
If current 'decl' is DCL_DECLARATION, so it must has
'declaration specifier' and 'declarator list'
*/
#define DECL_spec(d)				(d)->u0.specifier
#define DECL_decl_list(d)			(d)->u0.declarator_list
#define DECL_decl_scope(d)			(d)->u0.scope

/*
If current 'decl' is a function define, the followed member
record its body
*/
#define DECL_fun_body(d)			(d)->u2.fun_body

/*
Record qualification of DCL.
If current 'decl' is DCL_POINTER or DCL_ID, the 
followed member record its quanlifier specicfier.
*/
#define DECL_qua(d)					(d)->qualifier

//Line number
#define DECL_lineno(d)				(d)->lineno

/*
If current 'decl' is a DCL_DECLARATOR, the followed member
record it initializing tree
*/
#define DECL_init_tree(d)			(d)->u2.init

//If current 'decl' is DCL_ID , the followed member record it
#define DECL_id(d)					(d)->u1.id

/*
If current 'decl' is DCL_ARRAY, the followed members record its
base and index value/expression, which may be NULL.
During the AST generation, DECL_array_index_exp() is avaiable, and 
the actually integer value is calculated after c_parse() finished.
*/
#define DECL_array_index(d)			(d)->u1.u12.u121.index
#define DECL_array_index_exp(d)		(d)->u1.u12.u121.index_exp

/*
If current 'decl' is DCL_FUN, the followed members record its
base and parameter list declaration, which may be NULL
*/
//#define DECL_fun_base(d)			(d)->u1.u13.fbase
#define DECL_fun_para_list(d)		(d)->u1.u13.para_list

//Record content if current 'decl' is DCL_DECLARATOR or DCL_ABS_DECLARATOR
#define DECL_child(d)				(d)->child

/*
Bit field ONLY used in DCL_DECLARATOR, and record in decl.cpp
checking in typeck.cpp
It will not be zero if current decl record a bit field
*/
#define DECL_bit_len(d)				(d)->u1.u16.bit_len
//#define DECL_bit_exp(d)			(d)->u1.u16.bit_exp

/*
'd' must be TYPE-NAME ,get pure declarator list 
The macro without validation check, plz call 
get_pure_declarator if you want to check.
*/
#define PURE_DECL(d)				DECL_child(DECL_decl_list(d))
#define MAX_ARRAY_INDX				0xfffffff
//END DECL


//Format DECL to dump
#define DECL_FMT_INDENT_INTERVAL 4


//Exported Functions
ENUM * add_to_enum_list(ENUM_LIST ** e_list , ENUM * e);
DECL * add_to_user_type_list(USER_TYPE_LIST ** ut_list , DECL * decl);

ULONGLONG compute_size_of_array(DECL * decl);
DECL * cp_type_name(DECL * src);
DECL * cp_decl_complete(DECL * src);
TYPE * cp_spec(TYPE * ty);
DECL * cp_decl(DECL * src);
DECL * cp_decl_begin_at(DECL * header);
INT compute_union_type_size(TYPE * ty);
INT compute_struct_type_size(TYPE * ty);

bool declaration();
bool declaration_list();
void dump_decl(DECL * dcl);
CHAR * dump_decl(DECL * dcl, CHAR * buf);
void dump_struct(STRUCT * s);
void dump_union(UNION * s);
void dump_type(TYPE * ty, DECL * decl);

//Dump C style type-info
INT format_dcrl_reverse(IN OUT CHAR buf[], IN DECL * decl);
INT format_struct_union(IN OUT CHAR buf[], IN TYPE * ty);
INT format_enum(IN OUT CHAR buf[], IN TYPE * ty);
INT format_enum_complete(IN OUT CHAR buf[], IN TYPE * ty);
INT format_enum_complete(IN OUT CHAR buf[], IN ENUM * e);
INT format_struct_union_complete(IN OUT CHAR buf[], IN TYPE * ty);
INT format_struct_complete(IN OUT CHAR buf[], IN STRUCT * s);
INT format_union_complete(IN OUT CHAR buf[], IN UNION * u);
INT format_decl_spec(IN OUT CHAR buf[], IN TYPE * ty, DECL * decl);
INT format_parameter_list(IN OUT CHAR buf[], IN DECL * decl);
INT format_user_type_spec(IN OUT CHAR buf[], IN TYPE * ty);
INT format_user_type_spec(IN OUT CHAR buf[], IN DECL * ut);
INT format_base_type_spec(IN OUT CHAR buf[], IN TYPE * ty);
INT format_stor_spec(IN OUT CHAR buf[], IN TYPE * ty);
INT format_declarator(IN OUT CHAR buf[], IN DECL * decl);
INT format_declaration(IN OUT CHAR buf[], IN DECL * decl);	

//Dump DECL-TREE style type-info
INT format_dcrl(IN DECL * decl, INT indent);
INT format_parameter_list(IN DECL * decl, INT indent);
INT format_user_type_spec(IN TYPE * ty, INT indent);
INT format_user_type_spec(IN DECL * ut, INT indent);
INT format_declarator(IN DECL * decl, INT indent);
INT format_declaration(IN DECL * decl, INT indent);

bool is_decl_exist_in_outer_scope(IN CHAR * name, OUT DECL ** dcl);
bool is_decl_equal(IN DECL * d1, IN DECL * d2);
bool is_abs_declaraotr(DECL * declarator);
bool is_user_type_decl(DECL * dcl);
bool is_user_type_ref(DECL * dcl);
bool is_struct_complete(IN TYPE * type_spec);
bool is_union_complete(IN TYPE * type_spec);
bool is_local_variable(DECL * dcl);
bool is_global_variable(DECL * dcl);
bool is_static(DECL * dcl);
bool is_constant(DECL * dcl);
bool is_volatile(DECL * dcl);
bool is_restrict(DECL * dcl);
bool is_initialized(DECL * dcl);
bool is_inline(DECL * dcl);
bool is_unique_decl(DECL * decl_list, DECL * decl);
bool is_declaration(DECL * decl);
bool is_simple_base_type(TYPE * ty);
bool is_simple_base_type(INT des);
bool is_complex_type(DECL * dcl);
bool is_array(DECL * dcl);
bool is_pointer(DECL * dcl);
bool is_fun_decl(DECL * dcl);
bool is_fun_void_return(DECL * dcl);
bool is_fun_pointer(DECL * dcl);
bool is_arith(DECL * dcl);
bool is_integer(DECL * dcl);
bool is_integer(TYPE * ty);
bool is_fp(DECL * dcl);
bool is_fp(TYPE * ty);
bool is_union_exist_in_outer_scope(IN CHAR * tag, OUT UNION ** s);
bool is_struct_type_exist_in_cur_scope(CHAR * tag, OUT STRUCT ** s);
bool is_struct_exist_in_outer_scope(IN CHAR * tag, OUT STRUCT ** s);
bool is_enum_id_exist_in_outer_scope(IN CHAR * cl, OUT ENUM ** e);
bool is_enum_const_exist_in_outer_scope(CHAR * cl, OUT ENUM ** e, OUT INT * idx);
bool is_enum_const_exist_in_cur_scope(IN CHAR * cl, OUT ENUM ** e, OUT INT * idx);
bool is_user_type_exist(USER_TYPE_LIST * ut_list, CHAR * ut_name	, DECL ** ut);
bool is_struct_type_exist(IN STRUCT * st_list, IN CHAR * tag, OUT STRUCT ** s);
bool is_union_type_exist(IN UNION * u_list, IN CHAR * tag, OUT UNION ** s);
bool is_union(TYPE * type);
bool is_union(DECL * decl);
bool is_struct(TYPE * type);
bool is_struct(DECL * decl);
bool is_bitfield(DECL * decl);

DECL * get_pure_declarator(DECL * decl);
DECL * get_parameter_list(DECL * dcl, OUT DECL ** fun_dclor = NULL);
DECL * get_decl_id(DECL * dcl);
SYM * get_decl_sym(DECL * dcl);
DECL * get_decl_in_scope(IN CHAR * name, SCOPE * scope);
TREE * get_decl_id_tree(DECL * dcl);
INT get_enum_val_idx(ENUM * e, CHAR * ev_name);
INT get_enum_const_val(ENUM * e, INT idx);
INT get_simply_type_size_in_byte(TYPE * ty);
ULONG get_complex_type_size_in_byte(DECL * decl);
INT get_decl_size(DECL * decl);
INT get_pointer_base_size(DECL * decl);
DECL * get_pointer_decl(DECL * decl);
DECL * get_pointer_base_decl(DECL * decl, TYPE ** ty);
TYPE * get_pure_type_spec(TYPE * type);
INT get_declarator_size_in_byte(DECL * d);
INT get_pointer_type_spec();
CHAR * get_enum_const_name(ENUM * e, INT idx);
UINT get_struct_field_ofst(STRUCT * st, CHAR * name);

DECL * new_decl(DCL dcl_type);
DECL * new_var_decl(IN SCOPE * scope, IN CHAR * name);
TYPE * new_type();
TYPE * new_type(INT cate);
ENUM * new_enum();

DECL * type_name();
DECL * trans_to_pointer(DECL * decl, bool is_append);


//Exported Variables
extern INT g_alignment;
extern CHAR * g_dcl_name[];
#endif

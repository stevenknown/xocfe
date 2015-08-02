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

#define EVAL_LIST_val(el)			((el)->val)
#define EVAL_LIST_name(el)			((el)->str)
#define EVAL_LIST_next(el)			((el)->next)
#define EVAL_LIST_prev(el)			((el)->prev)
class EnumValueList {
public:
	INT val;
	SYM	* str;
	EnumValueList * next;
	EnumValueList * prev;
};


/*
Enum

Record Enum info, and field 'name' reserved its character description.
*/
#define ENUM_name(e)				((e)->name)
#define ENUM_vallist(e)				((e)->pevlist)
class Enum {
public:
	INT val;
	SYM * name;
	EnumValueList * pevlist;
};


//EnumList
#define ENUM_LIST_enum(el)			((el)->e)
#define ENUM_LIST_next(el)			((el)->next)
#define ENUM_LIST_prev(el)			((el)->prev)
class EnumList {
public:
	EnumList * next;
	EnumList * prev;
	Enum * e;
};


//UserTypeList record user defined type with 'typedef'
#define USER_TYPE_LIST_utype(utl)	((utl)->ut)
#define USER_TYPE_LIST_next(utl)	((utl)->next)
#define USER_TYPE_LIST_prev(utl)	((utl)->prev)
class UserTypeList {
public:
	UserTypeList * next;
	UserTypeList * prev;
	Decl * ut;
};


//Struct
#define STRUCT_decl_list(s)			((s)->m_decl_list)
#define STRUCT_tag(s)				((s)->tag)
#define STRUCT_is_complete(s)		((s)->is_complete)
#define STRUCT_align(s)				((s)->align)
class Struct {
public:	
	Decl * m_decl_list;
	SYM * tag;
	bool is_complete;
	UINT align;
};


//Union
#define UNION_decl_list(s)			((s)->m_decl_list)
#define UNION_tag(s)				((s)->tag)
#define UNION_is_complete(s)		((s)->is_complete)
#define UNION_align(s)				((s)->align)
class Union {
public:
	Decl * m_decl_list;
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
#define T_SPEC_ENUM				(0x4000)
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
#define TYPE_des(t)				((t)->m_descriptor)
#define TYPE_parent(t)			((t)->m_sub_field[0])
#define TYPE_next(t)			((t)->m_sub_field[1])
#define TYPE_prev(t)			((t)->m_sub_field[2])
#define TYPE_user_type(t)		((t)->u1.m_user_type) //user type define
#define TYPE_enum_type(t)		((t)->u1.m_enum_type) //enum type define
#define TYPE_struct_type(t)		((t)->u1.m_struct_type) //struct type define
#define TYPE_union_type(t)		((t)->u1.m_union_type) //union type define

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
#define IS_ENUM_TYPE(t)			HAVE_FLAG(TYPE_des(t), T_SPEC_ENUM)
#define IS_DOUBLE(t)			HAVE_FLAG(t, T_SPEC_DOUBLE)
#define IS_FLOAT(t)				HAVE_FLAG(t, T_SPEC_FLOAT)

//The type specifier represent a variable or type-name via user type.
#define IS_USER_TYPE_REF(t)		HAVE_FLAG(TYPE_des(t), T_SPEC_USER_TYPE)

//The declaration defined a user type via 'typedef' operator.
#define IS_TYPEDEF(t)			HAVE_FLAG(TYPE_des(t), T_STOR_TYPEDEF)

/* This class represent memory location modifier, basic and user defined type, 
such as: const, volatile, void, char, short, int, long, longlong, float, 
	double, bool, signed, unsigned, struct, union, enum-specifier, 
	typedef-name, auto, register, static, extern, typedef. */
class TypeSpec {
public:	
	ULONG m_descriptor;

	union {
		TypeSpec * m_decl_list; //record struct or union body
		Decl * m_user_type; //record user defined type
		Enum * m_enum_type; //enumerator type
		Struct * m_struct_type; //structure type
		Union * m_union_type; //union type
	} u1;

	TypeSpec * m_sub_field[MAX_TYPE_FLD];
	
public:
	TypeSpec() { clean(); }

	void clean()
	{
		m_descriptor = 0;
		u1.m_decl_list = NULL;
		for (INT i = 0; i < MAX_TYPE_FLD; i++) {
			m_sub_field[i] = NULL;
		}
	}

	void copy(TypeSpec const& ty)
	{
		m_descriptor = ty.m_descriptor;
		u1.m_decl_list = ty.u1.m_decl_list;
		for (UINT i = 0; i < MAX_TYPE_FLD; i++) {
			m_sub_field[i] = ty.m_sub_field[i];
		}
	}
};


//
//START Decl
//

//Define type of Decl.
typedef enum {
	DCL_NULL = 0,
	DCL_ARRAY,          //array    declarator
	DCL_POINTER,        //pointer  declarator
	DCL_FUN,            //function declarator
	DCL_ID,             //identifier
	DCL_VARIABLE,       //variable length parameter, e.g: ...
	DCL_TYPE_NAME,      //if current decl is TYPE_NAME,  it descript a
					    //abstract type spec
	DCL_DECLARATOR,     //declarator
	DCL_DECLARATION,    //declaration
	DCL_ABS_DECLARATOR, //abstract declarator
} DCL;

class Decl {
public:
	#ifdef _DEBUG_
	UINT id;
	#endif
	DCL decl_type;
	Decl * prev;
	Decl * next;
	Decl * child;
	UINT lineno; //record line number of declaration.

	//record the num of fields while the base of Decl is Struct/Union.
	UINT fieldno;

	//memory alignment of declaration in the current context.
	UINT align;
	TypeSpec * base_type_spec; //Record base type-spec of field.

	//1 indicates that the Decl enclosed with a pair of '(' ')'.
	BYTE is_paren:1;
	BYTE is_bit_field:1; //descripte a bit field.

	//descripte a function's property: 1: definition, 0:declaration.
	BYTE is_fun_def:1;
	BYTE is_init:1; //has a initializing expression.
	BYTE is_sub_field:1; //Decl is a sub field of struct/union.
	BYTE is_formal_para:1; //Decl is a formal parameter.

	struct {
		//declaration specifier
		//only locate on top of decl link list
		TypeSpec * specifier;

		Decl * declarator_list; //declarator list
		SCOPE * scope; //declaration reside in
	} u0; //only for DCL_DECLARATION used

	TypeSpec * qualifier; //quanlifier for POINTER/ID

	union {
		struct {
			union {
				ULONGLONG dimval;
				Tree * dimexp;
			} u121;
			Decl * abase;
		} u12; //only for DCL_ARRAY used

		struct {
			Decl * para_list;
			Decl * fbase;
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

		//Record an identifier, only used by DCL_ID.
		//Tree type is closely related to specific language.
		Tree * id;
	} u1;

	union {
		//Record a function body
		//ONLY record as a child of DCL_DECLARATION
		SCOPE * fun_body;

		//Record an initializing tree
		//ONLY record as a child of DCL_DECLARATOR
		Tree * init;
	} u2;
};

#ifdef _DEBUG_
#define DECL_uid(d)					(d)->id
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
'declaration specifier' and 'declarator list'.
Specifier describe the basic type such as int, char, struct/union.
Declarator describe the compound type such as pointer, array, function, etc.
*/
#define DECL_spec(d)				(d)->u0.specifier
#define DECL_decl_list(d)			(d)->u0.declarator_list
#define DECL_decl_scope(d)			(d)->u0.scope


//If current 'decl' is a function define, the followed member record its body
#define DECL_fun_body(d)			(d)->u2.fun_body

/*
Record qualification of DCL.
If current 'decl' is DCL_POINTER or DCL_ID, the
followed member record its quanlifier specicfier.
qualifier include const, volatile, restrict.
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

NOTE: During the Decl generation processing, DECL_array_dim_exp() is avaiable,
it records the expressions of dimension,
and the actually dimension value will be calculated after parsing array type
finished, and compute_array_dim() will be invoked to compute the integer value.
Meanwile, DECL_array_dim() is avaiable.
*/
#define DECL_array_dim(d)			(d)->u1.u12.u121.dimval
#define DECL_array_dim_exp(d)		(d)->u1.u12.u121.dimexp

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
'd' must be TypeSpec-NAME ,get pure declarator list
The macro without validation check, plz call
get_pure_declarator if you want to check.
*/
#define PURE_DECL(d)				DECL_child(DECL_decl_list(d))
#define MAX_ARRAY_INDX				0xfffffff
//END Decl


//Format Decl to dump
#define DECL_FMT_INDENT_INTERVAL 4


//Exported Functions
Decl * add_to_user_type_list(UserTypeList ** ut_list , Decl * decl);

ULONGLONG compute_size_of_array(Decl * decl);
Decl * cp_type_name(Decl * src);
Decl * cp_decl_complete(Decl * src);
TypeSpec * cp_spec(TypeSpec * ty);
Decl * cp_decl(Decl * src);
Decl * cp_decl_begin_at(Decl * header);
INT compute_union_type_size(TypeSpec * ty);
INT compute_struct_type_size(TypeSpec * ty);

bool declaration();
bool declaration_list();
void dump_decl(Decl * dcl);
CHAR * dump_decl(Decl * dcl, CHAR * buf);
void dump_struct(Struct * s);
void dump_union(Union * s);
void dump_type(TypeSpec * ty, bool is_ptr);

Decl * expand_user_type(Decl * ut);

//Dump C style type-info
INT format_dcrl_reverse(IN OUT CHAR buf[], IN Decl * decl);
INT format_struct_union(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_enum(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_enum_complete(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_enum_complete(IN OUT CHAR buf[], IN Enum * e);
INT format_struct_union_complete(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_struct_complete(IN OUT CHAR buf[], IN Struct * s);
INT format_union_complete(IN OUT CHAR buf[], IN Union * u);
INT format_decl_spec(IN OUT CHAR buf[], IN TypeSpec * ty, bool is_ptr);
INT format_parameter_list(IN OUT CHAR buf[], IN Decl * decl);
INT format_user_type_spec(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_user_type_spec(IN OUT CHAR buf[], IN Decl * ut);
INT format_base_type_spec(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_stor_spec(IN OUT CHAR buf[], IN TypeSpec * ty);
INT format_declarator(IN OUT CHAR buf[], IN Decl * decl);
INT format_declaration(IN OUT CHAR buf[], IN Decl * decl);

//Dump Decl-Tree style type-info
INT format_dcrl(IN Decl * decl, INT indent);
INT format_parameter_list(IN Decl * decl, INT indent);
INT format_user_type_spec(IN TypeSpec * ty, INT indent);
INT format_user_type_spec(IN Decl * ut, INT indent);
INT format_declarator(IN Decl * decl, INT indent);
INT format_declaration(IN Decl * decl, INT indent);
Decl * factor_user_type(Decl * decl);
Enum * find_enum(EnumList * elst , Enum * e);

bool is_decl_exist_in_outer_scope(IN CHAR * name, OUT Decl ** dcl);
bool is_decl_equal(IN Decl * d1, IN Decl * d2);
bool is_abs_declaraotr(Decl * declarator);
bool is_user_type_decl(Decl * dcl);
bool is_user_type_ref(Decl * dcl);
bool is_struct_complete(TypeSpec const* type_spec);
bool is_union_complete(TypeSpec const* type_spec);
bool is_local_variable(Decl * dcl);
bool is_global_variable(Decl * dcl);
bool is_static(Decl * dcl);
bool is_constant(Decl * dcl);
bool is_volatile(Decl * dcl);
bool is_restrict(Decl * dcl);
bool is_initialized(Decl * dcl);
bool is_inline(Decl * dcl);
bool is_unique_decl(Decl * m_decl_list, Decl * decl);
bool is_declaration(Decl * decl);
bool is_simple_base_type(TypeSpec * ty);
bool is_simple_base_type(INT des);
bool is_complex_type(Decl * dcl);
bool is_array(Decl * dcl);
bool is_pointer(Decl * dcl);
bool is_fun_decl(Decl * dcl);
bool is_fun_void_return(Decl * dcl);
bool is_fun_pointer(Decl * dcl);
bool is_arith(Decl * dcl);
bool is_integer(Decl * dcl);
bool is_integer(TypeSpec * ty);
bool is_fp(Decl * dcl);
bool is_fp(TypeSpec * ty);
bool is_union_exist_in_outer_scope(IN CHAR * tag, OUT Union ** s);
bool is_struct_type_exist_in_cur_scope(CHAR * tag, OUT Struct ** s);
bool is_struct_exist_in_outer_scope(IN CHAR * tag, OUT Struct ** s);
bool is_enum_id_exist_in_outer_scope(IN CHAR * cl, OUT Enum ** e);
bool is_enum_const_exist_in_outer_scope(CHAR * name, OUT Enum ** e,
										OUT INT * idx);
bool is_enum_const_exist_in_cur_scope(IN CHAR * cl, OUT Enum ** e,
									  OUT INT * idx);
bool is_user_type_exist(UserTypeList * ut_list, CHAR * ut_name, Decl ** ut);
bool is_struct_type_exist(List<Struct*> & struct_list, IN CHAR * tag, 
							OUT Struct ** s);
bool is_union_type_exist(List<Union*> & u_list, IN CHAR * tag, OUT Union ** s);
bool is_union(TypeSpec * type);
bool is_union(Decl * decl);
bool is_struct(TypeSpec * type);
bool is_struct(Decl * decl);
bool is_bitfield(Decl * decl);

Decl * get_pure_declarator(Decl * decl);
Decl * get_parameter_list(Decl * dcl, OUT Decl ** fun_dclor = NULL);
Decl * get_decl_id(Decl * dcl);
SYM * get_decl_sym(Decl * dcl);
Decl * get_decl_in_scope(IN CHAR * name, SCOPE * scope);
Tree * get_decl_id_tree(Decl * dcl);
INT get_enum_val_idx(Enum * e, CHAR * ev_name);
INT get_enum_const_val(Enum * e, INT idx);
INT get_simply_type_size_in_byte(TypeSpec * ty);
ULONG get_complex_type_size_in_byte(Decl * decl);
INT get_decl_size(Decl * decl);
UINT get_pointer_base_size(Decl * decl);
Decl * get_pointer_decl(Decl * decl);
Decl * get_array_decl(Decl * decl);
ULONG get_array_elemnum_to_dim(Decl * arr, UINT dim);
Decl * get_pointer_base_decl(Decl * decl, TypeSpec ** ty);
TypeSpec * get_pure_type_spec(TypeSpec * type);
INT get_declarator_size_in_byte(Decl * d);
CHAR * get_enum_const_name(Enum * e, INT idx);
UINT get_struct_field_ofst(Struct * st, CHAR * name);

Decl * new_declaration(TypeSpec * spec, Decl * declor, SCOPE * sc);
Decl * new_decl(DCL dcl_type);
Decl * new_var_decl(IN SCOPE * scope, IN CHAR * name);
TypeSpec * new_type();
TypeSpec * new_type(INT cate);
Enum * new_enum();

Decl * type_name();
Decl * trans_to_pointer(Decl * decl, bool is_append);


//Exported Variables
extern INT g_alignment;
extern CHAR * g_dcl_name[];
#endif

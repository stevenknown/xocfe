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

class Scope;

#define EVAL_LIST_val(el) ((el)->val)
#define EVAL_LIST_name(el) ((el)->str)
#define EVAL_LIST_next(el) ((el)->next)
#define EVAL_LIST_prev(el) ((el)->prev)
class EnumValueList {
public:
    INT val;
    Sym * str;
    EnumValueList * next;
    EnumValueList * prev;
};


//Enum
//Record Enum info, and field 'name' reserved its character description.
#define ENUM_name(e) ((e)->name)
#define ENUM_vallist(e) ((e)->pevlist)
class Enum {
public:
    INT val;
    Sym * name;
    EnumValueList * pevlist;
};


//EnumList
#define ENUM_LIST_enum(el) ((el)->e)
#define ENUM_LIST_next(el) ((el)->next)
#define ENUM_LIST_prev(el) ((el)->prev)
class EnumList {
public:
    EnumList * next;
    EnumList * prev;
    Enum * e;
};


//UserTypeList record user defined type with 'typedef'
#define USER_TYPE_LIST_utype(utl) ((utl)->ut)
#define USER_TYPE_LIST_next(utl) ((utl)->next)
#define USER_TYPE_LIST_prev(utl) ((utl)->prev)
class UserTypeList {
public:
    UserTypeList * next;
    UserTypeList * prev;
    Decl * ut;
};


//Aggregation
#define AGGR_decl_list(s) ((s)->m_decl_list)
#define AGGR_is_complete(s) ((s)->is_complete)
#define AGGR_tag(s) ((s)->tag)
#define AGGR_align(s) ((s)->align)
#define AGGR_field_align(s) ((s)->field_align)
#define AGGR_pack_align(s) ((s)->pack_align)
#define AGGR_scope(s) ((s)->scope)
class Aggr {
public:
    bool is_complete;
    Decl * m_decl_list;
    Sym * tag;
    UINT align; //alignment that whole structure have to align.
    UINT field_align; //alignment that each fields in structure have to align.
                      //0 indicates there is requirement to field align.
    UINT pack_align; //User declared field alignment.
    Scope * scope;
};


//Struct
#define STRUCT_decl_list(s) AGGR_decl_list(s)
#define STRUCT_tag(s) AGGR_tag(s)
#define STRUCT_is_complete(s) AGGR_is_complete(s)
#define STRUCT_align(s) AGGR_align(s)
#define STRUCT_scope(s) AGGR_scope(s)
class Struct : public Aggr {
public:
};


//Union
#define UNION_decl_list(s) AGGR_decl_list(s)
#define UNION_tag(s) AGGR_tag(s)
#define UNION_is_complete(s) AGGR_is_complete(s)
#define UNION_align(s) AGGR_align(s)
#define UNION_scope(s) AGGR_scope(s)
class Union : public Aggr {
public:
};


//Qualifier
#define T_QUA_CONST (0x1)
#define T_QUA_VOLATILE (0x2)
#define T_QUA_RESTRICT (0x4)

//Specifier
#define T_SPEC_VOID (0x8)
#define T_SPEC_CHAR (0x10)
#define T_SPEC_SHORT (0x20)
#define T_SPEC_INT (0x40)
#define T_SPEC_LONG (0x80)
#define T_SPEC_FLOAT (0x100)
#define T_SPEC_DOUBLE (0x200)
#define T_SPEC_SIGNED (0x400)
#define T_SPEC_UNSIGNED (0x800)
#define T_SPEC_STRUCT (0x1000)
#define T_SPEC_UNION (0x2000)
#define T_SPEC_ENUM (0x4000)
#define T_SPEC_USER_TYPE (0x8000)
#define T_SPEC_LONGLONG (0x10000)
#define T_SPEC_BOOL (0x20000)

//Storage type
#define T_STOR_AUTO (0x40000)
#define T_STOR_REG (0x80000)
#define T_STOR_STATIC (0x100000)
#define T_STOR_EXTERN (0x200000)
#define T_STOR_TYPEDEF (0x400000)
#define T_STOR_INLINE (0x800000)


#define MAX_TYPE_FLD 3
#define TYPE_des(t) ((t)->m_descriptor)
#define TYPE_parent(t) ((t)->m_sub_field[0])
#define TYPE_next(t) ((t)->m_sub_field[1])
#define TYPE_prev(t) ((t)->m_sub_field[2])
#define TYPE_user_type(t) ((t)->u1.m_user_type) //user type define
#define TYPE_enum_type(t) ((t)->u1.m_enum_type) //enum type define
//struct type define
#define TYPE_struct_type(t) ((t)->u1.m_struct_type)
//union type define
#define TYPE_union_type(t) ((t)->u1.m_union_type)
#define TYPE_aggr_type(t) ((t)->u1.m_aggr_type)

#define IS_CONST(t) HAVE_FLAG(TYPE_des(t), T_QUA_CONST)
#define IS_VOLATILE(t) HAVE_FLAG(TYPE_des(t), T_QUA_VOLATILE)
#define IS_RESTRICT(t) HAVE_FLAG(TYPE_des(t), T_QUA_RESTRICT)
#define IS_AGGR(t) (IS_STRUCT(t) || IS_UNION(t))
#define IS_STRUCT(t) HAVE_FLAG(TYPE_des(t), T_SPEC_STRUCT)
#define IS_UNION(t) HAVE_FLAG(TYPE_des(t), T_SPEC_UNION)
#define IS_TYPE(t,T) HAVE_FLAG(TYPE_des(t), T)
#define IS_TYPED(des,T) HAVE_FLAG((des), T)
#define IS_REG(t) HAVE_FLAG(TYPE_des(t), T_STOR_REG)
#define IS_STATIC(t) HAVE_FLAG(TYPE_des(t), T_STOR_STATIC)
#define IS_EXTERN(t) HAVE_FLAG(TYPE_des(t), T_STOR_EXTERN)
#define IS_INLINE(t) HAVE_FLAG(TYPE_des(t), T_STOR_INLINE)
#define IS_ENUM_TYPE(t) HAVE_FLAG(TYPE_des(t), T_SPEC_ENUM)
#define IS_DOUBLE(t) HAVE_FLAG(t, T_SPEC_DOUBLE)
#define IS_FLOAT(t) HAVE_FLAG(t, T_SPEC_FLOAT)

//The type specifier represent a variable or type-name via user type.
#define IS_USER_TYPE_REF(t) HAVE_FLAG(TYPE_des(t), T_SPEC_USER_TYPE)

//The declaration defined a user type via 'typedef' operator.
#define IS_TYPEDEF(t) HAVE_FLAG(TYPE_des(t), T_STOR_TYPEDEF)

//This class represent memory location modifier, basic and user defined type,
//such as: const, volatile, void, char, short, int, long, longlong, float,
//double, bool, signed, unsigned, struct, union, enum-specifier,
//typedef-name, auto, register, static, extern, typedef.
class TypeSpec {
public:
    ULONG m_descriptor;
    union {
        TypeSpec * m_decl_list; //record struct or union body
        Decl * m_user_type; //record user defined type
        Enum * m_enum_type; //enumerator type
        Struct * m_struct_type; //struct type
        Union * m_union_type; //union type
        Aggr * m_aggr_type; //aggregation type
    } u1;
    TypeSpec * m_sub_field[MAX_TYPE_FLD];
public:
    TypeSpec() { clean(); }

    void clean()
    {
        m_descriptor = 0;
        u1.m_decl_list = nullptr;
        for (INT i = 0; i < MAX_TYPE_FLD; i++) {
            m_sub_field[i] = nullptr;
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
    DCL_ARRAY, //array declarator
    DCL_POINTER, //pointer declarator
    DCL_FUN, //function declarator
    DCL_ID, //identifier
    DCL_VARIABLE, //variable length parameter, e.g: ...
    DCL_TYPE_NAME, //if current decl is TYPE_NAME,  it descript a
                   //abstract type spec
    DCL_DECLARATOR, //declarator
    DCL_DECLARATION, //declaration
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
    Tree * placeholder; //record the placeholder in stmt list of scope.
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
        Scope * scope; //declaration reside in
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
            //Record number of bit which restricted by type-spec descriptor.
            //This value was computed in computeConstExp().
            //only be seen as a child of DCL_DECLARATOR.
            //only be valid in struct-member declare.
            //only was effective after 'bit_exp' has been computed
            //completely in typeck.cpp file.
            INT  bit_len;
        } u16;

        //Record an identifier, only used by DCL_ID.
        //Tree type is closely related to specific language.
        Tree * id;

        //Record the formal parameter position in parameter-list.
        //Used if Decl is formal parameter.
        UINT formal_param_pos;
    } u1;

    union {
        //Record a function body
        //ONLY record as a child of DCL_DECLARATION
        Scope * fun_body;

        //Record an initializing tree
        //ONLY record as a child of DCL_DECLARATOR
        Tree * init;
    } u2;
};

#ifdef _DEBUG_
#define DECL_uid(d) (d)->id
#endif
//ONLY used in DCL_DECLARATION
#define DECL_is_formal_para(d) (d)->is_formal_para
#define DECL_dt(d) (d)->decl_type
#define DECL_next(d) (d)->next
#define DECL_prev(d) (d)->prev
#define DECL_is_paren(d) (d)->is_paren
#define DECL_is_bit_field(d) (d)->is_bit_field
#define DECL_is_sub_field(d) (d)->is_sub_field
#define DECL_is_fun_def(d) (d)->is_fun_def //ONLY used in DCL_DECLARATION
#define DECL_is_init(d) (d)->is_init //ONLY used in DCL_DECLARATOR
#define DECL_fieldno(d) (d)->fieldno
#define DECL_align(d) (d)->align
#define DECL_base_type_spec(d) (d)->base_type_spec

//If current 'decl' is DCL_DECLARATION, so it must has
//'declaration specifier' and 'declarator list'.
//Specifier describe the basic type such as int, char, struct/union.
//Declarator describe the compound type such as pointer, array, function, etc.
#define DECL_spec(d) (d)->u0.specifier
#define DECL_decl_list(d) (d)->u0.declarator_list
#define DECL_decl_scope(d) (d)->u0.scope

//If current 'decl' is a function define, the followed member record its body.
#define DECL_fun_body(d) (d)->u2.fun_body

//Record the formal parameter position if Decl is a parameter.
#define DECL_formal_param_pos(d) (d)->u1.formal_param_pos

//Record qualification of DCL.
//If current 'decl' is DCL_POINTER or DCL_ID, the
//followed member record its quanlifier specicfier.
//qualifier include const, volatile, restrict.
#define DECL_qua(d) (d)->qualifier

//Line number
#define DECL_lineno(d) (d)->lineno

//If current 'decl' is a DCL_DECLARATOR, the followed member
//record it initializing tree
#define DECL_init_tree(d) (d)->u2.init

//If current 'decl' is DCL_ID , the followed member record it
#define DECL_id(d) (d)->u1.id

//If current 'decl' is DCL_ARRAY, the followed members record its
//base and index value/expression, which may be nullptr.
//
//NOTE: During the Decl generation processing, DECL_array_dim_exp() is avaiable,
//it records the expressions of dimension,
//and the actually dimension value will be calculated after parsing array type
//finished, and compute_array_dim() will be invoked to compute the integer value.
//Meanwile, DECL_array_dim() is avaiable.
#define DECL_array_dim(d) (d)->u1.u12.u121.dimval
#define DECL_array_dim_exp(d) (d)->u1.u12.u121.dimexp

//If current 'decl' is DCL_FUN, the followed members record its
//base and parameter list declaration, which may be nullptr
//#define DECL_fun_base(d) (d)->u1.u13.fbase
#define DECL_fun_para_list(d) (d)->u1.u13.para_list

//Record content if current 'decl' is DCL_DECLARATOR or DCL_ABS_DECLARATOR
#define DECL_child(d) (d)->child

//Bit field ONLY used in DCL_DECLARATOR, and record in decl.cpp
//checking in typeck.cpp
//It will not be zero if current decl record a bit field
#define DECL_bit_len(d) (d)->u1.u16.bit_len
//#define DECL_bit_exp(d) (d)->u1.u16.bit_exp

//Record the placeholder in stmt list of scope.
//The placeholder is used to mark the lexicographical order of declarataion.
#define  DECL_placeholder(d) (d)->placeholder

//'d' must be TypeSpec-NAME, get pure declarator list
//The macro without validation check, plz call
//get_pure_declarator if you want to check.
#define PURE_DECL(d) DECL_child(DECL_decl_list(d))
#define MAX_ARRAY_INDX 0xfffffff
//END Decl

//Format Decl to dump
#define DECL_FMT_INDENT_INTERVAL 4

//Exported Functions
Decl * addToUserTypeList(UserTypeList ** ut_list , Decl * decl);

//Copy Decl of src is DCL_TYPE_NAME, or generate TYPE_NAME accroding
//to src information.
Decl * cp_typename(Decl const* src);
Decl * cp_type_name(Decl const* src);
TypeSpec * cp_spec(TypeSpec * ty);

//Copy whole Decl, include all its specifier, qualifier, and declarator.
Decl * cp_decl_fully(Decl const* src);

//Only copy 'src', excepting its field.
Decl * cp_decl(Decl const* src);

//Duplication declarator list begin at 'header'
Decl * cp_decl_begin_at(Decl const* header);

UINT computeUnionTypeSize(TypeSpec * ty);
UINT computeStructTypeSize(TypeSpec * ty);
UINT computeScalarTypeBitSize(UINT des);

Tree * declaration();
Tree * declaration_list();
void dump_decl(Decl const* dcl);
void dump_decl(Decl const* dcl, StrBuf & buf);

Decl * expand_user_type(Decl * ut);

//Dump C style type-info
INT format_enum_complete(StrBuf & buf, Enum const* e);
INT format_struct_union_complete(StrBuf & buf, TypeSpec const* ty);
INT format_struct_complete(StrBuf & buf, Struct const* s);
INT format_union_complete(StrBuf & buf, Union const* u);
INT format_decl_spec(StrBuf & buf, TypeSpec const* ty, bool is_ptr);
INT format_parameter_list(StrBuf & buf, Decl const* decl);
INT format_user_type_spec(StrBuf & buf, TypeSpec const* ty);
INT format_user_type_spec(StrBuf & buf, Decl const* ut);
INT format_declarator(StrBuf & buf, TypeSpec const* ty, Decl const* decl);
INT format_declaration(StrBuf & buf, Decl const* decl);

//Dump Decl-Tree style type-info
INT format_dcrl(Decl const* decl, INT indent);
INT format_parameter_list(Decl const* decl, INT indent);
INT format_user_type_spec(TypeSpec const* ty, INT indent);
INT format_user_type_spec(Decl const* ut, INT indent);
INT format_declarator(IN Decl const* decl, TypeSpec const* ty, INT indent);
INT format_declaration(IN Decl const* decl, INT indent);
Decl * factor_user_type(Decl * decl);
Enum * find_enum(EnumList * elst , Enum * e);
bool findEnumConst(CHAR const* name, OUT Enum ** e, OUT INT * idx);

bool is_extern(Decl const*dcl);
bool is_decl_exist_in_outer_scope(CHAR const* name, OUT Decl ** dcl);
bool is_decl_equal(Decl const* d1, Decl const* d2);
bool is_abs_declaraotr(Decl const* declarator);
bool is_user_type_decl(Decl const* dcl);
bool is_user_type_ref(Decl const* dcl);
//Return true if aggregation definition is complete.
bool is_aggr_complete(TypeSpec const* type);
bool is_struct_complete(TypeSpec const* type_spec);
bool is_union_complete(TypeSpec const* type_spec);
bool is_local_variable(Decl const* dcl);
bool is_global_variable(Decl const* dcl);
bool is_static(Decl const* dcl);
bool is_constant(Decl const* dcl);
bool is_volatile(Decl const* dcl);
bool is_restrict(Decl const* dcl);
bool is_initialized(Decl const* dcl);
bool is_inline(Decl const* dcl);
bool is_unique_decl(Decl const* m_decl_list, Decl const* decl);
bool is_declaration(Decl const* decl);
bool is_simple_base_type(TypeSpec const* ty);
bool is_simple_base_type(INT des);
bool is_complex_type(Decl const* dcl);
bool is_array(Decl const* dcl);
bool is_pointer(Decl const* dcl);
bool is_pointer_point_to_array(Decl const* decl);
bool is_any(Decl const* dcl);
bool is_fun_decl(Decl const* dcl);
bool is_fun_void_return(Decl const* dcl);
bool is_fun_pointer(Decl const* dcl);
bool is_scalar(Decl const* dcl);
bool is_arith(Decl const* dcl);
bool is_integer(Decl const* dcl);
bool is_integer(TypeSpec const* ty);
bool is_fp(Decl const* dcl);
bool is_fp(TypeSpec const* ty);
bool is_float(Decl const* dcl);
bool is_double(Decl const* dcl);
bool is_bool(TypeSpec const* ty);
bool is_bool(Decl const* dcl);
bool is_union_exist_in_outer_scope(Scope * scope,
                                   CHAR const* tag,
                                   OUT Union ** s);
//Return true if the union typed declaration have already existed in both
//current and all of outer scopes.
bool is_union_exist_in_outer_scope(Scope * scope,
                                   Sym const* tag,
                                   OUT Union ** s);
bool is_struct_type_exist_in_cur_scope(CHAR const* tag, OUT Struct ** s);
//Return true if the struct typed declaration have already existed in both
//current and all of outer scopes.
bool is_aggr_exist_in_outer_scope(Scope * scope,
                                  Sym const* tag,
                                  TypeSpec const* spec,
                                  OUT Aggr ** s);
bool is_aggr_exist_in_outer_scope(Scope * scope,
                                  CHAR const* tag,
                                  TypeSpec const* spec,
                                  OUT Aggr ** s);
bool is_struct_exist_in_outer_scope(Scope * scope,
                                    Sym const* tag,
                                    OUT Struct ** s);
bool is_struct_exist_in_outer_scope(Scope * scope,
                                    CHAR const* tag,
                                    OUT Struct ** s);
bool is_enum_id_exist_in_outer_scope(CHAR const* cl, OUT Enum ** e);

//Return true if enum-value existed in current scope.
bool is_enum_exist(EnumList const* e_list,
                   CHAR const* e_name,
                   OUT Enum ** e,
                   OUT INT * idx);
bool is_user_type_exist(UserTypeList const* ut_list, CHAR const* ut_name,
                        Decl ** ut);
bool is_struct_type_exist(List<Struct*> const& struct_list,
                          Sym const* tag,
                          OUT Struct ** s);
bool is_struct_type_exist(List<Struct*> const& struct_list,
                          CHAR const* tag,
                          OUT Struct ** s);
bool is_union_type_exist(List<Union*> const& u_list,
                         CHAR const* tag,
                         OUT Union ** s);
//Seach Union list accroding to the 'tag' of union-type.
bool is_union_type_exist(List<Union*> const& u_list,
                         Sym const* tag,
                         OUT Union ** u);
bool is_aggr(TypeSpec const* type);
bool is_aggr(Decl const* decl);
bool is_union(TypeSpec const* type);
bool is_union(Decl const* decl);
bool is_struct(TypeSpec const* type);
bool is_struct(Decl const* decl);
bool is_bitfield(Decl const* decl);

Tree * get_decl_init_tree(Decl const* dcl);
CHAR const* get_aggr_type_name(TypeSpec const* type);
Decl const* gen_type_name(TypeSpec * ty);
Decl const* get_return_type(Decl const* dcl);
CHAR const* get_decl_name(Decl * dcl);
Sym * get_decl_sym(Decl const* dcl);
Decl const* get_pure_declarator(Decl const* decl);
Decl * get_parameter_list(Decl * dcl, OUT Decl ** fun_dclor = nullptr);
Decl const* get_decl_id(Decl const* dcl);
Decl * get_decl_in_scope(CHAR const* name, Scope const* scope);
Tree * get_decl_id_tree(Decl const* dcl);
INT get_enum_const_val(Enum const* e, INT idx);
UINT getSpecTypeSize(TypeSpec const* ty);
ULONG getComplexTypeSize(Decl const* decl);
UINT get_decl_size(Decl const* decl);
UINT get_pointer_base_size(Decl const* decl);
Decl const* get_pointer_declarator(Decl const* decl);
Decl const* get_pointer_decl(Decl const* decl);
Decl * get_first_array_decl(Decl * decl);
Decl const* get_array_base_declarator(Decl const* dcl);
Decl * get_array_base_decl(Decl const* decl);
Decl * get_array_elem_decl(Decl const* decl);
ULONG get_array_elemnum_to_dim(Decl const* arr, UINT dim);
ULONG get_array_elemnum(Decl const* arr, UINT dim);
ULONG get_array_elem_bytesize(Decl const* arr);
UINT get_array_dim(Decl const* arr);
Decl * get_pointer_base_decl(Decl const* decl, TypeSpec ** ty);
TypeSpec * get_pure_type_spec(TypeSpec * type);
CHAR const* get_enum_const_name(Enum const* e, INT idx);
UINT get_aggr_field(Aggr const* st, CHAR const* name, Decl ** fld_decl);
UINT get_aggr_field(Aggr const* st, INT idx, Decl ** fld_decl);
Struct * get_struct_spec(Decl const* decl);
Union * get_union_spec(Decl const* decl);
Aggr * get_aggr_spec(Decl const* decl);
TypeSpec const* get_decl_spec(Decl const* decl);

Decl * new_declaration(TypeSpec * spec, Decl * declor, Scope * sc,
                       Tree * inittree);
Decl * new_decl(DCL dcl_type);
Decl * new_var_decl(IN Scope * scope, CHAR const* name);
TypeSpec * new_type();
TypeSpec * new_type(INT cate);
Enum * new_enum();

void set_decl_init_tree(Decl const* decl, Tree * initval);

Decl * type_name();
Decl * trans_to_pointer(Decl * decl, bool is_append);

//Exported Variables
extern INT g_alignment;
extern CHAR const* g_dcl_name[];
#endif

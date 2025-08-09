/*@
Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com
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

namespace xfe {

#define ALLOW_CONST_VOLATILE 1
#define DECL_ID_UNDEF 0
#define AGGR_ID_UNDEF 0
#define AGGR_ANONY_ID_UNDEF 0

class Scope;

#define EVAL_is_evaluated(el) ((el)->is_value_evaluated)
#define EVAL_val(el) ((el)->val)
#define EVAL_name(el) ((el)->str)
#define EVAL_next(el) ((el)->next)
#define EVAL_prev(el) ((el)->prev)
class EnumValueList {
public:
    //Set to true if the value of current enumator has been evaluated.
    BYTE is_value_evaluated:1;
    INT val;
    xoc::Sym const* str;
    EnumValueList * next;
    EnumValueList * prev;
public:
    INT getVal() const { return EVAL_val(this); }
    bool is_evaluated() const { return EVAL_is_evaluated(this); }
};


//Enum
//Record Enum info, and field 'name' reserved its character description.
//e.g: enum X { ... }; X is the name of enumerator.
#define ENUM_name(e) ((e)->m_name)
#define ENUM_vallist(e) ((e)->m_vallist)
#define ENUM_is_complete(e) ((e)->m_is_complete)
class Enum {
    //Retrieve constant value of Enum Iterm and Index in Enum List by
    //given name.
    //idx: index in enum 'e' constant list, start at 0.
    bool isEnumValExistAndEvalValue(EnumValueList const* evl,
                                    CHAR const* vname,
                                    OUT INT * eval, OUT INT * idx) const;
public:
    BYTE m_is_complete:1;
    xoc::Sym const* m_name;
    EnumValueList * m_vallist;
public:
    void copy(Enum const& src)
    {
        ENUM_name(this) = src.getName();
        ENUM_vallist(this) = src.getValList();
        ENUM_is_complete(this) = src.is_complete();
    }

    EnumValueList * getValList() const { return ENUM_vallist(this); }
    xoc::Sym const* getName() const { return ENUM_name(this); }

    bool is_complete() const { return ENUM_is_complete(this); }

    //idx: index in enum 'e' constant list, start at 0.
    bool isEnumValExist(CHAR const* vname, OUT INT * idx) const;

    //Retrieve constant value of Enum Iterm and Index in Enum List
    //by given name.
    //idx: index in enum 'e' constant list, start at 0.
    bool isEnumValExistAndEvalValue(CHAR const* vname,
                                    OUT INT * eval, OUT INT * idx) const;
};

class CompareEnumTab {
    COPY_CONSTRUCTOR(CompareEnumTab);
public:
    CompareEnumTab() {}

    //Note the function createKey() will modify parameter's contents, thus the
    //'const' qualifier is unusable.
    Enum * createKey(Enum * t);
    bool is_less(Enum const* t1, Enum const* t2) const;
    bool is_equ(Enum const* t1, Enum const* t2) const;
};

class EnumTab : public TTab<Enum*, CompareEnumTab> {};
typedef TTabIter<Enum*> EnumTabIter;

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
#define AGGR_id(s) ((s)->m_id)
#define AGGR_decl_list(s) ((s)->m_decl_list)
#define AGGR_is_complete(s) ((s)->m_is_aggr_complete)
#define AGGR_tag(s) ((s)->m_tag)
#define AGGR_align(s) ((s)->m_align)
#define AGGR_field_align(s) ((s)->m_field_align)
#define AGGR_pack_align(s) ((s)->m_pack_align)
#define AGGR_scope(s) ((s)->m_scope)
class Aggr {
public:
    UINT m_id:31;
    UINT m_is_aggr_complete:1;
    UINT m_align; //alignment that whole structure have to align.
    //Alignment that each fields in structure have to align.
    //0 indicates there is requirement to field align.
    UINT m_field_align;
    UINT m_pack_align; //User declared field alignment.
    Decl * m_decl_list;
    xoc::Sym const* m_tag;
    Scope * m_scope;
public:
    //Compute new alignment size according to given 'size' and 'max_field_size'.
    UINT computeAlignedSize(UINT size, UINT max_field_size) const;

    Decl * getDeclList() const { return AGGR_decl_list(this); }
    xoc::Sym const* getTag() const { return AGGR_tag(this); }
    UINT getAlign() const { return AGGR_align(this); }
    UINT getPackAlign() const { return AGGR_pack_align(this); }
    Scope * getScope() const { return AGGR_scope(this); }

    UINT id() const { return AGGR_id(this); }
    bool is_equal(Aggr const& src) const;
    bool is_complete() const { return AGGR_is_complete(this); }
};


//Struct
class Struct : public Aggr {
public:
};


//Union
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
#define TYPE_user_type(t) ((t)->m_user_type) //reord user type
#define TYPE_enum_type(t) ((t)->u1.m_enum_type) //record enum type
#define TYPE_aggr_type(t) ((t)->u1.m_aggr_type) //record aggregate type

#define IS_TYPE(des,T) HAVE_FLAG((des), T)

typedef ULONG DesSet;

//This class represents memory location modifier, basic type, and user defined
//type, such as: const, volatile, void, char, short, int, long, longlong,
//float, double, bool, signed, unsigned, struct, union, enum-specifier,
//typedef-name, auto, register, static, extern, typedef.
class TypeAttr {
public:
    DesSet m_descriptor; //records the attributes.
    Decl * m_user_type; //record user defined type.
    union {
        void * anony; //records an anonymous pointer that is used internally.
        Enum * m_enum_type; //record the enumerator that consist of list
                            //enum-iterm.
        Aggr * m_aggr_type; //record the aggregation.
    } u1;
    TypeAttr * m_sub_field[MAX_TYPE_FLD];
public:
    TypeAttr() { clean(); }

    void clean()
    {
        m_descriptor = 0;
        u1.anony = nullptr;
        for (INT i = 0; i < MAX_TYPE_FLD; i++) {
            m_sub_field[i] = nullptr;
        }
    }
    void copy(TypeAttr const& ty)
    {
        m_descriptor = ty.m_descriptor;
        u1.anony = ty.u1.anony;
        for (UINT i = 0; i < MAX_TYPE_FLD; i++) {
            m_sub_field[i] = ty.m_sub_field[i];
        }
    }
    static UINT computeStructTypeSize(Aggr const* s);
    static UINT computeUnionTypeSize(Aggr const* s);
    static UINT computeScalarTypeBitSize(UINT des);

    void dump() const;

    //Get specifior type, specifior type refers to
    //Non-pointer and non-array type.
    // e.g: int a;
    //      void a;
    //      struct a;
    //      union a;
    //      enum a;
    //      USER_DEFINED_TYPE_NAME a;
    UINT getSpecTypeSize() const;
    Enum * getEnumType() const { return TYPE_enum_type(this); }
    Aggr * getAggrType() const { return TYPE_aggr_type(this); }
    Decl * getUserType() const { return TYPE_user_type(this); }
    CHAR const* getAggrTypeName() const;
    DesSet getDes() const { return TYPE_des(this); }

    //If current type-attr is a user-defined type, return the real type-spec.
    //Real type-spec could be obtained by expanding user-type.
    TypeAttr * getPureTypeAttr();

    bool is_const() const { return HAVE_FLAG(TYPE_des(this), T_QUA_CONST); }
    bool is_volatile() const
    { return HAVE_FLAG(TYPE_des(this), T_QUA_VOLATILE); }
    bool is_restrict() const
    { return HAVE_FLAG(TYPE_des(this), T_QUA_RESTRICT); }
    bool is_aggr() const { return is_struct() || is_union(); }
    bool is_struct() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_STRUCT); }
    bool is_union() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_UNION); }
    bool is_auto() const { return HAVE_FLAG(TYPE_des(this), T_STOR_AUTO); }
    bool is_reg() const { return HAVE_FLAG(TYPE_des(this), T_STOR_REG); }
    bool is_static() const { return HAVE_FLAG(TYPE_des(this), T_STOR_STATIC); }
    bool is_extern() const { return HAVE_FLAG(TYPE_des(this), T_STOR_EXTERN); }
    bool is_inline() const { return HAVE_FLAG(TYPE_des(this), T_STOR_INLINE); }
    bool is_enum() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_ENUM); }
    bool is_double() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_DOUBLE); }
    bool is_float() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_FLOAT); }
    bool is_void() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_VOID); }
    bool is_char() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_CHAR); }
    bool is_bool() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_BOOL); }
    bool is_short() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_SHORT); }
    bool is_int() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_INT); }
    bool is_long() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_LONG); }
    bool is_signed() const { return HAVE_FLAG(TYPE_des(this), T_SPEC_SIGNED); }
    bool is_unsigned() const
    { return HAVE_FLAG(TYPE_des(this), T_SPEC_UNSIGNED); }
    bool is_longlong() const
    { return HAVE_FLAG(TYPE_des(this), T_SPEC_LONGLONG); }

    //Return true if the specifier represents a variable or type-name via
    //user type.
    bool is_user_type_ref() const
    { return HAVE_FLAG(TYPE_des(this), T_SPEC_USER_TYPE); }

    //Return true if the declaration defined a user type via 'typedef' operator.
    bool is_typedef() const
    { return HAVE_FLAG(TYPE_des(this), T_STOR_TYPEDEF); }
    bool is_integer() const
    {
        return is_char() || is_short()|| is_int() || is_bool() || is_long() ||
               is_longlong() || is_signed() || is_unsigned() || is_enum();
    }
    bool is_fp() const { return is_float() || is_double(); }
    bool is_equal(TypeAttr const& ty) const;

    //Return true if the type is not aggregate and compound type.
    bool isSimpleType() const { return is_void() || is_integer() || is_fp(); }

    //Return true if 'qua' is suffice for constrains of language.
    bool isValidSpecifier() const
    { return isSimpleType() || is_aggr() || is_user_type_ref(); }

    //Return true if aggregation definition is complete.
    bool isAggrComplete() const;
    bool isAggrExpanded() const;
    bool isStructExpanded() const;
    bool isUnionExpanded() const;

    static bool isSimpleType(INT des)
    {
        return des & T_SPEC_VOID ||
               des & T_SPEC_ENUM ||
               des & T_SPEC_CHAR ||
               des & T_SPEC_SHORT ||
               des & T_SPEC_INT ||
               des & T_SPEC_LONGLONG ||
               des & T_SPEC_LONG ||
               des & T_SPEC_FLOAT ||
               des & T_SPEC_DOUBLE ||
               des & T_SPEC_SIGNED ||
               des & T_SPEC_UNSIGNED;
    }
};


//
//START Decl
//

//Example to show the structure of class Decl.
//  e.g1: int * a, * const * volatile b[10];
//  declaration----
//      |          |--attribute (int)
//      |          |--declarator1 (DCL_DECLARATOR)
//      |                |---decl-type (id:a)
//      |                |---decl-type (pointer)
//      |
//      |          |--declarator2 (DCL_DECLARATOR)
//      |                |---decl-type (id:b)
//      |                |---decl-type (array:dim=10)
//      |                |---decl-type (pointer:volatile)
//      |                |---decl-type (pointer:const)
//
//  e.g2: int (*q)[30];
//  declaration----
//      |          |--attribute (int)
//      |          |--declarator1 (DCL_DECLARATOR)
//      |                |---decl-type (id:q)
//      |                |---decl-type (pointer)
//      |                |---decl-type (array:dim=30)
//
//  e.g3: unsigned long const (* const c)(void);
//  declaration----
//                |--attribute (unsigned long const)
//                |--declarator1 (DCL_DECLARATOR)
//                      |---decl-type (id:c)
//                      |---decl-type (pointer:const)
//                      |---decl-type (function)
//
//  e.g4: USER_DEFINED_TYPE var;
//  declaration----
//                |--attribute (T_SPEC_USER_TYPE)
//                |--declarator (DCL_DECLARATOR)
//                      |---decl-type (id:var)
//
//  e.g5: Abstract declarator, often used in parameters.
//  int *
//  declaration----
//                |--attribute (int)
//                |--declarator (DCL_ABS_DECLARATOR)
//                      |---nullptr
//
//  e.g6: double (*arr[10][40])[20][30];
//  Note the lowest dimension, which iterates most slowly, is at the most right
//  of decl-type list.
//  In this example, array:dim=40 is the lowest dimension.
//  declaration----
//      |         |--attribute (double)
//      |         |--declarator1 (DCL_DECLARATOR)
//      |               |---decl-type (id:arr)
//      |               |---decl-type (array:dim=10)
//      |               |---decl-type (array:dim=40)
//      |               |---decl-type (pointer)
//      |               |---decl-type (array:dim=20)
//      |               |---decl-type (array:dim=30)
//
// e.g7: function declaration:
// int * foo(char a, short b);
// declaration----
//      |         |--attribute (int)
//      |         |--declarator1 (DCL_DECLARATOR)
//      |               |---decl-type (id:foo)
//      |               |---decl-type (fun)
//      |                       |---decl-type (DECLARATION of a)
//      |                       |---decl-type (DECLARATION of b)
//      |               |---decl-type (pointer) return value type is pointer.
//
// e.g8: function declaration:
// struct {...} S;
// S * foo(char a, short b);
// declaration----
//      |         |--attribute (struct S)
//      |         |--declarator1 (DCL_DECLARATOR)
//      |               |---decl-type (id:foo)
//      |               |---decl-type (fun)
//      |                       |---decl-type (DECLARATION of a)
//      |                       |---decl-type (DECLARATION of b)
//      |               |---decl-type (pointer) return value type is pointer.
//
// e.g9: function pointer:
// struct {...} S;
// S * (*bar)(char a);
// declaration----
//      |         |--attribute (struct S)
//      |         |--declarator1 (DCL_DECLARATOR)
//      |               |---decl-type (id:bar)
//      |               |---decl-type (pointer)
//      |               |---decl-type (fun)
//      |                       |---decl-type (DECLARATION of a)
//      |               |---decl-type (pointer) return value type is pointer.
//
//The layout of Declaration:
//Decl, with DECL_dt is DCL_DECLARATION or DCL_TYPE_NAME
//    |->Scope
//    |->TypeAttr
//        |->const|volatile
//        |->void|long|int|short|char|float|double|signed|unsigned|struct|union
//        |->auto|register|static|extern|typedef
//    |->DCL_DECLARATOR | DCL_ABS_DECLARATOR
//        |->DCL_ID(optional)->DCL_FUN->DCL_POINTER->...

#define DECL_id(d) ((d)->m_id)
//ONLY used in DCL_DECLARATION
#define DECL_is_formal_param(d) ((d)->m_is_formal_param)
#define DECL_is_anony_aggr(d) ((d)->m_is_anony_aggregate)
#define DECL_dt(d) ((d)->decl_type)
#define DECL_next(d) ((d)->next)
#define DECL_prev(d) ((d)->prev)
#define DECL_is_paren(d) ((d)->m_is_paren)
#define DECL_is_bit_field(d) ((d)->m_is_bit_field)
#define DECL_is_sub_field(d) ((d)->m_is_sub_field)
#define DECL_is_fun_def(d) ((d)->m_is_fun_def) //ONLY used in DCL_DECLARATION
#define DECL_is_init(d) ((d)->m_is_init) //ONLY used in DCL_DECLARATOR
#define DECL_fieldno(d) ((d)->fieldno)
#define DECL_align(d) ((d)->align)
#define DECL_base_type_spec(d) ((d)->base_type_spec)

//If current 'decl' is DCL_DECLARATION, it must include 'type attribute' and
//'declarator'. Because one type-attribute could decorate multiple declarators,
//there is a list of declarators actually, namely, 'declarator_list'.
//Type attribute includes type-specifier, type-qualifier, storage-specifier.
//Type Specifier describes the first-class type of C such as int, char,
//struct/union, etc.
//Declarator-list records and describes the compound type, which is consist of
//identifier, pointer, array, function, variadic, etc.
#define DECL_spec(d) ((d)->u0.type_attribute)
#define DECL_decl_list(d) ((d)->u0.declarator_list)
#define DECL_decl_scope(d) ((d)->u0.scope)

//If current 'decl' is a function define, the followed member record its body.
#define DECL_fun_body(d) ((d)->u2.fun_body)

//Record the formal parameter position if Decl is a parameter.
#define DECL_formal_param_pos(d) ((d)->u1.formal_param_pos)

//Record qualification of DCL.
//If current 'decl' is DCL_POINTER or DCL_ID, the
//followed member record its quanlifier specicfier.
//qualifier include const, volatile, restrict.
#define DECL_qua(d) ((d)->qualifier)

//Line number
#define DECL_lineno(d) ((d)->lineno)

//If current 'decl' is a DCL_DECLARATOR, the followed member
//record it initializing tree
#define DECL_init_tree(d) ((d)->u2.init)

//If current 'decl' is DCL_ID , the followed member record it
#define DECL_id_tree(d) ((d)->u1.id_tree)

//If current 'decl' is DCL_ARRAY, the followed members record its
//base and index value/expression, which may be nullptr.
//
//NOTE: During the Decl generation processing, DECL_array_dim_exp() is avaiable,
//it records the expressions of dimension,
//and the actually dimension value will be calculated after parsing array type
//finished, and compute_array_dim() will be invoked to compute the integer
//value.
//Meanwile, DECL_array_dim() is avaiable.
#define DECL_array_dim(d) ((d)->u1.u12.u121.dimval)
#define DECL_array_dim_exp(d) ((d)->u1.u12.u121.dimexp)

//If current 'decl' is DCL_FUN, the followed members record its
//base and parameter list declaration, which may be nullptr
//#define DECL_fun_base(d) (d)->u1.u13.fbase
#define DECL_fun_para_list(d) ((d)->u1.u13.para_list)

//Record content if current 'decl' is DCL_DECLARATOR or DCL_ABS_DECLARATOR
#define DECL_child(d) ((d)->child)

//Bit field ONLY used in DCL_DECLARATOR, and record in decl.cpp
//checking in typeck.cpp
//It will not be zero if current decl record a bit field
#define DECL_bit_len(d) ((d)->u1.u16.bit_len)
//#define DECL_bit_exp(d) ((d)->u1.u16.bit_exp)

//Record the placeholder in stmt list of scope.
//The placeholder is used to mark the lexicographical order of declarataion.
//The order is often used to determine where should to
//insert initialization code.
#define DECL_placeholder(d) ((d)->placeholder)

//'d' must be DECLARATION or TYPE-NAME, get pure declarator list
//The macro without validation check, plz call
//getTraitList if you expect verification.
#define DECL_trait(d) DECL_child(DECL_decl_list(d))
#define MAX_ARRAY_INDX 0xfffffff
//END Decl

//Format Decl to dump
#define DECL_FMT_INDENT_INTERVAL 2

//Define type of Decl.
typedef enum {
    DCL_NULL = 0,
    DCL_ARRAY, //array declarator
    DCL_POINTER, //pointer declarator
    DCL_FUN, //function declarator
    DCL_ID, //identifier
    DCL_VARIADIC, //variadic parameter, e.g: ...
    DCL_TYPE_NAME, //if current decl is TYPE_NAME,  it descript a
                   //abstract type spec
    DCL_DECLARATOR, //declarator
    DCL_DECLARATION, //declaration
    DCL_ABS_DECLARATOR, //abstract declarator
} DCL;

class Decl {
public:
    //1 indicates that the Decl enclosed with a pair of '(' ')'.
    BYTE m_is_paren:1;
    BYTE m_is_bit_field:1; //descripte a bit field.

    //descripte a function's property: 1: definition, 0:declaration.
    BYTE m_is_fun_def:1;
    BYTE m_is_init:1; //has a initializing expression.
    BYTE m_is_sub_field:1; //Decl is a sub field of struct/union.
    BYTE m_is_formal_param:1; //Decl is a formal parameter.
    BYTE m_is_anony_aggregate:1; //Decl is an anonymous aggregate, which
                                 //means it does NOT have identifier.

    UINT m_id;
    UINT lineno; //record line number of declaration.

    //record the num of fields while the base of Decl is Struct/Union.
    UINT fieldno;

    //memory alignment of declaration in the current context.
    UINT align;

    DCL decl_type;
    Decl * prev;
    Decl * next;
    Decl * child;
    Tree * placeholder; //record the placeholder in stmt list of scope.
    TypeAttr * base_type_spec; //Record base type-spec of field.
    TypeAttr * qualifier; //quanlifier for POINTER/ID

    struct {
        //Record declaration attributes, includes specifier, qualifier,
        //storage. Only locate on top of decl link list
        TypeAttr * type_attribute;

        Decl * declarator_list; //declarator list
        Scope * scope; //declaration reside in
    } u0; //only for DCL_DECLARATION used

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
        Tree * id_tree;

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

public:
    //Converts current declaration into pointer type from its origin type.
    //NOTCIE: 'this' cannot be pointer.
    //This function transform to pointer type by appending a DCL_POINTER.
    //In order to achieve, insert DCL_POINTER type before
    //the first array type.
    //e.g: ID->ARRAY->ARRAY => ID->POINTER->ARRAY->ARRAY
    void convertToPointerType();

    void dump() const;
    void dump(xcom::DefFixedStrBuf & buf) const;

    DCL getDeclType() const { return DECL_dt(this); }
    Sym const* getDeclSym() const;
    CHAR const* getDeclName() const;
    Tree * getDeclInitTree() const;
    Decl * getPureDeclaratorList() const { return DECL_decl_list(this); }
    Decl const* getReturnType() const;
    Decl const* getDeclIdTree() const;

    //Pick out the trait declarator specification list
    //    e.g:
    //        int * a [10];
    //        the trait list is :  a->[10]->*
    //
    //        int (*) [10];
    //        the trait list is :  *->[10]
    Decl const* getTraitList() const;

    //Compute byte size to complex type.
    //complex type means the type is either pointer or array.
    //e.g: int * a;
    //     int a [];
    ULONG getComplexTypeSize() const;

    //Compute the byte size of current declaration.
    //The function will compute whole array byte size.
    UINT getDeclByteSize() const;

    //Compute byte size of pointer base declarator.
    //e.g: Given 'int *(*p)[3]', the pointer-base is 'int * [3]'.
    UINT getPointerBaseSize() const;

    //The function get the POINTER decl-type in decl-list.
    //   declaration----
    //       |          |--type-spec (int)
    //       |          |--declarator1 (DCL_DECLARATOR)
    //       |                |---decl-type (id:a)
    //       |                |---decl-type (pointer)
    //
    //e.g:given Decl as : 'int * a', then the second decl-type in the decl-list
    //    must be DCL_POINTER, the first is DCL_ID 'a'.
    //    And simplar for abs-decl, as an example 'int *', the first decltor
    //    in the type-chain must be DCL_POINTER.
    Decl const* getPointerDeclarator() const;

    //Return the *first* Decl structure which indicate a pointer
    //in pure-list of declaration.
    //e.g: int * p; the declarator is: DCL_ID(p)->DCL_POINTER(*).
    //return DCL_POINTER.
    Decl const* getPointerDecl() const;

    //Return the Decl structure of return-value of function declaration of
    //function pointer type if exist.
    //e.g:int bar(); the returned Decl is NULL.
    //    int * zoo(); the returned Decl is DCL_POINTER.
    //Note the TypeAttr of return-value is 'int'.
    Decl const* getReturnValueDecl() const;

    //Return the *first* Decl structure which indicate an array
    //in pure-list of declaration.
    //e.g:int p[10][20]; the declarator is:
    //    DCL_ID(p)->DCL_ARRAY(20)->DCL_ARRAY(10).
    //return DCL_ARRAY(20).
    Decl * getFirstArrayDecl() const;

    //The function get the base decloarator of ARRAY in decl-list.
    //e.g: the function will return 'pointer' decl-type.
    //    declaration----
    //        |          |--type-spec (int)
    //        |          |--declarator1 (DCL_DECLARATOR)
    //        |                |---decl-type (id:a)
    //        |                |---decl-type (array) the highest dimension
    //        |                |---decl-type (array)
    //        |                |---decl-type (pointer)
    Decl const* getArrayBaseDeclarator() const;

    //Get and generate array base declaration.
    //Note current declaration must be array. If array is multiple-dimension,
    //the funtion constructs return the base type by striping all dimensions.
    //e.g: given int arr[10][20];
    //  the declarator is: ID(arr)->ARRAY(10)->ARRAY(20).
    //  the function construct and return decl: 'int'.
    Decl * getArrayBaseDecl() const;

    //Get and generate array element declaration.
    //Note if array is multiple-dimension, the funtion only construct and
    //return the element type of sub-dimension.
    //e.g: given int arr[10][20];
    //  the declarator is: ID(arr)->ARRAY(10)->ARRAY(20).
    //  The function constructs and returns an array decl: 'int [20]';
    Decl * getArrayElemDecl() const;

    //Get the number of element to given dimension.
    //Note the field DECL_array_dim of array is only
    //available after compute_array_dim() invoked, and
    //it compute the really number of array element via
    //DECL_array_dim_exp, that is a constant expression.
    //'arr': array declaration.
    //'dim': given dimension to compute, start at 0 which is
    //    the closest dimension
    //    to leading ID, in decl-type list.
    //    e.g: int arr[8][12][24];
    //    In C language, [24] is the lowest dimension.
    //    where decl-type list will be:
    //      ID:'arr' -> ARRAY[8] -> ARRAY[12] -> ARRAY[24]
    //    dim 0 indicates ARRAY[8], the highest dimension of 'arr'.
    ULONG getArrayElemnumToDim(UINT dim) const;

    //Get the number of elements in entire array.
    ULONG getArrayElemNum() const
    {
        UINT dn = getArrayDim();
        UINT en = 1;
        for (UINT i = 0; i < dn; i++) {
            en *= getArrayElemnumToDim(i);
        }
        return en;
    }

    //Get the bytesize of array element.
    ULONG getArrayElemByteSize() const
    {
        ASSERT0(is_array());
        ASSERT0(getTypeAttr());
        return getTypeAttr()->getSpecTypeSize();
    }

    //Return the dimension of given array.
    //Note array should be DCL_DECLARATION or DCL_TYPE_NAME.
    UINT getArrayDim() const;

    //Get base type of POINTER.
    Decl * getPointerBaseDecl(TypeAttr ** ty) const;

    //Record qualification of DCL.
    //If current 'decl' is DCL_POINTER or DCL_ID, the
    //followed member record its quanlifier specicfier.
    //qualifier include const, volatile, restrict.
    TypeAttr * getQua() const
    {
        ASSERT0(is_dt_pointer() || is_dt_id());
        return DECL_qua(this);
    }
    Struct * getStructSpec() const
    {
        ASSERT0(is_struct());
        return (Struct*)getTypeAttr()->getAggrType();

    }
    Union * getUnionSpec() const
    {
        ASSERT0(is_union());
        return (Union*)getTypeAttr()->getAggrType();
    }
    Aggr * getAggrSpec() const
    {
        ASSERT0(is_aggr());
        return getTypeAttr()->getAggrType();
    }
    TypeAttr * getTypeAttr() const
    {
        ASSERT0(is_dt_declaration() || is_dt_typename());
        return DECL_spec(this);
    }

    //Pick out the declarator.
    //e.g:int * a [10];
    //    the declarator is :
    //      declaration
    //          |->declarator
    //                 |->a->[10]->*
    Decl const* getDeclarator() const;

    //Get the function body scope.
    Scope * getFunBody() const { return DECL_fun_body(this); }

    //Return the scope that current declaration is resided in.
    Scope * getDeclScope() const { return DECL_decl_scope(this); }
    UINT getLineno() const { return DECL_lineno(this); }

    UINT id() const { return DECL_id(this); }
    bool is_dt_array() const { return getDeclType() == DCL_ARRAY; }
    bool is_dt_pointer() const { return getDeclType() == DCL_POINTER; }
    bool is_dt_fun() const { return getDeclType() == DCL_FUN; }
    bool is_dt_id() const { return getDeclType() == DCL_ID; }
    bool is_dt_var() const { return getDeclType() == DCL_VARIADIC; }
    bool is_dt_typename() const { return getDeclType() == DCL_TYPE_NAME; }
    bool is_dt_declarator() const { return getDeclType() == DCL_DECLARATOR; }
    bool is_dt_declaration() const
    { return getDeclType() == DCL_DECLARATION; }
    bool is_dt_abs_declarator() const
    { return getDeclType() == DCL_ABS_DECLARATOR; }
    bool is_equal(Decl const& src) const;
    bool is_extern() const { return getTypeAttr()->is_extern(); }

    //Return ture if 'dcl' is type declaration that declared with 'typedef'.
    //e.g: typedef int * INTP; where INTP is an user type declaration.
    bool is_user_type_decl() const
    {
        ASSERT0(DECL_dt(this) == DCL_DECLARATION);
        return getTypeAttr()->is_typedef();
    }

    //Return true if dcl is daclared with user defined type.
    //e.g: typedef int * INTP;  INTP xx; xx is user type referrence.
    bool is_user_type_ref() const
    {
        ASSERT0(DECL_dt(this) == DCL_DECLARATION ||
                DECL_dt(this) == DCL_TYPE_NAME);
        ASSERT0(getTypeAttr() != nullptr);
        return getTypeAttr()->is_user_type_ref();
    }

    bool is_static() const
    {
        ASSERTN(DECL_dt(this) == DCL_DECLARATION, ("needs declaration"));
        ASSERTN(getTypeAttr(), ("miss specify type"));
        if (getTypeAttr()->is_static()) {
            return true;
        }
        return false;
    }

    //Return true if current decl declared with 'const'.
    bool is_constant() const;

    //Return true if current declaration declared with 'volatile'.
    bool is_volatile() const;

    //Return true if dcl declared with 'inline'.
    bool is_inline() const
    {
        ASSERTN(DECL_dt(this) == DCL_DECLARATION, ("requires declaration"));
        ASSERT0(getTypeAttr());
        return getTypeAttr()->is_inline();
    }

    //Return true for arithmetic type which include integer and float.
    bool is_arith() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
                ("expect type-name or dcrlaration"));
        TypeAttr * ty = getTypeAttr();
        return is_scalar() && (ty->is_integer() || ty->is_fp());
    }
    bool is_any() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
               ("expect type-name or dcrlaration"));
        return getTypeAttr()->is_void();
    }
    bool is_bool() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
               ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_bool();
    }
    bool is_char() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
               ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_char();
    }

    //Return true if current decl declared with 'restrict'.
    bool is_restrict() const;

    //Pointer, array, struct, union are not scalar type.
    bool is_scalar() const;
    bool is_aggr() const { return is_struct() || is_union(); }
    bool is_struct() const;
    bool is_union() const;
    bool is_bitfield() const;
    bool is_fp() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
                ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_fp();
    }

    //Is single decision float-point.
    bool is_float() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
                ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_float();
    }

    //Return true if it is double decision float-point.
    bool is_double() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
                ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_double();
    }

    //Return true if it is scalar integer type.
    bool is_integer() const
    {
        ASSERTN(DECL_dt(this) == DCL_TYPE_NAME ||
                DECL_dt(this) == DCL_DECLARATION,
               ("expect type-name or dcrlaration"));
        return is_scalar() && getTypeAttr()->is_integer();
    }

    //Return if current declaration is aggregate and is complete defined.
    bool isAggrComplete() const
    { return is_aggr() && getTypeAttr()->isAggrComplete(); }

    //Return if current declaration is aggregate but is NOT complete defined.
    //The incomplete aggregate type is usually used in forward-declaration.
    bool isAggrInComplete() const
    { return is_aggr() && !getTypeAttr()->isAggrComplete(); }

    //Return true if declaration|type_name is a char-array.
    //e.g: char x[10]; x is a char-array.
    //     'abcd' is a char-array.
    bool isCharArray() const
    { return is_array() && getTypeAttr()->is_char(); }

    //What is 'complex type'? Non-pointer and non-array type.
    //e.g: int * a;
    //     int a[];
    bool is_complex_type() const;
    bool is_pointer() const;
    bool is_array() const;
    bool is_declaration() const;
    bool is_initialized() const;
    bool is_fun_def() const { return DECL_is_fun_def(this); }
    bool is_fun_decl() const;
    bool is_fun_return_void() const;
    bool is_fun_return_pointer() const;
    bool is_fun_pointer() const;
    bool is_local_variable() const;
    bool is_global_variable() const;
    bool is_anony_aggr() const { return DECL_is_anony_aggr(this); }

    //Return true if current declaration/type-name indicates a reference of
    //function type, except function definition.
    bool isFunTypeRef() const
    { return is_fun_pointer() || is_fun_decl() || is_fun_def(); }
    bool isPointerPointToArray() const;
    bool isPointer() const { return is_pointer() || is_fun_return_pointer(); }
    bool is_formal_param() const
    {
        ASSERT0(is_dt_declaration());
        return DECL_is_formal_param(this);
    }
    static bool is_decl_equal(Decl const* d1, Decl const* d2);

    //Note VOID type can not be regarded as pointer because it does not have
    //base-pointee-type.
    bool regardAsPointer() const { return isPointer() || isFunTypeRef(); }

    void setDeclInitTree(Tree * initval);
};

//The function generate type-name, and convert it to pointer type.
Decl * convertToPointerTypeName(Decl const* decl);

//Duplicate src and generate new type-name.
Decl * dupTypeName(Decl const* src);
TypeAttr * dupSpec(TypeAttr const* ty);

//Copy whole Decl, include all its specifier, qualifier, storage
//and declarator.
Decl * dupDeclFully(Decl const* src);

//Only copy 'src', excepting its field.
Decl * dupDecl(Decl const* src);

//Duplication declarator list begin at 'header'
Decl * dupDeclBeginAt(Decl const* header);
Tree * declaration();
Tree * declaration_list();

//The function will change 'ut' and expand user-defined type that declared
//with 'typedef' in C, and remove the 'typedef' attribute from type-specifier.
//The result type is only consist of the first-class type defined in C.
//The function will modify 'ut' by replacing type-name with the first-class
//type.
Decl * expandUserType(Decl * ut);

//Dump C style type-info
INT format_enum_complete(xcom::DefFixedStrBuf & buf, Enum const* e);
INT format_aggr_complete(xcom::DefFixedStrBuf & buf, TypeAttr const* ty);
INT format_struct_complete(xcom::DefFixedStrBuf & buf, Struct const* s);
INT format_union_complete(xcom::DefFixedStrBuf & buf, Union const* u);
INT format_parameter_list(xcom::DefFixedStrBuf & buf, Decl const* decl);
INT format_user_type(xcom::DefFixedStrBuf & buf, TypeAttr const* ty);
INT format_user_type(xcom::DefFixedStrBuf & buf, Decl const* ut);
INT format_declarator(xcom::DefFixedStrBuf & buf, TypeAttr const* ty,
                      Decl const* decl);

//To avoid recursive aggregate type, format field
//only with incomplete aggregate type.
//e.g:union node;
//    typedef union node * nodeptr;
//    struct vector {
//      nodeptr a[1]; //union node includes struct vector.
//    };
//    union node {
//      struct vector vec; //struct vector includes union node.
//    }
//is_complete: true to dump aggregate's declarations list recursively.
//             Note this might leading to infinite invocation of current
//             function if meeting recursive struct.
INT format_attr(xcom::DefFixedStrBuf & buf, TypeAttr const* ty,
                bool is_complete);
INT format_declaration(xcom::DefFixedStrBuf & buf, Decl const* decl,
                       bool is_complete);

//Dump Decl-Tree style type-info
INT format_dcrl(Decl const* decl, INT indent);
INT format_parameter_list(Decl const* decl, INT indent);
INT format_user_type(TypeAttr const* ty, INT indent);
INT format_user_type(Decl const* ut, INT indent);
INT format_declarator(IN Decl const* decl, TypeAttr const* ty, INT indent);
INT format_declaration(IN Decl const* decl, INT indent, bool is_complete);
bool findEnumVal(CHAR const* name, OUT Enum ** e, OUT INT * idx);

bool isDeclExistInOuterScope(CHAR const* name, OUT Decl ** dcl);
bool isAbsDeclaraotr(Decl const* declarator);
bool isUniqueDecl(Decl const* decl_list, Decl const* decl);
bool isUnionExistInOuterScope(
    Scope * scope, CHAR const* tag, bool is_complete, OUT Union ** s);

//Return true if the union typed declaration have already existed in both
//current and all of outer scopes.
bool isUnionExistInOuterScope(
    Scope * scope, Sym const* tag, bool is_complete, OUT Union ** s);
bool isStructTypeExistInCurScope(
    CHAR const* tag, bool is_complete, OUT Struct ** s);

//Return true if the struct typed declaration have already existed in both
//current and all of outer scopes.
bool isAggrExistInOuterScope(
    Scope * scope, Sym const* tag, bool is_complete, TypeAttr const* spec,
    OUT Aggr ** s);
bool isAggrExistInOuterScope(
    Scope * scope, CHAR const* tag, bool is_complete, TypeAttr const* spec,
    OUT Aggr ** s);
bool isStructExistInOuterScope(
    Scope * scope, Sym const* tag, bool is_complete, OUT Struct ** s);
bool isStructExistInOuterScope(
    Scope * scope, CHAR const* tag, bool is_complete, OUT Struct ** s);
bool isEnumTagExistInOuterScope(CHAR const* cl, OUT Enum ** e);
bool isUserTypeExistInOuterScope(CHAR const* cl, OUT Decl ** ut);

//Return true if enum-value existed in current scope.
bool isUserTypeExist(
    UserTypeList const* ut_list, CHAR const* ut_name, Decl ** ut);
bool isAggrTypeExist(
    List<Aggr*> const* aggrs, Sym const* tag, bool is_complete, OUT Aggr ** s);
bool isAggrTypeExist(
    List<Aggr*> const* aggrs, CHAR const* tag, bool is_complete, OUT Aggr ** s);
bool inFirstSetOfDeclaration();

//fun_dclor: record the declarator that indicates a parameter list.
Decl * get_parameter_list(Decl * dcl, OUT Decl ** fun_dclor = nullptr);
Decl * get_decl_in_scope(CHAR const* name, Scope const* scope);
INT get_enum_const_val(Enum const* e, INT idx);
CHAR const* get_enum_const_name(Enum const* e, INT idx);
bool get_aggr_field(
    TypeAttr const* ty, CHAR const* name, Decl ** fld_decl, UINT * fld_ofst);
bool get_aggr_field(
    TypeAttr const* ty, INT idx, Decl ** fld_decl, UINT * fld_ofst);

//The function make up TYPE_NAME according to given DECLARATION.
//The function just do copy if 'src' is TYPE_NAME, otherwise generate
//TYPE_NAME according 'src' information.
Decl * genTypeName(Decl const* src);
Decl const* genTypeName(TypeAttr * ty);

Decl * newDeclaration(
    TypeAttr * spec, Decl * declor, Scope * sc, Tree * inittree);
Decl * newDecl(DCL dcl_type);
Decl * newVarDecl(IN Scope * scope, CHAR const* name);
TypeAttr * newTypeAttr();
TypeAttr * newTypeAttr(INT cate);
Enum * newEnum();

Decl * makeupAndExpandUserType(Decl const* ut);

Decl * type_name();

//Exported Variables
extern INT g_alignment;
extern UINT g_decl_count;
extern UINT g_aggr_count;

//The counter for anonymous name of aggregate.
extern UINT g_aggr_anony_name_count;
extern CHAR const* g_dcl_name[];

} //namespace xfe
#endif

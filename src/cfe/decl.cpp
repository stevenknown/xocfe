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
#include "cfeinc.h"
#include "cfecommacro.h"

namespace xfe {

static Tree * initializer(TypeAttr * qua);
static Decl * declarator(TypeAttr const* ts, TypeAttr * qua);
static TypeAttr * specifier_qualifier_list();
static Decl * aggr_declaration();
static Decl * aggr_declarator_list(TypeAttr const* ts, TypeAttr * qua);
static Decl * abstract_declarator(TypeAttr * qua);
static Decl * pointer(TypeAttr ** qua);
static INT compute_array_dim(Decl * dclr, bool allow_dim0_is_empty);
static Tree * refine_tree_list(Tree * t);
static bool isEnumTagExist(EnumTab const* entab, CHAR const* id_name,
                           OUT Enum ** e);
static INT format_base_spec(StrBuf & buf, TypeAttr const* ty);
static INT format_aggr(StrBuf & buf, TypeAttr const* ty);
static INT format_aggr(StrBuf & buf, Aggr const* s);
static INT format_aggr_complete(StrBuf & buf, Aggr const* s);
static UINT computeArrayByteSize(TypeAttr const* spec, Decl const* decl);
static void type_spec_aggr_field(Aggr * aggr, TypeAttr * ty);
static bool parse_function_definition(Decl * declaration);
static bool checkAggrComplete(Decl * decl);
static bool checkBitfield(Decl * decl);
static void fixExternArraySize(Decl * declaration);
static void add_enum(TypeAttr * ta);
static void inferAndSetEValue(EnumValueList * evals);

UINT g_decl_count = DECL_ID_UNDEF + 1;
UINT g_aggr_count = AGGR_ID_UNDEF + 1;
UINT g_aggr_anony_name_count = AGGR_ANONY_ID_UNDEF + 1;
INT g_alignment = PRAGMA_ALIGN; //default alignment.
CHAR const* g_dcl_name [] = { //character of DCL enum-type.
    "",
    "ARRAY",
    "POINTER",
    "FUN",
    "ID",
    "VARIABLE",
    "TYPE_NAME",
    "DECLARATOR",
    "DECLARATION",
    "ABS_DECLARATOR",
};


static void * xmalloc(unsigned long size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    ASSERT0(p != nullptr);
    ::memset(p, 0, size);
    return p;
}


//Complement the INT specifier.
//e.g: unsigned => unsigned int
//    register => register int
//    char => unsigned char
static void complement_qualifier(TypeAttr * attr, TypeAttr * qua)
{
    ASSERT0(attr);

    //Note C language supports declaring variable with INT if there is
    //no specifer.
    //e.g: extern a; will complement to extern int a;
    INT des = TYPE_des(attr);
    if (des == T_SPEC_UNSIGNED ||
        des == T_SPEC_SIGNED ||
        des == T_STOR_STATIC || //C supports declaring default type with INT
        des == T_STOR_EXTERN || //C supports declaring default type with INT
        des == T_STOR_REG ||
        des == T_STOR_INLINE ||
        des == T_STOR_AUTO) {
        //These types default to 'int' in C.
        SET_FLAG(TYPE_des(attr), T_SPEC_INT);
    }

    if (HAVE_FLAG(TYPE_des(attr), T_SPEC_CHAR) &&
        !HAVE_FLAG(TYPE_des(attr), T_SPEC_SIGNED)) {
        //char => unsigned char
        SET_FLAG(TYPE_des(attr), T_SPEC_UNSIGNED);
    }

    if ((qua->is_volatile() || qua->is_const()) &&
        !attr->isSimpleType()) {
        //volatile => volatile int
        SET_FLAG(TYPE_des(attr), T_SPEC_INT);
    }
}


//size: byte size.
//align: the alignment of 'size' should conform.
//       Note 0 indicates there is NO requirement to field align.
static UINT pad_align(UINT size, UINT align)
{
    ASSERT0(align > 0);
    if (size % align != 0) {
        size = (size / align + 1) * align;
    }
    return size;
}


//The function computes the byte offset after appending a field that size is
//'field_size'.
//field_align: 0 indicates there is NO requirement to field align.
static UINT compute_field_ofst_consider_pad(Aggr const* st, UINT ofst,
                                            UINT field_size, UINT elemnum,
                                            UINT field_align)
{
    //return ofst + (UINT)xcom::ceil_align(field_size, st->getAlign());
    if (field_align != 0) {
        ofst = pad_align(ofst, field_align);
    } else {
        ofst = pad_align(ofst, field_size);
    }
    ASSERTN(elemnum >= 1, ("at least one element"));
    return ofst + field_size * elemnum;
}


//Calculate byte size of pure decl-type list, but without the 'specifier'.
//There only 2 type of decl-type: pointer and array.
//e.g  Given type is: int *(*p)[3][4], and calculating the
//  size of '*(*) [3][4]'.
//  The order of decl is: p->*->[3]->[4]->*
static UINT getDeclaratorSize(TypeAttr const* spec, Decl const* d)
{
    if (d == nullptr) { return 0; }
    if (d->is_pointer()) { return BYTE_PER_POINTER; }
    if (d->is_array()) { return computeArrayByteSize(spec, d); }
    return 0;
}


//The function constructs TYPE_NAME according to given DECLARATION.
//The function just do copy if 'src' is TYPE_NAME, otherwise generate
//TYPE_NAME accroding to 'src' information.
Decl * genTypeName(Decl const* src)
{
    if (src->is_dt_typename()) {
        return dupDeclFully(src);
    }

    //Generate type_name.
    Decl * type_name = newDecl(DCL_TYPE_NAME);
    DECL_spec(type_name) = dupSpec(DECL_spec(src));

    Decl * decl_list_child = DECL_trait(src);
    ASSERT0(decl_list_child && decl_list_child->is_dt_id());

    Decl * decl_list = newDecl(DCL_ABS_DECLARATOR);
    DECL_child(decl_list) = dupDeclBeginAt(DECL_next(decl_list_child));

    DECL_decl_list(type_name) = decl_list;

    return type_name;
}


//The function constructs TYPE_NAME according to given TypeAttr.
Decl const* genTypeName(TypeAttr * ty)
{
    Decl * decl = newDecl(DCL_TYPE_NAME);
    DECL_decl_list(decl) = newDecl(DCL_ABS_DECLARATOR);
    DECL_spec(decl) = ty;
    return decl;
}


//Copy whole Decl, include all its specifier, qualifier, and declarator.
Decl * dupDeclFully(Decl const* src)
{
    Decl * res = nullptr;
    ASSERT0(src);
    if (src->is_dt_declaration() || src->is_dt_typename()) {
        res = dupDecl(src);
        DECL_spec(res) = dupSpec(DECL_spec(src));
        DECL_decl_list(res) = dupDecl(DECL_decl_list(src));
        if (DECL_decl_list(res) != nullptr) {
            ASSERT0(DECL_dt(DECL_decl_list(res)) == DCL_DECLARATOR ||
                    DECL_dt(DECL_decl_list(res)) == DCL_ABS_DECLARATOR);
            DECL_trait(res) = dupDeclBeginAt(DECL_trait(src));
        }
    } else if (DECL_dt(src) == DCL_DECLARATOR ||
               src->is_dt_abs_declarator()) {
        res = dupDecl(src);
        DECL_child(res) = dupDeclBeginAt(DECL_child(src));
    } else {
        ASSERT0(src->is_dt_array() || src->is_dt_pointer() ||
                src->is_dt_fun() || src->is_dt_id() || src->is_dt_var());
        res = dupDeclBeginAt(src);
    }
    return res;
}


//Only copy 'src', excepting its field.
Decl * dupDecl(Decl const* src)
{
    Decl * q = newDecl(DECL_dt(src));
    ::memcpy(q, src, sizeof(Decl));
    DECL_spec(q) = nullptr;
    DECL_decl_list(q) = nullptr;
    DECL_child(q) = nullptr;
    DECL_prev(q) = nullptr;
    DECL_next(q) = nullptr;
    return q;
}


//
//START TypeAttr
//
//Note the function only compare specifiers and do not consider storage.
//e.g: These two type-attrs are equivalent.
//  typedef _IO_FILE {int  _flags ; struct _IO_FILE  *  _chain ; }
//          _IO_FILE {int  _flags ; struct _IO_FILE  *  _chain ; }
bool TypeAttr::is_equal(TypeAttr const& ty) const
{
    if (this == &ty) { return true; }

    ULONG this_des = TYPE_des(this);
    ULONG des = TYPE_des(&ty);
    REMOVE_FLAG(this_des, T_STOR_TYPEDEF);
    REMOVE_FLAG(des, T_STOR_TYPEDEF);

    if (this_des != des) { return false; }

    for (UINT i = 0; i < MAX_TYPE_FLD; i++) {
        TypeAttr const* thissubta = m_sub_field[i];
        TypeAttr const* tysubta = ty.m_sub_field[i];
        if ((thissubta != nullptr) ^ (tysubta != nullptr)) {
            return false;
        }
        if (thissubta != nullptr && !thissubta->is_equal(*tysubta)) {
            return false;
        }
    }

    if (is_aggr()) {
        //TypeAttr and Decl may be link each other.
        //e.g: struct A { struct A*p; };
        return TYPE_aggr_type(this)->is_equal(*TYPE_aggr_type(&ty));
    }

    if (is_user_type_ref()) {
        return Decl::is_decl_equal(TYPE_user_type(this), TYPE_user_type(&ty));
    }

    ASSERT0(this_des == 0 || TypeAttr::isSimpleType(this_des));
    return true;
}


void TypeAttr::dump() const
{
    if (g_logmgr == nullptr) { return; }
    StrBuf buf(128);
    format_attr(buf, this, false);
    note(g_logmgr, "\n%s\n", buf.buf);
}


CHAR const* TypeAttr::getAggrTypeName() const
{
    return isStructExpanded() ? "struct" : "union";
}


//Get specifior type, specifior type refers to Non-pointer and non-array type.
// e.g: int a;
//      void a;
//      struct a;
//      union a;
//      enum a;
//      USER_DEFINED_TYPE_NAME a;
UINT TypeAttr::getSpecTypeSize() const
{
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_LONGLONG))
    { return BYTE_PER_LONGLONG; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_VOID)) { return BYTE_PER_CHAR; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_CHAR)) { return BYTE_PER_CHAR; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_BOOL)) { return BYTE_PER_CHAR; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_SHORT)) { return BYTE_PER_SHORT; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_INT)) { return BYTE_PER_INT; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_LONG)) { return BYTE_PER_LONG; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_FLOAT)) { return BYTE_PER_FLOAT; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_DOUBLE)) { return BYTE_PER_DOUBLE; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_STRUCT)) {
        return computeStructTypeSize(getAggrType());
    }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_UNION)) {
        return computeUnionTypeSize(getAggrType());
    }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_ENUM)) { return BYTE_PER_ENUM; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_SIGNED)) { return BYTE_PER_INT; }
    if (HAVE_FLAG(TYPE_des(this), T_SPEC_UNSIGNED)) { return BYTE_PER_INT; }
    return 0;
}


UINT TypeAttr::computeScalarTypeBitSize(UINT des)
{
    ULONG bitsize = 0; //the max bit size the group hold.
    switch (des) {
    case T_SPEC_CHAR|T_SPEC_UNSIGNED:
    case T_SPEC_CHAR|T_SPEC_SIGNED:
    case T_SPEC_CHAR:
        bitsize = BYTE_PER_CHAR * BIT_PER_BYTE;
        break;
    case T_SPEC_SHORT|T_SPEC_UNSIGNED:
    case T_SPEC_SHORT|T_SPEC_SIGNED:
    case T_SPEC_SHORT:
        bitsize = BYTE_PER_SHORT * BIT_PER_BYTE;
        break;
    case T_SPEC_ENUM:
    case T_SPEC_INT|T_SPEC_UNSIGNED:
    case T_SPEC_INT|T_SPEC_SIGNED:
    case T_SPEC_INT:
    case T_SPEC_SIGNED:
    case T_SPEC_UNSIGNED:
        bitsize = BYTE_PER_INT * BIT_PER_BYTE;
        break;
    case T_SPEC_LONG|T_SPEC_UNSIGNED:
    case T_SPEC_LONG|T_SPEC_SIGNED:
    case T_SPEC_LONG:
        bitsize = BYTE_PER_LONG * BIT_PER_BYTE;
        break;
    case T_SPEC_LONGLONG|T_SPEC_UNSIGNED:
    case T_SPEC_LONGLONG|T_SPEC_SIGNED:
    case T_SPEC_LONGLONG:
        bitsize = BYTE_PER_LONGLONG * BIT_PER_BYTE;
        break;
    case T_SPEC_DOUBLE:
    case T_SPEC_DOUBLE|T_SPEC_LONG:
    case T_SPEC_DOUBLE|T_SPEC_LONGLONG:
        bitsize = BYTE_PER_DOUBLE * BIT_PER_BYTE;
        break;
    case T_SPEC_FLOAT:
    case T_SPEC_FLOAT|T_SPEC_LONG:
    case T_SPEC_FLOAT|T_SPEC_LONGLONG:
        bitsize = BYTE_PER_FLOAT * BIT_PER_BYTE;
        break;
    default: UNREACHABLE();
    }
    return bitsize;
}
//END TypeAttr


//
//START Aggr
//
bool Aggr::is_equal(Aggr const& src) const
{
    if (this == &src) { return true; }
    Decl const* srcf = AGGR_decl_list(&src);
    Decl const* curf = AGGR_decl_list(this);
    for (; curf != nullptr && srcf != nullptr;
         curf = DECL_next(curf), srcf = DECL_next(srcf)) {

        //TypeAttr and Decl may be link each other.
        //e.g: struct A { struct A*p; };
        //if (!curf->is_equal(*srcf)) { return false; }
        if (!Decl::is_decl_equal(curf, srcf)) { return false; }
    }
    if (curf != nullptr || srcf != nullptr) { return false; }
    return true;
}
//END Aggr


//
//START Decl
//
bool Decl::is_equal(Decl const& src) const
{
    if (this == &src) { return true; }
    if (DECL_dt(this) != DECL_dt(&src)) { return false; }
    switch (DECL_dt(this)) {
    case DCL_NULL: ASSERT0(0);
    case DCL_ARRAY:
        if (DECL_array_dim(this) != DECL_array_dim(&src)) {
            return false;
        }
        return true;
    case DCL_POINTER:
        if ((DECL_qua(this) != nullptr) ^ (DECL_qua(&src) != nullptr)) {
            return false;
        }
        if (!DECL_qua(this)->is_equal(*DECL_qua(&src))) {
            return false;
        }
        return true;
    case DCL_FUN:
        return true;
    case DCL_ID:
        if (TREE_id_name(DECL_id_tree(this)) !=
            TREE_id_name(DECL_id_tree(&src))) {
            return false;
        }
        if ((DECL_qua(this) != nullptr) ^ (DECL_qua(&src) != nullptr)) {
            return false;
        }
        if (!DECL_qua(this)->is_equal(*DECL_qua(&src))) {
            return false;
        }
        return true;
    case DCL_VARIADIC:
        return true;
    case DCL_TYPE_NAME:
    case DCL_DECLARATOR:
    case DCL_DECLARATION:
    case DCL_ABS_DECLARATOR:
        //All of these kind of DCL are compound type.
        break;
    default: UNREACHABLE();
    }

    TypeAttr const* thista = getTypeAttr();
    TypeAttr const* srcta = DECL_spec(&src);
    if ((thista != nullptr) ^ (srcta != nullptr)) {
        return false;
    }
    if (thista != nullptr && !thista->is_equal(*srcta)) {
        return false;
    }

    Decl const* thisf = getTraitList();
    Decl const* srcf = src.getTraitList();
    for (; thisf != nullptr && srcf != nullptr;
         thisf = DECL_next(thisf), srcf = DECL_next(srcf)) {
        if (!thisf->is_equal(*srcf)) {
            return false;
        }
    }
    if (thisf != nullptr || srcf != nullptr) { return false; }
    return true;
}


bool Decl::is_restrict() const
{
    ASSERTN(is_dt_declaration(), ("needs declaration"));
    if (is_pointer()) {
        Decl const* x = get_pointer_decl();
        ASSERT0(x);
        TypeAttr const* ty = DECL_qua(x);
        if (ty != nullptr && ty->is_restrict()) {
            return true;
        }
    }
    return false;
}


//Pointer, array, struct, union are not scalar type.
bool Decl::is_scalar() const
{
    ASSERTN(get_decl_type() == DCL_DECLARATION ||
            get_decl_type() == DCL_TYPE_NAME, ("needs declaration"));
    TypeAttr const* attr = getTypeAttr();
    if (!attr->isSimpleType()) { return false; }

    Decl const* dcl = getTraitList();
    if (dcl == nullptr) { return true; }

    if (dcl->get_decl_type() == DCL_ID) {
        dcl = DECL_next(dcl);
    }

    if (dcl == nullptr) { return true; }

    ASSERT0(dcl->get_decl_type() == DCL_POINTER ||
            dcl->get_decl_type() == DCL_ARRAY ||
            dcl->get_decl_type() == DCL_FUN);
    return false;
}


bool Decl::is_local_variable() const
{
    ASSERTN(is_dt_declaration(), ("needs declaration"));
    Scope const* sc = DECL_decl_scope(this);
    ASSERTN(sc, ("variable must be allocated within a scope."));
    if (SCOPE_level(sc) >= FUNCTION_SCOPE && !is_static()) {
        return true;
    }
    return false;
}


bool Decl::is_global_variable() const
{
    ASSERTN(is_dt_declaration(), ("needs declaration"));
    Scope const* sc = DECL_decl_scope(this);
    ASSERTN(sc, ("variable must be allocated within a scope."));
    if (SCOPE_level(sc) == GLOBAL_SCOPE) {
        return true;
    }
    if (SCOPE_level(sc) >= FUNCTION_SCOPE && is_static()) {
        return true;
    }
    return false;
}

//Return true if dcl has initial value.
bool Decl::is_initialized() const
{
    ASSERTN(is_dt_declaration() || is_dt_declarator(),
            ("requires declaration"));
    Decl const* dcl = this;
    if (dcl->is_dt_declaration()) {
        dcl = DECL_decl_list(dcl); //get DCRLARATOR
        ASSERTN(DECL_dt(dcl) == DCL_DECLARATOR ||
                dcl->is_dt_abs_declarator(), ("requires declaration"));
    }
    if (DECL_is_init(dcl)) {
        return true;
    }
    return false;
}


Decl const* Decl::get_decl_id_tree() const
{
    Decl const* pdcl = getTraitList();
    while (pdcl != nullptr) {
        if (pdcl->is_dt_id()) {
            return pdcl;
        }
        pdcl = DECL_next(pdcl);
    }
    return nullptr;
}


Decl const* Decl::get_return_type() const
{
    ASSERT0(is_dt_declaration());
    Decl const* retty = genTypeName(getTypeAttr());
    Decl const* tylst = getTraitList();

    ASSERTN(tylst->is_dt_id(),
        ("'id' should be declarator-list-head. Illegal function declaration"));
    Decl * func_type = DECL_next(tylst);
    ASSERTN(func_type->is_dt_fun(), ("must be function type"));
    Decl * return_type = DECL_next(func_type);
    if (return_type == nullptr) {
        return retty;
    }

    DECL_trait(retty) = dupDeclBeginAt(return_type);
    return retty;
}


CHAR const* Decl::get_decl_name() const
{
    Sym const* sym = get_decl_sym();
    if (sym == nullptr) { return nullptr; }
    return sym->getStr();
}


Sym const* Decl::get_decl_sym() const
{
    Decl const* dcl = get_decl_id_tree();
    if (dcl != nullptr) {
        return TREE_id_name(DECL_id_tree(dcl));
    }
    return nullptr;
}


void Decl::set_decl_init_tree(Tree * initval)
{
    ASSERT0(is_dt_declaration());
    Decl * dclor = DECL_decl_list(this); //get DCRLARATOR
    ASSERTN(DECL_dt(dclor) == DCL_DECLARATOR ||
            dclor->is_dt_abs_declarator(), ("requires declaration"));
    if (initval == nullptr) {
        DECL_is_init(dclor) = false;
    } else {
        DECL_is_init(dclor) = true;
    }
    DECL_init_tree(dclor) = initval;
}


Tree * Decl::get_decl_init_tree() const
{
    ASSERT0(is_initialized());
    Decl const* dcl = this;
    if (dcl->is_dt_declaration()) {
        dcl = DECL_decl_list(dcl); //get DCRLARATOR
        ASSERTN(DECL_dt(dcl) == DCL_DECLARATOR ||
                dcl->is_dt_abs_declarator(), ("requires declaration"));
    }
    ASSERT0(DECL_is_init(dcl));
    ASSERT0(DECL_init_tree(dcl));
    return DECL_init_tree(dcl);
}


//Return true if 'd1' and 'd2' are the same identifier.
//Note d1 and d2's identifier may be NULL.
bool Decl::is_decl_equal(Decl const* d1, Decl const* d2)
{
    Scope const* s1 = DECL_decl_scope(d1);
    Scope const* s2 = DECL_decl_scope(d2);
    if (s1 == s2) {
        if (d1->get_decl_sym() == d2->get_decl_sym()) {
            return true;
        }
    }
    return false;
}


//Distinguish the declaration and definition of variable.
//Return true if 'decl' is a declaration, otherwise it is a definition.
bool Decl::is_declaration() const
{
    if (DECL_is_fun_def(this)) {
        UNREACHABLE();
    }
    return false;
}


//Pick out the pure declarator specification list
//e.g:int * a [10];
//    the pure declarator list is :  a->[10]->*
//
//    int (*) [10];
//    the pure declarator list is :  *->[10]
Decl const* Decl::getTraitList() const
{
    Decl const* decl = this;
    switch (DECL_dt(decl)) {
    case DCL_ARRAY:
    case DCL_POINTER:
    case DCL_ID:
    case DCL_FUN: //function-pointer type.
        break;
    case DCL_VARIADIC:
        ASSERTN(0, ("can not be in declaration"));
        break;
    case DCL_TYPE_NAME:
        decl = DECL_decl_list(decl);
        if (decl == nullptr) {
            return nullptr;
        }
        ASSERTN(decl->is_dt_abs_declarator(),
                ("must be DCL_ABS_DECLARATOR in TYPE_NAME"));
        decl = DECL_child(decl);
        break;
    case DCL_DECLARATOR:
    case DCL_ABS_DECLARATOR:
        decl = DECL_child(decl);
        break;
    case DCL_DECLARATION:
        decl = DECL_decl_list(decl);
        if (decl == nullptr) {
            return nullptr;
        }
        ASSERT0(DECL_dt(decl) == DCL_DECLARATOR ||
                decl->is_dt_abs_declarator());
        decl = DECL_child(decl);
        break;
    default: ASSERTN(0, ("unknown Decl"));
    }
    return decl;
}


//Return the dimension of given array.
//Note array should be DCL_DECLARATION or DCL_TYPE_NAME.
UINT Decl::getArrayDim() const
{
    ASSERT0(is_dt_declaration() || is_dt_typename());
    ASSERT0(is_array());
    Decl * dclr = const_cast<Decl*>(getTraitList());
    //Get the first ARRAY decl-type.
    while (dclr != nullptr) {
        if (dclr->is_dt_array()) { break; }
        dclr = DECL_next(dclr);
    }

    //Count the dimension.
    UINT ndim = 0;
    while (dclr != nullptr) {
        if (DECL_dt(dclr) != DCL_ARRAY) { break; }
        dclr = DECL_next(dclr);
        ndim++;
    }
    return ndim;
}


//Get the number of element to given dimension.
//Note the field DECL_array_dim of array is only
//available after compute_array_dim() invoked, and
//it compute the really number of array element via
//DECL_array_dim_exp, that is a constant expression.
//arr: array declaration.
//dim: given dimension to compute, start at 0 which is the closest and
//     highest dimension to leading ID, in decl-type list.
//     e.g: int arr[8][12][24];
//     In C language, [24] is the lowest dimension.
//     where decl-type list will be:
//       ID:'arr' -> ARRAY[8] -> ARRAY[12] -> ARRAY[24]
//     dim 0 indicates ARRAY[8], the highest dimension of 'arr'.
ULONG Decl::getArrayElemnumToDim(UINT dim) const
{
    Decl const* dcl = const_cast<Decl*>(this)->get_first_array_decl();
    ASSERT0(dcl);
    UINT i = 0;
    while (i < dim && dcl != nullptr) {
        if (DECL_dt(dcl) != DCL_ARRAY) {
            break;
        }
        dcl = DECL_next(dcl);
        i++;
    }

    if (dcl == nullptr || DECL_dt(dcl) != DCL_ARRAY) {
        return 0;
    }
    return (ULONG)DECL_array_dim(dcl);
}


void Decl::dump(StrBuf & buf) const
{
    if (g_logmgr == nullptr) { return; }
    format_declaration(buf, this, true);
    note(g_logmgr, "\n%s\n", buf.buf);
}


void Decl::dump() const
{
    format_declaration(this, g_logmgr->getIndent(), true);
}


//What is 'complex type'? Non-pointer and non-array type.
//e.g: int * a;
//     int a[];
bool Decl::is_complex_type() const
{
    Decl const* dcl = getTraitList();
    if (dcl == nullptr) { return false; }
    return dcl->is_pointer() || dcl->is_array();
}


bool Decl::is_bitfield() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("need TypeAttr-NAME or DCRLARATION"));
    Decl const* dcl = get_declarator();
    return dcl != nullptr && DECL_is_bit_field(dcl);
}


bool Decl::is_struct() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("need TypeAttr-NAME or DCRLARATION"));
    if (is_pointer() || is_array()) {
        //Complex type is consist of type-specifier and declarator.
        return false;
    }
    return getTypeAttr()->is_struct();
}


//Is dcl is array declarator,
//    for decl : int a[], the second decltor must be DCL_ARRAY,
//    the first is DCL_ID.
//    for abs-decl: int [], the first decltor must be DCL_ARRAY.
bool Decl::is_array() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_ARRAY:
            return true;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                   !dcl->is_dt_declarator() &&
                   !dcl->is_dt_abs_declarator() &&
                   !dcl->is_dt_typename(),
                   ("\nunsuitable Decl type locate here in is_array()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


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
Decl const* Decl::get_pointer_declarator() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            //function-pointer type:
            //  DCL_POINTER->DCL_FUN
            //function-decl type:
            //  DCL_ID->DCL_FUN
            if (DECL_prev(dcl) != nullptr &&
                DECL_dt(DECL_prev(dcl)) == DCL_POINTER) {
                return dcl;
            }
            return nullptr;
        case DCL_POINTER:
            return dcl;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                    !dcl->is_dt_declarator() &&
                    !dcl->is_dt_abs_declarator() &&
                    !dcl->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return nullptr;
        }
        dcl = DECL_next(dcl);
    }
    return nullptr;
}


ULONG Decl::getComplexTypeSize() const
{
    TypeAttr const* spec = getTypeAttr();
    Decl const* d = nullptr;
    if (is_dt_declaration() || is_dt_typename()) {
        d = getTraitList();
        ASSERTN(d != nullptr, ("composing type expected decl-spec"));
    } else {
        err(g_src_line_num, "expected declaration or type-name");
        return 0;
    }

    ASSERTN(spec, ("composing type expected specifier"));
    ULONG declor_size = getDeclaratorSize(spec, d);
    if (d->is_array()) {
        Decl const* base_dcl = d->getArrayBaseDeclarator();
        if (base_dcl != nullptr && base_dcl->is_dt_pointer()) {
            //If base of array is pointer, then we can disgard the base type
            //of the pointer, because all pointer size is identical.
            return declor_size * BYTE_PER_POINTER;
        }

        //The base of array could only be pointer, what else could there be?
        ASSERT0(base_dcl == nullptr);
        ULONG s = spec->getSpecTypeSize();
        return declor_size * s;
    }

    ASSERT0(d->is_pointer());
    return declor_size;
}


//Compute the byte size of declaration.
//This function will compute array size.
UINT Decl::get_decl_size() const
{
    TypeAttr const* spec = getTypeAttr();
    if (is_dt_declaration() || is_dt_typename()) {
        Decl const* d = getDeclarator(); //get declarator
        ASSERTN(d && (d->is_dt_declarator() || d->is_dt_abs_declarator()),
                ("illegal declarator"));
        if (d->is_complex_type()) {
            return getComplexTypeSize();
        }
        return spec->getSpecTypeSize();
    }
    ASSERTN(0, ("unexpected declaration"));
    return 0;
}


Decl * Decl::get_array_elem_decl() const
{
    ASSERT0(is_array());
    //Return sub-dimension of base if 'decl' is
    //multi-dimensional array.
    Decl * elemdcl = dupDeclFully(this);
    ASSERT0(DECL_trait(elemdcl));
    Decl * td = DECL_trait(elemdcl);
    if (td->is_dt_id()) {
        //e.g: If td is: ID->ARRAY->ARRAY.
        //We elide ID and keep ARRAY->ARRAY.
        td = DECL_next(td);
    }
    if (td->is_dt_array() || td->is_dt_pointer()) {
        //e.g: If td is: ARRAY->ARRAY.
        //We elide the first ARRAY, and keep the second ARRAY.
        xcom::remove(&DECL_trait(elemdcl), td);
    }
    return elemdcl;
}


//Get and generate array base declaration.
//Note current declaration must be array. If array is multiple-dimension,
//the funtion constructs return the base type by striping all dimensions.
//e.g: given int arr[10][20];
//     the function construct and return decl: 'int'.
Decl * Decl::get_array_base_decl() const
{
    ASSERT0(is_array());    
    Decl * newdecl = dupDeclFully(this);
    ASSERT0(DECL_trait(newdecl));

    //Given declator list, we elide the most outside multiple-dimension ARRAY,
    //and keep the rest and the declator of base type.
    //e.g: If trait list is: ID->ARRAY0->ARRAY1->POINTER->ARRAY2->ARRAY3.
    //  We elide ARRAY0->ARRAY1, the returned declator will be:
    //  ID->POINTER->ARRAY2->ARRAY3.
    Decl * dclor = newdecl->get_first_array_decl();
    Decl * next = nullptr;
    while (dclor != nullptr && dclor->is_dt_array()) {
        next = DECL_next(dclor);
        xcom::remove(&DECL_trait(newdecl), dclor);
        dclor = next;
    }

    return newdecl;
}


Decl * Decl::get_first_array_decl() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("expect DCRLARATION"));
    ASSERTN(is_array(), ("expect pointer type"));
    Decl * x = const_cast<Decl*>(getTraitList());
    while (x != nullptr) {
        switch (DECL_dt(x)) {
        case DCL_FUN:
        case DCL_POINTER:
            return nullptr;
        case DCL_ARRAY:
            return x;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!x->is_dt_declaration() &&
                   !x->is_dt_declarator() &&
                   !x->is_dt_abs_declarator() &&
                   !x->is_dt_typename(),
                   ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return nullptr;
        }
        x = DECL_next(x);
    }
    return nullptr;
}


Decl const* Decl::get_return_value_decl() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("expect DCRLARATION"));
    ASSERT0(is_fun_decl() || is_fun_pointer());
    for (Decl const* dcl = getTraitList();
         dcl != nullptr; dcl = DECL_next(dcl)) {
        if (dcl->is_dt_fun()) {
            //CASE:
            //  e.g: int f()
            //  ID->FUN is func-decl, there is no return-value decl.
            //  TypeAttr is 'int'.
            //
            //  e.g: void f()
            //  ID->FUN is func-decl, there is no return-value decl.
            //
            //  e.g: void ( * f() ) [], ID->FUN->*->[]
            //  ID->FUN->... is func-decl, return-value decl is a pointer.
            //
            //  e.g: void (* (* f)() ) [], ID->*->FUN->*->[]
            //  ID->*->FUN->... is NOT func-decl, it is a func-pointer, and
            //  the return-value decl is a pointer.
            return DECL_next(dcl);
        }
    }
    return nullptr;
}


Decl const* Decl::get_pointer_decl() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("expect DCRLARATION"));
    ASSERTN(is_pointer(), ("expect pointer type"));
    Decl const* x = getTraitList();
    while (x != nullptr) {
        switch (DECL_dt(x)) {
        case DCL_FUN:
            //function-pointer type:
            //    DCL_ID->POINTER->DCL_FUN
            //function-decl type:
            //    DCL_ID->DCL_FUN
            if (DECL_prev(x) != nullptr &&
                DECL_dt(DECL_prev(x)) == DCL_POINTER) {
                return DECL_prev(x);
            }
            return nullptr;
        case DCL_POINTER:
            return x;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!x->is_dt_declaration() &&
                    !x->is_dt_declarator() &&
                    !x->is_dt_abs_declarator() &&
                    !x->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return nullptr;
        }
        x = DECL_next(x);
    }
    return nullptr;
}


//Get base type of POINTER.
Decl * Decl::get_pointer_base_decl(TypeAttr ** ty) const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("expect DCRLARATION"));
    ASSERTN(is_pointer() || is_fun_return_pointer(), ("expect pointer type"));

    if (ty != nullptr) {
        *ty = getTypeAttr();
    }

    if (is_pointer()) {
        Decl * d = const_cast<Decl*>(getTraitList());
        if (d->is_dt_id()) {
            d = DECL_next(d);
            ASSERTN(d->is_dt_pointer() || d->is_dt_fun(),
                ("expect pointer declarator"));
            return DECL_next(d); //get Decl that is the heel of '*'
        }
        else if (d->is_dt_pointer() || d->is_dt_fun()) {
            return DECL_next(d); //get Decl that is the heel of '*'
        }

        ASSERTN(0, ("it is not a pointer type"));
        return nullptr;
    }

    ASSERT0(is_fun_return_pointer());
    Decl const* d = get_return_value_decl();
    ASSERT0(d->get_decl_type() == DCL_POINTER);
    return DECL_next(d); //get Decl that is the heel of '*'
}


//Compute byte size of pointer base declarator.
//e.g: Given 'int *(*p)[3]', the pointer-base is 'int * [3]'.
UINT Decl::get_pointer_base_size() const
{
    ASSERT0(is_dt_declaration() || is_dt_typename());
    if (isFunTypeRef()) {
        //Base size of function-pointer is the size of pointer.
        return BYTE_PER_POINTER;
    }

    TypeAttr * ty = nullptr;
    Decl * d = get_pointer_base_decl(&ty);
    if (d == nullptr) {
        if (ty == nullptr) { return 0; }

        //Base-type of pointer must be recoreded in TypeAttr.
        TypeAttr const* pure = ty->getPureTypeAttr();
        if (pure->is_aggr()) {
            //Try finding complete aggregate definition.
            Aggr const* aggr = Scope::retrieveCompleteType(
                pure->getAggrType(), true);
            if (aggr == nullptr || !aggr->is_complete()) {
                //The aggregate is incomplete, and bytesize is 0.
                return 0;
            }

            //Compute bytesize of complete aggregate.
            UINT s = TypeAttr::computeStructTypeSize(aggr);
            ASSERTN(s != 0, ("base-type size should not be zero"));
            return s;
        }

        UINT s = pure->getSpecTypeSize();
        ASSERTN(s != 0, ("base-type size should not be zero"));
        return s;
    }

    UINT s = 1;
    UINT e = getDeclaratorSize(getTypeAttr(), d);
    if (!d->is_pointer()) {
        s = ty->getSpecTypeSize();
    }
    ASSERTN(e != 0, ("declarator size cannot be zero"));
    return e * s;
}


bool Decl::is_union() const
{
    ASSERTN(is_dt_typename() || is_dt_declaration(),
            ("need TypeAttr-NAME or DCRLARATION"));
    if (is_pointer() || is_array()) {
        //Complex type is consist of type-specifier and declarator.
        return false;
    }
    return getTypeAttr()->is_union();
}


//Return true if the return-value type is VOID.
bool Decl::is_fun_return_void() const
{
    if (!is_fun_decl() && !is_fun_pointer()) { return false; }
    if (getTypeAttr()->is_void() &&
        get_return_value_decl() == nullptr) {
        return true;
    }
    return false;
}


//Return true if the return-value type is a pointer.
bool Decl::is_fun_return_pointer() const
{
    if (!is_fun_decl() && !is_fun_pointer()) { return false; }
    Decl const* d = get_return_value_decl();
    if (d != nullptr && d->get_decl_type() == DCL_POINTER) {
        return true;
    }
    return false;
}


//Return true if 'dcl' is function-type declaration or reference.
bool Decl::is_fun_decl() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            if (DECL_prev(dcl) == nullptr ||
                (DECL_prev(dcl) != nullptr &&
                 DECL_dt(DECL_prev(dcl)) == DCL_ID)) {
                //CASE:
                //  ID->FUN is func-decl,
                //  e.g: void f()
                //
                //  ID->FUN->... is func-decl,
                //  e.g: void ( * f() ) [], ID->FUN->*->[]
                //
                //  ID->*->FUN->... is NOT func-decl, it is a func-pointer
                //  e.g: void (* (* f)() ) [], ID->*->FUN->*->[]
                return true;
            }
            return false;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                    !dcl->is_dt_declarator() &&
                    !dcl->is_dt_abs_declarator() &&
                    !dcl->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_fun()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


//Return true if 'dcl' is function pointer variable.
bool Decl::is_fun_pointer() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            //  e.g: void f();
            //  Trait is: ID->FUN is func-declaration.
            //
            //  e.g: void ( * f() ) [];
            //  Trait is: ID->FUN->*->[]
            //  ID->FUN->... is func-declaration, where '...' is the
            //  return-value-type of function.
            //
            //  e.g: void (* (* f)() ) [];
            //  Trait is: ID->*->FUN->*->[]
            //  ID->*->FUN->... is func-pointer.
            if (DECL_prev(dcl) != nullptr &&
                DECL_dt(DECL_prev(dcl)) == DCL_POINTER) {
                return true;
            }
            return false;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                    !dcl->is_dt_declarator() &&
                    !dcl->is_dt_abs_declarator() &&
                    !dcl->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_fun()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


bool Decl::isPointerPointToArray() const
{
    if (!is_pointer()) { return false; }
    Decl const* base_decl = get_pointer_base_decl(nullptr);
    return base_decl != nullptr && base_decl->is_array();
}


//Is 'dcl' a pointer-declarator,
//e.g:Given Decl as : 'int * a', then the second decltor in the type-chain
//    must be DCL_POINTER, the first is DCL_ID 'a'.
//    And simplar for abs-decl, as an example 'int *', the first decltor
//    in the type-chain must be DCL_POINTER.
bool Decl::is_pointer() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            //function-pointer type:
            //    DCL_POINTER->DCL_FUN
            //function-decl type:
            //    DCL_ID->DCL_FUN
            if (DECL_prev(dcl) != nullptr &&
                DECL_dt(DECL_prev(dcl)) == DCL_POINTER) {
                return true;
            }
            return false;
        case DCL_POINTER:
            return true;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                    !dcl->is_dt_declarator() &&
                    !dcl->is_dt_abs_declarator() &&
                    !dcl->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


Decl const* Decl::getArrayBaseDeclarator() const
{
    Decl const* dcl = getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_ARRAY:
            while (dcl != nullptr  && dcl->is_dt_array()) {
                dcl = DECL_next(dcl);
            }
            return dcl;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            ASSERTN(!dcl->is_dt_declaration() &&
                    !dcl->is_dt_declarator() &&
                    !dcl->is_dt_abs_declarator() &&
                    !dcl->is_dt_typename(),
                    ("\nunsuitable Decl type locate here in is_array()\n"));
            return nullptr;
        }
        dcl = DECL_next(dcl);
    }
    return nullptr;
}


//Pick out the declarator.
//e.g:int * a [10];
//    the declarator is :
//      declaration
//          |->declarator
//                 |->a->[10]->*
Decl const* Decl::get_declarator() const
{
    Decl const* decl = this;
    switch (DECL_dt(decl)) {
    case DCL_TYPE_NAME:
        decl = DECL_decl_list(decl);
        ASSERTN(decl == nullptr ||
                decl->is_dt_abs_declarator(),
                ("must be DCL_ABS_DECLARATOR in TYPE_NAME"));
        return decl;
    case DCL_DECLARATOR:
    case DCL_ABS_DECLARATOR:
        return decl;
    case DCL_DECLARATION:
        decl = DECL_decl_list(decl);
        ASSERT0(decl == nullptr ||
                DECL_dt(decl) == DCL_DECLARATOR ||
                decl->is_dt_abs_declarator());
        return decl;
    default: ASSERTN(0, ("Not a declarator"));
    }
    UNREACHABLE();
    return nullptr;
}
//END Decl



//
//START CompareEnumTab
//
bool CompareEnumTab::is_less(Enum const* t1, Enum const* t2) const
{
    return t1 < t2;
}


bool CompareEnumTab::is_equ(Enum const* t1, Enum const* t2) const
{
    return t1 == t2;
}


//Note the function createKey() will modify parameter's contents, thus the
//'const' qualifier is unusable.
Enum * CompareEnumTab::createKey(Enum * t)
{
    //Enum * res = newEnum();
    //res->copy(*t);
    //return res;
    return t;
}
//END CompareEnumTab


//Duplication declarator list begin at 'header'
Decl * dupDeclBeginAt(Decl const* header)
{
    if (header == nullptr) { return nullptr; }
    Decl * newl = nullptr, * p;
    while (header != nullptr) {
        p = dupDecl(header);
        xcom::add_next(&newl, p);
        header = DECL_next(header);
    }
    return newl;
}


Decl * newDecl(DCL dcl_type)
{
    Decl * d = (Decl*)xmalloc(sizeof(Decl));
    DECL_dt(d) = dcl_type;
    DECL_id(d) = g_decl_count++;
    return d;
}


static Struct * newStruct(CHAR const* tag, bool is_complete)
{
    Struct * s = (Struct*)xmalloc(sizeof(Struct));
    if (tag != nullptr) {
        AGGR_tag(s) = g_fe_sym_tab->add(tag);
    }
    AGGR_is_complete(s) = false;
    AGGR_id(s) = g_aggr_count++;
    return s;
}


static Union * newUnion(CHAR const* tag, bool is_complete)
{
    return (Union*)newStruct(tag, is_complete);
}


//Construct declaration.
//'spec': specifier
//'declor': declarator.
Decl * newDeclaration(TypeAttr * spec, Decl * declor, Scope * sc,
                      Tree * inittree)
{
    Decl * declaration = newDecl(DCL_DECLARATION);
    DECL_decl_scope(declaration) = sc;
    DECL_spec(declaration) = spec;
    Decl * declarator = newDecl(DCL_DECLARATOR);
    DECL_child(declarator) = declor;
    DECL_decl_list(declaration) = declarator;
    if (inittree != nullptr) {
        DECL_is_init(declarator) = true;
        DECL_init_tree(declarator) = inittree;
    }
    return declaration;
}


//Construct new declaration within given scope.
Decl * newVarDecl(IN Scope * scope, CHAR const* name)
{
    Decl * declaration = newDecl(DCL_DECLARATION);
    DECL_decl_scope(declaration) = scope;

    //Make TypeAttr
    TypeAttr * ty = newTypeAttr();
    TYPE_des(ty) |= T_SPEC_VOID;
    DECL_spec(declaration) = ty;

    //Make Tree node.
    Tree * tree = allocTreeNode(TR_ID, 0);
    Sym const* sym = g_fe_sym_tab->add(name);
    TREE_id_name(tree) = sym;

    //Make DCL_DECLARATOR.
    Decl * declor = newDecl(DCL_DECLARATOR);
    Decl * id = newDecl(DCL_ID);
    DECL_id_tree(id) = tree;
    DECL_child(declor) = id;

    //
    DECL_decl_list(declaration) = declor;
    return declaration;
}


//Abstract declarator does not have ID.
bool isAbsDeclaraotr(Decl const* declarator)
{
    ASSERT0(declarator);
    declarator = declarator->getTraitList();
    if (declarator == nullptr) { return true; }

    Sym const* id = declarator->get_decl_sym();
    if (id == nullptr) { return true; }

    return false;
}


//Return true if aggregation definition is complete.
bool TypeAttr::isAggrComplete() const
{
    ASSERT0(is_aggr());
    TypeAttr * type = const_cast<TypeAttr*>(this)->getPureTypeAttr();
    return TYPE_aggr_type(type) != nullptr &&
           TYPE_aggr_type(type)->is_complete();
}


bool isStructTypeExistInCurScope(CHAR const* tag, bool is_complete,
                                 Struct ** s)
{
    Scope * sc = g_cur_scope;
    if (isAggrTypeExist((List<Aggr*>*)sc->getStructList(),
                        tag, is_complete, (Aggr**)s)) {
        return true;
    }
    return false;
}


//Is dcl a indirection declarator,
//e.g array , pointer or function pointer
static bool is_indirection(Decl const* dcl)
{
    dcl = dcl->getTraitList();
    while (dcl != nullptr) {
        switch (DECL_dt(dcl)) {
        case DCL_ARRAY:
        case DCL_POINTER:
        case DCL_FUN:
            return true;
        case DCL_ID:
        case DCL_VARIADIC:
            break;
        default:
            if (dcl->is_dt_declaration() ||
                DECL_dt(dcl) == DCL_DECLARATOR ||
                dcl->is_dt_abs_declarator() ||
                dcl->is_dt_typename()) {
               ASSERTN(0, ("\nunsuitable Decl type locate here"
                           " in is_indirection()\n"));
            }
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


//name: unique symbol for each of scope.
//dcl:   DCL_DECLARATION info
bool isDeclExistInOuterScope(CHAR const* name, OUT Decl ** dcl)
{
    for (Scope const* scope = g_cur_scope; scope != nullptr;
         scope = SCOPE_parent(scope)) {
        for (Decl * dcl_list = scope->getDeclList();
             dcl_list != nullptr;) {//declaration list
            Decl * dr = dcl_list;
            dcl_list = DECL_next(dcl_list);
            Sym const* sym = dr->get_decl_sym();
            if (sym == nullptr) {
                continue;
            }
            if (::strcmp(sym->getStr(), name) == 0) {
                *dcl = dr;
                return true;
            }
        }
    }
    return false;
}


//Return true if 'decl' is unique at a list of Decl.
bool isUniqueDecl(Decl const* decl_list, Decl const* decl)
{
    Decl const* dcl = decl_list;
    while (dcl != nullptr) {
        if (Decl::is_decl_equal(dcl, decl) && dcl != decl) {
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return true;
}


Decl * get_decl_in_scope(CHAR const* name, Scope const* scope)
{
    if (scope == nullptr) { return nullptr; }

    Decl * dcl_list = scope->getDeclList();
    while (dcl_list != nullptr) { //declaration list
        Decl * dr = dcl_list;
        dcl_list = DECL_next(dcl_list);
        Sym const* sym = dr->get_decl_sym();
        if (sym == nullptr) { continue; }
        if (::strcmp(sym->getStr(), name) == 0) {
            return dr;
        }
    }
    return nullptr;
}


//Reference an user defined type-name.
static TypeAttr * typedef_name(TypeAttr * ty)
{
    Decl * ut = nullptr;
    if (g_real_token != T_ID) { return nullptr; }
    if (!isUserTypeExistInOuterScope(g_real_token_string, &ut)) {
        return nullptr;
    }
    if (ty == nullptr) {
        ty = newTypeAttr();
    }
    TYPE_des(ty) |= T_SPEC_USER_TYPE;
    TYPE_user_type(ty)= ut;
    CParser::match(T_ID);
    return ty;
}


static INT ck_type_spec_legally(TypeAttr * ty)
{
    INT des = TYPE_des(ty);
    StrBuf buf1(64);
    StrBuf buf2(64);
    //struct or union
    BYTE c1 = (HAVE_FLAG(des, T_SPEC_STRUCT) ||
               HAVE_FLAG(des, T_SPEC_UNION)) != 0,
         c2 = HAVE_FLAG(des, T_SPEC_ENUM) != 0,
         c3 = ty->isSimpleType() != 0,
         c4 = HAVE_FLAG(des, T_SPEC_USER_TYPE) != 0;

    //signed
    if (ONLY_HAVE_FLAG(des, T_SPEC_SHORT)) {
        //des only contained SHORT
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SHORT|T_SPEC_INT)) {
        //des only contained SHORT and INT concurrent
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_SHORT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_SHORT|T_SPEC_INT)) {
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_INT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_INT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONG|T_SPEC_INT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONG|T_SPEC_INT)) {
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONGLONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONGLONG|T_SPEC_INT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG|T_SPEC_INT)) {
        //des contained SIGNED, LONG, LONG, INT concurrent
        return ST_SUCC;
    }

    //unsiged
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_SHORT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_SHORT|T_SPEC_INT)) {
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_INT)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONGLONG)) {
        return ST_SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONG)) { return ST_SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONG|T_SPEC_INT)) {
        return ST_SUCC;
    }

    if (c1 == 1 && c2 == 1) {
        err(g_real_line_num,
            "struct or union cannot compatilable with enum-type");
        return ST_ERR;
    }
    if (c1 == 1 && c3 == 1) {
        format_base_spec(buf1, ty);
        err(g_real_line_num,
            "struct or union cannot compatilable with '%s'", buf1.buf);
        return ST_ERR;
    }
    if (c1 == 1 && c4 == 1) {
        format_user_type(buf1, ty);
        err(g_real_line_num,
            "struct or union cannot compatilable with '%s'", buf1.buf);
        return ST_ERR;
    }
    if (c2 == 1 && c3 == 1) {
        format_base_spec(buf1, ty);
        err(g_real_line_num, "enum-type cannot compatilable with '%s'",
            buf1.buf);
        return ST_ERR;
    }
    if (c2 == 1 && c4 == 1) {
        format_user_type(buf1, ty);
        err(g_real_line_num, "enum-type cannot compatilable with '%s'",
            buf1.buf);
        return ST_ERR;
    }
    if (c3 == 1 && c4 == 1) {
        format_user_type(buf1, ty);
        format_base_spec(buf2, ty);
        err(g_real_line_num,
            "'%s' type cannot compatilable with '%s'", buf1.buf, buf2.buf);
        return ST_ERR;
    }
    return ST_SUCC;
}


//Extract the qualifier from 'ty' and fulfill 'qua'.
static void extract_qualifier(TypeAttr * ty, TypeAttr * qua)
{
    ASSERT0(ty && qua);
    if (ty->is_const()) {
        SET_FLAG(TYPE_des(qua), T_QUA_CONST);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_CONST);
    }
    if (ty->is_volatile()) {
        SET_FLAG(TYPE_des(qua), T_QUA_VOLATILE);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
    }
    if (ty->is_restrict()) {
        SET_FLAG(TYPE_des(qua), T_QUA_RESTRICT);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_RESTRICT);
    }
}


static void consume_tok_to_semi()
{
    while (g_real_token != T_SEMI &&
           g_real_token != T_END &&
           g_real_token != T_UNDEF) {
        CParser::match(g_real_token);
    }
    if (g_real_token == T_SEMI) {
        CParser::match(g_real_token);
    }
}


static Decl * buildAnonyDeclarator()
{
    Decl * anony_declarator = newDecl(DCL_DECLARATOR);

    CHAR tmpbuf[64];
    SNPRINTF(tmpbuf, 64, "#anony_aggr%u", g_aggr_anony_name_count++);
    Sym const* sym = g_fe_sym_tab->add(tmpbuf);
    g_cur_scope->addToSymList(sym);
    Decl * dcl = newDecl(DCL_ID);
    DECL_id_tree(dcl) = CParser::id(sym, T_ID);
    DECL_qua(dcl) = nullptr;

    DECL_child(anony_declarator) = dcl;
    return anony_declarator; 
}


static Decl * buildDeclaration(TypeAttr * attr, Decl * declarator, UINT lineno)
{
    Decl * declaration = newDecl(DCL_DECLARATION);
    DECL_spec(declaration) = attr;
    DECL_decl_list(declaration) = declarator;
    DECL_align(declaration) = g_alignment;
    DECL_decl_scope(declaration) = g_cur_scope;
    DECL_lineno(declaration) = lineno;
    return declaration;
}


//The function parses the declaration of field that belongs to aggregate.
//aggr_declaration:
//    specifier-qualifier-list struct-declarator-list;
static Decl * aggr_declaration()
{
    TypeAttr * attr = specifier_qualifier_list();
    if (attr == nullptr) {
        err(g_real_line_num,
            "miss qualifier, illegal member declaration of aggregation");
        consume_tok_to_semi();
        return nullptr;
    }

    TypeAttr * qualifier = newTypeAttr();
    extract_qualifier(attr, qualifier);

    bool is_anony_aggr = false;
    Decl * dcl_list = aggr_declarator_list(attr, qualifier);
    if (dcl_list == nullptr && attr->is_aggr()) {
        //If aggreate's declarator is NULL, it means the aggregate is
        //anonymous, and create an anonymous declaration to facilitate
        //following accessing to anonymous fields.
        dcl_list = buildAnonyDeclarator(); 
        is_anony_aggr = true;
    }

    while (dcl_list != nullptr) {
        Decl * dcl = dcl_list;
        dcl_list = DECL_next(dcl_list);
        DECL_next(dcl) = DECL_prev(dcl) = nullptr;

        Decl * declaration = newDecl(DCL_DECLARATION);
        DECL_spec(declaration) = attr;
        DECL_decl_list(declaration) = dcl;
        DECL_align(declaration) = g_alignment;
        DECL_decl_scope(declaration) = g_cur_scope;
        DECL_lineno(declaration) = g_real_line_num;
        DECL_is_anony_aggr(declaration) = is_anony_aggr;

        if (declaration->is_user_type_decl()) {
            err(g_real_line_num,
                "illegal storage class, should not use typedef in "
                "struct/union declaration.");
            continue;
        }

        if (attr->is_user_type_ref()) {
            declaration = makeupAndExpandUserType(declaration);
            DECL_align(declaration) = g_alignment;
            DECL_decl_scope(declaration) = g_cur_scope;
            DECL_lineno(declaration) = g_real_line_num;
        }

        g_cur_scope->addDecl(declaration);
    }

    if (g_real_token != T_SEMI) {
        err(g_real_line_num,
            "meet illegal '%s', expected ';' after struct field declaration",
            g_real_token_string);
    } else {
        CParser::match(T_SEMI);
    }
    return g_cur_scope->getDeclList();
}


//The function parses declarations of field that belongs to aggregate.
static Decl * aggr_declaration_list()
{
    while (g_real_token != T_RLPAREN) {
        if (CParser::isTerminateToken() || is_too_many_err()) {
            return g_cur_scope->getDeclList();
        }
        aggr_declaration();
    }
    return g_cur_scope->getDeclList();
}


static TypeAttr * type_spec_struct(TypeAttr * ty)
{
    TYPE_des(ty) |= T_SPEC_STRUCT;
    CParser::match(T_STRUCT);
    if (ck_type_spec_legally(ty) != ST_SUCC) {
        err(g_real_line_num, "type specifier is illegal");
        return ty;
    }

    INT alignment = g_alignment; //record alignment before struct declaration.
    Struct * s = nullptr;
    if (g_real_token == T_ID) {
        //Meet struct definition. And current format is:
        //  'struct' 'TAG' '{' ... '}' 'ID';
        //Find current and all of outer scope to find the
        //identical declaration or declaring.
        //C permit forward declaration, namely use first, define second.
        //e.g:struct EX * ex;
        //    struct EX { ... };
        //Here, we make an incomplete struct/union, then find the complete
        //version and refill the declaration if there are requirments to
        //access its field.
        if (!isStructExistInOuterScope(g_cur_scope,
                                       g_real_token_string, false, &s)) {
            s = newStruct(g_real_token_string, false);
            g_cur_scope->addStruct(s);
            //Note we do not append anonymous aggregate into scope list because
            //user can not find the aggregate through tag name. Thus there will
            //multiple aggregates that have same data structure layout.
        }
        CParser::match(T_ID);
    }

    if (g_real_token == T_LLPAREN) {
        if (s == nullptr) {
            //Struct format as either struct TAG {} or struct {}.
            //The struct declarated without TAG.
            s = newStruct(nullptr, false);
            AGGR_scope(s) = g_cur_scope;
        }
        if (s->is_complete()) {
            //Report error if there exist a previous declaration.
            ASSERT0(AGGR_tag(s));
            err(g_real_line_num, "struct '%s' redefined", 
                AGGR_tag(s)->getStr());
            return ty;
        }
        type_spec_aggr_field(s, ty);
    }

    if (s == nullptr) {
        //There is neither 'TAG' nor '{'.
        err(g_real_line_num, "illegal use '%s'", g_real_token_string);
        return ty;
    }

    //We must update alignment always, because user may apply #pragma directive
    //anywhere.
    //e.g:#pragma align (4)
    //    struct A a1;
    //    ...
    //    #pragma align (8)
    //    struct A a2;
    //    ...
    //  In actually, a1 and a2 are implemented in different alignment.
    AGGR_align(s) = alignment;

    TYPE_aggr_type(ty) = s;
    return ty;
}


static void type_spec_aggr_field(Aggr * aggr, TypeAttr * ty)
{
    ASSERT0(aggr);
    CParser::match(T_LLPAREN);
    push_scope(false);

    //UINT errn = g_err_msg_list.get_elem_count();
    AGGR_decl_list(aggr) = aggr_declaration_list();
    if (AGGR_decl_list(aggr) == nullptr) {
        //Empty field list, for compiler convenient, insert one byte field.
        Decl * var = newVarDecl(g_cur_scope, "#placeholder");
        AGGR_decl_list(aggr) = var;
    }
    pop_scope();
    //if (g_err_msg_list.get_elem_count() == errn) {
    //    AGGR_is_complete(aggr) = true;
    //}

    //Numbering field id.
    UINT i = 0;
    for (Decl * field = AGGR_decl_list(aggr);
         field != nullptr; field = DECL_next(field)) {
        DECL_fieldno(field) = i++;
        DECL_is_sub_field(field) = true;
        DECL_base_type_spec(field) = ty;
    }

    if (CParser::match(T_RLPAREN) != ST_SUCC) {
        err(g_real_line_num, "expected '}' after %s definition",
            ty->getAggrTypeName());
        return;
    }
    AGGR_is_complete(aggr) = true;
}


static TypeAttr * type_spec_union(TypeAttr * ty)
{
    TYPE_des(ty) |= T_SPEC_UNION;
    CParser::match(T_UNION);
    if (ck_type_spec_legally(ty) != ST_SUCC) {
        err(g_real_line_num, "type specifier is illegal");
        return ty;
    }

    INT alignment = g_alignment; //record alignment before union declaration.
    Union * s = nullptr;
    if (g_real_token == T_ID) {
        //union definition
        //format is: 'union' 'TAG' '{' ... '}' 'ID';
        //Find current and all of outer scope to find the
        //identical declaration or declaring, and refill the
        //declaration which has been declared before.
        //C permit forward declaration, namely use first, define second.
        //e.g:union EX * ex;
        //    union EX { ... };
        //Here, we make an incomplete struct/union, then find the complete
        //versionand refill the declaration if there are requirments to
        //access its field.
        if (!isUnionExistInOuterScope(g_cur_scope, g_real_token_string,
                                      false, &s)) {
            s = newUnion(g_real_token_string, false);
            g_cur_scope->addUnion(s);
        }
        CParser::match(T_ID);
    }

    if (g_real_token == T_LLPAREN) {
        //'s' is a incomplete union declaration,
        //and it is permitted while we define a
        //non-pointer variable as member of 's'.
        if (s == nullptr) {
            //Union format as either union TAG {} or union {}.
            //The union declarated without TAG.
            s = newUnion(nullptr, false);
            AGGR_scope(s) = g_cur_scope;
            //Note we do not append anonymous aggregate into scope list because
            //user can not find the aggregate through tag name. Thus there will
            //multiple aggregates that have same data structure layout.
        }
        if (s->is_complete()) {
            //Report error if there exist a previous declaration.
            ASSERT0(AGGR_tag(s));
            err(g_real_line_num, "union '%s' redefined", AGGR_tag(s)->getStr());
            return ty;
        }
        type_spec_aggr_field(s, ty);
    }

    if (s == nullptr) {
        //There is neither 'TAG' nor '{'.
        err(g_real_line_num, "illegal use '%s'", g_real_token_string);
        return ty;
    }

    //We must change alignment always, because user
    //may apply #pragma align anywhere.
    //e.g
    //    #pragma align (4)
    //    union A{...} a1;
    //    ...
    //    #pragma align (8)
    //    union A a2;
    //    ...
    //So, a1 and a2 are implement as different alignment!
    AGGR_align(s) = alignment;

    TYPE_aggr_type(ty) = s;
    return ty;
}


static TypeAttr * type_spec(TypeAttr * ty)
{
    if (ty == nullptr) {
        ty = newTypeAttr();
    }
    switch (g_real_token) {
    case T_VOID:
        CParser::match(T_VOID);
        SET_FLAG(TYPE_des(ty), T_SPEC_VOID);
        break;
    case T_CHAR:
        CParser::match(T_CHAR);
        SET_FLAG(TYPE_des(ty), T_SPEC_CHAR);
        break;
    case T_SHORT:
        CParser::match(T_SHORT);
        SET_FLAG(TYPE_des(ty), T_SPEC_SHORT);
        break;
    case T_INT:
        CParser::match(T_INT);
        SET_FLAG(TYPE_des(ty), T_SPEC_INT);
        break;
    case T_LONG:
        CParser::match(T_LONG);
        if (ty->is_long()) {
            //It seems longlong might confuse user.
            //warn("'long long' is not ANSI C89, "
            //      "using longlong as an alternative");
            REMOVE_FLAG(TYPE_des(ty), T_SPEC_LONG);
            SET_FLAG(TYPE_des(ty), T_SPEC_LONGLONG);
        } else if (ty->is_longlong()) {
            err(g_real_line_num, "type specifier is illegal");
            return ty;
        } else {
            SET_FLAG(TYPE_des(ty), T_SPEC_LONG);
        }
        break;
    case T_LONGLONG:
        CParser::match(T_LONGLONG);
        SET_FLAG(TYPE_des(ty), T_SPEC_LONGLONG);
        break;
    case T_BOOL:
        CParser::match(T_BOOL);
        SET_FLAG(TYPE_des(ty), T_SPEC_BOOL);
        break;
    case T_FLOAT:
        CParser::match(T_FLOAT);
        SET_FLAG(TYPE_des(ty), T_SPEC_FLOAT);
        break;
    case T_DOUBLE:
        CParser::match(T_DOUBLE);
        SET_FLAG(TYPE_des(ty), T_SPEC_DOUBLE);
        break;
    case T_SIGNED:
        CParser::match(T_SIGNED);
        SET_FLAG(TYPE_des(ty), T_SPEC_SIGNED);
        break;
    case T_UNSIGNED:
        CParser::match(T_UNSIGNED);
        SET_FLAG(TYPE_des(ty), T_SPEC_UNSIGNED);
        break;
    case T_STRUCT:
        return type_spec_struct(ty);
    case T_UNION:
        return type_spec_union(ty);
    default:; //do nothing
    }
    return ty;
}


//enumerator:
//  identifier
//  identifier = constant_expression
static EnumValueList * enumrator(Enum * en)
{
    if (g_real_token != T_ID) { return nullptr; }

    LONGLONG idx = 0;
    EnumValueList * evl = (EnumValueList*)xmalloc(sizeof(EnumValueList));
    EVAL_name(evl) = g_fe_sym_tab->add(g_real_token_string);

    Enum * tmp = nullptr;
    if (g_cur_scope->isEnumExist(g_real_token_string, &tmp, (INT*)&idx)) {
        err(g_real_line_num, "'%s' : redefinition , different basic type",
            g_real_token_string);
        return evl;
    }

    xcom::add_next(&ENUM_vallist(en), evl);
 
    CParser::match(T_ID);
    if (g_real_token != T_ASSIGN) {
        inferAndSetEValue(en->getValList());
        return evl;
    }

    CParser::match(T_ASSIGN);

    //constant expression
    if (CParser::inFirstSetOfExp(g_real_token)) {
        Tree * t = CParser::conditional_exp();
        LONGLONG val;
        if (t == nullptr || !computeConstExp(t, &val, 0)) {
            err(g_real_line_num, "expected constant expression");
            return evl;
        }
        EVAL_val(evl) = (INT)val;
        EVAL_is_evaluated(evl) = true;
        return evl;
    }

    err(g_real_line_num,
        "syntax error : constant expression cannot used '%s'",
        g_real_token_string);
    return evl;
}


//enumerator_list:
//  enumerator
//  enumerator_list , enumerator
static void enumerator_list(Enum * en)
{
    EnumValueList * ev = enumrator(en);
    if (ev == nullptr) { return; }

    while (g_real_token == T_COMMA) {
        CParser::match(T_COMMA);
        EnumValueList * nev = enumrator(en);
        if (nev == nullptr) { break; }
    }
}


//enum_specifier:
//  enum identifier { enumerator_list }
//  enum            { enumerator_list }
//  enum identifier
static TypeAttr * enum_spec(TypeAttr * ty)
{
    if (ty == nullptr) { ty = newTypeAttr(); }
    if (ty->getEnumType() == nullptr) { TYPE_enum_type(ty) = newEnum(); }

    TYPE_des(ty) |= T_SPEC_ENUM;
    CParser::match(T_ENUM);

    if (g_real_token == T_ID) {
        //Parse enumerator's name. Note the name is optional.
        ENUM_name(ty->getEnumType()) = g_fe_sym_tab->add(g_real_token_string);
        CParser::match(T_ID);
    }

    if (g_real_token != T_LLPAREN) { return ty; }

    CParser::match(T_LLPAREN);

    //Check enum-name if meet enumerator definition.
    //Note the enum-name is optional.
    //In C, enum-name redeclaration is allowed. e.g:enum A; ... enum A;
    //However, enum redefinition is illegal. e.g:enum A{}; ... enum A{};
    Enum * e = nullptr;
    Sym const* enumname = ty->getEnumType()->getName();
    if (enumname != nullptr &&
        isEnumTagExistInOuterScope(enumname->getStr(), &e)) {
        ASSERT0(e);
        if (e->is_complete()) {
            err(g_real_line_num, "'%s' : enum type redefinition",
                enumname->getStr());
            return ty;
        } else {
            //enum declaration.
            //e.g: typedef enum tagX X;
        }
    }

    //Infer the enum-value after parsing whole enum-item rather than now.
    add_enum(ty);

    //Because enum-item may referrence the value that declared
    //before, passing whole enumertor to parse new enum-item.
    //e.g: enum { v1, v2, v3=v1; }; //v3 referred value of v1.
    enumerator_list(ty->getEnumType());

    ENUM_is_complete(ty->getEnumType()) = true;

    if (CParser::match(T_RLPAREN) != ST_SUCC) {
        err(g_real_line_num, "miss '}' during enum type declaring");
    }
    return ty;
}


//qualifier:  one of
//  const
//  volatile
static TypeAttr * qualifier(IN TypeAttr * ty)
{
    if (ty == nullptr) {
        ty = newTypeAttr();
    }
    switch (g_real_token) {
    case T_CONST:
        CParser::match(T_CONST);
        if (ty->is_const()) {
            err(g_real_line_num, "same type qualifier used more than once");
            return ty;
        }
        #if (ALLOW_CONST_VOLATILE == 1)
        SET_FLAG(TYPE_des(ty), T_QUA_CONST);
        #else
        if (ty->is_volatile()) {
            err(g_real_line_num, "variable can not both const and volatile");
            return ty;
        }
        REMOVE_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
        SET_FLAG(TYPE_des(ty), T_QUA_CONST);
        #endif
        break;
    case T_VOLATILE:
        CParser::match(T_VOLATILE);
        if (ty->is_volatile()) {
            err(g_real_line_num, "same type qualifier used more than once");
            return ty;
        }

        //If there exist 'const' spec, so 'volatile' is omitted.
        #if (ALLOW_CONST_VOLATILE == 1)
        SET_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
        #else
        if (ty->is_const()) {
            err(g_real_line_num, "variable can not both const and volatile");
            return ty;
        }
        #endif
        break;
    case T_RESTRICT:
        CParser::match(T_RESTRICT);
        SET_FLAG(TYPE_des(ty), T_QUA_RESTRICT);
        break;
    default:;
    }
    return ty;
}


//storage_class_specifier:  one of
//  auto
//  register
//  static
//  extern
//  inline
//  typedef
static TypeAttr * stor_spec(IN TypeAttr * ty)
{
    if (ty == nullptr) { ty = newTypeAttr(); }

    if ((HAVE_FLAG(TYPE_des(ty), T_STOR_AUTO) &&
         g_real_token != T_AUTO) ||
        (!ONLY_HAVE_FLAG(TYPE_des(ty), T_STOR_AUTO) &&
         g_real_token == T_AUTO)) {
        err(g_real_line_num,
            "auto can not specified with other type-specifier");
        return nullptr;
    }

    if ((HAVE_FLAG(TYPE_des(ty), T_STOR_STATIC) &&
         g_real_token == T_EXTERN) ||
        (HAVE_FLAG(TYPE_des(ty), T_STOR_EXTERN) &&
         g_real_token == T_STATIC)) {
        err(g_real_line_num,
            "static and extern can not be specified meanwhile");
        return nullptr;
    }

    switch (g_real_token) {
    case T_AUTO:
        CParser::match(T_AUTO);
        SET_FLAG(TYPE_des(ty), T_STOR_AUTO);
        break;
    case T_REGISTER:
        CParser::match(T_REGISTER);
        SET_FLAG(TYPE_des(ty), T_STOR_REG);
        break;
    case T_STATIC:
        CParser::match(T_STATIC);
        SET_FLAG(TYPE_des(ty), T_STOR_STATIC);
        break;
    case T_EXTERN:
        CParser::match(T_EXTERN);
        SET_FLAG(TYPE_des(ty), T_STOR_EXTERN);
        break;
    case T_INLINE:
        CParser::match(T_INLINE);
        SET_FLAG(TYPE_des(ty), T_STOR_INLINE);
        break;
    case T_TYPEDEF:
        CParser::match(T_TYPEDEF);
        SET_FLAG(TYPE_des(ty), T_STOR_TYPEDEF);
        break;
    default:;
    }
    return ty;
}


//Parse user defined type.
static TypeAttr * parseUserType(TypeAttr * ty, bool * parse_finish)
{
    //Do some prechecking of TypeAttr.
    if (ty != nullptr) {
        if (ty->is_user_type_ref()) {
            err(g_real_line_num, "redeclared user defined type.");
            *parse_finish = true;
            return ty;
        }

        if (ty->is_aggr()) {
            err(g_real_line_num, "redeclared %s type.",
                ty->getAggrTypeName());
            *parse_finish = true;
            return ty;
        }
    }

    //Process user-defined-type.
    TypeAttr * p = typedef_name(ty);
    ASSERT0(p);
    return p;
}


//Parse user defined aggragate.
static TypeAttr * parseAggrType(TypeAttr * ty, bool * parse_finish,
                                Aggr * s, bool is_struct)
{
    if (ty != nullptr) {
        if (ty->is_user_type_ref()) {
            err(g_real_line_num, "redeclared user defined type.");
            *parse_finish = true;
            return ty;
        }

        if (ty->is_aggr()) {
            if (ty->is_typedef()) {
                if (!TYPE_aggr_type(ty)->is_equal(*s)) {
                    err(g_real_line_num,
                        "re-typedef %s with different contents.",
                        ty->getAggrTypeName());
                    *parse_finish = true;
                    return ty;
                } else {
                    ; //Re-typedef is allowed if contents is the same.
                }
            } else {
                err(g_real_line_num, "redeclared %s type.",
                    ty->getAggrTypeName());
                *parse_finish = true;
                return ty;
            }
        }
    }

    ASSERT0(s);
    if (ty == nullptr) {
        ty = newTypeAttr();
    }
    TYPE_des(ty) |= is_struct ? T_SPEC_STRUCT : T_SPEC_UNION;
    TYPE_aggr_type(ty) = s;
    CParser::match(T_ID);
    return ty;
}


//parse_finish: record the result of the parsing. Set to true if the parsing
//              of type-specifier should finish when the function return.
static TypeAttr * parseSpecifierOrId(TypeAttr * ty, bool * parse_finish)
{
    Decl * ut = nullptr;
    if (isUserTypeExistInOuterScope(g_real_token_string, &ut)) {
        return parseUserType(ty, parse_finish);
    }

    Struct * s = nullptr;
    if (isStructExistInOuterScope(g_cur_scope, g_real_token_string,
                                  false, &s)) {
        return parseAggrType(ty, parse_finish, s, true);
    }

    Union * u = nullptr;
    if (isUnionExistInOuterScope(g_cur_scope, g_real_token_string,
                                 false, &u)) {
        return parseAggrType(ty, parse_finish, u, false);
    }

    //g_real_token is not a specifier, may be error occurred, the
    //current parsing have to finish.
    *parse_finish = true;

    return ty;
}


static bool isLegalTypeAttr(TypeAttr const* ty)
{
    ASSERT0(ty);
    DesSet basic_scalar_ds = T_SPEC_VOID|T_SPEC_CHAR|T_SPEC_SHORT|T_SPEC_INT|
                             T_SPEC_LONGLONG|
                             T_SPEC_BOOL|T_SPEC_LONG|T_SPEC_FLOAT|T_SPEC_DOUBLE|
                             T_SPEC_SIGNED|T_SPEC_UNSIGNED;
    DesSet scalar_ds = T_SPEC_VOID|T_SPEC_CHAR|T_SPEC_SHORT|T_SPEC_INT|
                       T_SPEC_LONGLONG|
                       T_SPEC_BOOL|T_SPEC_LONG|T_SPEC_FLOAT|T_SPEC_DOUBLE|
                       T_SPEC_SIGNED|T_SPEC_UNSIGNED|T_SPEC_ENUM;
    DesSet aggr_ds = T_SPEC_STRUCT|T_SPEC_UNION;
    DesSet stor_ds = T_STOR_AUTO|T_STOR_REG|T_STOR_STATIC|
                     T_STOR_EXTERN|T_STOR_INLINE|T_STOR_TYPEDEF;

    switch (g_real_token) {
    case T_AUTO:
    case T_REGISTER:
    case T_STATIC:
    case T_EXTERN:
    case T_INLINE:
    case T_TYPEDEF:
        if (HAVE_FLAG(ty->getDes(), stor_ds)) {
            err(g_real_line_num, "multiple '%s' used", g_real_token_string);
            return false;
        }
        break;
    case T_VOID:
    case T_CHAR:
    case T_SHORT:
    case T_INT:
    case T_LONGLONG:
    case T_BOOL:
    case T_LONG:
    case T_FLOAT:
    case T_DOUBLE:
    case T_SIGNED:
    case T_UNSIGNED:
        if (HAVE_FLAG(ty->getDes(), aggr_ds) ||
            HAVE_FLAG(ty->getDes(), T_SPEC_ENUM) ||
            ty->is_user_type_ref()) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    case T_STRUCT:
    case T_UNION:
        if (HAVE_FLAG(ty->getDes(), scalar_ds) || ty->is_user_type_ref()) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    case T_ENUM:
        if (HAVE_FLAG(ty->getDes(), basic_scalar_ds) ||
            HAVE_FLAG(ty->getDes(), aggr_ds) ||
            ty->is_user_type_ref()) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    case T_CONST:
        if (HAVE_FLAG(ty->getDes(), T_QUA_VOLATILE)) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    case T_VOLATILE:
        if (HAVE_FLAG(ty->getDes(), T_QUA_CONST)) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    case T_RESTRICT:        
        break;
    case T_ID:
        if (HAVE_FLAG(ty->getDes(), scalar_ds) ||
            HAVE_FLAG(ty->getDes(), aggr_ds) ||
            ty->is_user_type_ref()) {
            StrBuf buf(16);
            format_attr(buf, ty, true);
            err(g_real_line_num, "'%s' is conflict with '%s'",
                g_real_token_string, buf.buf);
            return false;
        }
        break;
    default:;
    }
    return true;
}


//declaration_specifiers:
//    storage_class_specifier declaration_specifiers
//    storage_class_specifier
//    type_specifier declaration_specifiers
//    type_specifier
//    qualifier declaration_specifiers
//    qualifier
static TypeAttr * declaration_spec()
{
    TypeAttr * ty = nullptr;
    for (;;) {
        if (ty != nullptr && ty->is_user_type_ref() && g_real_token == T_ID) {
            //Complete type-attr parsing.
            return ty;
        }
        if (ty != nullptr && !isLegalTypeAttr(ty)) { return ty; }

        switch (g_real_token) {
        case T_AUTO:
        case T_REGISTER:
        case T_STATIC:
        case T_EXTERN:
        case T_INLINE:
        case T_TYPEDEF:
            ty = stor_spec(ty);
            break;
        case T_VOID:
        case T_CHAR:
        case T_SHORT:
        case T_INT:
        case T_LONGLONG:
        case T_BOOL:
        case T_LONG:
        case T_FLOAT:
        case T_DOUBLE:
        case T_SIGNED:
        case T_UNSIGNED:
            ty = type_spec(ty);
            break;
        case T_STRUCT:
        case T_UNION:
            return type_spec(ty);
        case T_ENUM:
            ty = enum_spec(ty);
            break;
        case T_CONST:
        case T_VOLATILE:
        case T_RESTRICT:
            ty = qualifier(ty);
            break;
        case T_ID: {
            bool parse_finish = false;
            ty = parseSpecifierOrId(ty, &parse_finish);
            if (parse_finish) { return ty; }
            break;
        }
        default: goto END;
        }
    }
END:
    return ty;
}


//'fun_dclor': parameter list.
Decl * get_parameter_list(Decl * dcl, OUT Decl ** fun_dclor)
{
    dcl = const_cast<Decl*>(dcl->getTraitList());
    while (dcl != nullptr && DECL_dt(dcl) != DCL_FUN) {
        dcl = DECL_next(dcl);
    }

    if (fun_dclor != nullptr) {
        *fun_dclor = dcl;
    }

    return DECL_fun_para_list(dcl);
}


//parameter_declaration:
//    declaration_specifiers declarator
//    declaration_specifiers abstract_declarator
//    declaration_specifiers
static Decl * parameter_declaration()
{
    Decl * declaration = newDecl(DCL_DECLARATION);
    TypeAttr * attr = declaration_spec();
    if (attr == nullptr) {
        return nullptr;
    }

    TypeAttr * qualifier = newTypeAttr();

    //Extract qualifier, and regarded it as the qualifier
    //to the subsequently POINTER or ID.
    extract_qualifier(attr, qualifier);
    complement_qualifier(attr, qualifier);

    //'DCL_ID' should be the list-head if it exist.
    Decl * dcl_list = reverse_list(abstract_declarator(qualifier));

    DECL_spec(declaration) = attr;

    if (dcl_list == nullptr ||
        (dcl_list != nullptr && dcl_list->is_dt_id())) {
        DECL_decl_list(declaration) = newDecl(DCL_DECLARATOR);
    } else {
        DECL_decl_list(declaration) = newDecl(DCL_ABS_DECLARATOR);
    }

    DECL_trait(declaration) = dcl_list;

    //array parameter has at least one elem.
    compute_array_dim(declaration, false);

    if (attr->is_user_type_ref()) {
        //Factor the user defined type which via typedef.
        declaration = makeupAndExpandUserType(declaration);
        DECL_align(declaration) = g_alignment;
        DECL_decl_scope(declaration) = g_cur_scope;
        DECL_lineno(declaration) = g_real_line_num;
    }

    return declaration;
}


//parameter_type_list:
//    parameter_list
//    parameter_list , ...
//parameter_list:
//    parameter_declaration
//    parameter_list , parameter_declaration
//The above bnf can covert to
//
//parameter_type_list:
//    parameter_declaration
//    parameter_declaration , parameter_declaration
//    parameter_declaration , ...
static Decl * parameter_type_list()
{
    Decl * declaration = nullptr;
    for (;;) {
        Decl * t = parameter_declaration();
        if (t == nullptr) {
            return declaration;
        }
        xcom::add_next(&declaration, t);

        if (g_real_token == T_COMMA) {
            CParser::match(T_COMMA);
        } else if (g_real_token == T_RPAREN ||
                   CParser::isTerminateToken() ||
                   is_too_many_err()) {
            break;
        }

        //'...' must be the last parameter-declarator
        if (g_real_token == T_DOTDOTDOT) {
            CParser::match(T_DOTDOTDOT);
            t = newDecl(DCL_VARIADIC);
            xcom::add_next(&declaration, t);
            break;
        }
    }
    return declaration;
}


//direct_abstract_declarator:
//    ( abstract_declarator )
//    direct_abstract_declarator [ constant_expression ]
//                               [ constant_expression ]
//    direct_abstract_declarator [                     ]
//                               [                     ]
//                               (                     )
//                               ( parameter_type_list )
//    direct_abstract_declarator (                     )
//    direct_abstract_declarator ( parameter_type_list )
static Decl * direct_abstract_declarator(TypeAttr * qua)
{
    Decl * dcl = nullptr, * ndcl = nullptr;
    switch (g_real_token) {
    case T_LPAREN: //'(' abstract_declarator ')'
        CParser::match(T_LPAREN);
        dcl = abstract_declarator(qua);
        //Here 'dcl' can be NUL L
        if (CParser::match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')'");
            return dcl;
        }
        DECL_is_paren(dcl) = 1;
        break;
    case T_ID: { //identifier
        Sym const* sym = g_fe_sym_tab->add(g_real_token_string);
        g_cur_scope->addToSymList(sym);
        dcl = newDecl(DCL_ID);
        DECL_id_tree(dcl) = CParser::id();
        DECL_qua(dcl) = qua;
        CParser::match(T_ID);
        break;
    }
    default:;
    }

    switch (g_real_token) {
    case T_LSPAREN: { //outer level operator is ARRAY
        Tree * t = nullptr;
        while (g_real_token == T_LSPAREN) {
            CParser::match(T_LSPAREN);
            Decl * ndcl2 = newDecl(DCL_ARRAY);
            t = CParser::conditional_exp();
            if (CParser::match(T_RSPAREN) != ST_SUCC) {
                err(g_real_line_num, "miss ']'");
                return dcl;
            }
            DECL_array_dim_exp(ndcl2) = t;

            //'id' should be the last one in declarator-list.
            xcom::insertbefore_one(&dcl, dcl, ndcl2);
        }
        break;
    }
    case T_LPAREN: {
        //current level operator is function-pointer/function-definition
        //Parameter list.
        CParser::match(T_LPAREN);
        ndcl = newDecl(DCL_FUN);
        //DECL_fun_base(ndcl) = dcl;
        push_scope(true);

        //Check if param declaration is void, such as: foo(void).
        Decl * param_decl = parameter_type_list();
        if (xcom::cnt_list(param_decl) == 1 &&
            param_decl->is_any() &&
            param_decl->is_scalar()) {
            ;
        } else {
            DECL_fun_para_list(ndcl) = param_decl;
        }

        pop_scope();
        xcom::insertbefore_one(&dcl, dcl, ndcl);
        if (CParser::match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')'");
            return dcl;
        }
        break;
    }
    default:;
    }
    return dcl;
}


//abstract_declarator:
//    pointer
//    pointer direct_abstract_declarator
//            direct_abstract_declarator
static Decl * abstract_declarator(TypeAttr * qua)
{
    Decl * ptr = pointer(&qua);
    Decl * dcl = direct_abstract_declarator(qua);
    if (ptr == nullptr && dcl == nullptr) {
        return nullptr;
    }
    if (dcl == nullptr) {
        return ptr;
    }
    //Keep DCL_ID is the last one if it exist.
    //e.g:
    //    ptr is '*', dcl is '[]'->'ID'
    //    return: '*'->'[]'->'ID'
    xcom::insertbefore(&dcl, dcl, ptr);
    return dcl;
}


//specifier_qualifier_list:
//    type_specifier specifier_qualifier_list
//    type_specifier
//    qualifier specifier_qualifier_list
//    qualifier
static TypeAttr * specifier_qualifier_list()
{
    TypeAttr * ty = nullptr;
    TypeAttr * p = nullptr;
    for (;;) {
        switch (g_real_token) {
        case T_VOID:
        case T_CHAR:
        case T_SHORT:
        case T_INT:
        case T_LONGLONG:
        case T_BOOL:
        case T_LONG:
        case T_FLOAT:
        case T_DOUBLE:
        case T_SIGNED:
        case T_UNSIGNED:
        case T_STRUCT:
        case T_UNION:
            ty = type_spec(ty);
            break;
        case T_ENUM:
            ty = enum_spec(ty);
            break;
        case T_CONST:
        case T_VOLATILE:
            ty = qualifier(ty);
            break;
        case T_ID:
            p = typedef_name(ty);
            if (p == nullptr) { return ty; }
            ty = p;
            break;
        default: goto END;
        }
    }
END:
    return ty;
}


//type_name:
//    specifier_qualifier_list abstract_declarator
//    specifier_qualifier_list
//NOTICE: Do not include user defined type
Decl * type_name()
{
    //Parse specifier and qualifier.
    //e.g: char * const*, here 'char' is specifier, '* const*' is qualifier.
    TypeAttr * attr = specifier_qualifier_list();
    if (attr == nullptr) {
        return nullptr;
    }

    //Parse POINTER/ARRAY/ID, and complement their qualifier.
    //Collect const/volatile prefix, add them to POINTER/ARRAY/ID.
    //e.g: const int a; Add const qualifier to ID 'a'.
    //    const int * a; Add const qualifier to POINTER '*'.
    TypeAttr * qualifier = newTypeAttr();
    extract_qualifier(attr, qualifier);
    Decl * abs_decl = abstract_declarator(qualifier);

    //Generate type_name.
    Decl * type_name = newDecl(DCL_TYPE_NAME);
    DECL_spec(type_name) = attr;
    DECL_decl_list(type_name) = newDecl(DCL_ABS_DECLARATOR);
    DECL_trait(type_name) = xcom::reverse_list(abs_decl);
    complement_qualifier(attr, qualifier);
    compute_array_dim(type_name, false);
    return type_name;
}


//initializer_list:
//    initializer
//    initializer_list , initializer
static Tree * initializer_list(TypeAttr * qua)
{
    Tree * t = initializer(qua);
    if (t == nullptr) { return nullptr; }

    Tree * last = get_last(t);
    while (g_real_token == T_COMMA) {
        CParser::match(T_COMMA);
        if (g_real_token == T_RLPAREN) { break; }

        Tree * nt = initializer(qua);
        if (nt == nullptr) { break; }

        xcom::add_next(&t, &last, nt);
        last = get_last(nt);
    }
    return t;
}


//initializer:
//    assignment_expression
//    { initializer_list }
//    { initializer_list , }
static Tree * initializer(TypeAttr * qua)
{
    Tree * t = nullptr, * es = nullptr;
    switch (g_real_token) {
    case T_LLPAREN: {
        UINT lineno = g_real_line_num;
        CParser::match(T_LLPAREN);
        t = initializer_list(qua);
        if (g_real_token == T_COMMA) {
            CParser::match(T_COMMA);
            if (CParser::match(T_RLPAREN) != ST_SUCC) {
                err(g_real_line_num, "syntax error '%s'", g_real_token_string);
                return t;
            }
        } else if (CParser::match(T_RLPAREN) != ST_SUCC) {
            err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
            return t;
        }
        es = allocTreeNode(TR_INITVAL_SCOPE, lineno);
        TREE_initval_scope(es) = t;
        t = es;
        return t;
    }
    default:
        if (CParser::inFirstSetOfExp(g_real_token)) {
            return CParser::exp();
        }
        if (g_real_token == T_RLPAREN) {
            //An empty {}.
            return nullptr;
        }
        err(g_real_line_num,
            "syntax error : initializing cannot used '%s'",
            g_real_token_string);
    }
    return t;
}


//aggr_declarator:
//    declarator
//               : constant_expression
//    declarator : constant_expression
static Decl * aggr_declarator(TypeAttr const* ts, TypeAttr * qua)
{
    LONGLONG idx = 0;
    Decl * dclr = declarator(ts, qua);
    if (dclr == nullptr) {
        if (g_real_token == T_COLON) {
            err(g_real_line_num, "miss identifier in bit-field declaration");
        }
        return nullptr;
    }

    dclr = reverse_list(dclr);
    Decl * declarator = newDecl(DCL_DECLARATOR);
    DECL_child(declarator) = dclr;
    compute_array_dim(declarator, true);

    if (g_real_token == T_COLON) {
        //Prase bit field in aggregate.
        Tree * t = nullptr;
        if (is_indirection(dclr)) {
            Sym const* s = dclr->get_decl_sym();
            ASSERTN(s != nullptr, ("member name cannot be nullptr"));
            err(g_real_line_num,
                "'%s' : pointer type cannot assign bit length", s->getStr());
            return declarator;
        }
        CParser::match(T_COLON);
        t = CParser::conditional_exp();
        if (!computeConstExp(t, &idx, 0)) {
            err(g_real_line_num, "expected constant expression");
            return declarator;
        }

        //bit length must be check in typeck.cpp
        DECL_bit_len(declarator) = (INT)idx;
        DECL_is_bit_field(declarator) = true;
    }
    return declarator;
}


//aggr_declarator_list:
//    aggr_declarator
//    aggr_declarator_list , aggr_declarator
static Decl * aggr_declarator_list(TypeAttr const* ts, TypeAttr * qua)
{
    Decl * dclr = aggr_declarator(ts, qua);
    Decl * ndclr = nullptr;
    if (dclr == nullptr) { return nullptr; }

    while (g_real_token == T_COMMA) {
        CParser::match(T_COMMA);
        ndclr = aggr_declarator(ts, qua);
        xcom::add_next(&dclr, ndclr);
    }

    return dclr;
}


//Calculate constant expression when parsing the variable
//declaration of array type.
//
//'allow_dim0_is_empty': parameter array's lowest dimension size
//can NOT be zero.
static INT compute_array_dim(Decl * dclr, bool allow_dim0_is_empty)
{
    //e.g: int (*(a[1][2]))[3][4];
    BYTE dim = 0;
    INT st = ST_SUCC;
    dclr = const_cast<Decl*>(dclr->getTraitList());
    while (dclr != nullptr) {
        if (dclr->is_dt_array()) {
            dim++;
        } else {
            //Recompute dim size for next array type:
            //e.g: int (*(a[1][2]))[3][4];
            //trait dclr: ID(a)->[1]->[2]->PTR->[3]->[4]
            dim = 0;
        }

        if (dim >= 1) {
            Tree * t = DECL_array_dim_exp(dclr);
            LONGLONG idx = 0;
            if (t == nullptr) {
                if (dim > 1) {
                    err(g_real_line_num,
                        "size of dimension %dth can not be zero,"
                        " may be miss subscript",
                        dim);
                    st = ST_ERR;
                    goto NEXT;
                }
                if (!allow_dim0_is_empty) {
                    //If size of dim0 is 0, set it to 1 by default means
                    //the array contain at least one element.
                    idx = 1;
                }
            } else if (t != nullptr) {
                if (!computeConstExp(t, &idx, 0)) {
                    err(g_real_line_num, "expected constant expression");
                    st = ST_ERR;
                    goto NEXT;
                }
                 if (idx < 0 || idx > MAX_ARRAY_INDX) {
                    err(g_real_line_num,
                        "negative subscript or subscript is too large");
                    st = ST_ERR;
                    goto NEXT;
                }
                if (idx == 0 && t != nullptr) {
                    err(g_real_line_num,
                        "cannot allocate an array of constant size 0");
                    st = ST_ERR;
                    goto NEXT;
                }
            }
            DECL_array_dim(dclr) = idx;
        }
NEXT:
        dclr = DECL_next(dclr);
    }
    return st;
}


static Decl * buildDeclarator(Decl * dclor_list, UINT align)
{
    Decl * declarator = newDecl(DCL_DECLARATOR);
    DECL_child(declarator) = dclor_list;
    compute_array_dim(declarator,
                      true); //array dim size should be determined
                             //by init value.
    //Assigns alignment to variables.
    DECL_align(declarator) = g_alignment;
    return declarator;
}


//init_declarator:
//    declarator
//    declarator = initializer
static Decl * init_declarator(TypeAttr const* ts, TypeAttr * qua)
{
    Decl * dclr_list = declarator(ts, qua);
    if (dclr_list == nullptr) { return nullptr; }
    dclr_list = reverse_list(dclr_list);

    //dclr_list is DCL_DECLARATOR node
    Decl * declarator = buildDeclarator(dclr_list, g_alignment);
    return declarator;
}


static bool process_initializer(Decl * declaration, TypeAttr * qua)
{ 
    if (g_real_token != T_ASSIGN) { return true; }

    ASSERT0(declaration);
    Decl * declarator = declaration->getDeclarator();

    CParser::match(T_ASSIGN);
    DECL_init_tree(declarator) = initializer(qua);
    if (DECL_init_tree(declarator) == nullptr) {
        warn(g_real_line_num, "initial value is empty");

        //TBD: Do we allow an empty initialization?
        //err(g_real_line_num, "initial value is nullptr");
        //Give up parsing subsequent tokens if initialization is empty.
        //suck_tok_to(0, T_SEMI, T_END, T_UNDEF);

        //Error recovery: fill in a placeholer to let the compilation
        //goes on.
        DECL_init_tree(declarator) = buildInitvalScope(buildInt(0));
    }

    if (DECL_init_tree(declarator)->getCode() == TR_INITVAL_SCOPE &&
        TREE_initval_scope(DECL_init_tree(declarator)) == nullptr) {
        warn(g_real_line_num, "initial value is empty");

        //TBD: Do we allow an empty initialization?
        //err(g_real_line_num, "initial value is nullptr");
        //Give up parsing subsequent tokens if initialization is empty.
        //suck_tok_to(0, T_SEMI, T_END, T_UNDEF);

        //Error recovery: fill in a placeholer to let the compilation
        //goes on.
        TREE_initval_scope(DECL_init_tree(declarator)) = buildInt(0);
    }

    DECL_is_init(declarator) = true;
    return true;
}


static Tree * appendInitPlaceholder(Decl * declaration, Tree ** dcl_tree_list)
{
    //Record the placeholder in stmt list of scope.
    //The placeholder is used to mark the lexicographical order
    //of dearataion. The order is often used to determine where should to
    //insert initialization code.
    Tree * t = NEWTN(TR_DECL);
    TREE_decl(t) = declaration;
    DECL_placeholder(declaration) = t;
    xcom::add_next(dcl_tree_list, t);
    return t;
}


static bool isFollowSetOfDeclaration(TOKEN tok)
{
    return g_real_token == T_SEMI || g_real_token == T_COMMA;
}


static bool assembleDeclaration(TypeAttr * attr, Decl * declarator,
                                UINT lineno, Decl ** declaration,
                                Tree ** dcl_tree_list)
{
    *declaration = buildDeclaration(attr, declarator, lineno);
    Tree * t = appendInitPlaceholder(*declaration, dcl_tree_list);

    if (attr->is_user_type_ref()) {
        //Current declaration reference user-defined-type.
        //Expand user type recursively with the first-class type of C.        
        *declaration = makeupAndExpandUserType(*declaration);
        DECL_placeholder(*declaration) = t;
        DECL_align(*declaration) = g_alignment;
        DECL_decl_scope(*declaration) = g_cur_scope;
        DECL_lineno(*declaration) = lineno;
    }
    return true;
}


static bool process_declaration(OUT Decl * declaration)
{
    if (declaration->is_fun_decl()) {
        if (g_real_token == T_LLPAREN) {
            //Function Definition.
            //The function will add declaration into scope.
            if (!parse_function_definition(declaration)) {
                return false;
            }
        } else if (isFollowSetOfDeclaration(g_real_token)) {
            //End of Function Declaration.            
            DECL_is_fun_def(declaration) = false;

            //Current declaration is variable/function declaration.
            g_cur_scope->addDecl(declaration);
        } else {
            //Nothing at all.
            err(g_real_line_num,
                "illegal function definition/declaration, "
                "might be miss ';' or '{'");
            return false;
        }
    } else {
        //Check the declaration that should be unique at current scope.
        //Variable definition/declaration.
        if (!isUniqueDecl(g_cur_scope->getDeclList(), declaration)) {
            err(g_real_line_num, "'%s' already defined",
                declaration->get_decl_sym()->getStr());
            return false;
        }

        //Current declaration is variable/function declaration.
        g_cur_scope->addDecl(declaration);
    }

    if (declaration->is_user_type_decl()) {
        //Current declaration is user typedef declaration.
        //As the preivous parsing in 'declarator()' has recoginzed that
        //current identifier is identical exactly in current scope,
        //it is dispensable to warry about the redefinition, even if
        //invoking isUserTypeExist().
        g_cur_scope->addToUserTypeList(declaration);
    }
    return true;
}


static bool post_process_of_declarator(TypeAttr * attr,
                                       Decl * declarator,
                                       UINT lineno,
                                       OUT Decl ** declaration,
                                       OUT Tree ** dcl_tree_list)
{
    if (!assembleDeclaration(attr, declarator, lineno, declaration,
                             dcl_tree_list)) {
        return false;
    }
    ASSERT0(*declaration);

    if (!process_declaration(*declaration)) {
        return false;
    }

    if (!checkAggrComplete(*declaration)) {
        return false;
    }

    if (!checkBitfield(*declaration)) {
        return false;
    }

    return true;
}


static bool post_process_of_initializer(TypeAttr * attr,
                                        Decl * declaration,
                                        UINT lineno,
                                        bool * is_last_decl,
                                        Tree ** dcl_tree_list)
{
    if (declaration->is_initialized()) {
        process_init_of_declaration(declaration);
    } else {
        //Check the size of array dimension.
        if (declaration->is_array()) {
            fixExternArraySize(declaration);

            //This function also do check in addition to compute array size.
            declaration->get_decl_size();
        }
    }

    *is_last_decl = DECL_is_fun_def(declaration);
    return true;
}


//init_declarator_list:
//    init_declarator
//    init_declarator_list, init_declarator
static bool init_declarator_list(TypeAttr * ts, TypeAttr * qua,
                                 UINT lineno, bool * is_last_decl,
                                 Tree ** dcl_tree_list)
{
    do {
        Decl * declarator = init_declarator(ts, qua);
        if (declarator == nullptr) {
            //For enum type, there is no enum variable declared, such as:
            //  enum {X, Y, Z};
            //  enum _tag {X, Y, Z};
            //return nullptr; //no variable declared
            if (g_real_token == T_COMMA) {
                CParser::match(T_COMMA);
                //There is no variable/type-name declared.
                continue;
            } else {
                //Current token is not belong to declaration. Thus
                //declaration may finish.
                break;
            }
        }

        Decl * declaration = nullptr;
        if (!post_process_of_declarator(ts, declarator, lineno,
                                        &declaration, dcl_tree_list)) {
            return false;
        }

        ASSERT0(declaration);
        process_initializer(declaration, qua);

        Decl * dclor = declaration->getDeclarator();
        ASSERTN(dclor, ("declaration misses declarator"));
        if (dclor->getTraitList() == nullptr) {
            err(g_real_line_num, "declaration expected identifier");
            return false;
        }

        if (!post_process_of_initializer(ts, declaration, lineno, is_last_decl,
                                         dcl_tree_list)) {
            return false;
        }

        if (g_real_token == T_COMMA) { CParser::match(T_COMMA); }
        else { break; }
    } while (!CParser::isTerminateToken());
    return true;
}


//ARRAY MODE:     S (D)[e]
//POINTER MODE:   S * D
//FUNCTION MODE:  S (D)(p)
//
//direct_declarator:
//    identifier
//    (declarator)
//    direct_declarator [ constant_expression ]
//    direct_declarator [                     ]
//    direct_declarator ( parameter_type_list )
//    //direct_declarator ( identifier_list ) obsolete C proto
//    direct_declarator (                 )
static Decl * direct_declarator(TypeAttr const* ts, TypeAttr * qua)
{
    bool is_paren = false;
    Decl * dcl = nullptr;
    switch (g_real_token) {
    case T_LPAREN: //'(' declarator ')'
        CParser::match(T_LPAREN);
        dcl = declarator(ts, qua);
        if (CParser::match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')'");
            goto FAILED;
        }
        if (dcl == nullptr) {
            err(g_real_line_num, "must have identifier declared");
            goto FAILED;
        }
        is_paren = true;
        break;
    case T_ID: { //identifier
        if (!ts->isValidSpecifier()) {
            err(g_real_line_num, 
                "meet '%s', illegal qualifier of declaration",
                g_real_token_string);
        }
        Sym const* sym = g_fe_sym_tab->add(g_real_token_string);
        g_cur_scope->addToSymList(sym);
        dcl = newDecl(DCL_ID);
        DECL_id_tree(dcl) = CParser::id();
        DECL_qua(dcl) = qua;
        CParser::match(T_ID);
        break;
    }
    default:;
    }

    if (dcl == nullptr) { return nullptr; }

    switch (g_real_token) {
    case T_LSPAREN: { //'[', the declarator is an array.
        Tree * t = nullptr;
        while (g_real_token == T_LSPAREN) {
            CParser::match(T_LSPAREN);
            Decl * ndcl = newDecl(DCL_ARRAY);
            t = CParser::conditional_exp();
            if (CParser::match(T_RSPAREN) != ST_SUCC) {
                err(g_real_line_num,
                    "meet '%s', illegal array declaration, may be miss ']'",
                    g_real_token_string);
                goto FAILED;
            }
            DECL_array_dim_exp(ndcl) = t;
            DECL_is_paren(ndcl) = is_paren;

            //'id' should be the last one in declarator-list.
            xcom::insertbefore_one(&dcl, dcl, ndcl);
        }
        break;
    }
    case T_LPAREN: { //'(', the declarator is a function decl/def.
        CParser::match(T_LPAREN);
        Decl * ndcl = newDecl(DCL_FUN);
        push_scope(true);

        //Check if param declaration is void, such as: foo(void).
        Decl * param_decl = parameter_type_list();
        if (xcom::cnt_list(param_decl) == 1 &&
            param_decl->is_any() &&
            param_decl->is_scalar()) {
            ;
        } else {
            DECL_fun_para_list(ndcl) = param_decl;
        }

        pop_scope();
        DECL_is_paren(ndcl) = is_paren;
        xcom::insertbefore_one(&dcl, dcl, ndcl);

        if (CParser::match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num,
                "meet '%s', illegal parameter declaration, may be miss ')'",
                g_real_token_string);
            goto FAILED;
        }
        break;
    }
    default:; //do nothing
    }
    return dcl;
FAILED:
    return dcl;
}


//Copy specifier.
TypeAttr * dupSpec(TypeAttr const* ty)
{
    TypeAttr * new_ty = newTypeAttr();
    new_ty->copy(*ty);
    return new_ty;
}


//pointer:
//    '*' type-qualifier-list(pass)
//    '*' type-qualifier-list(pass) pointer
static Decl * pointer(TypeAttr ** qua)
{
    Decl * ndcl = nullptr;
    TypeAttr * new_qua = *qua;
    while (g_real_token == T_ASTERISK) {
        CParser::match(T_ASTERISK);
        Decl * dcl = newDecl(DCL_POINTER);
        DECL_qua(dcl) = new_qua;
        new_qua = newTypeAttr();
        qualifier(new_qua);
        if (new_qua->is_restrict()) {
            SET_FLAG(TYPE_des(DECL_qua(dcl)), T_QUA_RESTRICT);
            REMOVE_FLAG(TYPE_des(new_qua), T_QUA_RESTRICT);
        }
        xcom::add_next(&ndcl, dcl);
    }
    qualifier(new_qua); //CParser::match qualifiers for what are following identifier.
    *qua = new_qua;
    return ndcl;
}


//declarator:
//    pointer  direct_declarator
//             direct_declarator
static Decl * declarator(TypeAttr const* ts, TypeAttr * qua)
{
    Decl * ptr = pointer(&qua);
    Decl * dclr = direct_declarator(ts, qua);
    if (dclr == nullptr) {
        return nullptr;
    }

    //e.g:
    //    ptr is '*', dclr is '[]'->'ID'
    //    return: '*'->'[]'->'ID'
    xcom::insertbefore(&dclr, dclr, ptr);
    return dclr; //'id' is the list tail.
}


static INT checkLabel(Scope * s)
{
    if (s == nullptr) { return ST_ERR; }
    LabelInfo * lref = SCOPE_ref_label_list(s).get_head();
    while (lref != nullptr) {
        CHAR const* name = lref->getOrgName()->getStr();
        ASSERT0(name);
        LabelInfo const* li = SCOPE_label_list(s).get_head();
        for (; li != nullptr; li = SCOPE_label_list(s).get_next()) {
            if (::strcmp(li->getOrgName()->getStr(), name) == 0) {
                set_lab_used(li);
                break;
            }
        }
        if (li == nullptr) {
            err(map_lab2lineno(lref), "label '%s' was undefined", name);
            return ST_ERR;
        }
        lref = SCOPE_ref_label_list(s).get_next();
    }

    for (LabelInfo const* lj = SCOPE_label_list(s).get_head();
         lj != nullptr; lj = SCOPE_label_list(s).get_next()) {
        if (!is_lab_used(lj)) {
            warn(g_real_line_num, "'%s' unreferenced label",
                 lj->getOrgName()->getStr());
        }
    }
    return ST_SUCC;
}


static void fixParamArrayIndex(Decl * decl)
{
    ASSERT0(decl->is_dt_declaration());
    ASSERT0(decl->is_formal_param());
    ASSERT0(decl->is_pointer());
    TypeAttr * ty = nullptr;
    Decl * d = decl->get_pointer_base_decl(&ty);
    if (d == nullptr || d->is_dt_pointer()) { return; }

    if (d->is_dt_array() && DECL_array_dim(d) == 0) {
        //Fix array index, it can not be 0.
        //C allows the first dimension of parameter be zero.
        //e.g: void foo (char p[][20]) is legal syntax, but
        //     the declaration is char p[1][20].
        DECL_array_dim(d) = 1;
    }

    if (getDeclaratorSize(decl->getTypeAttr(), d) == 0) {
        err(g_real_line_num,
            "Only the first dimension size can be 0, "
            "the lower dimension size can not be 0");
    }
}


//Refine the parameter which is array type.
static Tree * refineArrayParam(Tree * t, Tree * base, Decl * base_decl)
{
    ASSERT0(t->getCode() == TR_ARRAY && base->getCode() == TR_ID &&
            base_decl->is_formal_param());

    //Verfiy and fix formal parameters with array type.
    //Check if decl is pointer that pointed to an array.
    //e.g: 'int (*p)[]'
    //the referrence should do same operation as its declaration.
    Decl const* base_of_pt = base_decl->getTraitList();
    if (DECL_dt(base_of_pt) == DCL_ID) {
        base_of_pt = DECL_next(base_of_pt);
    }

    if (base_of_pt != nullptr && DECL_dt(base_of_pt) == DCL_POINTER) {
        if (DECL_next(base_of_pt) != nullptr &&
            DECL_dt(DECL_next(base_of_pt)) == DCL_ARRAY) {
            base_of_pt = DECL_next(base_of_pt);
        }
    }

    if (base_of_pt != nullptr && base_of_pt->is_dt_array()) {
        //The basetype of pointer is an array.
        //e.g:Given a[] to (*a)[], the decl-list is:
        //    ID(a)->ARR 
        //Convert to (*a)[], the decl-list is:
        //    ID(a)->PTR->ARR         
        Tree * deref = buildDeref(base);
        TREE_array_base(t) = deref;
        Tree::setParent(t, deref);
        fixParamArrayIndex(base_decl);
    }
    return t;
}


//Change array to pointer if it is formal parameter.
//Fulfill the first dimension to at least 1 if it is a parameter.
static Tree * refineArray(Tree * t)
{
    ASSERT0(t->getCode() == TR_ARRAY);

    //Formal parameter of array type is a pointer in actually.
    //Insert a Dereference to comfort the C specification.
    Tree * base = TREE_array_base(t);
    if (base->getCode() != TR_ID) { return t; }

    //ID is unique to its scope.
    CHAR const* name = TREE_id_name(base)->getStr();
    ASSERT0(TREE_id_decl(base));
    Scope * s = TREE_id_decl(base)->getDeclScope();
    Decl * decl = get_decl_in_scope(name, s);
    ASSERT0(decl != nullptr);
    if (decl->is_formal_param()) {
        return refineArrayParam(t, base, decl);
    }
    return t;
}


//Do refinement and amendment for tree.
//    * Revise formal parameter. In C spec, formal array is pointer
//      that point to an array in actually.
static Tree * refine_tree(Tree * t)
{
    if (t == nullptr) { return nullptr; }
    if (t->getCode() == TR_ARRAY) {
        t = refineArray(t);
    } else if (t->getCode() == TR_SCOPE) {
        Scope * s = TREE_scope(t);
        SCOPE_stmt_list(s) = refine_tree_list(s->getStmtList());
    }

    for (UINT i = 0; i < MAX_TREE_FLDS; i++) {
        refine_tree_list(TREE_fld(t, i));
    }
    return t;
}


static Tree * refine_tree_list(Tree * t)
{
    if (t == nullptr) { return nullptr; }
    Tree * head = t;
    int i = 0;
    while (t != nullptr) {
        refine_tree(t);
        t = TREE_nsib(t);
        i++;
    }
    return head;
}


//Convert Tree in terms of C specification.
static void refine_func(Decl * func)
{
    Scope * scope = DECL_fun_body(func);
    Tree * t = scope->getStmtList();
    if (t == nullptr) { return; }

    t = refine_tree_list(t);
    if (!g_err_msg_list.has_msg()) {
        ASSERTN(TREE_parent(t) == nullptr,
                ("parent node of Tree is nullptr"));
    }
    SCOPE_stmt_list(scope) = t;
}


//Converts current declaration into pointer type from its origin type.
//Note 'this' cannot be pointer.
void Decl::convertToPointerType()
{
    ASSERTN(is_dt_declaration(), ("only DCRLARATION is allowed"));
    ASSERTN(!is_pointer(), ("only DCRLARATION is allowed"));
    Decl const* trait = getTraitList(); //TODO: old trait should be free.
    Decl * new_trait = nullptr;
    bool isdo = true;
    INT count = 0;

    //is_append: transform to pointer type by inserting a DCL_POINTER.
    //  In order to achieve inserting DCL_POINTER type before
    //  the first array type.
    //  e.g: ID->ARRAY->ARRAY => ID->POINTER->ARRAY->ARRAY
    bool is_append = true;
    while (trait != nullptr) {
        switch (DECL_dt(trait)) {
        case DCL_FUN: //Function declarator
        case DCL_ID: //Identifier
        case DCL_VARIADIC: //Variable length parameter
        case DCL_POINTER: { //POINTER  declarator
            if (count > 0) {
                isdo = false;
            }
            xcom::add_next(&new_trait, dupDecl(trait));
            break;
        }
        case DCL_ARRAY: { //ARRAY declarator        
            if (is_append) {
                is_append = false;
                xcom::add_next(&new_trait, newDecl(DCL_POINTER));
                isdo = false;
            }

            ASSERT0_DUMMYUSE(!isdo);
            Decl * p = dupDecl(trait);
            DECL_is_paren(p) = 1;
            xcom::add_next(&new_trait, p);
            break;
        }        
        default: ASSERTN(0, ("unexpected Decl type"));
        }
        trait = DECL_next(trait);
    }

    DECL_trait(this) = new_trait;
    ASSERT0(is_pointer());
}


bool isAggrExistInOuterScope(Scope * scope, CHAR const* tag, bool is_complete,
                             TypeAttr const* ta, OUT Aggr ** s)
{
    if (ta->isStructExpanded()) {
        return isStructExistInOuterScope(scope, tag, is_complete, (Struct**)s);
    }
    ASSERT0(ta->isUnionExpanded());
    return isUnionExistInOuterScope(scope, tag, is_complete, (Union**)s);
}


bool isAggrExistInOuterScope(Scope * scope, Sym const* tag, bool is_complete,
                             TypeAttr const* ta, OUT Aggr ** s)
{
    if (ta->isStructExpanded()) {
        return isStructExistInOuterScope(scope, tag, is_complete, (Struct**)s);
    }
    ASSERT0(ta->isUnionExpanded());
    return isUnionExistInOuterScope(scope, tag, is_complete, (Union**)s);
}


//Return true if the struct typed declaration have already existed in both
//current and all of outer scopes.
bool isStructExistInOuterScope(Scope * scope, CHAR const* tag,
                               bool is_complete, OUT Struct ** s)
{
    ASSERT0(scope);
    for (Scope * sc = scope; sc != nullptr; sc = SCOPE_parent(sc)) {
        if (isAggrTypeExist((List<Aggr*>*)sc->getStructList(),
                            tag, is_complete, (Aggr**)s)) {
            return true;
        }
    }
    return false;
}


//Return true if the struct typed declaration have already existed in both
//current and all of outer scopes.
bool isStructExistInOuterScope(Scope * scope, Sym const* tag,
                               bool is_complete, OUT Struct ** s)
{
    ASSERT0(scope);
    for (Scope * sc = scope; sc != nullptr; sc = SCOPE_parent(sc)) {
        if (isAggrTypeExist((List<Aggr*>*)sc->getStructList(),
                            tag, is_complete, (Aggr**)s)) {
            return true;
        }
    }
    return false;
}


//Return true if the union typed declaration have already existed in both
//current and all of outer scopes.
bool isUnionExistInOuterScope(Scope * scope, CHAR const* tag,
                              bool is_complete, OUT Union ** s)
{
    Scope * sc = scope;
    while (sc != nullptr) {
        if (isAggrTypeExist((List<Aggr*>*)sc->getUnionList(), tag,
                            is_complete, (Aggr**)s)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


//Return true if the union typed declaration have already existed in both
//current and all of outer scopes.
bool isUnionExistInOuterScope(Scope * scope, Sym const* tag,
                              bool is_complete, OUT Union ** s)
{
    Scope * sc = scope;
    while (sc != nullptr) {
        if (isAggrTypeExist((List<Aggr*>*)sc->getUnionList(), tag,
                            is_complete, (Aggr**)s)) {
            return true;
        }
        sc = sc->getParent();
    }
    return false;
}


//Return true if 'name' indicates an enum-value which have already been
//defined in either current scope or outer scopes.
//name: enum name to be checked.
//e: enum set.
//idx: index in enum 'e' value list, start at 0.
bool findEnumVal(CHAR const* name, OUT Enum ** e, OUT INT * idx)
{
    for (Scope * sc = g_cur_scope; sc != nullptr; sc = SCOPE_parent(sc)) {
        if (sc->isEnumExist(name, e, idx)) {
            return true;
        }
    }
    return false;
}


//Enum typed identifier is effective at all of outer scopes.
bool isEnumTagExistInOuterScope(CHAR const* cl, OUT Enum ** e)
{
    Scope * sc = g_cur_scope;
    while (sc != nullptr) {
        if (isEnumTagExist(sc->getEnumTab(), cl, e)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


//
//START Enum
//
//idx: index in enum 'e' constant list, start at 0.
bool Enum::isEnumValExist(CHAR const* vname, OUT INT * idx) const
{
    if (vname == nullptr) { return false; }
    INT eval = 0;
    return isEnumValExistAndEvalValue(ENUM_vallist(this), vname, &eval, idx);
}


//Retrieve constant value of Enum Iterm and Index in Enum List by given name.
//idx: index in enum 'e' constant list, start at 0.
bool Enum::isEnumValExistAndEvalValue(CHAR const* vname,
                                      OUT INT * eval, OUT INT * idx) const
{
    return isEnumValExistAndEvalValue(ENUM_vallist(this), vname, eval, idx);
}


//Retrieve constant value of Enum Iterm and Index in Enum List by given name.
//idx: index in enum 'e' constant list, start at 0.
bool Enum::isEnumValExistAndEvalValue(EnumValueList const* evl,
                                      CHAR const* vname,
                                      OUT INT * eval, OUT INT * idx) const
{
    if (evl == nullptr || vname == nullptr) { return false; }
    INT i = 0;
    INT val = 0;
    while (evl != nullptr) {
        if (evl->is_evaluated()) {
            val = evl->getVal();
        }
        if (::strcmp(EVAL_name(evl)->getStr(), vname) == 0) {
            ASSERT0(eval);
            *eval = val;
            *idx = i;
            return true;
        }
        evl = EVAL_next(evl);
        i++;
        val++;
    }
    return false;
}
//END Enum


//Return true if enum identifier existed.
static bool isEnumTagExist(EnumTab const* entab, CHAR const* id_name,
                           OUT Enum ** e)
{
    if (entab == nullptr || id_name == nullptr) { return false; }
    EnumTabIter it;
    for (Enum * en = entab->get_first(it);
         en != nullptr; en = entab->get_next(it)) {  
        if (en->getName() == nullptr) {
            continue;
        }
        if (::strcmp(en->getName()->getStr(), id_name) == 0) {
            *e = en;
            return true;
        }
    }
    return false;
}


bool isUserTypeExist(UserTypeList const* ut_list, CHAR const* ut_name,
                     OUT Decl ** decl)
{
    if (ut_list == nullptr || ut_name == nullptr) { return false; }

    for (UserTypeList const* utl = ut_list; utl != nullptr;
         utl = USER_TYPE_LIST_next(utl)) {
        Decl * dcl = USER_TYPE_LIST_utype(utl);
        if (::strcmp(dcl->get_decl_sym()->getStr(), ut_name) == 0) {
            *decl = dcl;
            return true;
        }
    }
    return false;
}


bool isUserTypeExistInOuterScope(CHAR const* ut_name, OUT Decl ** decl)
{
    Scope * sc = g_cur_scope;
    while (sc != nullptr) {
        if (isUserTypeExist(SCOPE_user_type_list(sc), ut_name, decl)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


bool isAggrTypeExist(List<Aggr*> const* aggrs, Sym const* tag,
                     bool is_complete, OUT Aggr ** s)
{
    if (tag == nullptr) { return false; }
    xcom::C<Aggr*> * ct;
    for (Aggr * st = aggrs->get_head(&ct);
         ct != nullptr; st = aggrs->get_next(&ct)) {
        if (st->getTag() == tag && (!is_complete || st->is_complete())) {
            *s = st;
            return true;
        }
    }
    return false;
}


bool isAggrTypeExist(List<Aggr*> const* aggrs, CHAR const* tag,
                     bool is_complete, OUT Aggr ** s)
{
    if (tag == nullptr) { return false; }
    xcom::C<Aggr*> * ct;
    for (Aggr * st = aggrs->get_head(&ct);
         ct != nullptr; st = aggrs->get_next(&ct)) {
        Sym const* sym = st->getTag();
        if (sym == nullptr) { continue; }
        if (::strcmp(sym->getStr(), tag) == 0 &&
            (!is_complete || st->is_complete())) {
            *s = st;
            return true;
        }
    }
    return false;
}


bool inFirstSetOfDeclaration()
{
    if (g_real_token == T_ID) {
        Decl * ut = nullptr;
        //Note the tag of aggregate type should not belong to the first-set of
        //decaration.
        //e.g: typedef struct tagS S;
        //     S is belong to the first-set, whereas tagS is not.
        if (isUserTypeExistInOuterScope(g_real_token_string, &ut)) {
            return true;
        }
        return false;
    }

    if (Tree::is_type_spec(g_real_token) ||
        Tree::is_type_quan(g_real_token) ||
        Tree::is_stor_spec(g_real_token)) {
        return true;
    }

    return false;
}


Enum * newEnum()
{
    return (Enum*)xmalloc(sizeof(Enum));
}


TypeAttr * newTypeAttr()
{
    TypeAttr * ty = (TypeAttr*)xmalloc(sizeof(TypeAttr));
    ty->clean();
    return ty;
}


TypeAttr * newTypeAttr(INT cate)
{
    TypeAttr * ty = (TypeAttr*)xmalloc(sizeof(TypeAttr));
    ty->clean();
    TYPE_des(ty) = cate;
    return ty;
}


//Compute array byte size.
//'decl' presents DCL_DECLARATOR or DCL_ABS_DECLARATOR,
//Compute byte size of total array.
static UINT computeArrayByteSize(TypeAttr const* spec, Decl const* decl)
{
    if (DECL_dt(decl) == DCL_DECLARATOR) {
        decl = DECL_child(decl);
        if (DECL_dt(decl) != DCL_ID) {
            err(g_src_line_num, "declarator absent identifier");
            return 0;
        }
        decl = DECL_next(decl);
    } else if (decl->is_dt_abs_declarator()) {
        decl = DECL_child(decl);
    }
    if (decl->is_dt_id()) {
        decl = DECL_next(decl);
    }
    if (decl == nullptr) {
        return 0;
    }

    UINT num = 0;
    UINT dim = 0;
    while (decl != nullptr && decl->is_dt_array()) {
        UINT dimsz = (UINT)DECL_array_dim(decl);
        if (dimsz == 0) {
            if (spec->is_extern()) {
                dimsz = 1;
            } else {
                warn(g_src_line_num,
                     "size of %dth dimension should not be zero", dim);
                return 0;
            }
        }

        if (num == 0) {
            //Meet the first dim
            num = dimsz;
        } else {
            num *= dimsz;
        }

        dim++;
        decl = DECL_next(decl);
    }

    //Note host's UINT may be longer than target machine.
    ASSERTN(computeMaxBitSizeForValue(num) <= BYTE_PER_INT * BIT_PER_BYTE,
            ("too large array"));
    return num;
}


//Return byte size of a group of bit fields that consisted of integer type.
//Note the function will update dcl to next declaration.
static UINT computeBitFieldByteSize(Decl const** dcl)
{
    ASSERTN((*dcl)->is_integer(), ("must be handled in aggr_declarator()"));
    ASSERT0(DECL_spec(*dcl));

    //The integer type of the bit group.
    ULONG int_ty = TYPE_des(DECL_spec(*dcl));

    //the max bit size the group hold.
    ULONG int_bitsize = TypeAttr::computeScalarTypeBitSize(int_ty);

    UINT bitsize = 0;
    UINT total_bitsize = int_bitsize;
    for (; (*dcl) != nullptr;) {
        TypeAttr * ty2 = DECL_spec(*dcl);
        ASSERT0(ty2);

        Decl const* declarator = (*dcl)->get_declarator();
        ASSERT0(declarator);

        if (!DECL_is_bit_field(declarator)) {
            break;
        }

        ASSERT0(DECL_bit_len(declarator) > 0);

        if (TYPE_des(ty2) != int_ty) { break; }

        if (bitsize + DECL_bit_len(declarator) > int_bitsize) {
            total_bitsize += int_bitsize;
            bitsize = 0;
        }

        bitsize += DECL_bit_len(declarator);

        *dcl = DECL_next(*dcl);
    }

    ASSERT0(total_bitsize != 0);
    return total_bitsize / BIT_PER_BYTE;
}


//Compute new alignment size according to given 'size' and 'max_field_size'.
UINT Aggr::computeAlignedSize(UINT size, UINT max_field_size) const
{
    if (AGGR_align(this) < max_field_size) {
        //Ensure field alignment is compatible with target machine's
        //alignment constraint.
        max_field_size = pad_align(max_field_size, AGGR_align(this));
    }

    if (AGGR_pack_align(this) != 0) {
        size = pad_align(size, AGGR_align(this));
    } else {
        //There is no user declared constraint on struct alignment,
        //thus aligning the struct in its natural alignment.
        size = pad_align(size, max_field_size);
    }
    return size;
}


static UINT compute_field_ofst(Aggr const* s, UINT ofst,
                               Decl const* dcl, UINT field_align,
                               UINT * elem_bytesize)
{
    if (dcl->is_array()) {
        Decl const* elem_dcl = dcl->get_array_base_decl();
        *elem_bytesize = elem_dcl->get_decl_size();
        UINT elem_num = dcl->get_array_elemnum();
        if (elem_num != 0) {
            //C-language allows the highest dimension of array to be zero.
            //e.g:struct { int a; char cc[]; };
            ofst = compute_field_ofst_consider_pad(s, ofst, *elem_bytesize,
                                                   elem_num,
                                                   AGGR_field_align(s));
        }
        return ofst;
    }

    *elem_bytesize = dcl->get_decl_size();
    ofst = compute_field_ofst_consider_pad(s, ofst, *elem_bytesize,
                                           1, AGGR_field_align(s));
    return ofst;
}


UINT TypeAttr::computeStructTypeSize(Aggr const* s)
{
    UINT ofst = 0;
    UINT max_field_sz = 0;
    for (Decl const* dcl = s->getDeclList(); dcl != nullptr;) {
        if (dcl->is_bitfield()) {
            //dcl will be updated to next declaration in
            //computeBitFieldByteSize().
            UINT bytesize = computeBitFieldByteSize(&dcl);
            ofst = compute_field_ofst_consider_pad(s, ofst, bytesize, 1,
                                                   AGGR_field_align(s));
            max_field_sz = MAX(max_field_sz, bytesize);
            continue;
        }

        UINT elem_bytesize = 0;
        UINT newofst = compute_field_ofst(s, ofst, dcl, AGGR_field_align(s),
                                          &elem_bytesize);
        if (newofst < ofst) {
            err(g_real_line_num, "field size may be too large");
            //Error recovery: to avoid ASSERTION in size verification.
            newofst += 4;
        }
        ofst = newofst;
        max_field_sz = MAX(max_field_sz, elem_bytesize);        
        dcl = DECL_next(dcl);
    }

    UINT size = ofst;
    return s->computeAlignedSize(size, max_field_sz);
}


UINT TypeAttr::computeUnionTypeSize(Aggr const* s)
{
    UINT size = 0;
    for (Decl const* dcl = s->getDeclList();
         dcl != nullptr; dcl = DECL_next(dcl)) {
        size = MAX(size, dcl->get_decl_size());
    }
    return s->computeAlignedSize(size, size);
}


static INT format_base_spec(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    if (!ty->isSimpleType()) { return ST_ERR; }
    if (ty->is_signed()) { buf.strcat("signed "); }
    if (ty->is_unsigned()) { buf.strcat("unsigned "); }
    if (ty->is_char()) { buf.strcat("char "); }
    if (ty->is_short()) { buf.strcat("short "); }
    if (ty->is_long()) { buf.strcat("long "); }
    if (ty->is_int()) { buf.strcat("int "); }
    if (ty->is_longlong()) { buf.strcat("longlong "); }
    if (ty->is_float()) { buf.strcat("float "); }
    if (ty->is_double()) { buf.strcat("double "); }
    if (ty->is_void()) { buf.strcat("void "); }
    return ST_SUCC;
}


INT format_enum_complete(StrBuf & buf, Enum const* e)
{
    if (e == nullptr) { return ST_SUCC; }
    if (e->getName() != nullptr) {
        buf.strcat("%s ", e->getName()->getStr());
    }

    EnumValueList const* ev = e->getValList();
    if (ev != nullptr) {
        buf.strcat("{");
        while (ev != nullptr) {
            buf.strcat("%s(%d) ", EVAL_name(ev)->getStr(), ev->getVal());
            ev = EVAL_next(ev);
        }
        buf.strcat("} ");
    }
    return ST_SUCC;
}


static INT format_enum_complete(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    buf.strcat("enum ");
    format_enum_complete(buf, ty->getEnumType());
    return ST_SUCC;
}


//Format union's name and members.
INT format_union_complete(StrBuf & buf, Union const* u)
{
    if (u == nullptr) { return ST_SUCC; }
    buf.strcat("union ");
    return format_aggr_complete(buf, u);

    //if (u->getTag()) {
    //    buf.strcat(u->getTag()->getStr());
    //}

    ////dump members.
    //Decl * member = u->getDeclList();
    //buf.strcat("{");
    //while (member != nullptr) {
    //    format_declaration(buf, member);
    //    buf.strcat("; ");
    //    member = DECL_next(member);
    //}
    //buf.strcat("}");
    //return ST_SUCC;
}


//Format struct's name and members.
INT format_struct_complete(StrBuf & buf, Struct const* s)
{
    if (s == nullptr) { return ST_SUCC; }
    buf.strcat("struct ");
    return format_aggr_complete(buf, s);

    //if (s->getTag() != nullptr) {
    //    buf.strcat(s->getTag()->getStr());
    //}

    ////dump members.
    //Decl * member = s->getDeclList();
    //buf.strcat("{");
    //while (member != nullptr) {
    //    ASSERT0(member->is_dt_declaration());
    //    if (member->is_aggr() && TYPE_aggr_type(DECL_spec(member)) == s) {
    //        //It will be recursive definition of struction/union,
    //        //e.g:
    //        //struct S {
    //        //    struct S {
    //        //        ...
    //        //    }
    //        //}
    //        ASSERT0(member->is_pointer());
    //    }
    //    format_declaration(buf, member);
    //    buf.strcat("; ");
    //    member = DECL_next(member);
    //}
    //buf.strcat("}");
    //return ST_SUCC;
}


//Format struct/union's name and members.
static INT format_aggr_complete(StrBuf & buf, Aggr const* s)
{
    if (s == nullptr) { return ST_SUCC; }
    format_aggr(buf, s);

    if (!s->is_complete()) { return ST_SUCC; }

    //dump members.
    Decl * member = s->getDeclList();
    buf.strcat("{");
    while (member != nullptr) {
        ASSERTN(member->is_dt_declaration(), ("illegal member"));
        if (member->is_aggr() && member->getTypeAttr()->getAggrType() == s) {
            //It will be recursive definition of struction/union,
            //e.g:
            //struct S {
            //    struct S {
            //        ...
            //    }
            //}
            ASSERT0(member->is_pointer());
        }

        //To avoid recursive aggregate type, format field
        //only with incomplete aggregate type.
        //e.g:union node;
        //    typedef union node * nodeptr;
        //    struct vector {
        //      nodeptr a[1]; //union node includes struct vector.
        //    };
        //    union node {
        //      struct vector vec; //struct vector includes union node.
        //    };
        format_declaration(buf, member, false);
        buf.strcat("; ");
        member = DECL_next(member);
    }
    buf.strcat("}");
    return ST_SUCC;
}


//Format struct/union's name and members.
INT format_aggr_complete(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    return format_aggr_complete(buf, ty->getAggrType());
}


//Format aggregation type's name, the formation does not include field-members.
static INT format_aggr(StrBuf & buf, Aggr const* s)
{
    //Illegal type, TYPE_aggr_type can not be nullptr,
    //one should filter this case before invoke format_aggr();
    if (s == nullptr) {
        //May be error occurred.
        buf.strcat("?? ");
        return ST_SUCC;
    }

    //dump tag
    if (s->getTag() != nullptr) {
        buf.strcat("%s ", s->getTag()->getStr());
    }

    //dump id
    buf.strcat("(id:%d) ", s->id());
    return ST_SUCC;
}


//Format aggregation type's name, the formation does not include field-members.
static INT format_aggr(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    if (ty->is_aggr()) {
        buf.strcat("%s ", ty->getAggrTypeName());
    } else {
        err(g_src_line_num, "expected a struct or union");
        return ST_ERR;
    }
    return format_aggr(buf, TYPE_aggr_type(ty));
}


static INT format_storage(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    if (ty->is_reg()) { buf.strcat("register "); }
    if (ty->is_static()) { buf.strcat("static "); }
    if (ty->is_extern()) { buf.strcat("extern "); }
    if (ty->is_typedef()) { buf.strcat("typedef "); }
    return ST_SUCC;
}


INT format_qualifier(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    if (ty->is_const()) { buf.strcat("const "); }
    if (ty->is_volatile()) { buf.strcat("volatile "); }
    return ST_SUCC;
}


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
INT format_attr(StrBuf & buf, TypeAttr const* ty, bool is_complete)
{
    if (ty == nullptr) { return ST_SUCC; }
    bool is_aggr = (BYTE)(ty->is_struct() || ty->is_union());
    bool is_enum = (BYTE)ty->is_enum();
    bool is_base = (ty->isSimpleType()) != 0;
    bool is_ut = (BYTE)ty->is_user_type_ref();
    format_storage(buf, ty);
    format_qualifier(buf, ty);
    if (is_aggr) {
        if (is_complete) {
            format_aggr_complete(buf, ty);
        } else {
            format_aggr(buf, ty);
        }
        return ST_SUCC;
    }

    if (is_enum) {
        return format_enum_complete(buf, ty);
    }

    if (is_base) {
        return format_base_spec(buf, ty);
    }

    if (is_ut) {
        return format_user_type(buf, ty);
    }
    return ST_ERR;
}


INT format_parameter_list(StrBuf & buf, Decl const* decl)
{
    if (decl == nullptr) { return ST_SUCC; }
    while (decl != nullptr) {
        format_declaration(buf, decl, true);
        buf.strcat(",");
        decl = DECL_next(decl);
    }
    return ST_SUCC;
}


static INT format_dcrl_reverse(StrBuf & buf, TypeAttr const* ty,
                               Decl const* decl)
{
    if (decl == nullptr) { return ST_SUCC; }
    switch (DECL_dt(decl)) {
    case DCL_POINTER: {
        TypeAttr * quan = DECL_qua(decl);
        bool blank = false;
        if (quan != nullptr) {
            if (quan->is_const()) {
                buf.strcat("const ");
                blank = true;
            }
            if (quan->is_volatile()) {
                buf.strcat("volatile ");
                blank = true;
            }
            if (quan->is_restrict()) {
                buf.strcat("restrict ");
                blank = true;
            }
        }
        if (!blank) { buf.strcat(" "); }
        buf.strcat("* ");
        if (DECL_is_paren(decl)) {
            buf.strcat("(");
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
            buf.strcat(")");
        } else {
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
        }
        break;
    }
    case DCL_ID: {
        Tree * t = DECL_id_tree(decl);
        TypeAttr * quan = DECL_qua(decl);
        bool blank = false;
        if (quan != nullptr) {
            ASSERT0(!quan->is_restrict());
            if (quan->is_volatile()) {
                buf.strcat("volatile ");
                blank = true;
            }
            if (quan->is_const()) {
                buf.strcat("const ");
                blank = true;
            }
        }
        if (!blank) { buf.strcat(" "); }
        buf.strcat("%s ", TREE_id_name(t)->getStr());
        if (DECL_is_paren(decl)) {
            buf.strcat("(");
            //p = p + strlen(p);
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
            buf.strcat(")");
        } else {
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
        }
        break;
    }
    case DCL_FUN:
        if (DECL_prev(decl) != nullptr &&
            DECL_dt(DECL_prev(decl)) == DCL_POINTER) {
            //FUN_POINTER
            buf.strcat("(");
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
            buf.strcat(")");
        } else {
            //FUN_DECL
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
        }
        buf.strcat("(");
        format_parameter_list(buf, DECL_fun_para_list(decl));
        buf.strcat(")");
        break;
    case DCL_ARRAY: {
        if (DECL_is_paren(decl)) {
            StrBuf tmpbuf(8);
            format_dcrl_reverse(tmpbuf, ty, DECL_prev(decl));
            if (!tmpbuf.is_empty()) {
                buf.strcat("(%s)", tmpbuf.buf);
            }
        } else {
            format_dcrl_reverse(buf, ty, DECL_prev(decl));
        }
        //bound of each dimensions should be computed while
        //the DECLARATION parsed.
        LONGLONG v = DECL_array_dim(decl);
        if (ty != nullptr && ty->is_extern() && v == 0) {
            //Set extern array to own one elemement at least.
            UNREACHABLE(); //should be handled in declaration()
        }
        //CHAR const* p = buf + strlen(buf);
        buf.strcat("[%lld]", v);
        break;
    }
    default: ASSERTN(0, ("unknown Decl type"));
    }
    return ST_SUCC;
}


INT format_declarator(StrBuf & buf, TypeAttr const* ty, Decl const* decl)
{
    CHAR b[128];
    b[0] = 0;
    if (decl == nullptr) { return ST_SUCC; }
    if (decl->is_dt_abs_declarator()||
        DECL_dt(decl) == DCL_DECLARATOR) {
        if (DECL_bit_len(decl)) {
            SNPRINTF(b, 128, ":%d", DECL_bit_len(decl));
        }
        decl = DECL_child(decl);
    }
    if (decl != nullptr) {
        ASSERTN((decl->is_dt_array() ||
                 decl->is_dt_pointer() ||
                 decl->is_dt_fun() ||
                 decl->is_dt_id() ||
                 decl->is_dt_var()),
                ("unknown declarator"));

        while (DECL_next(decl) != nullptr) { decl = DECL_next(decl); }
        format_dcrl_reverse(buf, ty, decl);
        buf.strcat(b);
    }
    return ST_SUCC;
}


INT format_user_type(StrBuf & buf, TypeAttr const* ty)
{
    if (ty == nullptr) { return ST_SUCC; }
    ASSERT0(HAVE_FLAG(TYPE_des(ty), T_SPEC_USER_TYPE));
    Decl * ut = TYPE_user_type(ty);
    ASSERT0(ut != nullptr);
    return format_user_type(buf, ut);
}


INT format_user_type(StrBuf & buf, Decl const* ut)
{
    if (ut == nullptr) { return ST_SUCC; }
    return format_declaration(buf, ut, true);
}


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
INT format_declaration(StrBuf & buf, Decl const* decl, bool is_complete)
{
    if (decl == nullptr) { return ST_SUCC; }
    if (decl->is_dt_declaration() || decl->is_dt_typename()) {
        TypeAttr * ty = decl->getTypeAttr();
        Decl * dcl = DECL_decl_list(decl);
        format_attr(buf, ty, !decl->is_pointer() && is_complete);
        format_declarator(buf, ty, dcl);
        return ST_SUCC;
    }

    if (DECL_dt(decl) == DCL_DECLARATOR ||
        decl->is_dt_abs_declarator()) {
        Decl * dcl = DECL_decl_list(decl);
        buf.strcat("nullptr ");
        format_declarator(buf, nullptr, dcl);
    } else if (decl->is_dt_pointer() ||
               decl->is_dt_array() ||
               decl->is_dt_fun() ||
               decl->is_dt_id()) {
        buf.strcat("nullptr ");
        format_declarator(buf, nullptr, decl);
    } else if (decl->is_dt_var()) {
        buf.strcat("...");
    } else {
        ASSERTN(0, ("Unkonwn Decl type"));
    }
    return ST_ERR;
}


INT format_parameter_list(Decl const* decl, INT indent)
{
    for (; decl != nullptr; ) {
        format_declaration(decl, indent, true);
        decl = DECL_next(decl);
        if (decl != nullptr) prt(g_logmgr, ", \n");
    }
    return ST_SUCC;
}


INT format_dcrl(Decl const* decl, INT indent)
{
    if (decl == nullptr) {
        return ST_SUCC;
    }
    switch (DECL_dt(decl)) {
    case DCL_POINTER: {
        TypeAttr * quan = DECL_qua(decl);
        if (quan != nullptr) {
            if (quan->is_const()) prt(g_logmgr, "const ");
            if (quan->is_volatile()) prt(g_logmgr, "volatile ");
            if (quan->is_restrict()) prt(g_logmgr, "restrict ");
        }
        if (DECL_next(decl) != nullptr) {
            if (DECL_dt(DECL_next(decl)) != DCL_FUN) {
                prt(g_logmgr, "POINTER");
                prt(g_logmgr, " -> ");
            }
            format_dcrl(DECL_next(decl), indent);
        } else {
            prt(g_logmgr, "POINTER");
        }
        break;
    }
    case DCL_ID: {
        Tree * t = DECL_id_tree(decl);
        TypeAttr * quan = DECL_qua(decl);
        if (quan != nullptr) {
            ASSERT0(!quan->is_restrict());
            if (quan->is_const()) prt(g_logmgr, "const ");
            if (quan->is_volatile()) prt(g_logmgr, "volatile ");
        }
        prt(g_logmgr, "ID:'%s'", TREE_id_name(t)->getStr());
        if (DECL_next(decl) != nullptr) { prt(g_logmgr, " -> ");    }
        format_dcrl(DECL_next(decl), indent);
        break;
    }
    case DCL_FUN:
        if (DECL_prev(decl) != nullptr &&
            DECL_dt(DECL_prev(decl)) == DCL_POINTER) {
            prt(g_logmgr, "FUN_POINTER");
        } else {
            prt(g_logmgr, "FUN_DECL");
        }

        if (DECL_fun_para_list(decl) == nullptr) {
            prt(g_logmgr, ",PARAM:()\n");
        } else {
            prt(g_logmgr, ",PARAM:(");
            format_parameter_list(DECL_fun_para_list(decl),
                                  indent + DECL_FMT_INDENT_INTERVAL);
            prt(g_logmgr, ")\n");
        }
        if (DECL_next(decl) != nullptr) {
            prt(g_logmgr, " RET_VAL_DCL_TYPE:");
        }
        format_dcrl(DECL_next(decl), indent);
        break;
    case DCL_ARRAY: {
        prt(g_logmgr, "ARRAY");
        //bound of each dimensions should be computed while
        //the DECLARATION parsed.
        LONGLONG v = DECL_array_dim(decl);
        prt(g_logmgr, "[%lld]", v);
        if (DECL_next(decl) != nullptr) { prt(g_logmgr, " -> ");    }
        if (DECL_is_paren(decl)) {
            //prt(g_logmgr, "(");
            format_dcrl(DECL_next(decl), indent);
            //prt(g_logmgr, ")");
        } else {
            format_dcrl(DECL_next(decl), indent);
        }
        break;
     }
    default: ASSERTN(0, ("unknown Decl type"));
    }
    return ST_SUCC;
}


INT format_declarator(Decl const* decl, TypeAttr const* ty, INT indent)
{
    DUMMYUSE(ty);
    if (decl == nullptr) { return ST_SUCC; }

    if (decl->is_dt_abs_declarator()||
        DECL_dt(decl) == DCL_DECLARATOR) {
        prt(g_logmgr, "%s", g_dcl_name[DECL_dt(decl)]);
        prt(g_logmgr, "(uid:%d)", DECL_id(decl));
        if (DECL_bit_len(decl)) {
            prt(g_logmgr, ",bitfield:%d", DECL_bit_len(decl));
        }
        note(g_logmgr, "\n");
        decl = DECL_child(decl);
    }

    if (decl != nullptr) {
        ASSERTN((decl->is_dt_array() ||
                 decl->is_dt_pointer() ||
                 decl->is_dt_fun() ||
                 decl->is_dt_id() ||
                 decl->is_dt_var()),
                 ("unknown declarator"));
        g_logmgr->incIndent(DECL_FMT_INDENT_INTERVAL);
        format_dcrl(decl, indent + DECL_FMT_INDENT_INTERVAL);
        g_logmgr->decIndent(DECL_FMT_INDENT_INTERVAL);
    }
    return ST_SUCC;
}


INT format_user_type(TypeAttr const* ty, INT indent)
{
    if (ty == nullptr) {
        return ST_SUCC;
    }
    if ((TYPE_des(ty) & T_SPEC_USER_TYPE) == 0) {
        return ST_ERR;
    }
    Decl * ut = TYPE_user_type(ty);
    return format_user_type(ut, indent);
}


INT format_user_type(Decl const* ut, INT indent)
{
    if (ut == nullptr) {
        return ST_SUCC;
    }
    return format_declaration(ut, indent, true);
}


INT format_declaration(Decl const* decl, INT indent, bool is_complete)
{
    if (decl == nullptr || g_logmgr == nullptr) { return ST_SUCC; }
    note(g_logmgr, "\n");

    StrBuf sbuf(128);
    if (decl->is_dt_declaration() || decl->is_dt_typename()) {
        TypeAttr * ty = decl->getTypeAttr();
        Decl * dcl = DECL_decl_list(decl);
        prt(g_logmgr, "%s", g_dcl_name[DECL_dt(decl)]);
        prt(g_logmgr, "(id:%d)", DECL_id(decl));
        prt(g_logmgr, "LOC:%d", DECL_lineno(decl));
        note(g_logmgr, "\n");

        format_attr(sbuf, ty, !decl->is_pointer() && is_complete);

        g_logmgr->incIndent(DECL_FMT_INDENT_INTERVAL);

        prt(g_logmgr, "SPECIFIER:%s", sbuf.buf);

        note(g_logmgr, "\n");
        format_declarator(dcl, decl->getTypeAttr(),
                          indent + DECL_FMT_INDENT_INTERVAL);

        g_logmgr->decIndent(DECL_FMT_INDENT_INTERVAL);

        return ST_SUCC;
    }

    if (DECL_dt(decl) == DCL_DECLARATOR ||
        decl->is_dt_abs_declarator()) {
        Decl * dcl = DECL_decl_list(decl);
        prt(g_logmgr, "%s", g_dcl_name[DECL_dt(decl)]);
        note(g_logmgr, "\n");
        format_declarator(dcl, nullptr, indent + DECL_FMT_INDENT_INTERVAL);
        return ST_SUCC;
    }

    if (decl->is_dt_pointer() ||
        decl->is_dt_array() ||
        decl->is_dt_fun() ||
        decl->is_dt_id()) {
        prt(g_logmgr, "%s ", g_dcl_name[DECL_dt(decl)]);
        format_declarator(decl, nullptr, indent + DECL_FMT_INDENT_INTERVAL);
        return ST_SUCC;
    }

    if (decl->is_dt_var()) {
        prt(g_logmgr, "... ");
        return ST_SUCC;
    }

    ASSERTN(0, ("Unkonwn Decl type"));
    return ST_ERR;
}
//END DECL_FMT


//Fetch const value of 't' refered.
INT get_enum_const_val(Enum const* e, INT idx)
{
    if (e == nullptr) { return -1; }

    EnumValueList const* ev = ENUM_vallist(e);
    while (idx > 0 && ev != nullptr) {
        ev = EVAL_next(ev);
        idx--;
    }

    if (ev == nullptr) {
        err(g_src_line_num, "enum const No.%d is not exist", idx);
        return -1;
    }

    return ev->getVal();
}


//Fetch const value of 't' refered
CHAR const* get_enum_const_name(Enum const* e, INT idx)
{
    if (e == nullptr) { return nullptr; }

    EnumValueList * evl = ENUM_vallist(e);
    while (idx > 0 && evl != nullptr) {
        evl = EVAL_next(evl);
        idx--;
    }

    if (evl == nullptr) {
        err(g_src_line_num, "enum const No.%d is not exist", idx);
        return nullptr;
    }

    return EVAL_name(evl)->getStr();
}


//If current type-attr is a user-defined type, return the real type-spec.
//Real type-spec could be obtained by expanding user-type.
TypeAttr * TypeAttr::getPureTypeAttr()
{
    if (is_user_type_ref()) {
        return getUserType()->getTypeAttr()->getPureTypeAttr();
    }
    return this;
}


bool TypeAttr::isStructExpanded() const
{
    TypeAttr const* type = const_cast<TypeAttr*>(this)->getPureTypeAttr();
    return type != nullptr && type->is_struct();
}


bool TypeAttr::isAggrExpanded() const
{
    TypeAttr const* type = const_cast<TypeAttr*>(this)->getPureTypeAttr();
    return (type != nullptr) && (type->is_struct() || type->is_union());
}


bool TypeAttr::isUnionExpanded() const
{
    TypeAttr const* type = const_cast<TypeAttr*>(this)->getPureTypeAttr();
    return type != nullptr && type->is_union();
}


//The function generate type-name, and convert it to pointer type.
Decl * convertToPointerTypeName(Decl const* decl)
{
    ASSERT0(decl);
    Decl * tn = dupTypeName(decl);
    Decl * declor = tn->getDeclarator();
    if (declor->is_dt_abs_declarator()) {
        ASSERTN(tn->getTraitList() == nullptr ||
                !tn->getTraitList()->is_dt_id(), ("invalid abs-declarator"));
        xcom::insertbefore_one(&DECL_trait(tn), DECL_trait(tn),
                               newDecl(DCL_POINTER));
        return tn;
    }

    ASSERTN(tn->getTraitList() ||
            tn->getTraitList()->is_dt_id(), ("invalid abs-declarator"));
    xcom::insertafter_one(&DECL_trait(tn), newDecl(DCL_POINTER));
    ASSERT0(tn->is_pointer());
    return tn;
}


//Duplicate src and generate new type-name.
Decl * dupTypeName(Decl const* src)
{
    ASSERTN(src->is_dt_typename(), ("dupTypeName"));
    Decl * dest = newDecl(DCL_TYPE_NAME);
    DECL_decl_list(dest) = dupDecl(DECL_decl_list(src));
    DECL_trait(dest) = nullptr;
    DECL_spec(dest) = DECL_spec(src);

    Decl * p = DECL_trait(src), * q;
    while (p != nullptr) {
        q = dupDecl(p);
        xcom::add_next(&DECL_trait(dest), q);
        p = DECL_next(p);
    }
    return dest;
}


//Get offset of appointed 'name' in struct/union 'st'.
bool get_aggr_field(TypeAttr const* ty, CHAR const* name, Decl ** fld_decl,
                    UINT * fld_ofst)
{
    ASSERT0(ty->is_aggr());
    Aggr * s = ty->getAggrType();
    bool is_union = ty->is_union();
    UINT ofst = 0;
    for (Decl * dcl = s->getDeclList(); dcl != nullptr;
         dcl = DECL_next(dcl)) {
        Sym const* sym = dcl->get_decl_sym();
        if (::strcmp(name, sym->getStr()) == 0) {
            if (fld_decl != nullptr) {
                *fld_decl = dcl;
            }
            if (fld_ofst != nullptr) {
                *fld_ofst = ofst;
            }            
            return true;
        }
        if (dcl->is_anony_aggr()) {
            UINT inner_ofst = 0;
            if (get_aggr_field(dcl->getTypeAttr(), name,
                               fld_decl, &inner_ofst)) {
                if (fld_ofst != nullptr) {
                    *fld_ofst = ofst + inner_ofst;
                }
                return true;
            }            
        }
        if (is_union) {
            //Each field in UNION is offset from 0.
            continue;
        }

        UINT elem_bytesize = 0;
        ofst = compute_field_ofst(s, ofst, dcl, AGGR_field_align(s),
                                  &elem_bytesize);
    }    
    return false;
}


//Get offset and declaration of field indexed by 'idx'.
//idx: the idx of field, start at 0.
bool get_aggr_field(TypeAttr const* ty, INT idx, Decl ** fld_decl,
                    UINT * fld_ofst)
{
    ASSERT0(ty->is_aggr());
    Aggr * s = ty->getAggrType();
    Decl * dcl = s->getDeclList();
    UINT ofst = 0;
    bool is_union = ty->is_union();
    for (; dcl != nullptr && idx >= 0; dcl = DECL_next(dcl), idx--) {
        if (idx == 0) {
            if (fld_decl != nullptr) {
                *fld_decl = dcl;
            }
            if (fld_ofst != nullptr) {
                *fld_ofst = ofst;
            }
            return true;
        }
        if (is_union) {
            //Each field in UNION is offset from 0.
            continue;
        }

        UINT elem_bytesize = 0;
        ofst = compute_field_ofst(s, ofst, dcl, AGGR_field_align(s),
                                  &elem_bytesize);
    }
    return false;
}


static void remove_redundant_para(Decl * declaration)
{
    ASSERT0(declaration->is_dt_declaration() ||
            declaration->is_dt_typename());
    Decl * dclor;
    Decl * para_list = get_parameter_list(declaration, &dclor);
    if (para_list != nullptr) {
        TypeAttr * spec = para_list->getTypeAttr();
        ASSERT0(spec != nullptr);
        if (spec->is_void()) {
            if (isAbsDeclaraotr(para_list) && !para_list->is_pointer()) {
                //e.g int foo(void), there is no any parameter.
                DECL_fun_para_list(dclor) = nullptr;
                return;
            }
            if (!isAbsDeclaraotr(para_list) && !para_list->is_pointer()) {
                err(g_real_line_num, "the first parameter has incomplete type");
                return;
            }
        }
    }
}


//Check struct/union completeness if decl is struct/union.
//Return true if there is no error occur.
static bool checkAggrComplete(Decl * decl)
{
    if (decl->is_pointer() || decl->is_fun_return_pointer()) {
        //Pointer does not require aggragate to be complete.
        return true;
    }
    TypeAttr * attr = decl->getTypeAttr();
    if (!attr->isAggrExpanded()) {
        return true;
    }
    if (attr->is_typedef()) {
        //'typedef' allows referring an incomplete type. There is no need to
        //check incompleteness if 'decl' is typedef.
        return true;
    }

    bool errs = false; //claim error
    if (attr->isStructExpanded() && !attr->isAggrComplete()) {
        errs = true;
    } else if (attr->isUnionExpanded() && !attr->isAggrComplete()) {
        errs = false;
    }
    if (!errs) { return true; }

    //Error occurred.
    StrBuf buf(64);
    Sym const* sym = decl->get_decl_sym();
    if (sym != nullptr) {
        format_aggr_complete(buf, attr->getPureTypeAttr());
        err(g_real_line_num, "'%s' uses incomplete defined %s : %s",
            sym->getStr(), attr->getAggrTypeName(), buf.buf);
        return false;
    }

    err(g_real_line_num, "uses incomplete defined %s without name",
        attr->getAggrTypeName());
    return false;
}


static bool checkBitfield(Decl * decl)
{
    if (decl->is_bitfield() && decl->is_pointer()) {
        err(g_real_line_num, "pointer type can not assign bit length");
        return false;
    }
    return true;
}


static bool parse_function_definition(Decl * declaration)
{
    //Function definition only permit in global scope in C spec.
    if (SCOPE_level(g_cur_scope) != GLOBAL_SCOPE) {
        err(g_real_line_num,
            "miss ';' before '{' , function define should at global scope");
        return false;
    }

    //Check if 'decl' is unique at scope declaration list.
    Decl * dcl = g_cur_scope->getDeclList();
    while (dcl != nullptr) {
        if (Decl::is_decl_equal(dcl, declaration) && declaration != dcl
            && DECL_is_fun_def(dcl)) {
            err(g_real_line_num, "function '%s' already defined",
                dcl->get_decl_sym()->getStr());
            return false;
        }
        dcl = DECL_next(dcl);
    }

    //Add decl to scope here to support recursive func-call.
    g_cur_scope->addDecl(declaration);

    //At function definition mode, identifier of each
    //parameters cannot be nullptr.
    if (isAbsDeclaraotr(DECL_decl_list(declaration))) {
        err(g_real_line_num,
            "expected formal parameter list, not a type list");
        return false;
    }

    remove_redundant_para(declaration);
    Decl * para_list = get_parameter_list(declaration);
    DECL_fun_body(declaration) = CParser::compound_stmt(para_list);

    DECL_is_fun_def(declaration) = true;
    ASSERTN(SCOPE_level(g_cur_scope) == GLOBAL_SCOPE,
            ("Funtion declaration should in global scope"));

    refine_func(declaration);
    if (ST_SUCC != checkLabel(g_cur_scope->getLastSubScope())) {
        err(g_real_line_num, "illegal label used");
        return false;
    }

    //Check return value at typeck.cpp if
    //'DECL_fun_body(dcl)' is nullptr
    return true;
}


static Decl * factor_user_type_rec(Decl const* decl, TypeAttr ** new_spec)
{
    ASSERT0(decl->is_dt_declaration() || decl->is_dt_typename());
    TypeAttr const* spec = decl->getTypeAttr();
    Decl * new_declor = nullptr;
    if (spec->is_user_type_ref()) {
        new_declor = factor_user_type_rec(spec->getUserType(), new_spec);
    } else {
        *new_spec = dupSpec(spec);
        REMOVE_FLAG(TYPE_des(*new_spec), T_STOR_TYPEDEF);
    }

    Decl * cur_declor = const_cast<Decl*>(decl->getTraitList());
    if (cur_declor->is_dt_id()) {
        //neglect the first DCL_ID node, we only need the rest.
        cur_declor = DECL_next(cur_declor);
    }

    cur_declor = dupDeclBeginAt(cur_declor);
    xcom::insertbefore(&new_declor, new_declor, cur_declor);
    return new_declor;
}


//The function will change 'ut' and expand user-defined type that declared
//with 'typedef' in C, and remove the 'typedef' attribute from type-specifier.
//The result type is only consist of the first-class type defined in C.
//The function will modify 'ut' by replacing type-name with the first-class
//type.
Decl * expandUserType(Decl * ut)
{
    ASSERT0(ut->is_user_type_ref() || ut->is_user_type_decl());
    ASSERT0(ut->is_dt_typename() || ut->is_dt_declaration());
    if (ut->is_user_type_ref()) {
        Decl * tmp = expandUserType(ut->getTypeAttr()->getUserType());
        ASSERT0(tmp->getTypeAttr() != nullptr);
        REMOVE_FLAG(TYPE_des(tmp->getTypeAttr()), T_STOR_TYPEDEF);
        DECL_spec(ut) = DECL_spec(tmp);
        tmp = const_cast<Decl*>(tmp->getTraitList());
        if (tmp->is_dt_id()) {
            tmp = DECL_next(tmp);
        }
        if (tmp != nullptr) {
            xcom::add_next(&DECL_trait(ut), tmp);
        }
        return ut;
    }

    Decl * tmp = dupDeclFully(ut);
    ASSERT0(tmp->getTypeAttr() != nullptr);
    REMOVE_FLAG(TYPE_des(tmp->getTypeAttr()), T_STOR_TYPEDEF);
    return tmp;
}


//The function make up new type-name or declaration and will expand
///user-defined type into the first-class type.
//e.g: typedef int * INTP;
//    'INTP a' will be factored to 'int * a'.
Decl * makeupAndExpandUserType(Decl const* ut)
{
    ASSERT0(ut->is_dt_declaration() || ut->is_dt_typename());
    TypeAttr const* spec = ut->getTypeAttr();
    ASSERT0(spec->is_user_type_ref());

    //Indicates 'ut' specifier is 'typedef' or not.
    bool is_typedef = spec->is_typedef();

    //Create new type-specifer according to the factored type information.
    TypeAttr * new_spec = nullptr;
    Decl * new_declor = factor_user_type_rec(spec->getUserType(), &new_spec);
    ASSERT0(new_spec);
    if (is_typedef) {
        SET_FLAG(TYPE_des(new_spec), T_STOR_TYPEDEF);
    }
    SET_FLAG(TYPE_des(new_spec), (TYPE_des(spec) & T_STOR_REG));
    SET_FLAG(TYPE_des(new_spec), (TYPE_des(spec) & T_STOR_EXTERN));
    SET_FLAG(TYPE_des(new_spec), (TYPE_des(spec) & T_STOR_INLINE));
    SET_FLAG(TYPE_des(new_spec), (TYPE_des(spec) & T_STOR_STATIC));
    SET_FLAG(TYPE_des(new_spec), (TYPE_des(spec) & T_STOR_AUTO));

    Tree * inittree = nullptr;
    if (ut->is_dt_declaration() && ut->is_initialized()) {
        inittree = ut->get_decl_init_tree();
    }
    Decl * cur_declor = const_cast<Decl*>(ut->getTraitList());
    if (cur_declor == nullptr) {
        //cur_declor may be abstract declarator list.
        //There is at least DCL_ID if the declaration is typedef.
        //e.g: typedef int*; is invalid.
        ASSERT0(!is_typedef);
        return newDeclaration(new_spec, new_declor, g_cur_scope, inittree);
    }

    ASSERTN(DECL_decl_list(ut), ("miss declarator"));
    ASSERTN(cur_declor->is_dt_id() ||
            DECL_decl_list(ut)->is_dt_abs_declarator(),
        ("either decl is abstract declarator or miss typedef/variable name."));

    //Neglect the first DCL_ID, we only need the rest.
    xcom::insertbefore(&new_declor, new_declor,
                       dupDeclBeginAt(DECL_next(cur_declor)));

    //Prepend an ID to be the head of declarator list.
    DECL_next(cur_declor) = nullptr;
    ASSERT0(DECL_prev(cur_declor) == nullptr);
    xcom::insertbefore_one(&new_declor, new_declor, cur_declor);

    //Create a new declaration with factored specifier.
    return newDeclaration(new_spec, new_declor, g_cur_scope, inittree);
}


//The function infers and set the value of each enum item.
//The value is compile-time constant.
static void inferAndSetEValue(EnumValueList * evals)
{
    INT i = 0;
    for (; evals != nullptr; evals = EVAL_next(evals), i++) {
        if (evals->is_evaluated()) {
            i = evals->getVal();
            continue;
        }

        EVAL_val(evals) = i;
        EVAL_is_evaluated(evals) = true;
    }
}


//The function add the enumerator into current scope.
//Enumerator value always be retrieved as compilation time constant.
static void add_enum(TypeAttr * ta)
{
    TYPE_enum_type(ta) = g_cur_scope->addEnum(ta->getEnumType());
    ASSERT0(TYPE_enum_type(ta));
}


//Reshape zero-dimension array declaration to be including at least one element.
static void fixExternArraySize(Decl * declaration)
{
    ASSERT0(declaration->is_dt_declaration());
    ASSERT0(declaration->is_array());
    if (!declaration->is_extern()) {
        return;
    }

    Decl * decl = const_cast<Decl*>(declaration->getTraitList());
    if (decl->is_dt_id()) {
        decl = DECL_next(decl);
    }
    if (decl == nullptr) {
        return;
    }
    while (decl != nullptr && decl->is_dt_array()) {
        UINT dimsz = (UINT)DECL_array_dim(decl);
        if (dimsz == 0) {
            DECL_array_dim(decl) = 1;
        }
        decl = DECL_next(decl);
    }
}


//declaration:
//  declaration_spec init_declarator_list;
//  declaration_spec ;
//Return declaration-trees if variable declaration is found.
Tree * declaration()
{
    //Parse specifier.
    UINT lineno = g_real_line_num;
    TypeAttr * attr = declaration_spec();
    if (attr == nullptr) { return nullptr; }

    //Deal with informations that is encoded in specifier.
    //Note, In C language, type-attribute includes qualifier.
    TypeAttr * qualifier = newTypeAttr();
    extract_qualifier(attr, qualifier);
    complement_qualifier(attr, qualifier);

    if (attr->is_enum()) {
        //Deal with enumerator value.
        //Note enumerator should have been added into scope's enum-table.
        //The function infers and assigns the value of each item in enumerator.
        inferAndSetEValue(attr->getEnumType()->getValList());
    }

    bool is_last_decl = false;
    Tree * dcl_tree_list = nullptr;
    init_declarator_list(attr, qualifier, lineno, &is_last_decl,
                         &dcl_tree_list);
    if (!is_last_decl) {
        //A special error handling here, to check the syntax of C.
        //To diagnose if there is an ending ';' for the last stuff.
        if (g_real_token != T_SEMI) {
            err(g_real_line_num,
                "meet '%s', expected ';' after declaration",
                g_real_token_string);
        } else {
            CParser::match(T_SEMI);
        }
    }
    
    return dcl_tree_list;
}


//Return tree node if variable declaration is found.
Tree * declaration_list()
{
    Tree * decl_list = nullptr;
    Tree * last = nullptr;
    while (inFirstSetOfDeclaration()) {
        Tree * decl = declaration();
        xcom::add_next(&decl_list, &last, decl);
    }
    return decl_list;
}

} //namespace xfe

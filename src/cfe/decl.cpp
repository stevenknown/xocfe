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
#include "cfecom.h"

/*
Example to show the structure of class Decl.
    e.g1:
    int * a, * const * volatile b[10];
    declaration----
        |          |--type-spec (int)
        |          |--declarator1 (DCL_DECLARATOR)
        |                |---decl-type (id:a)
        |                |---decl-type (pointer)
        |
        |          |--declarator2 (DCL_DECLARATOR)
        |                |---decl-type (id:b)
        |                |---decl-type (array:dim=10)
        |                |---decl-type (pointer:volatile)
        |                |---decl-type (pointer:const)

    e.g2:
    int (*q)[30];
    declaration----
        |          |--type-spec (int)
        |          |--declarator1 (DCL_DECLARATOR)
        |                |---decl-type (id:q)
        |                |---decl-type (pointer)
        |                |---decl-type (array:dim=30)

    e.g3:
    unsigned long const (* const c)(void);
    declaration----
                  |--type-spec (unsigned long const)
                  |--declarator1 (DCL_DECLARATOR)
                        |---decl-type (id:c)
                        |---decl-type (pointer:const)
                        |---decl-type (function)

    e.g4:
    USER_DEFINED_TYPE var;
    declaration----
                  |--type-spec (T_SPEC_USER_TYPE)
                  |--declarator (DCL_DECLARATOR)
                        |---decl-type (id:var)

    e.g5: Abstract declarator, often used in parameters.
    int *
    declaration----
                  |--type-spec (int)
                  |--declarator (DCL_ABS_DECLARATOR)
                        |---NULL


The layout of Declaration:
Decl, with DECL_dt is DCL_DECLARATION or DCL_TYPE_NAME
    |->SCOPE
    |->TypeSpec Specifier
        |->const|volatile
        |->void|long|int|short|char|float|double|signed|unsigned|struct|union
        |->auto|register|static|extern|typedef
    |->DCL_DECLARATOR | DCL_ABS_DECLARATOR
        |->DCL_ID(optional)->DCL_FUN->DCL_POINTER->...
*/

static Tree * initializer(TypeSpec * qua);
static Decl * declarator(TypeSpec * qua);
static TypeSpec * specifier_qualifier_list();
static Decl * struct_declaration();
static Decl * struct_declarator_list(TypeSpec * qua);
static Decl * abstract_declarator(TypeSpec * qua);
static Decl * pointer(TypeSpec ** qua);
static INT compute_array_dim(Decl * dclr, bool allow_dim0_is_empty);
static Tree * refine_tree_list(Tree * t);
static bool is_enum_const_name_exist(Enum * e, CHAR * ev_name, OUT INT * idx);
static bool is_enum_id_exist(IN EnumList * e_list,
                             IN CHAR * e_id_name, OUT Enum ** e);

#ifdef _DEBUG_
UINT g_decl_counter = 1;
#endif
INT g_alignment = PRAGMA_ALIGN; //default alignment.
CHAR * g_dcl_name [] = {
//character of DCL enum-type.
    "",
    "ARRAY",
    "POINTER",
    "FUN",
    "ID",
    "VARIABLE",
    "TYPE_NAME",
    "DCRLARATOR",
    "DCRLARATION",
    "ABS_DCRLARATOR",
};


static void * xmalloc(unsigned long size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    ASSERT0(p != NULL);
    memset(p, 0, size);
    return p;
}


/* Complement the INT specifier.
e.g: unsigned a => unsigned int a
    register a => register int a
*/
static void complement_qua(TypeSpec * ty)
{
    ASSERT0(ty);

    INT des = TYPE_des(ty);
    if (des == T_SPEC_UNSIGNED ||
        des == T_SPEC_SIGNED ||
        des == T_STOR_STATIC ||
        des == T_STOR_EXTERN ||
        des == T_STOR_REG ||
        des == T_STOR_AUTO) {
        SET_FLAG(TYPE_des(ty), T_SPEC_INT);
    }
}


//Copy Decl and its specifier and qualifier, except its siblings.
Decl * cp_decl_complete(Decl * src)
{
    Decl * res = NULL;
    ASSERT0(src);
    if (DECL_dt(src) == DCL_DECLARATION ||
        DECL_dt(src) == DCL_TYPE_NAME) {
        res = cp_decl(src);
        DECL_spec(res) = cp_spec(DECL_spec(src));
        DECL_decl_list(res) = cp_decl(DECL_decl_list(src));
        if (DECL_decl_list(res) != NULL) {
            ASSERT0(DECL_dt(DECL_decl_list(res)) == DCL_DECLARATOR ||
                     DECL_dt(DECL_decl_list(res)) == DCL_ABS_DECLARATOR);
            DECL_child(DECL_decl_list(res)) =
                    cp_decl_begin_at(DECL_child(DECL_decl_list(src)));
        }
    } else if (DECL_dt(src) == DCL_DECLARATOR ||
               DECL_dt(src) == DCL_ABS_DECLARATOR) {
        res = cp_decl(src);
        DECL_child(res) = cp_decl_begin_at(DECL_child(src));
    } else {
        ASSERT0(DECL_dt(src) == DCL_ARRAY ||
                 DECL_dt(src) == DCL_POINTER ||
                 DECL_dt(src) == DCL_FUN ||
                 DECL_dt(src) == DCL_ID ||
                 DECL_dt(src) == DCL_VARIABLE);
        res = cp_decl_begin_at(src);
    }
    return res;
}


//Completely memory copy of 'src', excepting the chain-linking region
Decl * cp_decl(Decl * src)
{
    Decl * q = new_decl(DECL_dt(src));
    memcpy(q, src, sizeof(Decl));
    DECL_spec(q) = NULL;
    DECL_decl_list(q) = NULL;
    DECL_child(q) = NULL;
    DECL_prev(q) = NULL;
    DECL_next(q) = NULL;
    return q;
}


//Duplication declarator list begin at 'header'
Decl * cp_decl_begin_at(Decl * header)
{
    if (header == NULL) { return NULL; }
    Decl * newl = NULL, * p;
    while (header != NULL) {
        p = cp_decl(header);
        add_next(&newl, p);
        header = DECL_next(header);
    }
    return newl;
}


Decl * new_decl(DCL dcl_type)
{
    Decl * d = (Decl*)xmalloc(sizeof(Decl));
    DECL_dt(d) = dcl_type;
    #ifdef _DEBUG_
    DECL_uid(d) = g_decl_counter++;
    #endif
    return d;
}


/*
Construct declaration.
'spec': specifier
'declor': declarator.
*/
Decl * new_declaration(TypeSpec * spec, Decl * declor, SCOPE * sc)
{
    Decl * declaration = new_decl(DCL_DECLARATION);
    DECL_decl_scope(declaration) = sc;
    DECL_spec(declaration) = spec;
    Decl * declarator = new_decl(DCL_DECLARATOR);
    DECL_child(declarator) = declor;
    DECL_decl_list(declaration) = declarator;
    return declaration;
}


/*
Construct new declaration within given scope.
Front-end dependent.
*/
Decl * new_var_decl(IN SCOPE * scope, IN CHAR * name)
{
    Decl * declaration = new_decl(DCL_DECLARATION);
    DECL_decl_scope(declaration) = scope;

    //Make TypeSpec
    TypeSpec * ty = new_type();
    TYPE_des(ty) |= T_SPEC_VOID;
    DECL_spec(declaration) = ty;

    //Make Tree node.
    Tree * tree = new_tree_node(TR_ID, 0);
    SYM * sym = g_fe_sym_tab->add(name);
    TREE_id(tree) = sym;

    //Make DCL_DECLARATOR.
    Decl * declor = new_decl(DCL_DECLARATOR);
    Decl * id = new_decl(DCL_ID);
    DECL_id(id) = tree;
    DECL_child(declor) = id;

    //
    DECL_decl_list(declaration) = declor;
    return declaration;
}


Tree * get_decl_id_tree(Decl * dcl)
{
    while (dcl == NULL) {
        if (DECL_dt(dcl) == DCL_ID) {
            return DECL_id(dcl);
        }
    }
    return NULL;

}


Decl * get_decl_id(Decl * dcl)
{
    ASSERT0(dcl != NULL);
    Decl * pdcl = get_pure_declarator(dcl);
    while (pdcl != NULL) {
        if (DECL_dt(pdcl) == DCL_ID) {
            return pdcl;
        }
        pdcl = DECL_next(pdcl);
    }
    return NULL;
}


SYM * get_decl_sym(Decl * dcl)
{
    dcl = get_decl_id(dcl);
    if (dcl != NULL) {
        return TREE_id(DECL_id(dcl));
    }
    return NULL;
}


//Return true if dcl declared with 'inline'.
bool is_inline(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION, ("need declaration"));
    TypeSpec * ty = DECL_spec(dcl);
    ASSERT0(ty);
    if (IS_INLINE(ty)) {
        return true;
    }
    return false;
}


//Return true if dcl declared with 'const'.
bool is_constant(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION, ("need declaration"));
    TypeSpec * ty = DECL_spec(dcl);
    ASSERT0(ty);
    if (IS_CONST(ty)) {
        return true;
    }
    return false;
}


//Return true if dcl has initial value.
bool is_initialized(Decl * dcl)
{
    ASSERT(dcl && (DECL_dt(dcl) == DCL_DECLARATION ||
                    DECL_dt(dcl) == DCL_DECLARATOR),
            ("need declaration"));
    if (DECL_dt(dcl) == DCL_DECLARATION) {
        dcl = DECL_decl_list(dcl); //get DCRLARATOR
        ASSERT(DECL_dt(dcl) == DCL_DECLARATOR,
                ("need declaration"));
    }
    if (DECL_is_init(dcl)) {
        return true;
    }
    return false;
}


bool is_volatile(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION, ("need declaration"));
    TypeSpec * ty = DECL_spec(dcl);
    ASSERT0(ty);
    if (IS_VOLATILE(ty)) {
        return true;
    }
    return false;
}


bool is_restrict(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION, ("need declaration"));
    if (is_pointer(dcl)) {
        Decl * x = get_pointer_decl(dcl);
        ASSERT0(x);
        TypeSpec * ty = DECL_qua(x);
        if (ty != NULL && IS_RESTRICT(ty)) {
            return true;
        }
    }
    return false;
}


bool is_global_variable(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION, ("need declaration"));
    SCOPE * sc = DECL_decl_scope(dcl);
    ASSERT(sc, ("variable must be allocated within a scope."));
    if (SCOPE_level(sc) == GLOBAL_SCOPE) {
        return true;
    }
    if (SCOPE_level(sc) >= FUNCTION_SCOPE && is_static(dcl)) {
        return true;
    }
    return false;
}


bool is_static(Decl * dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_DECLARATION,
                ("need declaration"));
    ASSERT(DECL_spec(dcl), ("miss specify type"));
    if (IS_STATIC(DECL_spec(dcl))) {
        return true;
    }
    return false;
}


bool is_local_variable(Decl * dcl)
{
    ASSERT(DECL_dt(dcl)==DCL_DECLARATION, ("need declaration"));
    SCOPE * sc = DECL_decl_scope(dcl);
    ASSERT(sc, ("variable must be allocated within a scope."));
    if (SCOPE_level(sc) >= FUNCTION_SCOPE && !is_static(dcl)) {
        return true;
    }
    return false;
}


//Abstract declarator does not have ID.
bool is_abs_declaraotr(Decl * declarator)
{
    ASSERT0(declarator != NULL);
    declarator = get_pure_declarator(declarator);
    if (declarator == NULL) {
        return true;
    }
    SYM * id = get_decl_sym(declarator);
    if (id == NULL) {
        return true;
    }
    return false;
}


//Return true if dcl is daclared with user defined type.
//e.g: typedef int * INTP;  INTP xx; xx is user type referrence.
bool is_user_type_ref(Decl * dcl)
{
    ASSERT0(DECL_dt(dcl) == DCL_DECLARATION ||
             DECL_dt(dcl) == DCL_TYPE_NAME);
    ASSERT0(DECL_spec(dcl) != NULL);
    return IS_USER_TYPE_REF(DECL_spec(dcl));
}


//Return ture if 'dcl' is type declaration that declared with 'typedef'.
//e.g: typedef int * INTP; where INTP is an user type declaration.
bool is_user_type_decl(Decl * dcl)
{
    ASSERT0(DECL_dt(dcl) == DCL_DECLARATION);
    return IS_TYPEDEF(DECL_spec(dcl));
}


//Return true if struct definition is complete.
bool is_struct_complete(TypeSpec const* type)
{
    type = get_pure_type_spec(const_cast<TypeSpec*>(type));
    ASSERT0(IS_STRUCT(type));
    return TYPE_struct_type(type) != NULL &&
           STRUCT_is_complete(TYPE_struct_type(type));
}


//Return true if union definition is complete.
bool is_union_complete(TypeSpec const* type)
{
    type = get_pure_type_spec(const_cast<TypeSpec*>(type));
    ASSERT0(IS_UNION(type));
    return TYPE_union_type(type) != NULL &&
           UNION_is_complete(TYPE_union_type(type));
}


bool is_struct_type_exist_in_cur_scope(CHAR * tag, OUT Struct ** s)
{
    SCOPE * sc = g_cur_scope;
    if (is_struct_type_exist(SCOPE_struct_list(sc), tag, s)) {
        return true;
    }
    return false;
}


/*
Is dcl a indirection declarator,
e.g array , pointer or function pointer
*/
static bool is_indirection(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL) {
        switch (DECL_dt(dcl)) {
        case DCL_ARRAY:
        case DCL_POINTER:
        case DCL_FUN:
            return true;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            if (DECL_dt(dcl) == DCL_DECLARATION ||
               DECL_dt(dcl) == DCL_DECLARATOR ||
               DECL_dt(dcl) == DCL_ABS_DECLARATOR ||
               DECL_dt(dcl) == DCL_TYPE_NAME) {
               ASSERT(0, ("\nunsuitable Decl type locate here in is_indirection()\n"));
            }
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


/*
name: unique symbol for each of scope.
dcl:   DCL_DECLARATION info
*/
bool is_decl_exist_in_outer_scope(IN CHAR * name, OUT Decl ** dcl)
{
    SCOPE * scope = g_cur_scope;
    Decl * dr = NULL, * dcl_list = NULL;
    while (scope != NULL) {
        dcl_list = SCOPE_decl_list(scope);
        while (dcl_list != NULL) {//declaration list
            dr = dcl_list;
            dcl_list = DECL_next(dcl_list);
            SYM * sym = get_decl_sym(dr);
            if (sym == NULL) {
                continue;
            }
            if (strcmp(SYM_name(sym), name) == 0) {
                *dcl = dr;
                return true;
            }
        }
        scope = SCOPE_parent(scope);
    }
    return false;
}


//Return true if 'd1' and 'd2' are the same identifier.
bool is_decl_equal(IN Decl * d1, IN Decl * d2)
{
    SCOPE * s1 = DECL_decl_scope(d1);
    SCOPE * s2 = DECL_decl_scope(d2);
    if (s1 == s2) {
        CHAR const* name1 = SYM_name(get_decl_sym(d1));
        CHAR const* name2 = SYM_name(get_decl_sym(d2));
        if (strcmp(name1, name2) == 0) {
            return true;
        }
    }
    return false;
}


//Return true if 'decl' is unique at a list of Decl.
bool is_unique_decl(Decl * decl_list, Decl * decl)
{
    Decl * dcl = decl_list;
    while (dcl != NULL) {
        if (is_decl_equal(dcl, decl) && dcl != decl) {
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return true;
}


/*
Distinguish the declaration and definition of variable.
Return true if 'decl' is a declaration, otherwise it is a definition.
*/
bool is_declaration(Decl * decl)
{
    if (DECL_is_fun_def(decl)) {
        ASSERT0(0);
    }
    return false;
}



Decl * get_decl_in_scope(IN CHAR * name, SCOPE * scope)
{
    Decl * dr = NULL, * dcl_list = NULL;
    if (scope == NULL) {
        return NULL;
    }
     dcl_list = SCOPE_decl_list(scope);
    while (dcl_list != NULL) { //declaration list
        dr = dcl_list;
        dcl_list = DECL_next(dcl_list);
        SYM * sym = get_decl_sym(dr);
        if (sym == NULL) { continue; }
        if (strcmp(SYM_name(sym), name) == 0) {
            return dr;
        }
    }
    return NULL;
}


//Reference an user defined type-name.
static TypeSpec * typedef_name(TypeSpec * ty)
{
    Decl * ut = NULL;
    if (g_real_token != T_ID) return NULL;
    if (!is_user_type_exist_in_outer_scope(g_real_token_string, &ut)) {
        return NULL;
    }
    if (ty == NULL) {
        ty = new_type();
    }
    TYPE_des(ty) |= T_SPEC_USER_TYPE;
    TYPE_user_type(ty)= ut;
    match(T_ID);
    return ty;
}


static INT ck_type_spec_legally(TypeSpec * ty)
{
    INT des = TYPE_des(ty);
    CHAR buf1[MAX_BUF_LEN]; buf1[0] = 0;
    CHAR buf2[MAX_BUF_LEN]; buf2[0] = 0;
    //struct or union
    BYTE c1 = (HAVE_FLAG(des, T_SPEC_STRUCT) ||
               HAVE_FLAG(des, T_SPEC_UNION)) != 0,
         c2 = HAVE_FLAG(des, T_SPEC_ENUM) != 0,
         c3 = is_simple_base_type(ty) != 0,
         c4 = HAVE_FLAG(des, T_SPEC_USER_TYPE) != 0;

    //signed
    if (ONLY_HAVE_FLAG(des, T_SPEC_SHORT)) {
        //des only contained SHORT
        goto SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SHORT|T_SPEC_INT)) {
        //des only contained SHORT and INT concurrent
        goto SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_SHORT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_SHORT|T_SPEC_INT)) {
        goto SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_INT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_INT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONG|T_SPEC_INT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONG|T_SPEC_INT)) {
        goto SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONGLONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_LONGLONG|T_SPEC_INT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_SIGNED|T_SPEC_LONGLONG|T_SPEC_INT)) {
        //des contained SIGNED, LONG, LONG, INT concurrent
        goto SUCC;
    }

    //unsiged
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_SHORT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_SHORT|T_SPEC_INT)) {
        goto SUCC;
    }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_INT)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONGLONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONG)) { goto SUCC; }
    if (ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED|T_SPEC_LONG|T_SPEC_INT)) {
        goto SUCC;
    }

    if (c1 == 1 && c2 == 1) {
        err(g_real_line_num,
            "struct or union cannot compatilable with enum-type");
        goto FAILED;
    }
    if (c1 == 1 && c3 == 1) {
        format_base_type_spec(buf1, ty);
        err(g_real_line_num,
            "struct or union cannot compatilable with '%s'", buf1);
        goto FAILED;
    }
    if (c1 == 1 && c4 == 1) {
        format_user_type_spec(buf1, ty);
        err(g_real_line_num,
            "struct or union cannot compatilable with '%s'", buf1);
        goto FAILED;
    }
    if (c2 == 1 && c3 == 1) {
        format_base_type_spec(buf1, ty);
        err(g_real_line_num, "enum-type cannot compatilable with '%s'", buf1);
        goto FAILED;
    }
    if (c2 == 1 && c4 == 1) {
        format_user_type_spec(buf1, ty);
        err(g_real_line_num, "enum-type cannot compatilable with '%s'", buf1);
        goto FAILED;
    }
    if (c3 == 1 && c4 == 1) {
        format_user_type_spec(buf1, ty);
        format_base_type_spec(buf2, ty);
        err(g_real_line_num,
            "'%s' type cannot compatilable with '%s'", buf1, buf2);
        goto FAILED;
    }
SUCC:
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


//Extract the qualifier from 'ty' and fulfill 'qua'.
static void extract_qualifier(TypeSpec * ty, TypeSpec * qua)
{
    ASSERT0(ty && qua);

    if (IS_CONST(ty)) {
        SET_FLAG(TYPE_des(qua), T_QUA_CONST);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_CONST);
    }

    if (IS_VOLATILE(ty)) {
        SET_FLAG(TYPE_des(qua), T_QUA_VOLATILE);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
    }

    if (IS_RESTRICT(ty)) {
        SET_FLAG(TYPE_des(qua), T_QUA_RESTRICT);
        REMOVE_FLAG(TYPE_des(ty), T_QUA_RESTRICT);
    }
}


//Up to date, there is no other differences between
//union definition and struct definition.
static Decl * union_declaration()
{
    return struct_declaration();
}


static void consume_tok_to_semi()
{
    while (g_real_token != T_SEMI &&
           g_real_token != T_END &&
           g_real_token != T_NUL) {
        match(g_real_token);
    }
    if (g_real_token == T_SEMI) {
        match(g_real_token);
    }
}


/*
struct_declaration:
    specifier-qualifier-list struct-declarator-list;
*/
static Decl * struct_declaration()
{
    TypeSpec * type_spec = specifier_qualifier_list();
    if (type_spec == NULL) {
        err(g_real_line_num,
            "miss qualifier, illegal member declaration of struct");
        consume_tok_to_semi();
        return NULL;
    }

    TypeSpec * qualifier = new_type();
    extract_qualifier(type_spec, qualifier);

    Decl * dcl_list = struct_declarator_list(qualifier);
    while (dcl_list != NULL) {
        Decl * dcl = dcl_list;
        dcl_list = DECL_next(dcl_list);
        DECL_next(dcl) = DECL_prev(dcl) = NULL;

        Decl * declaration = new_decl(DCL_DECLARATION);
        DECL_spec(declaration) = type_spec;
        DECL_decl_list(declaration) = dcl;
        DECL_align(declaration) = g_alignment;
        DECL_decl_scope(declaration) = g_cur_scope;
        DECL_lineno(declaration) = g_real_line_num;

        if (is_user_type_decl(declaration)) {
            err(g_real_line_num,
                "illegal storage class, should not use typedef in "
                "struct/union declaration.");
            continue;
        }

        if (IS_USER_TYPE_REF(type_spec)) {
            declaration = factor_user_type(declaration);
            DECL_align(declaration) = g_alignment;
            DECL_decl_scope(declaration) = g_cur_scope;
            DECL_lineno(declaration) = g_real_line_num;
        }

        add_next(&SCOPE_decl_list(g_cur_scope), declaration);
        DECL_decl_scope(declaration) = g_cur_scope;
    }

    if (g_real_token != T_SEMI) {
        err(g_real_line_num, "expected ';' after struct declaration");
    } else {
        match(T_SEMI);
    }
    return SCOPE_decl_list(g_cur_scope);
}


static Decl * union_declaration_list()
{
    while (g_real_token != T_RLPAREN) {
        if (g_real_token == T_END ||
            g_real_token == T_NUL ||
            is_too_many_err()) {
            return SCOPE_decl_list(g_cur_scope);
        }
        union_declaration();
    }
    return SCOPE_decl_list(g_cur_scope);
}


static Decl * struct_declaration_list()
{
    while (g_real_token != T_RLPAREN) {
        if (g_real_token == T_END ||
            g_real_token == T_NUL ||
            is_too_many_err()) {
            return SCOPE_decl_list(g_cur_scope);
        }

        Decl * field_decl = NULL;
        if ((field_decl = struct_declaration()) == NULL) {
            break;
        }
    }
    return SCOPE_decl_list(g_cur_scope);
}


static TypeSpec * type_spec_struct(TypeSpec * ty)
{
    TYPE_des(ty) |= T_SPEC_STRUCT;
    match(T_STRUCT);
    if (ck_type_spec_legally(ty) != ST_SUCC) {
        err(g_real_line_num, "type specifier is illegal");
        return ty;
    }

    Struct * s = NULL;
    if (g_real_token == T_ID) {
        /*
        struct definition
        format is: 'struct' 'TAG' '{' ... '}' 'ID';
        Find current and all of outer scope to find the
        identical declaration or declaring, and refill
        the declaring declared previous.
        */
        if (!is_struct_exist_in_outer_scope(g_real_token_string, &s)) {
            s = (Struct*)xmalloc(sizeof(Struct));
            STRUCT_tag(s) = g_fe_sym_tab->add(g_real_token_string);
            STRUCT_is_complete(s) = false;
            SCOPE_struct_list(g_cur_scope).append_tail(s);
        }
        match(T_ID);
    }

    if (g_real_token == T_LLPAREN) {
        /*
        formas is either: struct TAG { ...
        or: struct { ...
        */
        if (s == NULL) {
            //The struct declarated without TAG.
            s = (Struct*)xmalloc(sizeof(Struct));
            STRUCT_tag(s) = NULL;
            STRUCT_is_complete(s) = false;
            SCOPE_struct_list(g_cur_scope).append_tail(s);
        }

        //Report error if there already exist a declaration before.
        if (STRUCT_is_complete(s)) {
            err(g_real_line_num, "struct '%s' redefined",
                SYM_name(STRUCT_tag(s)));
            return ty;
        }

        match(T_LLPAREN);
        enter_sub_scope(false);
        UINT errn = g_err_msg_list.get_elem_count();
        STRUCT_decl_list(s) = struct_declaration_list();
        return_to_parent_scope();

        if (g_err_msg_list.get_elem_count() == errn) {
            STRUCT_is_complete(s) = true;
        }

        //Numbering field id
        Decl * field = STRUCT_decl_list(s);
        INT i = 0;
        while (field != NULL) {
            DECL_fieldno(field) = i++;
            DECL_is_sub_field(field) = true;
            DECL_base_type_spec(field) = ty;
            field = DECL_next(field);
        }

        if (match(T_RLPAREN) != ST_SUCC) {
            err(g_real_line_num, "expected '}' after struct definition");
            return ty;
        }
    }

    /*
    We must change alignment always, because user may apply
    #pragma align anywhere.
    e.g
        #pragma align (4)
        struct A {...} a1;
        ...
        #pragma align (8)
        struct A a2;
        ...
        So, a1 and a2 are implement as different alignment!
    */
    if (s == NULL) {
        //There is neither 'TAG' nor '{'.
        err(g_real_line_num, "illegal use '%s'", g_real_token_string);
        return ty;
    }
    STRUCT_align(s) = g_alignment;
    TYPE_struct_type(ty) = s;
    return ty;
}


static TypeSpec * type_spec_union(TypeSpec * ty)
{
    TYPE_des(ty) |= T_SPEC_UNION;
    match(T_UNION);
    if (ck_type_spec_legally(ty) != ST_SUCC) {
        err(g_real_line_num, "type specifier is illegal");
        return ty;
    }

    Union * s = NULL;
    if (g_real_token == T_ID) {
        /*
        union definition
        format is: 'union' 'TAG' '{' ... '}' 'ID';
        */
        /*
        Find current and all of outer scope to find the
        identical declaration or declaring, and refill the
        declaration which has been declared before.
        */
        if (!is_union_exist_in_outer_scope(g_real_token_string, &s)) {
            s = (Union*)xmalloc(sizeof(Union));
            UNION_tag(s) = g_fe_sym_tab->add(g_real_token_string);
            UNION_is_complete(s) = false;
            SCOPE_union_list(g_cur_scope).append_tail(s);
        }
        match(T_ID);
    }

    if (g_real_token == T_LLPAREN) {
        //formas: union TAG { ...
        if (s == NULL) {
            //The struct declarated without TAG.
            s = (Union*)xmalloc(sizeof(Union));
            UNION_is_complete(s) = false;
            /*
            's' is a incomplete union declaration,
            and it is permitted while we define a
            non-pointer variable as member of 's'
            */
            SCOPE_union_list(g_cur_scope).append_tail(s);
        }

        //Report error if there already exist a declaration before.
        if (UNION_is_complete(s)) {
            err(g_real_line_num, "union '%s' redefined", SYM_name(UNION_tag(s)));
            return ty;
        }

        match(T_LLPAREN);
        enter_sub_scope(false);

        UINT errn = g_err_msg_list.get_elem_count();
        UNION_decl_list(s) = union_declaration_list();
        return_to_parent_scope();

        if (errn == g_err_msg_list.get_elem_count()) {
            UNION_is_complete(s) = true;
        }

        //Numbering field id
        Decl * field = UNION_decl_list(s);
        INT i = 0;
        while (field != NULL) {
            DECL_fieldno(field) = i++;
            DECL_is_sub_field(field) = true;
            DECL_base_type_spec(field) = ty;
            field = DECL_next(field);
        }

        if (match(T_RLPAREN) != ST_SUCC) {
            err(g_real_line_num, "expected '}' followed union definition");
            return ty;
        }
    }

    /*
    We must change alignment always, because user
    may apply #pragma align anywhere.
    e.g
        #pragma align (4)
        union A{...} a1;
        ...
        #pragma align (8)
        union A a2;
        ...
    So, a1 and a2 are implement as different alignment!
    */
    UNION_align(s) = g_alignment;
    if (s == NULL) {
        err(g_real_line_num, "illegal use '%s'", g_real_token_string);
        return ty;
    }

    TYPE_union_type(ty) = s;
    return ty;
}


static TypeSpec * type_spec(TypeSpec * ty)
{
    if (ty == NULL) {
        ty = new_type();
    }
    switch (g_real_token) {
    case T_VOID:
        match(T_VOID);
        TYPE_des(ty) |= T_SPEC_VOID;
        break;
    case T_CHAR:
        match(T_CHAR);
        TYPE_des(ty) |= T_SPEC_CHAR;
        break;
    case T_SHORT:
        match(T_SHORT);
        TYPE_des(ty) |= T_SPEC_SHORT;
        break;
    case T_INT:
        match(T_INT);
        TYPE_des(ty) |= T_SPEC_INT;
        break;
    case T_LONG:
        match(T_LONG);
        if (IS_TYPE(ty, T_SPEC_LONG)) {
            warn1("'long long' is not ANSI C89, but you can use 'longlong' for long integer");
            TYPE_des(ty) &= ~T_SPEC_LONG;
            TYPE_des(ty) |= T_SPEC_LONGLONG;
        } else if (IS_TYPE(ty, T_SPEC_LONGLONG)) {
            err(g_real_line_num, "type specifier is illegal");
            goto FAILED;
        } else {
            TYPE_des(ty) |= T_SPEC_LONG;
        }
        break;
    case T_LONGLONG:
        match(T_LONGLONG);
        TYPE_des(ty) |= T_SPEC_LONGLONG;
        break;
    case T_BOOL:
        match(T_BOOL);
        TYPE_des(ty) |= T_SPEC_BOOL;
        break;
    case T_FLOAT:
        match(T_FLOAT);
        TYPE_des(ty) |= T_SPEC_FLOAT;
        break;
    case T_DOUBLE:
        match(T_DOUBLE);
        TYPE_des(ty) |= T_SPEC_DOUBLE;
        break;
    case T_SIGNED:
        match(T_SIGNED);
        TYPE_des(ty) |= T_SPEC_SIGNED;
        break;
    case T_UNSIGNED:
        match(T_UNSIGNED);
        TYPE_des(ty) |= T_SPEC_UNSIGNED;
        break;
    case T_STRUCT:
        return type_spec_struct(ty);
    case T_UNION:
        return type_spec_union(ty);
    default:; //do nothing
    }
    return ty;
FAILED:
    return ty;
}


/*
enumerator:
    identifier
    identifier = constant_expression
*/
static EnumValueList * enumrator()
{
    EnumValueList * evl = NULL;
    Enum * e = NULL;
    LONGLONG idx = 0;
    if (g_real_token == T_ID) {
        evl = (EnumValueList*)xmalloc(sizeof(EnumValueList));
        EVAL_LIST_name(evl) = g_fe_sym_tab->add(g_real_token_string);
        if (is_enum_const_exist_in_cur_scope(g_real_token_string,
                                             &e, (INT*)&idx)) {
            err(g_real_line_num, "'%s' : redefinition , different basic type",
                 g_real_token_string);
            goto FAILED;
        }
        match(T_ID);
        if (g_real_token == T_ASSIGN) {
            match(T_ASSIGN);
            //constant expression
            if (is_in_first_set_of_exp_list(g_real_token)) {
                Tree * t = conditional_exp();
                idx = 0;
                if (t == NULL) {
                    err(g_real_line_num, "empty constant expression");
                    goto FAILED;
                }
                if (!compute_constant_exp(t, &idx, 0)) {
                    err(g_real_line_num, "expected constant expression");
                    goto FAILED;
                }
                EVAL_LIST_val(evl) = (INT)idx;
            } else {
                err(g_real_line_num,
                    "syntax error : constant expression cannot used '%s'",
                    g_real_token_string);
                goto FAILED;
            }
        }
    }
FAILED:
    return evl;
}


/*
enumerator_list:
    enumerator
    enumerator_list , enumerator
*/
static EnumValueList * enumerator_list()
{
    EnumValueList * evl = enumrator(), * nevl = NULL;
    if (evl == NULL) { return NULL; }

    EnumValueList * last = get_last(evl);
    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        nevl = enumrator();
        if (nevl == NULL) { break; }
        add_next(&evl, &last, nevl);
        last = nevl;
    }
    return evl;
}


/*
enum_specifier:
    enum identifier { enumerator_list }
    enum            { enumerator_list }
    enum identifier
*/
static TypeSpec * enum_spec(TypeSpec * ty)
{
    if (ty == NULL) { ty = new_type(); }

    TYPE_des(ty) |= T_SPEC_ENUM;
    match(T_ENUM);

    if (g_real_token == T_ID) {
        //Parse enum name. Note that the name is optional.
        SYM * sym = g_fe_sym_tab->add(g_real_token_string);
        TYPE_enum_type(ty) = new_enum();
        ENUM_name(TYPE_enum_type(ty)) = sym;
        match(T_ID);
    }

    if (g_real_token == T_LLPAREN) {
        //Parse the definition of a list of enum constant.
        if (TYPE_enum_type(ty) == NULL) { TYPE_enum_type(ty) = new_enum(); }

        match(T_LLPAREN);

        ENUM_vallist(TYPE_enum_type(ty)) = enumerator_list();

        if (match(T_RLPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss '}' during enum type declaring");
            goto FAILED;
        }

        //Check enum name if it is given. The name is optional.
        Enum * e = NULL;
        SYM * enumname = ENUM_name(TYPE_enum_type(ty));
        if (enumname != NULL &&
            is_enum_id_exist_in_outer_scope(SYM_name(enumname), &e)) {
            err(g_real_line_num, "'%s' : enum type redefinition",
                SYM_name(enumname));
            goto FAILED;
        }
    }
    return ty;
FAILED:
    return ty;
}


/*
type_qualifier:  one of
    const
    volatile
*/
static TypeSpec * quan_spec(IN TypeSpec * ty)
{
    if (ty == NULL) {
        ty = new_type();
    }
    switch (g_real_token) {
    case T_CONST:
        match(T_CONST);
        if (IS_CONST(ty)) {
            err(g_real_line_num, "same type qualifier used more than once");
            goto FAILED;
        }
    #if (ALLOW_CONST_VOLATILE == 1)
        SET_FLAG(TYPE_des(ty), T_QUA_CONST);
    #else
        if (IS_VOLATILE(ty)) {
            err(g_real_line_num, "variable can not both const and volatile");
            goto FAILED;
        }
        REMOVE_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
        SET_FLAG(TYPE_des(ty), T_QUA_CONST);
    #endif
        break;
    case T_VOLATILE:
        match(T_VOLATILE);
        if (IS_VOLATILE(ty)) {
            err(g_real_line_num, "same type qualifier used more than once");
            goto FAILED;
        }

        //If there exist 'const' spec, so 'volatile' is omitted.
    #if (ALLOW_CONST_VOLATILE == 1)
        SET_FLAG(TYPE_des(ty), T_QUA_VOLATILE);
    #else
        if (IS_CONST(ty)) {
            err(g_real_line_num, "variable can not both const and volatile");
            goto FAILED;
        }
    #endif
        break;
    case T_RESTRICT:
        match(T_RESTRICT);
        SET_FLAG(TYPE_des(ty), T_QUA_RESTRICT);
        break;
    default:;
    }
    return ty;
FAILED:
    return ty;
}


/*
storage_class_specifier:  one of
    auto
    register
    static
    extern
    inline
    typedef
*/
static TypeSpec * stor_spec(IN TypeSpec * ty)
{
    if (ty == NULL) { ty = new_type(); }

    if ((HAVE_FLAG(TYPE_des(ty), T_STOR_AUTO) &&
         g_real_token != T_AUTO) ||
        (!ONLY_HAVE_FLAG(TYPE_des(ty), T_STOR_AUTO) &&
         g_real_token == T_AUTO)) {
        err(g_real_line_num,
            "auto can not specified with other type-specifier");
        goto FAILED;
    }

    if ((HAVE_FLAG(TYPE_des(ty), T_STOR_STATIC) &&
         g_real_token == T_EXTERN) ||
        (HAVE_FLAG(TYPE_des(ty), T_STOR_EXTERN) &&
         g_real_token == T_STATIC)) {
        err(g_real_line_num, "static and extern can not be specified meanwhile");
        goto FAILED;
    }

    switch (g_real_token) {
    case T_AUTO:
        match(T_AUTO);
        SET_FLAG(TYPE_des(ty), T_STOR_AUTO);
        break;
    case T_REGISTER:
        match(T_REGISTER);
        SET_FLAG(TYPE_des(ty), T_STOR_REG);
        break;
    case T_STATIC:
        match(T_STATIC);
        SET_FLAG(TYPE_des(ty), T_STOR_STATIC);
        break;
    case T_EXTERN:
        match(T_EXTERN);
        SET_FLAG(TYPE_des(ty), T_STOR_EXTERN);
        break;
    case T_INLINE:
        match(T_INLINE);
        SET_FLAG(TYPE_des(ty), T_STOR_INLINE);
        break;
    case T_TYPEDEF:
        match(T_TYPEDEF);
        SET_FLAG(TYPE_des(ty), T_STOR_TYPEDEF);
        break;
    default:;
    }
    return ty;
FAILED:
    return NULL;
}


/*
declaration_specifiers:
    storage_class_specifier declaration_specifiers
    storage_class_specifier
    type_specifier declaration_specifiers
    type_specifier
    type_qualifier declaration_specifiers
    type_qualifier
*/
static TypeSpec * declaration_spec()
{
    TypeSpec * ty = NULL, * p = NULL;
    for (;;) {
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
        case T_STRUCT:
        case T_UNION:
            ty = type_spec(ty);
            break;
        case T_ENUM:
            ty = enum_spec(ty);
            break;
        case T_CONST:
        case T_VOLATILE:
        case T_RESTRICT:
            ty = quan_spec(ty);
            break;
        case T_ID:
            {
                Decl * ut = NULL;
                Struct * s = NULL;
                Union * u = NULL;
                if (is_user_type_exist_in_outer_scope(
                                g_real_token_string, &ut)) {
                    if (ty != NULL) {
                        if (IS_USER_TYPE_REF(ty)) {
                            err(g_real_line_num, "redeclared user defined type.");
                            return ty;
                        }
                        if (IS_STRUCT(ty)) {
                            err(g_real_line_num, "redeclared struct type.");
                            return ty;
                        }
                        if (IS_UNION(ty)) {
                            err(g_real_line_num, "redeclared union type.");
                            return ty;
                        }
                    }

                    p = typedef_name(ty);
                    if (p == NULL) return ty;
                    ty = p;
                } else if (is_struct_exist_in_outer_scope(
                                g_real_token_string, &s)) {
                    if (ty != NULL) {
                        if (IS_USER_TYPE_REF(ty)) {
                            err(g_real_line_num, "redeclared user defined type.");
                            return ty;
                        }
                        if (IS_STRUCT(ty)) {
                            err(g_real_line_num, "redeclared struct type.");
                            return ty;
                        }
                        if (IS_UNION(ty)) {
                            err(g_real_line_num, "redeclared union type.");
                            return ty;
                        }
                    }
                    ASSERT0(s != NULL);
                    if (ty == NULL) {
                        ty = new_type();
                    }
                    TYPE_des(ty) |= T_SPEC_STRUCT;
                    TYPE_struct_type(ty) = s;
                    match(T_ID);
                } else if (is_union_exist_in_outer_scope(
                                g_real_token_string, &u)) {
                    if (ty != NULL) {
                        if (IS_USER_TYPE_REF(ty)) {
                            err(g_real_line_num, "redeclared user defined type.");
                            return ty;
                        }
                        if (IS_STRUCT(ty)) {
                            err(g_real_line_num, "redeclared struct type.");
                            return ty;
                        }
                        if (IS_UNION(ty)) {
                            err(g_real_line_num, "redeclared union type.");
                            return ty;
                        }
                    }

                    ASSERT0(u != NULL);
                    if (ty == NULL) {
                        ty = new_type();
                    }
                    TYPE_des(ty) |= T_SPEC_UNION;
                    TYPE_union_type(ty) = u;
                    match(T_ID);
                } else {
                    //g_real_token is not a type.
                    return ty;
                }
            }
            break;
        default: goto END;
        } //end switch
    } //end while
END:
    return ty;
}


//'fun_dclor': able to modify parameter list.
Decl * get_parameter_list(IN Decl * dcl, OUT Decl ** fun_dclor)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL && DECL_dt(dcl) != DCL_FUN) {
        dcl = DECL_next(dcl);
    }
    if (fun_dclor != NULL) {
        *fun_dclor = dcl;
    }
     dcl = DECL_fun_para_list(dcl);
    return dcl;
}


/*
parameter_declaration:
    declaration_specifiers declarator
    declaration_specifiers abstract_declarator
    declaration_specifiers
*/
static Decl * parameter_declaration()
{
    Decl * declaration = new_decl(DCL_DECLARATION);

    TypeSpec * type_spec = declaration_spec();
    if (type_spec == NULL) {
        return NULL;
    }

    complement_qua(type_spec);

    TypeSpec * qualifier = new_type();

    //Extract qualifier, and regarded it as the qualifier
    //to the subsequently POINTER or ID.
    extract_qualifier(type_spec, qualifier);

    //'DCL_ID' should be the list-head if it exist.
    Decl * dcl_list = reverse_list(abstract_declarator(qualifier));

    DECL_spec(declaration) = type_spec;

    if (dcl_list == NULL || (dcl_list != NULL && DECL_dt(dcl_list) == DCL_ID)) {
        DECL_decl_list(declaration) = new_decl(DCL_DECLARATOR);
    } else {
        DECL_decl_list(declaration) = new_decl(DCL_ABS_DECLARATOR);
    }

    DECL_child(DECL_decl_list(declaration)) = dcl_list;

    //array parameter has at least one elem.
    compute_array_dim(declaration, false);

    if (IS_USER_TYPE_REF(type_spec)) {
        //Factor the user defined type which via typedef.
        declaration = factor_user_type(declaration);
        DECL_align(declaration) = g_alignment;
        DECL_decl_scope(declaration) = g_cur_scope;
        DECL_lineno(declaration) = g_real_line_num;
    }

    return declaration;
}


/*
parameter_type_list:
    parameter_list
    parameter_list , ...
parameter_list:
    parameter_declaration
    parameter_list , parameter_declaration
The above bnf can covert to

parameter_type_list:
    parameter_declaration
    parameter_declaration , parameter_declaration
    parameter_declaration , ...

*/
static Decl * parameter_type_list()
{
    Decl * declaration = NULL , * t = NULL;
    for (;;) {
        t = parameter_declaration();
        if (t == NULL) {
            goto FIN;
        }
        add_next(&declaration, t);
        if (g_real_token == T_COMMA) {
            match(T_COMMA);
        } else if (g_real_token == T_RPAREN ||
                 g_real_token == T_END ||
                 g_real_token == T_NUL ||
                 is_too_many_err()) {
            break;
        }

        //'...' must be the last parameter-declarator
        if (g_real_token == T_DOTDOTDOT) {
            match(T_DOTDOTDOT);
            t = new_decl(DCL_VARIABLE);
            add_next(&declaration, t);
            break;
        }
    }
FIN:
    return declaration;
}


/*
direct_abstract_declarator:
    ( abstract_declarator )
    direct_abstract_declarator [ constant_expression ]
                               [ constant_expression ]
    direct_abstract_declarator [                     ]
                               [                     ]
                               (                     )
                               ( parameter_type_list )
    direct_abstract_declarator (                     )
    direct_abstract_declarator ( parameter_type_list )
*/
static Decl * direct_abstract_declarator(TypeSpec * qua)
{
    Decl * dcl = NULL, * ndcl = NULL;
    switch (g_real_token) {
    case T_LPAREN: //'(' abstract_declarator ')'
        match(T_LPAREN);
        dcl = abstract_declarator(qua);
        //Here 'dcl' can be NUL L
        if (match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')'");
            goto FAILED;
        }
        DECL_is_paren(dcl) = 1;
        break;
    case T_ID: //identifier
        {
            SYM * sym = g_fe_sym_tab->add(g_real_token_string);
            add_to_symtab_list(&SCOPE_sym_tab_list(g_cur_scope), sym);
            dcl = new_decl(DCL_ID);
            DECL_id(dcl) = id();
            DECL_qua(dcl) = qua;
            match(T_ID);
        }
        break;
    default:;
    }

    switch (g_real_token) {
    case T_LSPAREN: //outer level operator is ARRAY
        {
            Tree * t = NULL;
            while (g_real_token == T_LSPAREN) {
                match(T_LSPAREN);
                Decl * ndcl = new_decl(DCL_ARRAY);
                t = conditional_exp();
                if (match(T_RSPAREN) != ST_SUCC) {
                    err(g_real_line_num, "miss ']'");
                    goto FAILED;
                }
                DECL_array_dim_exp(ndcl) = t;

                //'id' should be the last one in declarator-list.
                insertbefore_one(&dcl, dcl, ndcl);
            }
        }
        break;
    case T_LPAREN:
        {
            //current level operator is function-pointer/function-definition
            //Parameter list.
            match(T_LPAREN);
            ndcl = new_decl(DCL_FUN);
            //DECL_fun_base(ndcl) = dcl;
            enter_sub_scope(true);
            DECL_fun_para_list(ndcl) = parameter_type_list();
            return_to_parent_scope();
            insertbefore_one(&dcl, dcl, ndcl);
            if (match(T_RPAREN) != ST_SUCC) {
                err(g_real_line_num, "miss ')'");
                goto FAILED;
            }
        }
        break;
    default:;
    }
    return dcl;
FAILED:
    return dcl;
}


/*
abstract_declarator:
    pointer
    pointer direct_abstract_declarator
            direct_abstract_declarator
*/
static Decl * abstract_declarator(TypeSpec * qua)
{
    Decl * ptr = pointer(&qua);
    Decl * dcl = direct_abstract_declarator(qua);
    if (ptr == NULL && dcl == NULL) {
        return NULL;
    } else if (dcl == NULL) {
        return ptr;
    } else {
        /*
        Keep DCL_ID is the last one if it exist.
        e.g:
            ptr is '*', dcl is '[]'->'ID'
            return: '*'->'[]'->'ID'
        */
        insertbefore(&dcl, dcl, ptr);
    }
    return dcl;
}


/*
specifier_qualifier_list:
    type_specifier specifier_qualifier_list
    type_specifier
    type_qualifier specifier_qualifier_list
    type_qualifier
*/
static TypeSpec * specifier_qualifier_list()
{
    TypeSpec * ty = NULL;
    TypeSpec * p = NULL;
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
            ty = quan_spec(ty);
            break;
        case T_ID:
            p = typedef_name(ty);
            if (p == NULL) {return ty;}
            ty = p;
            break;
        default: goto END;
        }//end switch
    }//end while
END:
    return ty;
}


/*
type_name:
    specifier_qualifier_list abstract_declarator
    specifier_qualifier_list
NOTICE: Do not include user defined type
*/
Decl * type_name()
{
    //Parse specifier and qualifier.
    //e.g: char * const*, here 'char' is specifier, '* const*' is qualifier.
    TypeSpec * type_spec = specifier_qualifier_list();
    if (type_spec == NULL) {
        return NULL;
    }

    /* Parse POINTER/ARRAY/ID, and complement their qualifier.
    Collect const/volatile prefix, add them to POINTER/ARRAY/ID.
    e.g: const int a; Add const qualifier to ID 'a'.
        const int * a; Add const qualifier to POINTER '*'.
    */
    TypeSpec * qualifier = new_type();
    extract_qualifier(type_spec, qualifier);
    Decl * abs_decl = abstract_declarator(qualifier);

    //Generate type_name.
    Decl * type_name = new_decl(DCL_TYPE_NAME);
    DECL_spec(type_name) = type_spec;
    DECL_decl_list(type_name) = new_decl(DCL_ABS_DECLARATOR);
    DECL_child(DECL_decl_list(type_name)) = reverse_list(abs_decl);
    complement_qua(type_spec);
    compute_array_dim(type_name, false);
    return type_name;
}


/*
initializer_list:
    initializer
    initializer_list , initializer
*/
static Tree * initializer_list(TypeSpec * qua)
{
    Tree * t = initializer(qua);
    if (t == NULL) { return NULL; }

    Tree * last = get_last(t);
    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        if (g_real_token == T_RLPAREN) { break; }

        Tree * nt = initializer(qua);
        if (nt == NULL) { break; }

        add_next(&t, &last, nt);

        last = get_last(nt);
    }
    return t;
}


/*
initializer:
    assignment_expression
    { initializer_list }
    { initializer_list , }
*/
static Tree * initializer(TypeSpec * qua)
{
    Tree * t = NULL, * es = NULL;
    switch (g_real_token) {
    case T_LLPAREN:
        match(T_LLPAREN);
        t = initializer_list(qua);
        if (g_real_token == T_COMMA) {
            match(T_COMMA);
            if (match(T_RLPAREN) != ST_SUCC) {
                err(g_real_line_num, "syntax error '%s'", g_real_token_string);
                goto FAILED;
            }
        } else if (match(T_RLPAREN) != ST_SUCC) {
            err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
            goto FAILED;
        }
        es = new_tree_node(TR_EXP_SCOPE, g_real_line_num);
        TREE_exp_scope(es) = t;
        t = es;
        break;
    default:
        if (is_in_first_set_of_exp_list(g_real_token)) {
            t = exp();
        } else {
            err(g_real_line_num, "syntax error : initializing cannot used '%s'",
                 g_real_token_string);
            goto FAILED;
        }
    }
    return t;
FAILED:
    return t;

}


/*
struct_declarator:
    declarator
               : constant_expression
    declarator : constant_expression
*/
static Decl * struct_declarator(TypeSpec * qua)
{
    LONGLONG idx = 0;
    Decl * dclr = declarator(qua);
    if (dclr == NULL) {
        return NULL;
    }
    dclr = reverse_list(dclr);
    Decl * declarator = new_decl(DCL_DECLARATOR);
    DECL_child(declarator) = dclr;
    compute_array_dim(declarator, true);
    if (g_real_token == T_COLON) {
        Tree * t = NULL;
        if (is_indirection(dclr)) {
            SYM * s = get_decl_sym(dclr);
            ASSERT(s != NULL, ("member name cannot be NULL"));
            err(g_real_line_num,
                "'%s' : pointer type cannot assign bit length", SYM_name(s));
            goto FAILED;
        }
        match(T_COLON);
        t = conditional_exp();
        if (!compute_constant_exp(t, &idx, 0)) {
            err(g_real_line_num, "expected constant expression");
            goto FAILED;
        }
        //bit length must be check in typeck.cpp
        DECL_bit_len(declarator) = (INT)idx;
        DECL_is_bit_field(declarator) = true;
    }
    return declarator;
FAILED:
    return declarator;
}


/*
struct_declarator_list:
    struct_declarator
    struct_declarator_list , struct_declarator
*/
static Decl * struct_declarator_list(TypeSpec * qua)
{
    Decl * dclr = struct_declarator(qua), * ndclr = NULL;
    if (dclr == NULL) return NULL;
    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        ndclr = struct_declarator(qua);
        add_next(&dclr, ndclr);
    }
    return dclr;
}


/*
Pick out the pure declarator specification list
    e.g:
        int * a [10];
        the pure declarator list is :  a->[10]->*

        int (*) [10];
        the pure declarator list is :  *->[10]
*/
Decl * get_pure_declarator(Decl * decl)
{
    ASSERT0(decl != NULL);
    switch (DECL_dt(decl)) {
    case DCL_ARRAY:
    case DCL_POINTER:
        break;
    case DCL_FUN:
        //function-pointer type.
        break;
    case DCL_ID:
        break;
    case DCL_VARIABLE:
        ASSERT(0, ("can not in declaration"));
        break;
    case DCL_TYPE_NAME:
        decl = DECL_decl_list(decl);
        if (decl == NULL) {
            return NULL;
        }
        ASSERT(DECL_dt(decl) == DCL_ABS_DECLARATOR,
                ("must be DCL_ABS_DECLARATOR in TYPE_NAME"));
        decl = DECL_child(decl);
        break;
    case DCL_DECLARATOR:
        decl = DECL_child(decl);
        break;
    case DCL_DECLARATION:
        decl = DECL_decl_list(decl);
        if (decl == NULL) {
            return NULL;
        }
        ASSERT0(DECL_dt(decl) == DCL_DECLARATOR ||
                 DECL_dt(decl) == DCL_ABS_DECLARATOR);
        decl = DECL_child(decl);
        break;
    case DCL_ABS_DECLARATOR:
        decl = DECL_child(decl);
        break;
    default: ASSERT(0, ("unknown Decl"));
    }
    return decl;
}


/* Get the number of element to given dimension.
Note the field DECL_array_dim of array is only
available after compute_array_dim() invoked, and
it compute the really number of array element via
DECL_array_dim_exp, that is a constant expression.
'arr': array declaration.
'dim': given dimension to compute, start at 0. */
ULONG get_array_elemnum_to_dim(Decl * arr, UINT dim)
{
    Decl * dcl = get_array_decl(arr);
    ASSERT0(dcl);
    UINT i = 0;
    while (i < dim && dcl != NULL) {
        if (DECL_dt(dcl) != DCL_ARRAY) {
            break;
        }
        dcl = DECL_next(dcl);
        i++;
    }

    if (dcl == NULL || DECL_dt(dcl) != DCL_ARRAY) {
        return 0;
    }
    return (ULONG)DECL_array_dim(dcl);
}


/* Calculate constant expression when parsing the variable
declaration of array type.

'allow_dim0_is_empty': parameter array's lowest dimension size
can NOT be zero. */
static INT compute_array_dim(Decl * dclr, bool allow_dim0_is_empty)
{
    //e.g: int (*(a[1][2]))[3][4];
    BYTE dim = 0;
    INT st = ST_SUCC;
    dclr = get_pure_declarator(dclr);
    while (dclr != NULL) {
        if (DECL_dt(dclr) == DCL_ARRAY) {
            dim++;
        } else {
            /*
            Recompute dim size for next array type:
            e.g: int (*(a[1][2]))[3][4];
            pure dclr: ID(a)->[1]->[2]->PTR->[3]->[4]
            */
            dim = 0;
        }

        if (dim >= 1) {
            Tree * t = DECL_array_dim_exp(dclr);
            LONGLONG idx = 0;
            if (t == NULL) {
                if (dim > 1) {
                    err(g_real_line_num,
                        "size of dimension %dth can not be zero, may be miss subscript",
                        dim);
                    st = ST_ERR;
                    goto NEXT;
                }
                if (!allow_dim0_is_empty) {
                    //If size of dim0 is 0, set it to 1 by default means the array
                    //contain at least one element.
                    idx = 1;
                }
            } else if (t != NULL) {
                if (!compute_constant_exp(t, &idx, 0)) {
                    err(g_real_line_num, "expected constant expression");
                    st = ST_ERR;
                    goto NEXT;
                }
                 if (idx < 0 || idx > MAX_ARRAY_INDX) {
                    err(g_real_line_num, "negative subscript or subscript is too large");
                    st = ST_ERR;
                    goto NEXT;
                }
                if (idx == 0 && t != NULL) {
                    err(g_real_line_num, "cannot allocate an array of constant size 0");
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


/*
init_declarator:
    declarator
    declarator = initializer
*/
static Decl * init_declarator(TypeSpec * qua)
{
    Decl * dclr = declarator(qua);
    if (dclr == NULL) return NULL;
    dclr = reverse_list(dclr);

    //dclr is DCL_DECLARATOR node
    Decl * declarator = new_decl(DCL_DECLARATOR);
    DECL_child(declarator) = dclr;
    compute_array_dim(declarator,
                true /*array dim size should be determined by init value.*/);

    if (g_real_token == T_ASSIGN) {
        match(T_ASSIGN);
        DECL_init_tree(declarator) = initializer(qua);
        if (DECL_init_tree(declarator) == NULL ||
           (TREE_type(DECL_init_tree(declarator)) == TR_EXP_SCOPE &&
            TREE_exp_scope(DECL_init_tree(declarator)) == NULL)) {
            err(g_real_line_num, "initialization cannot be NULL");
            suck_tok_to(0,T_SEMI,T_END,T_NUL);
        }
        DECL_is_init(declarator) = 1;
    }
    return declarator;
}


/*
init_declarator_list:
    init_declarator
    init_declarator_list, init_declarator
*/
static Decl * init_declarator_list(TypeSpec * qua)
{
    Decl * dclr = init_declarator(qua);
    if (dclr == NULL) {
        return NULL;
    }

    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        Decl * ndclr = init_declarator(qua);
        add_next(&dclr, ndclr);
    }
    return dclr;
}


/*
ARRAY MODE:     S (D)[e]
POINTER MODE:   S * D
FUNCTION MODE:  S (D)(p)

direct_declarator:
    identifier
    (declarator)
    direct_declarator [ constant_expression ]
    direct_declarator [                     ]
    direct_declarator ( parameter_type_list )
    //direct_declarator ( identifier_list ) obsolete C proto
    direct_declarator (                 )
*/
static Decl * direct_declarator(TypeSpec * qua)
{
    INT is_paren = 0;
    Decl * dcl = NULL;
    switch (g_real_token) {
    case T_LPAREN: //'(' declarator ')'
        match(T_LPAREN);
        dcl = declarator(qua);
        if (match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')'");
            goto FAILED;
        }
        if (dcl == NULL) {
            err(g_real_line_num, "must have identifier declared");
            goto FAILED;
        }
        is_paren = 1;
        break;
    case T_ID: //identifier
        {
            SYM * sym = g_fe_sym_tab->add(g_real_token_string);
            add_to_symtab_list(&SCOPE_sym_tab_list(g_cur_scope), sym);
            dcl = new_decl(DCL_ID);
            DECL_id(dcl) = id();
            DECL_qua(dcl) = qua;
            match(T_ID);
        }
        break;
    default:;
    }

    if (dcl == NULL) { return NULL; }

    switch (g_real_token) {
    case T_LSPAREN: //'[', the declarator is an array.
        {
            Tree * t = NULL;
            while (g_real_token == T_LSPAREN) {
                match(T_LSPAREN);
                Decl * ndcl = new_decl(DCL_ARRAY);
                t = conditional_exp();
                if (match(T_RSPAREN) != ST_SUCC) {
                    err(g_real_line_num, "miss ']'");
                    goto FAILED;
                }
                DECL_array_dim_exp(ndcl) = t;
                DECL_is_paren(ndcl) = is_paren;

                //'id' should be the last one in declarator-list.
                insertbefore_one(&dcl, dcl, ndcl);
            }
        }
        break;
    case T_LPAREN: //'(', the declarator is a function decl/def.
        {
            match(T_LPAREN);
            Decl * ndcl = new_decl(DCL_FUN);
            enter_sub_scope(true);
            DECL_fun_para_list(ndcl) = parameter_type_list();
            return_to_parent_scope();
            DECL_is_paren(ndcl) = is_paren;
            insertbefore_one(&dcl, dcl, ndcl);
            if (match(T_RPAREN) != ST_SUCC) {
                err(g_real_line_num, "miss ')'");
                goto FAILED;
            }
        }
        break;
    default:; //do nothing
    }
    return dcl;
FAILED:
    return dcl;
}


//Copy specifier.
TypeSpec * cp_spec(TypeSpec * ty)
{
    TypeSpec * new_ty = new_type();
    new_ty->copy(*ty);
    return new_ty;
}


/*
pointer:
    '*' type-qualifier-list(pass)
    '*' type-qualifier-list(pass) pointer
*/
static Decl * pointer(TypeSpec ** qua)
{
    Decl * ndcl = NULL;
    TypeSpec * new_qua = *qua;
    while (g_real_token == T_ASTERISK) {
        match(T_ASTERISK);
        Decl * dcl = new_decl(DCL_POINTER);
        DECL_qua(dcl) = new_qua;
        new_qua = new_type();
        quan_spec(new_qua);
        if (IS_RESTRICT(new_qua)) {
            SET_FLAG(TYPE_des(DECL_qua(dcl)), T_QUA_RESTRICT);
            REMOVE_FLAG(TYPE_des(new_qua), T_QUA_RESTRICT);
        }
        add_next(&ndcl, dcl);
    }
    quan_spec(new_qua); //match qualifier for followed ID.
    *qua = new_qua;
    return ndcl;
}


/*
declarator:
    pointer  direct_declarator
             direct_declarator
*/
static Decl * declarator(TypeSpec * qua)
{
    Decl * ptr = pointer(&qua);
    Decl * dclr = direct_declarator(qua);
    if (dclr == NULL) {
        return NULL;
    }

    /*
    e.g:
        ptr is '*', dclr is '[]'->'ID'
        return: '*'->'[]'->'ID'
    */
    insertbefore(&dclr, dclr, ptr);
    return dclr; //'id' is the list tail.
}


static INT label_ck(SCOPE *s)
{
    if (s == NULL) return ST_ERR;
    LabelInfo * lref = SCOPE_ref_label_list(s).get_head();
    LabelInfo * lj = NULL;
    while (lref != NULL) {
        CHAR * name = SYM_name(LABEL_INFO_name(lref));
        ASSERT0(name);
        LabelInfo * li = SCOPE_label_list(s).get_head();
        for (; li != NULL; li = SCOPE_label_list(s).get_next()) {
            if (strcmp(SYM_name(LABEL_INFO_name(li)), name) == 0) {
                LABEL_INFO_is_used(li) = true;
                break;
            }
        }
        if (li == NULL) {
            err(map_lab2lineno(lref), "label '%s' was undefined", name);
            goto FAILED;
        }
        lref = SCOPE_ref_label_list(s).get_next();
    }

    lj = SCOPE_label_list(s).get_head();
    for (; lj != NULL; lj = SCOPE_label_list(s).get_next()) {
        if (!LABEL_INFO_is_used(lj)) {
            warn1("'%s' unreferenced label", SYM_name(LABEL_INFO_name(lj)));
        }
    }
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


CHAR * dump_decl(Decl * dcl, CHAR * buf)
{
    if (g_tfile == NULL) return NULL;
    bool is_ret = true;
    if (buf == NULL) {
        CHAR tmp[MAX_BUF_LEN*10];
        tmp[0] = 0;
        buf = tmp;
        is_ret = false;
    }
    format_declaration(buf, dcl);
    fflush(g_tfile);
    if (!is_ret) {
        fprintf(g_tfile, "\n%s\n", buf);
        fflush(g_tfile);
        return NULL;
    }
    fflush(g_tfile);
    return buf;
}


void dump_decl(Decl * dcl)
{
    format_declaration(dcl, 0);
}


static void fix_para_array_index(Decl * decl)
{
    TypeSpec * ty = NULL;
    ASSERT0(DECL_is_formal_para(decl));
    ASSERT0(is_pointer(decl));
    Decl * d = get_pointer_base_decl(decl, &ty);
    if (d == NULL || DECL_dt(d) == DCL_POINTER) { return; }

    if (DECL_dt(d) == DCL_ARRAY && DECL_array_dim(d) == 0) {
        /*
        Fix array index, it can not be 0.
        C allows the first dimension of parameter be zero.
        e.g: void foo (char p[][20]) is legal syntax, but
            the declaration is char p[1][20].
        */
        DECL_array_dim(d) = 1;
    }

    if (get_declarator_size_in_byte(d) == 0) {
        err(g_real_line_num,
            "Only the first dimension size can be 0, "
            "the lower dimension size can not be 0");
    }
}


//Change array to pointer if it is formal parameter.
//Fulfill the first dimension to at least 1 if it is a parameter.
static Tree * refineArray(Tree * t)
{
    ASSERT0(TREE_type(t) == TR_ARRAY);

    /*
    Formal parameter of array type is a pointer in actually.
    Insert a Dereference to comfort the C specification.
    */
    Tree * base = TREE_array_base(t);
    if (TREE_type(base) == TR_ID) {
        //dump_decl(TREE_id_decl(base), 0);

        //ID is unique to its scope.
        CHAR * name = SYM_name(TREE_id(base));
        ASSERT0(TREE_id_decl(base));
        SCOPE * s = DECL_decl_scope(TREE_id_decl(base));
        Decl * decl = get_decl_in_scope(name, s);
        ASSERT0(decl != NULL);
        if (DECL_is_formal_para(decl)) {
            /*
            Verfiy and fix formal parameters with array type.
            Check if decl is pointer that pointed to an array.
            e.g: 'int (*p)[]'
            the referrence should do same operation as its declaration.
            */
            Decl * base_of_pt = get_pure_declarator(decl);
            if (DECL_dt(base_of_pt) == DCL_ID) {
                base_of_pt = DECL_next(base_of_pt);
            }

            if (base_of_pt != NULL && DECL_dt(base_of_pt) == DCL_POINTER) {
                if (DECL_next(base_of_pt) != NULL &&
                    DECL_dt(DECL_next(base_of_pt)) == DCL_ARRAY) {
                    base_of_pt = DECL_next(base_of_pt);
                }
            }

            if (base_of_pt != NULL && DECL_dt(base_of_pt) == DCL_ARRAY) {
                //The base of pointer is an array. Convert a[] to (*a)[].
                Tree * deref = new_tree_node(TR_DEREF, TREE_lineno(base));
                TREE_lchild(deref) = base;
                setParent(deref, TREE_lchild(deref));
                TREE_array_base(t) = deref;
                setParent(t, TREE_array_base(t));
                fix_para_array_index(decl);
            }
        }
    }
    return t;
}


/*
Do refinement and amendment for tree.
    * Revise formal parameter. In C spec, formal array is pointer
      that point to an array in actually.
*/
static Tree * refine_tree(Tree * t)
{
    if (t == NULL) return NULL;
    if (TREE_type(t) == TR_ARRAY) {
        t = refineArray(t);
    } else if (TREE_type(t) == TR_SCOPE) {
        SCOPE * s = TREE_scope(t);
        SCOPE_stmt_list(s) = refine_tree_list(SCOPE_stmt_list(s));
    }

    for (UINT i = 0; i < MAX_TREE_FLDS; i++) {
        refine_tree_list(TREE_fld(t, i));
    }
    return t;
}


static Tree * refine_tree_list(Tree * t)
{
    if (t == NULL) return NULL;
    Tree * head = t;
    int i = 0;
    while (t != NULL) {
        refine_tree(t);
        t = TREE_nsib(t);
        i++;
    }
    return head;
}


//Convert Tree in terms of C specification.
static void refine_func(Decl * func)
{
    SCOPE * scope = DECL_fun_body(func);
    Tree * t = SCOPE_stmt_list(scope);
    if (t != NULL) {
        t = refine_tree_list(t);
        ASSERT(TREE_parent(t) == NULL, ("parent node of Tree is NULL"));
        SCOPE_stmt_list(scope) = t;
    }
}


/*
Converts 'decl' into pointer type from its origin type.
NOTCIE: 'decl' cannot be pointer.

'decl': a declarator, not a pointer type.
'is_append': transform to pointer type by appending a DCL_POINTER.
    In order to achieve this purpose, insert a DCL_POINTER type before
    the first array type.
    e.g: ID->ARRAY->ARRAY => ID->POINTER->ARRAY->ARRAY
*/
Decl * trans_to_pointer(Decl * decl, bool is_append)
{
    ASSERT(DECL_dt(decl) == DCL_DECLARATION,
            ("only DCRLARATION is valid"));
    ASSERT(!is_pointer(decl), ("only DCRLARATION is valid"));
    Decl * pure = get_pure_declarator(decl);
    Decl * p = pure;
    Decl * new_pure = NULL;
    bool isdo = true;
    INT count = 0;
    while (pure != NULL) {
        switch (DECL_dt(pure)) {
        case DCL_FUN: //Function declarator
        case DCL_ID: //Identifier
        case DCL_VARIABLE: //Variable length parameter
        case DCL_POINTER: //POINTER  declarator
            {
                if (count > 0) {
                    isdo = false;
                }
                p = cp_decl(pure);
                add_next(&new_pure, p);
                break;
            }
        case DCL_ARRAY: //ARRAY declarator
            {
                if (is_append) {
                    is_append = false;
                    p = new_decl(DCL_POINTER);
                    add_next(&new_pure, p);
                    isdo = false;
                }

                if (!isdo) {
                    p = cp_decl(pure);
                    DECL_is_paren(p) = 1;
                    add_next(&new_pure, p);
                } else {
                    count++;
                    p = new_decl(DCL_POINTER);
                    add_next(&new_pure, p);
                }
                break;
            }
        case DCL_TYPE_NAME:
            //if current decl is TYPE_NAME,  it descript a declarator

        case DCL_DECLARATOR:  //declarator
        case DCL_DECLARATION: //declaration
        case DCL_ABS_DECLARATOR: //abstract declarator
        default:
            ASSERT(0, ("unexpected Decl type over here"));
        }
        pure = DECL_next(pure);
    }
    PURE_DECL(decl) = new_pure;
    ASSERT(is_pointer(decl), ("transform failed!"));
    return decl;
}


//Return Enum if it has exist in 'elst', otherwise return NULL.
Enum * find_enum(EnumList * elst , Enum * e)
{
    if (elst == NULL || e == NULL) { return NULL; }
    EnumList *    p = elst;
    while (p != NULL) {
        if (ENUM_LIST_enum(p) == e) {
            return e;
        }
        p = ENUM_LIST_next(p);
    }
    return NULL;
}


/*
Return NULL indicate we haven't found it in 'ut_list', and
append 'ut' to tail of the list as correct, otherwise return
the finded one.
*/
Decl * add_to_user_type_list(UserTypeList ** ut_list , Decl * decl)
{
   if (ut_list == NULL || decl == NULL) return NULL;
   if ((*ut_list) == NULL) {
        *ut_list = (UserTypeList*)xmalloc(sizeof(UserTypeList));
        USER_TYPE_LIST_utype(*ut_list) = decl;
        return NULL;
   } else {
        UserTypeList * p = *ut_list, * q = NULL;
        while (p != NULL) {
            q = p;
            if (USER_TYPE_LIST_utype(p) == decl) {
                //'sym' already exist, return 'sym' as result
                return decl;
            }
            p = USER_TYPE_LIST_next(p);
        }
        USER_TYPE_LIST_next(q) =
            (UserTypeList*)xmalloc(sizeof(UserTypeList));
        USER_TYPE_LIST_prev(USER_TYPE_LIST_next(q)) = q;
        q = USER_TYPE_LIST_next(q);
        USER_TYPE_LIST_utype(q) = decl;
    }
    return NULL;
}


//Return true if enum-value existed.
static bool is_enum_exist(IN EnumList * e_list, IN CHAR * e_name,
                          OUT Enum ** e, OUT INT * idx)
{
    if (e_list == NULL || e_name == NULL) return false;
    EnumList * el = e_list;
    while (el != NULL) {
        if (is_enum_const_name_exist(ENUM_LIST_enum(el), e_name, idx)) {
            *e = ENUM_LIST_enum(el);
            return true;
        }
        el = ENUM_LIST_next(el);
    }
    return false;
}


//Enum typed identifier is effective at all of outer scopes.
bool is_enum_id_exist_in_outer_scope(IN CHAR * cl, OUT Enum ** e)
{
    SCOPE * sc = g_cur_scope;
    while (sc != NULL) {
        if (is_enum_id_exist(SCOPE_enum_list(sc), cl, e)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


/*
Return true if the struct typed declaration have already existed in both
current and all of outer scopes.
*/
bool is_struct_exist_in_outer_scope(IN CHAR * tag, OUT Struct ** s)
{
    SCOPE * sc = g_cur_scope;
    while (sc != NULL) {
        if (is_struct_type_exist(SCOPE_struct_list(sc), tag, s)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


/*
Return true if the union typed declaration have already existed in both
current and all of outer scopes.
*/
bool is_union_exist_in_outer_scope(IN CHAR * tag, OUT Union ** s)
{
    SCOPE * sc = g_cur_scope;
    while (sc != NULL) {
        if (is_union_type_exist(SCOPE_union_list(sc), tag, s)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


/*
Return true if 'name' indicate an enum const which have already been
defined in both current and all of outer scopes.
'name': enum name to be checked.
'e': enum type set.
'idx': index in 'e' const list, start at 0.
*/
bool is_enum_const_exist_in_outer_scope(CHAR * name, OUT Enum ** e,
                                        OUT INT * idx)
{
    SCOPE * sc = g_cur_scope;
    while (sc != NULL) {
        if (is_enum_exist(SCOPE_enum_list(sc), name, e, idx)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


//Enum value only has effectivity at current scope.
bool is_enum_const_exist_in_cur_scope(IN CHAR * cl, OUT Enum ** e,
                                      OUT INT * idx)
{
    if (is_enum_exist(SCOPE_enum_list(g_cur_scope), cl, e, idx)) {
        return true;
    }
    return false;
}


static bool is_enum_const_name_exist(Enum * e, CHAR * ev_name, OUT INT * idx)
{
    if (e == NULL || ev_name == NULL) return false;
    EnumValueList * evl = ENUM_vallist(e);
    INT i = 0;
    while (evl != NULL) {
        if (strcmp(SYM_name(EVAL_LIST_name(evl)) , ev_name) == 0) {
            *idx = i;
            return true;
        }
        evl = EVAL_LIST_next(evl);
        i++;
    }
    return false;
}


//Return true if enum identifier existed.
static bool is_enum_id_exist(IN EnumList * e_list, IN CHAR * e_id_name,
                             OUT Enum ** e)
{
    if (e_list == NULL || e_id_name == NULL) return false;
    EnumList * el = e_list;
    while (el != NULL) {
        Enum * tmp = ENUM_LIST_enum(el);
        if (ENUM_name(tmp) == NULL) continue;
        if (strcmp(SYM_name(ENUM_name(tmp)), e_id_name) == 0) {
            *e = tmp;
            return true;
        }
        el = ENUM_LIST_next(el);
    }
    return false;
}


INT get_enum_val_idx(IN Enum * e, IN CHAR * ev_name)
{
    if (e == NULL || ev_name == NULL) return -1;
    EnumValueList * evl=ENUM_vallist(e);
    INT i = 0;
    while (evl != NULL) {
        if (strcmp(SYM_name(EVAL_LIST_name(evl)) , ev_name) == 0) {
            return i;
        }
        evl = EVAL_LIST_next(evl);
        i++;
    }
    return -1;
}


bool is_user_type_exist(IN UserTypeList * ut_list, IN CHAR * ut_name,
                        OUT Decl ** decl)
{
    if (ut_list == NULL || ut_name == NULL) return false;
    UserTypeList * utl = ut_list;
    while (utl != NULL) {
        Decl * dcl = USER_TYPE_LIST_utype(utl);
        if (strcmp(SYM_name(get_decl_sym(dcl)),ut_name) == 0) {
            *decl = dcl;
            return true;
        }
        utl = USER_TYPE_LIST_next(utl);
    }
    return false;

}


bool is_struct_type_exist(List<Struct*> & struct_list,
                          IN CHAR * tag, OUT Struct ** s)
{
    if (tag == NULL) { return false; }

    C<Struct*> * ct;
    for (Struct * st = struct_list.get_head(&ct);
         ct != NULL; st = struct_list.get_next(&ct)) {
        SYM * sym = STRUCT_tag(st);
        if (sym == NULL) { continue; }

        if (strcmp(SYM_name(sym), tag) == 0) {
            *s = st;
            return true;
        }
    }
    return false;
}


//Seach Union list accroding to the 'tag' of union-type.
bool is_union_type_exist(List<Union*> & u_list, IN CHAR * tag, OUT Union ** u)
{
    if (tag == NULL) { return false; }

    C<Union*> * ct;
    for (Union * st = u_list.get_head(&ct);
         st != NULL; st = u_list.get_next(&ct)) {
        SYM * sym = UNION_tag(st);
        if (sym == NULL) { continue; }

        if (strcmp(SYM_name(sym), tag) == 0) {
            *u = st;
            return true;
        }
    }
    return false;
}


Enum * new_enum()
{
    return (Enum*)xmalloc(sizeof(Enum));
}


TypeSpec * new_type()
{
    TypeSpec * ty = (TypeSpec*)xmalloc(sizeof(TypeSpec));
    ty->clean();
    return ty;
}


TypeSpec * new_type(INT cate)
{
    TypeSpec * ty = (TypeSpec*)xmalloc(sizeof(TypeSpec));
    ty->clean();
    TYPE_des(ty) = cate;
    return ty;
}


//'decl' presents DCL_DECLARATOR or DCL_ABS_DECLARATOR,
//Compute size of total array.
ULONGLONG compute_size_of_array(Decl * decl)
{
    if (DECL_dt(decl) == DCL_DECLARATOR) {
        decl = DECL_child(decl);
        if (DECL_dt(decl) != DCL_ID) {
            err(g_src_line_num, "declarator absent identifier");
            return 0;
        }
        decl = DECL_next(decl);
    } else if (DECL_dt(decl) == DCL_ABS_DECLARATOR) {
        decl = DECL_child(decl);
    }
    if (DECL_dt(decl) == DCL_ID) {
        decl = DECL_next(decl);
    }
    if (decl == NULL) {
        return 0;
    }

    ULONGLONG num = 0;
    INT    dim = 0;
    while (decl != NULL && DECL_dt(decl) == DCL_ARRAY) {
        if (DECL_array_dim(decl) == 0) {
            err(g_src_line_num, "size of %dth dimension can not be zero", dim);
            return 0;
        }
        if (num == 0) {
            //Meet the first dim
            num = DECL_array_dim(decl);
        } else {
            num *= DECL_array_dim(decl);
        }
        dim++;
        decl = DECL_next(decl);
    }
    ASSERT(get_const_bit_len(num) < 64, ("too large array"));
    return num;
}


INT compute_struct_type_size(TypeSpec * ty)
{
    ASSERT0(IS_STRUCT(ty));
    ASSERT0(is_struct_complete(ty));
    Struct * s = TYPE_struct_type(ty);
    Decl * dcl = STRUCT_decl_list(s);
    INT size = 0;
    while (dcl != NULL) {
        size += get_decl_size(dcl);
        dcl = DECL_next(dcl);
    }

    INT mod = size % STRUCT_align(s);
    if (mod != 0) {
        size = (size / STRUCT_align(s) + 1) * STRUCT_align(s);
    }
    return size;
}


INT compute_union_type_size(TypeSpec * ty)
{
    ASSERT0(IS_UNION(ty));
    ASSERT0(is_union_complete(ty));
    Union * s = TYPE_union_type(ty);
    Decl * dcl = UNION_decl_list(s);
    INT size = 0;
    while (dcl != NULL) {
        size = MAX(size, get_decl_size(dcl));
        dcl = DECL_next(dcl);
    }

    INT mod = size % UNION_align(s);
    if (mod != 0) {
        size = (size / UNION_align(s) + 1) * UNION_align(s);
    }
    return size;
}


/*
What is 'complex type'? Non-pointer and non-array type.
e.g : int * a;
      int a[];
*/
bool is_complex_type(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    if (dcl == NULL) { return false; }
    return (is_pointer(dcl) || is_array(dcl));
}


/*
 What is 'simply type'? Non-pointer and non-array type.
 e.g : int a;
       void a;
       struct a;
       union a;
       enum a;
       USER_DEFINED_TYPE_NAME a;
*/
INT get_simply_type_size_in_byte(TypeSpec * spec)
{
    if (spec == NULL) { return 0; }
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_VOID)) return BYTE_PER_INT;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_CHAR)) return BYTE_PER_CHAR;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_SHORT)) return BYTE_PER_SHORT;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_INT)) return BYTE_PER_INT;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_LONGLONG)) return BYTE_PER_LONGLONG;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_LONG)) return BYTE_PER_LONG;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_FLOAT)) return BYTE_PER_FLOAT;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_DOUBLE)) return BYTE_PER_DOUBLE;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_STRUCT)) {
        return compute_struct_type_size(spec);
    }
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_UNION)) {
        return compute_union_type_size(spec);
    }
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_ENUM)) return BYTE_PER_ENUM;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_SIGNED)) return BYTE_PER_INT;
    if (HAVE_FLAG(TYPE_des(spec), T_SPEC_UNSIGNED)) return BYTE_PER_INT;
    return 0;
}


/* Compute byte size to complex type.
complex type means the type is either pointer or array.
e.g : int * a;
      int a [];
*/
ULONG get_complex_type_size_in_byte(Decl * decl)
{
    if (decl == NULL) { return 0; }

    TypeSpec * spec = DECL_spec(decl);
    Decl * d = NULL;
    if (DECL_dt(decl) == DCL_DECLARATION ||
       DECL_dt(decl) == DCL_TYPE_NAME) {
        d = get_pure_declarator(decl);
        ASSERT(d != NULL, ("composing type expected decl-spec"));
    } else {
        err(g_src_line_num, "expected declaration or type-name");
        return 0;
    }

    ASSERT(spec != NULL, ("composing type expected specifier"));
    ULONG declor_size = get_declarator_size_in_byte(d);
    if (is_array(d)) {
        ULONG s = get_simply_type_size_in_byte(spec);
        return declor_size * s;
    }
    return declor_size;
}


//Compute the byte size of declaration.
//This function also do check in addition to compute array size.
INT get_decl_size(Decl * decl)
{
    TypeSpec * spec = DECL_spec(decl);
    Decl * d = NULL;
    if (DECL_dt(decl) == DCL_DECLARATION ||
        DECL_dt(decl) == DCL_TYPE_NAME) {
        d = DECL_decl_list(decl); //get declarator
        ASSERT(d &&
                (DECL_dt(d) == DCL_DECLARATOR ||
                 DECL_dt(d) == DCL_ABS_DECLARATOR),
                ("illegal declarator"));
        if (is_complex_type(d)) {
            return get_complex_type_size_in_byte(decl);
        } else {
            return get_simply_type_size_in_byte(spec);
        }
    } else {
        ASSERT(0, ("expected declaration"));
    }
    return 0;
}


/* Calculate byte size of pure decl-type list, but without the 'specifier'.
There only 2 type of decl-type: pointer and array.
    e.g  Given type is: int *(*p)[3][4], and calculating the
        size of '*(*) [3][4]'.
        The order of decl is: p->*->[3]->[4]->*
*/
INT get_declarator_size_in_byte(Decl * d)
{
    if (d == NULL) {
        return 0;
    }

    if (is_pointer(d)) {
        return BYTE_PER_POINTER;
    }

    if (is_array(d)) {
        INT e = (INT)compute_size_of_array(d);
        return e;
    }
    return 0;
}


/* Return the *first* Decl structure which indicate an array
in pure-list of declaration.

e.g: int p[10][20]; the declarator is: DCL_ID(p)->DCL_ARRAY(20)->DCL_ARRAY(10).
return DCL_ARRAY(20).
*/
Decl * get_array_decl(Decl * decl)
{
    ASSERT(DECL_dt(decl) == DCL_TYPE_NAME ||
            DECL_dt(decl) == DCL_DECLARATION ,
            ("expect DCRLARATION"));
    ASSERT(is_array(decl), ("expect pointer type"));
    Decl * x = get_pure_declarator(decl);
    while (x != NULL) {
        switch (DECL_dt(x)) {
        case DCL_FUN:
        case DCL_POINTER:
            return NULL;
        case DCL_ARRAY:
            return x;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(x) != DCL_DECLARATION &&
                    DECL_dt(x) != DCL_DECLARATOR &&
                    DECL_dt(x) != DCL_ABS_DECLARATOR &&
                    DECL_dt(x) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return NULL;
        }
        x = DECL_next(x);
    }
    return NULL;
}


/* Return the *first* Decl structure which indicate a pointer
in pure-list of declaration.

e.g: int * p; the declarator is: DCL_ID(p)->DCL_POINTER(*).
return DCL_POINTER.
*/
Decl * get_pointer_decl(Decl * decl)
{
    ASSERT(DECL_dt(decl) == DCL_TYPE_NAME ||
            DECL_dt(decl) == DCL_DECLARATION ,
            ("expect DCRLARATION"));
    ASSERT(is_pointer(decl), ("expect pointer type"));
    Decl * x = get_pure_declarator(decl);
    while (x != NULL) {
        switch (DECL_dt(x)) {
        case DCL_FUN:
            /*
            function-pointer type:
                DCL_ID->POINTER->DCL_FUN
            function-decl type:
                DCL_ID->DCL_FUN
            */
            if (DECL_prev(x) != NULL &&
                DECL_dt(DECL_prev(x)) == DCL_POINTER) {
                return DECL_prev(x);
            }
            return NULL;
        case DCL_POINTER:
            return x;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(x) != DCL_DECLARATION &&
                    DECL_dt(x) != DCL_DECLARATOR &&
                    DECL_dt(x) != DCL_ABS_DECLARATOR &&
                    DECL_dt(x) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return NULL;
        }
        x = DECL_next(x);
    }
    return NULL;
}


//Get base type of POINTER.
Decl * get_pointer_base_decl(Decl * decl, TypeSpec ** ty)
{
    ASSERT(DECL_dt(decl) == DCL_TYPE_NAME ||
            DECL_dt(decl) == DCL_DECLARATION ,
            ("expect DCRLARATION"));
    ASSERT(is_pointer(decl), ("expect pointer type"));
    *ty = DECL_spec(decl);
    Decl * d = get_pure_declarator(decl);
    if (DECL_dt(d) == DCL_ID) {
        d = DECL_next(d);
        ASSERT(DECL_dt(d) == DCL_POINTER ||
                DECL_dt(d) == DCL_FUN,
                ("expect pointer declarator"));
        return DECL_next(d); //get Decl that is the heel of '*'
    } else if (DECL_dt(d) == DCL_POINTER || DECL_dt(d) == DCL_FUN) {
        return DECL_next(d); //get Decl that is the heel of '*'
    }
    ASSERT(0, ("it is not a pointer type"));
    return NULL;
}


//Compute byte size of pointer base declarator.
//e.g: Given 'int *(*p)[3]', the pointer-base is 'int * [3]'.
UINT get_pointer_base_size(Decl * decl)
{
    TypeSpec * ty = NULL;
    Decl * d = get_pointer_base_decl(decl, &ty);
    if (d == NULL) {
        //base type of pointer oughts to be TypeSpec-SPEC.
        if (ty != NULL &&
            ((is_struct(ty) && !is_struct_complete(ty)) ||
             (is_union(ty) && !is_union_complete(ty)))) {
            //The struct/union is incomplete.
            return 0;
        }

        INT s = get_simply_type_size_in_byte(ty);
        ASSERT(s != 0, ("simply type size cannot be zero"));
        return s;
    }

    INT s = 1;
    INT e = get_declarator_size_in_byte(d);
    if (!is_pointer(d)) {
        s = get_simply_type_size_in_byte(ty);
    }
    ASSERT(e != 0, ("declarator size cannot be zero"));
    return e * s;
}


bool is_simple_base_type(TypeSpec * ty)
{
    if (ty == NULL) { return false; }
    return (TYPE_des(ty) & T_SPEC_VOID ||
            TYPE_des(ty) & T_SPEC_CHAR ||
            TYPE_des(ty) & T_SPEC_SHORT ||
            TYPE_des(ty) & T_SPEC_INT ||
            TYPE_des(ty) & T_SPEC_LONGLONG ||
            TYPE_des(ty) & T_SPEC_LONG ||
            TYPE_des(ty) & T_SPEC_FLOAT ||
            TYPE_des(ty) & T_SPEC_DOUBLE ||
            TYPE_des(ty) & T_SPEC_SIGNED ||
            TYPE_des(ty) & T_SPEC_UNSIGNED);
}


bool is_simple_base_type(INT des)
{
    return (des & T_SPEC_VOID ||
            des & T_SPEC_ENUM ||
            des & T_SPEC_CHAR ||
            des & T_SPEC_SHORT ||
            des & T_SPEC_INT ||
            des & T_SPEC_LONGLONG ||
            des & T_SPEC_LONG ||
            des & T_SPEC_FLOAT ||
            des & T_SPEC_DOUBLE ||
            des & T_SPEC_SIGNED ||
            des & T_SPEC_UNSIGNED);
}


INT format_enum_complete(IN OUT CHAR buf[], IN Enum * e)
{
    if (e == NULL) { return ST_SUCC; }
    CHAR * p = buf + strlen(buf);
    if (ENUM_name(e)) {
        sprintf(p, "%s ", SYM_name(ENUM_name(e)));
        p = p + strlen(p);
    }
    if (ENUM_vallist(e)) {
        strcat(p, "{");
        EnumValueList * ev= ENUM_vallist(e);
        while (ev != NULL) {
            p = p + strlen(p);
            sprintf(p, "%s ", SYM_name(EVAL_LIST_name(ev)));
            ev = EVAL_LIST_next(ev);
        }
        strcat(buf, "} ");
    }
    return ST_SUCC;
}


INT format_enum_complete(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL) { return ST_SUCC; }
    strcat(buf, "enum ");
    format_enum_complete(buf, TYPE_enum_type(ty));
    return ST_SUCC;
}


INT format_enum(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL)    { return ST_SUCC; }
    strcat(buf, "enum ");
    Enum * e = TYPE_enum_type(ty);
    if (e != NULL && ENUM_name(e) != NULL) {
        CHAR * p = buf + strlen(buf);
        sprintf(p, "%s ", SYM_name(ENUM_name(e)));
    }
    return ST_SUCC;
}


//Format union's name and members.
INT format_union_complete(IN OUT CHAR buf[], IN Union * u)
{
    if (u == NULL) { return ST_SUCC; }
    strcat(buf, "union ");
    Decl * member = UNION_decl_list(u);
    if (UNION_tag(u)) {
        strcat(buf, SYM_name(UNION_tag(u)));
    }
    //Printf members.
    strcat(buf, "{");
    while (member != NULL) {
        format_declaration(buf, member);
        strcat(buf,"; ");
        member = DECL_next(member);
    }
    strcat(buf, "}");
    return ST_SUCC;
}


//Format struct's name and members.
INT format_struct_complete(IN OUT CHAR buf[], IN Struct * s)
{
    if (s == NULL) { return ST_SUCC; }
    strcat(buf, "struct ");
    Decl * member = STRUCT_decl_list(s);
    if (STRUCT_tag(s)) {
        strcat(buf, SYM_name(STRUCT_tag(s)));
    }

    //Printf members.
    strcat(buf, "{");
    while (member != NULL) {
        ASSERT0(DECL_dt(member) == DCL_DECLARATION);
        if ((is_struct(member) || is_union(member)) &&
            TYPE_struct_type(DECL_spec(member)) == s) {
            /*
            It will be recursive definition of struction/union,
            e.g:
            struct S {
                struct S {
                    ...
                }
            }
            */
            ASSERT0(is_pointer(member));
        }
        format_declaration(buf, member);
        strcat(buf, "; ");
        member = DECL_next(member);
    }
    strcat(buf, "}");
    return ST_SUCC;
}


//Format struct/union's name and members.
INT format_struct_union_complete(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL) { return ST_SUCC; }
    format_struct_union(buf, ty);

    //Printf members.
    Struct * s = TYPE_struct_type(ty);
    if (s == NULL) { return ST_SUCC; }

    Decl * member = STRUCT_decl_list(s);
    strcat(buf, "{");
    while (member != NULL) {
        format_declaration(buf, member);
        strcat(buf, "; ");
        member = DECL_next(member);
    }
    strcat(buf, "}");
    return ST_SUCC;
}


//Format struct/union's name, it does not include members.
INT format_struct_union(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL)    { return ST_SUCC; }
    if (TYPE_des(ty) & T_SPEC_STRUCT) {
        strcat(buf, "struct ");
    } else if (TYPE_des(ty) & T_SPEC_UNION) {
        strcat(buf, "union ");
    } else {
        err(g_src_line_num, "expected a struct or union");
        return ST_ERR;
    }
    Struct * s = TYPE_struct_type(ty);

    //Illegal type, TYPE_struct_type can not be NULL,
    //one should filter this case before invoke format_struct_union();
    ASSERT0(s);

    //printf tag
    if (STRUCT_tag(s)) {
        CHAR * p = buf + strlen(buf);
        sprintf(p, "%s ", SYM_name(STRUCT_tag(s)));
    }
    return ST_SUCC;
}


INT format_stor_spec(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL)    { return ST_SUCC; }
    if (IS_REG(ty)) strcat(buf,"register ");
    if (IS_STATIC(ty)) strcat(buf,"static ");
    if (IS_EXTERN(ty)) strcat(buf,"extern ");
    if (IS_TYPEDEF(ty)) strcat(buf,"typedef ");
    return ST_SUCC;
}


INT format_quan_spec(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (ty == NULL)    { return ST_SUCC; }
    if (IS_CONST(ty)) strcat(buf,"const ");
    if (IS_VOLATILE(ty)) strcat(buf,"volatile ");
    return ST_SUCC;
}


INT format_decl_spec(IN OUT CHAR buf[], IN TypeSpec * ty, bool is_ptr)
{
    if (ty == NULL) { return ST_SUCC; }
    BYTE is_su = (BYTE)(IS_STRUCT(ty) || IS_UNION(ty)),
         is_enum = (BYTE)IS_ENUM_TYPE(ty) ,
         is_base = (is_simple_base_type(ty)) != 0 ,
         is_ut = (BYTE)IS_USER_TYPE_REF(ty);
    format_stor_spec(buf, ty);
    format_quan_spec(buf, ty);
    if (is_su) {
        if (is_ptr) {
            format_struct_union(buf, ty);
        } else {
            format_struct_union_complete(buf, ty);
        }
        return ST_SUCC;
    } else if (is_enum) {
        //format_enum(buf, ty);
        return format_enum_complete(buf, ty);
    } else if (is_base) {
        return format_base_type_spec(buf, ty);
    } else if (is_ut) {
        return format_user_type_spec(buf, ty);
    }
    return ST_ERR;
}


INT format_parameter_list(IN OUT CHAR buf[], IN Decl * decl)
{
    if (decl == NULL) { return ST_SUCC; }
    while (decl != NULL) {
        format_declaration(buf, decl);
        strcat(buf, ",");
        decl = DECL_next(decl);
    }
    return ST_SUCC;
}


INT format_dcrl_reverse(IN OUT CHAR buf[], IN Decl * decl)
{
    if (decl == NULL) {
        return ST_SUCC;
    }
    switch (DECL_dt(decl)) {
    case DCL_POINTER:
        {
            TypeSpec * quan = DECL_qua(decl);
            if (quan != NULL) {
                if (IS_CONST(quan)) strcat(buf,"const ");
                if (IS_VOLATILE(quan)) strcat(buf,"volatile ");
                if (IS_RESTRICT(quan)) strcat(buf,"restrict ");
            }
            strcat(buf,"* ");
            if (DECL_is_paren(decl)) {
                strcat(buf, "(");
                format_dcrl_reverse(buf, DECL_prev(decl));
                strcat(buf, ")");
            } else {
                format_dcrl_reverse(buf, DECL_prev(decl));
            }
        }
        break;
    case DCL_ID:
        {
            Tree * t = DECL_id(decl);
            TypeSpec * quan = DECL_qua(decl);
            if (quan != NULL) {
                ASSERT0(!IS_RESTRICT(quan));
                if (IS_VOLATILE(quan)) strcat(buf,"volatile ");
                if (IS_CONST(quan)) strcat(buf,"const ");
            }
            CHAR * p = buf + strlen(buf);
            sprintf(p, "%s ", SYM_name(TREE_id(t)));
            if (DECL_is_paren(decl)) {
                strcat(p, "(");
                p = p + strlen(p);
                format_dcrl_reverse(p, DECL_prev(decl));
                strcat(p, ")");
            } else {
                format_dcrl_reverse(p, DECL_prev(decl));
            }
        }
        break;
    case DCL_FUN:
        if (DECL_prev(decl) != NULL &&
            DECL_dt(DECL_prev(decl)) == DCL_POINTER) {
            //FUN_POINTER
            strcat(buf, "(");
            format_dcrl_reverse(buf,DECL_prev(decl));
            strcat(buf, ")");
        } else {
            //FUN_DECL
            format_dcrl_reverse(buf,DECL_prev(decl));
        }
        strcat(buf, "(");
        format_parameter_list(buf, DECL_fun_para_list(decl));
        strcat(buf, ")");
        break;
    case DCL_ARRAY:
        {
            if (DECL_is_paren(decl)) {
                strcat(buf, "(");
                format_dcrl_reverse(buf,DECL_prev(decl));
                strcat(buf, ")");
            } else {
                format_dcrl_reverse(buf,DECL_prev(decl));
            }
            //bound of each dimensions should be computed while
            //the DECLARATION parsed.
            LONGLONG v = DECL_array_dim(decl);
            CHAR * p = buf + strlen(buf);
            sprintf(p, "[%lld]", v);
            break;
        }
    default:
        ASSERT(0, ("unknown Decl type"));
    }
    return ST_SUCC;
}


INT format_declarator(IN OUT CHAR buf[], IN Decl * decl)
{
    CHAR b[10];
    b[0] = 0;
    if (decl == NULL) { return ST_SUCC; }
    if (DECL_dt(decl) == DCL_ABS_DECLARATOR||
        DECL_dt(decl) == DCL_DECLARATOR) {
        if (DECL_bit_len(decl)) {
            sprintf(b, ":%d", DECL_bit_len(decl));
        }
        decl = DECL_child(decl);
    }
    if (decl != NULL) {
        ASSERT((DECL_dt(decl) == DCL_ARRAY ||
            DECL_dt(decl) == DCL_POINTER ||
            DECL_dt(decl) == DCL_FUN     ||
            DECL_dt(decl) == DCL_ID      ||
            DECL_dt(decl) == DCL_VARIABLE),
            ("unknown declarator"));

        while (DECL_next(decl) != NULL)
            decl = DECL_next(decl);
        format_dcrl_reverse(buf, decl);
        strcat(buf, b);
    }
    return ST_SUCC;
    //return ST_ERR;
}


INT format_user_type_spec(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (buf == NULL || ty == NULL) return ST_SUCC;
    ASSERT0(HAVE_FLAG(TYPE_des(ty), T_SPEC_USER_TYPE));
    Decl * ut = TYPE_user_type(ty);
    ASSERT0(ut != NULL);
    return format_user_type_spec(buf, ut);
}


INT format_user_type_spec(IN OUT CHAR buf[], IN Decl * ut)
{
    if (buf == NULL || ut == NULL) return ST_SUCC;
    return format_declaration(buf, ut);
}


INT format_base_type_spec(IN OUT CHAR buf[], IN TypeSpec * ty)
{
    if (buf == NULL || ty == NULL) { return ST_SUCC; }
    if (!is_simple_base_type(ty)) {
        return ST_ERR;
    }
    if (TYPE_des(ty) & T_SPEC_SIGNED)
        strcat(buf, "signed ");
    if (TYPE_des(ty) & T_SPEC_UNSIGNED)
        strcat(buf, "unsigned ");
    if (TYPE_des(ty) & T_SPEC_CHAR)
        strcat(buf, "char ");
    if (TYPE_des(ty) & T_SPEC_SHORT)
        strcat(buf, "short ");
    if (TYPE_des(ty) & T_SPEC_LONG)
        strcat(buf, "long ");
    if (TYPE_des(ty) & T_SPEC_INT)
        strcat(buf, "int ");
    if (TYPE_des(ty) & T_SPEC_LONGLONG)
        strcat(buf, "longlong ");
    if (TYPE_des(ty) & T_SPEC_FLOAT)
        strcat(buf, "float ");
    if (TYPE_des(ty) & T_SPEC_DOUBLE)
        strcat(buf, "double ");
    if (TYPE_des(ty) & T_SPEC_VOID)
        strcat(buf, "void ");
    return ST_SUCC;

}


INT format_declaration(IN OUT CHAR buf[], IN Decl * decl)
{
    if (decl == NULL) { return ST_SUCC; }
    if (DECL_dt(decl) == DCL_DECLARATION ||
        DECL_dt(decl) == DCL_TYPE_NAME) {
        TypeSpec * ty = DECL_spec(decl);
        Decl * dcl = DECL_decl_list(decl);
        format_decl_spec(buf, ty, is_pointer(decl));
        format_declarator(buf, dcl);
        return ST_SUCC;
    } else if (DECL_dt(decl) == DCL_DECLARATOR ||
               DECL_dt(decl) == DCL_ABS_DECLARATOR) {
        Decl * dcl = DECL_decl_list(decl);
        strcat(buf, "NULL ");
        format_declarator(buf, dcl);
    } else if (DECL_dt(decl) == DCL_POINTER ||
               DECL_dt(decl) == DCL_ARRAY ||
               DECL_dt(decl) == DCL_FUN ||
               DECL_dt(decl) == DCL_ID) {
        strcat(buf, "NULL ");
        format_declarator(buf, decl);
    } else if (DECL_dt(decl) == DCL_VARIABLE) {
        strcat(buf, "...");
    } else {
        ASSERT(0, ("Unkonwn Decl type"));
    }
    return ST_ERR;
}


//Dump Decl Tree...
//Print indent blank.
static void pd(INT indent)
{
    while (indent-- > 0) { fprintf(g_tfile, " "); }
}


INT format_parameter_list(IN Decl * decl, INT indent)
{
    if (decl == NULL) { return ST_SUCC; }
    while (decl != NULL) {
        format_declaration(decl, indent);
        decl = DECL_next(decl);
        if (decl != NULL) fprintf(g_tfile, ", \n");
    }
    return ST_SUCC;
}


INT format_dcrl(IN Decl * decl, INT indent)
{
    if (decl == NULL) {
        return ST_SUCC;
    }
    switch (DECL_dt(decl)) {
    case DCL_POINTER:
        {
            TypeSpec * quan = DECL_qua(decl);
            if (quan != NULL) {
                if (IS_CONST(quan)) fprintf(g_tfile, "const ");
                if (IS_VOLATILE(quan)) fprintf(g_tfile, "volatile ");
                if (IS_RESTRICT(quan)) fprintf(g_tfile, "restrict ");
            }
            if (DECL_next(decl) != NULL) {
                if (DECL_dt(DECL_next(decl)) != DCL_FUN) {
                    fprintf(g_tfile, "POINTER");
                    fprintf(g_tfile, " -> ");
                }
                format_dcrl(DECL_next(decl), indent);
            } else {
                fprintf(g_tfile, "POINTER");
            }
        }
        break;
    case DCL_ID:
        {
            Tree * t = DECL_id(decl);
            TypeSpec * quan = DECL_qua(decl);
            if (quan != NULL) {
                ASSERT0(!IS_RESTRICT(quan));
                if (IS_CONST(quan)) fprintf(g_tfile, "const ");
                if (IS_VOLATILE(quan)) fprintf(g_tfile, "volatile ");
            }
            fprintf(g_tfile, "ID:'%s'", SYM_name(TREE_id(t)));
            if (DECL_next(decl) != NULL) { fprintf(g_tfile, " -> ");    }
            format_dcrl(DECL_next(decl), indent);
        }
        break;
    case DCL_FUN:
        ASSERT0(DECL_prev(decl) != NULL);
        if (DECL_dt(DECL_prev(decl)) == DCL_POINTER) {
            fprintf(g_tfile, "FUN_POINTER");
        } else if (DECL_dt(DECL_prev(decl)) == DCL_ID) {
            fprintf(g_tfile, "FUN_DECL");
        } else {
            ASSERT0(0);
        }

        if (DECL_fun_para_list(decl) == NULL) {
            fprintf(g_tfile, ",PARAM:()\n");
        } else {
            fprintf(g_tfile, ",PARAM:(\n");
            format_parameter_list(DECL_fun_para_list(decl),
                                  indent + DECL_FMT_INDENT_INTERVAL);
            pd(indent);
            fprintf(g_tfile, ")\n");
        }
        pd(indent);
        if (DECL_next(decl) != NULL) {
            fprintf(g_tfile, " RET_VAL_DCL_TYPE:");
        }
        format_dcrl(DECL_next(decl), indent);
        break;
    case DCL_ARRAY:
        {
            fprintf(g_tfile, "ARRAY");
            //bound of each dimensions should be computed while
            //the DECLARATION parsed.
            LONGLONG v = DECL_array_dim(decl);
            fprintf(g_tfile, "[%lld]", v);
            if (DECL_next(decl) != NULL) { fprintf(g_tfile, " -> ");    }
            if (DECL_is_paren(decl)) {
                //fprintf(g_tfile, "(");
                format_dcrl(DECL_next(decl), indent);
                //fprintf(g_tfile, ")");
            } else {
                format_dcrl(DECL_next(decl), indent);
            }
            break;
        }
    default:
        ASSERT(0, ("unknown Decl type"));
    }
    return ST_SUCC;
}


INT format_declarator(IN Decl * decl, INT indent)
{
    if (decl == NULL) { return ST_SUCC; }
    pd(indent);
    if (DECL_dt(decl) == DCL_ABS_DECLARATOR||
        DECL_dt(decl) == DCL_DECLARATOR) {
        fprintf(g_tfile, "%s", g_dcl_name[DECL_dt(decl)]);
        if (DECL_bit_len(decl)) {
            fprintf(g_tfile, ",bitfield:%d", DECL_bit_len(decl));
        }
        fprintf(g_tfile, "\n");
        decl = DECL_child(decl);
    }

    if (decl != NULL) {
        ASSERT((DECL_dt(decl) == DCL_ARRAY ||
            DECL_dt(decl) == DCL_POINTER ||
            DECL_dt(decl) == DCL_FUN     ||
            DECL_dt(decl) == DCL_ID      ||
            DECL_dt(decl) == DCL_VARIABLE),
            ("unknown declarator"));
        pd(indent + DECL_FMT_INDENT_INTERVAL);
        format_dcrl(decl, indent + DECL_FMT_INDENT_INTERVAL);
    }
    return ST_SUCC;
}


INT format_user_type_spec(IN TypeSpec * ty, INT indent)
{
    if (ty == NULL) {
        return ST_SUCC;
    }
    if ((TYPE_des(ty) & T_SPEC_USER_TYPE) == 0) {
        return ST_ERR;
    }
    Decl * ut = TYPE_user_type(ty);
    return format_user_type_spec(ut, indent);
}


INT format_user_type_spec(IN Decl * ut, INT indent)
{
    if (ut == NULL) {
        return ST_SUCC;
    }
    return format_declaration(ut, indent);
}


INT format_declaration(IN Decl * decl, INT indent)
{
    if (decl == NULL || g_tfile == NULL) return ST_SUCC;
    fprintf(g_tfile, "\n");
    pd(indent);
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    if (DECL_dt(decl) == DCL_DECLARATION || DECL_dt(decl) == DCL_TYPE_NAME) {
        TypeSpec * ty = DECL_spec(decl);
        Decl * dcl = DECL_decl_list(decl);
        fprintf(g_tfile, "%s", g_dcl_name[DECL_dt(decl)]);
        #ifdef _DEBUG_
        fprintf(g_tfile, "(uid:%d)", DECL_uid(decl));
        #endif
        fprintf(g_tfile, "(line:%d)", DECL_lineno(decl));
        fprintf(g_tfile, "\n");

        format_decl_spec(buf, ty, is_pointer(decl));
        pd(indent + DECL_FMT_INDENT_INTERVAL);
        fprintf(g_tfile, "SPECIFIER:%s", buf);

        fprintf(g_tfile, "\n");
        format_declarator(dcl, indent + DECL_FMT_INDENT_INTERVAL);
        fflush(g_tfile);
        return ST_SUCC;
    } else if (DECL_dt(decl) == DCL_DECLARATOR ||
               DECL_dt(decl) == DCL_ABS_DECLARATOR) {
        Decl * dcl = DECL_decl_list(decl);
        fprintf(g_tfile, "%s", g_dcl_name[DECL_dt(decl)]);
        fprintf(g_tfile, "\n");
        format_declarator(dcl, indent + DECL_FMT_INDENT_INTERVAL);
    } else if (DECL_dt(decl) == DCL_POINTER ||
             DECL_dt(decl) == DCL_ARRAY ||
             DECL_dt(decl) == DCL_FUN ||
             DECL_dt(decl) == DCL_ID) {
        fprintf(g_tfile, "%s ", g_dcl_name[DECL_dt(decl)]);
        format_declarator(decl, indent + DECL_FMT_INDENT_INTERVAL);
    } else if (DECL_dt(decl) == DCL_VARIABLE) {
        fprintf(g_tfile, "... ");
    } else {
        ASSERT(0, ("Unkonwn Decl type"));
    }
    fflush(g_tfile);
    return ST_ERR;
}
//END DECL_FMT


//Fetch const value of 't' refered
INT get_enum_const_val(Enum * e, INT idx)
{
    if (e == NULL) { return -1; }
    EnumValueList * evl = ENUM_vallist(e);
    while (idx > 0 && evl != NULL) {
        evl = EVAL_LIST_next(evl);
        idx--;
    }
    if (evl == NULL) {
        err(g_src_line_num, "enum const No.%d is not exist", idx);
        return -1;
    }
    return EVAL_LIST_val(evl);
}


//Fetch const value of 't' refered
CHAR * get_enum_const_name(Enum * e, INT idx)
{
    if (e == NULL) { return NULL; }
    EnumValueList * evl = ENUM_vallist(e);
    while (idx > 0 && evl != NULL) {
        evl = EVAL_LIST_next(evl);
        idx--;
    }
    if (evl == NULL) {
        err(g_src_line_num, "enum const No.%d is not exist", idx);
        return NULL;
    }
    return SYM_name(EVAL_LIST_name(evl));
}


//If type is a user-defined type, return the actually type-spec.
TypeSpec * get_pure_type_spec(TypeSpec * type)
{
    Decl * utdcl;
    if (IS_USER_TYPE_REF(type)) {
        utdcl = TYPE_user_type(type);
        return get_pure_type_spec(DECL_spec(utdcl));
    }
    return type;
}


bool is_bitfield(Decl * decl)
{
    decl = get_pure_declarator(decl);
    return decl != NULL && DECL_is_bit_field(decl);
}


bool is_struct(TypeSpec * type)
{
    type = get_pure_type_spec(type);
    return type != NULL && IS_STRUCT(type);
}


bool is_struct(Decl * decl)
{
    ASSERT(decl &&
            (DECL_dt(decl) == DCL_TYPE_NAME ||
             DECL_dt(decl) == DCL_DECLARATION),
            ("need TypeSpec-NAME or DCRLARATION"));
    return is_struct(DECL_spec(decl));
}


bool is_union(TypeSpec * type)
{
    type = get_pure_type_spec(type);
    return type != NULL && IS_UNION(type);
}


bool is_union(Decl * decl)
{
    ASSERT(decl &&
            (DECL_dt(decl) == DCL_TYPE_NAME ||
             DECL_dt(decl) == DCL_DECLARATION),
            ("need TypeSpec-NAME or DCRLARATION"));
    return is_union(DECL_spec(decl));
}


//Is float-point.
bool is_fp(Decl * dcl)
{
    ASSERT(dcl &&
            (DECL_dt(dcl) == DCL_TYPE_NAME ||
             DECL_dt(dcl) == DCL_DECLARATION),
            ("expect type-name or dcrlaration"));
    return is_fp(DECL_spec(dcl));
}


//Is float-point.
bool is_fp(TypeSpec * ty)
{
    return (IS_TYPE(ty, T_SPEC_FLOAT) || IS_TYPE(ty, T_SPEC_DOUBLE));
}


//Is integer
bool is_integer(Decl *dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_TYPE_NAME ||
            DECL_dt(dcl) == DCL_DECLARATION,
            ("expect type-name or dcrlaration"));
    return is_integer(DECL_spec(dcl));
}


//Is integer
bool is_integer(TypeSpec * ty)
{
    return (IS_TYPE(ty, T_SPEC_CHAR) ||
            IS_TYPE(ty, T_SPEC_SHORT)||
            IS_TYPE(ty, T_SPEC_INT)  ||
            IS_TYPE(ty, T_SPEC_LONG) ||
            IS_TYPE(ty, T_SPEC_LONGLONG) ||
            IS_TYPE(ty, T_SPEC_SIGNED) ||
            IS_TYPE(ty, T_SPEC_UNSIGNED) ||
            IS_TYPE(ty, T_SPEC_ENUM));
}


/*
Return boolean for arithmetic type.
Include integer and float pointer.
*/
bool is_arith(Decl *dcl)
{
    ASSERT(DECL_dt(dcl) == DCL_TYPE_NAME ||
            DECL_dt(dcl) == DCL_DECLARATION,
            ("expect type-name or dcrlaration"));
    TypeSpec * ty = DECL_spec(dcl);
    return is_integer(ty) || is_fp(ty);
}


//Return true if the return-value type is VOID.
bool is_fun_void_return(Decl * dcl)
{
    if (!is_fun_decl(dcl)) return false;
    if (HAVE_FLAG(TYPE_des(DECL_spec(dcl)), T_SPEC_VOID) &&
        !is_pointer(dcl)) {
        return true;
    }
    return false;
}


//Return true if 'dcl' is function-type declaration.
bool is_fun_decl(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            if (DECL_next(dcl) == NULL ||
                (DECL_prev(dcl) != NULL &&
                 DECL_dt(DECL_prev(dcl)) == DCL_ID)) {
                /*
                CASE:
                    ID->FUN is func-decl,
                    e.g: void f()

                    ID->FUN->... is func-decl,
                    e.g: void ( * f() ) [], ID->FUN->*->[]

                    ID->*->FUN->... is NOT func-decl, it is a func-pointer
                    e.g: void (* (* f)() ) [], ID->*->FUN->*->[]
                */
                return true;
            }
            return false;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(dcl) != DCL_DECLARATION &&
                    DECL_dt(dcl) != DCL_DECLARATOR &&
                    DECL_dt(dcl) != DCL_ABS_DECLARATOR &&
                    DECL_dt(dcl) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_fun()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


//Return true if 'dcl' is function pointer variable.
bool is_fun_pointer(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            /*
            CASE:
                ID->FUN is func-decl,
                e.g: void f()

                ID->FUN->... is func-decl
                e.g: void ( * f() ) [], ID->FUN->*->[]

                ID->*->FUN->... is func-pointer
                e.g: void (* (* f)() ) [], ID->*->FUN->*->[]
            */
            if (DECL_prev(dcl) != NULL &&
                DECL_dt(DECL_prev(dcl)) == DCL_POINTER) {
                return true;
            }
            return false;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(dcl) != DCL_DECLARATION &&
                    DECL_dt(dcl) != DCL_DECLARATOR &&
                    DECL_dt(dcl) != DCL_ABS_DECLARATOR &&
                    DECL_dt(dcl) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_fun()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


/*
Is 'dcl' a pointer-declarator,
e.g:Given Decl as : 'int * a', then the second decltor in the type-chain
    must be DCL_POINTER, the first is DCL_ID 'a'.
    And simplar for abs-decl, as an example 'int *', the first decltor
    in the type-chain must be DCL_POINTER.
*/
bool is_pointer(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL) {
        switch (DECL_dt(dcl)) {
        case DCL_FUN:
            /*
            function-pointer type:
                DCL_POINTER->DCL_FUN
            function-decl type:
                DCL_ID->DCL_FUN
            */
            if (DECL_prev(dcl) &&
                DECL_dt(DECL_prev(dcl)) == DCL_POINTER) {
                return true;
            }
            return false;
        case DCL_POINTER:
            return true;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(dcl) != DCL_DECLARATION &&
                    DECL_dt(dcl) != DCL_DECLARATOR &&
                    DECL_dt(dcl) != DCL_ABS_DECLARATOR &&
                    DECL_dt(dcl) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_pointer()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


/*
Is dcl is array declarator,
    for decl : int a[], the second decltor must be DCL_ARRAY,
    the first is DCL_ID.
    for abs-decl: int [], the first decltor must be DCL_ARRAY.
*/
bool is_array(Decl * dcl)
{
    dcl = get_pure_declarator(dcl);
    while (dcl != NULL) {
        switch (DECL_dt(dcl)) {
        case DCL_ARRAY:
            return true;
        case DCL_ID:
        case DCL_VARIABLE:
            break;
        default:
            ASSERT(DECL_dt(dcl) != DCL_DECLARATION &&
                    DECL_dt(dcl) != DCL_DECLARATOR &&
                    DECL_dt(dcl) != DCL_ABS_DECLARATOR &&
                    DECL_dt(dcl) != DCL_TYPE_NAME,
                    ("\nunsuitable Decl type locate here in is_array()\n"));
            return false;
        }
        dcl = DECL_next(dcl);
    }
    return false;
}


//Create a new type-name, and ONLY copy declaration info list and type-spec.
Decl * cp_type_name(Decl * src)
{
    ASSERT(DECL_dt(src) == DCL_TYPE_NAME, ("cp_type_name"));
    Decl * dest = new_decl(DCL_TYPE_NAME);
    DECL_decl_list(dest) = cp_decl(DECL_decl_list(src));
    PURE_DECL(dest) = NULL;
    DECL_spec(dest) = DECL_spec(src);

    Decl * p = PURE_DECL(src), * q;
    while (p != NULL) {
        q = cp_decl(p);
        add_next(&PURE_DECL(dest), q);
        p = DECL_next(p);
    }
    return dest;
}


//Get offset of appointed 'name' in struct 'st'
UINT get_struct_field_ofst(Struct * st, CHAR * name)
{
    Decl * decl = STRUCT_decl_list(st);
    UINT ofst = 0;
    UINT size = 0;
    while (decl != NULL) {
        SYM * sym = get_decl_sym(decl);
        if (strcmp(name, SYM_name(sym)) == 0) {
            return ofst;
        }
        size = get_decl_size(decl);
        ofst += (UINT)ceil_align(size, STRUCT_align(st));
        decl = DECL_next(decl);
    }
    ASSERT(0, ("Unknown struct field"));
    return 0;
}


void dump_struct(Struct * s)
{
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    ASSERT(ST_SUCC == format_struct_complete(buf, s),
            ("illegal struct type"));
    note("\n%s  %s\n", buf, STRUCT_is_complete(s) ? "intact" : "incomplete");
}


void dump_union(Union * s)
{
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    ASSERT(ST_SUCC == format_union_complete(buf, s), ("illegal union type"));
    note("\n%s %s\n", buf, UNION_is_complete(s) ? "intact" : "incomplete");
}


//Dump type specifier.
//ty: is TypeSpec specifier of declaration.
void dump_type(TypeSpec * ty, bool is_ptr)
{
    if (g_tfile == NULL) { return; }
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    format_decl_spec(buf, ty, is_ptr);
    fprintf(g_tfile, "\n%s\n", buf);
    fflush(g_tfile);
}


static void remove_redundant_para(Decl * declaration)
{
    ASSERT0(DECL_dt(declaration) == DCL_DECLARATION);
    Decl * dclor;
    Decl * para_list = get_parameter_list(declaration, &dclor);
    if (para_list != NULL) {
        TypeSpec * spec = DECL_spec(para_list);
        ASSERT0(spec != NULL);
        if (IS_TYPE(spec, T_SPEC_VOID)) {
            if (is_abs_declaraotr(para_list) && !is_pointer(para_list)) {
                //e.g int foo(void), there is no any parameter.
                DECL_fun_para_list(dclor) = NULL;
                return;
            }
            if (!is_abs_declaraotr(para_list) && !is_pointer(para_list)) {
                err(g_real_line_num, "the first parameter has incomplete type");
                return;
            }
        }
    }
}


//Check struct/union completeness if decl is struct/union.
static bool check_struct_union_complete(Decl * decl)
{
    TypeSpec * type_spec = DECL_spec(decl);
    if (is_struct(type_spec) || is_union(type_spec)) {
        SYM * sym = get_decl_sym(decl);
        CHAR const* name = SYM_name(sym);
        if (!is_pointer(decl)) {
            bool e = false; //claim error
            CHAR const* t = NULL;
            if (is_struct(type_spec) && !is_struct_complete(type_spec)) {
                e = true;
                t = "struct";
            } else if (is_union(type_spec) && !is_union_complete(type_spec)) {
                e = false;
                t = "union";
            }
            if (e) {
                //error occur!
                ASSERT0(t);
                CHAR buf[512];
                buf[0] = 0;
                if (name != NULL) {
                    format_struct_union_complete(buf,
                            get_pure_type_spec(type_spec));
                    err(g_real_line_num,
                        "'%s' uses incomplete defined %s : %s",
                        name, t, buf);
                    return false;
                } else {
                    err(g_real_line_num,
                        "uses incomplete definfed %s without name", t);
                    return false;
                }
            }
        }
    }
    return true;
}


static bool check_bitfield(Decl * decl)
{
    if (is_bitfield(decl) && is_pointer(decl)) {
        err(g_real_line_num, "pointer type can not assign bit length");
        return false;
    }
    return true;
}


static bool func_def(Decl * declaration)
{
    //Function definition only permit in global scope in C spec.
    if (SCOPE_level(g_cur_scope) != GLOBAL_SCOPE) {
        err(g_real_line_num, "miss ';' before '{' , function define should at global scope");
        return false;
    }

    //Check if 'decl' is unique at scope declaration list.
    Decl * dcl = SCOPE_decl_list(g_cur_scope);
    while (dcl != NULL) {
        if (is_decl_equal(dcl, declaration) && declaration != dcl
            && DECL_is_fun_def(dcl)) {
            err(g_real_line_num, "function '%s' already defined",
                SYM_name(get_decl_sym(dcl)));
            return false;
        }
        dcl = DECL_next(dcl);
    }

    //Add decl to scope here to support recursive func-call.
    add_next(&SCOPE_decl_list(g_cur_scope), declaration);

    //At function definition mode, identifier of each parameters cannot be NULL.
    if (is_abs_declaraotr(DECL_decl_list(declaration))) {
        err(g_real_line_num,
            "expected formal parameter list, not a type list");
        return false;
    }

    remove_redundant_para(declaration);
    Decl * para_list = get_parameter_list(declaration);
    DECL_fun_body(declaration) = compound_stmt(para_list);
    //dump_scope(DECL_fun_body(declaration), 0xfffFFFF);

    DECL_is_fun_def(declaration) = true;
    ASSERT(SCOPE_level(g_cur_scope) == GLOBAL_SCOPE,
            ("Funtion declaration should in global scope"));

    refine_func(declaration);
    if (ST_SUCC != label_ck(get_last_sub_scope(g_cur_scope))) {
        err(g_real_line_num, "illegal label used");
        return false;
    }

    //Check return value at typeck.cpp if
    //'DECL_fun_body(dcl)' is NULL
    return true;
}


static Decl * factor_user_type_rec(Decl * decl, TypeSpec ** new_spec)
{
    ASSERT0(DECL_dt(decl) == DCL_DECLARATION);

    TypeSpec * spec = DECL_spec(decl);
    Decl * new_declor = NULL;
    if (IS_USER_TYPE_REF(spec)) {
        new_declor = factor_user_type_rec(TYPE_user_type(spec), new_spec);
    } else {
        *new_spec = cp_spec(spec);
        REMOVE_FLAG(TYPE_des(*new_spec), T_STOR_TYPEDEF);
    }

    Decl * cur_declor = get_pure_declarator(decl);
    if (DECL_dt(cur_declor) == DCL_ID) {

        //neglect the first DCL_ID node, we only need the rest.
        cur_declor = DECL_next(cur_declor);
    }

    cur_declor = cp_decl_begin_at(cur_declor);
    insertbefore(&new_declor, new_declor, cur_declor);
    return new_declor;
}


//Expanding user-defined type, declared with 'typedef' in C
Decl * expand_user_type(Decl * ut)
{
    ASSERT0(is_user_type_ref(ut) || is_user_type_decl(ut));
    ASSERT0(DECL_dt(ut) == DCL_TYPE_NAME ||
             DECL_dt(ut) == DCL_DECLARATION);
    if (is_user_type_ref(ut)) {
        Decl * tmp = expand_user_type(TYPE_user_type(DECL_spec(ut)));
        ASSERT0(DECL_spec(tmp) != NULL);
        REMOVE_FLAG(TYPE_des(DECL_spec(tmp)), T_STOR_TYPEDEF);
        DECL_spec(ut) = DECL_spec(tmp);
        tmp = get_pure_declarator(tmp);
        if (DECL_dt(tmp) == DCL_ID) {
            tmp = DECL_next(tmp);
        }
        if (tmp != NULL) {
            add_next(&PURE_DECL(ut), tmp);
        }
        return ut;
    }

    Decl * tmp = cp_decl_complete(ut);
    ASSERT0(DECL_spec(tmp) != NULL);
    REMOVE_FLAG(TYPE_des(DECL_spec(tmp)), T_STOR_TYPEDEF);
    return tmp;
}


/* Factor the compound user type into basic type.
e.g: typedef int * INTP;
    'INTP a' will be factored to 'int * a'. */
Decl * factor_user_type(Decl * decl)
{
    ASSERT0(DECL_dt(decl) == DCL_DECLARATION ||
             DECL_dt(decl) == DCL_TYPE_NAME);
    TypeSpec * spec = DECL_spec(decl);
    ASSERT0(IS_USER_TYPE_REF(spec));

    //Indicate current specifier is typedef operation.
    bool is_typedef = IS_TYPEDEF(spec);

    //Create new type specifer according to the factored type information.
    TypeSpec * new_spec = NULL;
    Decl * new_declor = factor_user_type_rec(TYPE_user_type(spec), &new_spec);
    ASSERT0(new_spec);
    if (is_typedef) {
        SET_FLAG(TYPE_des(new_spec), T_STOR_TYPEDEF);
    }

    Decl * cur_declor = get_pure_declarator(decl);
    if (cur_declor == NULL) {
        //cur_declor may be abstract declarator list.
        //There is at least DCL_ID if the declaration is typedef.
        ASSERT0(!is_typedef);
        return new_declaration(new_spec, new_declor, g_cur_scope);
    }

    ASSERT(DECL_decl_list(decl), ("miss declarator"));
    ASSERT(DECL_dt(cur_declor) == DCL_ID ||
            DECL_dt(DECL_decl_list(decl)) == DCL_ABS_DECLARATOR,
        ("either decl is abstract declarator or miss typedef/variable name."));

    //Neglect the first DCL_ID node, we only need the rest.
    insertbefore(&new_declor, new_declor,
                 cp_decl_begin_at(DECL_next(cur_declor)));

    //Put an ID to be the head of declarator list.
    DECL_next(cur_declor) = NULL;
    ASSERT0(DECL_prev(cur_declor) == NULL);
    insertbefore_one(&new_declor, new_declor, cur_declor);

    //Create a new declaration with factored specifier.
    Decl * declaration = new_declaration(new_spec, new_declor, g_cur_scope);
    return declaration;
}


static void process_enum(TypeSpec * ty)
{
    if (!IS_ENUM_TYPE(ty) || TYPE_enum_type(ty) == NULL) { return; }

    EnumValueList * evals = ENUM_vallist(TYPE_enum_type(ty));
    if (evals == NULL) { return; }

    EnumList * elst = (EnumList*)xmalloc(sizeof(EnumList));
    ENUM_LIST_enum(elst) = TYPE_enum_type(ty);

    ASSERT0(!find_enum(SCOPE_enum_list(g_cur_scope), TYPE_enum_type(ty)));
    insertbefore_one(&SCOPE_enum_list(g_cur_scope),
                     SCOPE_enum_list(g_cur_scope), elst);
}


/*
declaration:
    declaration_spec init_declarator_list;
    declaration_spec ;
Return true if variable definition is found.
*/
bool declaration()
{
    bool is_last_fun_def = false;
    Decl * dcl_list = NULL;
    Decl * dcl = NULL;
    UINT lineno = g_real_line_num;
     TypeSpec * type_spec = declaration_spec();

    if (type_spec == NULL) { return false; }

    TypeSpec * qualifier = new_type();
    extract_qualifier(type_spec, qualifier);
    complement_qua(type_spec);

    process_enum(type_spec);

    bool def_var_or_perform_init = false;
    dcl_list = init_declarator_list(qualifier);
    if (dcl_list == NULL) {
        /*
        For enum type, there is no enum variable defined, such as:
            enum {X, Y, Z};
            enum _tag {X, Y, Z};
        */
        def_var_or_perform_init = false;
        goto FIN;
    }
    def_var_or_perform_init = true;

    DECL_align(dcl_list) = g_alignment;
    if (DECL_child(dcl_list) == NULL) {
        err(g_real_line_num, "declaration expected identifier");
        return def_var_or_perform_init;
    }

    while (dcl_list != NULL) {
        dcl = dcl_list;
        dcl_list = DECL_next(dcl_list);

        //Generate the DCL_DECLARATION accroding TypeSpec, DECLLARATOR.
        DECL_next(dcl) = DECL_prev(dcl) = NULL;

        Decl * declaration = new_decl(DCL_DECLARATION);
        DECL_spec(declaration) = type_spec;
        DECL_decl_list(declaration) = dcl;
        DECL_align(declaration) = g_alignment;
        DECL_decl_scope(declaration) = g_cur_scope;
        DECL_lineno(declaration) = lineno;

        if (IS_USER_TYPE_REF(type_spec)) {
            declaration = factor_user_type(declaration);
            DECL_align(declaration) = g_alignment;
            DECL_decl_scope(declaration) = g_cur_scope;
            DECL_lineno(declaration) = lineno;
        }

        //dump_decl(declaration, 0);

        if (is_fun_decl(declaration)) {
            if (g_real_token == T_LLPAREN) {
                if (!func_def(declaration)) {
                    goto FAILED;
                }
            } else if (g_real_token == T_SEMI) {
                //Function Declaration.
                add_next(&SCOPE_decl_list(g_cur_scope), declaration);
                DECL_is_fun_def(declaration) = 0;
            } else {
                err(g_real_line_num,
                    "illegal function definition/declaration, might be miss ';'");
                goto FAILED;
            }
        } else {
            //Common variable definition/declaration.
            //Check the declarator that should be unique at current scope.
            if (!is_unique_decl(SCOPE_decl_list(g_cur_scope), declaration)) {
                err(g_real_line_num, "'%s' already defined",
                     SYM_name(get_decl_sym(declaration)));
                goto FAILED;
            }
            add_next(&SCOPE_decl_list(g_cur_scope), declaration);
        }

        if (is_user_type_decl(declaration)) { //typedef declaration
            /* As the preivous parsing in 'declarator()' has recoginzed that
            current identifier is identical exactly in current scope,
            it is dispensable to warry about the redefinition, even if
            invoking is_user_type_exist(). */
            add_to_user_type_list(&SCOPE_user_type_list(g_cur_scope),
                                  declaration);
        }

        if (!check_struct_union_complete(declaration)) {
            goto FAILED;
        }

        if (!check_bitfield(declaration)) {
            goto FAILED;
        }

        if (DECL_is_init(DECL_decl_list(declaration))) {
            process_init(declaration);
        } else {
            //Check the size of array dimension.
            if (is_array(declaration)) {
                //This function also do check in addition to compute array size.
                get_decl_size(declaration);
            }
        }

        is_last_fun_def = DECL_is_fun_def(declaration);
    }
FIN:
    if (!is_last_fun_def) {
        if (g_real_token != T_SEMI) {
            err(g_real_line_num, "expected ';' after declaration");
        } else {
            match(T_SEMI);
        }
    }
    return def_var_or_perform_init;
FAILED:
    return def_var_or_perform_init;
}


bool declaration_list()
{
    bool find = false;
    while (is_in_first_set_of_declarator()) {
        find |= declaration();
    }
    return find;
}

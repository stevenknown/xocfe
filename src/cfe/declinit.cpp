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
#include "cfeinc.h"
#include "cfecommacro.h"

static void replaceBaseWith(Tree const* newbase, Tree * stmts);
static INT processAggrInit(Decl const* dcl, Tree * initval, OUT Tree ** stmts);
static INT processScope(Scope * scope, bool is_collect_stmt);

static INT processArrayInitRecur(Decl const* dcl, Tree * initval, UINT curdim,
                                 xcom::Vector<UINT> & dimvec,
                                 OUT Tree ** stmts)
{
    if (TREE_type(initval) == TR_INITVAL_SCOPE) {
        Decl const* dummy_elemdcl = get_array_elem_decl(dcl);
        if (is_aggr(dummy_elemdcl)) {
            Tree * inittree = nullptr;
            //If element of array is Aggregate type.
            processAggrInit(dummy_elemdcl, initval, &inittree);

            Tree * arr_ref = buildArray(dcl, dimvec);
            TREE_lineno(arr_ref) = TREE_lineno(initval);
            replaceBaseWith(arr_ref, inittree);
            xcom::add_next(stmts, inittree);
            return ST_SUCC;
        }

        if (is_array(dummy_elemdcl)) {
            curdim++;
            UINT pos_in_curdim = 0;
            for (Tree * t = TREE_initval_scope(initval);
                 t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
                dimvec.set(curdim, pos_in_curdim);
                processArrayInitRecur(dcl, t, curdim, dimvec, stmts);
            }
            return ST_SUCC;
        }

        UNREACHABLE();
        return ST_ERR;
    }

    Tree * lhs = buildArray(dcl, dimvec);
    TREE_lineno(lhs) = TREE_lineno(initval);
    Tree * assign = buildAssign(lhs, copyTree(initval));
    TREE_lineno(assign) = TREE_lineno(initval);
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


//dcl: the declaration of array.
//initval: the initial-value tree to array.
//stmts: records generated tree to perform initialization of array if it is
//       not NULL, otherwise append the genereted stmt after placeholder.
static INT processArrayInit(Decl const* dcl, Tree * initval, OUT Tree ** stmts)
{
    ASSERT0(initval && TREE_type(initval) == TR_INITVAL_SCOPE);

    //Record the position in each dimension of array.
    //e.g: given array[I][J][K], curdim begins at the left-first dimension I,
    //the position in dimension I begins at 0.
    xcom::Vector<UINT> dimvec;
    //TBD:Could 'dcl' be declared as zero dimension array? May be it is true
    //in dynamic-type language.
    for (INT i = get_array_dim(dcl) - 1; i >= 0; i--) {
        dimvec.set(i, 0);
    }
    UINT pos_in_curdim = 0;
    UINT curdim = 0;
    Tree * stmtlst = nullptr;
    for (Tree * t = TREE_initval_scope(initval);
         t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
        dimvec.set(curdim, pos_in_curdim);
        if (stmts != nullptr) {
            processArrayInitRecur(dcl, t, curdim, dimvec, stmts);
        } else {
            processArrayInitRecur(dcl, t, curdim, dimvec, &stmtlst);
        }
    }

    if (stmtlst != nullptr) {
        ASSERT0(DECL_placeholder(dcl));
        //Because placeholder will never be header of list, the insertion will
        //always insert a node after placeholder.
        Tree * pl = DECL_placeholder(dcl);
        xcom::insertafter(&pl, stmtlst);
    }
    return ST_SUCC;
}


static Tree * canonArrayInitVal(Decl const* dcl, Tree * initval)
{
    ASSERT0(is_array(dcl) && DECL_dt(dcl) == DCL_DECLARATION);
    if (TREE_type(initval) == TR_INITVAL_SCOPE) {
        return initval;
    }

    TypeSpec const* ty = get_decl_spec(dcl);
    if (IS_TYPE(ty, T_SPEC_CHAR)) {
        //In C lang, char array can be initialied by string.
        //e.g: char arr[] = "hello";
        ASSERT0(TREE_type(initval) == TR_STRING);
        char const* str = TREE_string_val(initval)->getStr();
        size_t len = ::strlen(str) + 1;
        Tree * last = nullptr;
        Tree * explst = nullptr;
        UINT lineno = TREE_lineno(initval);
        for (UINT i = 0; i < len; i++) {
            Tree * v = buildUInt(str[i]);
            TREE_lineno(v) = lineno;
            xcom::add_next(&explst, &last, v);
        }
        Tree * new_initval = buildInitvalScope(explst);
        TREE_lineno(new_initval) = lineno;
        return new_initval;
    }

    UNREACHABLE();
    return initval;
}


//Note the function does not check whether initval scope matchs the
//declaration. It should be diagnosticed before.
static INT processArrayInit(Decl const* dcl, OUT Tree ** stmts)
{
    ASSERT0(is_array(dcl));
    Tree * initval = get_decl_init_tree(dcl);
    Tree * new_initval = canonArrayInitVal(dcl, initval);
    if (new_initval != initval) {
        set_decl_init_tree(dcl, new_initval);
    }
    return processArrayInit(dcl, new_initval, stmts);
}


//stmts: a list of assignment.
static void replaceBaseWith(Tree const* newbase, Tree * stmts)
{
    ASSERT0(newbase);
    for (Tree * t = stmts; t != nullptr; t = TREE_nsib(t)) {
        ASSERT0(TREE_type(t) == TR_ASSIGN);
        Tree * lhs = TREE_lchild(t);
        ASSERT0(lhs);
        Tree * base = get_base(lhs);
        ASSERT0(base && TREE_type(base) == TR_ID);

        Tree * dup = copyTree(newbase);
        TREE_base_region(TREE_parent(base)) = dup;
        TREE_parent(dup) = TREE_parent(base);
    }
}


static INT processAggrInitRecur(Decl const* dcl, Decl const* flddecl,
                                Tree * initval, UINT curdim,
                                xcom::Vector<UINT> & fldvec,
                                OUT Tree ** stmts)
{
    if (TREE_type(initval) == TR_INITVAL_SCOPE) {
        if (is_aggr(flddecl)) {
            curdim++;
            UINT pos_in_curdim = 0;
            for (Tree * t = TREE_initval_scope(initval);
                 t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
                Decl * flddecl_of_fld = nullptr;
                get_aggr_field(get_aggr_spec(flddecl), (INT)pos_in_curdim,
                               &flddecl_of_fld);
                ASSERT0(flddecl_of_fld);
                fldvec.set(curdim, pos_in_curdim);
                processAggrInitRecur(dcl, flddecl_of_fld, t,
                                     curdim, fldvec, stmts);
            }
            return ST_SUCC;
        }

        if (is_array(flddecl)) {
            Tree * inittree = nullptr;
            if (ST_SUCC != processArrayInit(flddecl, initval, &inittree)) {
                return ST_ERR;
            }
            Tree * aggr_ref = buildAggrFieldRef(dcl, fldvec);
            TREE_lineno(aggr_ref) = TREE_lineno(initval);
            ASSERT0(is_aggr_field_access(aggr_ref));
            replaceBaseWith(aggr_ref, inittree);
            xcom::add_next(stmts, inittree);
            return ST_SUCC;
        }

        UNREACHABLE();
        return ST_ERR;
    }

    Tree * lhs = buildAggrFieldRef(dcl, fldvec);
    TREE_lineno(lhs) = TREE_lineno(initval);
    Tree * assign = buildAssign(lhs, copyTree(initval));
    TREE_lineno(assign) = TREE_lineno(initval);
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


//dcl: the declaration of aggregate.
//initval: the initial-value tree to aggregate.
//stmts: records generated tree to perform initialization of array if it is
//       not NULL, otherwise append the genereted stmt after placeholder.
static INT processAggrInit(Decl const* dcl, Tree * initval,
                             OUT Tree ** stmts)
{
    ASSERT0(initval && TREE_type(initval) == TR_INITVAL_SCOPE);

    //Record the position in each dimension of array.
    //e.g: given array[I][J][K], curdim begins at the left-first dimension I,
    //the position in dimension I begins at 0.
    xcom::Vector<UINT> fieldvec;

    UINT pos_in_curdim = 0;
    UINT curdim = 0;
    Tree * stmtlst = nullptr;
    for (Tree * t = TREE_initval_scope(initval);
         t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
        Decl * flddecl = nullptr;
        get_aggr_field(get_aggr_spec(dcl), pos_in_curdim, &flddecl);
        ASSERT0(flddecl);
        fieldvec.clean();
        fieldvec.set(curdim, pos_in_curdim);
        if (stmts != nullptr) {
            processAggrInitRecur(dcl, flddecl, t, curdim, fieldvec, stmts);
        } else {
            processAggrInitRecur(dcl, flddecl, t, curdim, fieldvec, &stmtlst);
        }
    }

    if (stmtlst != nullptr) {
        ASSERT0(DECL_placeholder(dcl));
        //Because placeholder will never be header of list, the insertion will
        //always insert a node after placeholder.
        Tree * pl = DECL_placeholder(dcl);
        xcom::insertafter(&pl, stmtlst);
    }
    return ST_SUCC;
}


static INT processAggrInit(Decl const* dcl, OUT Tree ** stmts)
{
    ASSERT0(is_struct(dcl) || is_union(dcl));
    Tree * initval = get_decl_init_tree(dcl);
    return processAggrInit(dcl, initval, stmts);
}


//stmts: records generated stmts if it is not NULL, otherwise append the
//genereted stmt after placeholder.
static INT processScalarInit(Decl const* dcl, OUT Tree ** stmts)
{
    Tree * initval = get_decl_init_tree(dcl);
    Tree * assign = buildAssign(dcl, initval);
    TREE_lineno(assign) = TREE_lineno(initval);
    if (stmts != nullptr) {
        xcom::add_next(stmts, assign);
    } else {
        ASSERT0(DECL_placeholder(dcl));
        //Because placeholder will never be header of list, the insertion will
        //always insert a node after placeholder.
        Tree * pl = DECL_placeholder(dcl);
        xcom::insertafter(&pl, assign);
    }
    return ST_SUCC;
}


static INT processDeclList(Decl const* decl, OUT Tree ** stmts)
{
    for (Decl const* dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        if (!is_initialized(dcl)) { continue; }
        if (is_pointer(dcl)) {
            if (ST_SUCC != processScalarInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (is_array(dcl)) {
            if (ST_SUCC != processArrayInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (is_struct(dcl) || is_union(dcl)) {
            if (ST_SUCC != processAggrInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (ST_SUCC != processScalarInit(dcl, stmts)) { return ST_ERR; }
    }
    return ST_SUCC;
}


//Process stmt list.
//is_collect_stmt: true if we are going to collect all generated stmts,
//                 otherwise these stmts will be append to placeholder.
static INT processStmt(Tree ** head, bool is_collect_stmt)
{
    Tree * t = *head;
    while (t != nullptr) {
        switch (TREE_type(t)) {
        case TR_SCOPE:
            ASSERT0(TREE_scope(t));
            if (ST_SUCC != processScope(TREE_scope(t), is_collect_stmt)) {
                return ST_ERR;
            }
            break;
        case TR_IF:
            if (ST_SUCC != processStmt(&TREE_if_true_stmt(t),
                                       is_collect_stmt)) {
                return ST_ERR;
            }
            if (ST_SUCC != processStmt(&TREE_if_false_stmt(t),
                                       is_collect_stmt)) {
                return ST_ERR;
            }
            break;
        case TR_DO:
            if (ST_SUCC != processStmt(&TREE_dowhile_body(t),
                                       is_collect_stmt)) {
                return ST_ERR;
            }
            break;
        case TR_WHILE:
            if (ST_SUCC != processStmt(&TREE_whiledo_body(t),
                                       is_collect_stmt)) {
                return ST_ERR;
            }
            break;
        case TR_FOR: {
            Tree * stmts = nullptr;
            if (TREE_for_scope(t) != nullptr) {
                Decl const* decllist = SCOPE_decl_list(TREE_for_scope(t));
                if (ST_SUCC != processDeclList(decllist, is_collect_stmt ?
                                                         &stmts : nullptr)) {
                    return ST_ERR;
                }
            }
            if (ST_SUCC != processStmt(&TREE_for_body(t), is_collect_stmt)) {
                return ST_ERR;
            }
            if (stmts != nullptr) {
                xcom::insertbefore(head, t, stmts);
            }
            break;
        }
        case TR_SWITCH:
            if (ST_SUCC != processStmt(&TREE_switch_body(t),
                                       is_collect_stmt)) {
                return ST_ERR;
            }
            break;
        default: ; //do nothing
        }
        t = TREE_nsib(t);
    }
    return ST_SUCC;
}


//Process local declaration's initialization.
//is_collect_stmt: true if we are going to collect all generated stmts,
//                 otherwise these stmts will be append to placeholder.
static INT processScope(Scope * scope, bool is_collect_stmt)
{
    Decl * decl_list = SCOPE_decl_list(scope);
    Tree * stmts = nullptr;
    if (ST_SUCC != processDeclList(decl_list, is_collect_stmt ?
                                              &stmts : nullptr)) {
        return ST_ERR;
    }

    //Process stmt list.
    if (ST_SUCC != processStmt(&SCOPE_stmt_list(scope), is_collect_stmt)) {
        return ST_ERR;
    }

    if (is_collect_stmt) {
        xcom::insertbefore(&SCOPE_stmt_list(scope), SCOPE_stmt_list(scope),
                           stmts);
    }
    return ST_SUCC;
}


static INT processFuncDef(Decl * dcl)
{
    ASSERT0(DECL_is_fun_def(dcl));

    //Set to true if we are going to collect all generated stmts.
    bool is_collect_stmt = false;
    if (ST_SUCC != processScope(DECL_fun_body(dcl), is_collect_stmt)) {
        return ST_ERR;
    }
    if (g_err_msg_list.get_elem_count() > 0) {
        return ST_ERR;
    }
    return ST_SUCC;
}


//Infer type to tree nodes.
INT processDeclInit()
{
    if (get_global_scope() == nullptr) { return ST_SUCC; }

    for (Decl * dcl = SCOPE_decl_list(get_global_scope());
         dcl != nullptr; dcl = DECL_next(dcl)) {
        if (DECL_is_fun_def(dcl)) {
            if (ST_SUCC != processFuncDef(dcl)) { return ST_ERR; }
            if (g_err_msg_list.get_elem_count() > 0) {
                return ST_ERR;
            }
        }
    }

    return ST_SUCC;
}

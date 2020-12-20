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

static INT processArrayInitRecur(Decl const* dcl, Tree * initval, UINT curdim,
                                 xcom::Vector<UINT> & dimvec,
                                 OUT Tree ** stmts)
{
    if (TREE_type(initval) == TR_INITVAL_SCOPE) {
        curdim++;
        UINT pos_in_curdim = 0;
        for (Tree * t = TREE_initval_scope(initval);
             t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
            dimvec.set(curdim, pos_in_curdim);
            processArrayInitRecur(dcl, t, curdim, dimvec, stmts);
        }
        return ST_SUCC;
    }

    Tree * lhs = buildArray(dcl, dimvec);
    Tree * assign = buildAssign(lhs, copyTree(initval));
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


//Note the function does not check whether initval scope matchs the
//declaration. It should be diagnosticed before.
static INT processArrayInit(Decl const* dcl, OUT Tree ** stmts)
{
    ASSERT0(is_array(dcl));
    Tree * initval = get_decl_init_tree(dcl);
    ASSERT0(TREE_type(initval) == TR_INITVAL_SCOPE);

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
    for (Tree * t = TREE_initval_scope(initval);
         t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
        dimvec.set(curdim, pos_in_curdim);
        processArrayInitRecur(dcl, t, curdim, dimvec, stmts);
    }
    return ST_SUCC;
}


static INT processStructInitRecur(Decl const* dcl, Tree * initval, UINT curdim,
                                  xcom::Vector<UINT> & fldvec,
                                  OUT Tree ** stmts)
{
    if (TREE_type(initval) == TR_INITVAL_SCOPE) {
        curdim++;
        UINT pos_in_curdim = 0;
        for (Tree * t = TREE_initval_scope(initval);
             t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
            fldvec.set(curdim, pos_in_curdim);
            processStructInitRecur(dcl, t, curdim, fldvec, stmts);
        }
        return ST_SUCC;
    }

    Tree * lhs = buildAggrFieldRef(dcl, fldvec);
    Tree * assign = buildAssign(lhs, copyTree(initval));
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


static INT processStructInit(Decl const* dcl, OUT Tree ** stmts)
{
    ASSERT0(is_struct(dcl));
    Tree * initval = get_decl_init_tree(dcl);
    ASSERT0(TREE_type(initval) == TR_INITVAL_SCOPE);

    //Record the position in each dimension of array.
    //e.g: given array[I][J][K], curdim begins at the left-first dimension I,
    //the position in dimension I begins at 0.
    xcom::Vector<UINT> fieldvec;
    //TBD:Could 'dcl' be declared as zero dimension array? May be it is true
    //in dynamic-type language.
    //for (INT i = get_array_dim(dcl) - 1; i >= 0; i--) {
    //    dimvec.set(i, 0);
    //}
    UINT pos_in_curdim = 0;
    UINT curdim = 0;
    for (Tree * t = TREE_initval_scope(initval);
         t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
        fieldvec.set(curdim, pos_in_curdim);
        processStructInitRecur(dcl, t, curdim, fieldvec, stmts);
    }
    return ST_SUCC;
}


static INT processUnionInit(Decl const* dcl, OUT Tree ** stmts)
{
    return ST_SUCC;
}


static INT processScalarInit(Decl const* dcl, OUT Tree ** stmts)
{
    Tree * initval = get_decl_init_tree(dcl);
    Tree * assign = buildAssign(dcl, initval);
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


static INT processDeclList(Decl const* decl, OUT Tree ** stmts)
{
    for (Decl const* dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        if (!is_initialized(dcl)) { continue; }
        if (is_array(dcl)) {
            if (ST_SUCC != processArrayInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (is_struct(dcl)) {
            if (ST_SUCC != processStructInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (is_union(dcl)) {
            if (ST_SUCC != processUnionInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (ST_SUCC != processScalarInit(dcl, stmts)) { return ST_ERR; }
    }
    return ST_SUCC;
}


//Process local declaration's initialization.
static INT processScope(Scope * scope)
{
    Decl * decl_list = SCOPE_decl_list(scope);
    Tree * stmts = nullptr;
    if (ST_SUCC != processDeclList(decl_list, &stmts)) {
        return ST_ERR;
    }
    xcom::insertbefore(&SCOPE_stmt_list(scope), SCOPE_stmt_list(scope), stmts);
    return ST_SUCC;
} 


//Process stmt list.
static INT processStmt(Tree * t)
{
    while (t != nullptr) {
        switch (TREE_type(t)) {
        case TR_SCOPE:
            ASSERT0(TREE_scope(t));
            if (ST_SUCC != processScope(TREE_scope(t))) {
                return ST_ERR;
            }
            break;
        case TR_IF:
            if (ST_SUCC != processStmt(TREE_if_true_stmt(t))) {
                return ST_ERR;
            }
            if (ST_SUCC != processStmt(TREE_if_false_stmt(t))) {
                return ST_ERR;
            }
            break;
        case TR_DO:
            if (ST_SUCC != processStmt(TREE_dowhile_body(t))) {
                return ST_ERR;
            }
            break;
        case TR_WHILE:
            if (ST_SUCC != processStmt(TREE_whiledo_body(t))) {
                return ST_ERR;
            }
            break;
        case TR_FOR: {
            Tree * stmts = nullptr;
            if (TREE_for_scope(t) != nullptr) {
                if (ST_SUCC != processDeclList(
                        SCOPE_decl_list(TREE_for_scope(t)), &stmts)) {
                    return ST_ERR;
                }
            }
            if (ST_SUCC != processStmt(TREE_for_body(t))) {
                return ST_ERR;
            }
            if (stmts != nullptr) {
                xcom::insertbefore(&TREE_for_body(t), TREE_for_body(t), stmts);
            }
            break;
        }
        case TR_SWITCH:
            if (ST_SUCC != processStmt(TREE_switch_body(t))) {
                return ST_ERR;
            }
            break;
        default: ; //do nothing
        }
        t = TREE_nsib(t);
    }
    return ST_SUCC;
}


static INT processFuncDef(Decl * dcl)
{
    ASSERT0(DECL_is_fun_def(dcl));
    if (ST_SUCC != processScope(DECL_fun_body(dcl))) {
        return ST_ERR;
    }

    //Process stmt list.
    if (ST_SUCC != processStmt(SCOPE_stmt_list(DECL_fun_body(dcl)))) {
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

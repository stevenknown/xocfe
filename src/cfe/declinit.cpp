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

static void replaceBaseWith(Tree const* newbase, Tree * stmts);
static INT processAggrInit(Decl const* dcl, Tree * initval, OUT Tree ** stmts);
static INT processScope(Scope * scope, bool is_collect_stmt);
static INT processFuncDef(Decl * dcl);
static INT processAggrInitRecur(
    Decl const* dcl, Decl * flddecl, Tree ** initval, UINT curdim,
    xcom::Vector<UINT> & fldvec, OUT Tree ** stmts);

static INT processArrayInitRecur(Decl const* dcl, Tree * initval, UINT curdim,
                                 xcom::Vector<UINT> & dimvec,
                                 OUT Tree ** stmts)
{
    if (initval->getCode() == TR_INITVAL_SCOPE) {
        Decl const* dummy_elemdcl = dcl->getArrayElemDecl();
        if (dummy_elemdcl->is_aggr()) {
            Tree * inittree = nullptr;
            //If element of array is Aggregate type.
            processAggrInit(dummy_elemdcl, initval, &inittree);

            Tree * arr_ref = buildArray(dcl, dimvec);
            TREE_lineno(arr_ref) = initval->getLineno();
            replaceBaseWith(arr_ref, inittree);
            xcom::add_next(stmts, inittree);
            return ST_SUCC;
        }

        if (dummy_elemdcl->is_array()) {
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
    TREE_lineno(lhs) = initval->getLineno();
    Tree * assign = buildAssign(lhs, copyTree(initval));
    TREE_lineno(assign) = initval->getLineno();
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


//dcl: the declaration of array, it may be modifed.
//initval: the initial-value tree to array.
//stmts: records generated tree to perform initialization of array if it is
//       not NULL, otherwise append the genereted stmt after placeholder.
static INT processArrayInit(Decl * dcl, Tree * initval, OUT Tree ** stmts)
{
    ASSERT0(initval && initval->getCode() == TR_INITVAL_SCOPE);

    //Record the position in each dimension of array.
    //e.g: given array[I][J][K], curdim begins at the left-first dimension I,
    //the position in dimension I begins at 0.
    xcom::Vector<UINT> dimvec;
    //TBD:Could 'dcl' be declared as zero dimension array? May be it is true
    //in dynamic-type language.
    for (INT i = dcl->getArrayDim() - 1; i >= 0; i--) {
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
    ASSERT0(dcl->is_array() && DECL_dt(dcl) == DCL_DECLARATION);
    if (initval->getCode() == TR_INITVAL_SCOPE) {
        return initval;
    }

    TypeAttr const* ty = dcl->getTypeAttr();
    if (ty->is_char()) {
        //In C lang, char array can be initialied by string.
        //e.g: char arr[] = "hello";
        ASSERT0(initval->getCode() == TR_STRING);
        char const* str = TREE_string_val(initval)->getStr();
        size_t len = ::strlen(str) + 1;
        Tree * last = nullptr;
        Tree * explst = nullptr;
        UINT lineno = initval->getLineno();
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
static INT processArrayInit(Decl * dcl, OUT Tree ** stmts)
{
    ASSERT0(dcl->is_array());
    Tree * initval = dcl->getDeclInitTree();
    Tree * new_initval = canonArrayInitVal(dcl, initval);
    if (new_initval != initval) {
        dcl->setDeclInitTree(new_initval);
    }
    return processArrayInit(dcl, new_initval, stmts);
}


//If assignment is accessing Aggregate, replaces base-region of LHS of
//to complete reference of struct field, newbase.
//If assignment is accessing Array, replaces array-base to complete
//reference of newbase.
//e.g:
//  typedef struct State {
//      char name[128];
//  } State;
//  State ms = {{2}};
//  replace:
//    ASSIGN(id : 28) : =
//      ARRAY(id : 25)
//        BASE :
//          name(id : 24) base
//        INDX :
//          IMM(id : 26) : 0
//      IMM(id : 27) : 2
//  to:
//    ASSIGN(id : 28)
//      ARRAY(id : 25)
//        BASE :
//          DMEM(id : 32)
//            ID(id : 33) : 'ms0'
//            name(id : 34) base
//        INDX :
//          IMM(id : 26) : 0
//      IMM(id : 27) : 2
//stmts: a list of assignment.
static void replaceBaseWith(Tree const* newbase, Tree * stmts)
{
    ASSERT0(newbase);
    for (Tree * t = stmts; t != nullptr; t = TREE_nsib(t)) {
        ASSERT0(t->getCode() == TR_ASSIGN);
        Tree * lhs = TREE_lchild(t);
        ASSERT0(lhs);
        Tree * base = get_base(lhs);
        ASSERT0(base && base->getCode() == TR_ID);

        Tree * dup = copyTree(newbase);
        TREE_base_region(TREE_parent(base)) = dup;
        TREE_parent(dup) = TREE_parent(base);
    }
}


static INT processAggrInitRecurByIterFieldDecl(
    Decl const* dcl, Decl * flddecl,
    Tree ** initval, UINT curdim, xcom::Vector<UINT> & fldvec,
    OUT Tree ** stmts)
{
    ASSERT0(initval && flddecl);
    ASSERT0((*initval)->getCode() != TR_INITVAL_SCOPE && flddecl->is_aggr());
    curdim++;

    //Because programmer may not enclose initval in angle brackets '{ }', we
    //have to iterate the Aggrate field-declaration-list to match the initval.
    //e.g:
    //  struct M {
    //    int a;
    //    struct N { int b,c; }
    //  };
    //  struct M m = {1,2,3};
    //  The more regular syntax is:
    //  struct M m = {1,{2,3}};
    Aggr const* fldaggr = flddecl->getAggrSpec();
    ASSERT0(fldaggr);
    UINT pos_in_curdim = 0;
    for (Decl * flddecl_of_fld = fldaggr->getDeclList();
         flddecl_of_fld != nullptr && (*initval) != nullptr;
         flddecl_of_fld = DECL_next(flddecl_of_fld),
         *initval = TREE_nsib(*initval),
         pos_in_curdim++) {
        fldvec.set(curdim, pos_in_curdim);
        INT st = processAggrInitRecur(
            dcl, flddecl_of_fld, initval, curdim, fldvec, stmts);
        if (st != ST_SUCC) {
            return st;
        }
        if ((*initval) == nullptr) {
            //The remain elements in initval list has been processed.
            break;
        }
        if (dcl->is_union()) {
            //The initial behaviour of UNION only stores the first initval to
            //the first field element in dcl's UNION declaration.
            break;
        }
    }
    return ST_SUCC;
}


//dcl: it may be modifed
static INT processAggrInitRecurByIterValue(
    Decl const* dcl, Decl * flddecl,
    Tree * initval, UINT curdim, xcom::Vector<UINT> & fldvec,
    OUT Tree ** stmts)
{
    ASSERT0(initval->getCode() == TR_INITVAL_SCOPE);
    if (flddecl->is_aggr()) {
        curdim++;
        UINT pos_in_curdim = 0;
        for (Tree * t = TREE_initval_scope(initval);
             t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
            Decl * flddecl_of_fld = nullptr;
            get_aggr_field(flddecl->getTypeAttr(), (INT)pos_in_curdim,
                           &flddecl_of_fld, nullptr);
            ASSERT0(flddecl_of_fld);
            fldvec.set(curdim, pos_in_curdim);
            processAggrInitRecur(
                dcl, flddecl_of_fld, &t, curdim, fldvec, stmts);
            if (t == nullptr) {
                //The remain elements in initval list has been processed.
                break;
            }
            if (dcl->is_union()) {
                //The initial behaviour of UNION only stores the first
                //initval to the first field element in dcl's UNION
                //declaration.
                break;
            }
        }
        return ST_SUCC;
    }
    if (flddecl->is_array()) {
        Tree * inittree = nullptr;
        if (ST_SUCC != processArrayInit(flddecl, initval, &inittree)) {
            return ST_ERR;
        }
        Tree * aggr_ref = buildAggrFieldRef(dcl, fldvec);
        TREE_lineno(aggr_ref) = initval->getLineno();
        ASSERT0(aggr_ref->is_aggr_field_access());
        replaceBaseWith(aggr_ref, inittree);
        xcom::add_next(stmts, inittree);
        return ST_SUCC;
    }
    UNREACHABLE();
    return ST_ERR;
}


//dcl: it may be modifed
static INT processAggrInitRecur(
    Decl const* dcl, Decl * flddecl, Tree ** initval, UINT curdim,
    xcom::Vector<UINT> & fldvec, OUT Tree ** stmts)
{
    if ((*initval)->getCode() == TR_INITVAL_SCOPE) {
        return processAggrInitRecurByIterValue(
            dcl, flddecl, *initval, curdim, fldvec, stmts);
    }
    if (flddecl->is_aggr()) {
        return processAggrInitRecurByIterFieldDecl(
            dcl, flddecl, initval, curdim, fldvec, stmts);
    }
    Tree * lhs = buildAggrFieldRef(dcl, fldvec);
    TREE_lineno(lhs) = (*initval)->getLineno();
    Tree * assign = buildAssign(lhs, copyTree(*initval));
    TREE_lineno(assign) = (*initval)->getLineno();
    xcom::add_next(stmts, assign);
    return ST_SUCC;
}


static INT processCopyConstruct(Decl const* dcl, Tree * initval,
                                OUT Tree ** stmts)
{
    Tree * assign = buildAssign(dcl, copyTree(initval));
    TREE_lineno(assign) = initval->getLineno();
    Tree * tstmtlst = nullptr;
    if (stmts != nullptr) {
        xcom::add_next(stmts, assign);
    } else {
        xcom::add_next(&tstmtlst, assign);
    }
    if (tstmtlst != nullptr) {
        ASSERT0(DECL_placeholder(dcl));
        //Because placeholder will never be header of list, the insertion will
        //always insert a node after placeholder.
        Tree * pl = DECL_placeholder(dcl);
        xcom::insertafter(&pl, tstmtlst);
    }
    return ST_SUCC;
}


//dcl: the declaration of aggregate.
//initval: the initial-value tree to aggregate.
//stmts: records generated tree to perform initialization of array if it is
//       not NULL, otherwise append the genereted stmt after placeholder.
static INT processAggrInit(Decl const* dcl, Tree * initval,
                           OUT Tree ** stmts)
{
    ASSERT0(initval);
    if (initval->getCode() != TR_INITVAL_SCOPE) {
        return processCopyConstruct(dcl, initval, stmts);
    }

    //Record the position in each dimension of array.
    //e.g: given array[I][J][K], curdim begins at the left-first dimension I,
    //the position in dimension I begins at 0.
    xcom::Vector<UINT> fieldvec;
    UINT pos_in_curdim = 0;
    UINT curdim = 0;
    Tree * stmtlst = nullptr;
    ASSERT0(dcl->is_aggr());
    for (Tree * t = TREE_initval_scope(initval);
         t != nullptr; t = TREE_nsib(t), pos_in_curdim++) {
        Decl * flddecl = nullptr;
        get_aggr_field(dcl->getTypeAttr(), pos_in_curdim, &flddecl, nullptr);
        ASSERT0(flddecl);
        fieldvec.clean();
        fieldvec.set(curdim, pos_in_curdim);
        if (stmts != nullptr) {
            processAggrInitRecur(dcl, flddecl, &t, curdim, fieldvec, stmts);
        } else {
            processAggrInitRecur(dcl, flddecl, &t, curdim, fieldvec, &stmtlst);
        }
        if (t == nullptr) {
            //The remain elements in initval list has been processed.
            break;
        }
        if (dcl->is_union()) {
            //The initial behaviour of UNION only stores the first initval to
            //the first field element in dcl's UNION declaration.
            break;
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
    ASSERT0(dcl->is_aggr());
    Tree * initval = dcl->getDeclInitTree();
    return processAggrInit(dcl, initval, stmts);
}


//stmts: records generated stmts if it is not NULL, otherwise append the
//genereted stmt after placeholder.
static INT processScalarInit(Decl const* dcl, OUT Tree ** stmts)
{
    Tree * initval = dcl->getDeclInitTree();
    Tree * assign = buildAssign(dcl, initval);
    TREE_lineno(assign) = initval->getLineno();
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


static INT processDeclList(Decl * decl, OUT Tree ** stmts)
{
    for (Decl * dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        if (dcl->is_fun_def()) {
            if (ST_SUCC != processFuncDef(dcl) ||
                g_err_msg_list.has_msg()) {
                return ST_ERR;
            }
            continue;
        }
        if (!dcl->is_initialized()) { continue; }
        if (dcl->is_pointer()) {
            if (ST_SUCC != processScalarInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (dcl->is_array()) {
            if (ST_SUCC != processArrayInit(dcl, stmts)) { return ST_ERR; }
            continue;
        }
        if (dcl->is_aggr()) {
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
        switch (t->getCode()) {
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
                Decl * decllist = TREE_for_scope(t)->getDeclList();
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


//Process declaration's initialization of given scope.
//is_collect_stmt: true if we are going to collect all generated stmts,
//                 otherwise these stmts will be append to placeholder.
static INT processScope(Scope * scope, bool is_collect_stmt)
{
    ASSERT0(scope);
    Decl * decl_list = scope->getDeclList();
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
    ASSERT0(dcl->is_fun_def());

    //Set to true if we are going to collect all generated stmts.
    bool is_collect_stmt = false;
    return processScope(dcl->getFunBody(), is_collect_stmt);
}


//Infer type to tree nodes.
INT processDeclInit()
{
    if (get_global_scope() == nullptr) { return ST_SUCC; }
    processScope(get_global_scope(), true);
    return ST_SUCC;
}

} //namespace xfe

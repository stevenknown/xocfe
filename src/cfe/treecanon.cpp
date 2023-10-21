/*@Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com

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
DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/
#include "cfeinc.h"
#include "cfecommacro.h"

namespace xfe {

bool TreeCanon::handleParam(Decl * formalp, Decl * realp)
{
    return true;
}


//Return true if there is no error occur during handling tree list.
Tree * TreeCanon::handleAssign(Tree * t, TreeCanonCtx * ctx)
{
    TREE_lchild(t) = handleTreeList(TREE_lchild(t), ctx);
    TREE_rchild(t) = handleTreeList(TREE_rchild(t), ctx);
    t->setParentForKid();
    return t;
}


Tree * TreeCanon::handleString(Tree * t, TreeCanonCtx * ctx)
{
    ASSERT0(t->getCode() == TR_STRING);
    if (t->parent()->getCode() == TR_ARRAY) {
        //e.g: ... = "abcd"[3];
        Decl * ty = t->getResultType();
        ASSERT0(ty);
        if (t == TREE_array_base(t->parent()) && ty->is_array()) {
            //In C language, string can be accessed via array operator.
            //The reference of the string should be represented as LDA.
            //e.g: ... = "abcd"[3];
            //     ... = ARRAY(STR(abcd)) will be transfomed to :
            //     ... = ARRAY(LDA(STR(abcd)))
            Tree * tparent = t->parent();
            Tree * newt = buildLda(t);

            //Do NOT call TypeTran() to compute the result-type of newt, because
            //TreeCanon will insert LDA prior to original tree node, which will
            //confuse the TypeTran() and generate the wrong result-type.
            TREE_result_type(newt) = convertToPointerTypeName(
                t->getResultType());

            Tree::setParent(tparent, newt);
            TCC_change(ctx) = true;
            return newt;
        }

        //t may be array subscript exp.
        return t;
    }
    return t;
}


Tree * TreeCanon::handleId(Tree * t, TreeCanonCtx * ctx)
{
    ASSERT0(t->getCode() == TR_ID);
    ASSERT0(TREE_id_decl(t));
    if (t->parent() != nullptr && //Enum ID does not have parent.
        t->parent()->getCode() == TR_ARRAY) {
        if (t == TREE_array_base(t->parent()) &&
            t->getResultType()->is_array()) {
            //In C language, identifier of array is just a label.
            //The reference of the label should be represented as LDA.
            //e.g: int * p; int a[10]; p = a[10];
            //     ARRAY(ID(a)) will be handled to : p = ARRAY(LDA(ID(a)))
            Tree * tparent = t->parent();
            Tree * newt = buildLda(t);

            //Do NOT call TypeTran() to compute the result-type of newt, because
            //TreeCanon will insert LDA prior to original tree node, which will
            //confuse the TypeTran() and generate the wrong result-type.
            TREE_result_type(newt) = convertToPointerTypeName(
                t->getResultType());

            Tree::setParent(tparent, newt);
            TCC_change(ctx) = true;
            return newt;
        }

        //t may be array subscript exp.
        return t;
    }

    if (t->getResultType()->is_array()) {
        //In C language, identifier of array is just a label.
        //The reference of the label should be represented as LDA.
        //e.g: int * p; int a[10]; p = ID(a);
        //     will be handled to : p = LDA(ID(a)
        Tree * tparent = t->parent();
        Tree * newt = buildLda(t);

        //Do NOT call TypeTran() to compute the result-type of newt, because
        //TreeCanon will insert LDA prior to original tree node, which will
        //confuse the TypeTran() and generate the wrong result-type.
        TREE_result_type(newt) = convertToPointerTypeName(
            t->getResultType());

        Tree::setParent(tparent, newt);
        TCC_change(ctx) = true;
        return newt;
    }

    return t;
}


Tree * TreeCanon::handleLda(Tree * t, TreeCanonCtx * ctx)
{
    ASSERT0(TREE_lchild(t)->getCode() != TR_LDA);
    TREE_lchild(t) = handleTreeList(TREE_lchild(t), ctx);
    t->setParentForKid();
    switch (TREE_lchild(t)->getCode()) {
    case TR_ID:
    case TR_DMEM:
    case TR_INDMEM:
    case TR_ARRAY:
    case TR_DEREF:
    case TR_STRING:
    case TR_INC:
    case TR_DEC:
        return t;
    case TR_LDA:
        //LDA has been generated when handling 'lchild'. Thus elide current LDA.
        return TREE_lchild(t);
    default: ASSERT0(0);
    }
    return t;
}


//Return original tree if there is no change, or new tree.
Tree * TreeCanon::handleCvt(Tree * t, TreeCanonCtx * ctx)
{
    TREE_cvt_exp(t) = handleTreeList(TREE_cvt_exp(t), ctx);
    t->setParentForKid();
    return t;
}


//Return true if there is no error occur during handling tree list.
Tree * TreeCanon::handleCall(Tree * t, TreeCanonCtx * ctx)
{
    TREE_para_list(t) = handleTreeList(TREE_para_list(t), ctx);
    TREE_fun_exp(t) = handleTreeList(TREE_fun_exp(t), ctx);
    t->setParentForKid();

    Decl * fun_decl = TREE_fun_exp(t)->getResultType();

    //Return type is the call type.
    //And here constructing return value type.
    //TypeAttr * ty = DECL_spec(fun_decl);
    Decl const* pure_decl = fun_decl->getTraitList();
    if (DECL_dt(pure_decl) == DCL_FUN) {
        pure_decl = DECL_next(pure_decl);
    }

    //Do legality handleing first for the return value type.
    if (pure_decl) {
        if (pure_decl->is_array()) {
            err(t->getLineno(), "function cannot returns array");
        }
        if (pure_decl->is_fun_decl()) {
            err(t->getLineno(), "function cannot returns function");
        }
    }

    //Check parameter list
    Decl * formal_param_decl = get_parameter_list(
        TREE_fun_exp(t)->getResultType());
    Tree * real_param = TREE_para_list(t);
    INT count = 0;
    if (formal_param_decl != nullptr) {
        while (formal_param_decl != nullptr && real_param != nullptr) {
            count++;
            Decl * pld = real_param->getResultType();
            if (!handleParam(formal_param_decl, pld)) {
                err(t->getLineno(), "%dth parameter type incompatible", count);
                return t;
            }

            formal_param_decl = DECL_next(formal_param_decl);
            real_param = TREE_nsib(real_param);

            if (formal_param_decl &&
                DECL_dt(formal_param_decl) == DCL_VARIADIC) {
                ASSERTN(!DECL_next(formal_param_decl),
                        ("DCL_VARIABLE must be last formal-parameter"));
                formal_param_decl = nullptr;
                real_param = nullptr;
            }
        }
    }

    if (formal_param_decl != nullptr || real_param != nullptr) {
        CHAR * name = nullptr;
        if (TREE_fun_exp(t)->getCode() == TR_ID) {
            name = SYM_name(TREE_id_name(TREE_fun_exp(t)));
        }

        Decl * p = get_parameter_list(TREE_fun_exp(t)->getResultType());
        UINT c = 0;
        while (p != nullptr) {
            c++;
            p = DECL_next(p);
        }

        if (count == 0) {
            err(t->getLineno(),
                "function '%s' cannot take any parameter",
                name != nullptr ? name : "");
        } else {
            err(t->getLineno(),
                "function '%s' should take %d parameters",
                name != nullptr ? name : "", c);
        }
    }
    return t;
}


Tree * TreeCanon::handleAggrAccess(Tree * t, TreeCanonCtx * ctx)
{
    ASSERT0(t->getCode() == TR_DMEM || t->getCode() == TR_INDMEM);
    TREE_base_region(t) = handleTreeList(TREE_base_region(t), ctx);
    Tree::setParent(t, TREE_base_region(t));

    Tree * field = TREE_field(t);
    ASSERT0(field->getResultType());
    if (field->getCode() == TR_ID && field->getResultType()->is_array()) {
        //Field is an array identifier.
        //In C language, array identifier is just a label.
        //The reference of the label should be represented as LDA.
        //e.g1: b = s.arr;
        //  will be transformed to: b = LDA(s.arr)
        //
        //e.g2: s.bg[3] = ...
        //  ARRAY
        //    |--3
        //    |--DMEM/INDMEM
        //         |--s
        //         |--bg

        //  ARRAY
        //    |--3
        //    |--LDA
        //         |--DMEM/INDMEM
        //              |--s
        //              |--bg
        Tree * tparent = t->parent();
        Tree * newt = buildLda(t);

        //Do NOT call TypeTran() to compute the result-type of newt, because
        //TreeCanon will insert LDA prior to original tree node, which will
        //confuse the TypeTran() and generate the wrong result-type.
        TREE_result_type(newt) = convertToPointerTypeName(
            t->getResultType());

        Tree::setParent(tparent, newt);
        TCC_change(ctx) = true;
        return newt;
    }

    TREE_field(t) = handleTreeList(TREE_field(t), ctx);
    Tree::setParent(t, TREE_field(t));
    return t;
}


Tree * TreeCanon::handleArray(Tree * t, TreeCanonCtx * ctx)
{
    TREE_array_base(t) = handleTreeList(TREE_array_base(t), ctx);
    TREE_array_indx(t) = handleTreeList(TREE_array_indx(t), ctx);
    t->setParentForKid();

    Tree * base = t->getArrayBase();
    ASSERT0(base);
    if (!base->getResultType()->regardAsPointer()) {
        //CASE: array parameter will be converted from 'a[]' to '(*a)[]'.
        //However type of '*a' is array. We have to convert the type
        //to pointer to suffice for the prerequisite of tree2ir.
        //Because tree2ir demands the base type of array must be pointer.
        ASSERT0(base->getCode() == TR_DEREF);
        Tree * parent = base->parent();
        ASSERT0(parent->getCode() == TR_ARRAY);
        Tree * lda = buildLda(base);
        TREE_array_base(parent) = lda;
        Tree::setParent(parent, lda);

        //Do NOT call TypeTran() to compute the result-type of newt, because
        //TreeCanon will insert LDA prior to original tree node, which will
        //confuse the TypeTran() and generate the wrong result-type.
        TREE_result_type(lda) = convertToPointerTypeName(
            base->getResultType());
    }
    return t;
}


//Return true if there is no error occur during handling tree list.
Tree * TreeCanon::handleTree(Tree * t, TreeCanonCtx * ctx)
{
    ASSERT0(ctx);
    switch (t->getCode()) {
    case TR_ASSIGN:
        return handleAssign(t, ctx);
    case TR_ID:
        return handleId(t, ctx);
    case TR_STRING:
        return handleString(t, ctx);
    case TR_IMM:
    case TR_IMML:
    case TR_IMMU:
    case TR_IMMUL:
    case TR_FP: // double
    case TR_FPF: // float
    case TR_FPLD: // long double
    case TR_ENUM_CONST:
    case TR_LOGIC_OR: // logical or ||
    case TR_LOGIC_AND: // logical and &&
    case TR_INCLUSIVE_OR: // inclusive or |
    case TR_XOR: // exclusive or
    case TR_INCLUSIVE_AND: // inclusive and &
    case TR_SHIFT: // >> <<
    case TR_EQUALITY: // == !=
    case TR_RELATION: // < > >= <=
    case TR_ADDITIVE: // '+' '-'
    case TR_MULTI: // '*' '/' '%'
        TREE_lchild(t) = handleTreeList(TREE_lchild(t), ctx);
        TREE_rchild(t) = handleTreeList(TREE_rchild(t), ctx);
        t->setParentForKid();
        return t;
    case TR_SCOPE:
        SCOPE_stmt_list(TREE_scope(t)) = handleTreeList(
            TREE_scope(t)->getStmtList(), ctx);
        return t;
    case TR_INITVAL_SCOPE:
        break;
    case TR_IF:
        TREE_if_det(t) = handleTreeList(TREE_if_det(t), ctx);
        TREE_if_true_stmt(t) = handleTreeList(TREE_if_true_stmt(t), ctx);
        TREE_if_false_stmt(t) = handleTreeList(TREE_if_false_stmt(t), ctx);
        t->setParentForKid();
        return t;
    case TR_DO:
        TREE_dowhile_body(t) = handleTreeList(TREE_dowhile_body(t), ctx);
        TREE_dowhile_det(t) = handleTreeList(TREE_dowhile_det(t), ctx);
        t->setParentForKid();
        return t;
    case TR_WHILE:
        TREE_whiledo_det(t) = handleTreeList(TREE_whiledo_det(t), ctx);
        TREE_whiledo_body(t) = handleTreeList(TREE_whiledo_body(t), ctx);
        t->setParentForKid();
        return t;
    case TR_FOR:
        TREE_for_init(t) = handleTreeList(TREE_for_init(t), ctx);
        TREE_for_det(t) = handleTreeList(TREE_for_det(t), ctx);
        TREE_for_step(t) = handleTreeList(TREE_for_step(t), ctx);
        TREE_for_body(t) = handleTreeList(TREE_for_body(t), ctx);
        t->setParentForKid();
        return t;
    case TR_SWITCH:
        TREE_switch_det(t) = handleTreeList(TREE_switch_det(t), ctx);
        TREE_switch_body(t) = handleTreeList(TREE_switch_body(t), ctx);
        t->setParentForKid();
        return t;
    case TR_BREAK:
    case TR_CONTINUE:
    case TR_GOTO:
    case TR_LABEL:
    case TR_DEFAULT:
    case TR_CASE:
        break;
    case TR_RETURN:
        TREE_ret_exp(t) = handleTreeList(TREE_ret_exp(t), ctx);
        t->setParentForKid();
        return t;
    case TR_COND:
        TREE_det(t) = handleTreeList(TREE_det(t), ctx);
        TREE_true_part(t) = handleTreeList(TREE_true_part(t), ctx);
        TREE_false_part(t) = handleTreeList(TREE_false_part(t), ctx);
        t->setParentForKid();
        return t;
    case TR_CVT:
        return handleCvt(t, ctx);
    case TR_TYPE_NAME: //user defined type or C standard type
        break;
    case TR_LDA: // &a get address of 'a'
        return handleLda(t, ctx);
    case TR_DEREF: // *p  dereferencing the pointer 'p'
    case TR_PLUS: // +123
    case TR_MINUS: // -123
    case TR_REV: // Reverse
    case TR_NOT: // get non-value
        TREE_lchild(t) = handleTreeList(TREE_lchild(t), ctx);
        t->setParentForKid();
        return t;
    case TR_INC: //++a
    case TR_POST_INC: //a++
        TREE_inc_exp(t) = handleTreeList(TREE_inc_exp(t), ctx);
        t->setParentForKid();
        return t;
    case TR_DEC: //--a
    case TR_POST_DEC: //a--
        TREE_dec_exp(t) = handleTreeList(TREE_dec_exp(t), ctx);
        t->setParentForKid();
        return t;
    case TR_SIZEOF: // sizeof(a)
        TREE_sizeof_exp(t) = handleTreeList(TREE_sizeof_exp(t), ctx);
        t->setParentForKid();
        return t;
    case TR_CALL:
        return handleCall(t, ctx);
    case TR_ARRAY:
        return handleArray(t, ctx);
    case TR_DMEM: // a.b
    case TR_INDMEM: // a->b
        return handleAggrAccess(t, ctx);
    case TR_PRAGMA:
    case TR_PREP:
    case TR_DECL:
        break;
    default: ASSERTN(0, ("unknown tree type:%d", t->getCode()));
    }
    return t;
}


//Return true if there is no error occur during handling tree list.
Tree * TreeCanon::handleTreeList(Tree * tl, TreeCanonCtx * ctx)
{
    Tree * next = nullptr;
    for (Tree * t = tl; t != nullptr; t = next) {
        next = TREE_nsib(t);
        TreeCanonCtx lctx;
        Tree * newt = handleTree(t, &lctx);
        if (newt != t) {
            xcom::replace(&tl, t, newt);
        }
        if (g_err_msg_list.has_msg()) {
            return nullptr;
        }
        ctx->unionInfoBottomUp(lctx);
    }
    return tl;
}


INT TreeCanonicalize()
{
    if (g_err_msg_list.has_msg()) {
        return ST_ERR;
    }

    Scope * s = get_global_scope();
    if (s == nullptr) { return ST_SUCC; }

    for (Decl * dcl = s->getDeclList(); dcl != nullptr; dcl = DECL_next(dcl)) {
        ASSERT0(dcl->getDeclScope() == s);
        if (!dcl->is_fun_def()) { continue; }

        TreeCanon tc;
        TreeCanonCtx ctx;
        SCOPE_stmt_list(DECL_fun_body(dcl)) = tc.handleTreeList(
            dcl->getFunBody()->getStmtList(), &ctx);
        if (g_err_msg_list.has_msg()) {
            return ST_ERR;
        }
    }

    TreeCanon tc;
    TreeCanonCtx ctx;
    SCOPE_stmt_list(s) = tc.handleTreeList(s->getStmtList(), &ctx);
    if (g_err_msg_list.has_msg()) {
        return ST_ERR;
    }
    return ST_SUCC;
}

} //namespace xfe

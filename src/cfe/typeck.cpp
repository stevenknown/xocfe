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

static bool checkTreeList(Tree * t, TYCtx * cont);

//Checking compatible between formal parameter and real parameter.
//'formalp': formal parameter
//'realp': real parameter
static bool checkParam(Decl * formalp, Decl * realp)
{
    DUMMYUSE(realp);
    DUMMYUSE(formalp);
    //TODO
    return true;
}


//Declaration checking
static INT checkDeclaration(Decl const* d)
{
    ASSERT0(DECL_dt(d) == DCL_DECLARATION);
    Decl const* dclor = d->getTraitList();
    ASSERT0(dclor);
    while (dclor != nullptr) {
        if (dclor->is_dt_fun()) {
            Decl * ret_value_type = DECL_next(dclor);
            if (ret_value_type) {
                if (ret_value_type->is_dt_fun()) {
                    err(g_real_line_num,
                        "return value type of function can not be a function");
                    return ST_ERR;
                }
                if (ret_value_type->is_dt_array()) {
                    err(g_real_line_num,
                        "return value type of function can not be an array");
                    return ST_ERR;
                }
            }
        }
        dclor = DECL_next(dclor);
    }
    return ST_SUCC;
}


static bool checkCallParamList(Tree * t, TYCtx * cont)
{
    ASSERT0(t->getCode() == TR_CALL);
    //Check parameter list
    Decl * formal_param_decl = get_parameter_list(
        TREE_fun_exp(t)->getResultType());
    Tree * real_param = TREE_para_list(t);
    INT count = 0;
    if (formal_param_decl != nullptr) {
        while (formal_param_decl != nullptr && real_param != nullptr) {
            count++;
            Decl * pld = real_param->getResultType();
            if (!checkParam(formal_param_decl, pld)) {
                err(t->getLineno(), "%dth parameter type incompatible", count);
                return false;
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

        return false;
    }
    return true;
}


static bool checkCall(Tree * t, TYCtx * cont)
{
    if (!checkTreeList(TREE_para_list(t), cont)) {
        return false;
    }
    if (!checkTreeList(TREE_fun_exp(t), cont)) {
        return false;
    }
    Tree * callee = TREE_fun_exp(t);
    Decl * fun_decl = callee->getResultType();
    ASSERTN(fun_decl, ("can not infer out return-type of call"));
    if (callee->getCode() == TR_ID) {
        //t is direct function call with symbol function name.
        if (!fun_decl->is_fun_decl() && !fun_decl->is_fun_def() &&
            !fun_decl->is_fun_pointer()) {
            err(callee->getLineno(), "'%s' is not a function",
                TREE_id_name(callee)->getStr());
            return false;
        }
    } else {
        //t is indirect function call through computable expression.
        if (!fun_decl->is_fun_pointer()) {
            xcom::StrBuf buf(64);
            format_declaration(buf, fun_decl, true);
            err(callee->getLineno(), "callee '%s' is not a function pointer",
                TOKEN_INFO_name(get_token_info(TREE_token(callee))));
            return false;
        }
    }

    //Return type is the call type.
    //And here constructing return value type.
    //TypeAttr * ty = DECL_spec(fun_decl);
    Decl const* pure_decl = fun_decl->getTraitList();
    if (pure_decl != nullptr && pure_decl->is_dt_fun()) {
        pure_decl = DECL_next(pure_decl);
    }

    //Do legality checking for the return-value type.
    if (pure_decl != nullptr) {
        if (pure_decl->is_array()) {
            err(t->getLineno(), "function cannot returns array");
        }
        if (pure_decl->is_fun_decl()) {
            err(t->getLineno(), "function cannot returns function");
        }
    }

    //Do legality checking for parameter list type matching.
    if (!checkCallParamList(t, cont)) {
        return false;
    }
    return true;
}


//True if decl can be convert to pointer without cast.
bool isConsistentWithPointer(Tree * t)
{
    ASSERT0(t);
    Decl * decl = t->getResultType();
    ASSERT0(decl);

    switch (t->getCode()) {
    case TR_IMM:
    case TR_IMMU:
    case TR_IMML:
    case TR_IMMUL:
        return true;
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
        return false;
    default: break;
    }
    if (decl->is_scalar()) {
        return false;
    }

    return true;
}


//Check lhs of assignment.
static void checkAssignLHS(Tree * t)
{
    ASSERT0(t);
    switch (TREE_lchild(t)->getCode()) {
    case TR_ID:
        ASSERT0(t->getResultType());
        if (t->getResultType()->is_fun_decl() ||
            t->getResultType()->is_fun_def()) {
            ASSERT0(TREE_id_name(TREE_lchild(t)));
            err(t->getLineno(),
                "function '%s': can not be left-value",
                TREE_id_name(TREE_lchild(t))->getStr());
        }
        break;
    case TR_DEREF:
    case TR_DMEM:
    case TR_INDMEM:
    case TR_ARRAY:
        break;
    default:
        err(t->getLineno(),
            "'%s': the left operand must be left-value",
            TOKEN_INFO_name(get_token_info(TREE_token(t))));
    }
}


static bool checkAssign(Tree * t, TYCtx * cont)
{
    checkTreeList(TREE_lchild(t), cont);
    checkTreeList(TREE_rchild(t), cont);
    if ((TREE_lchild(t)->getResultType()->is_pointer() &&
         !isConsistentWithPointer(TREE_rchild(t))) ||
        (TREE_rchild(t)->getResultType()->is_pointer() &&
         !isConsistentWithPointer(TREE_lchild(t)))) {
        xcom::StrBuf bufl(64);
        xcom::StrBuf bufr(64);
        format_declaration(bufl, TREE_lchild(t)->getResultType(), true);
        format_declaration(bufr, TREE_rchild(t)->getResultType(), true);
        warn(t->getLineno(),
             "should not assign '%s' to '%s'", bufr.buf, bufl.buf);
    }
    //Check LHS of assignment.
    checkAssignLHS(t);
    return true;
}


static bool checkLda(Tree * t, TYCtx * cont)
{
    bool res = checkTreeList(TREE_lchild(t), cont);
    switch (TREE_lchild(t)->getCode()) {
    case TR_ID:
    case TR_ARRAY:
    case TR_DEREF:
    case TR_DMEM:
    case TR_INDMEM:
    case TR_STRING:
    case TR_INC:
    case TR_DEC:
        return true;
    default:
        err(t->getLineno(), "'&' needs l-value");
        return false;
    }
    return res;
}


static bool checkReturn(Tree * t, TYCtx * cont)
{
    bool res = checkTreeList(TREE_ret_exp(t), cont);
    Decl const* funcdecl = cont->current_func_declaration;
    ASSERT0(funcdecl);
    if (funcdecl->is_fun_return_void() && TREE_ret_exp(t) != nullptr) {
        //The current function does not have a return
        //value accroding to its declaration. But in C language, this is
        //NOT an error, just a warning. Thus we still give a return result.
        //e.g: void get_bar(void) { return 10; }
        warn(t->getLineno(),
             "'return' with a value, in function returning void.");
    }
    return res;
}


static bool checkCvt(Tree * t, TYCtx * cont)
{
    bool res = checkTreeList(TREE_cvt_exp(t), cont);
    Decl const* srcty = TREE_cvt_exp(t)->getResultType();
    Decl const* tgtty = t->getResultType();
    ASSERT0(srcty && tgtty);
    bool src_is_arr = srcty->is_array();
    bool tgt_is_arr = tgtty->is_array();
    bool src_is_aggr = srcty->is_aggr();
    bool tgt_is_aggr = tgtty->is_aggr();
    bool src_is_sc = srcty->is_scalar();
    bool tgt_is_sc = tgtty->is_scalar();
    bool src_is_pt = srcty->isPointer();
    bool tgt_is_pt = tgtty->isPointer();
    bool src_is_fp = srcty->is_fp();
    bool tgt_is_fp = tgtty->is_fp();
    xcom::StrBuf bufsrc(64);
    xcom::StrBuf buftgt(64);

    if ((src_is_arr && tgt_is_aggr) ||
        (src_is_aggr && tgt_is_arr) ||
        (src_is_arr && tgt_is_arr)) {
        format_declaration(bufsrc, srcty, true);
        format_declaration(buftgt, tgtty, true);
        err(t->getLineno(),
            "can not convert '%s' to '%s'", bufsrc.buf, buftgt.buf);
        return false;
    }

    if (((src_is_arr || src_is_aggr) && tgt_is_sc) ||
        ((tgt_is_arr || tgt_is_aggr) && src_is_sc)) {
        format_declaration(bufsrc, srcty, true);
        format_declaration(buftgt, tgtty, true);
        err(t->getLineno(),
            "can not convert '%s' to '%s'", bufsrc.buf, buftgt.buf);
        return false;
    }

    if ((src_is_pt && tgt_is_fp) || (tgt_is_pt && src_is_fp)) {
        format_declaration(bufsrc, srcty, true);
        format_declaration(buftgt, tgtty, true);
        err(t->getLineno(),
            "can not convert '%s' to '%s'", bufsrc.buf, buftgt.buf);
        return false;
    }
    return res;
}


static void checkDeclInit(Decl const* decl, TYCtx * cont)
{
    for (Decl const* dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        if (!dcl->is_initialized()) { continue; }
        Tree * inittree = dcl->get_decl_init_tree();
        ASSERT0(inittree);
        checkTreeList(inittree, cont);
    }
}


static void checkInitValScope(Tree const* t, TYCtx * cont)
{
    //TBD: Does it necessary to check the Decl Type of node in init-value.
    //checkTreeList(TREE_initval_scope(t), cont);
}


//Perform type checking.
bool checkTreeList(Tree * t, TYCtx * cont)
{
    TYCtx ct;
    if (cont == nullptr) {
        cont = &ct;
    }
    for (; t != nullptr; t = TREE_nsib(t)) {
        g_src_line_num = t->getLineno();
        switch (t->getCode()) {
        case TR_ASSIGN:
            checkAssign(t, cont);
            break;
        case TR_ID:
        case TR_IMM:
        case TR_IMML:
        case TR_IMMU:
        case TR_IMMUL:
        case TR_FP: // double
        case TR_FPF: // float
        case TR_FPLD: // long double
        case TR_ENUM_CONST:
        case TR_STRING:
            break;
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
            checkTreeList(TREE_lchild(t), cont);
            checkTreeList(TREE_rchild(t), cont);
            break;
        case TR_SCOPE:
            checkDeclInit(TREE_scope(t)->getDeclList(), cont);
            checkTreeList(TREE_scope(t)->getStmtList(), cont);
            break;
        case TR_INITVAL_SCOPE:
            checkInitValScope(t, cont);
            break;
        case TR_IF:
            checkTreeList(TREE_if_det(t), cont);
            checkTreeList(TREE_if_true_stmt(t), cont);
            checkTreeList(TREE_if_false_stmt(t), cont);
            break;
        case TR_DO:
            checkTreeList(TREE_dowhile_body(t), cont);
            checkTreeList(TREE_dowhile_det(t), cont);
            break;
        case TR_WHILE:
            checkTreeList(TREE_whiledo_det(t), cont);
            checkTreeList(TREE_whiledo_body(t), cont);
            break;
        case TR_FOR:
            checkTreeList(TREE_for_init(t), cont);
            checkTreeList(TREE_for_det(t), cont);
            checkTreeList(TREE_for_step(t), cont);
            checkTreeList(TREE_for_body(t), cont);
            break;
        case TR_SWITCH:
            checkTreeList(TREE_switch_det(t), cont);
            checkTreeList(TREE_switch_body(t), cont);
            break;
        case TR_BREAK:
        case TR_CONTINUE:
        case TR_GOTO:
        case TR_LABEL:
        case TR_DEFAULT:
        case TR_CASE:
            break;
        case TR_RETURN:
            if (!checkReturn(t, cont)) { return false; }
            break;
        case TR_COND:
            checkTreeList(TREE_det(t), cont);
            checkTreeList(TREE_true_part(t), cont);
            checkTreeList(TREE_false_part(t), cont);
            break;
        case TR_CVT:
            checkCvt(t, cont);
            break;
        case TR_TYPE_NAME: //user defined type or C standard type
            break;
        case TR_LDA: // &a get address of 'a'
            checkLda(t, cont);
            break;
        case TR_DEREF: // *p  dereferencing the pointer 'p'
        case TR_PLUS: // +123
        case TR_MINUS: // -123
        case TR_REV: // Reverse
        case TR_NOT: // get non-value
            checkTreeList(TREE_lchild(t), cont);
            break;
        case TR_INC: //++a
        case TR_POST_INC: //a++
            checkTreeList(TREE_inc_exp(t), cont);
            break;
        case TR_DEC: //--a
        case TR_POST_DEC: //a--
            checkTreeList(TREE_dec_exp(t), cont);
            break;
        case TR_SIZEOF: // sizeof(a)
            checkTreeList(TREE_sizeof_exp(t), cont);
            break;
        case TR_CALL:
            if (!checkCall(t, cont)) { return false; }
            break;
        case TR_ARRAY:
            checkTreeList(TREE_array_base(t), cont);
            checkTreeList(TREE_array_indx(t), cont);
            break;
        case TR_DMEM: // a.b
        case TR_INDMEM: // a->b
        case TR_PRAGMA:
        case TR_PREP:
        case TR_DECL:
            break;
        default: ASSERTN(0, ("unknown tree type:%d", t->getCode()));
        }
    }
    return true;
}


INT TypeCheck()
{
    Scope * s = get_global_scope();
    if (s == nullptr) { return ST_SUCC; }
    for (Decl * dcl = s->getDeclList(); dcl != nullptr; dcl = DECL_next(dcl)) {
        ASSERT0(dcl->getDeclScope() == s);
        checkDeclaration(dcl);
        if (dcl->is_fun_def()) {
            TYCtx ct;
            ct.current_func_declaration = dcl;
            checkDeclInit(dcl->getFunBody()->getDeclList(), nullptr);
            checkTreeList(dcl->getFunBody()->getStmtList(), &ct);
            if (g_err_msg_list.has_msg()) {
                return ST_ERR;
            }
        }
    }

    checkTreeList(s->getStmtList(), nullptr);
    if (g_err_msg_list.has_msg()) {
        return ST_ERR;
    }

    return ST_SUCC;
}

} //namespace xfe

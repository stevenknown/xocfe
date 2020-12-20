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
    Decl const* dclor = get_pure_declarator(d);
    ASSERT0(dclor);
    while (dclor != nullptr) {
        if (DECL_dt(dclor) == DCL_FUN) {
            Decl * ret_value_type = DECL_next(dclor);
            if (ret_value_type) {
                if (DECL_dt(ret_value_type) == DCL_FUN) {
                    err(g_real_line_num,
                        "return value type of function can not be a function");
                    return ST_ERR;
                }
                if (DECL_dt(ret_value_type) == DCL_ARRAY) {
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


static bool checkCall(Tree * t, TYCtx * cont)
{
    if (ST_SUCC != TypeCheckTreeList(TREE_para_list(t), cont)) {
        return false;
    }
    if (ST_SUCC != TypeCheckTreeList(TREE_fun_exp(t), cont)) { return false; }
    Decl * fun_decl = TREE_result_type(TREE_fun_exp(t));

    //Return type is the call type.
    //And here constructing return value type.
    //TypeSpec * ty = DECL_spec(fun_decl);
    Decl * pure_decl = PURE_DECL(fun_decl);
    if (DECL_dt(pure_decl) == DCL_FUN) {
        pure_decl = DECL_next(pure_decl);
    }

    //Do legality checking first for the return value type.
    if (pure_decl) {
        if (is_array(pure_decl)) {
            err(TREE_lineno(t), "function cannot returns array");
        }
        if (is_fun_decl(pure_decl)) {
            err(TREE_lineno(t), "function cannot returns function");
        }
    }

    //Check parameter list
    Decl * formal_param_decl = get_parameter_list(
        TREE_result_type(TREE_fun_exp(t)));
    Tree * real_param = TREE_para_list(t);
    INT count = 0;
    if (formal_param_decl != nullptr) {
        while (formal_param_decl != nullptr && real_param != nullptr) {
            count++;
            Decl * pld = TREE_result_type(real_param);
            if (!checkParam(formal_param_decl, pld)) {
                err(TREE_lineno(t), "%dth parameter type incompatible", count);
                return false;
            }

            formal_param_decl = DECL_next(formal_param_decl);
            real_param = TREE_nsib(real_param);

            if (formal_param_decl &&
                DECL_dt(formal_param_decl) == DCL_VARIABLE) {
                ASSERTN(!DECL_next(formal_param_decl),
                       ("DCL_VARIABLE must be last formal-parameter"));
                formal_param_decl = nullptr;
                real_param = nullptr;
            }
        }
    }

    if (formal_param_decl != nullptr || real_param != nullptr) {
        CHAR * name = nullptr;
        if (TREE_type(TREE_fun_exp(t)) == TR_ID) {
            name = SYM_name(TREE_id(TREE_fun_exp(t)));
        }

        Decl * p = get_parameter_list(TREE_result_type(TREE_fun_exp(t)));
        UINT c = 0;
        while (p != nullptr) {
            c++;
            p = DECL_next(p);
        }

        if (count == 0) {
            err(TREE_lineno(t),
                "function '%s' cannot take any parameter",
                name != nullptr ? name : "");
        } else {
            err(TREE_lineno(t),
                "function '%s' should take %d parameters",
                name != nullptr ? name : "", c);
        }

        return false;
    }
    return true;
}


//True if decl can be convert to pointer without cast.
bool isConsistentWithPointer(Tree * t)
{
    ASSERT0(t);
    Decl * decl = TREE_result_type(t);
    ASSERT0(decl);

    switch (TREE_type(t)) {
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
    if (is_scalar(decl)) {
        return false;
    }

    return true;
}


//Check lhs of assignment.
static void checkAssignLHS(Tree * t)
{
    ASSERT0(t);
    switch (TREE_type(TREE_lchild(t))) {
    case TR_ID:
    case TR_DEREF:
    case TR_DMEM:
    case TR_INDMEM:
    case TR_ARRAY:
        break;
    default:
        err(TREE_lineno(t),
            "'%s': the left operand must be left-value",
            TOKEN_INFO_name(get_token_info(TREE_token(t))));
    }
}


static bool TypeCheckAssign(Tree * t, TYCtx * cont)
{
    TypeCheckTreeList(TREE_lchild(t), cont);
    TypeCheckTreeList(TREE_rchild(t), cont);
    if ((is_pointer(TREE_result_type(TREE_lchild(t))) &&
         !isConsistentWithPointer(TREE_rchild(t))) ||
        (is_pointer(TREE_result_type(TREE_rchild(t))) &&
         !isConsistentWithPointer(TREE_lchild(t)))) {
        xcom::StrBuf bufl(64);
        xcom::StrBuf bufr(64);
        format_declaration(bufl, TREE_result_type(TREE_lchild(t)));
        format_declaration(bufr, TREE_result_type(TREE_rchild(t)));
        warn(TREE_lineno(t),
             "should not assign '%s' to '%s'", bufr.buf, bufl.buf);
    }
    //Check lhs of assignment.
    checkAssignLHS(t);
    return true;
}


static void TypeCheckDeclInit(Decl const* decl, TYCtx * cont)
{
    for (Decl const* dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        if (!is_initialized(dcl)) { continue; }
        Tree * inittree = get_decl_init_tree(dcl);
        ASSERT0(inittree);
        TypeCheckTreeList(inittree, cont);    
    }
}


static void TypeCheckInitValScope(Tree const* t, TYCtx * cont)
{
    TypeCheckTreeList(TREE_initval_scope(t), cont);
}


//Perform type checking.
INT TypeCheckTreeList(Tree * t, TYCtx * cont)
{
    TYCtx ct;
    if (cont == nullptr) {
        cont = &ct;
    }

    for (; t != nullptr; t = TREE_nsib(t)) {
        g_src_line_num = TREE_lineno(t);
        switch (TREE_type(t)) {
        case TR_ASSIGN:
            TypeCheckAssign(t, cont);
            break;
        case TR_ID:
        case TR_IMM:
        case TR_IMML:
        case TR_IMMU:
        case TR_IMMUL:
        case TR_FP:            // double
        case TR_FPF:           // float
        case TR_FPLD:          // long double
        case TR_ENUM_CONST:
        case TR_STRING:
        case TR_LOGIC_OR:      // logical or ||
        case TR_LOGIC_AND:     // logical and &&
        case TR_INCLUSIVE_OR:  // inclusive or |
        case TR_XOR:           // exclusive or
        case TR_INCLUSIVE_AND: // inclusive and &
        case TR_SHIFT:         // >> <<
        case TR_EQUALITY:      // == !=
        case TR_RELATION:      // < > >= <=
        case TR_ADDITIVE:      // '+' '-'
        case TR_MULTI:         // '*' '/' '%'
            TypeCheckTreeList(TREE_lchild(t), cont);
            TypeCheckTreeList(TREE_rchild(t), cont);
            break;
        case TR_SCOPE:
            TypeCheckDeclInit(SCOPE_decl_list(TREE_scope(t)), cont);
            TypeCheckTreeList(SCOPE_stmt_list(TREE_scope(t)), cont);
            break;
        case TR_INITVAL_SCOPE:
            TypeCheckInitValScope(t, cont);
            break;
        case TR_IF:
            TypeCheckTreeList(TREE_if_det(t), cont);
            TypeCheckTreeList(TREE_if_true_stmt(t), cont);
            TypeCheckTreeList(TREE_if_false_stmt(t), cont);
            break;
        case TR_DO:
            TypeCheckTreeList(TREE_dowhile_body(t), cont);
            TypeCheckTreeList(TREE_dowhile_det(t), cont);
            break;
        case TR_WHILE:
            TypeCheckTreeList(TREE_whiledo_det(t), cont);
            TypeCheckTreeList(TREE_whiledo_body(t), cont);
            break;
        case TR_FOR:
            TypeCheckTreeList(TREE_for_init(t), cont);
            TypeCheckTreeList(TREE_for_det(t), cont);
            TypeCheckTreeList(TREE_for_step(t), cont);
            TypeCheckTreeList(TREE_for_body(t), cont);
            break;
        case TR_SWITCH:
            TypeCheckTreeList(TREE_switch_det(t), cont);
            TypeCheckTreeList(TREE_switch_body(t), cont);
            break;
        case TR_BREAK:
        case TR_CONTINUE:
        case TR_GOTO:
        case TR_LABEL:
        case TR_DEFAULT:
        case TR_CASE:
            break;
        case TR_RETURN:
            TypeCheckTreeList(TREE_ret_exp(t), cont);
            break;
        case TR_COND:      //formulized log_OR_exp?exp:cond_exp
            TypeCheckTreeList(TREE_det(t), cont);
            TypeCheckTreeList(TREE_true_part(t), cont);
            TypeCheckTreeList(TREE_false_part(t), cont);
            break;
        case TR_CVT:       //type convertion
            TypeCheckTreeList(TREE_cast_exp(t), cont);
            break;
        case TR_TYPE_NAME: //user defined type or C standard type
            break;
        case TR_LDA:       // &a get address of 'a'
        case TR_DEREF:     // *p  dereferencing the pointer 'p'
        case TR_PLUS:      // +123
        case TR_MINUS:     // -123
        case TR_REV:       // Reverse
        case TR_NOT:       // get non-value
            TypeCheckTreeList(TREE_lchild(t), cont);
            break;
        case TR_INC:       //++a
        case TR_POST_INC:  //a++
            TypeCheckTreeList(TREE_inc_exp(t), cont);
            break;
        case TR_DEC:       //--a
        case TR_POST_DEC:  //a--
            TypeCheckTreeList(TREE_dec_exp(t), cont);
            break;
        case TR_SIZEOF:    // sizeof(a)
            TypeCheckTreeList(TREE_sizeof_exp(t), cont);
            break;
        case TR_CALL:
            if (!checkCall(t, cont)) { goto FAILED; }
            break;
        case TR_ARRAY:
            TypeCheckTreeList(TREE_array_base(t), cont);
            TypeCheckTreeList(TREE_array_indx(t), cont);
            break;
        case TR_DMEM:      // a.b
        case TR_INDMEM:    // a->b
        case TR_PRAGMA:
        case TR_PREP:
            break;
        default: ASSERTN(0, ("unknown tree type:%d", TREE_type(t)));
        }
    }
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


INT TypeCheck()
{
    Scope * s = get_global_scope();
    Decl * dcl = SCOPE_decl_list(s);
    INT st = ST_SUCC;
    while (dcl != nullptr) {
        ASSERT0(DECL_decl_scope(dcl) == s);
        checkDeclaration(dcl);
        if (DECL_is_fun_def(dcl)) {
            TypeCheckDeclInit(SCOPE_decl_list(DECL_fun_body(dcl)), nullptr);
            Tree * stmt = SCOPE_stmt_list(DECL_fun_body(dcl));
            TypeCheckTreeList(stmt, nullptr);
            if (g_err_msg_list.get_elem_count() > 0) {
                st = ST_ERR;
                break;
            }
        }
        dcl = DECL_next(dcl);
    }
    return st;
}

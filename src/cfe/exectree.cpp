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

//Computing expected value in compiling period, such as constant expression.
// 'g_is_allow_float' cannot be used via extern , it must be assigned with
// 'compute_constant_value' absolutely.

static bool g_is_allow_float = false;
static Stack<CELL*> g_cell_stack;
static bool compute_conditional_exp(IN Tree * t);

static CELL * pushv(LONGLONG v)
{
    CELL * c = newcell(0);
    CELL_val(c) = v;
    CELL_line_no(c) = g_real_line_num;
    g_cell_stack.push(c);
    return c;
}


static LONGLONG popv()
{
    CELL *c = g_cell_stack.pop();
    if (!c) {
        err(g_real_line_num, "cell value stack cannot be nullptr");
        return -1;
    }
    free_cell(c);
    return (LONG)CELL_val(c);
}


static bool compute_enum_const(Tree * t)
{
    Enum * e = TREE_enum(t);
    INT i = TREE_enum_val_idx(t);
    EnumValueList * evl = ENUM_vallist(e);
    for (; i > 0; i--, evl = EVAL_LIST_next(evl)) {
        ASSERT0(evl);
    }

    ASSERT0(evl);
    pushv(EVAL_LIST_val(evl));
    return true;
}


//Compute byte size of TYPE_NAME and record in the cell stack, or record
//-1 if failed. The function return true if the computation is success,
//otherwise return false.
static bool compute_sizeof(Tree * t)
{
    Tree * p = TREE_sizeof_exp(t);
    if (TREE_type(p) == TR_TYPE_NAME) {
        Decl * dcl = TREE_type_name(p);
        ASSERT0(dcl && DECL_dt(dcl) == DCL_TYPE_NAME);
        ASSERT0(DECL_spec(dcl));

        if (is_user_type_ref(dcl)) {
            dcl = factor_user_type(dcl);
            TREE_type_name(p) = dcl;
        }

        ULONG sz = get_decl_size(dcl);
        //if (is_complex_type(abs_decl) || is_user_type_ref(dcl)) {
        //    sz = getComplexTypeSize(dcl);
        //} else {
        //    sz = getSimplyTypeSize(type_spec);
        //}

        if (sz != 0) {
            pushv(sz);
            return true;
        }
    } else {
        return compute_conditional_exp(p);
    }

    err(TREE_lineno(p), "'sizeof' need type-name");

    return false;
}


static bool compute_unary_op(Tree * t)
{
    if (!compute_conditional_exp(TREE_lchild(t))) {
        return false;
    }
    LONGLONG l = popv();
    switch (TREE_type(t)) {
    case TR_PLUS: // +123
        pushv(l);
        break;
    case TR_MINUS:  // -123
        pushv((LONGLONG)-l);
        break;
    case TR_REV:  // Reverse
        pushv(~l);
        break;
    case TR_NOT:  // get non-value
        pushv(!l);
        break;
    default:
        err(TREE_lineno(t),"illegal duality expression");
        return false;
    }
    return true;
}


static bool compute_binary_op(Tree * t)
{
    if (t == nullptr) { return false; }
    LONGLONG r,l;
    if (!compute_conditional_exp(TREE_lchild(t))) { return false; }
    if (!compute_conditional_exp(TREE_rchild(t))) { return false; }
    r = popv();
    l = popv();
    switch (TREE_type(t)) {
    case TR_LOGIC_OR: //logical or
        l = l || r;
        pushv(l);
        break;
    case TR_LOGIC_AND: //logical and
        l = l && r;
        pushv(l);
        break;
    case TR_INCLUSIVE_OR: //inclusive or
        l = l | r;
        pushv(l);
        break;
    case TR_INCLUSIVE_AND: //inclusive and
        l = l & r;
        pushv(l);
        break;
    case TR_XOR: //exclusive or
        l = l ^ r;
        pushv(l);
        break;
    case TR_EQUALITY: // == !=
        switch (TREE_token(t)) {
        case T_EQU:
            l = (l == r);
            pushv(l);
            break;
        case T_NOEQU:
            l = (l != r);
            pushv(l);
            break;
        default: UNREACHABLE();
        }
        break;
    case TR_RELATION: // < > >= <=
        switch (TREE_token(t)) {
        case T_LESSTHAN:
            l = (l == r);
            pushv(l);
            break;
        case T_MORETHAN:
            l = (l != r);
            pushv(l);
            break;
        case T_NOLESSTHAN:
            l = (l >= r);
            pushv(l);
            break;
        case T_NOMORETHAN:
            l = (l <= r);
            pushv(l);
            break;
        default: UNREACHABLE();
        }
        break;
    case TR_SHIFT:   // >> <<
        switch (TREE_token(t)) {
        case T_LSHIFT:
            l = (l >> r);
            pushv(l);
            break;
        case T_RSHIFT:
            l = (l << r);
            pushv(l);
            break;
        default: UNREACHABLE();
        }
        break;
    case TR_ADDITIVE: // '+' '-'
        switch (TREE_token(t)) {
        case T_ADD:
            l = (l + r);
            pushv(l);
            break;
        case T_SUB:
            l = (l - r);
            pushv(l);
            break;
        default: UNREACHABLE();
        }
        break;
    case TR_MULTI:    // '*' '/' '%'
        switch (TREE_token(t)) {
        case T_ASTERISK:
            l = (l * r);
            pushv(l);
            break;
        case T_DIV:
            l = (l / r);
            pushv(l);
            if (r == 0) { warn(TREE_lineno(t), "divisor is zero"); }
            break;
        case T_MOD:
            l = (l % r);
            pushv(l);
            if (r == 0) { warn(TREE_lineno(t), "divisor is zero"); }
            break;
        default: UNREACHABLE();
        }
        break;
    default:
        err(TREE_lineno(t),"illegal duality expression");
        return false;
    }
    return true;
}


static bool compute_conditional_exp(IN Tree * t)
{
    if (t == nullptr) { return true;}
    switch (TREE_type(t)) {
    case TR_ENUM_CONST:
        return compute_enum_const(t);
    case TR_PLUS: // +123
    case TR_MINUS:  // -123
    case TR_REV:  // Reverse
    case TR_NOT:  // get non-value
        return compute_unary_op(t);
    case TR_LOGIC_OR: //logical or
    case TR_LOGIC_AND: //logical and
    case TR_INCLUSIVE_OR: //inclusive or
    case TR_INCLUSIVE_AND: //inclusive and
    case TR_XOR: //exclusive or
    case TR_EQUALITY: // == !=
    case TR_RELATION: // < > >= <=
    case TR_SHIFT:   // >> <<
    case TR_ADDITIVE: // '+' '-'
    case TR_MULTI:// '*' '/' '%'
        return compute_binary_op(t);
    case TR_IMM:
    case TR_IMMU:
    case TR_IMML:
    case TR_IMMUL:
        pushv(TREE_imm_val(t));
        break;
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
        if (!g_is_allow_float) {
            err(TREE_lineno(t),"constant expression is not integral");
            return false;
        }
        pushv((LONGLONG)atof(SYM_name(TREE_fp_str_val(t))));
        break;
    case TR_SIZEOF:
        return compute_sizeof(t);
    case TR_ID:
        {
            Decl * dcl = nullptr;
            if (!is_decl_exist_in_outer_scope(SYM_name(TREE_id(t)), &dcl)) {
                err(TREE_lineno(t), "'%s' undefined");
                return false;
            }
            err(TREE_lineno(t), "expected constant expression");
            return false;

            //TODO: infer the constant value of ID.
            //pushv(get_decl_size(dcl));
        }
        break;
    case TR_COND:
        {
            if (!compute_conditional_exp(TREE_det(t))) { return false; }
            LONGLONG v = popv();
            if (v != 0) {
                return compute_conditional_exp(TREE_true_part(t));
            } else {
                return compute_conditional_exp(TREE_false_part(t));
            }
        }
        break;
    default:
        err(TREE_lineno(t), "expected constant expression");
        return false;
    }
    return true;
}


bool computeConstExp(IN Tree * t, OUT LONGLONG * v, bool is_allow_float)
{
    ASSERT0(t && v);
    g_is_allow_float = is_allow_float;
    if (!compute_conditional_exp(t)) {
        *v = 0;
        return false;
    }
    *v = popv();
    return true;
}

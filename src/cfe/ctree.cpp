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

namespace xfe {

UINT g_tree_count = TREE_ID_UNDEF + 1;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    if (p == nullptr) { return 0; }
    ::memset((void*)p,0,size);
    return p;
}


//Alloc a new tree node from 'g_pool_tree_used'.
Tree * allocTreeNode(TREE_CODE tnt, INT lineno)
{
    Tree * t = (Tree*)xmalloc(sizeof(Tree));
    TREE_id(t) = g_tree_count++;
    TREE_code(t) = tnt;
    TREE_lineno(t) = lineno;
    TREE_parent(t) = nullptr;
    return t;
}


bool Tree::is_indirect_tree_node() const
{
    switch (getCode()) {
    case TR_DMEM:
    case TR_INDMEM:
    case TR_DEREF:
        return true;
    default: break;
    }
    return false;
}


void dump_trees(Tree const* t)
{
    while (t != nullptr) {
        t->dump();
        t = TREE_nsib(t);
    }
}


static void dump_line(Tree const* t)
{
    xoc::prt(g_logmgr, " LOC:%d", t->getLineno());
}


static void dump_ty(Decl const* ty)
{
    xcom::DefFixedStrBuf sbuf;
    format_declaration(sbuf, ty, true);
    xoc::prt(g_logmgr, " TY<%s>", sbuf.getBuf());
}


static void dump_res_ty(Tree const* t)
{
    dump_ty(t->getResultType());
}


void Tree::dump() const
{
    Tree const* t = this;
    DUMMYUSE(t);
    if (t == nullptr || g_logmgr == nullptr) { return; }

    UINT dn = 2;
    switch (t->getCode()) {
    case TR_ASSIGN:
        //'='  '*='  '/='  '%='  '+='  '-='  '<<='
        //'>>='  '&='  '^='  '|='
        note(g_logmgr, "\nASSIGN(id:%u):%s",
             t->id(), TOKEN_INFO_name(get_token_info(TREE_token(t))));
        dump_res_ty(t);
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        dump_trees(TREE_rchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_ID: {
        CHAR const* name = TREE_id_name(t)->getStr();
        Decl const* id_decl = TREE_id_decl(t);
        if (id_decl != nullptr) {
            xcom::DefFixedStrBuf sbuf;
            format_declaration(sbuf, t->getResultType(), true);
            sbuf.strcat(" -- ");
            if (DECL_is_sub_field(id_decl)) {
                TypeAttr * ty = DECL_base_type_spec(id_decl);
                format_attr(sbuf, ty, id_decl->is_pointer());
                note(g_logmgr, "\n%s(id:%u) base-type:%s",
                     name, t->id(), sbuf.getBuf());
            } else {
                Scope * s = DECL_decl_scope(id_decl);
                format_declaration(sbuf, get_decl_in_scope(name, s), true);
                xoc::note(g_logmgr, "\nID(id:%u):'%s' Scope:%d Decl:%s",
                          t->id(), name, SCOPE_level(s), sbuf.getBuf());
            }
        } else {
            note(g_logmgr, "\nreferred ID(id:%d):'%s'",
                 t->id(), name);
            dump_res_ty(t);
        }
        dump_line(t);
        break;
    }
    case TR_IMM:
        #ifdef _VC6_
        note(g_logmgr, "\nIMM(id:%u):%d (0x%x)",
             t->id(),
             (INT)TREE_imm_val(t),
             (INT)TREE_imm_val(t));
        dump_res_ty(t);
        #else
        note(g_logmgr, "\nIMM(id:%u):%lld (0x%llx)",
             t->id(),
             (LONGLONG)TREE_imm_val(t),
             (ULONGLONG)TREE_imm_val(t));
        dump_res_ty(t);
        #endif
        dump_line(t);
        break;
    case TR_IMMU:
        #ifdef _VC6_
        note(g_logmgr, "\nIMMU(id:%u):%u (0x%x)",
             t->id(),
             (UINT)TREE_imm_val(t),
             (UINT)TREE_imm_val(t));
        dump_res_ty(t);
        #else
        note(g_logmgr, "\nIMMU(id:%u):%llu (0x%llx)",
             t->id(),
             (ULONGLONG)TREE_imm_val(t),
             (ULONGLONG)TREE_imm_val(t));
        dump_res_ty(t);
        #endif
        dump_line(t);
        break;
    case TR_IMML:
        note(g_logmgr, "\nIMML(id:%u):%lld",
             t->id(), (LONGLONG)TREE_imm_val(t));
        dump_res_ty(t);
        dump_line(t);
        break;
    case TR_IMMUL:
        note(g_logmgr, "\nIMMUL(id:%u):%llu",
             t->id(), (ULONGLONG)TREE_imm_val(t));
        dump_res_ty(t);
        dump_line(t);
        break;
    case TR_FP:
        note(g_logmgr, "\nFP double(id:%u):%s",
             t->id(), SYM_name(TREE_fp_str_val(t)));
        dump_res_ty(t);
        dump_line(t);
        break;
    case TR_FPF:
        note(g_logmgr, "\nFP float(id:%u):%s",
             t->id(), SYM_name(TREE_fp_str_val(t)));
        dump_res_ty(t);
        dump_line(t);
        break;
    case TR_FPLD:
        note(g_logmgr, "\nFP long double(id:%u):%s",
             t->id(), SYM_name(TREE_fp_str_val(t)));
        dump_res_ty(t);
        dump_line(t);
        break;
    case TR_ENUM_CONST: {
        INT v = get_enum_const_val(TREE_enum(t), TREE_enum_val_idx(t));
        CHAR const* s = get_enum_const_name(TREE_enum(t), TREE_enum_val_idx(t));
        note(g_logmgr, "\nENUM_CONST(id:%u):%s %d", t->id(), s, v);
        dump_res_ty(t);
        dump_line(t);
        break;
    }
    case TR_STRING:
        note(g_logmgr, "\nSTRING(id:%u):%s",
             t->id(), SYM_name(TREE_string_val(t)));
        dump_res_ty(t);
        break;
    case TR_LOGIC_OR: //logical or        ||
    case TR_LOGIC_AND: //logical and      &&
    case TR_INCLUSIVE_OR: //inclusive or  |
    case TR_INCLUSIVE_AND: //inclusive and &
    case TR_XOR: //exclusive or
    case TR_EQUALITY: // == !=
    case TR_RELATION: // < > >= <=
    case TR_SHIFT:   // >> <<
    case TR_ADDITIVE: // '+' '-'
    case TR_MULTI:    // '*' '/' '%'
        note(g_logmgr, "\nOP(id:%u):%s", t->id(),
             TOKEN_INFO_name(get_token_info(TREE_token(t))));
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        dump_trees(TREE_rchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_IF:
        note(g_logmgr, "\nIF(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_if_det(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nTRUE_STMT");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_if_true_stmt(t));
        g_logmgr->decIndent(dn);

        if (TREE_if_false_stmt(t)) {
            note(g_logmgr, "\nFALSE_STMT");
            g_logmgr->incIndent(dn);
            dump_trees(TREE_if_false_stmt(t));
            g_logmgr->decIndent(dn);
        }
        note(g_logmgr, "\nENDIF");
        break;
    case TR_DO:
        note(g_logmgr, "\nDO(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_dowhile_body(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nWHILE");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_dowhile_det(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_WHILE:
        note(g_logmgr, "\nWHILE(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_whiledo_det(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nDO");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_whiledo_body(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_FOR:
        note(g_logmgr, "\nFOR_INIT(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_for_init(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nFOR_DET");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_for_det(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nFOR_STEP");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_for_step(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nFOR_BODY");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_for_body(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_SWITCH:
        note(g_logmgr, "\nSWITCH_DET(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_switch_det(t));
        g_logmgr->decIndent(dn);
        note(g_logmgr, "\nSWITCH_BODY");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_switch_body(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_BREAK:
        note(g_logmgr, "\nBREAK(id:%u)", t->id());
        dump_line(t);
        break;
    case TR_CONTINUE:
        note(g_logmgr, "\nCONTINUE(id:%u)", t->id());
        dump_line(t);
        break;
    case TR_RETURN:
        note(g_logmgr, "\nRETURN(id:%u)", t->id());
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_ret_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_GOTO:
        note(g_logmgr, "\nGOTO(id:%u):%s",
             t->id(), SYM_name(LABELINFO_name(TREE_lab_info(t))));
        dump_line(t);
        break;
    case TR_LABEL:
        note(g_logmgr, "\nLABEL(id:%u):%s",
             t->id(), SYM_name(LABELINFO_name(TREE_lab_info(t))));
        dump_line(t);
        break;
    case TR_CASE:
        note(g_logmgr, "\nCASE(id:%u):%d",
             t->id(), TREE_case_value(t));
        dump_line(t);
        break;
    case TR_DEFAULT:
        note(g_logmgr, "\nDEFAULT(id:%u)", t->id());
        dump_line(t);
        break;
    case TR_COND: //formulized log_OR_exp?exp:cond_exp
        note(g_logmgr, "\nCOND_EXE(id:%u)", t->id());
        g_logmgr->incIndent(dn);
        dump_trees(TREE_det(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nTRUE_EXP");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_true_part(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nFALSE_EXP");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_false_part(t));
        g_logmgr->decIndent(dn);

        dump_line(t);
        break;
    case TR_CVT: //type convertion
        note(g_logmgr, "\nCONVERT(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_cvt_type(t));
        dump_trees(TREE_cvt_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_TYPE_NAME: { //user defined type ord C standard type
        xcom::DefFixedStrBuf sbuf;
        format_declaration(sbuf, t->getTypeName(), true);
        note(g_logmgr, "\nTYPE_NAME(id:%u):%s", t->id(), sbuf.getBuf());
        dump_line(t);
        break;
    }
    case TR_LDA: // &a get address of 'a'
        note(g_logmgr, "\nLDA(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DEREF: // *p  dereferencing the pointer 'p'
        note(g_logmgr, "\nDEREF(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_PLUS: // +123
        note(g_logmgr, "\nPOS(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_MINUS: // -123
        note(g_logmgr, "\nNEG(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_REV:  // Reverse
        note(g_logmgr, "\nREV(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_NOT:  // get non-value
        note(g_logmgr, "\nNOT(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_INC:   //++a
        note(g_logmgr, "\nPREV_INC(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_inc_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DEC: //--a
        note(g_logmgr, "\nPREV_DEC(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_dec_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_POST_INC: //a++
        note(g_logmgr, "\nPOST_INC(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_inc_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_POST_DEC: //a--
        note(g_logmgr, "\nPOST_INC(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_dec_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_SIZEOF: // sizeof(a)
        note(g_logmgr, "\nSIZEOF(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_sizeof_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DMEM:
        note(g_logmgr, "\nDMEM(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_base_region(t));
        dump_trees(TREE_field(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_INDMEM:
        note(g_logmgr, "\nINDMEM(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        dump_trees(TREE_base_region(t));
        dump_trees(TREE_field(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_ARRAY:
        note(g_logmgr, "\nARRAY(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        note(g_logmgr, "\nBASE:");

        g_logmgr->incIndent(dn);
        dump_trees(TREE_array_base(t));
        g_logmgr->decIndent(dn);

        note(g_logmgr, "\nINDX:");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_array_indx(t));
        g_logmgr->decIndent(dn);

        g_logmgr->decIndent(dn);
        break;
    case TR_CALL: {
        xcom::DefFixedStrBuf sbuf;
        format_declaration(sbuf, t->getResultType(), true);
        note(g_logmgr, "\nCALL(id:%u) RETV_TY<%s>", t->id(), sbuf.getBuf());
        dump_line(t);

        g_logmgr->incIndent(dn);
        //Dump function
        note(g_logmgr, "\nFUN_BASE:");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_fun_exp(t));
        g_logmgr->decIndent(dn);

        //Dump parameters
        note(g_logmgr, "\nPARAM_LIST:");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_para_list(t));
        g_logmgr->decIndent(dn);

        g_logmgr->decIndent(dn);
        break;
    }
    case TR_SCOPE:
        g_logmgr->incIndent(dn);
        note(g_logmgr, "\n{");
        dump_line(t);

        g_logmgr->incIndent(dn);
        TREE_scope(t)->dump(DUMP_SCOPE_FUNC_BODY|DUMP_SCOPE_STMT_TREE);
        g_logmgr->decIndent(dn);
        note(g_logmgr, "\n}");
        g_logmgr->decIndent(dn);
        break;
    case TR_INITVAL_SCOPE:
        g_logmgr->incIndent(dn);
        note(g_logmgr, "\n{ (id:%u)", t->id());
        g_logmgr->incIndent(dn);
        dump_trees(TREE_initval_scope(t));
        g_logmgr->decIndent(dn);
        note(g_logmgr, "\n}");
        g_logmgr->decIndent(dn);
        break;
    case TR_PRAGMA:
        note(g_logmgr, "\nPRAGMA(id:%u)", t->id());
        dump_line(t);
        for (TokenList * tl = TREE_token_lst(t);
             tl != nullptr; tl = TL_next(tl)) {
            switch (TL_tok(tl)) {
            case T_ID:
                prt(g_logmgr, " %s", SYM_name(TL_id_name(tl)));
                break;
            case T_STRING:
                prt(g_logmgr, " \"%s\"", SYM_name(TL_str(tl)));
                break;
            case T_CHAR_LIST:
                prt(g_logmgr, " '%s'", SYM_name(TL_chars(tl)));
                break;
            default:
                prt(g_logmgr, " %s", getTokenName(TL_tok(tl)));
            }
        }
        break;
    case TR_PREP:
        note(g_logmgr, "\nPREP(id:%u)", t->id());
        dump_line(t);

        for (TokenList * tl = TREE_token_lst(t);
             tl != nullptr; tl = TL_next(tl)) {
            switch (TL_tok(tl)) {
            case T_ID:
                prt(g_logmgr, " %s", SYM_name(TL_id_name(tl)));
                break;
            case T_STRING:
                prt(g_logmgr, " \"%s\"", SYM_name(TL_str(tl)));
                break;
            case T_CHAR_LIST:
                prt(g_logmgr, " '%s'", SYM_name(TL_chars(tl)));
                break;
            default:
                prt(g_logmgr, " %s", getTokenName(TL_tok(tl)));
            }
        }
        break;
    case TR_DECL:
        note(g_logmgr, "\nDECL(id:%u)", t->id());
        dump_res_ty(t);
        dump_line(t);

        g_logmgr->incIndent(dn);
        TREE_decl(t)->dump();
        g_logmgr->decIndent(dn);
        break;
    default:
        ASSERTN(0, ("unknown tree type:%d",t->getCode()));
        return;
    }
}


bool Tree::is_imm_int() const
{
    switch(getCode()) {
    case TR_IMM:
    case TR_IMML:
    case TR_IMMU:
    case TR_IMMUL:
        return true;
    default:;
    }
    return false;
}


bool Tree::is_imm_fp() const
{
    switch (getCode()) {
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
        return true;
    default:;
    }
    return false;
}


bool Tree::is_aggr_field_access() const
{
    return getCode() == TR_DMEM || getCode() == TR_INDMEM;
}


//Get base of array if exist.
Tree * get_array_base(Tree * t)
{
    ASSERT0(t->getCode() == TR_ARRAY);
    Tree * base = TREE_array_base(t);
    for (; base != nullptr && base->getCode() == TR_ARRAY;
         base = TREE_array_base(base)) {
    }
    ASSERT0(base);
    return base;
}


//Get base of aggregate if exist.
Tree * get_aggr_base(Tree * t)
{
    ASSERT0(t->is_aggr_field_access());
    Tree * base = TREE_base_region(t);
    for (; base != nullptr && base->is_aggr_field_access();
         base = TREE_base_region(base)) {
    }
    ASSERT0(base);
    return base;
}


//Get base of aggregate/array if exist.
Tree * get_base(Tree * t)
{
    Tree * base = nullptr;
    if (t->is_aggr_field_access()) {
        base = TREE_base_region(t);
    } else if (t->getCode() == TR_ARRAY) {
        base = TREE_array_base(t);
    } else {
        return nullptr;
    }

    ASSERT0(base);
    for (; base != nullptr; ) {
        if (!base->is_aggr_field_access() && base->getCode() != TR_ARRAY) {
             break;
         }

         if (base->is_aggr_field_access()) {
             base = TREE_base_region(base);
         } else if (base->getCode() == TR_ARRAY) {
             base = TREE_array_base(base);
         } else {
            UNREACHABLE();
         }
    }
    ASSERT0(base);
    return base;
}


//Return true if given tree node can be regarded as RHS of assignment.
bool Tree::isRHS() const
{
    switch (getCode()) {
    case TR_ASSIGN:
    case TR_IF:
    case TR_ELSE:
    case TR_DO:
    case TR_WHILE:
    case TR_FOR:
    case TR_SWITCH:
    case TR_BREAK:
    case TR_CONTINUE:
    case TR_RETURN:
    case TR_GOTO:
    case TR_LABEL:
    case TR_SCOPE:
    case TR_PRAGMA:
    case TR_PREP:
    case TR_DECL:
        return false;
    case TR_ID:
    case TR_IMM:
    case TR_IMMU:
    case TR_IMML:
    case TR_IMMUL:
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
    case TR_ENUM_CONST:
    case TR_STRING:
    case TR_LOGIC_OR:
    case TR_LOGIC_AND:
    case TR_INCLUSIVE_OR:
    case TR_INCLUSIVE_AND:
    case TR_XOR:
    case TR_EQUALITY:
    case TR_RELATION:
    case TR_SHIFT:
    case TR_ADDITIVE:
    case TR_MULTI:
    case TR_INTRI_FUN:
    case TR_DEFAULT:
    case TR_CASE:
    case TR_COND:
    case TR_CVT:
    case TR_TYPE_NAME:
    case TR_LDA:
    case TR_DEREF:
    case TR_INC:
    case TR_DEC:
    case TR_POST_INC:
    case TR_POST_DEC:
    case TR_PLUS:
    case TR_MINUS:
    case TR_REV:
    case TR_NOT:
    case TR_SIZEOF:
    case TR_DMEM:
    case TR_INDMEM:
    case TR_ARRAY:
    case TR_CALL:
    case TR_INITVAL_SCOPE:
        return true;
    default: UNREACHABLE();
    }
    return false;
}


Tree * Tree::getArrayBase() const
{
    Tree * base = const_cast<Tree*>(this);
    for (; base != nullptr && base->getCode() == TR_ARRAY;
         base = TREE_array_base(base));
    return base;
}


void Tree::setParentForKid()
{
    for (UINT i = 0; i < MAX_TREE_FLDS; i++) {
        Tree * t = TREE_fld(this, i);
        if (t != nullptr) {
            setParent(this , t);
        }
    }
}

} //namespace xfe

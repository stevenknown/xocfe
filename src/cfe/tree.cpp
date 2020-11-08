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

#ifdef _DEBUG_
static UINT g_tree_count = 1;
#endif

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    if (p == nullptr) { return 0; }
    ::memset(p,0,size);
    return p;
}


//Alloc a new tree node from 'g_pool_tree_used'.
Tree * allocTreeNode(TREE_TYPE tnt, INT lineno)
{
    Tree * t = (Tree*)xmalloc(sizeof(Tree));
#ifdef _DEBUG_
    t->id = g_tree_count++;
#endif
    TREE_type(t) = tnt;
    TREE_lineno(t) = lineno;
    TREE_parent(t) = nullptr;
    return t;
}


INT is_indirect_tree_node(Tree *t)
{
    switch (TREE_type(t)) {
    case TR_DMEM:
    case TR_INDMEM:
    case TR_DEREF:
        return 1;
    default: break;
    }
    return 0;
}


void dump_trees(Tree * t)
{
    while (t != nullptr) {
        dump_tree(t);
        t = TREE_nsib(t);
    }
}


static void dump_line(Tree const* t)
{
    xoc::prt(g_logmgr, " [%d]", TREE_lineno(t));
}


void dump_tree(Tree * t)
{
    DUMMYUSE(t);    
#ifdef _DEBUG_
    if (t == nullptr || g_logmgr == nullptr) { return; }
    
    UINT dn = 2;
    StrBuf sbuf(64);
    format_declaration(sbuf, TREE_result_type(t));
    switch (TREE_type(t)) {
    case TR_ASSIGN:
        //'='  '*='  '/='  '%='  '+='  '-='  '<<='
        //'>>='  '&='  '^='  '|='
        note(g_logmgr, "\nASSIGN(id:%u):%s <%s>",
             TREE_uid(t), TOKEN_INFO_name(get_token_info(TREE_token(t))),
             sbuf.buf);
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        dump_trees(TREE_rchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_ID: {
        CHAR * name = SYM_name(TREE_id(t));
        if (TREE_id_decl(t) != nullptr) {
            sbuf.strcat("-- ");
            if (DECL_is_sub_field(TREE_id_decl(t))) {
                TypeSpec * ty = DECL_base_type_spec(TREE_id_decl(t));
                format_decl_spec(sbuf, ty, is_pointer(TREE_id_decl(t)));
                note(g_logmgr, "\n%s(id:%u) base-type:%s",
                     name, TREE_uid(t), sbuf.buf);
            } else {
                SCOPE * s = DECL_decl_scope(TREE_id_decl(t));
                format_declaration(sbuf, get_decl_in_scope(name, s));
                note(g_logmgr, "\nID(id:%u):'%s' SCOPE:%d Decl:%s",
                     TREE_uid(t), name, SCOPE_level(s), sbuf.buf);
            }
        } else {
            note(g_logmgr, "\nreferred ID(id:%d):'%s' <%s>",
                 TREE_uid(t), name, sbuf.buf);
        }
        break;
    }
    case TR_IMM:
        #ifdef _VC6_
        note(g_logmgr, "\nIMM(id:%u):%d (0x%x) <%s>",
             TREE_uid(t),
             (INT)TREE_imm_val(t),
             (INT)TREE_imm_val(t),
             sbuf.buf);
        #else
        note(g_logmgr, "\nIMM(id:%u):%lld (0x%llx) <%s>",
             TREE_uid(t),
             (LONGLONG)TREE_imm_val(t),
             (ULONGLONG)TREE_imm_val(t),
             sbuf.buf);
        #endif
        break;
    case TR_IMMU:
        #ifdef _VC6_
        note(g_logmgr, "\nIMMU(id:%u):%u (0x%x) <%s>",
             TREE_uid(t),
             (UINT)TREE_imm_val(t),
             (UINT)TREE_imm_val(t),
             sbuf.buf);
        #else
        note(g_logmgr, "\nIMMU(id:%u):%llu (0x%llx) <%s>",
             TREE_uid(t),
             (ULONGLONG)TREE_imm_val(t),
             (ULONGLONG)TREE_imm_val(t),
             sbuf.buf);
        #endif
        break;
    case TR_IMML:
        note(g_logmgr, "\nIMML(id:%u):%lld <%s>",
             TREE_uid(t),
             (LONGLONG)TREE_imm_val(t),
             sbuf.buf);
        break;
    case TR_IMMUL:
        note(g_logmgr, "\nIMMUL(id:%u):%llu <%s>",
             TREE_uid(t),
             (ULONGLONG)TREE_imm_val(t),
             sbuf.buf);
        break;
    case TR_FP:
        note(g_logmgr, "\nFP double(id:%u):%s <%s>",
             TREE_uid(t),
             SYM_name(TREE_fp_str_val(t)),
             sbuf.buf);
        break;
    case TR_FPF:
        note(g_logmgr, "\nFP float(id:%u):%s <%s>",
             TREE_uid(t),
             SYM_name(TREE_fp_str_val(t)),
             sbuf.buf);
        break;
    case TR_FPLD:
        note(g_logmgr, "\nFP long double(id:%u):%s <%s>",
             TREE_uid(t),
             SYM_name(TREE_fp_str_val(t)),
             sbuf.buf);
        break;
    case TR_ENUM_CONST: {
        INT v = get_enum_const_val(TREE_enum(t), TREE_enum_val_idx(t));
        CHAR const* s = get_enum_const_name(TREE_enum(t), TREE_enum_val_idx(t));
        note(g_logmgr, "\nENUM_CONST(id:%u):%s %d <%s>",
             TREE_uid(t), s, v, sbuf.buf);
        break;
    }
    case TR_STRING:
        note(g_logmgr, "\nSTRING(id:%u):%s <%s>",
             TREE_uid(t), SYM_name(TREE_string_val(t)), sbuf.buf);
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
        note(g_logmgr, "\nOP(id:%u):%s <%s>", TREE_uid(t),
             TOKEN_INFO_name(get_token_info(TREE_token(t))), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        dump_trees(TREE_rchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_IF:
        note(g_logmgr, "\nIF(id:%u)", TREE_uid(t));
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
        note(g_logmgr, "\nDO(id:%u)", TREE_uid(t));
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
        note(g_logmgr, "\nWHILE(id:%u)", TREE_uid(t));
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
        note(g_logmgr, "\nFOR_INIT(id:%u)", TREE_uid(t));
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
        note(g_logmgr, "\nSWITCH_DET(id:%u)", TREE_uid(t));
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
        note(g_logmgr, "\nBREAK(id:%u)", TREE_uid(t));
        dump_line(t);
        break;
    case TR_CONTINUE:
        note(g_logmgr, "\nCONTINUE(id:%u)", TREE_uid(t));
        dump_line(t);
        break;
    case TR_RETURN:
        note(g_logmgr, "\nRETURN(id:%u)", TREE_uid(t));
        dump_line(t);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_ret_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_GOTO:
        note(g_logmgr, "\nGOTO(id:%u):%s",
             TREE_uid(t), SYM_name(LABELINFO_name(TREE_lab_info(t))));
        dump_line(t);
        break;
    case TR_LABEL:
        note(g_logmgr, "\nLABEL(id:%u):%s",
             TREE_uid(t), SYM_name(LABELINFO_name(TREE_lab_info(t))));
        dump_line(t);
        break;
    case TR_CASE:
        note(g_logmgr, "\nCASE(id:%u):%d",
             TREE_uid(t), TREE_case_value(t));
        dump_line(t);
        break;
    case TR_DEFAULT:
        note(g_logmgr, "\nDEFAULT(id:%u)", TREE_uid(t));
        dump_line(t);
        break;
    case TR_COND: //formulized log_OR_exp?exp:cond_exp
        note(g_logmgr, "\nCOND_EXE(id:%u)", TREE_uid(t));
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
        break;
    case TR_CVT: //type convertion
        note(g_logmgr, "\nCONVERT(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_cvt_type(t));
        dump_trees(TREE_cast_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_TYPE_NAME: //user defined type ord C standard type
        format_declaration(sbuf, TREE_type_name(t));
        note(g_logmgr, "\nTYPE_NAME(id:%u):%s", TREE_uid(t), sbuf.buf);
        break;
    case TR_LDA:   // &a get address of 'a'
        note(g_logmgr, "\nLDA(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DEREF: // *p  dereferencing the pointer 'p'
        note(g_logmgr, "\nDEREF(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_PLUS: // +123
        note(g_logmgr, "\nPOS(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_MINUS:  // -123
        note(g_logmgr, "\nNEG(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_REV:  // Reverse
        note(g_logmgr, "\nREV(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_NOT:  // get non-value
        note(g_logmgr, "\nNOT(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_lchild(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_INC:   //++a
        note(g_logmgr, "\nPREV_INC(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_inc_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DEC:   //--a
        note(g_logmgr, "\nPREV_DEC(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_dec_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_POST_INC: //a++
        note(g_logmgr, "\nPOST_INC(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_inc_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_POST_DEC: //a--
        note(g_logmgr, "\nPOST_INC(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_dec_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_SIZEOF: // sizeof(a)
        note(g_logmgr, "\nSIZEOF(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_sizeof_exp(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_DMEM:
        note(g_logmgr, "\nDMEM(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_base_region(t));
        dump_trees(TREE_field(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_INDMEM:
        note(g_logmgr, "\nINDMEM(id:%u) <%s>", TREE_uid(t), sbuf.buf);
        g_logmgr->incIndent(dn);
        dump_trees(TREE_base_region(t));
        dump_trees(TREE_field(t));
        g_logmgr->decIndent(dn);
        break;
    case TR_ARRAY:
        note(g_logmgr, "\nARRAY(id:%u) <%s>", TREE_uid(t), sbuf.buf);
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
    case TR_CALL:
        note(g_logmgr, "\nCALL(id:%u) RET-Type:<%s>", TREE_uid(t), sbuf.buf);
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
    case TR_SCOPE:
        g_logmgr->incIndent(dn);
        note(g_logmgr, "\n{");
        g_logmgr->incIndent(dn);
        dump_scope(TREE_scope(t),
                   DUMP_SCOPE_FUNC_BODY|DUMP_SCOPE_STMT_TREE);
        g_logmgr->decIndent(dn);
        note(g_logmgr, "\n}");
        g_logmgr->decIndent(dn);
        break;
    case TR_EXP_SCOPE:
        g_logmgr->incIndent(dn);
        note(g_logmgr, "\n{");
        g_logmgr->incIndent(dn);
        dump_trees(TREE_exp_scope(t));
        g_logmgr->decIndent(dn);
        note(g_logmgr, "\n}");
        g_logmgr->decIndent(dn);
        break;
    case TR_PRAGMA:
        note(g_logmgr, "\nPRAGMA(id:%u)", TREE_uid(t));
        dump_line(t);
        for (TokenList * tl = TREE_token_lst(t); tl != nullptr; tl = TL_next(tl)) {
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
        note(g_logmgr, "\nPREP(id:%u)", TREE_uid(t));
        for (TokenList * tl = TREE_token_lst(t); tl != nullptr; tl = TL_next(tl)) {
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
    default:
        ASSERTN(0, ("unknown tree type:%d",TREE_type(t)));
        return;
    }
#endif
}


bool is_imm_int(Tree * t)
{
    switch(TREE_type(t)) {
    case TR_IMM:
    case TR_IMML:
    case TR_IMMU:
    case TR_IMMUL:
        return true;
    default:;
    }
    return false;
}


bool is_imm_fp(Tree * t)
{
    switch (TREE_type(t)) {
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
        return true;
    default:;
    }
    return false;
}

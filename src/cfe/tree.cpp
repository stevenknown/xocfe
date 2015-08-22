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
	if (p == NULL) return 0;
	memset(p,0,size);
	return p;
}


//Alloc a new tree node from 'g_pool_tree_used'.
Tree * new_tree_node(TREE_TYPE tnt, INT lineno)
{
    Tree * t = (Tree*)xmalloc(sizeof(Tree));
#ifdef _DEBUG_
	t->id = g_tree_count++;
#endif
	TREE_type(t) = tnt;
	TREE_lineno(t) = lineno;
	TREE_parent(t) = NULL;
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
	while (t != NULL) {
		dump_tree(t);
		t = TREE_nsib(t);
	}
	fflush(g_tfile);
}


void dump_tree(Tree * t)
{
	UNUSED(t);
#ifdef _DEBUG_
	if (t == NULL || g_tfile == NULL) { return; }
	UINT dn = 2;
	static CHAR res_type_buf[1024];
	res_type_buf[0] = 0;
	format_declaration(res_type_buf, TREE_result_type(t));
	switch (TREE_type(t)) {
		case TR_ASSIGN:
			//'='  '*='  '/='  '%='  '+='  '-='  '<<='
			//'>>='  '&='  '^='  '|='
			note("\nASSIGN(id:%d):%s <%s>",
				TREE_uid(t),
				TOKEN_INFO_name(&g_token_info[TREE_token(t)]),
				res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			dump_trees(TREE_rchild(t));
			g_indent -= dn;
			break;
		case TR_ID:
			{
				CHAR * name = SYM_name(TREE_id(t));
				if (TREE_id_decl(t) != NULL) {
					strcat(res_type_buf, "-- ");
					CHAR * buf = res_type_buf + strlen(res_type_buf);
					if (DECL_is_sub_field(TREE_id_decl(t))) {
						TypeSpec * ty = DECL_base_type_spec(TREE_id_decl(t));
						format_decl_spec(buf, ty, is_pointer(TREE_id_decl(t)));
						note("\n%s(id:%d) base-type:%s",
							 name, TREE_uid(t), res_type_buf);
					} else {
						SCOPE * s = DECL_decl_scope(TREE_id_decl(t));
						format_declaration(buf, get_decl_in_scope(name, s));
						note("\nID(id:%d):'%s' SCOPE:%d Decl:%s",
							 TREE_uid(t), name, SCOPE_level(s), res_type_buf);
					}
				} else {
					note("\nreferred ID(id:%d):'%s' <%s>",
						 TREE_uid(t), name, res_type_buf);
				}
				break;
			}
		case TR_IMM:
			#ifdef _VC6_
			note("\nIMM(id:%d):%d (0x%x) <%s>",
				TREE_uid(t),
				(INT)TREE_imm_val(t),
				(INT)TREE_imm_val(t),
				res_type_buf);
			#else
			note("\nIMM(id:%d):%lld (0x%llx) <%s>",
				TREE_uid(t),
				TREE_imm_val(t),
				TREE_imm_val(t),
				res_type_buf);
			#endif
			break;
		case TR_IMML:
			note("\nIMML(id:%d):%lld <%s>",
				TREE_uid(t),
				TREE_imm_val(t),
				res_type_buf);
			break;
		case TR_FP:  //3.1415926
			note("\nFP(id:%d):%s <%s>",
				TREE_uid(t),
				SYM_name(TREE_fp_str_val(t)),
				res_type_buf);
			break;
		case TR_ENUM_CONST:
			{
				INT v = get_enum_const_val(TREE_enum(t), TREE_enum_val_idx(t));
				CHAR * s = get_enum_const_name(TREE_enum(t),
											   TREE_enum_val_idx(t));
				note("\nENUM_CONST(id:%d):%s %d <%s>",
					TREE_uid(t), s, v, res_type_buf);
				break;
			}
		case TR_STRING:
			note("\nSTRING(id:%d):%s <%s>",
				TREE_uid(t),
				SYM_name(TREE_string_val(t)),
				res_type_buf);
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
			note("\nOP(id:%d):%s <%s>",
				TREE_uid(t),
				TOKEN_INFO_name(&g_token_info[TREE_token(t)]),
				res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			dump_trees(TREE_rchild(t));
			g_indent -= dn;
			break;
		case TR_IF:
			note("\nIF(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_if_det(t));
			g_indent -= dn;
			note("\nTRUE_STMT");
			g_indent += dn;
			dump_trees(TREE_if_true_stmt(t));
			g_indent -= dn;
			if (TREE_if_false_stmt(t)) {
				note("\nFALSE_STMT");
				g_indent += dn;
				dump_trees(TREE_if_false_stmt(t));
				g_indent -= dn;
			}
			note("\nENDIF");
			break;
		case TR_DO:
			note("\nDO(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_dowhile_body(t));
			g_indent -= dn;
			note("\nWHILE");
			g_indent += dn;
			dump_trees(TREE_dowhile_det(t));
			g_indent -= dn;
			break;
		case TR_WHILE:
			note("\nWHILE(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_whiledo_det(t));
			g_indent -= dn;
			note("\nDO");
			g_indent += dn;
			dump_trees(TREE_whiledo_body(t));
			g_indent -= dn;
			break;
		case TR_FOR:
			note("\nFOR_INIT(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_for_init(t));
			g_indent -= dn;
			note("\nFOR_DET");
			g_indent += dn;
			dump_trees(TREE_for_det(t));
			g_indent -= dn;
			note("\nFOR_STEP");
			g_indent += dn;
			dump_trees(TREE_for_step(t));
			g_indent -= dn;
			note("\nFOR_BODY");
			g_indent += dn;
			dump_trees(TREE_for_body(t));
			g_indent -= dn;
			break;
		case TR_SWITCH:
			note("\nSWITCH_DET(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_switch_det(t));
			g_indent -= dn;
			note("\nSWITCH_BODY");
			g_indent += dn;
			dump_trees(TREE_switch_body(t));
			g_indent -= dn;
			break;
		case TR_BREAK:
			note("\nBREAK(id:%d)", TREE_uid(t));
			break;
		case TR_CONTINUE:
			note("\nCONTINUE(id:%d)", TREE_uid(t));
			break;
		case TR_RETURN:
			note("\nRETURN(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_ret_exp(t));
			g_indent -= dn;
			break;
		case TR_GOTO:
			note("\nGOTO(id:%d):%s",
				TREE_uid(t),
				SYM_name(LABEL_INFO_name(TREE_lab_info(t))));
			break;
		case TR_LABEL:
			note("\nLABEL(id:%d):%s",
				TREE_uid(t),
				SYM_name(LABEL_INFO_name(TREE_lab_info(t))));
			break;
		case TR_CASE:
			note("\nCASE(id:%d):%d",
				TREE_uid(t),
				TREE_case_value(t));
			break;
		case TR_DEFAULT:
			note("\nDEFAULT(id:%d)", TREE_uid(t));
			break;
		case TR_COND: //formulized log_OR_exp?exp:cond_exp
			note("\nCOND_EXE(id:%d)", TREE_uid(t));
			g_indent += dn;
			dump_trees(TREE_det(t));
			g_indent -= dn;
			note("\nTRUE_EXP");
			g_indent += dn;
			dump_trees(TREE_true_part(t));
			g_indent -= dn;
			note("\nFALSE_EXP");
			g_indent += dn;
			dump_trees(TREE_false_part(t));
			g_indent -= dn;
			break;
		case TR_CVT: //type convertion
			note("\nCONVERT(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_cvt_type(t));
			dump_trees(TREE_cast_exp(t));
			g_indent -= dn;
			break;
		case TR_TYPE_NAME: //user defined type ord C standard type
			res_type_buf[0] = 0;
			format_declaration(res_type_buf, TREE_type_name(t));
			note("\nTYPE_NAME(id:%d):%s", TREE_uid(t), res_type_buf);
			break;
		case TR_LDA:   // &a get address of 'a'
			note("\nLDA(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_DEREF: //*p  dereferencing the pointer 'p'
			note("\nDEREF(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_PLUS: // +123
			note("\nPOS(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_MINUS:  // -123
			note("\nNEG(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_REV:  // Reverse
			note("\nREV(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_NOT:  // get non-value
			note("\nNOT(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_lchild(t));
			g_indent -= dn;
			break;
		case TR_INC:   //++a
			note("\nPREV_INC(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_inc_exp(t));
			g_indent -= dn;
			break;
		case TR_DEC:   //--a
			note("\nPREV_DEC(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_dec_exp(t));
			g_indent -= dn;
			break;
		case TR_POST_INC: //a++
			note("\nPOST_INC(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_inc_exp(t));
			g_indent -= dn;
			break;
		case TR_POST_DEC: //a--
			note("\nPOST_INC(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_inc_exp(t));
			g_indent -= dn;
			break;
		case TR_SIZEOF: // sizeof(a)
			note("\nSIZEOF(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_sizeof_exp(t));
			g_indent -= dn;
			break;
		case TR_DMEM:
			note("\nDMEM(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_base_region(t));
			dump_trees(TREE_field(t));
			g_indent -= dn;
			break;
		case TR_INDMEM:
			note("\nINDMEM(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			dump_trees(TREE_base_region(t));
			dump_trees(TREE_field(t));
			g_indent -= dn;
			break;
		case TR_ARRAY:
			note("\nARRAY(id:%d) <%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			note("\nBASE:");

			g_indent += dn;
			dump_trees(TREE_array_base(t));
			g_indent -= dn;

			note("\nINDX:");
			g_indent += dn;
			dump_trees(TREE_array_indx(t));
			g_indent -= dn;

			g_indent -= dn;
			break;
		case TR_CALL:
			note("\nCALL(id:%d) RET-Type:<%s>", TREE_uid(t), res_type_buf);
			g_indent += dn;
			//Dump function
			note("\nFUN_BASE:");
			g_indent += dn;
			dump_trees(TREE_fun_exp(t));
			g_indent -= dn;

			//Dump parameters
			note("\nPARAM_LIST:");
			g_indent += dn;
			dump_trees(TREE_para_list(t));
			g_indent -= dn;

			g_indent -= dn;
			break;
		case TR_SCOPE:
			g_indent += dn;
			note("\n{");
			g_indent += dn;
			dump_scope(TREE_scope(t),
					   DUMP_SCOPE_FUNC_BODY|DUMP_SCOPE_STMT_TREE);
			g_indent -= dn;
			note("\n}");
			g_indent -= dn;
			break;
		case TR_EXP_SCOPE:
			g_indent += dn;
			note("\n{");
			g_indent += dn;
			dump_trees(TREE_exp_scope(t));
			g_indent -= dn;
			note("\n}");
			g_indent -= dn;
			break;
		case TR_PRAGMA:
			note("\nPRAGMA(id:%d)", TREE_uid(t));
			for (TokenList * tl = TREE_pragma_tok_lst(t);
				 tl != NULL; tl = TL_next(tl)) {
				switch (TL_tok(tl)) {
				case T_ID:
					fprintf(g_tfile, " %s", SYM_name(TL_id_name(tl)));
					break;
				case T_STRING:
					fprintf(g_tfile, " \"%s\"", SYM_name(TL_str(tl)));
					break;
				case T_CHAR_LIST:
					fprintf(g_tfile, " '%s'", SYM_name(TL_chars(tl)));
					break;
				default:
					fprintf(g_tfile, " %s", get_token_name(TL_tok(tl)));
				}
			}
			break;
		default:
			;ASSERT(0, ("unknown tree type:%d",TREE_type(t)));
			return;
	} //end switch
	fflush(g_tfile);
#endif
}

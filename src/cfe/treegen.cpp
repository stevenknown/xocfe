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

#define NEWT(ttt)  new_tree_node((ttt), g_real_line_num)


static TREE * statement();
static bool is_c_type_spec(TOKEN tok);
static bool is_c_type_quan(TOKEN tok);
static bool is_c_stor_spec(TOKEN tok);
static TREE * cast_exp();
static TREE * unary_exp();
static TREE * exp_stmt();
static TREE * postfix_exp();
static bool verify(TREE * t);

SMEM_POOL * g_pool_general_used = NULL;
SMEM_POOL * g_pool_st_used = NULL;
SMEM_POOL * g_pool_tree_used = NULL;
SYM_TAB * g_fe_sym_tab = NULL;
bool g_dump_token = false;
INT g_real_line_num;
CHAR * g_real_token_string = NULL;
TOKEN g_real_token = T_NUL;
static LIST<CELL*> g_cell_list;
bool g_enable_C99_declaration = true;
FILE * g_hsrc = NULL;

static void * xmalloc(ULONG size)
{
	void * p = smpool_malloc_h(size, g_pool_tree_used);
	IS_TRUE0(p);
	memset(p, 0, size);
	return p;
}


/*
Return NULL indicate we haven't found it in 'l_list', and
append 'label' to tail of the list as correct,
otherwise return 'l'.
Add a label into outmost scope of current function
*/
static LABEL_INFO * add_label(CHAR * name, INT lineno)
{
	LABEL_INFO * li;
	SCOPE * sc = g_cur_scope;
	while (sc && SCOPE_level(sc) != FUNCTION_SCOPE) {
		sc = SCOPE_parent(sc);
	}
	if (sc == NULL) {
		err(g_real_line_num, "label must be located in function");
		return NULL;
	}
	for (li = SCOPE_label_list(sc).get_head();
		 li != NULL; li = SCOPE_label_list(sc).get_next()) {
		if (strcmp(SYM_name(LABEL_INFO_name(li)), name) == 0) {
			err(g_real_line_num, "label : '%s' already defined",name);
			return NULL;
		}
	}
	li = new_clabel(g_fe_sym_tab->add(name), g_pool_general_used);
	set_map_lab2lineno(li, lineno);
	SCOPE_label_list(sc).append_tail(li);
	return li;
}


//Record a label reference into outmost scope of current function.
static LABEL_INFO * add_ref_label(CHAR * name, INT lineno)
{
	LABEL_INFO * li;
	SCOPE * sc = g_cur_scope;
	while (sc != NULL && SCOPE_level(sc) != FUNCTION_SCOPE) {
		sc = SCOPE_parent(sc);
	}
	if (sc == NULL) {
		err(g_src_line_num, "label reference illegal, are you crazy?");
		return NULL;
	}
	li = new_clabel(g_fe_sym_tab->add(name), g_pool_general_used);
	LABEL_INFO_is_used(li) = true;
	set_map_lab2lineno(li, lineno); //ONLY for debug-info or dumping
	SCOPE_ref_label_list(sc).append_tail(li);
	return li;
}


//list operation
//Append a token to tail of list
static void append_c_tail(ULONG v)
{
	//static CELL * g_cell_list_head=NULL;
	//static CELL * g_cell_list_tail=NULL;
	CELL * c = newcell(0);
	CELL_val(c) = (LONGLONG)v;
	CELL_line_no(c) = g_src_line_num;
	g_cell_list.append_tail(c);
}


//Append a token to head of list
static void append_c_head(ULONG v)
{
	CELL * c = newcell(0);
	CELL_val(c) = (LONGLONG)v;
	CELL_line_no(c) = g_src_line_num;
	g_cell_list.append_head(c);
}


//Append current token info descripte by 'g_cur_token','g_cur_token_string'
//and 'g_src_line_num'
static void append_tok_tail(TOKEN tok, CHAR * tokname, INT lineno)
{
	TOKEN_INFO * tki = (TOKEN_INFO*)xmalloc(sizeof(TOKEN_INFO));
	SYM * s = g_fe_sym_tab->add(tokname);
	TOKEN_INFO_name(tki) = SYM_name(s);
	TOKEN_INFO_token(tki) = tok;
	TOKEN_INFO_lineno(tki) = lineno;
	append_c_tail((ULONG)tki);
}


//Append current token info descripte by 'g_cur_token','g_cur_token_string'
//and 'g_src_line_num'
static void append_tok_head(TOKEN tok, CHAR * tokname, INT lineno)
{
	TOKEN_INFO * tki = (TOKEN_INFO*)xmalloc(sizeof(TOKEN_INFO));
	SYM * s = g_fe_sym_tab->add(tokname);
	TOKEN_INFO_name(tki) = SYM_name(s);
	TOKEN_INFO_token(tki) = tok;
	TOKEN_INFO_lineno(tki) = lineno;
	append_c_head((ULONG)tki);
}


//Remove a token from head of list
static ULONG remove_head_tok()
{
	CELL * c = g_cell_list.remove_head();
	if (c != NULL) {
		ULONG v = (ULONG)CELL_val(c);
		free_cell(c);
		return v;
	}
	return 0;
}


static ULONG get_head_tok()
{
	CELL * c = g_cell_list.get_head();
	if (c) {
		return (ULONG)CELL_val(c);
	}
	return 0;
}


static ULONG get_tail_tok()
{
	CELL * c = g_cell_list.get_tail();
	if (c) {
		return (ULONG)CELL_val(c);
	}
	return 0;
}


void dump_tok_list()
{
	CELL * c = g_cell_list.get_head();
	if (c) {
		scr("\nTOKEN:");
		for (; c; c = g_cell_list.get_next()) {
			CHAR * s = TOKEN_INFO_name((TOKEN_INFO*)CELL_val(c));
			scr("'%s' ", s);
		}
		scr("\n");
	}
}


bool is_in_first_set_of_declarator()
{
	if (g_real_token == T_ID) {
		DECL * ut = NULL;
		STRUCT * s;
		UNION * u;
		if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut) ||
			is_struct_exist_in_outer_scope(g_real_token_string, &s) ||
			is_union_exist_in_outer_scope(g_real_token_string, &u)) {
			return true;
		}
	} else if (is_c_type_spec(g_real_token) ||
			 is_c_type_quan(g_real_token) ||
			 is_c_stor_spec(g_real_token)) {
		return true;
	}
	return false;
}


static INT is_compound_terminal()
{
	return (g_real_token == T_RLPAREN);
}


static INT is_declarator_terminal()
{
	return (g_real_token == T_SEMI);
}


//Return true if 'tok' indicate terminal charactor, otherwise false.
bool is_in_first_set_of_exp_list(TOKEN tok)
{
	DECL * ut = NULL;
	switch (g_real_token) {
	case T_ID:
		if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut)) {
			//If there is a type-name, then it belongs to first-set of declarator.
			return false;
		} else {
			//May be identifier or enum-constant.
			return true;
		}
		break;
	case T_IMM:        //0~9
	case T_IMML:       //0~9L
	case T_IMMU:       //Unsigned
	case T_IMMUL:       //Unsigned Long
	case T_FP:         //decimal e.g 3.14
	case T_STRING:     //"abcd"
	case T_CHAR_LIST:  //'abcd'
	case T_LPAREN:     //(
	case T_ADD:        //+
	case T_SUB:        //-
	case T_ASTERISK:   //*
    case T_BITAND:     //&
	case T_NOT:        //!
	case T_REV:        //~ (reverse  a = ~a)
	case T_ADDADD:     //++
	case T_SUBSUB:     //--
	case T_SIZEOF:     //sizeof
		return true;
	default:;
	}
	return false;
}


static bool is_c_type_quan(TOKEN tok)
{
	switch (tok) {
	//scalar-type-spec
	case T_CONST:
	case T_VOLATILE:
		return true;
	default:;
	}
	return false;
}


static bool is_c_type_spec(TOKEN tok)
{
	switch (tok) {
	//scalar-type-spec
	case T_VOID:
	case T_CHAR:
	case T_SHORT:
	case T_INT:
	case T_LONGLONG:
	case T_BOOL:
	case T_LONG:
	case T_FLOAT:
	case T_DOUBLE:
	case T_SIGNED:
	case T_UNSIGNED:
	case T_STRUCT:
	case T_UNION:
	case T_ENUM:
		return true;
	default:;
	}
	return false;
}


static bool is_c_stor_spec(TOKEN tok)
{
	switch (tok) {
	case T_AUTO:
	case T_REGISTER:
	case T_EXTERN:
	case T_INLINE:
	case T_STATIC:
	case T_TYPEDEF:
		return true;
	default:;
	}
	return false;
}


static bool is_constant(TREE_TYPE tt)
{
	if (tt != TR_ENUM_CONST && tt != TR_IMM &&
		tt != TR_IMML && tt != TR_FP) {
		return false;
	}
	return true;
}


bool is_user_type_exist_in_outer_scope(CHAR * cl, OUT DECL ** ut)
{
	SCOPE * sc = g_cur_scope;
	while (sc) {
		if (is_user_type_exist(SCOPE_user_type_list(sc), cl, ut)) {
			return true;
		}
		sc = SCOPE_parent(sc);
	}
	return false;
}


INT is_user_type_exist_in_cur_scope(CHAR * cl, OUT DECL ** ut)
{
	SCOPE * sc = g_cur_scope;
	if (is_user_type_exist(SCOPE_user_type_list(sc), cl, ut)) {
		return 1;
	}
	return 0;
}


//Find if ID with named 'cl' exists and return the DECL.
static INT is_id_exist_in_outer_scope(CHAR * cl, OUT DECL ** d)
{
	return is_decl_exist_in_outer_scope(cl, d);
}


static INT is_first_set_of_unary_exp(TOKEN tok)
{
	return (tok == T_LPAREN    ||
			tok == T_ID        ||
			tok == T_IMM       ||
			tok == T_IMML      ||
			tok == T_IMMU      ||
			tok == T_IMMUL      ||
			tok == T_FP        ||
			tok == T_STRING    ||
			tok == T_CHAR_LIST ||
			tok == T_AND       ||
			tok == T_ADDADD    ||
			tok == T_SUBSUB    ||
			tok == T_ADD       ||
			tok == T_SUB       ||
			tok == T_NOT       ||
			tok == T_REV       ||
			tok == T_ASTERISK  ||
			tok == T_BITAND    ||
			tok == T_SIZEOF );
}


static INT is_assign_op(TOKEN tok)
{
 return (g_real_token == T_ASSIGN ||
		 g_real_token == T_BITANDEQU ||
		 g_real_token == T_BITOREQU  ||
         g_real_token == T_ADDEQU ||
		 g_real_token == T_SUBEQU ||
		 g_real_token == T_MULEQU ||
		 g_real_token == T_DIVEQU ||
		 g_real_token == T_XOREQU ||
		 g_real_token == T_RSHIFTEQU ||
		 g_real_token == T_LSHIFTEQU ||
		 g_real_token == T_REMEQU );
}


void set_parent(TREE * parent, TREE * child)
{
	if (child == NULL) { return; }
	while (child != NULL) {
		TREE_parent(child) = parent;
		child = TREE_nsibling(child);
	}
}


//Draw token until meeting any TOKEN list in '...'
void suck_tok_to(INT placeholder, ...)
{
    va_list arg;
	TOKEN tok;
	IS_TRUE0(sizeof(TOKEN) == sizeof(INT));
	while (1) {
		if (g_real_token == T_END || g_real_token == T_NUL) break;
		va_start(arg, placeholder);
		tok = (TOKEN)va_arg(arg, INT);
		while (tok != T_NUL) {
			if (tok == g_real_token) goto FIN;
			tok = (TOKEN)va_arg(arg, INT);
		}
		suck_tok();
	}
	va_end(arg);
FIN:
	va_end(arg);
	return ;
}


TREE * id()
{
	TREE * t = NEWT(TR_ID);
	TREE_token(t) = g_real_token;
	SYM * sym = g_fe_sym_tab->add(g_real_token_string);
	TREE_id(t) = sym;
	return t;
}


static INT gettok()
{
	get_token();
	g_real_token = g_cur_token;
	g_real_token_string = g_cur_token_string;
	g_real_line_num = g_src_line_num;
	return g_real_token;
}


static INT reset_tok()
{
	TOKEN_INFO * tki;
	if ((tki = (TOKEN_INFO*)remove_head_tok()) == NULL) {
		return g_real_token;
	}
	//Set the current token with the head element in token_list.
	g_real_token_string = TOKEN_INFO_name(tki);
	g_real_token = TOKEN_INFO_token(tki);
	g_real_line_num = TOKEN_INFO_lineno(tki);
	return g_real_token;
}


INT suck_tok()
{
	TOKEN_INFO * tki;
	if ((tki = (TOKEN_INFO*)remove_head_tok()) == NULL) {
		gettok();
	} else {
		//Set the current token with head in token-info list
		g_real_token_string = TOKEN_INFO_name(tki);
		g_real_token = TOKEN_INFO_token(tki);
		g_real_line_num = TOKEN_INFO_lineno(tki);
	}
	return ST_SUCC;
}


INT match(TOKEN tok)
{
	if (g_real_token == tok) {
		suck_tok();
	} else {
		return ST_ERR;
	}
	if (g_dump_token) {
		note("LINE:%10d, TOKEN:%s\n", g_real_line_num, g_real_token_string);
	}
	return ST_SUCC;
}


/*
Pry next 'num' token info.

'n': represent the next N token to current token.
	If n is 0, it will return current token.
*/
static TOKEN look_next_token(INT n, OUT CHAR ** tok_string,
							 OUT UINT * tok_line_num)
{
	TOKEN tok = T_NUL;
	if (n < 0) {
		return T_NUL;
	}
	if (n == 0) {
		return g_real_token;
	}
	INT count = g_cell_list.get_elem_count();
	if (count > 0) {
		//Pry in token_buffer
		if (n <= count) {
			//'n' can be finded in token-buffer
			CELL * c = g_cell_list.get_head_nth(n - 1);
			IS_TRUE0(c);
			return TOKEN_INFO_token((TOKEN_INFO*)CELL_val(c));
		} else {
			//New tokens need to be fetched into the buffer.
			n -= count;
			//Restore current token into token-buffer
			append_tok_head(g_real_token, g_real_token_string, g_real_line_num);
			while (n > 0) {
				//get new token from file
				gettok();
				tok = g_real_token;
				if (g_real_token == T_END || g_real_token == T_NUL) {
					reset_tok();
					return g_real_token;
				}
				append_tok_tail(g_real_token,
								g_real_token_string, g_real_line_num);
				n--;
			}
			reset_tok();
			return tok;
		} //end else
	} else { //count == 0
		//Fetch a number of n tokens into the buffer
		append_tok_tail(g_real_token, g_real_token_string, g_real_line_num);
		while (n > 0) {
			gettok();
			tok = g_real_token;
			if (tok_string != NULL) {
				*tok_string = g_real_token_string;
			}
			if (tok_line_num != NULL) {
				*tok_line_num = g_real_line_num;
			}
			if (g_real_token == T_END || g_real_token == T_NUL) {
				reset_tok();
				return g_real_token;
			}
			append_tok_tail(g_real_token, g_real_token_string, g_real_line_num);
			n--;
		}
		reset_tok();
		return tok;
	} //end else
	return tok;
}


/*
'num': pry the followed 'num' number of tokens.
'...': represent a token list which will to match.
*/
bool look_forward_token(INT num, ...)
{
	if (num <= 0) {
		return 0;
	}
	BYTE is_first = 1;
	va_list arg;
	va_start(arg, num);
	TOKEN v = (TOKEN)va_arg(arg, INT);
	if (num == 1) {
		va_end(arg);
		return g_real_token == v;
	}

	CELL * c = g_cell_list.get_head();
	if (c != NULL) {
		//append current real token to 'token-list'
		append_tok_head(g_real_token, g_real_token_string, g_real_line_num);

		//Restart again.
		c = g_cell_list.get_head();
		while (num > 0) {
			if (c) { //match element resided in token_list.
				TOKEN_INFO * tki = (TOKEN_INFO*)CELL_val(c);
				if (TOKEN_INFO_token(tki) != v) {
					goto UNMATCH;
				}
				c = g_cell_list.get_next();
			} else { //fetch new token to match.
				gettok();
				append_tok_tail(g_real_token, g_real_token_string,
								g_real_line_num);
				if (g_real_token != v) {
					goto UNMATCH;
				}
			}
			v = (TOKEN)va_arg(arg, INT);
			num--;
		}

	} else {
		//token_list is empty. So fetch new token to match.
		while (num > 0) {
			append_tok_tail(g_real_token, g_real_token_string, g_real_line_num);
			if (g_real_token != v) { goto UNMATCH; }
			gettok();
			v = (TOKEN)va_arg(arg, INT);
			num--;
		}
		append_tok_tail(g_real_token, g_real_token_string, g_real_line_num);
	}
	va_end(arg);
	reset_tok();
    return true;
UNMATCH:
	reset_tok();
    return false;
}


static TREE * param_list()
{
	TREE * t = exp(), * nt = NULL;
	while (g_real_token == T_COMMA) {
		match(T_COMMA);
		nt = exp();
		if (nt == NULL) {
			err(g_real_line_num, "miss patameter, syntax error : '%s'",
				g_real_token_string);
			return t;
		}
		add_tree_nsibling(&t, nt);
	}
	return t;
}


/*
Process the first part of postfix expression:
e.g:  a[10].u1.s++,  where a[10] is the first part,
	.u1.s++ is the second part.
*/
static TREE * primary_exp(IN OUT UINT * st)
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_ID:
		{
			ENUM * e = NULL;
			INT idx = 0;
			if (is_enum_const_exist_in_outer_scope(g_real_token_string,
												   &e, &idx)) {
				t = NEWT(TR_ENUM_CONST);
				TREE_enum(t) = e;
				TREE_enum_val_idx(t) = idx;
			} else {
				//STRUCT, UNION, TYPEDEF-NAME should be
				//parsed during declaration().
				DECL * dcl = NULL;
				t = id();
				if (!is_id_exist_in_outer_scope(g_real_token_string, &dcl)) {
					err(g_real_line_num, "'%s' undeclared identifier",
						g_real_token_string);
					match(T_ID);
					*st = ST_ERR;
					return NULL;
				}
				TREE_id_decl(t) = dcl;
			}
			match(T_ID);
		}
		break;
	case T_IMM:
		t = NEWT(TR_IMM);
		/*
		If the target integer hold in 'g_real_token_string' is longer than
		host ULONG type, it will be truncated now.
		*/
		TREE_token(t) = g_real_token;
		TREE_imm_val(t) = xatol(g_real_token_string, false);
		match(T_IMM);
		return t;
	case T_IMML:
		t = NEWT(TR_IMML);
		/*
		If the target integer hold in 'g_real_token_string' is longer than
		host ULONG type, it will be truncated now.
		*/
		TREE_token(t) = g_real_token;
		TREE_imm_val(t) = xatol(g_real_token_string, false);
		match(T_IMML);
		return t;
	case T_IMMU:
		t = NEWT(TR_IMM);
		TREE_token(t) = g_real_token;
		TREE_imm_val(t) = xatol(g_real_token_string, false);
		TREE_is_unsigned(t) = true;
		match(T_IMMU);
		return t;
	case T_IMMUL:
		t = NEWT(TR_IMML);
		TREE_token(t) = g_real_token;
		TREE_imm_val(t) = xatol(g_real_token_string, false);
		TREE_is_unsigned(t) = true;
		match(T_IMMUL);
		return t;
	case T_FP:         // decimal e.g 3.14
		t = NEWT(TR_FP);
		TREE_token(t) = g_real_token;
		TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
		match(T_FP);
		break;
    case T_STRING:     // "abcd"
		t = NEWT(TR_STRING);
		TREE_token(t) = g_real_token;
		TREE_string_val(t) = g_fe_sym_tab->add(g_real_token_string);
		match(T_STRING);
		return t;
	case T_CHAR_LIST:  // 'abcd'
		t = NEWT(TR_IMM);
		TREE_token(t) = g_real_token;
		TREE_imm_val(t) = xctoi(g_real_token_string);
		match(T_CHAR_LIST);
		break;
	case T_LPAREN:
		match(T_LPAREN);
		t = exp();
		if (match(T_RPAREN) != ST_SUCC) {
			err(g_real_line_num, "miss ')'");
			*st = ST_ERR;
			return NULL;
		}
		break;
	default:;
	}
	*st = ST_SUCC;
	return t;
}


/*
Process the first part of postfix expression:
e.g:  a[10].u1.s++,  where a[10] is the first part,
	.u1.s++ is the second part.
The second part include: [ ( . -> ++ --
*/
static TREE * postfix_exp_second_part(IN TREE * t)
{
AGAIN:
	switch (g_real_token) {
	case T_LSPAREN: //array reference
		{
			TREE * array_root = NULL;
			while (g_real_token == T_LSPAREN) {
				match(T_LSPAREN);
				array_root = NEWT(TR_ARRAY);
				TREE_array_base(array_root) = t;
				TREE_array_indx(array_root) = exp();
				set_parent(array_root, t);
				set_parent(array_root, TREE_array_indx(array_root));
				if (TREE_array_indx(array_root) == NULL) {
					err(g_real_line_num, "array index cannot be NULL");
					return NULL;
				}
				if (match(T_RSPAREN) != ST_SUCC) {
					err(g_real_line_num, "miss ']'");
					return NULL;
				}
				t = array_root;
			}
			if (g_real_token == T_LPAREN ||
				g_real_token == T_DOT ||
				g_real_token == T_ARROW ||
				g_real_token == T_SUBSUB ||
				g_real_token == T_ADDADD) {
				goto AGAIN;
			}
		}
		break;
	case T_LPAREN: // meet parameter list, it is function call
		{
			match(T_LPAREN);
			TREE * tp = NEWT(TR_CALL);
			TREE_fun_exp(tp) = t;
			TREE_para_list(tp) = param_list();
			if (match(T_RPAREN) != ST_SUCC) {
				err(g_real_line_num, "miss ')'");
				return t;
			}
			set_parent(tp, t);
			set_parent(tp, TREE_para_list(tp));
			t = tp;
		}
		break;
	case T_DOT: //direct struct member reference
	case T_ARROW: //indirect struct member reference
		{
			TREE * mem_ref = NULL;
			if (g_real_token == T_DOT) {
				mem_ref = NEWT(TR_DMEM);
			} else {
				mem_ref = NEWT(TR_INDMEM);
			}
			TREE_token(mem_ref) = g_real_token;
			match(g_real_token);
			TREE_base_region(mem_ref) = t;
			if (g_real_token != T_ID) {
				err(g_real_line_num, "member name is needed");
				return t;
			}
			TREE_field(mem_ref) = id();
			match(T_ID);
			set_parent(mem_ref, t);
			set_parent(mem_ref, TREE_field(mem_ref));
			t = mem_ref;
			if (g_real_token == T_LSPAREN ||
				g_real_token == T_LPAREN ||
				g_real_token == T_DOT ||
				g_real_token == T_ARROW ||
				g_real_token == T_SUBSUB ||
				g_real_token == T_ADDADD) {
				goto AGAIN;
			}
		}
		break;
	case T_ADDADD: //post incease
		{
			match(T_ADDADD);
			TREE * tp = NEWT(TR_POST_INC);
			TREE_token(tp) = g_real_token;
			TREE_inc_exp(tp) = t;
			set_parent(tp, t);
			t = tp;
			if (t == NULL) {
				err(g_real_line_num, "unary expression is needed");
				return t;
			}
			if (g_real_token == T_LSPAREN) {
				goto AGAIN;
			}
			if (g_real_token == T_ADDADD) {
				err(g_real_line_num, "'++' needs l-value");
				match(T_ADDADD);
				return t;
			}
			if (g_real_token == T_SUBSUB) {
				err(g_real_line_num, "'--' needs l-value");
				match(T_SUBSUB);
				return t;
			}
		}
		break;
	case T_SUBSUB: //post decrease
		{
			match(T_SUBSUB);
			TREE * tp = NEWT(TR_POST_DEC);
			TREE_token(tp) = g_real_token;
			TREE_dec_exp(tp) = t;
			set_parent(tp, t);
			t = tp;
			if (t == NULL) {
				err(g_real_line_num, "unary expression is needed");
				return t;
			}
			if (g_real_token == T_LSPAREN) {
				goto AGAIN;
			}
			if (g_real_token == T_ADDADD) {
				err(g_real_line_num, "'++' needs l-value");
				match(T_ADDADD);
				return t;
			}
			if (g_real_token == T_SUBSUB) {
				err(g_real_line_num, "'--' needs l-value");
				match(T_SUBSUB);
				return t;
			}
		}
		break;
	default:;
	}
	return t;
}


/*
Process the first part of postfix expression:
e.g:  a[10].u1.s++,  where a[10] is the first part,
	.u1.s++ is the second part.
*/
static TREE * postfix_exp()
{
	UINT st;
	TREE * t = primary_exp(&st);
	if (st != ST_SUCC) {
		return t;
	}
	return postfix_exp_second_part(t);
}


/*
Whether input lexicalgraphic words can be descripted with
	unary-expression   or
    ( type-name )

NOTE: C language only permit 'type-name' be enclosed by one pair of '()'.
	e.g: ((char*)) is illegal syntax.
*/
static TREE * unary_or_LP_typename_RP()
{
	TREE * t = NULL;
	TOKEN tok = T_NUL;
	CHAR * tok_string = NULL;
	if (g_real_token == T_LPAREN) {
		//Next exp either (exp) or (type_name), so after several '(', there
	  	//must be T_ID appearing .
		tok = look_next_token(1, &tok_string, NULL);
		if (is_c_type_spec(tok) ||
			is_c_type_quan(tok) ||
			is_c_stor_spec(tok)) {
			t = NEWT(TR_TYPE_NAME);
			if (match(T_LPAREN) != ST_SUCC) {
				err(g_real_line_num, "except '('");
				goto FAILED;
			}
			TREE_type_name(t) = type_name();
			if (match(T_RPAREN) != ST_SUCC) {
				err(g_real_line_num, "except ')'");
				goto FAILED;
			}
			if (TREE_type_name(t) == NULL) {
				err(g_real_line_num, "except 'type-name'");
				goto FAILED;
			}
		} else if (tok == T_ID) {
			//Record a User defined type
			DECL * ut = NULL;
			IS_TRUE0(tok_string != NULL);
			if (is_user_type_exist_in_outer_scope(tok_string, &ut)) {
				//User defined TYPE via 'typedef'.
				t = NEWT(TR_TYPE_NAME);
				if (match(T_LPAREN) != ST_SUCC) {
					err(g_real_line_num, "except '('");
					goto FAILED;
				}
				//Reference a type to do type-cast.
				TREE_type_name(t) = type_name();
				if (match(T_RPAREN) != ST_SUCC) {
					err(g_real_line_num, "except ')'");
					goto FAILED;
				}
				if (TREE_type_name(t) == NULL) {
					err(g_real_line_num, "except 'type-name'");
					goto FAILED;
				}
			} else {
				//It's a general identifier, may be a enum-const or id.
				t = unary_exp();
			}
		} else {
        //It's a general token, so need to pry continue, let's go!
			t = unary_exp();
		}
	} else if (is_first_set_of_unary_exp(g_real_token)) {
		t = unary_exp();
	}
	return t;
FAILED:
	return t;
}

/*
unary_operator:  one of
	&  *  +  -  ~  !

unary_expression:
	postfix_expression
	++ unary_expression
	-- unary_expression
	(&  *  +  -  ~  !) cast_expression
	sizeof unary_expression
	sizeof ( type_name )

*/
static TREE * unary_exp()
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_IMM:
	case T_IMML:
	case T_IMMU:
	case T_IMMUL:
	case T_FP:         // decimal e.g 3.14
	case T_STRING:     // "abcd"
	case T_CHAR_LIST:  // 'abcd'
	case T_LPAREN: //(exp)
	case T_ID:
		t = postfix_exp();
		break;
	case T_ASTERISK:
		t = NEWT(TR_DEREF);
		TREE_token(t) = g_real_token;
		match(T_ASTERISK);
		TREE_lchild(t) = cast_exp();
		set_parent(t, TREE_lchild(t));
		break;
	case T_BITAND:
		t = NEWT(TR_LDA);
		TREE_token(t) = g_real_token;
		match(T_BITAND);
		TREE_lchild(t) = cast_exp();
		set_parent(t, TREE_lchild(t));
		break;
	case T_ADDADD:
		t = NEWT(TR_INC);
		TREE_token(t) = g_real_token;
		match(T_ADDADD);
		TREE_inc_exp(t) = unary_exp();
		set_parent(t, TREE_inc_exp(t));
		if (TREE_inc_exp(t) == NULL) {
			err(g_real_line_num, "unary expression is needed");
			goto FAILED;
		}
		break;
	case T_SUBSUB:
		t = NEWT(TR_DEC);
		TREE_token(t) = g_real_token;
		match(T_SUBSUB);
		TREE_dec_exp(t) = unary_exp();
		set_parent(t,TREE_dec_exp(t));
		if (TREE_inc_exp(t) == NULL) {
			err(g_real_line_num, "unary expression is needed");
			goto FAILED;
		}
		break;
	case T_ADD:
		t = NEWT(TR_PLUS);
		TREE_token(t) = g_real_token;
		match(T_ADD);
		TREE_lchild(t) = cast_exp();
		set_parent(t,TREE_lchild(t));
		if (TREE_lchild(t) == NULL) {
			err(g_real_line_num, "cast expression is needed");
			goto FAILED;
		}
		break;
	case T_SUB:
		t = NEWT(TR_MINUS);
		TREE_token(t) = g_real_token;
		match(T_SUB);
		TREE_lchild(t) = cast_exp();
		set_parent(t,TREE_lchild(t));
		if (TREE_lchild(t) == NULL) {
			err(g_real_line_num, "cast expression is needed");
			goto FAILED;
		}
		break;
	case T_SIZEOF:
		t = NEWT(TR_SIZEOF);
		TREE_token(t) = g_real_token;
		match(T_SIZEOF);
		TREE_sizeof_exp(t) = unary_or_LP_typename_RP();
		set_parent(t, TREE_sizeof_exp(t));
		break;
	case T_REV:
		t = NEWT(TR_REV);
		TREE_token(t) = g_real_token;
		match(T_REV);
		TREE_lchild(t) = cast_exp();
		set_parent(t, TREE_lchild(t));
		if (TREE_lchild(t) == NULL) {
			err(g_real_line_num, "cast expression is needed");
			goto FAILED;
		}
		break;
	case T_NOT:
		t = NEWT(TR_NOT);
		TREE_token(t) = g_real_token;
		match(T_NOT);
		TREE_lchild(t) = cast_exp();
		set_parent(t,TREE_lchild(t));
		if (TREE_lchild(t) == NULL) {
			err(g_real_line_num, "cast expression is needed");
			goto FAILED;
		}
		break;
	default:;
	}
	return t ;
FAILED:
    scr("error in unary_exp()");
	return t;
}


/*
BNF:
cast_expression:
	unary_expression
	( type_name ) cast_expression

FIRST_SET:
cast_exp :
	id imm imml floatpoint string charlist ( + -
	* & ! ~ ++ -- sizeof

*/
static TREE * cast_exp()
{
	TREE * t = NULL, * p;
	p = unary_or_LP_typename_RP();
	if (p == NULL) return NULL;

	/*
	It might be follow-set token.
	if (p == NULL) {
		err(g_real_line_num,
			"syntax error : '%s' , except a identifier or typedef-name",
			g_real_token_string);
		goto FAILED;
	}
	*/

	if (TREE_type(p) == TR_TYPE_NAME) {
		t = NEWT(TR_CVT);
		TREE_cvt_type(t) = p;
		TREE_cast_exp(t) = cast_exp();
		set_parent(t, TREE_cvt_type(t));
		set_parent(t, TREE_cast_exp(t));
		if (TREE_cast_exp(t) == NULL) {
			err(g_real_line_num, "cast expression cannot be NULL");
			goto FAILED;
		}
	} else {
		t = p;
	}
	return t ;
FAILED:
    scr("error in cast_exp()");
	return t;
}


static TREE * multiplicative_exp()
{
	TREE * t = cast_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_ASTERISK ||
		   g_real_token == T_DIV ||
		   g_real_token == T_MOD) {
		p = NEWT(TR_MULTI);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
			/*
		     a*b*c  =>
			             *
			            / \
                       *   c
				      / \
					 a   b
			*/
		match(g_real_token);
		TREE_rchild(p) = cast_exp();
		set_parent(p,TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in multiplicative_exp()");
	return t;
}


static TREE * additive_exp()
{
	TREE * t = multiplicative_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_ADD || g_real_token == T_SUB) {
		p = NEWT(TR_ADDITIVE);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p, TREE_lchild(p));
		   /*
		     a+b-c  =>
			             -
			            / \
                       +   c
				      / \
					 a   b
		   */
		match(g_real_token);
		TREE_rchild(p) = multiplicative_exp();
		set_parent(p, TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in additive_exp()");
	return t;
}


static TREE * shift_exp()
{
	TREE * t = additive_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_LSHIFT || g_real_token == T_RSHIFT) {
		p = NEWT(TR_SHIFT);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		   /*
		     a<<b<<c  =>
			             <<
			            /  \
                       <<   c
				      /  \
					 a    b
		   */
		match(g_real_token);
		TREE_rchild(p) = additive_exp();
		set_parent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in shift_exp()");
	return t;
}


static TREE * relational_exp()
{
	TREE * t = shift_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_LESSTHAN || g_real_token == T_MORETHAN ||
		   g_real_token == T_NOMORETHAN || g_real_token == T_NOLESSTHAN) {
		p = NEWT(TR_RELATION);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		   /*
		     a<b<c   =>
			             <
			            / \
                       <   c
				      / \
					 a   b
		   */
		match(g_real_token);
		TREE_rchild(p) = shift_exp();
		set_parent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in relational_exp()");
	return t;
}


static TREE * equality_exp()
{
	TREE * t = relational_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_EQU || g_real_token == T_NOEQU) {
		p = NEWT(TR_EQUALITY);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p, TREE_lchild(p));
		   /*
		     a==b!=c   =>
			             !=
			            /  \
                       ==   c
				      /  \
					 a    b
		   */
		match(g_real_token);
		TREE_rchild(p) = relational_exp();
		set_parent(p, TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in equality_exp()");
	return t;
}


static TREE * AND_exp()
{
	TREE * t = equality_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_BITAND) {
		p = NEWT(TR_INCLUSIVE_AND);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p, TREE_lchild(p));
		   /*
		     a&b&c   =>  &
			            / \
                       &   c
				      / \
					 a   b
		   */
		match(T_BITAND);
		TREE_rchild(p) = equality_exp();
		set_parent(p,TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in AND_exp()");
	return t;
}


static TREE * exclusive_OR_exp()
{
	TREE * t = AND_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_XOR) {
		p = NEWT(TR_XOR);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		   /*
		     a^b^c   =>  ^
			            / \
                       ^   c
				      / \
					 a   b
		   */
		match(T_XOR);
		TREE_rchild(p) = AND_exp();
		set_parent(p,TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in exclusive_OR_exp()");
	return t;
}


static TREE * inclusive_OR_exp()
{
	TREE * t = exclusive_OR_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_BITOR) {
		p = NEWT(TR_INCLUSIVE_OR);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		   /*
		     a|b|c   =>  |
			            / \
                       |   c
				      / \
					 a   b
		   */
		match(T_BITOR);
		TREE_rchild(p) = exclusive_OR_exp();
		set_parent(p,TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in inclusive_OR_exp()");
	return t;
}


static TREE * logical_AND_exp()
{
	TREE * t = inclusive_OR_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_AND) {
		p = NEWT(TR_LOGIC_AND);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		   /*
		     a && b && c =>
			               &&
			              /  \
                         &&   c
					    /  \
					    a   b
		   */
		match(T_AND);
		TREE_rchild(p) = inclusive_OR_exp();
		set_parent(p,TREE_rchild(p));
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
		}
		t = p;
	}//end while
	return t ;
FAILED:
    scr("error in logical_AND_exp()");
	return t;
}

static TREE * logical_OR_exp()
{
	TREE * t = logical_AND_exp(), * p = NULL;
	if (t == NULL) return NULL;
	while (g_real_token == T_OR) {
		p = NEWT(TR_LOGIC_OR);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p,TREE_lchild(p));
		/*
		  a||b||c  =>  ||
		              /  \
    	             ||   c
				    /  \
				   a   b
		*/
		match(T_OR);
		TREE_rchild(p) = logical_AND_exp();
		set_parent(p,TREE_rchild(p));
    	if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "'%s': right operand cannot be NULL",
				TOKEN_INFO_name(&g_token_info[TREE_token(p)]));
			goto FAILED;
    	}
		t = p;
	} //end while
	return t ;
FAILED:
    scr("error in logical_OR_exp()");
	return t;
}


//logical_OR_expression ? expression : conditional_expression
TREE * conditional_exp()
{
	TREE * t = logical_OR_exp();
	if (g_real_token == T_QUES_MARK) {
		match(T_QUES_MARK);
		TREE * p = NEWT(TR_COND);
		TREE_det(p) = t;
		set_parent(p,t);
		t = p;

		TREE_true_part(t) = exp_list();
		set_parent(t,TREE_true_part(t));
		if (match(T_COLON) != ST_SUCC) {
			err(g_real_line_num, "condition expression is incomplete");
			goto FAILED;
		}
		TREE_false_part(t) = exp_list();
		set_parent(t,TREE_false_part(t));
	}
	return t ;
FAILED:
    scr("error in conditional_expt()");
	return t;
}


TREE * exp()
{
	TREE * t = conditional_exp(), * p;
	if (t == NULL) { return t; }
	if (is_assign_op(g_real_token)) {
		p = NEWT(TR_ASSIGN);
		TREE_token(p) = g_real_token;
		TREE_lchild(p) = t;
		set_parent(p, TREE_lchild(p));
        match(g_real_token);
		TREE_rchild(p) = exp();
		if (TREE_rchild(p) == NULL) {
			err(g_real_line_num, "expression miss r-value");
			goto FAILED;
		}
		set_parent(p, TREE_rchild(p));
  		t = p ;
	}
	return t;
FAILED:
    scr("error in expt()");
	return t;
}


TREE * exp_list()
{
	TREE * t = exp(), * nt = NULL;
	while (g_real_token == T_COMMA) {
		match(T_COMMA);
		nt = exp();
		add_tree_nsibling(&t, nt);
	}
	return t;
}


/*
jump_statement:
	goto identifier;
	continue;
	break;
	return 	         ;
	return expression;
*/
static TREE * jump_stmt()
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_GOTO:
		t = NEWT(TR_GOTO);
		TREE_token(t) = g_real_token;
		match(T_GOTO);
		if (g_real_token != T_ID) {
			err(g_real_line_num, "target address label needed for 'goto'");
			return t;
		}
		TREE_lab_info(t) = add_ref_label(g_real_token_string, g_real_line_num);
		match(T_ID);
		if (match(T_SEMI) != ST_SUCC) {
			err(g_real_line_num, "miss ';'");
			return t;
		}
		break;
	case T_BREAK:
		if (!is_sst_exist(st_DO) &&
		   !is_sst_exist(st_WHILE) &&
		   !is_sst_exist(st_FOR) &&
		   !is_sst_exist(st_SWITCH)) {
			err(g_real_line_num, "invalid use 'break'");
			return t;
		}
		t = NEWT(TR_BREAK);
		TREE_token(t) = g_real_token;
		match(T_BREAK);
		if (match(T_SEMI) != ST_SUCC) {
			err(g_real_line_num, "miss ';'");
			return t;
		}
		break;
	case T_RETURN:
		t = NEWT(TR_RETURN);
		TREE_token(t) = g_real_token;
		match(T_RETURN);
		if (g_real_token != T_SEMI) {
			TREE_ret_exp(t) = exp_list();
			set_parent(t,TREE_ret_exp(t));
		}
		if (match(T_SEMI) != ST_SUCC) {
			err(g_real_line_num, "miss ';'");
			return t;
		}
		break;
	case T_CONTINUE:
		if (!is_sst_exist(st_DO) &&
		   !is_sst_exist(st_WHILE) &&
		   !is_sst_exist(st_FOR) &&
		   !is_sst_exist(st_SWITCH)) {
			err(g_real_line_num, "invalid use 'continue'");
			return t;
		}
		t = NEWT(TR_CONTINUE);
		TREE_token(t) = g_real_token;
		match(T_CONTINUE);
		if (match(T_SEMI) != ST_SUCC) {
			err(g_real_line_num, "miss ';'");
			return t;
		}
		break;
	default:;
	}
	return t;
}


static TREE * sharp_start_stmt()
{
	TREE * t = NEWT(TR_PRAGMA);
	TREE_token(t) = g_real_token;
	IS_TRUE0(g_real_token == T_SHARP);
	match(T_SHARP);
	if (g_real_token != T_PRAGMA) {
		err(g_real_line_num,
			"illegal use '#', its followed keyword should be 'pragma'");
		return NULL;
	}
	match(T_PRAGMA);

	TOKEN_LIST * last = NULL;
	g_enable_newline_token = true;
	while (g_real_token != T_NEWLINE && g_real_token != T_NUL) {
		TOKEN_LIST * tl = (TOKEN_LIST*)xmalloc(sizeof(TOKEN_LIST));
		TL_tok(tl) = g_real_token;
		switch (g_real_token) {
		case T_ID:
			TL_id_name(tl) = g_fe_sym_tab->add(g_real_token_string);
			break;
		case T_STRING:
			TL_str(tl) = g_fe_sym_tab->add(g_real_token_string);
			break;
		case T_CHAR_LIST:
			TL_chars(tl) = g_fe_sym_tab->add(g_real_token_string);
			break;
		default: break;
		}
		add_next(&TREE_pragma_tok_lst(t), &last, tl);
		if (match(g_real_token) == ST_ERR) {
			break;
		}
	}
	g_enable_newline_token = false;
	if (g_real_token == T_NEWLINE) {
		match(T_NEWLINE);
	}
	return t;
}


static TREE * label_stmt()
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_ID:
		t = NEWT(TR_LABEL);
		TREE_token(t) = g_real_token;
		if ((TREE_lab_info(t) = add_label(g_real_token_string,
										  g_real_line_num)) == NULL) {
			err(g_real_line_num, "illegal label '%s' defined",
				g_real_token_string);
			return t;
		}
		match(T_ID);
		if (match(T_COLON) != ST_SUCC) {
			err(g_real_line_num, "label defined incompletely");
			return t;
		}
		break;
	case T_CASE:
		{
			LONGLONG idx = 0;
			TREE * nt = NULL;
			match(T_CASE);
			if (!is_sst_exist(st_DO) &&
			   !is_sst_exist(st_WHILE) &&
			   !is_sst_exist(st_FOR) &&
			   !is_sst_exist(st_SWITCH)) {
				err(g_real_line_num, "invalid use 'case'");
				return t;
			}
			t = NEWT(TR_CASE);
			TREE_token(t) = g_real_token;
			nt = postfix_exp(); //case's constant value
			if (!compute_constant_exp(nt, &idx, 0)) {
				err(g_real_line_num, "expected constant expression");
				return t;
			}
			if (get_const_bit_len(idx) >
				(sizeof(TREE_case_value(t)) * HOST_BITS_PER_BYTE)) {
				err(g_real_line_num, "bitsize of const is more than %dbit",
					(sizeof(TREE_case_value(t)) * HOST_BITS_PER_BYTE));
				return t;
			}
			TREE_case_value(t) = (INT)idx;
			if (match(T_COLON) != ST_SUCC) {
				err(g_real_line_num, "miss ':' before '%s'",
					g_real_token_string);
				return t;
			}
		}
		break;
	case T_DEFAULT:
		{
			t = NEWT(TR_DEFAULT);
			TREE_token(t) = g_real_token;

			match(T_DEFAULT);
			if (!is_sst_exist(st_DO) &&
			   !is_sst_exist(st_WHILE) &&
			   !is_sst_exist(st_FOR) &&
			   !is_sst_exist(st_SWITCH)) {
				err(g_real_line_num, "invalid use 'default'");
				return t;
			}
			if (match(T_COLON) != ST_SUCC) {
				err(g_real_line_num, "miss ':' before '%s'",
					g_real_token_string);
				return t;
			}
		}
		break;
	default:;
	}
	return t;
}


/*
{
do-body
}
while (determination)
*/
static TREE * do_while_stmt()
{
	TREE * t = NEWT(TR_DO);
	TREE_token(t) = g_real_token;
	match(T_DO);

	//do-body
	pushst(st_DO,0); //push down inherit properties
	TREE_dowhile_body(t) = statement();
	set_parent(t, TREE_dowhile_body(t));
	popst();

	if (match(T_WHILE) != ST_SUCC) { //while
		err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
		goto FAILED;
	}

	//determination
	if (match(T_LPAREN) != ST_SUCC) { //(
		err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
		goto FAILED;
	}
	TREE_dowhile_det(t) =  exp_list();
	set_parent(t,TREE_dowhile_det(t));
	if (TREE_dowhile_det(t) == NULL) {
		err(g_real_line_num, "while determination cannot be NULL");
		goto FAILED;
	}
	if (match(T_RPAREN) != ST_SUCC) { //)
		err(g_real_line_num, "miss ')'");
		goto FAILED;
	}

	if (g_real_token!=T_SEMI ) { //;
		err(g_real_line_num, "miss ';' after 'while'");
		goto FAILED;
	}
	return t;
FAILED:
	scr("error in do_while_stmt()");
	return t;

}


static TREE * while_do_stmt()
{
	TREE * t = NEWT(TR_WHILE);
	TREE_token(t) = g_real_token;
	match(T_WHILE);

	//determination
	if (match(T_LPAREN) != ST_SUCC) { //(
		err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
		goto FAILED;
	}
	TREE_whiledo_det(t) =  exp_list();
	set_parent(t,TREE_whiledo_det(t));
	if (TREE_whiledo_det(t) == NULL) {
		err(g_real_line_num, "while determination cannot be NULL");
		goto FAILED;
	}
	if (match(T_RPAREN) != ST_SUCC) { //)
		err(g_real_line_num, "miss ')'");
		goto FAILED;
	}

	//while-body
	pushst(st_WHILE,0); //push down inherit properties
    TREE_whiledo_body(t) = statement();
	set_parent(t,TREE_whiledo_body(t));
	popst();
	return t;
FAILED:
	scr("error in while_do_stmt()");
	return t;
}


TREE * for_stmt()
{
	TREE * t = NEWT(TR_FOR);
	TREE_token(t) = g_real_token;
 	match(T_FOR);
 	if (match(T_LPAREN) != ST_SUCC) {
		err(g_real_line_num, "synatx error : '%s', need '('",
			g_real_token_string);
		return t;;
	}

	//C89 specified init-variable declared in init-field of FOR is belong
	//to function level scope.
	if (g_enable_C99_declaration) {
		enter_sub_scope(false);
	}
	if (!declaration_list()) {
		//initializing expression
		TREE_for_init(t) = exp_list();
		set_parent(t, TREE_for_init(t));

		//EXPRESSION does not swallow the token ';'.
		if (match(T_SEMI) != ST_SUCC) {
			err(g_real_line_num, "miss ';' before determination");
			goto FAILED;
		}
	}

	TREE_for_det(t) = exp_list();
	set_parent(t,TREE_for_det(t));
	//EXPRESSION does not swallow the token ';'.
	if (match(T_SEMI) != ST_SUCC) {
		err(g_real_line_num, "miss ';' before step expression");
		goto FAILED;
	}

	TREE_for_step(t) = exp_list();
	set_parent(t, TREE_for_step(t));
	//EXPRESSION does not swallow the token ')'.
	if (match(T_RPAREN) != ST_SUCC) {
		err(g_real_line_num, "miss ')' after step expression");
		goto FAILED;
	}

	pushst(st_DO, 0); //push down inherit properties
	TREE_for_body(t) = statement();
	set_parent(t, TREE_for_body(t));
	popst();
	if (g_enable_C99_declaration) {
		return_to_parent_scope();
	}

	return t;
FAILED:
	if (g_enable_C99_declaration) {
		return_to_parent_scope();
	}
	scr("error in for_stmt()");
	return t;
}


static TREE * iter_stmt()
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_DO:
		t = do_while_stmt();
		break;
	case T_WHILE:
		t = while_do_stmt();
		break;
	case T_FOR:
		t = for_stmt();
		break;
	default:;
	}
	return t;
}


static TREE * if_stmt()
{
	TREE * t = NEWT(TR_IF);
	TREE_token(t) = g_real_token;
	match(T_IF);

	//determination
	if (g_real_token != T_LPAREN) { //(
		err(g_real_line_num,
			"syntax error : '%s', determination must enclosed by '(')'",
			g_real_token_string);
		goto FAILED;
	}
	match(T_LPAREN);

	TREE_if_det(t) = exp_list();
	set_parent(t, TREE_if_det(t));
	if (TREE_if_det(t) == NULL) {
		err(g_real_line_num, "'if' determination cannot be NULL");
		goto FAILED;
	}

	if (match(T_RPAREN) != ST_SUCC) { // )
		err(g_real_line_num, "miss ')'");
		goto FAILED;
	}

    //true part
	TREE_if_true_stmt(t) = statement();
	set_parent(t,TREE_if_true_stmt(t));

	if (g_real_token == T_ELSE) {
		match(T_ELSE);
		TREE_if_false_stmt(t) = statement();
		set_parent(t,TREE_if_false_stmt(t));
	}
	return t;
FAILED:
	scr("error in if_stmt()");
	return t;
}


static TREE * switch_stmt()
{
	TREE * t = NEWT(TR_SWITCH);
	TREE_token(t) = g_real_token;
	match(T_SWITCH);

	//determination
	if (match(T_LPAREN)  != ST_SUCC) { //(
		err(g_real_line_num,
			"syntax error : '%s', need '('", g_real_token_string);
		goto FAILED;
	}
	TREE_switch_det(t) = exp_list();
	set_parent(t, TREE_switch_det(t));
	if (TREE_switch_det(t) == NULL) {
		err(g_real_line_num, "switch determination cannot be NULL");
		goto FAILED;
	}
	if (match(T_RPAREN) != ST_SUCC) { // )
		err(g_real_line_num, "miss ')'");
		goto FAILED;
	}

	pushst(st_DO,0); //push down inherit properties
	TREE_switch_body(t) = statement();
	set_parent(t,TREE_switch_body(t));
	popst();
	return t;
FAILED:
	scr("error in switch_stmt()");
	return t;
}


static TREE * select_stmt()
{
	TREE * t = NULL;
	switch (g_real_token) {
	case T_IF:
		t = if_stmt();
		break;
	case T_SWITCH:
		t = switch_stmt();
		break;
	default:;
	}
	return t;
}


SCOPE * compound_stmt(DECL * para_list)
{
	TREE * t = NULL;
	SCOPE * s = NULL;
	INT cerr = 0;
	//enter a new sub-scope region
	SCOPE * cur_scope = enter_sub_scope(false);

	/*
	Convert ARRAY of ID to ARRAY of POINTER.
	In C, the parameter of function that declarated as 'int a[10]' is not
	really array of a, but is a pointer that pointed to array 'a', and
	should be 'int (*a)[10].
		e.g: Convert 'void f(int a[10])' -> 'void f(int (*a)[10])'
	*/
	while (para_list != NULL) {
		if (DECL_dt(para_list) != DCL_VARIABLE) {
			//append parameter list to declaration list of function body scope
			DECL * declaration = new_decl(DCL_DECLARATION);
			DECL_spec(declaration) = DECL_spec(para_list);
			DECL_decl_list(declaration) = cp_decl(DECL_decl_list(para_list));
			PURE_DECL(declaration) = PURE_DECL(para_list);

			//Array type formal parameter is always be treated as pointer type.
			if (is_array(declaration)) {
				declaration = trans_to_pointer(declaration, true);
				//dump_decl(declaration);
				//dump_decl(declaration, 0);
			}
			DECL_is_formal_para(declaration) = true;
			add_next(&SCOPE_decl_list(cur_scope), declaration);
			DECL_decl_scope(declaration) = cur_scope;

			//Append parameter list to symbol list of function body scope.
			SYM * sym = get_decl_sym(declaration);
			if (add_to_symtab_list(&SCOPE_sym_tab_list(cur_scope), sym)) {
				err(g_real_line_num, "'%s' already defined",
					g_real_token_string);
				goto FAILED;
			}
		}
		para_list = DECL_next(para_list);
	}
	match(T_LLPAREN);
	s = cur_scope;
	declaration_list();

	//statement list
	cerr = g_err_msg_list.get_elem_count();
	while (1) {
		if (g_real_token == T_END) {
			break;
		} else if (g_real_token == T_NUL) {
			break;
		} else if (g_err_msg_list.get_elem_count() >= TOO_MANY_ERR) {
			goto FAILED;
		} else if (is_compound_terminal()) {
			break;
		}
		t = statement();
		verify(t);
		add_tree_nsibling(&SCOPE_stmt_list(cur_scope), t);
		if ((cerr != (INT)g_err_msg_list.get_elem_count()) ||
			(cerr > 0 && t == NULL)) {
			suck_tok_to(0, T_SEMI, T_RLPAREN, T_END, T_NUL);
		}
		if (g_real_token == T_SEMI) {
			match(T_SEMI);
		}
		cerr = g_err_msg_list.get_elem_count();
	}
	if (match(T_RLPAREN) != ST_SUCC) {
		err(g_real_line_num, "miss '}'");
		goto FAILED;
	}

	//back to outer region
	return_to_parent_scope();
	return s;
FAILED:
	match(T_RLPAREN);
	return_to_parent_scope();
	return s;
}


/*
expression_statement:
	NULL
	expression;
*/
static TREE * exp_stmt()
{
	TREE * t = exp_list();
	//expression can be NULL
	if (match(T_SEMI) != ST_SUCC) {
		err(g_real_line_num, "syntax error : '%s', expected ';' be followed",
			g_real_token_string);
	}
	return t;
}


/*
statement:
	labeled_statement // this already be recog in dispatch()
	expression_list;
	compound_statement
	selection_statement
	iteration_statement
	jump_statement
*/
static TREE * statement()
{
	TREE * t = NULL;
	if (look_forward_token(2, T_ID, T_COLON)) {
		//current token is 'ID', and next is ':'
		t = label_stmt();
	} else if (is_in_first_set_of_exp_list(g_real_token)) {
		t = exp_stmt();
	} else {
		switch (g_real_token) {
		case T_CASE:
		case T_DEFAULT:
			t = label_stmt();
			break;
		case T_LLPAREN:
			t = NEWT(TR_SCOPE);
			TREE_scope(t) = compound_stmt(NULL);
			break;
		case T_IF:
		case T_SWITCH:
			t = select_stmt();
			break;
		case T_DO:
		case T_WHILE:
		case T_FOR:
			t = iter_stmt();
			break;
		case T_GOTO:
		case T_BREAK:
		case T_RETURN:
		case T_CONTINUE:
			t = jump_stmt();
			break;
		case T_SEMI: //null statement
			match(T_SEMI);
			break;
		case T_SHARP:
			t = sharp_start_stmt();
			break;
		default:
			if (is_c_type_quan(g_real_token) ||
				is_c_type_spec(g_real_token) ||
				is_c_stor_spec(g_real_token)) {
				/*
				err(g_real_line_num,
					"'%s' is out of definition after or before block",
					g_real_token_string);
				*/
				declaration_list(); //Supported define variables anywhere.
			} else {
				err(g_real_line_num,
					"syntax error : illegal used '%s'", g_real_token_string);
			}
		}
	}
	return t;
}


//Verify the TREE node legality.
static bool verify(TREE * t)
{
	if (t == NULL)
		return true;

	switch (TREE_type(t)) {
	case TR_SCOPE:
	case TR_CALL:
	case TR_ARRAY:
	case TR_TYPE_NAME:
	case TR_CVT:
	case TR_COND:
	case TR_LABEL:
		break;
	default:
		CHAR * name = get_token_name(TREE_token(t));
		IS_TRUE0(TREE_token(t) != T_NUL && name != NULL);
	}

	switch (TREE_type(t)) {
	case TR_ASSIGN:
	case TR_ID:
	case TR_IMM:
	case TR_IMML:
	case TR_FP:  //3.1415926
	case TR_ENUM_CONST:
	case TR_STRING:
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
	case TR_SCOPE:
	case TR_IF:
	case TR_DO:
	case TR_WHILE:
	case TR_FOR:
	case TR_SWITCH:
	case TR_BREAK:
	case TR_CONTINUE:
	case TR_RETURN:
	case TR_GOTO:
	case TR_LABEL:
	case TR_CASE:
	case TR_DEFAULT:
	case TR_COND: //formulized log_OR_exp?exp:cond_exp
	case TR_CVT: //type convertion
	case TR_LDA:   // &a get address of 'a'
	case TR_DEREF: //*p  dereferencing the pointer 'p'
	case TR_PLUS: // +123
	case TR_MINUS:  // -123
	case TR_REV:  // Reverse
	case TR_NOT:  // get non-value
	case TR_INC:   //++a
	case TR_DEC:   //--a
	case TR_POST_INC: //a++  / (*a)++
	case TR_POST_DEC: //a--
	case TR_DMEM:   //
	case TR_INDMEM:
	case TR_ARRAY:
	case TR_CALL:
	case TR_PRAGMA:
		break;
	default: IS_TRUE(0, ("unknown tree type:%d", TREE_type(t)));
	} //end switch
	return true;
}


//Top level dispatch to parse DECLARATION or PLAUSE.
static TREE * dispatch()
{
	ENUM * e = NULL;
	INT idx = -1;
	DECL * ut = NULL;
	SYM * sym = NULL;
	TREE * t = NULL;
	switch (g_real_token) {
	case T_ID: // ID = (A-Z|a-z)( A-Z|a-z|0-9 )*
		/*
		Here we cannot determined which non-terminal-charactor the T_ID
		should be reduced correctly . Because T_ID could be reduced by
		followed grammar :
		  enumerator:
					identifier(T_ID)

	      direct-declarator:
					identifier(T_ID)

	      identifier-list:
					identifier(T_ID)

		  typedef-name:
					identifier(T_ID)

		  postfix-expression:
					identifier(T_ID)

		So all the first 4 cases are decl or type, only the last is ID
		or enumerator used, and the last case should check whether the
		ID has declared.

		We do NOT support such ambiguous C syntax:
			e.g: funtion return value type.
				foo() { ... }
			it is equal to
				int foo() { ... }
			This enforce parser has to look ahead many tokens.
			In order to keep parser succinct, we only
			support the latter, int foo() { ... }.
		*/
		if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut)) {
			//reduce to variable declaration
			declaration();
		} else if (is_enum_const_exist_in_outer_scope(g_real_token_string,
													  &e, &idx)) {
			t = exp_stmt();
		} else {
			// effective method
			if (look_forward_token(2, T_ID, T_COLON)) {
				t = label_stmt();
			} else {
				t = exp_stmt();
			}
		}
		break;
	case T_INTRI_FUN: //intrinsic function call
	case T_INTRI_VAL: //intrinsic value
		err(g_real_line_num, "unsupport INTRI_FUN and INTRI_VAL");
		break;
	case T_IMM:
	case T_IMML:
	case T_IMMU:
	case T_IMMUL:
	case T_FP:         //decimal e.g 3.14
	case T_STRING:     //"abcd"
	case T_CHAR_LIST:  //'abcd'
	case T_LPAREN:     //(
	case T_ADD:        //+
	case T_SUB:        //-
	case T_ASTERISK:   //*
    case T_BITAND:     //&
	case T_NOT:        //!
	case T_REV:        //~ (reverse  a = ~a)
	case T_ADDADD:     //++
	case T_SUBSUB:     //--
	case T_SIZEOF:     //sizeof
		t = exp_stmt();
		break;
	case T_SEMI:     // ;
		match(T_SEMI);
		break;
	case T_LLPAREN: //{
	case T_IF:      //if
	case T_SWITCH:  //switch
	case T_DO:      //do
	case T_WHILE:   //while
	case T_FOR:     //for
		t = statement();
		break;
	case T_GOTO:    //goto
	case T_BREAK:   //break
	case T_RETURN:  //return
	case T_CONTINUE://continue
		t = jump_stmt();
		break;
	case T_VOID:
	case T_CHAR:
	case T_SHORT:
	case T_INT:
	case T_LONGLONG:
	case T_BOOL:
	case T_LONG:
	case T_FLOAT:
	case T_DOUBLE:
	case T_SIGNED:
	case T_UNSIGNED:
	case T_STRUCT:
	case T_UNION:
	case T_AUTO:
	case T_REGISTER:
	case T_EXTERN:
	case T_INLINE:
	case T_STATIC:
	case T_TYPEDEF:
	case T_CONST:
	case T_VOLATILE:
	case T_ENUM:
		declaration();
		break;
	case T_END:      // end of file
		return ST_SUCC;
		break;
	case T_NUL:
		err(g_real_line_num, "unrecognized token :%s", g_real_token_string);
		return t;
	default:
		err(g_real_line_num, "unrecognized token :%s", g_real_token_string);
		return t;
	}
	IS_TRUE0(verify(t));
	return t;
}


//Initialize pool for parser.
void init_parser()
{
	init_key_word_tab();
	g_pool_general_used = smpool_create_handle(256, MEM_COMM);
	g_pool_tree_used = smpool_create_handle(128, MEM_COMM);
	g_pool_st_used = smpool_create_handle(64, MEM_COMM);
}


void fini_parser()
{
	smpool_free_handle(g_pool_general_used);
	smpool_free_handle(g_pool_tree_used);
	smpool_free_handle(g_pool_st_used);
	g_pool_general_used = NULL;
	g_pool_tree_used = NULL;
	g_pool_st_used = NULL;

	if (g_ofst_tab != NULL) {
		::free(g_ofst_tab);
		g_ofst_tab = NULL;
	}

	if (g_cur_line != NULL) {
		::free(g_cur_line);
		g_cur_line = NULL;
	}

	if (g_hsrc != NULL) {
		fclose(g_hsrc);
		g_hsrc = NULL;
	}
}


//Start to parse a file.
INT c_parser()
{
	IS_TRUE(g_hsrc && g_fe_sym_tab, ("must initialize them"));
	/*
	base_type_spec:   one of
		type-name void char short int long float double signed
		unsigned struct union auto register extern static typedef const
		volatile enum

	declarator:  one of
		id * (

	stmt: one of
		id imm imml floatpoint string charlist { ( +
		- * & ! ~ ++ -- if goto
		break return continue do while switch case default for
		sizeof

	jmp_stmt :
		goto break return continue

	sel_stmt :
		if switch
	iter :
		do while for

	exp :
		id imm imml floatpoint string charlist ( + -
		* & ! ~ ++ -- sizeof

	lab_stmt :
		id case default

	typedef_name :
		id

	enum_constant:
 		id
	*/
	IS_TRUE0(g_hsrc != NULL);
	gettok();

	//Create outermost scope for top region.
	g_cur_scope = new_scope();
	SCOPE_level(g_cur_scope) = GLOBAL_SCOPE; //First global scope
	while (1) {
		if (g_real_token == T_END) {
			//dump_scope(g_cur_scope,
			//			 DUMP_SCOPE_FUNC_BODY|DUMP_SCOPE_STMT_TREE);
			return ST_SUCC;
		} else if (g_real_token == T_NUL) {
			return ST_ERR;
		} else if (is_too_many_err()) {
			return ST_ERR;
		}
		if (dispatch() == NULL && g_err_msg_list.get_elem_count() != 0) {
			return ST_ERR;
		}
	}
}

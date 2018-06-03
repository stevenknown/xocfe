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
#include "cfecommacro.h"

#define NEWTN(ttt)  allocTreeNode((ttt), g_real_line_num)

static Tree * statement();
static bool is_c_type_spec(TOKEN tok);
static bool is_c_type_quan(TOKEN tok);
static bool is_c_stor_spec(TOKEN tok);
static Tree * cast_exp();
static Tree * unary_exp();
static Tree * exp_stmt();
static Tree * postfix_exp();

SMemPool * g_pool_general_used = NULL;
SMemPool * g_pool_st_used = NULL;
SMemPool * g_pool_tree_used = NULL;
SymTab * g_fe_sym_tab = NULL;
bool g_dump_token = false;
CHAR * g_real_token_string = NULL;
TOKEN g_real_token = T_NUL;
static List<CELL*> g_cell_list;
bool g_enable_C99_declaration = true;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    ASSERT0(p);
    ::memset(p, 0, size);
    return p;
}


#ifdef _DEBUG_
//Verify the Tree node legality.
static bool verify(Tree * t)
{
    if (t == NULL) { return true; }
    switch (TREE_type(t)) {
    case TR_SCOPE:
    case TR_CALL:
    case TR_ARRAY:
    case TR_TYPE_NAME:
    case TR_CVT:
    case TR_COND:
    case TR_LABEL:
        break;
    default: ASSERT0(TREE_token(t) != T_NUL && getTokenName(TREE_token(t)));
    }

    switch (TREE_type(t)) {
    case TR_ASSIGN:
    case TR_ID:
    case TR_IMM:
    case TR_IMML:
    case TR_IMMU:
    case TR_IMMUL:
    case TR_FP:
    case TR_FPF:
    case TR_FPLD:
    case TR_ENUM_CONST:
    case TR_STRING:
    case TR_LOGIC_OR:     // logical OR ||
    case TR_LOGIC_AND:    // logical AND &&
    case TR_INCLUSIVE_OR: // inclusive OR |
    case TR_INCLUSIVE_AND:// inclusive AND &
    case TR_XOR:          // exclusive OR
    case TR_EQUALITY:     // == !=
    case TR_RELATION:     // < > >= <=
    case TR_SHIFT:        // >> <<
    case TR_ADDITIVE:     // '+' '-'
    case TR_MULTI:        // '*' '/' '%'
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
    case TR_COND:         // formulized log_OR_exp?exp:cond_exp
    case TR_CVT:          // type convertion
    case TR_LDA:          // &a get address of 'a'
    case TR_DEREF:        // *p  dereferencing the pointer 'p'
    case TR_PLUS:         // +123
    case TR_MINUS:        // -123
    case TR_REV:          // Reverse
    case TR_NOT:          // get non-value
    case TR_INC:          // ++a
    case TR_DEC:          // --a
    case TR_POST_INC:     // a++ OR (*a)++
    case TR_POST_DEC:     // a--
    case TR_DMEM:
    case TR_INDMEM:
    case TR_ARRAY:
    case TR_CALL:
    case TR_PRAGMA:
    case TR_SIZEOF:
        break;
    default: ASSERT(0, ("unknown tree type:%d", TREE_type(t)));
    } //end switch
    return true;
}
#endif


//Return NULL indicate we haven't found it in 'l_list', and
//append 'label' to tail of the list as correct,
//otherwise return 'l'.
//Add a label into outmost scope of current function.
static LabelInfo * add_label(CHAR * name, INT lineno)
{
    LabelInfo * li;
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

    //Allocate different LabelInfo for different lines.
    li = allocCustomerLabel(g_fe_sym_tab->add(name), g_pool_general_used);
    //li = g_labtab.append_and_retrieve(li);
    set_map_lab2lineno(li, lineno);
    SCOPE_label_list(sc).append_tail(li);
    return li;
}


//Record a label reference into outmost scope of current function.
static LabelInfo * add_ref_label(CHAR * name, INT lineno)
{
    LabelInfo * li;
    SCOPE * sc = g_cur_scope;
    while (sc != NULL && SCOPE_level(sc) != FUNCTION_SCOPE) {
        sc = SCOPE_parent(sc);
    }
    if (sc == NULL) {
        err(g_src_line_num,
            "label reference illegal, and it should be used in function.");
        return NULL;
    }

    //Allocate different LabelInfo for different lines.
    li = allocCustomerLabel(g_fe_sym_tab->add(name), g_pool_general_used);
    //li = g_labtab.append_and_retrieve(li);
    set_lab_used(li);
    set_map_lab2lineno(li, lineno); //ONLY for debug-info or dumping
    SCOPE_ref_label_list(sc).append_tail(li);
    return li;
}


//list operation
//Append a token to tail of list
static void append_c_tail(size_t v)
{
    //static CELL * g_cell_list_head=NULL;
    //static CELL * g_cell_list_tail=NULL;
    CELL * c = newcell(0);
    CELL_val(c) = (LONGLONG)v;
    CELL_line_no(c) = g_src_line_num;
    g_cell_list.append_tail(c);
}


//Append a token to head of list
static void append_c_head(size_t v)
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
    TokenInfo * tki = (TokenInfo*)xmalloc(sizeof(TokenInfo));
    SYM * s = g_fe_sym_tab->add(tokname);
    TOKEN_INFO_name(tki) = SYM_name(s);
    TOKEN_INFO_token(tki) = tok;
    TOKEN_INFO_lineno(tki) = lineno;
    append_c_tail((size_t)tki);
}


//Append current token info descripte by 'g_cur_token','g_cur_token_string'
//and 'g_src_line_num'
static void append_tok_head(TOKEN tok, CHAR * tokname, INT lineno)
{
    TokenInfo * tki = (TokenInfo*)xmalloc(sizeof(TokenInfo));
    SYM * s = g_fe_sym_tab->add(tokname);
    TOKEN_INFO_name(tki) = SYM_name(s);
    TOKEN_INFO_token(tki) = tok;
    TOKEN_INFO_lineno(tki) = lineno;
    append_c_head((size_t)tki);
}


//Remove a token from head of list
static size_t remove_head_tok()
{
    CELL * c = g_cell_list.remove_head();
    if (c != NULL) {
        size_t v = (size_t)CELL_val(c);
        free_cell(c);
        return v;
    }
    return 0;
}


void dump_tok_list()
{
    CELL * c = g_cell_list.get_head();
    if (c != NULL) {
        prt2C("\nTOKEN:");
        for (; c; c = g_cell_list.get_next()) {
            CHAR * s = TOKEN_INFO_name((TokenInfo*)CELL_val(c));
            prt2C("'%s' ", s);
        }
        prt2C("\n");
    }
}


bool is_in_first_set_of_declarator()
{
    if (g_real_token == T_ID) {
        Decl * ut = NULL;
        Struct * s;
        Union * u;
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


//Return true if 'tok' indicate terminal charactor, otherwise false.
bool is_in_first_set_of_exp_list(TOKEN tok)
{
    Decl * ut = NULL;
    switch (tok) {
    case T_ID:
        if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut)) {
            //If there is a type-name, then it
            //belongs to first-set of declarator.
            return false;
        } else {
            //May be identifier or enum-constant.
            return true;
        }
        break;
    case T_IMM:       // 0~9
    case T_IMML:      // 0~9L
    case T_IMMU:      // Unsigned
    case T_IMMUL:     // Unsigned Long
    case T_FP:        // double decimal e.g 3.14
    case T_FPF:       // float decimal e.g 3.14
    case T_FPLD:      // long double decimal e.g 3.14
    case T_STRING:    // "abcd"
    case T_CHAR_LIST: // 'abcd'
    case T_LPAREN:    // (
    case T_ADD:       // +
    case T_SUB:       // -
    case T_ASTERISK:  // *
    case T_BITAND:    // &
    case T_NOT:       // !
    case T_REV:       // ~ (reverse  a = ~a)
    case T_ADDADD:    // ++
    case T_SUBSUB:    // --
    case T_SIZEOF:    // sizeof
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


bool is_user_type_exist_in_outer_scope(CHAR * cl, OUT Decl ** ut)
{
    SCOPE * sc = g_cur_scope;
    while (sc != NULL) {
        if (is_user_type_exist(SCOPE_user_type_list(sc), cl, ut)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


INT is_user_type_exist_in_cur_scope(CHAR * cl, OUT Decl ** ut)
{
    SCOPE * sc = g_cur_scope;
    if (is_user_type_exist(SCOPE_user_type_list(sc), cl, ut)) {
        return 1;
    }
    return 0;
}


//Find if ID with named 'cl' exists and return the Decl.
static inline INT is_id_exist_in_outer_scope(CHAR * cl, OUT Decl ** d)
{
    return is_decl_exist_in_outer_scope(cl, d);
}


static inline INT is_first_set_of_unary_exp(TOKEN tok)
{
    switch (tok) {
    case T_LPAREN:
    case T_ID:
    case T_IMM:
    case T_IMML:
    case T_IMMU:
    case T_IMMUL:
    case T_FP:
    case T_FPF:       //float decimal e.g 3.14
    case T_FPLD:      //long double decimal e.g 3.14
    case T_STRING:
    case T_CHAR_LIST:
    case T_AND:
    case T_ADDADD:
    case T_SUBSUB:
    case T_ADD:
    case T_SUB:
    case T_NOT:
    case T_REV:
    case T_ASTERISK:
    case T_BITAND:
    case T_SIZEOF:
        return true;
    default:;
    }
    return false;
}


static inline INT is_assign_op(TOKEN tok)
{
    switch (tok) {
    case T_ASSIGN:
    case T_BITANDEQU:
    case T_BITOREQU:
    case T_ADDEQU:
    case T_SUBEQU:
    case T_MULEQU:
    case T_DIVEQU:
    case T_XOREQU:
    case T_RSHIFTEQU:
    case T_LSHIFTEQU:
    case T_REMEQU:
        return true;
    default:;
    }
    return false;
}


void setParent(Tree * parent, Tree * child)
{
    if (child == NULL) { return; }
    while (child != NULL) {
        TREE_parent(child) = parent;
        child = TREE_nsib(child);
    }
}


//Draw token until meeting any TOKEN list in '...'
void suck_tok_to(INT placeholder, ...)
{
    va_list arg;
    TOKEN tok;
    ASSERT0_DUMMYUSE(sizeof(TOKEN) == sizeof(INT));
    for (;;) {
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


Tree * id()
{
    Tree * t = NEWTN(TR_ID);
    TREE_token(t) = g_real_token;
    SYM * sym = g_fe_sym_tab->add(g_real_token_string);
    TREE_id(t) = sym;
    return t;
}


static INT gettok()
{
    getNextToken();
    g_real_token = g_cur_token;
    g_real_token_string = g_cur_token_string;
    g_real_line_num = g_src_line_num;
    return g_real_token;
}


static INT reset_tok()
{
    TokenInfo * tki;
    if ((tki = (TokenInfo*)remove_head_tok()) == NULL) {
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
    TokenInfo * tki;
    if ((tki = (TokenInfo*)remove_head_tok()) == NULL) {
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


//Pry next 'num' token info.
//
//'n': represent the next N token to current token.
//    If n is 0, it will return current token.
static TOKEN look_next_token(
        INT n,
        OUT CHAR ** tok_string,
        OUT UINT * tok_line_num)
{
    TOKEN tok = T_NUL;
    if (n < 0) { return T_NUL; }

    if (n == 0) { return g_real_token; }

    INT count = g_cell_list.get_elem_count();
    if (count > 0) {
        //Pry in token_buffer
        if (n <= count) {
            //'n' can be finded in token-buffer
            CELL * c = g_cell_list.get_head_nth(n - 1);
            ASSERT0(c);
            return TOKEN_INFO_token((TokenInfo*)CELL_val(c));
        }

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
    }

    //For now, count == 0
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
}


//'num': pry the followed 'num' number of tokens.
//'...': represent a token list which will to match.
bool look_forward_token(INT num, ...)
{
    if (num <= 0) {
        return 0;
    }

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
                TokenInfo * tki = (TokenInfo*)CELL_val(c);
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


static Tree * param_list()
{
    Tree * t = exp();
    Tree * last = get_last(t);

    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        Tree * nt = exp();
        if (nt == NULL) {
            err(g_real_line_num, "miss patameter, syntax error : '%s'",
                g_real_token_string);
            return t;
        }
        add_next(&t, &last, nt);
    }
    return t;
}


//Process the first part of postfix expression:
//e.g:  a[10].u1.s++,  where a[10] is the first part,
//    .u1.s++ is the second part.
static Tree * primary_exp(IN OUT UINT * st)
{
    Tree * t = NULL;
    switch (g_real_token) {
    case T_ID:
        {
            Enum * e = NULL;
            INT idx = 0;
            if (findEnumConst(g_real_token_string, &e, &idx)) {
                t = NEWTN(TR_ENUM_CONST);
                TREE_enum(t) = e;
                TREE_enum_val_idx(t) = idx;
            } else {
                //Struct, Union, TYPEDEF-NAME should be
                //parsed during declaration().
                Decl * dcl = NULL;
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
            TREE_token(t) = g_real_token;
            match(T_ID);
        }
        break;
    case T_IMM:
        t = NEWTN(TR_IMM);
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_INT)xatoll(g_real_token_string, false);
        match(T_IMM);
        return t;
    case T_IMML:
        t = NEWTN(TR_IMML);
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_INT)xatoll(g_real_token_string, false);
        match(T_IMML);
        return t;
    case T_IMMU:
        t = NEWTN(TR_IMMU);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xatoll(g_real_token_string, false);
        match(T_IMMU);
        return t;
    case T_IMMUL:
        t = NEWTN(TR_IMMUL);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xatoll(g_real_token_string, false);
        match(T_IMMUL);
        return t;
    case T_FP:         // decimal e.g 3.14
        t = NEWTN(TR_FP);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        match(T_FP);
        break;
    case T_FPF:         // decimal e.g 3.14
        t = NEWTN(TR_FPF);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        match(T_FPF);
        break;
    case T_FPLD:         // decimal e.g 3.14
        t = NEWTN(TR_FPLD);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        match(T_FPLD);
        break;
    case T_STRING:     // "abcd"
        {
            t = NEWTN(TR_STRING);
            TREE_token(t) = g_real_token;
            CHAR * tbuf = NULL;
            UINT tbuflen = 0;
            SYM * sym = g_fe_sym_tab->add(g_real_token_string);
            match(T_STRING);

            //Concatenate string.
            for (; g_real_token == T_STRING; match(T_STRING)) {
                if (tbuf == NULL) {
                    tbuflen = strlen(SYM_name(sym)) +
                              strlen(g_real_token_string) + 1;
                    tbuf = (CHAR*)malloc(tbuflen);
                    sprintf(tbuf, "%s%s", SYM_name(sym), g_real_token_string);
                } else {
                    tbuflen += strlen(g_real_token_string);
                    CHAR * ttbuf = (CHAR*)malloc(tbuflen);
                    sprintf(ttbuf, "%s%s", tbuf, g_real_token_string);
                    free(tbuf);
                    tbuf = ttbuf;
                }
            }

            if (tbuf != NULL) {
                sym = g_fe_sym_tab->add(tbuf);
                free(tbuf);
            }
            TREE_string_val(t) = sym;
        }
        return t;
    case T_CHAR_LIST:  // 'abcd'
        t = NEWTN(TR_IMM);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = xctoi(g_real_token_string);
        match(T_CHAR_LIST);
        break;
    case T_LPAREN:
        match(T_LPAREN);
        t = exp_list();
        if (match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')' after expression");
            *st = ST_ERR;
            return NULL;
        }
        break;
    default:;
    }
    *st = ST_SUCC;
    return t;
}


//Process the first part of postfix expression:
//e.g:  a[10].u1.s++,  where a[10] is the first part,
//    .u1.s++ is the second part.
//The second part include: [ ( . -> ++ --
static Tree * postfix_exp_second_part(IN Tree * t)
{
AGAIN:
    switch (g_real_token) {
    case T_LSPAREN: //array reference
        {
            Tree * array_root = NULL;
            while (g_real_token == T_LSPAREN) {
                match(T_LSPAREN);
                array_root = NEWTN(TR_ARRAY);
                TREE_array_base(array_root) = t;
                TREE_array_indx(array_root) = exp();
                setParent(array_root, t);
                setParent(array_root, TREE_array_indx(array_root));
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
            Tree * tp = NEWTN(TR_CALL);
            TREE_fun_exp(tp) = t;
            TREE_para_list(tp) = param_list();
            if (match(T_RPAREN) != ST_SUCC) {
                err(g_real_line_num, "miss ')'");
                return t;
            }
            setParent(tp, t);
            setParent(tp, TREE_para_list(tp));
            t = tp;
        }
        break;
    case T_DOT: //direct struct member reference
    case T_ARROW: //indirect struct member reference
        {
            Tree * mem_ref = NULL;
            if (g_real_token == T_DOT) {
                mem_ref = NEWTN(TR_DMEM);
            } else {
                mem_ref = NEWTN(TR_INDMEM);
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
            setParent(mem_ref, t);
            setParent(mem_ref, TREE_field(mem_ref));
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
            Tree * tp = NEWTN(TR_POST_INC);
            TREE_token(tp) = g_real_token;
            TREE_inc_exp(tp) = t;
            setParent(tp, t);
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
            Tree * tp = NEWTN(TR_POST_DEC);
            TREE_token(tp) = g_real_token;
            TREE_dec_exp(tp) = t;
            setParent(tp, t);
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


//Process the first part of postfix expression:
//e.g:  a[10].u1.s++,  where a[10] is the first part,
//    .u1.s++ is the second part.
static Tree * postfix_exp()
{
    UINT st = 0;
    Tree * t = primary_exp(&st);
    if (st != ST_SUCC) {
        return t;
    }
    return postfix_exp_second_part(t);
}


//Whether input lexicalgraphic words can be descripted with
//  unary-expression   or
//  ( type-name )
//
//NOTE: C language only permit 'type-name' be enclosed by one pair of '()'.
//e.g: ((char*)) is illegal syntax. */
static Tree * unary_or_LP_typename_RP()
{
    Tree * t = NULL;
    TOKEN tok = T_NUL;
    CHAR * tok_string = NULL;
    if (g_real_token == T_LPAREN) {
        //Next exp either (exp) or (type_name), so after several '(', there
        //must be T_ID appearing .
        tok = look_next_token(1, &tok_string, NULL);
        if (is_c_type_spec(tok) ||
            is_c_type_quan(tok) ||
            is_c_stor_spec(tok)) {
            t = NEWTN(TR_TYPE_NAME);
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
            Decl * ut = NULL;
            ASSERT0(tok_string != NULL);
            if (is_user_type_exist_in_outer_scope(tok_string, &ut)) {
                //User defined Type via 'typedef'.
                t = NEWTN(TR_TYPE_NAME);
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


//unary_operator:  one of
//    &  *  +  -  ~  !
//
//unary_expression:
//    postfix_expression
//    ++ unary_expression
//    -- unary_expression
//    (&  *  +  -  ~  !) cast_expression
//    sizeof unary_expression
//    sizeof ( type_name )
static Tree * unary_exp()
{
    Tree * t = NULL;
    switch (g_real_token) {
    case T_IMM:
    case T_IMML:
    case T_IMMU:
    case T_IMMUL:
    case T_FP:        //decimal e.g 3.14
    case T_FPF:       //decimal e.g 3.14
    case T_FPLD:      //decimal e.g 3.14
    case T_STRING:    //"abcd"
    case T_CHAR_LIST: //'abcd'
    case T_LPAREN:    //(exp)
    case T_ID:
        t = postfix_exp();
        break;
    case T_ASTERISK:
        t = NEWTN(TR_DEREF);
        TREE_token(t) = g_real_token;
        match(T_ASTERISK);
        TREE_lchild(t) = cast_exp();
        setParent(t, TREE_lchild(t));
        break;
    case T_BITAND:
        t = NEWTN(TR_LDA);
        TREE_token(t) = g_real_token;
        match(T_BITAND);
        TREE_lchild(t) = cast_exp();
        setParent(t, TREE_lchild(t));
        break;
    case T_ADDADD:
        t = NEWTN(TR_INC);
        TREE_token(t) = g_real_token;
        match(T_ADDADD);
        TREE_inc_exp(t) = unary_exp();
        setParent(t, TREE_inc_exp(t));
        if (TREE_inc_exp(t) == NULL) {
            err(g_real_line_num, "unary expression is needed");
            goto FAILED;
        }
        break;
    case T_SUBSUB:
        t = NEWTN(TR_DEC);
        TREE_token(t) = g_real_token;
        match(T_SUBSUB);
        TREE_dec_exp(t) = unary_exp();
        setParent(t,TREE_dec_exp(t));
        if (TREE_inc_exp(t) == NULL) {
            err(g_real_line_num, "unary expression is needed");
            goto FAILED;
        }
        break;
    case T_ADD:
        t = NEWTN(TR_PLUS);
        TREE_token(t) = g_real_token;
        match(T_ADD);
        TREE_lchild(t) = cast_exp();
        setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == NULL) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_SUB:
        t = NEWTN(TR_MINUS);
        TREE_token(t) = g_real_token;
        match(T_SUB);
        TREE_lchild(t) = cast_exp();
        setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == NULL) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_SIZEOF:
        t = NEWTN(TR_SIZEOF);
        TREE_token(t) = g_real_token;
        match(T_SIZEOF);
        TREE_sizeof_exp(t) = unary_or_LP_typename_RP();
        setParent(t, TREE_sizeof_exp(t));
        break;
    case T_REV:
        t = NEWTN(TR_REV);
        TREE_token(t) = g_real_token;
        match(T_REV);
        TREE_lchild(t) = cast_exp();
        setParent(t, TREE_lchild(t));
        if (TREE_lchild(t) == NULL) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_NOT:
        t = NEWTN(TR_NOT);
        TREE_token(t) = g_real_token;
        match(T_NOT);
        TREE_lchild(t) = cast_exp();
        setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == NULL) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    default:;
    }
    return t ;
FAILED:
    prt2C("error in unary_exp()");
    return t;
}


Tree * gen_typename(Decl * decl)
{
    Tree * t = NEWTN(TR_TYPE_NAME);
    TREE_type_name(t) = decl;
    return t;
}


Tree * gen_cvt(Tree * tgt_type, Tree * src)
{
    ASSERT0(tgt_type && TREE_type(tgt_type) == TR_TYPE_NAME && src);
    Tree * t = NEWTN(TR_CVT);
    TREE_cvt_type(t) = tgt_type;
    TREE_cast_exp(t) = src;
    setParent(t, TREE_cvt_type(t));
    setParent(t, TREE_cast_exp(t));
    return t;
}


Tree * gen_cvt(Decl const* tgt_type, Tree * src)
{
    ASSERT0(tgt_type && src);
    Decl * dup = cp_typename(tgt_type);
    return gen_cvt(gen_typename(dup), src);
}


//BNF:
//cast_expression:
//    unary_expression
//    ( type_name ) cast_expression
//
//FIRST_SET:
//cast_exp :
//    id imm imml floatpoint string charlist ( + -
//    * & ! ~ ++ -- sizeof
static Tree * cast_exp()
{
    Tree * t = NULL, * p;
    p = unary_or_LP_typename_RP();
    if (p == NULL) { return NULL; }

    //It might be follow-set token.
    //if (p == NULL) {
    //    err(g_real_line_num,
    //        "syntax error : '%s' , except a identifier or typedef-name",
    //        g_real_token_string);
    //    goto FAILED;
    //}

    if (TREE_type(p) == TR_TYPE_NAME) {
        Tree * srcexp = cast_exp();
        if (srcexp == NULL) {
            err(g_real_line_num, "cast expression cannot be NULL");
            goto FAILED;
        }
        t = gen_cvt(p, srcexp);
        if (TREE_cast_exp(t) == NULL) {
            err(g_real_line_num, "cast expression cannot be NULL");
            goto FAILED;
        }
    } else {
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in cast_exp()");
    return t;
}


static Tree * multiplicative_exp()
{
    Tree * t = cast_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_ASTERISK ||
           g_real_token == T_DIV ||
           g_real_token == T_MOD) {
        p = NEWTN(TR_MULTI);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
            // a*b*c  =>
            //             *
            //           /  |
            //           *  c
            //          / | 
            //         a  b
        match(g_real_token);
        TREE_rchild(p) = cast_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;

FAILED:
    prt2C("error in multiplicative_exp()");
    return t;
}


static Tree * additive_exp()
{
    Tree * t = multiplicative_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_ADD || g_real_token == T_SUB) {
        p = NEWTN(TR_ADDITIVE);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p, TREE_lchild(p));
           //  a+b-c  =>
           //              -
           //             / |
           //            +  c
           //           / |
           //          a  b
        match(g_real_token);
        TREE_rchild(p) = multiplicative_exp();
        setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;

FAILED:
    prt2C("error in additive_exp()");
    return t;
}


static Tree * shift_exp()
{
    Tree * t = additive_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_LSHIFT || g_real_token == T_RSHIFT) {
        p = NEWTN(TR_SHIFT);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
           //  a<<b<<c  =>
           //              <<
           //             /  |
           //            <<  c
           //           /  |
           //          a   b
        match(g_real_token);
        TREE_rchild(p) = additive_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:

    prt2C("error in shift_exp()");
    return t;
}


static Tree * relational_exp()
{
    Tree * t = shift_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_LESSTHAN || g_real_token == T_MORETHAN ||
           g_real_token == T_NOMORETHAN || g_real_token == T_NOLESSTHAN) {
        p = NEWTN(TR_RELATION);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
           //  a<b<c   =>
           //              <
           //             / |
           //            <  c
           //           / |
           //          a  b
        match(g_real_token);
        TREE_rchild(p) = shift_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in relational_exp()");
    return t;
}


static Tree * equality_exp()
{
    Tree * t = relational_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_EQU || g_real_token == T_NOEQU) {
        p = NEWTN(TR_EQUALITY);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p, TREE_lchild(p));
           //  a==b!=c   =>
           //              !=
           //             /  |
           //            ==  c
           //           /  |
           //          a   b
        match(g_real_token);
        TREE_rchild(p) = relational_exp();
        setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in equality_exp()");
    return t;
}


static Tree * AND_exp()
{
    Tree * t = equality_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_BITAND) {
        p = NEWTN(TR_INCLUSIVE_AND);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p, TREE_lchild(p));
           //  a&b&c   =>  &
           //             / |
           //            &  c
           //           / |
           //          a  b
        match(T_BITAND);
        TREE_rchild(p) = equality_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in AND_exp()");
    return t;
}


static Tree * exclusive_OR_exp()
{
    Tree * t = AND_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_XOR) {
        p = NEWTN(TR_XOR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
           //  a^b^c   =>  ^
           //             / |
           //            ^  c
           //           / |
           //          a  b
        match(T_XOR);
        TREE_rchild(p) = AND_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in exclusive_OR_exp()");
    return t;
}


static Tree * inclusive_OR_exp()
{
    Tree * t = exclusive_OR_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_BITOR) {
        p = NEWTN(TR_INCLUSIVE_OR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
        //  a|b|c   =>  |
        //             / |
        //            |  c
        //           / |
        //          a  b
        match(T_BITOR);
        TREE_rchild(p) = exclusive_OR_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in inclusive_OR_exp()");
    return t;
}


static Tree * logical_AND_exp()
{
    Tree * t = inclusive_OR_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_AND) {
        p = NEWTN(TR_LOGIC_AND);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
        //a && b && c =>
        //              &&
        //             /  |
        //            &&  c
        //           /  |
        //           a  b
        match(T_AND);
        TREE_rchild(p) = inclusive_OR_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in logical_AND_exp()");
    return t;
}


static Tree * logical_OR_exp()
{
    Tree * t = logical_AND_exp(), * p = NULL;
    if (t == NULL) return NULL;
    while (g_real_token == T_OR) {
        p = NEWTN(TR_LOGIC_OR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p,TREE_lchild(p));
        //  a||b||c  =>  ||
        //              /  |
        //             ||  c
        //            /  |
        //           a   b
        match(T_OR);
        TREE_rchild(p) = logical_AND_exp();
        setParent(p,TREE_rchild(p));
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "'%s': right operand cannot be NULL",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t ;
FAILED:
    prt2C("error in logical_OR_exp()");
    return t;
}


//logical_OR_expression ? expression : conditional_expression
Tree * conditional_exp()
{
    Tree * t = logical_OR_exp();
    if (g_real_token == T_QUES_MARK) {
        match(T_QUES_MARK);
        Tree * p = NEWTN(TR_COND);
        TREE_det(p) = t;
        setParent(p,t);
        t = p;

        TREE_true_part(t) = exp_list();
        setParent(t,TREE_true_part(t));
        if (match(T_COLON) != ST_SUCC) {
            err(g_real_line_num, "condition expression is incomplete");
            goto FAILED;
        }
        TREE_false_part(t) = exp_list();
        setParent(t,TREE_false_part(t));
    }
    return t ;
FAILED:
    prt2C("error in conditional_expt()");
    return t;
}


Tree * exp()
{
    Tree * t = conditional_exp(), * p;
    if (t == NULL) { return t; }
    if (is_assign_op(g_real_token)) {
        p = NEWTN(TR_ASSIGN);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p, TREE_lchild(p));
        match(g_real_token);
        TREE_rchild(p) = exp();
        if (TREE_rchild(p) == NULL) {
            err(g_real_line_num, "expression miss r-value");
            goto FAILED;
        }
        setParent(p, TREE_rchild(p));
          t = p ;
    }
    return t;
FAILED:
    prt2C("error in expt()");
    return t;
}


Tree * exp_list()
{
    Tree * t = exp();
    Tree * last = get_last(t);
    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        Tree * nt = exp();
        add_next(&t, &last, nt);
        last = get_last(nt);
    }
    return t;
}


//jump_statement:
//    goto identifier;
//    continue;
//    break;
//    return              ;
//    return expression;
static Tree * jump_stmt()
{
    Tree * t = NULL;
    switch (g_real_token) {
    case T_GOTO:
        t = NEWTN(TR_GOTO);
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
        t = NEWTN(TR_BREAK);
        TREE_token(t) = g_real_token;
        match(T_BREAK);
        if (match(T_SEMI) != ST_SUCC) {
            err(g_real_line_num, "miss ';'");
            return t;
        }
        break;
    case T_RETURN:
        t = NEWTN(TR_RETURN);
        TREE_token(t) = g_real_token;
        match(T_RETURN);
        if (g_real_token != T_SEMI) {
            TREE_ret_exp(t) = exp_list();
            setParent(t,TREE_ret_exp(t));
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
        t = NEWTN(TR_CONTINUE);
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


static Tree * sharp_start_stmt()
{
    Tree * t = NEWTN(TR_PRAGMA);
    TREE_token(t) = g_real_token;
    ASSERT0(g_real_token == T_SHARP);
    match(T_SHARP);
    if (g_real_token != T_PRAGMA) {
        err(g_real_line_num,
            "illegal use '#', its followed keyword should be 'pragma'");
        return NULL;
    }
    match(T_PRAGMA);

    TokenList * last = NULL;
    g_enable_newline_token = true;
    while (g_real_token != T_NEWLINE && g_real_token != T_NUL) {
        TokenList * tl = (TokenList*)xmalloc(sizeof(TokenList));
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


static Tree * label_stmt()
{
    Tree * t = NULL;
    switch (g_real_token) {
    case T_ID:
        t = NEWTN(TR_LABEL);
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
            Tree * nt = NULL;
            match(T_CASE);
            if (!is_sst_exist(st_DO) &&
                !is_sst_exist(st_WHILE) &&
                !is_sst_exist(st_FOR) &&
                !is_sst_exist(st_SWITCH)) {
                err(g_real_line_num, "invalid use 'case'");
                return t;
            }

            t = NEWTN(TR_CASE);
            TREE_token(t) = g_real_token;
            nt = conditional_exp(); //case expression must be constant.
            if (!computeConstExp(nt, &idx, 0)) {
                err(g_real_line_num, "expected constant expression");
                return t;
            }
            if (computeConstBitLen((TMWORD)idx) >
                (BYTE_PER_INT * HOST_BIT_PER_BYTE)) {
                err(g_real_line_num, "bitsize of const is more than %dbit",
                    (sizeof(TREE_case_value(t)) * HOST_BIT_PER_BYTE));
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
            t = NEWTN(TR_DEFAULT);
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


//{ do-body }
//while (determination)
static Tree * do_while_stmt()
{
    Tree * t = NEWTN(TR_DO);
    TREE_token(t) = g_real_token;
    match(T_DO);

    //do-body
    pushst(st_DO, 0); //push down inherit properties
    TREE_dowhile_body(t) = statement();
    setParent(t, TREE_dowhile_body(t));
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

    TREE_dowhile_det(t) = exp_list();
    setParent(t, TREE_dowhile_det(t));
    if (TREE_dowhile_det(t) == NULL) {
        err(g_real_line_num, "while determination cannot be NULL");
        goto FAILED;
    }

    if (match(T_RPAREN) != ST_SUCC) { //)
        err(g_real_line_num, "miss ')'");
        goto FAILED;
    }

    if (g_real_token != T_SEMI ) { //;
        err(g_real_line_num, "miss ';' after 'while'");
        goto FAILED;
    }

    return t;

FAILED:
    prt2C("error in do_while_stmt()");
    return t;
}


static Tree * while_do_stmt()
{
    Tree * t = NEWTN(TR_WHILE);
    TREE_token(t) = g_real_token;
    match(T_WHILE);

    //determination
    if (match(T_LPAREN) != ST_SUCC) { //(
        err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
        goto FAILED;
    }
    TREE_whiledo_det(t) =  exp_list();
    setParent(t, TREE_whiledo_det(t));
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
    setParent(t,TREE_whiledo_body(t));
    popst();
    return t;
FAILED:
    prt2C("error in while_do_stmt()");
    return t;
}


Tree * for_stmt()
{
    Tree * t = NEWTN(TR_FOR);
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
        setParent(t, TREE_for_init(t));

        //EXPRESSION does not swallow the token ';'.
        if (match(T_SEMI) != ST_SUCC) {
            err(g_real_line_num, "miss ';' before determination");
            goto FAILED;
        }
    }

    TREE_for_det(t) = exp_list();
    setParent(t,TREE_for_det(t));
    //EXPRESSION does not swallow the token ';'.
    if (match(T_SEMI) != ST_SUCC) {
        err(g_real_line_num, "miss ';' before step expression");
        goto FAILED;
    }

    TREE_for_step(t) = exp_list();
    setParent(t, TREE_for_step(t));
    //EXPRESSION does not swallow the token ')'.
    if (match(T_RPAREN) != ST_SUCC) {
        err(g_real_line_num, "miss ')' after step expression");
        goto FAILED;
    }

    pushst(st_DO, 0); //push down inherit properties
    TREE_for_body(t) = statement();
    setParent(t, TREE_for_body(t));
    popst();
    if (g_enable_C99_declaration) {
        return_to_parent_scope();
    }

    return t;
FAILED:
    if (g_enable_C99_declaration) {
        return_to_parent_scope();
    }
    prt2C("error in for_stmt()");
    return t;
}


static Tree * iter_stmt()
{
    Tree * t = NULL;
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


static Tree * if_stmt()
{
    Tree * t = NEWTN(TR_IF);
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
    setParent(t, TREE_if_det(t));
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
    setParent(t,TREE_if_true_stmt(t));

    if (g_real_token == T_ELSE) {
        match(T_ELSE);
        TREE_if_false_stmt(t) = statement();
        setParent(t,TREE_if_false_stmt(t));
    }
    return t;
FAILED:
    prt2C("error in if_stmt()");
    return t;
}


static Tree * switch_stmt()
{
    Tree * t = NEWTN(TR_SWITCH);
    TREE_token(t) = g_real_token;
    match(T_SWITCH);

    //determination
    if (match(T_LPAREN)  != ST_SUCC) { //(
        err(g_real_line_num,
            "syntax error : '%s', need '('", g_real_token_string);
        goto FAILED;
    }
    TREE_switch_det(t) = exp_list();
    setParent(t, TREE_switch_det(t));
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
    setParent(t,TREE_switch_body(t));
    popst();
    return t;
FAILED:
    prt2C("error in switch_stmt()");
    return t;
}


static Tree * select_stmt()
{
    Tree * t = NULL;
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


SCOPE * compound_stmt(Decl * para_list)
{
    Tree * t = NULL;
    SCOPE * s = NULL;
    Tree * last;
    INT cerr = 0;
    //enter a new sub-scope region
    SCOPE * cur_scope = enter_sub_scope(false);

    //Append parameters to declaration list of function body scope.
    Decl * lastdcl = get_last(SCOPE_decl_list(cur_scope));
    UINT pos = 0;
    for (; para_list != NULL; para_list = DECL_next(para_list), pos++) {
        if (DECL_dt(para_list) == DCL_VARIABLE) {
            //VARIABLE parameter should be the last parameter.
            ASSERT0(DECL_next(para_list) == NULL);
            continue;
        }

        Decl * declaration = new_decl(DCL_DECLARATION);
        DECL_spec(declaration) = DECL_spec(para_list);
        DECL_decl_list(declaration) = cp_decl(DECL_decl_list(para_list));
        PURE_DECL(declaration) = PURE_DECL(para_list);

        if (is_array(declaration)) {
            //Array type formal parameter is always be treated
            //as pointer type.
            //Convert ARRAY of ID to ARRAY of POINTER.
            //In C, the parameter of function that declarated as
            //'int a[10]' is not really array of a, but is a pointer
            //that pointed to array 'a', and should be 'int (*a)[10].
            //e.g: Convert 'void f(int a[10])' -> 'void f(int (*a)[10])'.
            declaration = trans_to_pointer(declaration, true);
        }

        DECL_is_formal_para(declaration) = true;
        add_next(&SCOPE_decl_list(cur_scope), &lastdcl, declaration);
        DECL_decl_scope(declaration) = cur_scope;
        DECL_formal_param_pos(declaration) = pos;

        lastdcl = declaration;

        //Append parameter list to symbol list of function body scope.
        SYM * sym = get_decl_sym(declaration);
        if (add_to_symtab_list(&SCOPE_sym_tab_list(cur_scope), sym)) {
            err(g_real_line_num, "'%s' already defined",
                g_real_token_string);
            goto FAILED;
        }
    }

    match(T_LLPAREN);
    s = cur_scope;
    declaration_list();

    //statement list
    cerr = g_err_msg_list.get_elem_count();
    last = NULL;
    for (;;) {
        if (g_real_token == T_END || g_real_token == T_NUL) {
            break;
        } else if (g_err_msg_list.get_elem_count() >= TOO_MANY_ERR) {
            goto FAILED;
        } else if (is_compound_terminal()) {
            break;
        }

        t = statement();

        ASSERT0(verify(t));

        if (last == NULL) {
            last = get_last(SCOPE_stmt_list(cur_scope));
        }
        add_next(&SCOPE_stmt_list(cur_scope), &last, t);

        last = get_last(t);

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


//expression_statement:
//  NULL
//  expression;
static Tree * exp_stmt()
{
    Tree * t = exp_list();
    //expression can be NULL
    if (match(T_SEMI) != ST_SUCC) {
        err(g_real_line_num, "syntax error : '%s', expected ';' be followed",
            g_real_token_string);
    }
    return t;
}


//statement:
//  labeled_statement // this already be recog in dispatch()
//  expression_list;
//  compound_statement
//  selection_statement
//  iteration_statement
//  jump_statement
static Tree * statement()
{
    Tree * t = NULL;
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
            t = NEWTN(TR_SCOPE);
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
            //It mMay be varirable or type declaration.
            if (is_in_first_set_of_declarator()) {
                //err(g_real_line_num,
                //    "'%s' is out of definition after or before block",
                //    g_real_token_string);
                declaration_list(); //Supported define variables anywhere.
            } else {
                err(g_real_line_num,
                    "syntax error : illegal used '%s'", g_real_token_string);
            }
        }
    }
    return t;
}


//Top level dispatch to parse DECLARATION or PLAUSE.
static Tree * dispatch()
{
    Enum * e = NULL;
    INT idx = -1;
    Decl * ut = NULL;
    Tree * t = NULL;
    switch (g_real_token) {
    case T_ID: // ID = (A-Z|a-z)( A-Z|a-z|0-9 )*
        //Here we cannot determined which non-terminal-charactor the T_ID
        //should be reduced correctly . Because T_ID could be reduced by
        //followed grammar :
        //  enumerator:
        //            identifier(T_ID)
        //
        //  direct-declarator:
        //            identifier(T_ID)
        //
        //  identifier-list:
        //            identifier(T_ID)
        //
        //  typedef-name:
        //            identifier(T_ID)
        //
        //  postfix-expression:
        //            identifier(T_ID)
        //
        //So all the first 4 cases are decl or type, only the last is ID
        //or enumerator used, and the last case should check whether the
        //ID has declared.
        //
        //We do NOT support such ambiguous C syntax:
        //    e.g: funtion return value type.
        //        foo() { ... }
        //    it is equal to
        //        int foo() { ... }
        //    This enforce parser has to look ahead many tokens.
        //    In order to keep parser succinct, we only
        //    support the latter, int foo() { ... }.
        if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut)) {
            //reduce to variable declaration
            declaration();
        } else if (findEnumConst(g_real_token_string,
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
    case T_FP:         // decimal e.g 3.14
    case T_FPF:        // decimal e.g 3.14
    case T_FPLD:       // decimal e.g 3.14
    case T_STRING:     // "abcd"
    case T_CHAR_LIST:  // 'abcd'
    case T_LPAREN:     // (
    case T_ADD:        // +
    case T_SUB:        // -
    case T_ASTERISK:   // *
    case T_BITAND:     // &
    case T_NOT:        // !
    case T_REV:        // ~ (reverse  a = ~a)
    case T_ADDADD:     // ++
    case T_SUBSUB:     // --
    case T_SIZEOF:     // sizeof
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
    ASSERT0(verify(t));
    return t;
}


//Initialize pool for parser.
void initParser()
{
    initKeyWordTab();
    g_pool_general_used = smpoolCreate(256, MEM_COMM);
    g_pool_tree_used = smpoolCreate(128, MEM_COMM);
    g_pool_st_used = smpoolCreate(64, MEM_COMM);
}


void finiParser()
{
    smpoolDelete(g_pool_general_used);
    smpoolDelete(g_pool_tree_used);
    smpoolDelete(g_pool_st_used);
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
INT Parser()
{
    ASSERT(g_hsrc && g_fe_sym_tab, ("must initialize them"));
    //base_type_spec:   one of
    //    type-name void char short int long float double signed
    //    unsigned struct union auto register extern static typedef const
    //    volatile enum
    //
    //declarator:  one of
    //    id * (
    //
    //stmt: one of
    //    id imm imml floatpoint string charlist { ( +
    //    - * & ! ~ ++ -- if goto
    //    break return continue do while switch case default for
    //    sizeof
    //
    //jmp_stmt :
    //    goto break return continue
    //
    //sel_stmt :
    //    if switch
    //iter :
    //    do while for
    //
    //exp :
    //    id imm imml floatpoint string charlist ( + -
    //    * & ! ~ ++ -- sizeof
    //
    //lab_stmt :
    //    id case default
    //
    //typedef_name :
    //    id
    //
    //enum_constant:
    //     id
    ASSERT0(g_hsrc);
    gettok(); //Get first token.

    //Create outermost scope for top region.
    g_cur_scope = new_scope();
    SCOPE_level(g_cur_scope) = GLOBAL_SCOPE; //First global scope
    for (;;) {
        if (g_real_token == T_END) {
            //dump_scope(g_cur_scope, DUMP_SCOPE_FUNC_BODY|DUMP_SCOPE_STMT_TREE);
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

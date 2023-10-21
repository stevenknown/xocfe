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

bool g_enable_c99_declaration = true;
xoc::LogMgr * g_logmgr = nullptr;
static List<Cell*> g_cell_list;
static xcom::Vector<UINT> g_realline2srcline;
static bool g_dump_token = false;

static Tree * statement();
static Tree * cast_exp();
static Tree * unary_exp();
static Tree * exp_list();
static Tree * exp_list();
static bool look_forward_token(INT num, ...);
static INT suck_tok();
static void suck_tok_to(INT placeholder, ...);

//Remove a token from head of list
static size_t remove_head_tok()
{
    Cell * c = g_cell_list.remove_head();
    if (c != nullptr) {
        size_t v = (size_t)CELL_val(c);
        free_cell(c);
        return v;
    }
    return 0;
}


static void setMapRealLineToSrcLine(UINT realline, UINT srcline)
{
    g_realline2srcline.set(realline, srcline);
}


static TOKEN gettok()
{
    TOKEN tok = getNextToken();
    ASSERT0(tok == g_cur_token);
    g_real_token = tok;
    g_real_token_string = g_cur_token_string;
    ASSERT0(g_src_line_num >= g_disgarded_line_num);
    g_real_line_num = g_src_line_num - g_disgarded_line_num;
    if (g_disgarded_line_num != 0) {
        //Map the real line to the line in input file, where input file
        //may be the output from preprocessor.
        setMapRealLineToSrcLine(g_real_line_num, g_src_line_num);
    }
    return g_real_token;
}


static INT suck_tok()
{
    TokenInfo * tki = nullptr;
    if ((tki = (TokenInfo*)remove_head_tok()) == nullptr) {
        gettok();
    } else {
        //Set the current token with head in token-info list
        g_real_token_string = const_cast<CHAR*>(TOKEN_INFO_name(tki));
        g_real_token = TOKEN_INFO_token(tki);
        g_real_line_num = TOKEN_INFO_lineno(tki);
    }

    return ST_SUCC;
}


void CParser::setLogMgr(LogMgr * logmgr)
{
    g_logmgr = logmgr;
}


static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    ASSERT0(p);
    ::memset((void*)p, 0, size);
    return p;
}


//Return true if parsing have to terminate.
bool CParser::isTerminateToken()
{
    return g_real_token == T_END || g_real_token == T_UNDEF;
}


//
//START CParser
//
//Initialize pool for parser.
void CParser::init(xoc::LogMgr * lm, CHAR const* srcfile)
{
    ASSERT0(lm);
    setLogMgr(lm);
    g_scope_count = 0;
    g_tree_count = TREE_ID_UNDEF + 1;
    g_decl_count = DECL_ID_UNDEF + 1;
    g_aggr_count = AGGR_ID_UNDEF + 1;
    g_aggr_anony_name_count = AGGR_ANONY_ID_UNDEF + 1;
    g_pool_general_used = smpoolCreate(256, MEM_COMM);
    g_pool_tree_used = smpoolCreate(128, MEM_COMM);
    g_pool_st_used = smpoolCreate(64, MEM_COMM);
    if (!initSrcFile(srcfile)) {
        return;
    }
    initLexer();
    g_lab2lineno.init();
    g_lab_used.init();
}


void CParser::destroy()
{
    clean_free_cell_list();
    destroy_scope_list();
    smpoolDelete(g_pool_general_used);
    smpoolDelete(g_pool_tree_used);
    smpoolDelete(g_pool_st_used);
    g_pool_general_used = nullptr;
    g_pool_tree_used = nullptr;
    g_pool_st_used = nullptr;
    finiLexer();
    setLogMgr(nullptr);
    finiSrcFile();
    g_lab2lineno.destroy();
    g_lab_used.destroy();
}


#ifdef _DEBUG_
//Verify the Tree node legality.
static bool verify(Tree * t)
{
    if (t == nullptr) { return true; }
    switch (t->getCode()) {
    case TR_SCOPE:
    case TR_CALL:
    case TR_ARRAY:
    case TR_TYPE_NAME:
    case TR_CVT:
    case TR_COND:
    case TR_LABEL:
    case TR_DECL:
        break;
    default: ASSERT0(TREE_token(t) != T_UNDEF && getTokenName(TREE_token(t)));
    }

    switch (t->getCode()) {
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
    case TR_PREP:
    case TR_DECL:
        break;
    default: ASSERTN(0, ("unknown tree type:%d", t->getCode()));
    } //end switch
    return true;
}
#endif


//Return nullptr indicate we haven't found it in 'l_list', and
//append 'label' to tail of the list as correct,
//otherwise return 'l'.
//Add a label into outmost scope of current function.
static LabelInfo * add_label(CHAR * name, INT lineno)
{
    LabelInfo * li;
    Scope * sc = g_cur_scope;
    while (sc && SCOPE_level(sc) != FUNCTION_SCOPE) {
        sc = SCOPE_parent(sc);
    }

    if (sc == nullptr) {
        err(g_real_line_num, "label must be located in function");
        return nullptr;
    }

    for (li = SCOPE_label_list(sc).get_head();
         li != nullptr; li = SCOPE_label_list(sc).get_next()) {
        if (::strcmp(SYM_name(LABELINFO_name(li)), name) == 0) {
            err(g_real_line_num, "label : '%s' already defined",name);
            return nullptr;
        }
    }

    //Allocate different LabelInfo for different lines.
    li = allocCustomerLabel(g_fe_sym_tab->add(name), g_pool_general_used);
    set_map_lab2lineno(li, lineno);
    SCOPE_label_list(sc).append_tail(li);
    return li;
}


//Record a label reference into outmost scope of current function.
static LabelInfo * add_ref_label(CHAR * name, INT lineno)
{
    LabelInfo * li;
    Scope * sc = g_cur_scope;
    while (sc != nullptr && SCOPE_level(sc) != FUNCTION_SCOPE) {
        sc = SCOPE_parent(sc);
    }
    if (sc == nullptr) {
        err(g_src_line_num,
            "label reference illegal, and it should be used in function.");
        return nullptr;
    }

    //Allocate different LabelInfo for different lines.
    li = allocCustomerLabel(g_fe_sym_tab->add(name), g_pool_general_used);
    set_lab_used(li);
    set_map_lab2lineno(li, lineno); //ONLY for debug-info or dumping
    SCOPE_ref_label_list(sc).append_tail(li);
    return li;
}


//list operation
//Append a token to tail of list
static void append_c_tail(size_t v)
{
    //static Cell * g_cell_list_head=nullptr;
    //static Cell * g_cell_list_tail=nullptr;
    Cell * c = newcell(0);
    CELL_val(c) = (LONGLONG)v;
    CELL_line_no(c) = g_src_line_num;
    g_cell_list.append_tail(c);
}


//Append a token to head of list
static void append_c_head(size_t v)
{
    Cell * c = newcell(0);
    CELL_val(c) = (LONGLONG)v;
    CELL_line_no(c) = g_src_line_num;
    g_cell_list.append_head(c);
}


//Append current token info descripte by 'g_cur_token','g_cur_token_string'
//and 'g_src_line_num'
static void append_tok_tail(TOKEN tok, CHAR * tokname, INT lineno)
{
    TokenInfo * tki = (TokenInfo*)xmalloc(sizeof(TokenInfo));
    Sym const* s = g_fe_sym_tab->add(tokname);
    TOKEN_INFO_name(tki) = SYM_name(s);
    TOKEN_INFO_token(tki) = tok;
    TOKEN_INFO_lineno(tki) = lineno;
    append_c_tail((size_t)tki);
}


//Append current token info descripte by 'g_cur_token','g_cur_token_string'
//and 'g_src_line_num'
static void append_tok_head(TOKEN tok, CHAR const* tokname, INT lineno)
{
    TokenInfo * tki = (TokenInfo*)xmalloc(sizeof(TokenInfo));
    Sym const* s = g_fe_sym_tab->add(tokname);
    TOKEN_INFO_name(tki) = SYM_name(s);
    TOKEN_INFO_token(tki) = tok;
    TOKEN_INFO_lineno(tki) = lineno;
    append_c_head((size_t)tki);
}


void CParser::dump_tok_list()
{
    Cell * c = g_cell_list.get_head();
    if (c != nullptr) {
        prt("\nTOKEN:");
        for (; c; c = g_cell_list.get_next()) {
            CHAR const* s = TOKEN_INFO_name((TokenInfo*)CELL_val(c));
            prt("'%s' ", s);
        }
        prt("\n");
    }
}


static INT is_compound_terminal()
{
    return (g_real_token == T_RLPAREN);
}


//Return true if 'tok' indicate terminal charactor, otherwise false.
bool CParser::inFirstSetOfExp(TOKEN tok)
{
    Decl * ut = nullptr;
    switch (tok) {
    case T_ID:
        if (isUserTypeExistInOuterScope(g_real_token_string, &ut)) {
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


bool Tree::is_type_quan(TOKEN tok)
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


bool Tree::is_type_spec(TOKEN tok)
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


bool Tree::is_stor_spec(TOKEN tok)
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


//Find if ID with named 'cl' exists and return the Decl.
static bool isIdExistInOuterScope(CHAR const* cl, OUT Decl ** d)
{
    return isDeclExistInOuterScope(cl, d);
}


static bool isFirstSetOfUnaryExp(TOKEN tok)
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


//Draw token until meet any TOKEN list in '...'
static void suck_tok_to(INT placeholder, ...)
{
    va_list arg;
    TOKEN tok;
    ASSERT0(sizeof(TOKEN) == sizeof(INT));
    for (;;) {
        if (CParser::isTerminateToken()) { break; }
        va_start(arg, placeholder);
        tok = (TOKEN)va_arg(arg, INT);
        while (tok != T_UNDEF) {
            if (tok == g_real_token) { goto FIN; }
            tok = (TOKEN)va_arg(arg, INT);
        }
        suck_tok();
    }
    va_end(arg);
FIN:
    va_end(arg);
    return;
}


Tree * CParser::id()
{
    Sym const* sym = g_fe_sym_tab->add(g_real_token_string);
    return id(sym, g_real_token);
}


Tree * CParser::id(Sym const* name, TOKEN tok)
{
    Tree * t = NEWTN(TR_ID);
    TREE_token(t) = tok;
    TREE_id_name(t) = name;
    return t;
}


UINT CParser::mapRealLineToSrcLine(UINT realline)
{
    return g_realline2srcline.get(realline);
}


static TOKEN reset_tok()
{
    TokenInfo * tki = nullptr;
    if ((tki = (TokenInfo*)remove_head_tok()) == nullptr) {
        return g_real_token;
    }

    //Set the current token with the head element in token_list.
    g_real_token_string = const_cast<CHAR*>(TOKEN_INFO_name(tki));
    g_real_token = TOKEN_INFO_token(tki);
    g_real_line_num = TOKEN_INFO_lineno(tki);
    return g_real_token;
}


STATUS CParser::match(TOKEN tok)
{
    if (g_real_token == tok) {
        suck_tok();
    } else {
        return ST_ERR;
    }
    if (g_dump_token && g_logmgr != nullptr) {
        note(g_logmgr, "LINE:%10d, TOKEN:%s\n",
             g_real_line_num, g_real_token_string);
    }
    return ST_SUCC;
}


//Pry next 'num' token info.
//n: represent the next N token to current token.
//   If n is 0, it will return current token.
static TOKEN look_next_token(INT n,
                             OUT CHAR ** tok_string,
                             OUT UINT * tok_line_num)
{
    TOKEN tok = T_UNDEF;
    if (n < 0) { return T_UNDEF; }

    if (n == 0) { return g_real_token; }

    INT count = g_cell_list.get_elem_count();
    if (count > 0) {
        //Pry in token_buffer
        if (n <= count) {
            //'n' can be finded in token-buffer
            Cell * c = g_cell_list.get_head_nth(n - 1);
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
            if (CParser::isTerminateToken()) {
                reset_tok();
                return g_real_token;
            }
            append_tok_tail(g_real_token, g_real_token_string, g_real_line_num);
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
        if (tok_string != nullptr) {
            *tok_string = g_real_token_string;
        }
        if (tok_line_num != nullptr) {
            *tok_line_num = g_real_line_num;
        }
        if (CParser::isTerminateToken()) {
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
static bool look_forward_token(INT num, ...)
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

    Cell * c = g_cell_list.get_head();
    if (c != nullptr) {
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
    Tree * t = CParser::exp();
    Tree * last = xcom::get_last(t);

    while (g_real_token == T_COMMA) {
        CParser::match(T_COMMA);
        Tree * nt = CParser::exp();
        if (nt == nullptr) {
            err(g_real_line_num, "miss patameter, syntax error : '%s'",
                g_real_token_string);
            return t;
        }
        xcom::add_next(&t, &last, nt);
    }
    return t;
}


//Process the first part of postfix expression:
//e.g:  a[10].u1.s++,  where a[10] is the first part,
//    .u1.s++ is the second part.
static Tree * primary_exp(MOD UINT * st)
{
    Tree * t = nullptr;
    switch (g_real_token) {
    case T_ID: {
        Enum * e = nullptr;
        INT idx = 0;
        if (findEnumVal(g_real_token_string, &e, &idx)) {
            t = NEWTN(TR_ENUM_CONST);
            TREE_enum(t) = e;
            TREE_enum_val_idx(t) = idx;
        } else {
            //Struct, Union, TYPEDEF-NAME should be
            //parsed during declaration().
            Decl * dcl = nullptr;
            t = CParser::id();
            if (!isIdExistInOuterScope(g_real_token_string, &dcl)) {
                err(g_real_line_num, "'%s' undeclared identifier",
                    g_real_token_string);
                CParser::match(T_ID);
                *st = ST_ERR;
                return nullptr;
            }
            TREE_id_decl(t) = dcl;
        }
        TREE_token(t) = g_real_token;
        CParser::match(T_ID);
        break;
    }
    case T_IMM:
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        t = buildInt((HOST_INT)xcom::xatoll(g_real_token_string, false));
        CParser::match(T_IMM);
        return t;
    case T_IMML:
        t = NEWTN(TR_IMML);
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_INT)xcom::xatoll(g_real_token_string, false);
        CParser::match(T_IMML);
        return t;
    case T_IMMU:
        t = NEWTN(TR_IMMU);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xcom::xatoll(g_real_token_string, false);
        CParser::match(T_IMMU);
        return t;
    case T_IMMUL:
        t = NEWTN(TR_IMMUL);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xcom::xatoll(g_real_token_string, false);
        CParser::match(T_IMMUL);
        return t;
    case T_FP:         // decimal e.g 3.14
        t = NEWTN(TR_FP);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        CParser::match(T_FP);
        break;
    case T_FPF:         // decimal e.g 3.14
        t = NEWTN(TR_FPF);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        CParser::match(T_FPF);
        break;
    case T_FPLD:         // decimal e.g 3.14
        t = NEWTN(TR_FPLD);
        TREE_token(t) = g_real_token;
        TREE_fp_str_val(t) = g_fe_sym_tab->add(g_real_token_string);
        CParser::match(T_FPLD);
        break;
    case T_STRING: { // "abcd"
        t = NEWTN(TR_STRING);
        TREE_token(t) = g_real_token;
        CHAR * tbuf = nullptr;
        UINT tbuflen = 0;
        Sym const* sym = g_fe_sym_tab->add(g_real_token_string);
        CParser::match(T_STRING);

        //Concatenate string.
        for (; g_real_token == T_STRING; CParser::match(T_STRING)) {
            if (tbuf == nullptr) {
                tbuflen = (UINT)strlen(SYM_name(sym)) +
                          (UINT)strlen(g_real_token_string) + 1;
                tbuf = (CHAR*)malloc(tbuflen);
                sprintf(tbuf, "%s%s", SYM_name(sym), g_real_token_string);
            } else {
                tbuflen += (UINT)strlen(g_real_token_string);
                CHAR * ttbuf = (CHAR*)malloc(tbuflen);
                sprintf(ttbuf, "%s%s", tbuf, g_real_token_string);
                free(tbuf);
                tbuf = ttbuf;
            }
        }

        if (tbuf != nullptr) {
            sym = g_fe_sym_tab->add(tbuf);
            free(tbuf);
        }
        TREE_string_val(t) = sym;
        return t;
    }
    case T_CHAR_LIST:  // 'abcd'
        t = NEWTN(TR_IMM);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = xctoi(g_real_token_string);
        CParser::match(T_CHAR_LIST);
        break;
    case T_LPAREN:
        CParser::match(T_LPAREN);
        t = exp_list();
        if (CParser::match(T_RPAREN) != ST_SUCC) {
            err(g_real_line_num, "miss ')' after expression");
            *st = ST_ERR;
            return nullptr;
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
            Tree * array_root = nullptr;
            while (g_real_token == T_LSPAREN) {
                CParser::match(T_LSPAREN);
                array_root = NEWTN(TR_ARRAY);
                TREE_array_base(array_root) = t;
                TREE_array_indx(array_root) = CParser::exp();
                Tree::setParent(array_root, t);
                Tree::setParent(array_root, TREE_array_indx(array_root));
                if (TREE_array_indx(array_root) == nullptr) {
                    err(g_real_line_num, "array index cannot be nullptr");
                    return nullptr;
                }
                if (CParser::match(T_RSPAREN) != ST_SUCC) {
                    err(g_real_line_num, "miss ']'");
                    return nullptr;
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
            CParser::match(T_LPAREN);
            Tree * tp = NEWTN(TR_CALL);
            TREE_fun_exp(tp) = t;
            TREE_para_list(tp) = param_list();
            if (CParser::match(T_RPAREN) != ST_SUCC) {
                err(g_real_line_num, "miss ')'");
                return t;
            }
            Tree::setParent(tp, t);
            Tree::setParent(tp, TREE_para_list(tp));
            t = tp;
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
    case T_DOT: //direct struct member reference
    case T_ARROW: //indirect struct member reference
        {
            Tree * mem_ref = nullptr;
            if (g_real_token == T_DOT) {
                mem_ref = NEWTN(TR_DMEM);
            } else {
                mem_ref = NEWTN(TR_INDMEM);
            }
            TREE_token(mem_ref) = g_real_token;
            CParser::match(g_real_token);
            TREE_base_region(mem_ref) = t;
            if (g_real_token != T_ID) {
                err(g_real_line_num, "member name is needed");
                return t;
            }
            TREE_field(mem_ref) = CParser::id();
            CParser::match(T_ID);
            Tree::setParent(mem_ref, t);
            Tree::setParent(mem_ref, TREE_field(mem_ref));
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
            CParser::match(T_ADDADD);
            Tree * tp = NEWTN(TR_POST_INC);
            TREE_token(tp) = g_real_token;
            TREE_inc_exp(tp) = t;
            Tree::setParent(tp, t);
            t = tp;
            if (t == nullptr) {
                err(g_real_line_num, "unary expression is needed");
                return t;
            }
            if (g_real_token == T_LSPAREN) {
                goto AGAIN;
            }
            if (g_real_token == T_ADDADD) {
                err(g_real_line_num, "'++' needs l-value");
                CParser::match(T_ADDADD);
                return t;
            }
            if (g_real_token == T_SUBSUB) {
                err(g_real_line_num, "'--' needs l-value");
                CParser::match(T_SUBSUB);
                return t;
            }
        }
        break;
    case T_SUBSUB: //post decrease
        {
            CParser::match(T_SUBSUB);
            Tree * tp = NEWTN(TR_POST_DEC);
            TREE_token(tp) = g_real_token;
            TREE_dec_exp(tp) = t;
            Tree::setParent(tp, t);
            t = tp;
            if (t == nullptr) {
                err(g_real_line_num, "unary expression is needed");
                return t;
            }
            if (g_real_token == T_LSPAREN) {
                goto AGAIN;
            }
            if (g_real_token == T_ADDADD) {
                err(g_real_line_num, "'++' needs l-value");
                CParser::match(T_ADDADD);
                return t;
            }
            if (g_real_token == T_SUBSUB) {
                err(g_real_line_num, "'--' needs l-value");
                CParser::match(T_SUBSUB);
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
    Tree * t = nullptr;
    TOKEN tok = T_UNDEF;
    CHAR * tok_string = nullptr;
    if (g_real_token == T_LPAREN) {
        //Next exp either (exp) or (type_name), so after several '(', there
        //must be T_ID appearing .
        tok = look_next_token(1, &tok_string, nullptr);

        if (Tree::is_type_spec(tok) ||
            Tree::is_type_quan(tok) ||
            Tree::is_stor_spec(tok)) {
            t = NEWTN(TR_TYPE_NAME);
            if (CParser::match(T_LPAREN) != ST_SUCC) {
                err(g_real_line_num, "except '('");
                goto FAILED;
            }
            TREE_type_name(t) = type_name();
            if (CParser::match(T_RPAREN) != ST_SUCC) {
                err(g_real_line_num, "except ')'");
                goto FAILED;
            }
            if (TREE_type_name(t) == nullptr) {
                err(g_real_line_num, "except 'type-name'");
                goto FAILED;
            }
        } else if (tok == T_ID) {
            //Record a User defined type
            Decl * ut = nullptr;
            ASSERT0(tok_string != nullptr);
            if (isUserTypeExistInOuterScope(tok_string, &ut)) {
                //User defined Type via 'typedef'.
                t = NEWTN(TR_TYPE_NAME);
                if (CParser::match(T_LPAREN) != ST_SUCC) {
                    err(g_real_line_num, "except '('");
                    goto FAILED;
                }
                //Reference a type to do type-cast.
                TREE_type_name(t) = type_name();
                if (CParser::match(T_RPAREN) != ST_SUCC) {
                    err(g_real_line_num, "except ')'");
                    goto FAILED;
                }
                if (TREE_type_name(t) == nullptr) {
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
    } else if (isFirstSetOfUnaryExp(g_real_token)) {
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
    Tree * t = nullptr;
    switch (g_real_token) {
    case T_IMM:
    case T_IMML:
    case T_IMMU:
    case T_IMMUL:
    case T_FP: //decimal e.g 3.14
    case T_FPF: //decimal e.g 3.14
    case T_FPLD: //decimal e.g 3.14
    case T_STRING: //"abcd"
    case T_CHAR_LIST: //'abcd'
    case T_LPAREN: //(exp)
    case T_ID:
        t = postfix_exp();
        break;
    case T_ASTERISK:
        t = NEWTN(TR_DEREF);
        TREE_token(t) = g_real_token;
        CParser::match(T_ASTERISK);
        TREE_lchild(t) = cast_exp();
        Tree::setParent(t, TREE_lchild(t));
        break;
    case T_BITAND: {
        CParser::match(T_BITAND);
        Tree * base = cast_exp();
        t = buildLda(base);
        break;
    }
    case T_ADDADD:
        t = NEWTN(TR_INC);
        TREE_token(t) = g_real_token;
        CParser::match(T_ADDADD);
        TREE_inc_exp(t) = unary_exp();
        Tree::setParent(t, TREE_inc_exp(t));
        if (TREE_inc_exp(t) == nullptr) {
            err(g_real_line_num, "unary expression is needed");
            goto FAILED;
        }
        break;
    case T_SUBSUB:
        t = NEWTN(TR_DEC);
        TREE_token(t) = g_real_token;
        CParser::match(T_SUBSUB);
        TREE_dec_exp(t) = unary_exp();
        Tree::setParent(t,TREE_dec_exp(t));
        if (TREE_inc_exp(t) == nullptr) {
            err(g_real_line_num, "unary expression is needed");
            goto FAILED;
        }
        break;
    case T_ADD:
        t = NEWTN(TR_PLUS);
        TREE_token(t) = g_real_token;
        CParser::match(T_ADD);
        TREE_lchild(t) = cast_exp();
        Tree::setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == nullptr) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_SUB:
        t = NEWTN(TR_MINUS);
        TREE_token(t) = g_real_token;
        CParser::match(T_SUB);
        TREE_lchild(t) = cast_exp();
        Tree::setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == nullptr) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_SIZEOF:
        t = NEWTN(TR_SIZEOF);
        TREE_token(t) = g_real_token;
        CParser::match(T_SIZEOF);
        TREE_sizeof_exp(t) = unary_or_LP_typename_RP();
        Tree::setParent(t, TREE_sizeof_exp(t));
        break;
    case T_REV:
        t = NEWTN(TR_REV);
        TREE_token(t) = g_real_token;
        CParser::match(T_REV);
        TREE_lchild(t) = cast_exp();
        Tree::setParent(t, TREE_lchild(t));
        if (TREE_lchild(t) == nullptr) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    case T_NOT:
        t = NEWTN(TR_NOT);
        TREE_token(t) = g_real_token;
        CParser::match(T_NOT);
        TREE_lchild(t) = cast_exp();
        Tree::setParent(t,TREE_lchild(t));
        if (TREE_lchild(t) == nullptr) {
            err(g_real_line_num, "cast expression is needed");
            goto FAILED;
        }
        break;
    default:;
    }
    return t;
FAILED:
    prt("error in unary_exp()");
    return t;
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
    Tree * t = nullptr, * p;
    p = unary_or_LP_typename_RP();
    if (p == nullptr) { return nullptr; }

    //It might be follow-set token.
    //if (p == nullptr) {
    //    err(g_real_line_num,
    //        "syntax error : '%s' , except a identifier or typedef-name",
    //        g_real_token_string);
    //    goto FAILED;
    //}

    if (p->getCode() == TR_TYPE_NAME) {
        Tree * srcexp = cast_exp();
        if (srcexp == nullptr) {
            err(g_real_line_num, "cast expression cannot be nullptr");
            goto FAILED;
        }
        t = buildCvt(p, srcexp);
        if (TREE_cvt_exp(t) == nullptr) {
            err(g_real_line_num, "cast expression cannot be nullptr");
            goto FAILED;
        }
    } else {
        t = p;
    }
    return t;
FAILED:
    prt("error in cast_exp()");
    return t;
}


static Tree * multiplicative_exp()
{
    Tree * t = cast_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_ASTERISK ||
           g_real_token == T_DIV ||
           g_real_token == T_MOD) {
        p = NEWTN(TR_MULTI);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
            // a*b*c  =>
            //             *
            //           /  |
            //           *  c
            //          / |
            //         a  b
        CParser::match(g_real_token);
        TREE_rchild(p) = cast_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;

FAILED:
    prt("error in multiplicative_exp()");
    return t;
}


static Tree * additive_exp()
{
    Tree * t = multiplicative_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_ADD || g_real_token == T_SUB) {
        p = NEWTN(TR_ADDITIVE);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a+b-c  =>
           //              -
           //             / |
           //            +  c
           //           / |
           //          a  b
        CParser::match(g_real_token);
        TREE_rchild(p) = multiplicative_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;

FAILED:
    prt("error in additive_exp()");
    return t;
}


static Tree * shift_exp()
{
    Tree * t = additive_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_LSHIFT || g_real_token == T_RSHIFT) {
        p = NEWTN(TR_SHIFT);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a<<b<<c  =>
           //              <<
           //             /  |
           //            <<  c
           //           /  |
           //          a   b
        CParser::match(g_real_token);
        TREE_rchild(p) = additive_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:

    prt("error in shift_exp()");
    return t;
}


static Tree * relational_exp()
{
    Tree * t = shift_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_LESSTHAN || g_real_token == T_MORETHAN ||
           g_real_token == T_NOMORETHAN || g_real_token == T_NOLESSTHAN) {
        p = NEWTN(TR_RELATION);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a<b<c   =>
           //              <
           //             / |
           //            <  c
           //           / |
           //          a  b
        CParser::match(g_real_token);
        TREE_rchild(p) = shift_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in relational_exp()");
    return t;
}


static Tree * equality_exp()
{
    Tree * t = relational_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_EQU || g_real_token == T_NOEQU) {
        p = NEWTN(TR_EQUALITY);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a==b!=c   =>
           //              !=
           //             /  |
           //            ==  c
           //           /  |
           //          a   b
        CParser::match(g_real_token);
        TREE_rchild(p) = relational_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in equality_exp()");
    return t;
}


static Tree * AND_exp()
{
    Tree * t = equality_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_BITAND) {
        p = NEWTN(TR_INCLUSIVE_AND);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a&b&c   =>  &
           //             / |
           //            &  c
           //           / |
           //          a  b
        CParser::match(T_BITAND);
        TREE_rchild(p) = equality_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in AND_exp()");
    return t;
}


static Tree * exclusive_OR_exp()
{
    Tree * t = AND_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_XOR) {
        p = NEWTN(TR_XOR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
           //  a^b^c   =>  ^
           //             / |
           //            ^  c
           //           / |
           //          a  b
        CParser::match(T_XOR);
        TREE_rchild(p) = AND_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in exclusive_OR_exp()");
    return t;
}


static Tree * inclusive_OR_exp()
{
    Tree * t = exclusive_OR_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_BITOR) {
        p = NEWTN(TR_INCLUSIVE_OR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
        //  a|b|c   =>  |
        //             / |
        //            |  c
        //           / |
        //          a  b
        CParser::match(T_BITOR);
        TREE_rchild(p) = exclusive_OR_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in inclusive_OR_exp()");
    return t;
}


static Tree * logical_AND_exp()
{
    Tree * t = inclusive_OR_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_AND) {
        p = NEWTN(TR_LOGIC_AND);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
        //a && b && c =>
        //              &&
        //             /  |
        //            &&  c
        //           /  |
        //           a  b
        CParser::match(T_AND);
        TREE_rchild(p) = inclusive_OR_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in logical_AND_exp()");
    return t;
}


static Tree * logical_OR_exp()
{
    Tree * t = logical_AND_exp(), * p = nullptr;
    if (t == nullptr) { return nullptr; }
    while (g_real_token == T_OR) {
        p = NEWTN(TR_LOGIC_OR);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
        //  a||b||c  =>  ||
        //              /  |
        //             ||  c
        //            /  |
        //           a   b
        CParser::match(T_OR);
        TREE_rchild(p) = logical_AND_exp();
        Tree::setParent(p, TREE_rchild(p));
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "'%s': right operand cannot be nullptr",
                TOKEN_INFO_name(get_token_info(TREE_token(p))));
            goto FAILED;
        }
        t = p;
    }
    return t;
FAILED:
    prt("error in logical_OR_exp()");
    return t;
}


//logical_OR_expression ? expression : conditional_expression
Tree * CParser::conditional_exp()
{
    Tree * t = logical_OR_exp();
    if (g_real_token == T_QUES_MARK) {
        CParser::match(T_QUES_MARK);
        Tree * p = NEWTN(TR_COND);
        TREE_det(p) = t;
        Tree::setParent(p,t);
        t = p;

        TREE_true_part(t) = exp_list();
        Tree::setParent(t,TREE_true_part(t));
        if (CParser::match(T_COLON) != ST_SUCC) {
            err(g_real_line_num, "condition expression is incomplete");
            goto FAILED;
        }
        TREE_false_part(t) = exp_list();
        Tree::setParent(t,TREE_false_part(t));
    }
    return t;
FAILED:
    prt("error in conditional_expt()");
    return t;
}


Tree * CParser::exp()
{
    Tree * t = conditional_exp(), * p;
    if (t == nullptr) { return nullptr; }
    if (is_assign_op(g_real_token)) {
        p = NEWTN(TR_ASSIGN);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        Tree::setParent(p, TREE_lchild(p));
        CParser::match(g_real_token);
        TREE_rchild(p) = exp();
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "expression miss r-value");
            goto FAILED;
        }
        Tree::setParent(p, TREE_rchild(p));
        t = p;
    }
    return t;
FAILED:
    prt("error in expt()");
    return t;
}


Tree * exp_list()
{
    Tree * t = CParser::exp();
    Tree * last = xcom::get_last(t);
    while (g_real_token == T_COMMA) {
        CParser::match(T_COMMA);
        Tree * nt = CParser::exp();
        xcom::add_next(&t, &last, nt);
        last = xcom::get_last(nt);
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
    Tree * t = nullptr;
    switch (g_real_token) {
    case T_GOTO:
        t = NEWTN(TR_GOTO);
        TREE_token(t) = g_real_token;
        CParser::match(T_GOTO);
        if (g_real_token != T_ID) {
            err(g_real_line_num, "target address label needed for 'goto'");
            return t;
        }
        TREE_lab_info(t) = add_ref_label(g_real_token_string, g_real_line_num);
        CParser::match(T_ID);
        if (CParser::match(T_SEMI) != ST_SUCC) {
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
        CParser::match(T_BREAK);
        if (CParser::match(T_SEMI) != ST_SUCC) {
            err(g_real_line_num, "miss ';'");
            return t;
        }
        break;
    case T_RETURN:
        t = NEWTN(TR_RETURN);
        TREE_token(t) = g_real_token;
        CParser::match(T_RETURN);
        if (g_real_token != T_SEMI) {
            TREE_ret_exp(t) = exp_list();
            Tree::setParent(t,TREE_ret_exp(t));
        }
        if (CParser::match(T_SEMI) != ST_SUCC) {
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
        CParser::match(T_CONTINUE);
        if (CParser::match(T_SEMI) != ST_SUCC) {
            err(g_real_line_num, "miss ';'");
            return t;
        }
        break;
    default:;
    }
    return t;
}


static TokenList * matchTokenTillNewLine()
{
    //Match token till NEWLINE.
    g_enable_newline_token = true;
    TokenList * last = nullptr;
    TokenList * head = nullptr;
    while (g_real_token != T_NEWLINE && g_real_token != T_UNDEF) {
        TokenList * tl = (TokenList*)xmalloc(sizeof(TokenList));
        TL_tok(tl) = g_real_token;
        switch (g_real_token) {
        case T_IMM:
        case T_IMML:
        case T_IMMU:
        case T_IMMUL:
            TL_imm(tl) = (UINT)xcom::xatoll(g_real_token_string, false);
            break;
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
        xcom::add_next(&head, &last, tl);
        if (CParser::match(g_real_token) == ST_ERR) {
            break;
        }
    }

    g_enable_newline_token = false;
    if (g_real_token == T_NEWLINE) {
        CParser::match(T_NEWLINE);
    }
    return head;
}


static Tree * sharp_start_stmt()
{
    Tree * t = nullptr;
    ASSERT0(g_real_token == T_SHARP);
    CParser::match(T_SHARP);
    switch (g_real_token) {
    case T_PRAGMA:
        t = NEWTN(TR_PRAGMA);
        TREE_token(t) = g_real_token;
        CParser::match(T_PRAGMA);
        break;
    case T_IMM:
        //Current line generated by preprocessor.
        //e.g: # 1 "test/compile/prc.c"
        g_disgarded_line_num++;
        t = NEWTN(TR_PREP);
        TREE_token(t) = g_real_token;
        CParser::match(T_IMM);
        break;
    default:
        err(g_real_line_num,
            "illegal use '#', unknown command");
        return nullptr;
    }

    TokenList * h = matchTokenTillNewLine();
    xcom::add_next(&TREE_token_lst(t), h);
    return t;
}


static Tree * label_stmt()
{
    Tree * t = nullptr;
    switch (g_real_token) {
    case T_ID:
        t = NEWTN(TR_LABEL);
        TREE_token(t) = g_real_token;
        if ((TREE_lab_info(t) = add_label(g_real_token_string,
                                          g_real_line_num)) == nullptr) {
            err(g_real_line_num, "illegal label '%s' defined",
                g_real_token_string);
            return t;
        }
        CParser::match(T_ID);
        if (CParser::match(T_COLON) != ST_SUCC) {
            err(g_real_line_num, "label defined incompletely");
            return t;
        }
        break;
    case T_CASE:
        {
            LONGLONG idx = 0;
            Tree * nt = nullptr;
            CParser::match(T_CASE);
            if (!is_sst_exist(st_DO) &&
                !is_sst_exist(st_WHILE) &&
                !is_sst_exist(st_FOR) &&
                !is_sst_exist(st_SWITCH)) {
                err(g_real_line_num, "invalid use 'case'");
                return t;
            }

            t = NEWTN(TR_CASE);
            TREE_token(t) = g_real_token;
            nt = CParser::conditional_exp(); //case expression must be constant.
            if (!computeConstExp(nt, &idx, 0)) {
                err(g_real_line_num, "expected constant expression");
                return t;
            }
            if (computeMaxBitSizeForValue((TMWORD)idx) >
                (BYTE_PER_INT * HOST_BIT_PER_BYTE)) {
                err(g_real_line_num, "bitsize of const is more than %dbit",
                    (sizeof(TREE_case_value(t)) * HOST_BIT_PER_BYTE));
                return t;
            }
            TREE_case_value(t) = (INT)idx;
            if (CParser::match(T_COLON) != ST_SUCC) {
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

            CParser::match(T_DEFAULT);
            if (!is_sst_exist(st_DO) &&
               !is_sst_exist(st_WHILE) &&
               !is_sst_exist(st_FOR) &&
               !is_sst_exist(st_SWITCH)) {
                err(g_real_line_num, "invalid use 'default'");
                return t;
            }
            if (CParser::match(T_COLON) != ST_SUCC) {
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
    CParser::match(T_DO);

    //do-body
    pushst(st_DO, 0); //push down inherit properties
    TREE_dowhile_body(t) = statement();
    Tree::setParent(t, TREE_dowhile_body(t));
    popst();

    if (CParser::match(T_WHILE) != ST_SUCC) { //while
        err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
        goto FAILED;
    }

    //determination
    if (CParser::match(T_LPAREN) != ST_SUCC) { //(
        err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
        goto FAILED;
    }

    TREE_dowhile_det(t) = exp_list();
    Tree::setParent(t, TREE_dowhile_det(t));
    if (TREE_dowhile_det(t) == nullptr) {
        err(g_real_line_num, "while determination cannot be nullptr");
        goto FAILED;
    }

    if (CParser::match(T_RPAREN) != ST_SUCC) { //)
        err(g_real_line_num, "miss ')'");
        goto FAILED;
    }

    if (g_real_token != T_SEMI ) { //;
        err(g_real_line_num, "miss ';' after 'while'");
        goto FAILED;
    }

    return t;

FAILED:
    prt("error in do_while_stmt()");
    return t;
}


static Tree * while_do_stmt()
{
    Tree * t = NEWTN(TR_WHILE);
    TREE_token(t) = g_real_token;
    CParser::match(T_WHILE);

    //determination
    if (CParser::match(T_LPAREN) != ST_SUCC) { //(
        err(g_real_line_num, "syntax error : '%s'", g_real_token_string);
        goto FAILED;
    }
    TREE_whiledo_det(t) =  exp_list();
    Tree::setParent(t, TREE_whiledo_det(t));
    if (TREE_whiledo_det(t) == nullptr) {
        err(g_real_line_num, "while determination cannot be nullptr");
        goto FAILED;
    }
    if (CParser::match(T_RPAREN) != ST_SUCC) { //)
        err(g_real_line_num, "miss ')'");
        goto FAILED;
    }

    //while-body
    pushst(st_WHILE,0); //push down inherit properties
    TREE_whiledo_body(t) = statement();
    Tree::setParent(t,TREE_whiledo_body(t));
    popst();
    return t;
FAILED:
    prt("error in while_do_stmt()");
    return t;
}


Tree * for_stmt()
{
    Tree * t = NEWTN(TR_FOR);
    Tree * res = t;
    TREE_token(t) = g_real_token;
    CParser::match(T_FOR);
    if (CParser::match(T_LPAREN) != ST_SUCC) {
        err(g_real_line_num, "synatx error : '%s', need '('",
            g_real_token_string);
        return t;;
    }

    //C89 specified init-variable declared in init-field of FOR is belong
    //to function level scope.
    if (g_enable_c99_declaration) {
        push_scope(false);

        //Should record for-stmt's init-list scope.
        TREE_for_scope(t) = g_cur_scope;
    }

    Tree * decl_list = declaration_list();
    if (decl_list == nullptr) {
        //initializing expression
        TREE_for_init(t) = exp_list();
        Tree::setParent(t, TREE_for_init(t));

        //EXPRESSION does not swallow the token ';'.
        if (CParser::match(T_SEMI) != ST_SUCC) {
            err(g_real_line_num, "miss ';' before determination");
            goto FAILED;
        }
    } else {
        xcom::insertbefore(&res, res, decl_list);
    }

    TREE_for_det(t) = exp_list();
    Tree::setParent(t,TREE_for_det(t));
    //EXPRESSION does not swallow the token ';'.
    if (CParser::match(T_SEMI) != ST_SUCC) {
        err(g_real_line_num, "miss ';' before step expression");
        goto FAILED;
    }

    TREE_for_step(t) = exp_list();
    Tree::setParent(t, TREE_for_step(t));
    //EXPRESSION does not swallow the token ')'.
    if (CParser::match(T_RPAREN) != ST_SUCC) {
        err(g_real_line_num, "miss ')' after step expression");
        goto FAILED;
    }

    pushst(st_DO, 0); //push down inherit properties
    TREE_for_body(t) = statement();
    Tree::setParent(t, TREE_for_body(t));
    popst();

    if (g_enable_c99_declaration) {
        pop_scope();
    }
    return res;

FAILED:
    if (g_enable_c99_declaration) {
        pop_scope();
    }
    prt("error in for_stmt()");
    return res;
}


static Tree * iter_stmt()
{
    Tree * t = nullptr;
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
    CParser::match(T_IF);

    //determination
    if (g_real_token != T_LPAREN) { //(
        err(g_real_line_num,
            "syntax error : '%s', determination must enclosed by '(')'",
            g_real_token_string);
        goto FAILED;
    }
    CParser::match(T_LPAREN);

    TREE_if_det(t) = exp_list();
    Tree::setParent(t, TREE_if_det(t));
    if (TREE_if_det(t) == nullptr) {
        err(g_real_line_num, "'if' determination cannot be nullptr");
        goto FAILED;
    }

    if (CParser::match(T_RPAREN) != ST_SUCC) { // )
        err(g_real_line_num, "miss ')'");
        goto FAILED;
    }

    //true part
    TREE_if_true_stmt(t) = statement();
    Tree::setParent(t,TREE_if_true_stmt(t));

    if (g_real_token == T_ELSE) {
        CParser::match(T_ELSE);
        TREE_if_false_stmt(t) = statement();
        Tree::setParent(t,TREE_if_false_stmt(t));
    }
    return t;
FAILED:
    prt("error in if_stmt()");
    return t;
}


static Tree * switch_stmt()
{
    Tree * t = NEWTN(TR_SWITCH);
    TREE_token(t) = g_real_token;
    CParser::match(T_SWITCH);

    //determination
    if (CParser::match(T_LPAREN)  != ST_SUCC) { //(
        err(g_real_line_num,
            "syntax error : '%s', need '('", g_real_token_string);
        goto FAILED;
    }
    TREE_switch_det(t) = exp_list();
    Tree::setParent(t, TREE_switch_det(t));
    if (TREE_switch_det(t) == nullptr) {
        err(g_real_line_num, "switch determination cannot be nullptr");
        goto FAILED;
    }
    if (CParser::match(T_RPAREN) != ST_SUCC) { // )
        err(g_real_line_num, "miss ')'");
        goto FAILED;
    }

    pushst(st_DO,0); //push down inherit properties
    TREE_switch_body(t) = statement();
    Tree::setParent(t,TREE_switch_body(t));
    popst();
    return t;
FAILED:
    prt("error in switch_stmt()");
    return t;
}


static Tree * select_stmt()
{
    Tree * t = nullptr;
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


//Append parameters to declaration list of function body scope.
static bool append_parameters(Scope * cur_scope, Decl const* para_list)
{
    Decl * lastdcl = xcom::get_last(cur_scope->getDeclList());
    UINT pos = 0;
    for (; para_list != nullptr; para_list = DECL_next(para_list), pos++) {
        if (para_list->is_dt_var()) {
            ASSERTN(DECL_next(para_list) == nullptr,
                    ("VARIABLE parameter should be the last parameter."));
            continue;
        }

        Decl * declaration = newDecl(DCL_DECLARATION);
        DECL_spec(declaration) = DECL_spec(para_list);
        DECL_decl_list(declaration) = dupDecl(DECL_decl_list(para_list));
        DECL_trait(declaration) = DECL_trait(para_list);
        if (declaration->is_array()) {
            //Array type formal parameter is always be treated
            //as pointer type.
            //Convert ARRAY of ID to ARRAY of POINTER.
            //In C language, the parameter of function that declarated as
            //'int a[10]' is not really array of a, but is a pointer
            //that pointed to array 'a', and should be 'int (*a)[10].
            //e.g: Convert 'void f(int a[10])' -> 'void f(int (*a)[10])'.
            declaration->convertToPointerType();
        }

        DECL_is_formal_param(declaration) = true;
        xcom::add_next(&SCOPE_decl_list(cur_scope), &lastdcl, declaration);
        DECL_decl_scope(declaration) = cur_scope;
        DECL_formal_param_pos(declaration) = pos;

        lastdcl = declaration;

        //Append parameter list to symbol list of function body scope.
        Sym const* sym = declaration->get_decl_sym();
        if (g_cur_scope->addToSymList(sym) != nullptr) {
            err(g_real_line_num, "'%s' already defined",
                g_real_token_string);
            return false;
        }
    }
    return true;
}


static bool statement_list(Scope * cur_scope)
{
    INT cerr = g_err_msg_list.get_elem_count();
    Tree * last = nullptr;
    for (;;) {
        if (CParser::isTerminateToken()) {
            break;
        }
        if (g_err_msg_list.get_elem_count() >= TOO_MANY_ERR) {
            return false;
        }
        if (is_compound_terminal()) {
            break;
        }
        Tree * t = statement();
        ASSERT0(verify(t));
        if (last == nullptr) {
            last = xcom::get_last(cur_scope->getStmtList());
        }
        xcom::add_next(&SCOPE_stmt_list(cur_scope), &last, t);

        last = xcom::get_last(t);
        if ((cerr != (INT)g_err_msg_list.get_elem_count()) ||
            (cerr > 0 && t == nullptr)) {
            suck_tok_to(0, T_SEMI, T_RLPAREN, T_END, T_UNDEF);
        }

        if (g_real_token == T_SEMI) {
            CParser::match(T_SEMI);
        }

        cerr = g_err_msg_list.get_elem_count();
    }
    return true;
}


Scope * CParser::compound_stmt(Decl * para_list)
{
    //Enter a new sub-scope region.
    Scope * cur_scope = push_scope(false);
    Scope * s = nullptr;
    Tree * t;
    if (!append_parameters(cur_scope, para_list)) {
        goto FAILED;
    }

    CParser::match(T_LLPAREN);
    s = cur_scope;

    //Function local variable declaration list.
    t = declaration_list();
    if (t != nullptr) {
        cur_scope->addStmt(t);
    }

    //Statement list.
    if (!statement_list(cur_scope)) {
        goto FAILED;
    }

    if (CParser::match(T_RLPAREN) != ST_SUCC) {
        err(g_real_line_num, "miss '}'");
        goto FAILED;
    }

    //back to outer region
    pop_scope();
    return s;
FAILED:
    CParser::match(T_RLPAREN);
    pop_scope();
    return s;
}


//expression_statement:
//  nullptr
//  expression;
static Tree * exp_stmt()
{
    Tree * t = exp_list();
    //expression can be nullptr
    if (CParser::match(T_SEMI) != ST_SUCC) {
        err(g_real_line_num, "syntax error : '%s', expected ';' be followed",
            g_real_token_string);
    }
    return t;
}


static bool is_align_token(Sym const* sym)
{
    return ::strcmp(sym->getStr(), "align") == 0;
}


static bool process_pragma_imm(TokenList const* tl, UINT * imm)
{
    switch (TL_tok(tl)) {
    case T_IMM:
    case T_IMML:
    case T_IMMU:
    case T_IMMUL:
        *imm = (UINT)TL_imm(tl);
        return true;
    default:
        return false;
    }
    UNREACHABLE();
    return false;
}


static bool is_valid_alignment(UINT align)
{
    switch (align) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
        return true;
    default:;
    }
    return false;
}


//Alignment format is: pragma align (IMM)
//tl: the token-list indicates the 'align' token.
//Return true if the pragma is processed correctly, otherwise return false.
static bool process_pragma_align(TokenList const** tl)
{
    ASSERT0(TL_tok(*tl) == T_ID && TL_str(*tl) && is_align_token(TL_str(*tl)));
    *tl = TL_next(*tl);
    if (TL_tok(*tl) != T_LPAREN) { return false; }

    UINT align = g_alignment;
    *tl = TL_next(*tl);
    if (!process_pragma_imm(*tl, &align)) {
        return false;
    }

    if (!is_valid_alignment(align)) {
        warn(g_real_line_num, "alignment should be one of 1, 2, 4, 8, 16");
        return false;
    }

    *tl = TL_next(*tl);
    if (TL_tok(*tl) != T_RPAREN) { return false; }

    *tl = TL_next(*tl);
    g_alignment = align;
    return true;
}


static void process_pragma(Tree const* t)
{
    ASSERT0(t->getCode() == TR_PRAGMA);
    TokenList const* tl = TREE_token_lst(t);
    switch (TL_tok(tl)) {
    case T_ID:
        ASSERT0(TL_str(tl));
        if (is_align_token(TL_str(tl))) {
            //Alignment format is: pragma align (IMM)
            process_pragma_align(&tl);
            return;
        }
        break;
    default:;
    }
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
    if (look_forward_token(2, T_ID, T_COLON)) {
        //current token is 'ID', and next is ':'
        return label_stmt();
    }

    if (CParser::inFirstSetOfExp(g_real_token)) {
        return exp_stmt();
    }

    switch (g_real_token) {
    case T_CASE:
    case T_DEFAULT:
        return label_stmt();
    case T_LLPAREN: {
        Tree * t = NEWTN(TR_SCOPE);
        TREE_scope(t) = CParser::compound_stmt(nullptr);
        return t;
    }
    case T_IF:
    case T_SWITCH:
        return select_stmt();
    case T_DO:
    case T_WHILE:
    case T_FOR:
        return iter_stmt();
    case T_GOTO:
    case T_BREAK:
    case T_RETURN:
    case T_CONTINUE:
        return jump_stmt();
    case T_SEMI: //null statement
        CParser::match(T_SEMI);
        return nullptr;
    case T_SHARP: {
        Tree * t = sharp_start_stmt();
        if (t->getCode() == TR_PRAGMA) {
            process_pragma(t);
        } else if (t->getCode() == TR_PREP) {
            ; //nothing to do
        } else {
            ASSERTN(0, ("NEED SUPPORT"));
        }
        return t;
    }
    default:;
    }

    if (inFirstSetOfDeclaration()) {
        //It may be varirable or type declaration.
        if (!g_enable_c99_declaration) {
            //C89 options.
            err(g_real_line_num,
                "'%s' is out of definition after or before block",
                g_real_token_string);
        }
        return declaration_list(); //Supported define variables anywhere.
    }

    err(g_real_line_num, "syntax error : illegal used '%s'",
        g_real_token_string);
    return nullptr;
}


//Top level dispatch to parse DECLARATION or PLAUSE.
static Tree * dispatch()
{
    Enum * e = nullptr;
    INT idx = -1;
    Tree * t = nullptr;
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
        if (inFirstSetOfDeclaration()) {
            //reduce to variable declaration
            declaration();
        } else if (findEnumVal(g_real_token_string, &e, &idx)) {
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
        CParser::match(T_SEMI);
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
    case T_SHARP:
        t = sharp_start_stmt();
        break;
    case T_END:      // end of file
        return ST_SUCC;
    case T_UNDEF:
        err(g_real_line_num, "unrecognized token :%s", g_real_token_string);
        return t;
    default:
        err(g_real_line_num, "unrecognized token :%s", g_real_token_string);
        return t;
    }
    ASSERT0(verify(t));
    return t;
}


bool CParser::initSrcFile(CHAR const* fn)
{
    ASSERT0(fn);
    ASSERT0(g_hsrc == nullptr);
    g_hsrc = ::fopen(fn, "rb");
    if (g_hsrc == nullptr) {
        char const* msg = ::strerror(errno);
        err(0, "cannot open %s, error information is %s\n", fn, msg);
        return false;
    }
    return true;
}


void CParser::finiSrcFile()
{
    if (g_hsrc != nullptr) {
        ::fclose(g_hsrc);
        g_hsrc = nullptr;
    }
}


//Start to parse a file.
STATUS CParser::perform()
{
    if (g_hsrc == nullptr || g_fe_sym_tab == nullptr) {
        err(0, "source file and frontend symbol table are not initialized");
        return ST_ERR;
    }
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
    gettok(); //Get the first token.

    //Create outermost scope for top region.
    g_cur_scope = new_scope();
    SCOPE_level(g_cur_scope) = GLOBAL_SCOPE; //First global scope
    for (;;) {
        if (g_real_token == T_END) {
            return ST_SUCC;
        }
        if (g_real_token == T_UNDEF || is_too_many_err()) {
            return ST_ERR;
        }
        if (dispatch() == nullptr && g_err_msg_list.has_msg()) {
            return ST_ERR;
        }
    }
    UNREACHABLE();
    return ST_SUCC;
}
//END CParser

} //namespace xfe

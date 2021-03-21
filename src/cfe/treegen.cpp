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

#define NEWTN(tok)  allocTreeNode((tok), g_real_line_num)

static Tree * statement();
static bool is_c_type_spec(TOKEN tok);
static bool is_c_type_quan(TOKEN tok);
static bool is_c_stor_spec(TOKEN tok);
static Tree * cast_exp();
static Tree * unary_exp();
static Tree * exp_stmt();
static Tree * postfix_exp();

SMemPool * g_pool_general_used = nullptr;
SMemPool * g_pool_st_used = nullptr;
SMemPool * g_pool_tree_used = nullptr;
SymTab * g_fe_sym_tab = nullptr;
bool g_dump_token = false;
CHAR * g_real_token_string = nullptr;
TOKEN g_real_token = T_NUL;
static List<Cell*> g_cell_list;
bool g_enable_C99_declaration = true;
xcom::Vector<UINT> g_realline2srcline;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_tree_used);
    ASSERT0(p);
    ::memset(p, 0, size);
    return p;
}


Tree * buildIndmem(Tree * base, Decl const* fld)
{
    Tree * t = NEWTN(TR_INDMEM);
    TREE_token(t) = T_ARROW;
    TREE_base_region(t) = base;
    TREE_field(t) = buildId(fld);
    setParent(t, TREE_base_region(t));
    setParent(t, TREE_field(t));
    return t; 
}


//Build aggregate reference tree node.
//decl: aggregate declaration.
//fldvec: record a list of indices in aggregate.
//  e.g: struct T { unsigned int b, c; struct Q {int m,n;} char e; };
//  The accessing to m's fldvec is <2, 0>, whereas e's fldvec is <3>.
Tree * buildAggrFieldRef(Decl const* decl, xcom::Vector<UINT> & fldvec)
{
    ASSERT0(is_struct(decl) || is_union(decl));
    ASSERTN(fldvec.get_last_idx() >= 0, ("miss field index"));
    Tree * base = buildId(decl);
    Decl const* basedecl = decl;
    for (INT i = 0; i <= fldvec.get_last_idx(); i++) {
        Decl * flddecl = nullptr;
        if (is_struct(basedecl)) {
            Struct * spec = get_struct_spec(basedecl);
            get_aggr_field(spec, fldvec.get(i), &flddecl);
        } else if (is_union(basedecl)) {
            Union * spec = get_union_spec(basedecl);
            get_aggr_field(spec, fldvec.get(i), &flddecl);
        } else {
            ASSERTN(0, ("the function only handle aggregate reference"));
        }
        ASSERTN(flddecl, ("not find field"));

        if (is_pointer(basedecl)) {
            base = buildIndmem(base, flddecl);
        } else {
            base = buildDmem(base, flddecl);
        }

        basedecl = flddecl;
    }
    return base;
}


Tree * buildDmem(Tree * base, Decl const* fld)
{
    Tree * t = NEWTN(TR_DMEM);
    TREE_token(t) = T_DOT;
    TREE_base_region(t) = base;
    TREE_field(t) = buildId(fld);
    setParent(t, TREE_base_region(t));
    setParent(t, TREE_field(t));
    return t;
}


Tree * buildId(Decl const* decl)
{
    Tree * t = NEWTN(TR_ID);
    TREE_token(t) = T_ID;
    Sym * sym = get_decl_sym(decl);
    ASSERT0(sym);
    TREE_id(t) = sym;
    TREE_id_decl(t) = const_cast<Decl*>(decl);
    return t;
}


Tree * buildAggrRef(Tree * base, Decl const* fld)
{
    if (is_pointer(fld)) {
        return buildIndmem(base, fld);
    }
    return buildDmem(base, fld);
}


Tree * buildUInt(HOST_UINT val)
{
    Tree * t = NEWTN(TR_IMMU);
    //If the target integer hold in 'g_real_token_string' is longer than
    //host size_t type, it will be truncated now.
    TREE_token(t) = T_IMMU;
    TREE_imm_val(t) = (HOST_INT)val;
    return t; 
}


Tree * buildInitvalScope(Tree * exp_list)
{
    Tree * t = NEWTN(TR_INITVAL_SCOPE);
    TREE_initval_scope(t) = exp_list;
    return t;
}


Tree * buildInt(HOST_INT val)
{
    Tree * t = NEWTN(TR_IMM);
    //If the target integer hold in 'g_real_token_string' is longer than
    //host size_t type, it will be truncated now.
    TREE_token(t) = T_IMM;
    TREE_imm_val(t) = val;
    return t; 
}


Tree * buildString(Sym const* str)
{
    Tree * t = NEWTN(TR_STRING);
    TREE_token(t) = T_STRING;
    TREE_string_val(t) = str;
    return t;
}


Tree * buildAssign(Decl const* decl, Tree * rhs)
{
    Tree * id = buildId(decl);
    Tree * p = NEWTN(TR_ASSIGN);
    TREE_token(p) = T_ASSIGN;
    TREE_lchild(p) = id;
    setParent(p, TREE_lchild(p));
    TREE_rchild(p) = rhs;
    setParent(p, TREE_rchild(p));
    return p;
}


Tree * buildAssign(Tree * lhs, Tree * rhs)
{
    Tree * p = NEWTN(TR_ASSIGN);
    TREE_token(p) = T_ASSIGN;
    TREE_lchild(p) = lhs;
    setParent(p, TREE_lchild(p));
    TREE_rchild(p) = rhs;
    setParent(p, TREE_rchild(p));
    return p;
}


Tree * buildArray(Tree * base, xcom::Vector<UINT> & subexp_vec)
{
    ASSERTN(subexp_vec.get_last_idx() >= 0, ("miss dimension exp"));
    for (INT i = 0; i <= subexp_vec.get_last_idx(); i++) {
        Tree * array = NEWTN(TR_ARRAY);
        TREE_array_base(array) = base;
        setParent(array, base);
        TREE_array_indx(array) = buildInt(subexp_vec.get(i));
        setParent(array, TREE_array_indx(array));
        base = array; 
    }
    return base;
}


//Build array tree node.
//subexp_vec: record a list of subscript expressions.
//            e.g: arr[3][5], subexp_vec is <3, 5>.
Tree * buildArray(Decl const* decl, xcom::Vector<UINT> & subexp_vec)
{
    ASSERTN(subexp_vec.get_last_idx() >= 0, ("miss dimension exp"));
    return buildArray(buildId(decl), subexp_vec);
}


Tree * copyTreeList(Tree const* t)
{
    Tree * new_list = nullptr;
    while (t != nullptr) {
        Tree * newt = copyTree(t);
        xcom::add_next(&new_list, newt);
        t = TREE_nsib(t);
    }
    return new_list;
}


//Duplicate 't' and its kids, but without ir's sibiling node.
Tree * copyTree(Tree const* t)
{
    if (t == nullptr) { return nullptr; }    
    Tree * newt = NEWTN(TREE_type(t));    
    UINT id = TREE_uid(newt);
    ::memcpy(newt, t, sizeof(Tree));
    TREE_uid(newt) = id;
    TREE_parent(newt) = nullptr;
    TREE_psib(newt) = nullptr;
    TREE_nsib(newt) = nullptr;
    for (UINT i = 0; i < MAX_TREE_FLDS; i++) {
        Tree * kid = TREE_fld(t, i);
        if (kid == nullptr) { continue; }

        Tree * newkid_list = copyTreeList(kid);
        TREE_fld(newt, i) = newkid_list;
        for (Tree * xt = newkid_list; xt != nullptr; xt = TREE_nsib(xt)) {
            TREE_parent(xt) = newt;
        }
    }
    return newt;
}


#ifdef _DEBUG_
//Verify the Tree node legality.
static bool verify(Tree * t)
{
    if (t == nullptr) { return true; }
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
    case TR_PREP:
        break;
    default: ASSERTN(0, ("unknown tree type:%d", TREE_type(t)));
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
    //li = g_labtab.append_and_retrieve(li);
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
    Sym * s = g_fe_sym_tab->add(tokname);
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
    Sym * s = g_fe_sym_tab->add(tokname);
    TOKEN_INFO_name(tki) = SYM_name(s);
    TOKEN_INFO_token(tki) = tok;
    TOKEN_INFO_lineno(tki) = lineno;
    append_c_head((size_t)tki);
}


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


void dump_tok_list()
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


bool is_in_first_set_of_declarator()
{
    if (g_real_token == T_ID) {
        Decl * ut = nullptr;
        Struct * s = nullptr;
        Union * u = nullptr;
        if (is_user_type_exist_in_outer_scope(g_real_token_string, &ut) ||
            is_struct_exist_in_outer_scope(g_cur_scope,
                g_real_token_string, &s) ||
            is_union_exist_in_outer_scope(g_cur_scope,
                g_real_token_string, &u)) {
            return true;
        }
        return false;
    }
    if (is_c_type_spec(g_real_token) ||
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
    Decl * ut = nullptr;
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
    Scope * sc = g_cur_scope;
    while (sc != nullptr) {
        if (is_user_type_exist(SCOPE_user_type_list(sc), cl, ut)) {
            return true;
        }
        sc = SCOPE_parent(sc);
    }
    return false;
}


INT is_user_type_exist_in_cur_scope(CHAR * cl, OUT Decl ** ut)
{
    Scope * sc = g_cur_scope;
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
    if (child == nullptr) { return; }
    while (child != nullptr) {
        TREE_parent(child) = parent;
        child = TREE_nsib(child);
    }
}


//Draw token until meeting any TOKEN list in '...'
void suck_tok_to(INT placeholder, ...)
{
    va_list arg;
    TOKEN tok;
    ASSERT0(sizeof(TOKEN) == sizeof(INT));
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
    Sym * sym = g_fe_sym_tab->add(g_real_token_string);
    TREE_id(t) = sym;
    return t;
}


static void setMapRealLineToSrcLine(UINT realline, UINT srcline)
{
    g_realline2srcline.set(realline, srcline);
}


UINT mapRealLineToSrcLine(UINT realline)
{
    return g_realline2srcline.get(realline);
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


INT suck_tok()
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


INT match(TOKEN tok)
{
    if (g_real_token == tok) {
        suck_tok();
    } else {
        return ST_ERR;
    }
    if (g_dump_token) {
        note(g_logmgr, "LINE:%10d, TOKEN:%s\n",
             g_real_line_num, g_real_token_string);
    }
    return ST_SUCC;
}


//Pry next 'num' token info.
//
//'n': represent the next N token to current token.
//     If n is 0, it will return current token.
static TOKEN look_next_token(INT n,
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
        if (tok_string != nullptr) {
            *tok_string = g_real_token_string;
        }
        if (tok_line_num != nullptr) {
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
    Tree * t = exp();
    Tree * last = xcom::get_last(t);

    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        Tree * nt = exp();
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
static Tree * primary_exp(IN OUT UINT * st)
{
    Tree * t = nullptr;
    switch (g_real_token) {
    case T_ID: {
        Enum * e = nullptr;
        INT idx = 0;
        if (findEnumConst(g_real_token_string, &e, &idx)) {
            t = NEWTN(TR_ENUM_CONST);
            TREE_enum(t) = e;
            TREE_enum_val_idx(t) = idx;
        } else {
            //Struct, Union, TYPEDEF-NAME should be
            //parsed during declaration().
            Decl * dcl = nullptr;
            t = id();
            if (!is_id_exist_in_outer_scope(g_real_token_string, &dcl)) {
                err(g_real_line_num, "'%s' undeclared identifier",
                    g_real_token_string);
                match(T_ID);
                *st = ST_ERR;
                return nullptr;
            }
            TREE_id_decl(t) = dcl;
        }
        TREE_token(t) = g_real_token;
        match(T_ID);
        break;
    }
    case T_IMM:
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        t = buildInt((HOST_INT)xcom::xatoll(g_real_token_string, false));
        match(T_IMM);
        return t;
    case T_IMML:
        t = NEWTN(TR_IMML);
        //If the target integer hold in 'g_real_token_string' is longer than
        //host size_t type, it will be truncated now.
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_INT)xcom::xatoll(g_real_token_string, false);
        match(T_IMML);
        return t;
    case T_IMMU:
        t = NEWTN(TR_IMMU);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xcom::xatoll(g_real_token_string, false);
        match(T_IMMU);
        return t;
    case T_IMMUL:
        t = NEWTN(TR_IMMUL);
        TREE_token(t) = g_real_token;
        TREE_imm_val(t) = (HOST_UINT)xcom::xatoll(g_real_token_string, false);
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
    case T_STRING: { // "abcd"
        t = NEWTN(TR_STRING);
        TREE_token(t) = g_real_token;
        CHAR * tbuf = nullptr;
        UINT tbuflen = 0;
        Sym * sym = g_fe_sym_tab->add(g_real_token_string);
        match(T_STRING);

        //Concatenate string.
        for (; g_real_token == T_STRING; match(T_STRING)) {
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
        match(T_CHAR_LIST);
        break;
    case T_LPAREN:
        match(T_LPAREN);
        t = exp_list();
        if (match(T_RPAREN) != ST_SUCC) {
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
                match(T_LSPAREN);
                array_root = NEWTN(TR_ARRAY);
                TREE_array_base(array_root) = t;
                TREE_array_indx(array_root) = exp();
                setParent(array_root, t);
                setParent(array_root, TREE_array_indx(array_root));
                if (TREE_array_indx(array_root) == nullptr) {
                    err(g_real_line_num, "array index cannot be nullptr");
                    return nullptr;
                }
                if (match(T_RSPAREN) != ST_SUCC) {
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
            Tree * mem_ref = nullptr;
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
            if (t == nullptr) {
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
            if (t == nullptr) {
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
    Tree * t = nullptr;
    TOKEN tok = T_NUL;
    CHAR * tok_string = nullptr;
    if (g_real_token == T_LPAREN) {
        //Next exp either (exp) or (type_name), so after several '(', there
        //must be T_ID appearing .
        tok = look_next_token(1, &tok_string, nullptr);
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
            if (TREE_type_name(t) == nullptr) {
                err(g_real_line_num, "except 'type-name'");
                goto FAILED;
            }
        } else if (tok == T_ID) {
            //Record a User defined type
            Decl * ut = nullptr;
            ASSERT0(tok_string != nullptr);
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
    Tree * t = nullptr;
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
        if (TREE_inc_exp(t) == nullptr) {
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
        if (TREE_inc_exp(t) == nullptr) {
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
        if (TREE_lchild(t) == nullptr) {
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
        if (TREE_lchild(t) == nullptr) {
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
        if (TREE_lchild(t) == nullptr) {
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

    if (TREE_type(p) == TR_TYPE_NAME) {
        Tree * srcexp = cast_exp();
        if (srcexp == nullptr) {
            err(g_real_line_num, "cast expression cannot be nullptr");
            goto FAILED;
        }
        t = gen_cvt(p, srcexp);
        if (TREE_cast_exp(t) == nullptr) {
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
        setParent(p, TREE_lchild(p));
            // a*b*c  =>
            //             *
            //           /  |
            //           *  c
            //          / |
            //         a  b
        match(g_real_token);
        TREE_rchild(p) = cast_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
           //  a<<b<<c  =>
           //              <<
           //             /  |
           //            <<  c
           //           /  |
           //          a   b
        match(g_real_token);
        TREE_rchild(p) = additive_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
           //  a<b<c   =>
           //              <
           //             / |
           //            <  c
           //           / |
           //          a  b
        match(g_real_token);
        TREE_rchild(p) = shift_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
           //  a&b&c   =>  &
           //             / |
           //            &  c
           //           / |
           //          a  b
        match(T_BITAND);
        TREE_rchild(p) = equality_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
           //  a^b^c   =>  ^
           //             / |
           //            ^  c
           //           / |
           //          a  b
        match(T_XOR);
        TREE_rchild(p) = AND_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
        //  a|b|c   =>  |
        //             / |
        //            |  c
        //           / |
        //          a  b
        match(T_BITOR);
        TREE_rchild(p) = exclusive_OR_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
        //a && b && c =>
        //              &&
        //             /  |
        //            &&  c
        //           /  |
        //           a  b
        match(T_AND);
        TREE_rchild(p) = inclusive_OR_exp();
        setParent(p, TREE_rchild(p));
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
        setParent(p, TREE_lchild(p));
        //  a||b||c  =>  ||
        //              /  |
        //             ||  c
        //            /  |
        //           a   b
        match(T_OR);
        TREE_rchild(p) = logical_AND_exp();
        setParent(p, TREE_rchild(p));
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
    return t;
FAILED:
    prt("error in conditional_expt()");
    return t;
}


Tree * exp()
{
    Tree * t = conditional_exp(), * p;
    if (t == nullptr) { return nullptr; }
    if (is_assign_op(g_real_token)) {
        p = NEWTN(TR_ASSIGN);
        TREE_token(p) = g_real_token;
        TREE_lchild(p) = t;
        setParent(p, TREE_lchild(p));
        match(g_real_token);
        TREE_rchild(p) = exp();
        if (TREE_rchild(p) == nullptr) {
            err(g_real_line_num, "expression miss r-value");
            goto FAILED;
        }
        setParent(p, TREE_rchild(p));
        t = p;
    }
    return t;
FAILED:
    prt("error in expt()");
    return t;
}


Tree * exp_list()
{
    Tree * t = exp();
    Tree * last = xcom::get_last(t);
    while (g_real_token == T_COMMA) {
        match(T_COMMA);
        Tree * nt = exp();
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
    Tree * t = nullptr;
    ASSERT0(g_real_token == T_SHARP);
    match(T_SHARP);
    switch (g_real_token) {
    case T_PRAGMA:
        t = NEWTN(TR_PRAGMA);
        TREE_token(t) = g_real_token;
        match(T_PRAGMA);
        break;
    case T_IMM:
        //Current line generated by preprocessor.
        //e.g: # 1 "test/compile/prc.c"
        g_disgarded_line_num++;
        t = NEWTN(TR_PREP);
        TREE_token(t) = g_real_token;
        match(T_IMM);
        break;
    default:
        err(g_real_line_num,
            "illegal use '#', its followed keyword should be 'pragma'");
        return nullptr;
    }

    TokenList * last = nullptr;
    //Match token till NEWLINE.
    g_enable_newline_token = true;
    while (g_real_token != T_NEWLINE && g_real_token != T_NUL) {
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
        xcom::add_next(&TREE_token_lst(t), &last, tl);
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
        match(T_ID);
        if (match(T_COLON) != ST_SUCC) {
            err(g_real_line_num, "label defined incompletely");
            return t;
        }
        break;
    case T_CASE:
        {
            LONGLONG idx = 0;
            Tree * nt = nullptr;
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
            if (computeMaxBitSizeForValue((TMWORD)idx) >
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
    if (TREE_dowhile_det(t) == nullptr) {
        err(g_real_line_num, "while determination cannot be nullptr");
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
    prt("error in do_while_stmt()");
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
    if (TREE_whiledo_det(t) == nullptr) {
        err(g_real_line_num, "while determination cannot be nullptr");
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
    prt("error in while_do_stmt()");
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
        push_scope(false);

        //Should record for-stmt's init-list scope.
        TREE_for_scope(t) = g_cur_scope;
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
        pop_scope();
    }
    return t;

FAILED:
    if (g_enable_C99_declaration) {
        pop_scope();
    }
    prt("error in for_stmt()");
    return t;
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
    if (TREE_if_det(t) == nullptr) {
        err(g_real_line_num, "'if' determination cannot be nullptr");
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
    prt("error in if_stmt()");
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
    if (TREE_switch_det(t) == nullptr) {
        err(g_real_line_num, "switch determination cannot be nullptr");
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


Scope * compound_stmt(Decl * para_list)
{
    Tree * t = nullptr;
    Scope * s = nullptr;
    Tree * last;
    INT cerr = 0;
    //enter a new sub-scope region
    Scope * cur_scope = push_scope(false);

    //Append parameters to declaration list of function body scope.
    Decl * lastdcl = xcom::get_last(SCOPE_decl_list(cur_scope));
    UINT pos = 0;
    for (; para_list != nullptr; para_list = DECL_next(para_list), pos++) {
        if (DECL_dt(para_list) == DCL_VARIABLE) {
            //VARIABLE parameter should be the last parameter.
            ASSERT0(DECL_next(para_list) == nullptr);
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
        xcom::add_next(&SCOPE_decl_list(cur_scope), &lastdcl, declaration);
        DECL_decl_scope(declaration) = cur_scope;
        DECL_formal_param_pos(declaration) = pos;

        lastdcl = declaration;

        //Append parameter list to symbol list of function body scope.
        Sym * sym = get_decl_sym(declaration);
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
    last = nullptr;
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

        if (last == nullptr) {
            last = xcom::get_last(SCOPE_stmt_list(cur_scope));
        }
        xcom::add_next(&SCOPE_stmt_list(cur_scope), &last, t);

        last = xcom::get_last(t);

        if ((cerr != (INT)g_err_msg_list.get_elem_count()) ||
            (cerr > 0 && t == nullptr)) {
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
    pop_scope();
    return s;
FAILED:
    match(T_RLPAREN);
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
    if (match(T_SEMI) != ST_SUCC) {
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
    ASSERT0(TREE_type(t) == TR_PRAGMA);
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
    Tree * t = nullptr;
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
            TREE_scope(t) = compound_stmt(nullptr);
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
            process_pragma(t);
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
    Enum * e = nullptr;
    INT idx = -1;
    Decl * ut = nullptr;
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
    case T_SHARP:
        t = sharp_start_stmt();
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
    g_pool_general_used = nullptr;
    g_pool_tree_used = nullptr;
    g_pool_st_used = nullptr;

    if (g_ofst_tab != nullptr) {
        ::free(g_ofst_tab);
        g_ofst_tab = nullptr;
    }

    if (g_cur_line != nullptr) {
        ::free(g_cur_line);
        g_cur_line = nullptr;
    }

    if (g_hsrc != nullptr) {
        fclose(g_hsrc);
        g_hsrc = nullptr;
    }
}


void setLogMgr(LogMgr * logmgr)
{
    ASSERT0(g_logmgr == nullptr);
    g_logmgr = logmgr;
}


//Start to parse a file.
INT Parser()
{
    ASSERTN(g_hsrc && g_fe_sym_tab, ("must initialize them"));
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

        if (dispatch() == nullptr && g_err_msg_list.get_elem_count() != 0) {
            return ST_ERR;
        }
    }
}

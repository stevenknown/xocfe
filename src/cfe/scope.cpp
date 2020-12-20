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

//The outermost scope is global region which id is 0, and the inner
//scope scope is function body-stmt which id is 1, etc.
Scope * g_cur_scope = nullptr;
List<Scope*> g_scope_list;
UINT g_scope_count = 0;
LAB2LINE_MAP g_lab2lineno;
xcom::TTab<LabelInfo*> g_lab_used;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_general_used);
    ASSERT0(p);
    ::memset(p, 0, size);
    return p;
}


Scope * new_scope()
{
    Scope * sc = (Scope*)xmalloc(sizeof(Scope));
    sc->init(g_scope_count);
    g_scope_list.append_tail(sc);
    return sc;
}


void inc_indent(UINT ind)
{
    if (g_logmgr != nullptr)  {
        g_logmgr->incIndent(ind);
    }
}


void dec_indent(UINT ind)
{
    if (g_logmgr != nullptr)  {
        g_logmgr->decIndent(ind);
    }
}


//Be usually used in scope process.
//Return nullptr if this function do not find 'sym' in 'sym_list', and
//'sym' will be appended  into list, otherwise return 'sym'.
Sym * add_to_symtab_list(SymList ** sym_list , Sym * sym)
{
    if (sym_list == nullptr || sym == nullptr) {
        return nullptr;
    }
    if ((*sym_list) == nullptr) {
        *sym_list = (SymList*)xmalloc(sizeof(SymList));
        SYM_LIST_sym(*sym_list) = sym;
        return nullptr;
    } else {
        SymList * p = *sym_list, * q = nullptr;
        while (p != nullptr) {
            q = p;
            if (SYM_LIST_sym(p) == sym) {
                //'sym' already exist, return 'sym' as result
                return sym;
            }
            p = SYM_LIST_next(p);
        }
        SYM_LIST_next(q) = (SymList*)xmalloc(sizeof(SymList));
        SYM_LIST_prev(SYM_LIST_next(q)) = q;
        q = SYM_LIST_next(q);
        SYM_LIST_sym(q) = sym;
    }
    return nullptr;
}


//'is_tmp_sc': true if the new scope is used for temprary.
//And it will be removed from the sub-scope-list while return to
//the parent.
Scope * push_scope(bool is_tmp_sc)
{
    Scope * sc = new_scope();
    SCOPE_level(sc) = SCOPE_level(g_cur_scope) + 1;
    SCOPE_parent(sc) = g_cur_scope;
    SCOPE_is_tmp_sc(sc) = is_tmp_sc;

    //Add 'sc' as sub scope followed the most right one of subscope list.
    //e.g: first_sub_scope -> second_sub_scope -> ...
    xcom::add_next(&SCOPE_sub(g_cur_scope), sc);
    g_cur_scope = sc;
    return g_cur_scope;
}


Scope * pop_scope()
{
    Scope * parent = SCOPE_parent(g_cur_scope);
    if (SCOPE_is_tmp_sc(g_cur_scope)) {
        xcom::remove(&SCOPE_sub(parent), g_cur_scope);
    }
    g_cur_scope = parent;
    return g_cur_scope;
}


// Get GLOBAL_SCOPE level scope
Scope * get_global_scope()
{
    Scope * s = g_cur_scope;
    while (s) {
        if (SCOPE_level(s) == GLOBAL_SCOPE) {
            return s;
        }
        s = SCOPE_parent(s);
    }
    return nullptr;
}


Scope * get_last_sub_scope(Scope * s)
{
    ASSERT0(s);
    Scope * sub = SCOPE_sub(s);
    while (SCOPE_nsibling(sub) != nullptr) {
        sub = SCOPE_nsibling(sub);
    }
    return sub;
}


//Dump scope in a cascade of tree.
void dump_scope_tree(Scope * s, INT indent)
{
    if (g_logmgr == nullptr || s == nullptr) { return; }
    note(g_logmgr, "\n");
    INT i = indent;
    while (i-- > 0) { prt(g_logmgr, "    "); }
    prt(g_logmgr, "Scope(%d),level(%d)", SCOPE_id(s), SCOPE_level(s));
    dump_scope_tree(SCOPE_sub(s), indent + 1);
    dump_scope_tree(SCOPE_nsibling(s), indent);
}


static void dump_symbols(Scope * s)
{
    SymList * sym_list = SCOPE_sym_tab_list(s);
    if (sym_list != nullptr) {
        note(g_logmgr, "\nSYMBAL:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        bool first = true;
        while (sym_list != nullptr) {
            if (!first) {
                prt(g_logmgr, ",");
            }
            prt(g_logmgr, "%s", SYM_name(SYM_LIST_sym(sym_list)));
            sym_list = SYM_LIST_next(sym_list);
            first = false;
        }
        g_logmgr->decIndent(2);
    }
}


static void dump_labels(Scope * s)
{
    //All of customer defined labels in scope.
    LabelInfo * li = SCOPE_label_list(s).get_head();
    if (li != nullptr) {
        note(g_logmgr, "\nDEFINED LABEL:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        for (; li != nullptr; li = SCOPE_label_list(s).get_next()) {
            ASSERT0(map_lab2lineno(li) != 0);
            note(g_logmgr, "%s (def in line:%d)\n",
                 SYM_name(LABELINFO_name(li)),
                 map_lab2lineno(li));
        }
        g_logmgr->decIndent(2);
    }

    //All of refered labels in scope. 
    li = SCOPE_ref_label_list(s).get_head();
    if (li != nullptr) {
        note(g_logmgr, "\nREFED LABEL:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        for (; li != nullptr; li = SCOPE_ref_label_list(s).get_next()) {
            note(g_logmgr, "%s (use in line:%d)\n",
                 SYM_name(LABELINFO_name(li)),
                 map_lab2lineno(li));
        }
        g_logmgr->decIndent(2);
    }
}


static void dump_enums(Scope * s)
{
    EnumList * el = SCOPE_enum_list(s);
    if (el != nullptr) {
        StrBuf buf(64);
        note(g_logmgr, "\nENUM List:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        while (el != nullptr) {
            buf.clean();
            format_enum_complete(buf, ENUM_LIST_enum(el));
            note(g_logmgr, "%s\n", buf.buf);
            el = ENUM_LIST_next(el);
        }
        g_logmgr->decIndent(2);
    }
}


//Dump user defined type, declared by 'typedef'.
static void dump_user_defined_type(Scope * s)
{
    UserTypeList * utl = SCOPE_user_type_list(s);
    if (utl != nullptr) {
        StrBuf buf(64);
        note(g_logmgr, "\nUSER Type:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        while (utl != nullptr) {
            buf.clean();
            format_user_type_spec(buf, USER_TYPE_LIST_utype(utl));
            note(g_logmgr, "%s\n", buf.buf);
            utl = USER_TYPE_LIST_next(utl);
        }
        g_logmgr->decIndent(2);
    }
}


//structs
static void dump_structs(Scope * s)
{
    if (SCOPE_struct_list(s).get_elem_count() != 0) {
        note(g_logmgr, "\nSTRUCT:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");

        StrBuf buf(64);
        xcom::C<Struct*> * ct;
        for (Struct * st = SCOPE_struct_list(s).get_head(&ct);
             st != nullptr; st = SCOPE_struct_list(s).get_next(&ct)) {
            buf.clean();
            format_struct_complete(buf, st);
            note(g_logmgr, "%s\n", buf.buf);
        }

        g_logmgr->decIndent(2);
    }
}


//unions
static void dump_unions(Scope * s)
{
    if (SCOPE_union_list(s).get_elem_count() != 0) {
        note(g_logmgr, "\nUNION:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");

        StrBuf buf(64);
        xcom::C<Union*> * ct;
        for (Union * st = SCOPE_union_list(s).get_head(&ct);
             st != nullptr; st = SCOPE_union_list(s).get_next(&ct)) {
            buf.clean();
            format_union_complete(buf, st);
            note(g_logmgr, "%s\n", buf.buf);
        }

        g_logmgr->decIndent(2);
    }
}


static void dump_declarations(Scope * s, UINT flag)
{
    //declarations
    Decl * dcl = SCOPE_decl_list(s);
    if (dcl == nullptr) { return; }

    StrBuf buf(64);
    note(g_logmgr, "\nDECLARATIONS:");
    g_logmgr->incIndent(2);
    while (dcl != nullptr) {
        buf.clean();
        format_declaration(buf, dcl);
        note(g_logmgr, "\n%s", buf.buf);

        g_logmgr->incIndent(2);
        dump_decl(dcl);

        //Dump function body
        if (DECL_is_fun_def(dcl) && HAVE_FLAG(flag, DUMP_SCOPE_FUNC_BODY)) {
            g_logmgr->incIndent(2);
            dump_scope(DECL_fun_body(dcl), flag);
            g_logmgr->decIndent(2);
        }

        //Dump initializing value/expression.
        if (DECL_is_init(DECL_decl_list(dcl))) {
            prt(g_logmgr, " = ");
            g_logmgr->incIndent(8);
            dump_tree(DECL_init_tree(DECL_decl_list(dcl)));
            g_logmgr->decIndent(8);
        }

        g_logmgr->decIndent(2);
        note(g_logmgr, "\n");
        dcl = DECL_next(dcl);
    }
    g_logmgr->decIndent(2);
}


static void dump_stmts(Scope * s)
{
    Tree * t = SCOPE_stmt_list(s);
    if (t != nullptr) {
        note(g_logmgr, "\nSTATEMENT:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        dump_trees(t);
        g_logmgr->decIndent(2);
    }
}


void dump_scope_list(Scope * s, UINT flag)
{
    while (s != nullptr) {
        dump_scope(s, flag);
        s = SCOPE_nsibling(s);
    }
}


//Recusively dump Scope information, include symbol,
//label, type info, and trees.
void dump_scope(Scope * s, UINT flag)
{
    if (g_logmgr == nullptr) { return; }
    StrBuf buf(64);
    note(g_logmgr, "\nSCOPE(id:%d, level:%d)", SCOPE_id(s), SCOPE_level(s));
    g_logmgr->incIndent(2);
    dump_symbols(s);
    dump_labels(s);
    dump_enums(s);
    dump_user_defined_type(s);
    dump_structs(s);
    dump_unions(s);
    dump_declarations(s, flag);
    if (HAVE_FLAG(flag, DUMP_SCOPE_STMT_TREE)) {
        dump_stmts(s);
    }
    if (HAVE_FLAG(flag, DUMP_SCOPE_RECUR)) {
        dump_scope_list(SCOPE_sub(s), flag);
    }
    g_logmgr->decIndent(2);
}


void destroy_scope_list()
{
    for (Scope * sc = g_scope_list.get_head();
         sc != nullptr; sc = g_scope_list.get_next()) {
        sc->destroy();
    }
}


UINT map_lab2lineno(LabelInfo * li)
{
    return g_lab2lineno.get(li);
}


void set_map_lab2lineno(LabelInfo * li, UINT lineno)
{
    g_lab2lineno.set(li, lineno);
}


void set_lab_used(LabelInfo * li)
{
    g_lab_used.append(li);
}


bool is_lab_used(LabelInfo * li)
{
    return g_lab_used.find(li);
}

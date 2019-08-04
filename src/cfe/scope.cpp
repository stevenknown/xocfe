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

//The outermost scope is global region which id is 0, and the inner
//scope scope is function body-stmt which id is 1, etc.
SCOPE * g_cur_scope = NULL;
List<SCOPE*> g_scope_list;
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


SCOPE * new_scope()
{
    SCOPE * sc = (SCOPE*)xmalloc(sizeof(SCOPE));
    sc->init(g_scope_count);
    g_scope_list.append_tail(sc);
    return sc;
}


//Be usually used in scope process.
//Return NULL if this function do not find 'sym' in 'sym_list', and
//'sym' will be appended  into list, otherwise return 'sym'.
SYM * add_to_symtab_list(SYM_LIST ** sym_list , SYM * sym)
{
    if (sym_list == NULL || sym == NULL) {
        return NULL;
    }
    if ((*sym_list) == NULL) {
        *sym_list = (SYM_LIST*)xmalloc(sizeof(SYM_LIST));
        SYM_LIST_sym(*sym_list) = sym;
        return NULL;
    } else {
        SYM_LIST * p = *sym_list, * q = NULL;
        while (p != NULL) {
            q = p;
            if (SYM_LIST_sym(p) == sym) {
                //'sym' already exist, return 'sym' as result
                return sym;
            }
            p = SYM_LIST_next(p);
        }
        SYM_LIST_next(q) = (SYM_LIST*)xmalloc(sizeof(SYM_LIST));
        SYM_LIST_prev(SYM_LIST_next(q)) = q;
        q = SYM_LIST_next(q);
        SYM_LIST_sym(q) = sym;
    }
    return NULL;
}


//'is_tmp_sc': true if the new scope is used for temprary.
//And it will be removed from the sub-scope-list while return to
//the parent.
SCOPE * enter_sub_scope(bool is_tmp_sc)
{
    SCOPE * sc = new_scope();
    SCOPE_level(sc) = SCOPE_level(g_cur_scope) + 1;
    SCOPE_parent(sc) = g_cur_scope;
    SCOPE_is_tmp_sc(sc) = is_tmp_sc;

    //Add 'sc' as sub scope followed the most right one of subscope list.
    //e.g: first_sub_scope -> second_sub_scope -> ...
    xcom::add_next(&SCOPE_sub(g_cur_scope), sc);
    g_cur_scope = sc;
    return g_cur_scope;
}


SCOPE * return_to_parent_scope()
{
    SCOPE * parent = SCOPE_parent(g_cur_scope);
    if (SCOPE_is_tmp_sc(g_cur_scope)) {
        xcom::remove(&SCOPE_sub(parent), g_cur_scope);
    }
    g_cur_scope = parent;
    return g_cur_scope;
}


// Get GLOBAL_SCOPE level scope
SCOPE * get_global_scope()
{
    SCOPE * s = g_cur_scope;
    while (s) {
        if (SCOPE_level(s) == GLOBAL_SCOPE) {
            return s;
        }
        s = SCOPE_parent(s);
    }
    return NULL;
}


SCOPE * get_last_sub_scope(SCOPE * s)
{
    ASSERT0(s);
    SCOPE * sub = SCOPE_sub(s);
    while (SCOPE_nsibling(sub) != NULL) {
        sub = SCOPE_nsibling(sub);
    }
    return sub;
}


//Dump scope in a cascade of tree.
void dump_scope_tree(SCOPE * s, INT indent)
{
    if (g_tfile == NULL) { return; }
    if (s == NULL) return;
    note("\n");
    INT i = indent;
    while (i-- > 0) { prt("    "); }
    prt("SCOPE(%d),level(%d)", SCOPE_id(s), SCOPE_level(s));
    fflush(g_tfile);
    dump_scope_tree(SCOPE_sub(s), indent+1);
    dump_scope_tree(SCOPE_nsibling(s), indent);
}


//Recusively dump SCOPE information, include symbol, label, type info, and trees.
void dump_scope(SCOPE * s, UINT flag)
{
    if (g_tfile == NULL) { return; }
    StrBuf buf(64);
    note("\nSCOPE(id:%d, level:%d)", SCOPE_id(s), SCOPE_level(s));
    g_indent++;

    //symbols
    SYM_LIST * sym_list = SCOPE_sym_tab_list(s);
    if (sym_list != NULL) {
        note("\nSYMBAL:");
        g_indent++;
        note("\n");
        while (sym_list != NULL) {
            note("%s\n", SYM_name(SYM_LIST_sym(sym_list)));
            sym_list = SYM_LIST_next(sym_list);
        }
        g_indent--;
    }

    //all of defined customer label in code
    LabelInfo * li = SCOPE_label_list(s).get_head();
    if (li != NULL) {
        note("\nDEFINED LABEL:");
        g_indent++;
        note("\n");
        for (; li != NULL; li = SCOPE_label_list(s).get_next()) {
            ASSERT0(map_lab2lineno(li) != 0);
            note("%s (def in line:%d)\n",
                 SYM_name(LABEL_INFO_name(li)),
                 map_lab2lineno(li));
        }
        g_indent--;
    }

    //refered customer label in code
    li = SCOPE_ref_label_list(s).get_head();
    if (li != NULL) {
        note("\nREFED LABEL:");
        g_indent++;
        note("\n");
        for (; li != NULL; li = SCOPE_ref_label_list(s).get_next()) {
            note("%s (use in line:%d)\n",
                 SYM_name(LABEL_INFO_name(li)),
                 map_lab2lineno(li));
        }
        g_indent--;
    }

    //enums
    EnumList * el = SCOPE_enum_list(s);
    if (el != NULL) {
        note("\nENUM List:");
        g_indent++;
        note("\n");
        while (el != NULL) {
            buf.clean();
            format_enum_complete(buf, ENUM_LIST_enum(el));
            note("%s\n", buf.buf);
            el = ENUM_LIST_next(el);
        }
        g_indent--;
    }

    //user defined type, by 'typedef'
    UserTypeList * utl = SCOPE_user_type_list(s);
    if (utl != NULL) {
        note("\nUSER Type:");
        g_indent++;
        note("\n");
        while (utl != NULL) {
            buf.clean();
            format_user_type_spec(buf, USER_TYPE_LIST_utype(utl));
            note("%s\n", buf.buf);
            utl = USER_TYPE_LIST_next(utl);
        }
        g_indent--;
    }

    //structs
    if (SCOPE_struct_list(s).get_elem_count() != 0) {
        note("\nSTRUCT:");
        g_indent++;
        note("\n");

        xcom::C<Struct*> * ct;
        for (Struct * st = SCOPE_struct_list(s).get_head(&ct);
             st != NULL; st = SCOPE_struct_list(s).get_next(&ct)) {
            buf.clean();
            format_struct_complete(buf, st);
            note("%s\n", buf.buf);
        }

        g_indent--;
    }

    //unions
    if (SCOPE_union_list(s).get_elem_count() != 0) {
        note("\nUNION:");
        g_indent++;
        note("\n");

        xcom::C<Union*> * ct;
        for (Union * st = SCOPE_union_list(s).get_head(&ct);
             st != NULL; st = SCOPE_union_list(s).get_next(&ct)) {
            buf.clean();
            format_union_complete(buf, st);
            note("%s\n", buf.buf);
        }

        g_indent--;
    }

    //declarations
    Decl * dcl = SCOPE_decl_list(s);
    if (dcl != NULL) {
        note("\nDECLARATIONS:");
        note("\n");
        g_indent++;
        while (dcl != NULL) {
            buf.clean();
            format_declaration(buf, dcl);
            note("%s", buf.buf);
            dump_decl(dcl);

            //Dump function body
            if (DECL_is_fun_def(dcl) && HAVE_FLAG(flag, DUMP_SCOPE_FUNC_BODY)) {
                g_indent += 2;
                dump_scope(DECL_fun_body(dcl), flag);
                g_indent -= 2;
            }

            //Dump initializing value/expression.
            if (DECL_is_init(DECL_decl_list(dcl))) {
                note("= ");
                g_indent += 2;
                dump_tree(DECL_init_tree(DECL_decl_list(dcl)));
                g_indent -= 2;
            }

            note("\n");
            dcl = DECL_next(dcl);
        }
        g_indent--;
    }
    fflush(g_tfile);

    if (HAVE_FLAG(flag, DUMP_SCOPE_STMT_TREE)) {
        Tree * t = SCOPE_stmt_list(s);
        if (t != NULL) {
            note("\nSTATEMENT:");
            g_indent++;
            note("\n");
            dump_trees(t);
            g_indent--;
        }
    }
    g_indent--;
}


void destroy_scope_list()
{
    for (SCOPE * sc = g_scope_list.get_head();
         sc != NULL; sc = g_scope_list.get_next()) {
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

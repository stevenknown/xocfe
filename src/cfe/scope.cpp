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

//The outermost scope is global region which id is 0, and the inner
//scope scope is function body-stmt which id starts at 1, etc.
Scope * g_cur_scope = nullptr;
xcom::List<Scope*> g_scope_list;
UINT g_scope_count = 0;
Label2Lineno g_lab2lineno;
xcom::TTab<xoc::LabelInfo const*> g_lab_used;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_general_used);
    ASSERT0(p);
    ::memset((void*)p, 0, size);
    return p;
}


//
//START Scope
//
void Scope::init(UINT & sc)
{
    SCOPE_label_list(this).init();
    SCOPE_ref_label_list(this).init();
    SCOPE_struct_list(this).init();
    SCOPE_union_list(this).init();
    SCOPE_id(this) = sc++;
    SCOPE_level(this) = -1;
    SCOPE_parent(this) = nullptr;
    SCOPE_nsibling(this) = nullptr;
    SCOPE_sub(this)  = nullptr;
    SCOPE_enum_tab(this) = new EnumTab();
}

void Scope::destroy()
{
    SCOPE_label_list(this).destroy();
    SCOPE_ref_label_list(this).destroy();
    SCOPE_struct_list(this).destroy();
    SCOPE_union_list(this).destroy();
    delete SCOPE_enum_tab(this);
    SCOPE_enum_tab(this) = nullptr;
}


Enum * Scope::addEnum(Enum * e)
{
    ASSERT0(e);
    return getEnumTab()->append_and_retrieve(e);
}


void Scope::addStruct(Struct * s)
{
    ASSERT0(s);
    SCOPE_struct_list(this).append_tail(s);
    AGGR_scope(s) = this;
}


void Scope::addUnion(Union * u)
{
    ASSERT0(u);
    SCOPE_union_list(this).append_tail(u);
    AGGR_scope(u) = this;
}


void Scope::addDecl(Decl * decl)
{
    ASSERT0(decl);
    xcom::add_next(&SCOPE_decl_list(this), decl);
    DECL_decl_scope(decl) = this;
}


void Scope::addStmt(Tree * t)
{
    ASSERT0(t);
    xcom::add_next(&SCOPE_stmt_list(this), t);
}


//Return true if enum-value existed in current scope.
//idx: the index that indicates the position of pacticular Item in Enum.
bool Scope::isEnumExist(CHAR const* vname, OUT Enum ** e, OUT INT * idx) const
{
    if (vname == nullptr) { return false; }

    EnumTabIter it;
    EnumTab const* et = getEnumTab();
    for (Enum * en = et->get_first(it);
         en != nullptr; en = et->get_next(it)) {
        if (en->isEnumValExist(vname, idx)) {
            *e = en;
            return true;
        }
    }
    return false;
}
//END Scope


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
//'sym' will be appended into list, otherwise return 'sym'.
Sym const* Scope::addToSymList(Sym const* sym)
{
    if (sym == nullptr) { return nullptr; }
    if (SCOPE_sym_list(this) == nullptr) {
        SCOPE_sym_list(this) = (SymList*)xmalloc(sizeof(SymList));
        SYM_LIST_sym(SCOPE_sym_list(this)) = sym;
        return nullptr;
    }

    SymList * p = SCOPE_sym_list(this), * q = nullptr;
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
    return nullptr;
}


//Return nullptr indicate we haven't found it in 'ut_list', and
//append 'ut' to tail of the list as correct, otherwise return
//the finded one.
Decl * Scope::addToUserTypeList(Decl * decl)
{
    if (decl == nullptr) { return nullptr; }
    if (SCOPE_user_type_list(this) == nullptr) {
        SCOPE_user_type_list(this) = (UserTypeList*)xmalloc(
            sizeof(UserTypeList));
        USER_TYPE_LIST_utype(SCOPE_user_type_list(this)) = decl;
        return nullptr;
    }

    UserTypeList * p = SCOPE_user_type_list(this), * q = nullptr;
    while (p != nullptr) {
        q = p;
        if (USER_TYPE_LIST_utype(p) == decl) {
            //'sym' already exist, return 'sym' as result
            return decl;
        }
        p = USER_TYPE_LIST_next(p);
    }
    USER_TYPE_LIST_next(q) = (UserTypeList*)xmalloc(sizeof(UserTypeList));
    USER_TYPE_LIST_prev(USER_TYPE_LIST_next(q)) = q;
    q = USER_TYPE_LIST_next(q);
    USER_TYPE_LIST_utype(q) = decl;
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


void clean_global_scope()
{
    g_cur_scope = nullptr;
}


// Get GLOBAL_SCOPE level scope
Scope * get_global_scope()
{
    for (Scope * s = g_cur_scope; s != nullptr; s = SCOPE_parent(s)) {
        if (SCOPE_level(s) == GLOBAL_SCOPE) {
            return s;
        }
    }
    return nullptr;
}


Scope * Scope::getLastSubScope() const
{
    Scope * sub = SCOPE_sub(this);
    while (SCOPE_nsibling(sub) != nullptr) {
        sub = SCOPE_nsibling(sub);
    }
    return sub;
}


//Dump scope in a cascade of tree.
static void dump_scope_tree(Scope * s, INT indent)
{
    if (g_logmgr == nullptr || s == nullptr) { return; }
    note(g_logmgr, "\n");
    INT i = indent;
    while (i-- > 0) { prt(g_logmgr, "    "); }
    prt(g_logmgr, "Scope(%d),level(%d)", SCOPE_id(s), SCOPE_level(s));
    dump_scope_tree(SCOPE_sub(s), indent + 1);
    dump_scope_tree(SCOPE_nsibling(s), indent);
}


static void dump_symbols(Scope const* s)
{
    SymList * sym_list = SCOPE_sym_list(s);
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


static void dump_labels(Scope const* s)
{
    //All of customer defined labels in scope.
    C<LabelInfo*> * it;
    LabelInfo * li = SCOPE_label_list(s).get_head(&it);
    if (li != nullptr) {
        note(g_logmgr, "\nDEFINED LABEL:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        for (; li != nullptr; li = SCOPE_label_list(s).get_next(&it)) {
            ASSERT0(map_lab2lineno(li) != 0);
            note(g_logmgr, "%s (def in line:%d)\n",
                 SYM_name(LABELINFO_name(li)),
                 map_lab2lineno(li));
        }
        g_logmgr->decIndent(2);
    }

    //All of refered labels in scope.
    li = SCOPE_ref_label_list(s).get_head(&it);
    if (li != nullptr) {
        note(g_logmgr, "\nREFED LABEL:");
        g_logmgr->incIndent(2);
        note(g_logmgr, "\n");
        for (; li != nullptr; li = SCOPE_ref_label_list(s).get_next(&it)) {
            note(g_logmgr, "%s (use in line:%d)\n",
                 SYM_name(LABELINFO_name(li)),
                 map_lab2lineno(li));
        }
        g_logmgr->decIndent(2);
    }
}


static void dump_enums(Scope const* s)
{
    EnumTab * el = s->getEnumTab();
    if (el == nullptr) { return; }
    xcom::DefFixedStrBuf buf;
    note(g_logmgr, "\nENUM Tab:");
    g_logmgr->incIndent(2);
    note(g_logmgr, "\n");
    EnumTabIter it;
    for (Enum * e = el->get_first(it);
         e != nullptr; e = el->get_next(it)) {
        buf.clean();
        format_enum_complete(buf, e);
        note(g_logmgr, "%s\n", buf.getBuf());
    }
    g_logmgr->decIndent(2);
}


//Dump user defined type, declared by 'typedef'.
static void dump_user_defined_type(Scope const* s)
{
    UserTypeList * utl = SCOPE_user_type_list(s);
    if (utl == nullptr) { return; }

    note(g_logmgr, "\nUSER Type:");
    g_logmgr->incIndent(2);

    xcom::DefFixedStrBuf buf;
    while (utl != nullptr) {
        buf.clean();
        format_user_type(buf, USER_TYPE_LIST_utype(utl));
        note(g_logmgr, "\n%s", buf.getBuf());
        utl = USER_TYPE_LIST_next(utl);
    }
    g_logmgr->decIndent(2);
    note(g_logmgr, "\n");
}


//structs
static void dump_structs(Scope const* s)
{
    if (SCOPE_struct_list(s).get_elem_count() == 0) { return; }

    note(g_logmgr, "\nSTRUCT:");
    g_logmgr->incIndent(2);

    xcom::DefFixedStrBuf buf;
    xcom::C<Struct*> * ct;
    for (Struct * st = SCOPE_struct_list(s).get_head(&ct);
         st != nullptr; st = SCOPE_struct_list(s).get_next(&ct)) {
        buf.clean();
        format_struct_complete(buf, st);
        note(g_logmgr, "\n%s", buf.getBuf());
    }

    g_logmgr->decIndent(2);
    note(g_logmgr, "\n");
}


//unions
static void dump_unions(Scope const* s)
{
    if (SCOPE_union_list(s).get_elem_count() == 0) { return; }

    note(g_logmgr, "\nUNION:");
    g_logmgr->incIndent(2);
    note(g_logmgr, "\n");

    xcom::DefFixedStrBuf buf;
    xcom::C<Union*> * ct;
    for (Union * st = SCOPE_union_list(s).get_head(&ct);
         st != nullptr; st = SCOPE_union_list(s).get_next(&ct)) {
        buf.clean();
        format_union_complete(buf, st);
        note(g_logmgr, "\n%s", buf.getBuf());
    }

    g_logmgr->decIndent(2);
    note(g_logmgr, "\n");
}


static void dump_declarations(Scope const* s, UINT flag)
{
    //declarations
    Decl * dcl = s->getDeclList();
    if (dcl == nullptr) { return; }

    xcom::DefFixedStrBuf buf;
    note(g_logmgr, "\nDECLARATIONS:");
    g_logmgr->incIndent(2);
    while (dcl != nullptr) {
        buf.clean();
        format_declaration(buf, dcl, true);
        note(g_logmgr, "\n%s", buf.getBuf());

        g_logmgr->incIndent(2);
        dcl->dump();

        //Dump function body
        if (DECL_is_fun_def(dcl) && HAVE_FLAG(flag, DUMP_SCOPE_FUNC_BODY)) {
            g_logmgr->incIndent(2);
            DECL_fun_body(dcl)->dump(flag);
            g_logmgr->decIndent(2);
        }

        //Dump initializing value/expression.
        if (DECL_is_init(DECL_decl_list(dcl))) {
            prt(g_logmgr, " = ");
            g_logmgr->incIndent(8);
            DECL_init_tree(DECL_decl_list(dcl))->dump();
            g_logmgr->decIndent(8);
        }

        g_logmgr->decIndent(2);
        note(g_logmgr, "\n");
        dcl = DECL_next(dcl);
    }
    g_logmgr->decIndent(2);
}


static void dump_stmts(Scope const* s)
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


static void dump_scope_list(Scope * s, UINT flag)
{
    while (s != nullptr) {
        s->dump(flag);
        s = SCOPE_nsibling(s);
    }
}


//Recusively dump Scope information, include symbol,
//label, type info, and trees.
void Scope::dump(UINT flag) const
{
    if (g_logmgr == nullptr) { return; }

    Scope const* s = this;
    xcom::DefFixedStrBuf buf;
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
    g_scope_list.destroy();
    g_scope_list.init();
}


UINT map_lab2lineno(LabelInfo const* li)
{
    return g_lab2lineno.get(li);
}


void set_map_lab2lineno(LabelInfo const* li, UINT lineno)
{
    g_lab2lineno.set(li, lineno);
}


void set_lab_used(LabelInfo const* li)
{
    g_lab_used.append(li);
}


bool is_lab_used(LabelInfo const* li)
{
    return g_lab_used.find(li);
}


//Return complete aggregate if it has same tag with given 'aggr'.
//The function will find aggregate from current scope and all of outer scopes.
Aggr const* Scope::retrieveCompleteType(Aggr const* aggr, bool is_struct)
{
    if (aggr->is_complete()) { return aggr; }
    Aggr * findone = nullptr;
    if (is_struct) {
        ASSERT0(aggr->getScope());
        if (isStructExistInOuterScope(aggr->getScope(), aggr->getTag(),
                                      true, (Struct**)&findone)) {
            ASSERT0(findone && findone->is_complete());
            return findone;
        }
        return nullptr;
    }

    ASSERT0(aggr->getScope());
    if (isUnionExistInOuterScope(aggr->getScope(), aggr->getTag(),
                                 true, (Union**)&findone)) {
        ASSERT0(findone && findone->is_complete());
        return findone;
    }
    return nullptr;
}

} //namespace xfe

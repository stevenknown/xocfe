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
#ifndef __SCOPE_H__
#define __SCOPE_H__

class EnumTab;
class UserTypeList;
class Struct;
class Union;
class Decl;
class Enum;

class SymList {
public:
    SymList * next;
    SymList * prev;
    Sym const* sym;
};
#define SYM_LIST_sym(syml) (syml)->sym
#define SYM_LIST_next(syml) (syml)->next
#define SYM_LIST_prev(syml) (syml)->prev


//Scope
// |
// |--EnumTab
// |--SYM_LIST
// |--TYPE_LIST
// |--SUB_SCOPE
// |--UserTypeList

#define MAX_SCOPE_FILED 4
#define GLOBAL_SCOPE 0 //Global memory space
#define FUNCTION_SCOPE 1 //Function unit
#define REGION_SCOPE 2 //Region unit

#define SCOPE_id(sc) ((sc)->id)
#define SCOPE_is_tmp_sc(sc) ((sc)->is_tmp_scope)
#define SCOPE_parent(sc) ((sc)->parent) //owner scope, namely parent node
#define SCOPE_nsibling(sc) ((sc)->next) //next sibling, growing to right way
#define SCOPE_sub(sc) ((sc)->sub) //sub scope
#define SCOPE_level(sc) ((sc)->level)
#define SCOPE_enum_list(sc) ((sc)->enum_list)
#define SCOPE_enum_tab(sc) ((sc)->enum_tab)
#define SCOPE_sym_list(sc) ((sc)->sym_tab_list)
#define SCOPE_user_type_list(sc) ((sc)->utl_list)
#define SCOPE_label_list(sc) ((sc)->li_list)
#define SCOPE_ref_label_list(sc) ((sc)->lref_list)
#define SCOPE_decl_list(sc) ((sc)->decl_list)
#define SCOPE_struct_list(sc) ((sc)->struct_list)
#define SCOPE_union_list(sc) ((sc)->union_list)
#define SCOPE_stmt_list(sc) ((sc)->stmt_list)
class Scope {
    COPY_CONSTRUCTOR(Scope);
public:
    UINT is_tmp_scope:1;
    UINT id:31; //unique id
    INT level; //nested level
    Scope * parent;
    Scope * next;
    Scope * prev;
    Scope * sub;
    EnumTab * enum_tab; //enum-type tab
    UserTypeList * utl_list; //record type defined with 'typedef'
    Decl * decl_list; //record identifier declaration info
    SymList * sym_tab_list; //record identifier name
    Tree * stmt_list; //record statement list to generate code
    List<LabelInfo*> li_list; //label definition
    List<LabelInfo*> lref_list;//reference label
    List<Struct*> struct_list; //structure list of current scope
    List<Union*> union_list; //union list of current scope

public:
    Scope(UINT & sc) { SCOPE_enum_tab(this) = nullptr; init(sc); }
    ~Scope() { destroy(); }

    void init(UINT & sc);
    void destroy();

    //Return nullptr indicate we haven't found it in 'ut_list', and
    //append 'ut' to tail of the list as correct, otherwise return
    //the finded one.
    Decl * addToUserTypeList(Decl * decl);

    //Be usually used in scope process.
    //Return nullptr if this function do not find 'sym' in 'sym_list', and
    //'sym' will be appended into list, otherwise return 'sym'.
    Sym const* addToSymList(Sym const* sym);
    Enum * addEnum(Enum * e);
    void addStruct(Struct * s);
    void addUnion(Union * u);
    void addDecl(Decl * decl);
    void addStmt(Tree * t);

    Tree * getStmtList() const { return SCOPE_stmt_list(this); }
    SymList * getSymList() const { return SCOPE_sym_list(this); }
    EnumTab * getEnumTab() const { return SCOPE_enum_tab(this); }
    Decl * getDeclList() const { return SCOPE_decl_list(this); }
    List<Struct*> * getStructList() { return &SCOPE_struct_list(this); }
    List<Union*> * getUnionList() { return &SCOPE_union_list(this); }

    //Return true if enum-value existed in current scope.
    //idx: the index that indicates the position of pacticular Item in Enum.
    bool isEnumExist(CHAR const* vname, OUT Enum ** e, OUT INT * idx) const;
};


typedef TMap<LabelInfo*, UINT> LAB2LINE_MAP;


class CompareLab {
public:
    bool is_less(LabelInfo * t1, LabelInfo * t2) const
    { return computeLabelHashValue(t1) < computeLabelHashValue(t2); }

    bool is_equ(LabelInfo * t1, LabelInfo * t2) const
    { return isSameLabel(t1, t2); }

    LabelInfo * createKey(LabelInfo * t) { return t; }
};
typedef TTab<LabelInfo*, CompareLab> LabelTab;


//Exported functions
//The options of dump_scope function.
#define DUMP_SCOPE_FUNC_BODY 0x1
#define DUMP_SCOPE_STMT_TREE 0x2
#define DUMP_SCOPE_RECUR 0x4

Scope * push_scope(bool is_tmp_sc);
Scope * pop_scope();
Scope * new_scope();
Scope * get_last_sub_scope(Scope * s);
Scope * get_global_scope();
void dump_scope(Scope * s, UINT flag);
void dump_scope_tree(Scope * s, INT indent);
void dump_scope_list(Scope * s, UINT flag);
void destroy_scope_list();
UINT map_lab2lineno(LabelInfo * li);
void set_map_lab2lineno(LabelInfo * li, UINT lineno);
void set_lab_used(LabelInfo * li);
bool is_lab_used(LabelInfo * li);

//Export Variables
extern Scope * g_cur_scope;
extern LabelTab g_labtab;

#endif

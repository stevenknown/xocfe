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

class EnumList;
class UserTypeList;
class Struct;
class Union;
class Decl;
class Enum;

class SYM_LIST {
public:
	SYM_LIST * next;
	SYM_LIST * prev;
	SYM * sym;
};
#define SYM_LIST_sym(syml) (syml)->sym
#define SYM_LIST_next(syml) (syml)->next
#define SYM_LIST_prev(syml) (syml)->prev


/*
SCOPE
	|
	|--EnumList
	|--SYM_TAB_LIST
	|--TYPE_LIST
	|--SUB_SCOPE
	|--UserTypeList
*/
#define MAX_SCOPE_FILED 4
#define GLOBAL_SCOPE    0  //Global memory space
#define FUNCTION_SCOPE  1  //Function unit
#define REGION_SCOPE    2    //Region unit

#define SCOPE_id(sc)				((sc)->id)
#define SCOPE_is_tmp_sc(sc)			((sc)->is_tmp_scope)
#define SCOPE_parent(sc)			((sc)->parent) //owner scope, namely parent node
#define SCOPE_nsibling(sc)			((sc)->next) //next sibling , growing to right way
#define SCOPE_sub(sc)				((sc)->sub) //sub scope
#define SCOPE_level(sc)				((sc)->level)
#define SCOPE_enum_list(sc)			((sc)->enum_list)
#define SCOPE_sym_tab_list(sc)		((sc)->sym_tab_list)
#define SCOPE_user_type_list(sc)	((sc)->utl_list)
#define SCOPE_label_list(sc)		((sc)->li_list)
#define SCOPE_ref_label_list(sc)	((sc)->lref_list)
#define SCOPE_decl_list(sc)			((sc)->decl_list)
#define SCOPE_struct_list(sc)		((sc)->struct_list)
#define SCOPE_union_list(sc)		((sc)->union_list)
#define SCOPE_stmt_list(sc)			((sc)->stmt_list)
class SCOPE {
public:
	INT id; //unique id
	INT level; //nested level
	bool is_tmp_scope;
	SCOPE * parent;
	SCOPE * next;
	SCOPE * prev;
	SCOPE * sub;

	EnumList * enum_list;		//enum-type list
	UserTypeList * utl_list;	//record type defined with 'typedef'

	List<LabelInfo*> li_list;	//label definition
	List<LabelInfo*> lref_list;//reference label

	List<Struct*> struct_list;	//structure list of current scope
	List<Union*> union_list;	//union list of current scope

	Decl * decl_list;			//record identifier declaration info
	SYM_LIST * sym_tab_list;	//record identifier name

	Tree * stmt_list;			//record statement list to generate code

	inline void init(UINT & sc)
	{
		li_list.init();
		lref_list.init();
		struct_list.init();
		union_list.init();

		SCOPE_id(this) = sc++;
		SCOPE_level(this) = -1;
		SCOPE_parent(this) = NULL;
		SCOPE_nsibling(this) = NULL;
		SCOPE_sub(this)  = NULL;
	}

	void destroy()
	{
		li_list.destroy();
		lref_list.destroy();
		struct_list.destroy();
		union_list.destroy();
	}
};


typedef TMap<LabelInfo*, UINT> LAB2LINE_MAP;


class Compare_Lab {
public:
	bool is_less(LabelInfo * t1, LabelInfo * t2) const
	{ return computeLabelHashValue(t1) < computeLabelHashValue(t2); }

	bool is_equ(LabelInfo * t1, LabelInfo * t2) const
	{ return isSameLabel(t1, t2); }
};
typedef TTab<LabelInfo*, Compare_Lab> LABTAB;


//Exported functions
#define DUMP_SCOPE_FUNC_BODY		1
#define DUMP_SCOPE_STMT_TREE		2

SCOPE * enter_sub_scope(bool is_tmp_sc);
SCOPE * return_to_parent_scope();
SCOPE * new_scope();
SCOPE * get_last_sub_scope(SCOPE * s);
SCOPE * get_global_scope();
void dump_scope(SCOPE * s, UINT flag);
void dump_scope_tree(SCOPE * s, INT indent);
void destroy_scope_list();
UINT map_lab2lineno(LabelInfo * li);
void set_map_lab2lineno(LabelInfo * li, UINT lineno);


//Export Variables
extern SCOPE * g_cur_scope;
extern LABTAB g_labtab;

//Export Functions
SYM * add_to_symtab_list(SYM_LIST ** sym_list , SYM * sym);
#endif


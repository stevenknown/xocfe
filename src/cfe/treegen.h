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
#ifndef __TREE_GEN_H__
#define __TREE_GEN_H__

//Exported Variables
extern CHAR * g_real_token_string;
extern TOKEN g_real_token;
extern bool g_enable_C99_declaration;
extern SMemPool * g_pool_general_used;
extern SMemPool * g_pool_tree_used; //front end
extern SMemPool * g_pool_st_used;
extern SymTab * g_fe_sym_tab;
extern bool g_dump_token;


//Exported Functions
void initParser();
void finiParser();

Tree * buildString(Sym const* str);
Tree * buildInt(HOST_INT val);
Tree * buildId(Decl const* decl);
Tree * buildAssign(Decl const* decl, Tree * rhs);
Tree * buildAssign(Tree * lhs, Tree * rhs);
Tree * buildArray(Decl const* decl, xcom::Vector<UINT> & subexp_vec);
Tree * buildArray(Tree * base, xcom::Vector<UINT> & subexp_vec);
Tree * buildIndmem(Tree * base, Decl const* fld);
Tree * buildDmem(Tree * base, Decl const* fld);
//Build aggregate reference tree node.
//decl: aggregate declaration.
//fldvec: record a list of indices in aggregate.
//  e.g: struct T { unsigned int b, c; struct Q {int m,n;} char e; };
//  m's fldvec is <2, 1>, e's fldvec is <3>.
Tree * buildAggrFieldRef(Decl const* decl, xcom::Vector<UINT> & fldvec);
Tree * buildAggrRef(Tree * base, Decl const* fld);

//Duplicate 't' and its kids, but without ir's sibiling node.
Tree * copyTree(Tree const* t);
Tree * copyTreeList(Tree const* t);

INT Parser();
Scope * compound_stmt(Decl * para_list);
Tree * conditional_exp();

void dump_tok_list();

Tree * gen_typename(Decl * decl);
Tree * gen_cvt(Decl const* tgt_type, Tree * src);

Tree * id();
bool is_in_first_set_of_exp_list(TOKEN tok);
bool is_user_type_exist_in_outer_scope(CHAR * cl, OUT Decl ** ut);
bool is_in_first_set_of_declarator();

Tree * exp();
Tree * exp_list();

bool look_forward_token(INT num, ...);

INT match(TOKEN tok);
UINT mapRealLineToSrcLine(UINT realline);

void setParent(Tree * parent, Tree * child);
void setLogMgr(LogMgr * lm);
INT suck_tok();
void suck_tok_to(INT placeholder, ...);
#endif

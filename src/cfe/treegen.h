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
extern INT g_real_line_num;
extern CHAR * g_real_token_string;
extern TOKEN g_real_token;
extern bool g_enable_C99_declaration;
extern FILE * g_hsrc;
extern SMEM_POOL * g_pool_general_used;
extern SMEM_POOL * g_pool_tree_used; //front end
extern SMEM_POOL * g_pool_st_used; 
extern SYM_TAB * g_fe_sym_tab;
extern bool g_dump_token;


//Exported Functions
void init_parser();
void fini_parser();
INT c_parser();
void set_parent(TREE * parent, TREE * child);
bool is_in_first_set_of_exp_list(TOKEN tok);
bool is_user_type_exist_in_outer_scope(CHAR * cl, OUT DECL ** ut);
INT match(TOKEN tok);
TREE * conditional_exp();
TREE * exp();
TREE * exp_list();
TREE * id();
SCOPE * compound_stmt(DECL * para_list);
bool look_forward_token(INT num, ...);
INT suck_tok();
void suck_tok_to(INT placeholder, ...);
void dump_tok_list();
bool is_in_first_set_of_declarator();
#endif


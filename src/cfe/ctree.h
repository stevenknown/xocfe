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
#ifndef __CTREE_H__
#define __CTREE_H__

namespace xfe {

#define TREE_ID_UNDEF 0

//EnumList
// |
// |--ENUM1
//      |--Enum-VAL1
//            |--name
//            |--integer value
//      |--Enum-VAL2
//   |--ENUM2
typedef enum _TREE_CODE {
    TR_NUL = 0,

    //One of '='  '*='  '/='  '%='  '+='  '-='  '<<='
    //'>>='  '&='  '^='  '|='
    TR_ASSIGN,
    TR_ID,
    TR_IMM, //signed integer
    TR_IMMU, //unsigned integer
    TR_IMML, //long long integer
    TR_IMMUL, //unsigned long long integer
    TR_FP, //double
    TR_FPF, //float
    TR_FPLD, //long double
    TR_ENUM_CONST,
    TR_STRING,
    TR_LOGIC_OR, //logical or ||
    TR_LOGIC_AND, //logical and &&
    TR_INCLUSIVE_OR, //inclusive or |
    TR_INCLUSIVE_AND, //inclusive and &
    TR_XOR, //exclusive or
    TR_EQUALITY, // == !=
    TR_RELATION, // < > >= <=
    TR_SHIFT, // >> <<
    TR_ADDITIVE, // '+' '-'
    TR_MULTI, // '*' '/' '%'
    TR_INTRI_FUN,
    TR_IF,
    TR_ELSE,
    TR_DO,
    TR_WHILE,
    TR_FOR,
    TR_SWITCH,
    TR_BREAK,
    TR_CONTINUE,
    TR_RETURN,
    TR_GOTO,
    TR_LABEL,
    TR_DEFAULT,
    TR_CASE,
    TR_COND, // formulized log_OR_exp ? exp : cond_exp
    TR_CVT, // type convertion
    TR_TYPE_NAME, // user defined type ord C standard type
    TR_LDA, // &a, load address of 'a'
    TR_DEREF, // *p dereferencing the pointer 'p'
    TR_INC, // ++a
    TR_DEC, // --a
    TR_POST_INC, // a++
    TR_POST_DEC, // a--
    TR_PLUS, // +123
    TR_MINUS, // -123
    TR_REV, // Reverse
    TR_NOT, // get non-value
    TR_SIZEOF, // sizeof(a)
    TR_DMEM, // a.b, direct memory access
    TR_INDMEM, // a->b, indirect memory access
    TR_ARRAY,
    TR_CALL, // function call
    TR_SCOPE, // record a scope
    TR_INITVAL_SCOPE, // record a scope which only permit expression-list.
    TR_DECL, // record node that indicates declaration of variable or type-name.
    TR_PRAGMA, // pragma
    TR_PREP, // preprocessor output info
} TREE_CODE;


#define TL_prev(tl) (tl)->prev
#define TL_next(tl) (tl)->next
#define TL_tok(tl) (tl)->token
#define TL_id_name(tl) (tl)->u1.id_name
#define TL_chars(tl) (tl)->u1.chars
#define TL_str(tl) (tl)->u1.string
#define TL_imm(tl) (tl)->u1.imm
class TokenList {
public:
    TokenList * prev;
    TokenList * next;
    TOKEN token;
    union {
        Sym const* id_name; //identifier list
        Sym const* chars; //character quoted by ''', e.g:'abcd'
        Sym const* string;
        UINT imm; //immediate
    } u1;

public:
    TokenList * get_prev() const { return TL_prev(this); }
    TokenList * get_next() const { return TL_next(this); }
    TOKEN getToken() const { return TL_tok(this); }
    Sym const* getIdName() const { return TL_id_name(this); }
    Sym const* getChars() const { return TL_chars(this); }
    Sym const* getStr() const { return TL_str(this); }
};


//The following macro defined accessing method of AST node.
//1. Unary operator: & * + - ~ ! indicate via TREE_lchild
//2. Binary operator: '=' '*=' '/=' '%=' '+=' '-=' '<<=' '>>=' '&=' '^='
//   indicated via TREE_lchild and TREE_rchild.
#define MAX_TREE_FLDS 4
#define TREE_id(tn) ((tn)->m_id)
#define TREE_token(tn) ((tn)->m_tok)
#define TREE_lineno(tn) ((tn)->m_lineno)
#define TREE_code(tn) ((tn)->m_tree_node_code)
#define TREE_result_type(tn) ((tn)->m_result_type_name)
#define TREE_fld(tn,N) ((tn)->m_fld[N]) //access no.N child of tree
#define TREE_parent(tn) ((tn)->m_parent) //parent tree node
#define TREE_nsib(tn) ((tn)->next) //next sibling(default)
#define TREE_psib(tn) ((tn)->prev) //prev sibling
#define TREE_rchild(tn) ((tn)->m_fld[0]) //rchild of the tree
#define TREE_lchild(tn) ((tn)->m_fld[1]) //lchild of the tree
#define TREE_token_lst(tn) ((tn)->u1.token_list) //Pragma

//If (determiannt) { then-stmt-list } else { else-stmt-list }
#define TREE_if_det(tn) ((tn)->m_fld[0])  //determinant of if-stmt
#define TREE_if_true_stmt(tn) ((tn)->m_fld[1]) //then-stmt of if-stmt
#define TREE_if_false_stmt(tn) ((tn)->m_fld[2]) //else-stmt of if-stmt

//for (init-list; determinant; step-list) { stmt-list }
#define TREE_for_init(tn) ((tn)->m_fld[0]) //initialize of for-stmt
#define TREE_for_det(tn) ((tn)->m_fld[1]) //determinant of for-stmt
#define TREE_for_step(tn) ((tn)->m_fld[2]) //step of for-stmt
#define TREE_for_body(tn) ((tn)->m_fld[3]) //body of for-stmt

//do {body} while (determinant)
#define TREE_dowhile_det(tn) ((tn)->m_fld[0]) //determinant of dowhile-stmt
#define TREE_dowhile_body(tn) ((tn)->m_fld[1]) //body of dowhile-stmt

//while (determinant) do {body}
#define TREE_whiledo_det(tn) ((tn)->m_fld[0]) //determinant of whiledo-stmt
#define TREE_whiledo_body(tn) ((tn)->m_fld[1]) //body of whiledo-stmt

//switch (determinant) { stmt-list }
#define TREE_switch_det(tn) ((tn)->m_fld[0]) //determinant of switch-stmt
#define TREE_switch_body(tn) ((tn)->m_fld[1]) //statement of switch-stmt

//conditional exp
#define TREE_det(tn) ((tn)->m_fld[0])
#define TREE_true_part(tn) ((tn)->m_fld[1])
#define TREE_false_part(tn) ((tn)->m_fld[2])

//converting exp
#define TREE_cvt_type(tn) ((tn)->m_fld[0])
#define TREE_type_name(tn) ((tn)->u1.type_name)
//#define TREE_ct_type(tn) ((tn)->u1.ty)
#define TREE_cvt_exp(tn) ((tn)->m_fld[1])

//array referecne
#define TREE_array_base(tn) (tn)->m_fld[0]
#define TREE_array_indx(tn) (tn)->m_fld[1]

//function invoke
#define TREE_fun_exp(tn) (tn)->m_fld[0]
#define TREE_para_list(tn) (tn)->m_fld[1]

//struct/union member reference
#define TREE_base_region(tn) (tn)->m_fld[0]
#define TREE_field(tn) (tn)->m_fld[1]

//return expression
#define TREE_ret_exp(tn) (tn)->m_fld[0]

//inc/pos-inc
#define TREE_inc_exp(tn) (tn)->m_fld[0]

//dec/post-dec
#define TREE_dec_exp(tn) (tn)->m_fld[0]

//sizeof exp
#define TREE_sizeof_exp(tn) (tn)->m_fld[0]

//enum def
#define TREE_enum(t) (t)->u1.u11.e
#define TREE_enum_val_idx(t) (t)->u1.u11.indx

//imm def
#define TREE_imm_val(t) (t)->u1.ival

//string def
#define TREE_string_val(t) (t)->u1.sval

//float number def
#define TREE_fp_str_val(t) (t)->u1.sval

//LABEL_STMT
//label def info
#define TREE_lab_info(t) (t)->u1.lab_info
//case label def info
#define TREE_case_value(t) (t)->u1.case_value

//goto target label name
#define TREE_lab_name(t) (t)->u1.lab_name

//represent an identifier and its declaration
#define TREE_id_name(t) (t)->u1.u12.id_name
#define TREE_id_decl(t) (t)->u1.u12.id_decl

//record a scope
#define TREE_scope(t) (t)->u1.scope

//record a for-scope
#define TREE_for_scope(t) (t)->u1.for_scope

//record an exp-list
#define TREE_initval_scope(t) (t)->u1.exp_scope

//TR_DECL uses it.
//record the declaration of variable or type-name
#define TREE_decl(t) (t)->u1.decl

class Tree {
public:
    UINT m_id;
    TREE_CODE m_tree_node_code;
    INT m_lineno; ///line number in src file
    TOKEN m_tok; //record the token that tree-node related.
    Tree * m_parent;
    Tree * next;
    Tree * prev;
    //Record DCL_TYPE_NAME that is an abstract type
    //specifier to describing the result-data-type while current
    //Tree operator is acted.
    Decl * m_result_type_name;
    Tree * m_fld[MAX_TREE_FLDS]; //for any other use

    union {
        struct {
            Enum const* e;
            INT indx;
        } u11; //record a enum constant
        struct {
            Sym const* id_name; //record symbol of TR_ID in SymTab
            Decl * id_decl; //record legal declaration
        } u12;
        CLSym const* sval; //record a string in C-language SymTab
        Sym const* lab_name; //record a label name in SymTab
        HOST_INT ival; //record an integer value
        LabelInfo const* lab_info; //record a label info defined
                                   //in function level
        INT  case_value; //record a constant value of jump-case table
        //Type * ty; //C standard type description
        //USER_TYPE * uty; //user type description
        Decl * type_name;
        Scope * scope; //record scope if tree is scope
        Tree * exp_scope; //record a exp-list
        TokenList * token_list; //record a token-list
        Scope * for_scope; //record scope if tree is for-stmt
        Decl * decl; //record the declaration of variable or type-name.
    } u1;

public:
    void dump() const;

    TREE_CODE getCode() const { return TREE_code(this); }
    TOKEN getToken() const { return TREE_token(this); }
    Decl * getResultType() const { return TREE_result_type(this); }
    Decl * getTypeName() const { return TREE_type_name(this); }
    INT getLineno() const { return TREE_lineno(this); }
    Tree * getArrayBase() const;

    UINT id() const { return TREE_id(this); }
    //Return true if given tree node can be regarded as RHS of assignment.
    bool isRHS() const;
    bool is_indirect_tree_node() const;
    bool is_imm_int() const;
    bool is_imm_fp() const;
    bool is_aggr_field_access() const;
    static bool is_type_spec(TOKEN tok);
    static bool is_type_quan(TOKEN tok);
    static bool is_stor_spec(TOKEN tok);

    Tree * lchild() const { return TREE_lchild(this); }
    Tree * rchild() const { return TREE_rchild(this); }
    Tree * parent() const { return TREE_parent(this); }
    Tree * nsib() const { return TREE_nsib(this); }
    Tree * psib() const { return TREE_psib(this); }

    void setParentForKid();
    static void setParent(Tree * parent, Tree * child)
    {
        if (child == nullptr) { return; }
        while (child != nullptr) {
            TREE_parent(child) = parent;
            child = TREE_nsib(child);
        }
    }
};

//Exported Functions
Tree * allocTreeNode(TREE_CODE tnt, INT lineno);

void dump_trees(Tree const* t);

//Get base of array if exist.
Tree * get_array_base(Tree * t);
//Get base of aggregate if exist.
Tree * get_aggr_base(Tree * t);
//Get base of aggregate/array if exist.
Tree * get_base(Tree * t);

//Exported Variables
extern UINT g_tree_count;

} //namespace xfe
#endif

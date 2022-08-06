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

static Stack<Cell*> g_cell_stack;
ST_INFO g_st_info[] = {
    {st_NULL, "nullptr"},

    //non-terminal chacartor as followed
    {st_ID, "id" },
    {st_IMM, "imm"},
    {st_IMML, "imml"},
    {st_FP, "floatpoint"},
    {st_STRING, "string"},
    {st_CHAR_LIST, "charlist"},
    {st_INTRI_FUN, ""},
    {st_INTRI_VAL, ""},
    {st_LLPAREN, "{"},
    {st_RLPAREN, "}"},
    {st_LSPAREN , "["},
    {st_RSPAREN , "]"},
    {st_ASSIGN, "="},
    {st_LPAREN, "("},
    {st_RPAREN, ")"},
    {st_ADD, "+"},
    {st_SUB, "-"},
    {st_MUL, "*"},
    {st_DIV, "/"},
    {st_AND, "&&"},
    {st_BITANDEQU, "&="},
    {st_OR, "||"},
    {st_AT, "@"},
    {st_BITAND, "&"},
    {st_BITOR, "|"},
    {st_BITOREQU, "|="},
    {st_LESSTHAN, "<" },
    {st_MORETHAN, ">"},
    {st_RSHIFT, ">>"},
    {st_RSHIFTEQU, ">>="},
    {st_LSHIFT, "<<"},
    {st_LSHIFTEQU, "<<="},
    {st_NOMORETHAN, "<="},
    {st_NOLESSTHAN, ">="},
    {st_NOEQU, "!="},
    {st_NOT, "!"},
    {st_EQU, "=="},
    {st_ADDEQU, "+="},
    {st_SUBEQU, "-="},
    {st_MULEQU, "*="},
    {st_DIVEQU, "/="},
    {st_XOR, "^"},
    {st_XOREQU, "^="},
    {st_MODEQU, "%="},
    {st_MOD, "%"},
    {st_COLON, ":"},
    {st_DCOLON, "::"},
    {st_SEMI, ";"  },
    {st_QUOT, "\""  },
    {st_COMMA, ", " },
    {st_UNDERLINE, "_"},
    {st_LANDSCAPE, "-"},
    {st_REV, "~"},
    {st_DOT, "."},
    {st_QUES_MARK, "?"},
    {st_ARROW, "->"},
    {st_ADDADD, "++"},
    {st_SUBSUB, "--"},

    //The following token is C specail.
    {st_DOTDOTDOT, "..."},

    //scalar-type-spec
    {st_VOID, "void"},
    {st_CHAR, "char" },
    {st_SHORT, "short"},
    {st_INT, "int"},
    {st_LONG, "long"},
    {st_FLOAT, "float"},
    {st_DOUBLE, "double"},
    {st_SIGNED, "signed"},
    {st_UNSIGNED, "unsigned"},
    {st_LONGLONG, "longlong"},

    //struct-or-union
    {st_STRUCT, "struct"},
    {st_UNION, "union"},

    //control-clause
    {st_IF, "if"},
    {st_ELSE, "else"},
    {st_GOTO, "goto"},
    {st_BREAK, "break"},
    {st_RETURN, "return"},
    {st_CONTINUE, "continue"},
    {st_DO, "do"},
    {st_WHILE, "while"},
    {st_SWITCH, "switch"},
    {st_CASE, "case"},
    {st_DEFAULT, "default"},
    {st_FOR, "for"},

    //storage-class-spec
    {st_AUTO, "auto"},
    {st_REGISTER, "register"},
    {st_EXTERN, "extern"},
    {st_INLINE, "inline"},
    {st_STATIC, "static"},
    {st_TYPEDEF, "typedef"},

    //qualifiers-pass
    {st_CONST, "const"},
    {st_VOLATILE, "volatile"},

    //unary-operator
    {st_SIZEOF, "sizeof"},

    //enum type
    {st_ENUM, "enum"},

    //clause statment marker, non-terminal chacartor
    {st_translation_unit, "trans_unit"},
    {st_external_declaration, "ext_decl"},
    {st_function_definition, "fun_def"},
    {st_declaration, "decl"},
    {st_declaration_list, "decl_list"},
    {st_declaration_specifiers, "decl_spec"},
    {st_storage_class_specifier, "stor_spec"},
    {st_type_specifier, "type_spec"},
    {st_type_qualifier, "type_qua"},
    {st_struct_or_union_specifier, "st_un_spec"},
    {st_struct_or_union, "st_un"},
    {st_struct_declaration_list, "st_decl_list"},
    {st_init_declarator_list, "init_decl_list"},
    {st_init_declarator, "init_decl"},
    {st_struct_declaration, "st_decl"},
    {st_specifier_qualifier_list, "spec_qua"},
    {st_struct_declarator_list, "st_decl_list"},
    {st_struct_declarator, "st_decl"},
    {st_enum_specifier, "enum_spec"},
    {st_enumerator_list, "enum_list"},
    {st_enumerator, "enum"},
    {st_declarator, "decltor"},
    {st_direct_declarator, "dir_decltor"},
    {st_pointer, "pointer"},
    {st_type_qualifier_list, "type_qua_list"},
    {st_parameter_type_list, "para_type_list"},
    {st_parameter_list, "para_list"},
    {st_parameter_declaration, "para_decl"},
    {st_identifier_list, "id_list"},
    {st_initializer, "init"},
    {st_initializer_list, "init_list"},
    {st_type_name, "type_name"},
    {st_abstract_declarator, "abs_decltor"},
    {st_direct_abstract_declarator, "dir_abs_decltor"},
    {st_typedef_name, "typedef_name"},
    {st_statement, "stmt"},
    {st_labeled_statement, "lab_stmt"},
    {st_expression_statement, "exp_stmt"},
    {st_compound_statement, "comp_stmt"},
    {st_statement_list, "stmt_list"},
    {st_selection_statement, "sel_stmt"},
    {st_iteration_statement, "iter"},
    {st_jump_statement, "jmp_stmt"},
    {st_expression, "exp"},
    {st_assignment_expression, "assign_exp"},
    {st_assignment_operator, "assign_op"},
    {st_conditional_expression, "cond_exp"},
    {st_constant_expression, "const_exp"},
    {st_logical_OR_expression, "or_exp"},
    {st_logical_AND_expression, "and_exp"},
    {st_inclusive_OR_expression, "bitor_exp"},
    {st_exclusive_OR_expression, "xor_exp"},
    {st_AND_expression, "bitand_exp"},
    {st_equality_expression, "equ_exp"},
    {st_relational_expression, "rela_exp"},
    {st_shift_expression, "shif_exp"},
    {st_additive_expression, "add_exp"},
    {st_multiplicative_expression, "mul_exp"},
    {st_cast_expression, "cast_exp"},
    {st_unary_expression, "unary_exp"},
    {st_unary_operator, "unary_op"},
    {st_postfix_expression, "postf_exp"},
    {st_argument_expression_list, "arg_exp_list"},
    {st_constant, "const"},
};


//Utility for st.cpp
SST pushst(SST st, size_t v)
{
    Cell * c = newcell(st);
    CELL_val(c) = (LONGLONG)v;
    CELL_line_no(c) = g_real_line_num;
    g_cell_stack.push(c);
    return st;
}


SST popst()
{
    Cell * c = g_cell_stack.pop();
    SST sst = c ? (SST)CELL_type(c) : st_NULL;
    free_cell(c);
    return sst;
}


//Return the SST of the Nth element from top of stack.
SST get_top_nth_st(INT n)
{
    Cell * c = g_cell_stack.get_top_nth(n);
    return c ? (SST)CELL_type(c) : st_NULL;
}


//check whether 'sst' exist in the SST stack.
INT is_sst_exist(SST sst)
{
    for (Cell * c = g_cell_stack.get_head();
         c != nullptr; c = g_cell_stack.get_next()) {
        if (CELL_type(c) == sst) {
            return 1;
        }
    }
    return 0;
}


SST get_top_st()
{
    Cell * c = g_cell_stack.get_top();
    return c != nullptr ? (SST)CELL_type(c) : st_NULL;
}


void dump_st_stack()
{
    for (Cell * c = g_cell_stack.get_head(); c; c = g_cell_stack.get_next()) {
        switch (CELL_type(c)) {
        case st_ID:
            prt(g_logmgr, "%s ", SYM_name((Sym*)CELL_val(c)));
            break;
        case st_IMM:
            prt(g_logmgr, "%d ", (INT)CELL_val(c));
            break;
        case st_IMML:
            prt(g_logmgr, "%lld ", (LONGLONG)CELL_val(c));
            break;
        case st_FP:
            prt(g_logmgr, "%s ", SYM_name((Sym*)CELL_val(c)));
            break;
        case st_STRING:
            prt(g_logmgr, "\"%s\" ", SYM_name((Sym*)CELL_val(c)));
            break;
        case st_CHAR_LIST:
            prt(g_logmgr, "'%s' ", SYM_name((Sym*)CELL_val(c)));
            break;
        default:
            prt(g_logmgr, "%s ", g_st_info[CELL_type(c)].name);
        } //end switch
    }
    note(g_logmgr, "\n");
}

} //namespace xfe

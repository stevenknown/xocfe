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
#ifndef __ST_H__
#define __ST_H__

//We must maintenance the order of the first lex statement values as the TOKEN
//enumeration presented.
//e.g  the current value of lex statement is st_null, suchlike concurrence of
//the first token of TOKEN enumeration , and both of them are zero.
typedef enum {
    st_NULL = 0,
    //non-terminal    chacartor  as followed
    st_ID,         // ID = (A-Z|a-z)( A-Z|a-z|0-9 )*
    st_IMM,        // 0~9
    st_IMML,       // 0~9L
    st_FP,         // decimal e.g 3.14
    st_STRING,     // "abcd"
    st_CHAR_LIST,  // 'abcd'
    st_INTRI_FUN,  //intrinsic function call
    st_INTRI_VAL,  //intrinsic value
    st_LLPAREN,    //{
    st_RLPAREN,    //}
    st_LSPAREN ,   //[
    st_RSPAREN ,   //]
    st_ASSIGN,     // =
    st_LPAREN,     // (
    st_RPAREN,     // )
    st_ADD,        // +
    st_SUB,        // -
    st_MUL,        // *
    st_DIV,        // /
    st_AND,        //&&
    st_BITANDEQU,  //&=
    st_OR,         //||
    st_AT,         //@
    st_BITAND,     //&
    st_BITOR,      //|
    st_BITOREQU,   //|=
    st_LESSTHAN,   // <
    st_MORETHAN,   // >
    st_RSHIFT,     // >>
    st_RSHIFTEQU,  // >>=
    st_LSHIFT,     // <<
    st_LSHIFTEQU,  // <<=
    st_NOMORETHAN, //<=
    st_NOLESSTHAN, //>=
    st_NOEQU,      //!=
    st_NOT,        //!
    st_EQU,        // ==
    st_ADDEQU,     // +=
    st_SUBEQU,     // -=
    st_MULEQU,     // *=
    st_DIVEQU,     // /=
    st_XOR,        // ^
    st_XOREQU,     // ^=
    st_MODEQU,     // %=
    st_MOD,        // %
    st_COLON,      // :
    st_DCOLON,     // ::
    st_SEMI,       // ;
    st_QUOT,       // "
    st_COMMA,      //,
    st_UNDERLINE,  //_
    st_LANDSCAPE,  //-
    st_REV,        //~  reverse  a = ~a
    st_DOT,        //.
    st_QUES_MARK,  //?
    st_ARROW,      // ->
    st_ADDADD,     // ++
    st_SUBSUB,     // --

    //The following token is C specail.
    st_DOTDOTDOT, //...

    //scalar-type-spec
    st_VOID,
    st_CHAR,
    st_SHORT,
    st_INT,
    st_LONG,
    st_FLOAT,
    st_DOUBLE,
    st_SIGNED,
    st_UNSIGNED,
    st_LONGLONG,

    //struct-or-union
    st_STRUCT,
    st_UNION,

    //control-clause
    st_IF,
    st_ELSE,
    st_GOTO,
    st_BREAK,
    st_RETURN,
    st_CONTINUE,
    st_DO,
    st_WHILE,
    st_SWITCH,
    st_CASE,
    st_DEFAULT,
    st_FOR,

    //storage-class-spec
    st_AUTO,
    st_REGISTER,
    st_EXTERN,
    st_INLINE,
    st_STATIC,
    st_TYPEDEF,

    //qualifiers-pass
    st_CONST,
    st_VOLATILE,

    //unary-operator
    st_SIZEOF,

    //enum type
    st_ENUM,

    //clause statment marker, non-terminal    chacartor
    st_translation_unit,
        //external_declaration
        //translation_unit external_declaration

    st_external_declaration,
        //function_definition
        //declaration

    st_function_definition,
        //declaration_specifiers(pass) declarator declaration_list(pass)
        //{ declaration-list(pass) statement-list(pass) }

    st_declaration,
        //declaration_specifiers init_declarator_list(pass);

    st_declaration_list,
        //declaration
        //declaration_list declaration

    st_declaration_specifiers,
        //storage_class_specifier declaration_specifiers(pass)
        //type_specifier declaration_specifiers(pass)
        //type_qualifier declaration_specifiers(pass)

    st_storage_class_specifier,
        //one of auto  register  static  extern  typedef

    st_type_specifier,
        //one of
        //void   char  short  int  long  float  double  signed
        //unsigned  struct_of_union_specifier  enum_specifier  typedef_name

    st_type_qualifier,
        //one of
        //const  volatile

    st_struct_or_union_specifier,
        //struct_or_union identifier(pass)  {struct_declaration_list}
        //struct_or_union identifier

    st_struct_or_union,
        //one of
        //struct  union

    st_struct_declaration_list,
        //struct_declaration
        //struct_declaration_list struct_declaration

    st_init_declarator_list,
        //init_declarator
        //init_declarator_list , init_declarator

    st_init_declarator,
        //declarator
        //declarator = initializer

    st_struct_declaration,
        //specifier_qualifier_list struct_declarator_list;

    st_specifier_qualifier_list,
        //type_specifier specifier_qualifier_list(pass)
        //type_qualifier specifier_qualifier_list(pass)

    st_struct_declarator_list,
        //struct_declarator
        //struct_declarator_list , struct_declarator

    st_struct_declarator,
        //declarator
        //declarator(pass) , constant_expression

    st_enum_specifier,
        //enum identifier(pass) { enumerator_list }
        //enum identifier

    st_enumerator_list,
        //enumerator
        //enumerator_list , enumerator

    st_enumerator,
        //identifier
        //identifier = constant_expression

    st_declarator,
        //pointer(pass) direct_declarator

    st_direct_declarator,
        //identifier
        //(declarator)
        //direct_declarator [ constant_expression(pass) ]
        //direct_declarator ( parameter_type_list )
        //direct_declarator ( identifier_list(pass) )

    st_pointer,
        //* type_qualifier_list(pass)
        //* type_qualifier_list(pass) pointer

    st_type_qualifier_list,
        //type_qualifier
        //type_qualifier_list type_qualifier

    st_parameter_type_list,
        //parameter_list
    //    parameter_list , ...

    st_parameter_list,
        //parameter_declaration
        //parameter_list , parameter_declaration

    st_parameter_declaration,
        //declaration_specifiers declarator
        //declaration_specifiers abstract_declarator(pass)

    st_identifier_list,
        //identifier
        //identifier_list , identifier

    st_initializer,
        //assignment_expression
        //{ initializer_list }
        //{ initializer_list , }

    st_initializer_list,
        //initializer
        //initializer_list , initializer

    st_type_name,
        //specifier_qualifier_list abstract_declarator(pass)

    st_abstract_declarator,
        //pointer
        //pointer(pass) direct_abstract_declarator

    st_direct_abstract_declarator,
        //( abstract_declarator )
        //direct_abstract_declarator(pass) [ constant_expression(pass) ]
        //direct_abstract_declarator(pass) ( parameter_type_list(pass) )

    st_typedef_name,
        //identifier

    st_statement,
        //labeled_statement
        //expression_statement
        //compound_statement
        //selection_statement
        //iteration_statement
        //jump_statement

    st_labeled_statement,
        //identifier , statement
        //case constant_expression , statement
        //default , statement

    st_expression_statement,
        //expression(pass);

    st_compound_statement,
        //{ declaration_list(pass) statement_list(pass) }

    st_statement_list,
        //statement
        //statement_list statement

    st_selection_statement,
        //if ( expression ) statement
        //if ( expression ) statement else statement
        //switch ( expression ) statement

    st_iteration_statement,
        //while ( expression ) statement
        //do statement while ( expression ) ;
        //for ( expression(pass) ; expression(pass) ; expression(pass) ) statement

    st_jump_statement,
        //goto identifier;
        //continue;
        //break;
        //return expression(pass);

    st_expression,
        //assignment_expression
        //expression , assignment_expression

    st_assignment_expression,
        //conditional_expression
        //unary_expression assignment_operator assignment_expression

    st_assignment_operator,
        //one of
        //=  *=  /=  %=  +=  _=  <<=  >>=  &=  ^=  |=

    st_conditional_expression,
        //logical_OR_expression
        //logical_OR_expression ? expression , conditional_expression

    st_constant_expression,
        //conditional_expression

    st_logical_OR_expression,
        //logical_AND_expression
        //logical_OR_expression || logical_AND_expression

    st_logical_AND_expression,
        //inclusive_OR_expression
        //logical_AND_expression && inclusive_OR_expression

    st_inclusive_OR_expression,
        //exclusive_OR_expression
        //inclusive_OR_expression | exclusive_OR_expression

    st_exclusive_OR_expression,
        //AND_expression
        //exclusive_OR_expression ^ AND_expression

    st_AND_expression,
        //equality_expression
        //AND_expression & equality_expression

    st_equality_expression,
        //relational_expression
        //equality_expression == relational_expression
        //equality_expression != relational_expression

    st_relational_expression,
        //shift_expression
        //relational_expression < shift_expression
        //relational_expression > shift_expression
        //relational_expression <= shift_expression
        //relational_expression <= shift_expression

    st_shift_expression,
        //additive_expression
        //shift_expression << additive_expression
        //shift_expression >> additive_expression

    st_additive_expression,
        //multiplicative_expression
        //additive_expression + multiplicative_expression
        //additive_expression _ multiplicative_expression

    st_multiplicative_expression,
        //cast_expression
        //multiplicative_expression * cast_expression
        //multiplicative_expression / cast_expression
        //multiplicative_expression % cast_expression

    st_cast_expression,
        //unary_expression
        //( type_name ) cast_expression

    st_unary_expression,
        //postfix_expression
        //++ unary_expression
        //-- unary_expression
        //unary_operator cast_expression
        //sizeof unary_expression
        //sizeof ( type_name )

    st_unary_operator,
        //one of
        //&  *  +  -  ~  !

    st_postfix_expression,
        //postfix_expression [ expression ]
        //postfix_expression ( argument_expression_list(pass) )
        //postfix_expression . identifier
        //postfix_expression -> identifier
        //postfix_expression ++
        //postfix_expression --
        //constant
        //string
        //( expression )

    st_argument_expression_list,
        //assignment_expression
        //argument_expression_list , assignment_expression

    st_constant,
        //integer_constant
        //character_constant
        //floating_constant
        //enumeration_constant

    st_END
} SST;


//ST_INFO
#define ST_INFO_name(sti)    (sti)->name
#define ST_INFO_sst(sti)    (sti)->sst
class ST_INFO {
public:
    SST sst;
    CHAR * name;
};


//Exported Variables
extern ST_INFO g_st_info[];

//Exported Functions
SST pushst(SST st, size_t v);
SST popst();
INT is_sst_exist(SST sst);
SST get_top_st();
SST get_top_nth_st(INT n);
void dump_st_stack();
#endif

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
#ifndef _LEX_
#define _LEX_

//C language's key words.
typedef enum _TOKEN {
    T_NUL = 0,        // NULL
    T_ID,            // ID = (A-Z|a-z)( A-Z|a-z|0-9 )*
    T_IMM,            // 0~9
    T_IMML,            // 0~9L
    T_IMMU,            // Unsigned
    T_IMMUL,        // Unsigned Long
    T_FP,            //double decimal e.g 3.14
    T_FPF,            //float decimal e.g 3.14
    T_FPLD,            //long double decimal e.g 3.14
    T_STRING,        // "abcd"
    T_CHAR_LIST,    // 'abcd'
    T_INTRI_FUN,    // intrinsic function call
    T_INTRI_VAL,    // intrinsic value
    T_LLPAREN,        // {
    T_RLPAREN,        // }
    T_LSPAREN,        // [
    T_RSPAREN,        // ]
    T_ASSIGN,        // =
    T_LPAREN,        // (
    T_RPAREN,        // )
    T_ADD,            // +
    T_SUB,            // -
    T_ASTERISK,        // *
    T_DIV,            // /
    T_AND,            // &&
    T_BITANDEQU,    // &=
    T_OR,            // ||
    T_AT,            // @
    T_BITAND,        // &
    T_BITOR,        // |
    T_BITOREQU,        // |=
    T_LESSTHAN,        // <
    T_MORETHAN,        // >
    T_RSHIFT,        // >>
    T_RSHIFTEQU,    // >>=
    T_LSHIFT,        // <<
    T_LSHIFTEQU,    // <<=
    T_NOMORETHAN,    // <=
    T_NOLESSTHAN,    // >=
    T_NOEQU,        // !=
    T_NOT,            // !
    T_EQU,            // ==
    T_ADDEQU,        // +=
    T_SUBEQU,        // -=
    T_MULEQU,        // *=
    T_DIVEQU,        // /=
    T_XOR,            // ^
    T_XOREQU,        // ^=
    T_REMEQU,        // %=
    T_MOD,            // %
    T_COLON,        // :
    T_DCOLON,        // ::
    T_SEMI,            // ;
    T_QUOT,            // "
    T_COMMA,        // ,
    T_UNDERLINE,    // _
    T_LANDSCAPE,    // -
    T_REV,            // ~ reverse  e.g:a = ~a
    T_DOT,            // .
    T_QUES_MARK,    // ?
    T_ARROW,        // ->
    T_ADDADD,        // ++
    T_SUBSUB,        // --

    //The following token is C specail.
    T_DOTDOTDOT,    // ...

    //scalar-type-spec
    T_VOID,
    T_CHAR,
    T_SHORT,
    T_INT,
    T_LONG,
    T_FLOAT,
    T_DOUBLE,
    T_SIGNED,
    T_UNSIGNED,
    T_LONGLONG,
    T_BOOL,

    //boolean
    T_TRUE,
    T_FALSE,

    //struct-or-union
    T_STRUCT,
    T_UNION,

    //control-clause
    T_IF,
    T_ELSE,
    T_GOTO,
    T_BREAK,
    T_RETURN,
    T_CONTINUE,
    T_DO,
    T_WHILE,
    T_SWITCH,
    T_CASE,
    T_DEFAULT,
    T_FOR,

    //storage-class-spec
    T_AUTO,
    T_REGISTER,
    T_EXTERN,
    T_INLINE,
    T_STATIC,
    T_TYPEDEF,

    //qualifiers-pass
    T_CONST,
    T_VOLATILE,
    T_RESTRICT,

    //unary-operator
    T_SIZEOF,
    T_ENUM,

    //pragma
    T_SHARP,        // #
    T_PRAGMA,        // pragma

    T_NEWLINE,        // \n
    ////////////////////////////////////
    //DO NOT ADD Enum AFTER THIS LINE.//
    ////////////////////////////////////
    T_END,            // end of file
} TOKEN;


#define TOKEN_INFO_name(ti)     (ti)->name
#define TOKEN_INFO_token(ti)    (ti)->tok
#define TOKEN_INFO_lineno(ti)   (ti)->u1.lineno
class TokenInfo {
public:
    TOKEN tok;
    CHAR * name;
    union{
        INT  lineno;
    } u1;
};


#define KEYWORD_INFO_name(ti)    (ti)->name
#define KEYWORD_INFO_token(ti)    (ti)->tok
class KeywordInfo {
public:
    TOKEN tok;
    CHAR * name;
};


#define MAX_BUF_LINE            4096
#define OFST_TAB_LINE_SIZE      (g_ofst_tab_byte_size / sizeof(LONG))

//Exported Variables
extern UINT g_src_line_num; //line number of src file
extern CHAR g_cur_token_string[]; //the string name of current token.
extern CHAR * g_cur_line; //the current line during parsing of src file.
extern UINT g_cur_line_len; //the current line buffer length.
extern TOKEN g_cur_token; //the current token.
extern LONG * g_ofst_tab; //record the byte offset of each line in src file.
extern LONG g_ofst_tab_byte_size;//record entry number of offset table.
extern bool g_enable_newline_token; //set true to regard '\n' as token.
extern FILE * g_hsrc; //the file handler of source file.
extern INT g_real_line_num;

//Exported Functions
//This is the first function you should invoke before start lex scanning.
void init_key_word_tab();

//Get current token.
TOKEN get_token();

//Get the string name of current token.
CHAR * get_token_name(TOKEN tok);

TokenInfo const* get_token_info(TOKEN tok);
#endif

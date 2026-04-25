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
#include "../com/xcominc.h"
#include "err.h"
#include "cfeinc.h"
#include "cfecommacro.h"
#include "lex.h"

#define OCTAL_LITERAL_LEN_IN_STRING 3
#define HEX_LITERAL_LEN_IN_STRING 2

namespace xfe {

static INT g_cur_token_string_pos = 0;
static CHAR g_cur_char = 0; //See details about the paper about LL1
static bool g_is_dos = true;
static INT g_cur_line_pos = 0;
static INT g_cur_line_num = 0;
static CHAR g_file_buf[LEX_MAX_BUF_LINE];
static INT  g_file_buf_pos = LEX_MAX_BUF_LINE;
static INT  g_last_read_num = 0;

//Set true to return the newline charactors as normal character.
static bool g_use_newline_char = true;
static UINT g_cur_src_ofst = 0; //Record current file offset of src file

UINT g_src_line_num = 0; //line number of src file
TOKEN g_cur_token = T_UNDEF;

//The string buffer which token were reside.
CHAR g_cur_token_string[LEX_MAX_BUF_LINE] = {0};
CHAR * g_cur_line = nullptr; //Current parsing line of src file
UINT g_cur_line_len = 0; //The current line buf length ,than read from file buf
LONG * g_ofst_tab = nullptr; //Record offset of each line in src file
LONG g_ofst_tab_byte_size = 0; //Record byte size position of Offset Table
bool g_enable_newline_token = false; //Set true to regard '\n' as token.

//If true, recognize the true and false token.
bool g_enable_true_false_token = true;
FILE * g_hsrc = nullptr;
INT g_real_line_num = 0;

//Record the number of disgarded line, that always
//sparking by preprecossor.
UINT g_disgarded_line_num = 0;

//Make sure following Tokens or Keywords is consistent with
//declarations of TOKEN enumeration declared in lex.h.
//CAVEAT: The order of tokens must be consistent
//with declarations order in lex.h.
static TokenInfo g_token_info[] =
{
    { T_UNDEF,      "" },
    { T_ID,         "id" },
    { T_IMM,        "int" },
    { T_IMML,       "long int" },
    { T_IMMLL,      "long long int" },
    { T_IMMU,       "unsigned int" },
    { T_IMMUL,      "unsigned long int" },
    { T_IMMULL,     "unsigned long long int" },
    { T_FP,         "decimal" },
    { T_FPF,        "float decimal" },
    { T_FPLD,       "long double decimal" },
    { T_STRING,     "string" },
    { T_CHAR_LIST,  "char list" },
    { T_INTRI_FUN,  "" },
    { T_INTRI_VAL,  "" },
    { T_LLPAREN,    "{" },
    { T_RLPAREN,    "}" },
    { T_LSPAREN,    "[" },
    { T_RSPAREN,    "]" },
    { T_ASSIGN,     "=" },
    { T_LPAREN,     "(" },
    { T_RPAREN,     ")" },
    { T_ADD,        "+" },
    { T_SUB,        "-" },
    { T_ASTERISK,   "*" },
    { T_DIV,        "/" },
    { T_AND,        "&&" },
    { T_BITANDEQU,  "&=" },
    { T_OR,         "||" },
    { T_AT,         "@" },
    { T_BITAND,     "&" },
    { T_BITOR,      "|" },
    { T_BITOREQU,   "|=" },
    { T_LESSTHAN,   "<"},
    { T_MORETHAN,   ">" },
    { T_RSHIFT,     ">>"},
    { T_RSHIFTEQU,  ">>=" },
    { T_LSHIFT,     "<<" },
    { T_LSHIFTEQU,  "<<=" },
    { T_NOMORETHAN, "<=" },
    { T_NOLESSTHAN, ">=" },
    { T_NOEQU,      "!=" },
    { T_NOT,        "!" },
    { T_EQU,        "==" },
    { T_ADDEQU,     "+=" },
    { T_SUBEQU,     "-=" },
    { T_MULEQU,     "*=" },
    { T_DIVEQU,     "/=" },
    { T_XOR,        "^" },
    { T_XOREQU,     "^=" },
    { T_REMEQU,     "%=" },
    { T_MOD,        "%" },
    { T_COLON,      ":" },
    { T_DCOLON,     "::" },
    { T_SEMI,       ";" },
    { T_QUOT,       "\"" },
    { T_COMMA,      "," },
    { T_UNDERLINE,  "_" },
    { T_LANDSCAPE,  "-" },
    { T_REV,        "~" },
    { T_DOT,        "." },
    { T_QUES_MARK,  "?" },
    { T_ARROW,      "->" },
    { T_ADDADD,     "++" },
    { T_SUBSUB,     "--" },

    //The following token is C specification.
    { T_DOTDOTDOT,  "..." },

    //Scalar-type-specification
    { T_VOID,       "void" },
    { T_CHAR,       "char" },
    { T_SHORT,      "short" },
    { T_INT,        "int" },
    { T_LONG,       "long" },
    { T_FLOAT,      "float" },
    { T_DOUBLE,     "double" },
    { T_SIGNED,     "signed" },
    { T_UNSIGNED,   "unsigned" },
    { T_LONGLONG,   "longlong" },
    { T_BOOL,       "bool" },

    { T_TRUE,       "true" },
    { T_FALSE,      "false" },

    //struct-or-union
    { T_STRUCT,     "struct" },
    { T_UNION,      "union" },

    //control-clause
    { T_IF,         "if" },
    { T_ELSE,       "else" },
    { T_GOTO,       "goto" },
    { T_BREAK,      "break" },
    { T_RETURN,     "return" },
    { T_CONTINUE,   "continue" },
    { T_DO,         "do" },
    { T_WHILE,      "while" },
    { T_SWITCH,     "switch" },
    { T_CASE,       "case" },
    { T_DEFAULT,    "default" },
    { T_FOR,        "for" },

    //storage-class-spec
    { T_AUTO,       "auto" },
    { T_REGISTER,   "register" },
    { T_EXTERN,     "extern" },
    { T_INLINE,     "inline" },
    { T_STATIC,     "static" },
    { T_TYPEDEF,    "typedef" },

    //qualifiers-pass
    { T_CONST,      "const" },
    { T_VOLATILE,   "volatile" },
    { T_RESTRICT,   "restrict" },

    //unary-operator
    { T_SIZEOF,     "sizeof" },
    { T_ENUM,       "enum" },
    { T_SHARP,      "#" },
    { T_PRAGMA,     "pragma" },
    { T_NEWLINE,    "\\n" },
    { T_END,        "" },
};


//Define Keywords which distinguish Identifiers.
static KeywordInfo g_keyword_info[] = {
    //Scalar-type-specification
    { T_VOID,       "void" },
    { T_CHAR,       "char" },
    { T_SHORT,      "short" },
    { T_INT,        "int" },
    { T_LONG,       "long" },
    { T_FLOAT,      "float" },
    { T_DOUBLE,     "double" },
    { T_SIGNED,     "signed" },
    { T_UNSIGNED,   "unsigned" },
    { T_LONGLONG,   "longlong" },
    { T_BOOL,       "bool" },
    { T_TRUE,       "true" },
    { T_FALSE,      "false" },

    //struct-or-union
    { T_STRUCT,     "struct" },
    { T_UNION,      "union" },

    //control-clause
    { T_IF,         "if" },
    { T_ELSE,       "else" },
    { T_GOTO,       "goto" },
    { T_BREAK,      "break" },
    { T_RETURN,     "return" },
    { T_CONTINUE,   "continue" },
    { T_DO,         "do" },
    { T_WHILE,      "while" },
    { T_SWITCH,     "switch" },
    { T_CASE,       "case" },
    { T_DEFAULT,    "default" },
    { T_FOR,        "for" },

    //storage-class-spec
    { T_AUTO,       "auto" },
    { T_REGISTER,   "register" },
    { T_EXTERN,     "extern" },
    { T_INLINE,     "inline" },
    { T_STATIC,     "static" },
    { T_TYPEDEF,    "typedef" },

    //qualifiers-pass
    { T_CONST,      "const" },
    { T_VOLATILE,   "volatile" },
    { T_RESTRICT,   "restrict" },

    //unary-operator
    { T_SIZEOF,     "sizeof" },
    { T_ENUM,       "enum" },

    //pragma
    { T_SHARP,      "#" },
    { T_PRAGMA,     "pragma" },
    { T_NEWLINE,    "\\n" },
};


static UINT g_keyword_num = sizeof(g_keyword_info)/sizeof(g_keyword_info[0]);


//The function read one line (end by '\n') from source code buffer.
//Return status which will be ST_SUCC or ST_ERR.
static INT getLine()
{
    //Initialize or realloc offset table.
    if (g_ofst_tab == nullptr) {
        g_ofst_tab_byte_size = LEX_MAX_OFST_BUF_LEN * sizeof(LONG);
        g_ofst_tab = (LONG*)::malloc(g_ofst_tab_byte_size);
        ::memset((void*)g_ofst_tab, 0, g_ofst_tab_byte_size);
    } else if (OFST_TAB_LINE_SIZE < (g_src_line_num + 10)) {
        g_ofst_tab = (LONG*)::realloc(g_ofst_tab, g_ofst_tab_byte_size +
                                      LEX_MAX_OFST_BUF_LEN * sizeof(LONG));
        ::memset((void*)(((BYTE*)g_ofst_tab) + g_ofst_tab_byte_size),
                 0, LEX_MAX_OFST_BUF_LEN * sizeof(LONG));
        g_ofst_tab_byte_size += LEX_MAX_OFST_BUF_LEN * sizeof(LONG);
    }
    UINT pos = 0;
    bool has_some_chars_in_cur_line = false;
    for (;;) {
        if (g_cur_line == nullptr) {
            g_cur_line_len = LEX_MAX_BUF_LINE;
            g_cur_line = (CHAR*)::malloc(g_cur_line_len);
            if (g_cur_line == nullptr) {
                goto FAILED;
            }
        }

        //Read the most LEX_MAX_BUF_LINE characters from source file.
        if (g_file_buf_pos >= g_last_read_num) {
            ASSERT0(g_hsrc);
            INT dw = (INT)::fread(g_file_buf, 1, LEX_MAX_BUF_LINE, g_hsrc);
            if (dw == 0) {
                if (!has_some_chars_in_cur_line) {
                    //Some characters had been put into 'g_cur_line', but the
                    //last character of 'g_file_buf' is not '0xD,0xA'. Thus we
                    //should keep reading characters till the last character in
                    //'g_file_buf. But there is nothing more can be
                    //read from source file until the last file read, so 'dw'
                    //is zero.
                    //This situation may take place when we are meeting the
                    //file that is terminated without a '0xD,0xA'.
                    //TODO:Considering the case, we should not return
                    //'FEOF' directly , instead we should process the last
                    //characters in 'g_cur_line' correctly.
                    goto FEOF;
                }
                goto FIN;
            }
            g_last_read_num = dw;
            g_last_read_num = MIN(g_last_read_num, LEX_MAX_BUF_LINE);
            g_file_buf_pos = 0;
        }
        //Get one line characters from buffer which end up with '0xD,0xA'
        //under DOS or '0xA' under Linux.
        bool is_0xd_recog = false;
        while (g_file_buf_pos < g_last_read_num) {
            if (g_file_buf[g_file_buf_pos] == 0xd &&
                //DOS line-end characters.
                g_file_buf[g_file_buf_pos + 1] == 0xa) {
                g_is_dos = true;
                if (g_use_newline_char) {
                    g_cur_line[pos] = g_file_buf[g_file_buf_pos];
                    pos++;
                    g_file_buf_pos++;
                    g_cur_line[pos] = g_file_buf[g_file_buf_pos];
                    pos++;
                    g_file_buf_pos++;
                } else {
                    g_file_buf_pos += 2;
                }
                g_cur_src_ofst += 2;
                g_src_line_num++;
                goto FIN;
            }
            if (g_file_buf[g_file_buf_pos] == 0xa) { //unix text format
                if (is_0xd_recog) {
                    //We have met '0xD', the '0xA' is one of
                    //the terminate string '0xD,0xA' under DOS text format.
                    if (g_use_newline_char) {
                        g_cur_line[pos] = g_file_buf[g_file_buf_pos];
                        pos++;
                        g_file_buf_pos++;
                    } else {
                        g_file_buf_pos++; //omit the terminate charactor '0xa'
                    }
                    is_0xd_recog = false;
                } else {
                    g_is_dos = false;
                    if (g_use_newline_char) {
                        g_cur_line[pos] = g_file_buf[g_file_buf_pos];
                        pos++;
                        g_file_buf_pos++;
                    } else {
                        g_file_buf_pos ++;
                    }
                }
                g_cur_src_ofst++;
                g_src_line_num++;
                goto FIN;
            }
            if (g_file_buf[g_file_buf_pos] == 0xd && g_is_dos) {
                //0xD is the last charactor in 'g_file_buf', thus 0xA
                //should be recognized in getNextToken() in order to guarantee
                //the lex token parsing correctly, or else some exceptions
                //occurred in text file.
                is_0xd_recog = true;
                if (g_use_newline_char) {
                    g_cur_line[pos] = g_file_buf[g_file_buf_pos];
                    pos++;
                    g_file_buf_pos++;
                } else {
                    g_file_buf_pos++;
                }
                g_cur_src_ofst++;
                goto FIN;
            }
            if (pos >= g_cur_line_len) {
                //Escalate the line buffer.
                g_cur_line_len += LEX_MAX_BUF_LINE;
                g_cur_line = (CHAR*)::realloc(g_cur_line, g_cur_line_len);
            }
            g_cur_line[pos] = g_file_buf[g_file_buf_pos];
            pos++;
            g_file_buf_pos++;
            has_some_chars_in_cur_line = true;
            g_cur_src_ofst++;
        }
    }
FIN:
    ASSERT0((g_src_line_num + 1) < OFST_TAB_LINE_SIZE);
    g_ofst_tab[g_src_line_num + 1] = g_cur_src_ofst;
    g_cur_line[pos] = 0;
    g_cur_line_num = (INT)strlen(g_cur_line);
    g_cur_line_pos = 0;
    return ST_SUCC;
FAILED:
    return ST_ERR;
FEOF:
    g_src_line_num++;
    g_cur_line[pos] = 0;
    g_cur_line_num = 0;
    g_cur_line_pos = 0;
    return ST_EOF;
}


class String2Token : public HMap<CHAR const*, TOKEN, HashFuncString2> {
public:
    String2Token(UINT bsize) : HMap<CHAR const*, TOKEN, HashFuncString2>(bsize)
    {}
    virtual ~String2Token() {}
};

String2Token g_str2token(0);

//This is the first function you should invoke before start lex scanning.
static void initKeyWordTab()
{
    g_str2token.init(64); //Must be power of 2 since we use HashFuncString2.
    for (UINT i = 0; i < g_keyword_num; i++) {
        g_str2token.set(KEYWORD_INFO_name(&g_keyword_info[i]),
                        KEYWORD_INFO_token(&g_keyword_info[i]));
    }
}


static void finiKeyWordTab()
{
    g_str2token.destroy(); //Must be power of 2 since we use HashFuncString2.
}


void initLexer()
{
    g_cur_token_string_pos = 0;
    g_cur_char = 0;
    g_is_dos = true;
    g_cur_line_pos = 0;
    g_cur_line_num = 0;
    g_file_buf[0] = 0;
    g_last_read_num = 0;
    g_cur_src_ofst = 0; //record current file offset of src file
    g_src_line_num = 0; //record line number of src file
    g_cur_token = T_UNDEF;
    g_real_line_num = 0;
    g_disgarded_line_num = 0;
    ASSERT0(g_cur_line == nullptr && g_cur_line_len == 0);
    ASSERT0(g_ofst_tab == nullptr && g_ofst_tab_byte_size == 0);
    ASSERTN(g_hsrc, ("src file handler not initialized"));
    initKeyWordTab();
}


void finiLexer()
{
    if (g_ofst_tab != nullptr) {
        ::free(g_ofst_tab);
        g_ofst_tab = nullptr;
        g_ofst_tab_byte_size = 0;
    }
    if (g_cur_line != nullptr) {
        ::free(g_cur_line);
        g_cur_line = nullptr;
        g_cur_line_len = 0;
    }
    finiKeyWordTab();
}


static TOKEN getKeyWord(CHAR const* s)
{
    if (s == nullptr) return T_UNDEF;
    return g_str2token.get(s);
}


UINT getCurTokenStringLen()
{
    //e.g:current token string is "ab\0c", the function return 4.
    return g_cur_token_string_pos;
}


//Get a charactor from g_cur_line.
//If it meets the EOF, the return value will be -1.
static CHAR getNextChar()
{
    CHAR res = '0';
    INT st = 0;
    if (g_cur_line == nullptr) {
        if ((st = getLine()) == ST_SUCC) {
            res = g_cur_line[g_cur_line_pos];
            g_cur_line_pos++;
        } else if(st == ST_EOF) {
            res = ST_EOF;
        }
    } else if (g_cur_line_pos < g_cur_line_num) {
        res = g_cur_line[g_cur_line_pos];
        g_cur_line_pos++;
    } else {
        st = getLine();
        if (st == ST_SUCC) {
            do {
                res = g_cur_line[g_cur_line_pos];
                g_cur_line_pos++;
                if (g_cur_line_num != 0) {
                    break;
                }
                st = getLine();
            } while (st == ST_SUCC);
        } else if (st == ST_EOF) {
              res = ST_EOF;
        }
   }
   return res;
}


////////////////////////////////////////////////////////////////////////////////
//YOU SHOULD CONSTRUCT THE FOLLOWING FUNCTION ACCRODING TO YOUR LEXICAL       //
//TOKEN WORD.                                                                 //
//BEGIN FROM NOW.                                                             //
////////////////////////////////////////////////////////////////////////////////

//The function is always used as post-process when parsing a token.
//If given token is an immediate, the function will parse and check whether
//the suffix if exist is legal.
//e.g:0x7fffull, 1.1f, etc.
//t: the latest token that has been parsed by previous functions.
static TOKEN parse_suffix(TOKEN t)
{
    ASSERT0(t != T_UNDEF);
    if (xcom::upper(g_cur_char) == 'L') {
        //e.g: 1000L
        //t is long integer.
        if (t == T_IMM) {
            t = T_IMML;
            g_cur_char = getNextChar();
            if (xcom::upper(g_cur_char) == 'L') {
                //e.g: 1000LL
                t = T_IMMLL;
                g_cur_char = getNextChar();
                if (xcom::upper(g_cur_char) == 'U') {
                    //e.g: 1000LLU <=> 1000ULL
                    t = T_IMMULL;
                    g_cur_char = getNextChar();
                }
                return t;
            }
            if (xcom::upper(g_cur_char) == 'U') {
                //e.g: 1000LU <=> 1000UL
                g_cur_char = getNextChar();
                return T_IMMUL;
            }
            return t;
        }
        if (t == T_FP) {
            //e.g: 1.11L
            //If suffixed by the letter L, imm has type with long double.
            g_cur_char = getNextChar();
            return T_FPLD;
        }
        return t;
    }
    if (xcom::upper(g_cur_char) == 'U') {
        if (t == T_IMM) {
            //e.g: 1000U
            //t is unsigned integer.
            t = T_IMMU;
            g_cur_char = getNextChar();
            if (xcom::upper(g_cur_char) == 'L') {
                //e.g: 1000UL
                t = T_IMMUL;
                g_cur_char = getNextChar();
                if (xcom::upper(g_cur_char) == 'L') {
                    //e.g: 1000ULL
                    t = T_IMMULL;
                    g_cur_char = getNextChar();
                }
                return t;
            }
            return t;
        }
        ASSERT0(t == T_FP);
        err(g_real_line_num, "invalid suffix \"%c\" on float constant",
            g_cur_char);
        g_cur_char = getNextChar();
        return t;
    }
    if (xcom::upper(g_cur_char) == 'F') {
        //e.g:1.0F, emphasize that immeidate is float rather than double.
        if (t == T_IMM) {
            err(g_real_line_num, "invalid suffix \"%c\" on integer constant",
                g_cur_char);
            return t;
        }
        ASSERT0(t == T_FP);
        g_cur_char = getNextChar();
        t = T_FPF;
        return t;
    }
    return t;
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
static TOKEN t_num()
{
    CHAR c = getNextChar();
    CHAR b_is_fp = 0;
    TOKEN t = T_UNDEF;
    if (g_cur_char == '0' && (xcom::upper(c) == 'X')) {
        //hex
        g_cur_token_string[g_cur_token_string_pos++] = c;
        while (xcom::xisdigithex(c = getNextChar())) {
            g_cur_token_string[g_cur_token_string_pos++] = c;
        }
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = c;
        t = T_IMM;
        goto SUFFIX;
    }
    if (g_cur_char == '0' && (xcom::upper(c) == 'B')) {
        //binary
        g_cur_token_string[g_cur_token_string_pos++] = c;
        while (xcom::xisdigitbin(c = getNextChar())) {
            g_cur_token_string[g_cur_token_string_pos++] = c;
        }
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = c;
        t = T_IMM;
        goto SUFFIX;
    }

    if (xcom::xisdigit(c) || c == '.') {
        //'c' is decimal.
        if (c == '.') {
            b_is_fp = 1;
        }
        g_cur_token_string[g_cur_token_string_pos++] = c;
        if (b_is_fp) { //there is already present '.'
           while (xcom::xisdigit(c = getNextChar())) {
               g_cur_token_string[g_cur_token_string_pos++] = c;
           }
        } else {
            while (xcom::xisdigit(c = getNextChar()) || c == '.') {
               if (c == '.') {
                   if (!b_is_fp){
                       b_is_fp=1;
                   } else {
                       break;
                   }
               }
               g_cur_token_string[g_cur_token_string_pos++] = c;
           }
        }
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = c;
        if (b_is_fp) { t = T_FP; }
        else { t = T_IMM; }
    } else {
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = c;
        t = T_IMM; //t is '0','1','2','3','4','5','6','7','8','9'
    }
SUFFIX:
    return parse_suffix(t);
}


//User input is: \X...
//e.g:'\X','\A-\F','\xdd','\aabb'
static bool try_handle_escape_hex_digit(MOD CHAR & c)
{
    ASSERT0(xcom::upper(c) == 'X' || xcom::xisdigithex_alpha(c));
    //Finally, the escape \ddd consists of the backslash followed
    //by
    // 1. not more than 3 octal digits (e.g:\765) or
    // 2. not more than 2 hex digits start with 'x' (e.g:\xAB) or
    // 3. any length of hex digits (e.g:\123456789ABCDEF)
    //which are taken to specify the desired character.
    bool only_allow_two_hex = false;
    if (xcom::upper(c) == 'X') {
        only_allow_two_hex = true;
        c = getNextChar();
    }
    UINT n = 0;
    while (xcom::xisdigithex(c)) {
        g_cur_token_string[g_cur_token_string_pos] = c;
        g_cur_token_string_pos++;
        n++;
        c = getNextChar();
    }
    if (n > HEX_LITERAL_LEN_IN_STRING && only_allow_two_hex) {
        err(g_real_line_num,
            "constant literal is too large, only permit two hex digits");
    }
    return true;
}


//User input is: \[0-9]...
//c: a character which belongs to the range of [0-9].
static bool try_handle_escape_digit(MOD CHAR & c)
{
    ASSERT0(xcom::xisdigit(c));
    //Finally, the escape \ddd consists of the backslash followed
    //by
    // 1. not more than 3 octal digits (e.g:\765) or
    // 2. not more than 2 hex digits start with 'x' (e.g:\xAB) or
    // 3. any length of hex digits (e.g:\123456789ABCDEF)
    //which are taken to specify the desired character.
    UINT n = 0;
    while (xcom::xisdigithex_octal(c) && n < OCTAL_LITERAL_LEN_IN_STRING) {
        g_cur_token_string[g_cur_token_string_pos] = c;
        g_cur_token_string_pos++;
        c = getNextChar();
        n++;
    }
    g_cur_token_string[g_cur_token_string_pos] = 0;
    g_cur_token_string_pos -= n;

    //longlong type truncated to char type. e.g:0x1f5 trunated to 0xf5.
    CHAR o = (CHAR)xcom::xatoll(
        &g_cur_token_string[g_cur_token_string_pos], true);
    g_cur_token_string[g_cur_token_string_pos] = o;
    g_cur_token_string_pos++;
    return true;
}


//c is escape char.
static bool try_handle_escape_char(MOD CHAR & c)
{
    ASSERT0(c == '\\');
    c = getNextChar();
    switch (c) {
    case 'n':
        //newline, 0xa
        g_cur_token_string[g_cur_token_string_pos++] = '\n';
        c = getNextChar();
        return true;
    case 't':
        //horizontal tab
        g_cur_token_string[g_cur_token_string_pos++] = '\t';
        c = getNextChar();
        return true;
    case 'b':
        //backspace
        g_cur_token_string[g_cur_token_string_pos++] = '\b';
        c = getNextChar();
        return true;
    case 'r':
        //carriage return, 0xd
        g_cur_token_string[g_cur_token_string_pos++] = '\r';
        c = getNextChar();
        return true;
    case 'f':
        //form feed
        g_cur_token_string[g_cur_token_string_pos++] = '\f';
        c = getNextChar();
        return true;
    case '\\':
        //backslash
        g_cur_token_string[g_cur_token_string_pos++] = '\\';
        c = getNextChar();
        return true;
    case '\'':
        //single quote
        g_cur_token_string[g_cur_token_string_pos++] = '\'';
        c = getNextChar();
        return true;
    case '"':
        //double quote
        g_cur_token_string[g_cur_token_string_pos++] = '"';
        c = getNextChar();
        return true;
    default:
        if (xcom::xisdigit(c)) {
            return try_handle_escape_digit(c);
        }
        if (xcom::upper(c) == 'X' || xcom::xisdigithex_alpha(c)) {
            return try_handle_escape_hex_digit(c);
        }
    }
    g_cur_token_string[g_cur_token_string_pos++] = '\\';
    g_cur_token_string[g_cur_token_string_pos++] = c;
    c = getNextChar();
    return false; //The parameter is not an escape char.
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
static TOKEN t_string()
{
    ASSERT0(g_cur_char == '"');
    CHAR c = getNextChar();
    while (c != '"') {
        if (c == '\\' && try_handle_escape_char(c)) {
            continue;
        }
        g_cur_token_string[g_cur_token_string_pos++] = c;
        c = getNextChar();
    }
    g_cur_char = getNextChar();
    g_cur_token_string[g_cur_token_string_pos] = 0;
    return T_STRING;
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
static TOKEN t_char_list()
{
    CHAR c = getNextChar();
    while (c != '\'') {
        if (c == '\\') {
            //c is escape char.
            c = getNextChar();
            if (c == 'n' ) {
                //newline, 0xa
                g_cur_token_string[g_cur_token_string_pos++] = '\n';
                c = getNextChar();
                continue;
            }
            if (c == 't') {
                //horizontal tab
                g_cur_token_string[g_cur_token_string_pos++] = '\t';
                c = getNextChar();
                continue;
            }
            if (c == 'b') {
                //backspace
                g_cur_token_string[g_cur_token_string_pos++] = '\b';
                c = getNextChar();
                continue;
            }
            if (c == 'r') {
                //carriage return, 0xd
                g_cur_token_string[g_cur_token_string_pos++] = '\r';
                c = getNextChar();
                continue;
            }
            if (c == 'f') {
                //form feed
                g_cur_token_string[g_cur_token_string_pos++] = '\f';
                c = getNextChar();
                continue;
            }
            if (c == '\\') {
                //backslash
                g_cur_token_string[g_cur_token_string_pos++] = '\\';
                c = getNextChar();
                continue;
            }
            if (c == '\'') {
                //single quote
                g_cur_token_string[g_cur_token_string_pos++] = '\'';
                c = getNextChar();
                continue;
            }
            if (c >= '0' && c <= '9') {
                //Finally, the escape \ddd consists of the backslash followed
                //by
                // 1. not more than 3 octal digits or
                // 2. not more than 2 hex digits start with 'x' or
                // 3. any length of hex digits
                //which are taken to specify the desired character.
                UINT n = 0;
                while ((c >= '0' && c <= '7') && n < 3) {
                    g_cur_token_string[g_cur_token_string_pos++] = c;
                    n++;
                    c = getNextChar();
                }
                g_cur_token_string[g_cur_token_string_pos] = 0;
                g_cur_token_string_pos -= n;

                //long type truncated to char type.
                CHAR o = (CHAR)xatoll(&g_cur_token_string[
                    g_cur_token_string_pos], true);
                g_cur_token_string[g_cur_token_string_pos++] = o;
                continue;
            }
            if (xcom::upper(c) == 'X' || (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'Z')) {
                ///e.g:'\x','\X', '\a-\f', '\A-\Z'.
                //     '\xdd' or '\aabb'
                bool only_allow_two_hex = false;
                if (xcom::upper(c) == 'X') {
                    only_allow_two_hex = true;
                    c = getNextChar();
                }
                UINT n = 0;
                while (xcom::xisdigithex(c)) {
                    g_cur_token_string[g_cur_token_string_pos++] = c;
                    n++;
                    c = getNextChar();
                }
                if (n > 2 && only_allow_two_hex) {
                    err(g_real_line_num,
                        "constant too big, only permit two hex digits");
                }
                continue;
            }
            g_cur_token_string[g_cur_token_string_pos++] = '\\';
            g_cur_token_string[g_cur_token_string_pos++] = c;
            c = getNextChar();
            continue;
        }
        g_cur_token_string[g_cur_token_string_pos++] = c;
        c = getNextChar();
    }
    g_cur_char = getNextChar();
    g_cur_token_string[g_cur_token_string_pos] = 0;
    return T_CHAR_LIST;
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
static TOKEN t_id()
{
    CHAR c = getNextChar();
    while (xcom::xisalpha(c) || c == '_' || xcom::xisdigit(c)) {
        g_cur_token_string[g_cur_token_string_pos++] = c;
        c = getNextChar();
    }
    g_cur_char = c;
    g_cur_token_string[g_cur_token_string_pos] = 0;
    TOKEN tok = getKeyWord(g_cur_token_string);
    if (tok != T_UNDEF) {
        return tok;
    }
    return T_ID;
}


static TOKEN t_solidus_solidus(bool * is_restart)
{
    TOKEN t = T_UNDEF;
    INT st = getLine();
    if (st == ST_SUCC) {
        g_cur_char = getNextChar();
        ASSERT0(is_restart);
        *is_restart = true;
        return T_UNDEF;
    }
    if (st == ST_EOF) {
        return T_END;
    }
    return t;
}


static TOKEN t_solidus_asterisk(bool * is_restart)
{
    TOKEN t = T_UNDEF;
    UINT cur_line_num = g_src_line_num;
    CHAR c = getNextChar();
    CHAR c1 = 0;
    for (;;) {
        cur_line_num = g_src_line_num;
        c1 = getNextChar();
        if (c == '*' && c1 == '/') {
            if (g_src_line_num == cur_line_num) {
                //We meet the multipul-comment terminated token '*/',
                //so change the parsing state to normal.
                g_cur_char = getNextChar();

                //CASE: recur_lex.c, Do NOT recursive call into
                //getNextToken() if meeting end of comments.
                //Avoid stack overflow.
                //t = getNextToken();
                ASSERT0(is_restart);
                *is_restart = true;
                return T_UNDEF;
            }
            c = c1;
            continue;
        }
        if (c == ST_EOF || c1 == ST_EOF) {
            return T_END;
        }
        c = c1;
    }
    return t;
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
//is_restart: record the result if lexer need to restart getNextToken().
static TOKEN t_solidus(bool * is_restart)
{
    CHAR c = getNextChar();
    switch (c) {
    case '=': // /=
        g_cur_token_string[g_cur_token_string_pos++] = '/';
        g_cur_token_string[g_cur_token_string_pos++] = c;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        return T_DIVEQU;
    case '/': //single comment line
        return t_solidus_solidus(is_restart);
    case '*': // multi comment line
        return t_solidus_asterisk(is_restart);
    default:
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = c;
        return T_DIV;
    }
    return T_UNDEF;
}


//'g_cur_char' hold the current charactor right now.
//You should assign 'g_cur_char' the next valid charactor before
//the function return.
TOKEN t_dot()
{
    //Here g_cur_char is '.'
    CHAR c = 0;
    TOKEN t;
    g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
    c = getNextChar();
    if (c == '.') {
        // token string is ..
        g_cur_token_string[g_cur_token_string_pos++] = c;
        c = getNextChar();
        if (c == '.') {
            // token string is ...
            g_cur_token_string[g_cur_token_string_pos++] = c;
            t = T_DOTDOTDOT;
            c = getNextChar();
        } else {
            //Here '..' is a invalid token
            t = T_UNDEF;
        }
    } else {
        //token string is '.'
        t = T_DOT;
    }
    g_cur_token_string[g_cur_token_string_pos] = 0;
    g_cur_char = c;
    return t;
}


CHAR const* getTokenName(TOKEN tok)
{
    return TOKEN_INFO_name(&g_token_info[tok]);
}


TokenInfo const* get_token_info(TOKEN tok)
{
    ASSERT0(tok <= T_END);
    return &g_token_info[tok];
}


static TOKEN t_rest(bool * is_restart)
{
    TOKEN token = T_UNDEF;
    switch (g_cur_char) {
    case '-':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '=': //'-='
            token = T_SUBEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        case '>': //'->'
            token = T_ARROW;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        case '-': //'--'
            token = T_SUBSUB;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: //'-'
            token = T_SUB;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '+':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '=': //'+='
            token = T_ADDEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        case '+': //'++'
            token = T_ADDADD;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: //'+'
            token = T_ADD;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '%':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '=': //'%='
            token = T_REMEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: //'%'
            token = T_MOD;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '^':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == '=') { //'^='
            token = T_XOREQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { //'^'
            token = T_XOR;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '=':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == '=') { //'=='
            token = T_EQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { //'='
            token = T_ASSIGN;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '*':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == '=') { //'*='
            token = T_MULEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { //'*'
            token = T_ASTERISK;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '&':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == '&') { //'&&'
            token = T_AND;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else if (g_cur_char == '=') { //&=
            token = T_BITANDEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { //'&'
            token = T_BITAND;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '|':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '|': //'||'
            token = T_OR;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        case '=': //|=
            token = T_BITOREQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: // '|'
            token = T_BITOR;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case ':':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == ':') { //'::'
            token = T_DCOLON;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { //':'
            token = T_COLON;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '>':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '>':
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_char = getNextChar();
            if (g_cur_char == '=') { // >>=
                token = T_RSHIFTEQU;
                g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
                g_cur_token_string[g_cur_token_string_pos] = 0;
                g_cur_char = getNextChar();
            } else { // >>
                token = T_RSHIFT;
                g_cur_token_string[g_cur_token_string_pos] = 0;
            }
            break;
        case '=': // '>='
            token = T_NOLESSTHAN;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: //'>'
            token = T_MORETHAN;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '<':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        switch (g_cur_char) {
        case '<':
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_char = getNextChar();
            if (g_cur_char == '=') { // <<=
                token = T_LSHIFTEQU;
                g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
                g_cur_token_string[g_cur_token_string_pos] = 0;
                g_cur_char = getNextChar();
            } else { // <<
                token = T_LSHIFT;
                g_cur_token_string[g_cur_token_string_pos] = 0;
            }
            break;
        case '=': // '<='
            token = T_NOMORETHAN;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
            break;
        default: // '<'
            token = T_LESSTHAN;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '!':
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_char = getNextChar();
        if (g_cur_char == '=') { // '!='
            token = T_NOEQU;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else { // '!'
            token = T_NOT;
            g_cur_token_string[g_cur_token_string_pos] = 0;
        }
        break;
    case '/':
        token = t_solidus(is_restart);
        break;
    case '.':
        token = t_dot();
        break;
    /////////////////////////////////////////
    //DO NOT ADD NEW CASES AFTER THIS LINE.//
    /////////////////////////////////////////
    default:
        if (g_cur_token == T_END) {
            //Meet file end.
            token = T_END;
        } else {
            //There may be error occurred.
            token = T_UNDEF;
        }
    } //end switch
    return token;
}


//Get current token.
TOKEN getNextToken()
{
    if (g_cur_token == T_END) { return g_cur_token; }
    TOKEN token = T_UNDEF;
    g_cur_token_string_pos = 0;
    g_cur_token_string[0] = 0;
    while (g_cur_char == 0) { g_cur_char = getNextChar(); }
START:
    switch (g_cur_char) {
    case ST_EOF:
        token = T_END; //Meet file end.
        break;
    case 0xa:
    case 0xd:
        //'\n'
        if (g_enable_newline_token && g_cur_char == 0xa) {
            token = T_NEWLINE;
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            g_cur_token_string[g_cur_token_string_pos] = 0;
            g_cur_char = getNextChar();
        } else {
            //CASE: recur_lex.c, Do NOT recursive call into
            //getNextToken() if meeting end of source code line.
            //Avoid stack overflow.
            //token = getNextToken();
            g_cur_char = getNextChar();
            goto START;
        }
        break;
    case '\t':
        while ((g_cur_char = getNextChar()) == '\t') { }
        token = getNextToken();
        break;
    case ' ':
        while((g_cur_char = getNextChar())==' ') { }
        token = getNextToken();
        break;
    case '@':
        token = T_AT;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case ';':
        token = T_SEMI;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case ',':
        token = T_COMMA;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char=getNextChar();
        break;
    case '{':
        token = T_LLPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '}':
        token = T_RLPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '[':
        token = T_LSPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case ']':
        token = T_RSPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '(':
        token = T_LPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case ')':
        token = T_RPAREN;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '~':
        token = T_REV;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '?':
        token = T_QUES_MARK;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    case '#':
        token = T_SHARP;
        g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
        g_cur_token_string[g_cur_token_string_pos] = 0;
        g_cur_char = getNextChar();
        break;
    default:
        if (g_cur_char == '"') { //string
            token = t_string();
        } else if (g_cur_char == '\'') { //char list
            token = t_char_list();
        } else if (xcom::xisalpha(g_cur_char) ||
                   g_cur_char == '_') { //identifier
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            token = t_id();
            if (g_enable_true_false_token &&
                (token == T_TRUE || token == T_FALSE)) {
                if (token == T_TRUE) {
                    g_cur_token_string[0] = '1';
                    g_cur_token_string[1] = 0;
                } else {
                    g_cur_token_string[0] = '0';
                    g_cur_token_string[1] = 0;
                }
                token = T_IMM;
                g_cur_token_string_pos = 1;
            }
        } else if (xcom::xisdigit(g_cur_char) != 0) { //imm
            g_cur_token_string[g_cur_token_string_pos++] = g_cur_char;
            token = t_num();
        } else {
            bool is_restart = false;
            token = t_rest(&is_restart);
            if (is_restart) {
                ASSERT0(token == T_UNDEF);
                goto START;
            }
        }
    }
    g_cur_token = token;
    return token;
}


#ifdef _DEBUG_
//Only for test.
void test_lex()
{
    getNextToken();
    while (g_cur_token != T_END) {
        if (g_cur_token == T_UNDEF) {
            printf("ERROR in line:%u\n\t", g_src_line_num);
            printf("S:%10s, T:%10s",
                g_cur_token_string,
                g_token_info[g_cur_token].name);
            break;
        }
        printf("S:%10s, T:%10s, L:%10u ",
            g_cur_token_string,
            g_token_info[g_cur_token].name,
            g_src_line_num);
        getNextToken();
    }
    printf("\n\n\n");
}
#endif

} //namespace xfe

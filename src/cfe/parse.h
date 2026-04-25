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
#ifndef __PARSE_H__
#define __PARSE_H__

namespace xfe {

class CParser {
    COPY_CONSTRUCTOR(CParser);
    bool initSrcFile(CHAR const* fn);
    void finiSrcFile();
public:
    CParser(xoc::LogMgr * lm, CHAR const* srcfile) { init(lm, srcfile); }
    ~CParser() { destroy(); }

    static Tree * conditional_exp();
    static Scope * compound_stmt(Decl * para_list);

    void destroy();
    void dump_tok_list();

    static Tree * exp();

    void init(xoc::LogMgr * lm, CHAR const* srcfile);
    static Tree * id();
    static Tree * id(Sym const* name, TOKEN tok);
    static bool isTerminateToken();
    //Return true if 'tok' indicate terminal charactor, otherwise false.
    static bool inFirstSetOfExp(TOKEN tok);

    //Match the current token with 'tok'.
    //Return ST_SUCC if matched, otherwise ST_ERR.
    static STATUS match(TOKEN tok);
    static UINT mapRealLineToSrcLine(UINT realline);

    static void setLogMgr(LogMgr * logmgr);

    //Start to parse a file.
    STATUS perform();
};

//Exported Variables
extern CHAR * g_real_token_string;
extern TOKEN g_real_token;
extern SMemPool * g_pool_general_used;
extern SMemPool * g_pool_tree_used; //front end
extern SMemPool * g_pool_st_used;
extern CLSymTab * g_fe_sym_tab;
extern LogMgr * g_logmgr; //the file handler of log file.

} //namespace xfe
#endif

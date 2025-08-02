/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

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

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#ifndef _DBG_H_
#define _DBG_H_

namespace xoc {

class Region;
class DbxMgr;

#define MAX_FILE_INDEX 65535
typedef xcom::Vector<xoc::Sym const*> FileIdx2FileName;

enum LANG_TYPE {
    LANG_UNDEF = 0,
    LANG_CPP, //C++
    LANG_PYTHON, //Python
    LANG_GR, //GR
    LANG_LAST_COMMON = LANG_GR,

    #include "dbg_lang_ext.inc"

    ///////////////////////////////////////////////////////////////////////////
    //DO NOT ADD NEW LANG AFTER LANG_NUM.                                    //
    ///////////////////////////////////////////////////////////////////////////
    LANG_NUM //The number of LANG.
};

class LangInfo {
    COPY_CONSTRUCTOR(LangInfo);
public:
    typedef xcom::TMap<LANG_TYPE, UINT8> LANG2INDEX;
public:
    //Record the index for each language.
    LANG2INDEX m_frontend_lang_to_index;
public:
    LangInfo() { m_frontend_lang_to_index.clean(); };
    UINT8 getLangIndex(LANG_TYPE lang) const;
    UINT8 getLangNum() const;
    void setLangIndex(LANG_TYPE lang, UINT8 index);
};

typedef xcom::TMap<LANG_TYPE, FileIdx2FileName*> Lang2FileIdx2FileName;
typedef xcom::TMapIter<LANG_TYPE, FileIdx2FileName*>
    Lang2FileIdx2FileNameIter;

//Describe debug information.
//Note 0 indicates the line number info is invalid.
//In future, it will be determined that if the line number is 0,
//it means that this IR does not have any debugging information.
#define DBX_UNDEF 0
#define DBX_debug_infos(d)  ((d)->m_debug_infos)
class Dbx {
    Dbx(Dbx const&);
public:
    struct DebugInfo {
        UINT32 m_col_offset;
        UINT32 m_file_index;
        UINT32 m_lineno;  //Note line number cannot be 0.

        //Currently, in the actual project,
        //only two bits are used(Low 1 bit and 3 bit),
        //They are all common terms in compilers.
        //described as follows:
        //Low 1 bit: Flag for stmt.
        //Low 2 bit: Flag for basic block.
        //Low 3 bit: Flag for prologue_end.
        //Low 4 bit: Flag for epilogue_begin.
        //Below are the explanations for each name:
        //Low 1 bit: stmt represents whether it is a line,
        //e.g: .loc 1 55 66 stmt 1, .loc 1 56 88 stmt 1, .loc 1 56 99 stmt 0.
        //It can be observed that the first command is on line 55,
        //with stmt set to 1. The second one is on line 56,
        //with stmt also set to 1. The third one is still on line 56,
        // therefore stmt is set to 0.
        //Low 2 bit: In the current context, the 'loc' instruction
        //comes with a BB attribute but we haven't encountered it yet.
        //Low 3 bit: .loc signifies the end of a stack frame.
        //Low 4 bit: .loc signifies the start of a stack frame.
        UINT8 m_flag;
    };
    DebugInfo * m_debug_infos;
public:
    //The dbxMgr of each region is responsible for allocating memory
    //for the m_debug_infos in dbx, so each dbx must
    //be born and die with the region.
    Dbx() { m_debug_infos = nullptr; }
    ~Dbx() {}
    void clean(DbxMgr * dbx_mgr);
    void copy(Dbx const& dbx, DbxMgr * dbx_mgr);
    UINT32 getLine(LANG_TYPE lang, DbxMgr * dbx_mgr) const;
    UINT32 getColOffset(LANG_TYPE lang, DbxMgr * dbx_mgr) const;
    UINT32 getFileIndex(LANG_TYPE lang, DbxMgr * dbx_mgr) const;
    UINT8 getFlag(LANG_TYPE lang, DbxMgr * dbx_mgr) const;

    //During initialization, we will allocate memory for the m_debug_infos.
    //Note that we use the pool of dbgMgr.
    void init(DbxMgr * dbx_mgr);
    void setLine(LANG_TYPE lang, UINT32 line, DbxMgr * dbx_mgr);
    void setColOffset(LANG_TYPE lang, UINT32 col_offset,
                      DbxMgr * dbx_mgr);
    void setFileIndex(LANG_TYPE lang, UINT32 file_index,
                      DbxMgr * dbx_mgr);
    void setFlag(LANG_TYPE lang, UINT8 flag, DbxMgr * dbx_mgr);
};


#define DBXMGR_lang_info(d)  ((d)->m_lang_info)
class DbxMgr {
    COPY_CONSTRUCTOR(DbxMgr);
    SMemPool * m_pool;
    Lang2FileIdx2FileName m_lang2fi2fn;
public:
    #define PRTCTX_lang(p)  ((p)->m_lang)
    class PrtCtx {
    public:
        //Record the string will be prefix of dbx information.
        char const* prefix;

        //Record the IO handler.
        LogMgr * logmgr;

        //Required language.
        xoc::LANG_TYPE m_lang;
    public:
        PrtCtx(xoc::LANG_TYPE lang)
        {
            prefix = nullptr;
            logmgr = nullptr;
            m_lang = lang;
        }

        LogMgr * getLogMgr() const { return logmgr; }
    };
    LangInfo m_lang_info;
public:
    DbxMgr()
    {
        m_pool = nullptr;
        init();
    }
    virtual ~DbxMgr() { destroy(); }

    //Allocate memory for dbx based on the number of front-end languages.
    Dbx::DebugInfo * allocDbxInfos();

    //Allocate memory for dbx.
    Dbx * allocDbx();

    //Allocate memory for file vector.
    FileIdx2FileName * allocFileVec();

    void destroy();

    //Do some prepare work before print source file.
    virtual void doPrepareWorkBeforePrint() {}

    xoc::Sym const* getFileName(LANG_TYPE lang, UINT fileidx) const;

    void init();

    //Print source code line in internal stream.
    virtual void printSrcLine(IR const*, PrtCtx * ctx);

    //Print source code line in internal stream.
    virtual void printSrcLine(StrBuf &, IR const*, PrtCtx * ctx);

    //Print source code line in internal stream.
    virtual void printSrcLine(Dbx const*, PrtCtx *)
    {
        //Taget Dependent Code
        //Nothing to do.
    }

    //Print source code line in StrBuf.
    virtual void printSrcLine(StrBuf &, Dbx const*, PrtCtx *)
    {
        //Taget Dependent Code
        //Nothing to do.
    }

    void setFileName(LANG_TYPE lang, UINT fileidx, xoc::Sym const* s);
    virtual void setLangInfo();

    void * xmalloc(size_t size);
};


//Copy Dbx information from 'src' to 'tgt'.
void copyDbx(IR * tgt, IR const* src, Region * rg);

//Copy Dbx information from 'dbx' to 'tgt'.
void copyDbx(IR * tgt, Dbx const* dbx, Region * rg);

//Copy dbx from 'src' to each element in 'tgt_list'.
void copyDbxForList(IR * tgt_list, IR const* src, Region * rg);

//Copy dbx from 'dbx' to each element in 'tgt_list'.
void copyDbxForList(IR * tgt_list, Dbx const* dbx, Region * rg);

//Get source file line number of 'ir'.
UINT getLineNum(IR const* ir, LANG_TYPE language, DbxMgr * dbx_mgr);

//Get source file line number in 'dbx'.
UINT getLineNum(Dbx const* dbx, LANG_TYPE language, DbxMgr * dbx_mgr);
UINT getLineNumOfCPP(Dbx const* dbx, Region const* rg);
UINT getLineNumOfPython(Dbx const* dbx, Region const* rg);
UINT getLineNumOfGR(Dbx const* dbx, Region const* rg);

//Return the dbx of 'ir'.
Dbx * getDbx(IR const* ir);

//Set source file line number to 'ir'.
void setLineNum(IR * ir, UINT lineno, Region * rg,
                LANG_TYPE language = LANG_CPP);

//Set source file loc to 'ir'.
void setLoc(IR * ir, Region * rg, UINT line, UINT col,
            UINT file_index, LANG_TYPE language);

//Set dbx to 'ir'.
void setDbx(IR * ir, Dbx * dbx, Region * rg);

} //namespace xoc
#endif

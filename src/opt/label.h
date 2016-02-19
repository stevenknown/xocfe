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
#ifndef __LABEL_H__
#define __LABEL_H__

namespace xoc {

typedef enum {
    L_UNDEF = 0,
    L_CLABEL, //customer defined label
    L_ILABEL, //internal generated label
    L_PRAGMA, //pragma
} LABEL_TYPE;

#define PREFIX_OF_LABEL()     "_L"
#define POSTFIX_OF_LABEL()    ""
#define PREFIX_OF_CLABEL()    ""
#define POSTFIX_OF_CLABEL()   ""

#define ILABEL_STR_FORMAT  "%s%d%s" //prefix label-num postfix
#define ILABEL_CONT(li) \
    PREFIX_OF_LABEL(),LABEL_INFO_num(li),POSTFIX_OF_LABEL()

#define CLABEL_STR_FORMAT  "%s%s%s" //prefix label-name postfix
#define CLABEL_CONT(li) \
    PREFIX_OF_CLABEL(), SYM_name(LABEL_INFO_name(li)), POSTFIX_OF_CLABEL()


#define LABEL_INFO_type(l)              ((l)->ltype)
#define LABEL_INFO_name(l)              ((l)->u1.lab_name)
#define LABEL_INFO_num(l)               ((l)->u1.lab_num)
#define LABEL_INFO_pragma(l)            ((l)->u1.pragma_str)
#define LABEL_INFO_is_used(l)           ((l)->u2.s1.is_used)
#define LABEL_INFO_is_catch_start(l)    ((l)->u2.s1.is_catch_start)
#define LABEL_INFO_is_try_start(l)      ((l)->u2.s1.is_try_start)
#define LABEL_INFO_is_try_end(l)        ((l)->u2.s1.is_try_end)
#define LABEL_INFO_is_unreachable(l)    ((l)->u2.s1.is_unreachable)
#define LABEL_INFO_is_pragma(l)         (LABEL_INFO_type(l) == L_PRAGMA)
#define LABEL_INFO_b1(l)                ((l)->u2.b1)
class LabelInfo {
public:
    LABEL_TYPE ltype;

    union {
        SYM * lab_name;
        UINT lab_num;
        SYM * pragma_str;
    } u1;

    union {
        struct {
            BYTE is_used:1;

            //Set true if current label is the start
            //label of exception catch block.
            BYTE is_catch_start:1;

            //Set true if current label is the start
            //label of exception try block.
            BYTE is_try_start:1;

            //Set true if current label is the end
            //label of exception try block.
            BYTE is_try_end:1;

            //Set true if current label is a placeholer to indicate that
            //program control flow is terminate here.
            BYTE is_unreachable:1;
        } s1;
        BYTE b1;
    } u2;

public:
    void copy(LabelInfo const& li)
    {
        ltype = li.ltype;
        u1.lab_name = li.u1.lab_name;
        u2.b1 = li.u2.b1;
    }
};


//Exported Functions
//Simplest method to compute hash value.
inline UINT computeLabelHashValue(LabelInfo const* li)
{
    INT v = 0;
    if (LABEL_INFO_type(li) == L_CLABEL) {
        CHAR const* p = SYM_name(LABEL_INFO_name(li));
        while (*p != 0) {
            v += *p;
            p++;
        }
    } else {
        ASSERT0(LABEL_INFO_type(li) == L_ILABEL);
        v = (INT)LABEL_INFO_num(li);
        v = (INT)((((UINT)(-v)) >> 7) ^ 0xAC5AAC5A);
    }
    return (UINT)v;
}


LabelInfo * allocLabel(SMemPool * pool);
LabelInfo * allocInternalLabel(SMemPool * pool);
LabelInfo * allocCustomerLabel(SYM * st, SMemPool * pool);
inline bool isSameLabel(LabelInfo const* li1, LabelInfo const* li2)
{
    ASSERT0(li1 && li2);
    if (li1 == li2) { return true; }
    if (LABEL_INFO_type(li1) == LABEL_INFO_type(li2) &&
        LABEL_INFO_num(li1) == LABEL_INFO_num(li2)) {
        return true;
    }
    return false;
}
void dumpLabel(LabelInfo const* li);


class LabelHashFunc : public HashFuncBase<LabelInfo*> {
public:
    UINT get_hash_value(LabelInfo * li, UINT bucket_size) const
    { return ((UINT)computeLabelHashValue(li)) % bucket_size; }

    bool compare(LabelInfo * li1, LabelInfo * li2) const
    { return isSameLabel(li1, li2); }
};


class CustomerLabelHashFunc : public HashFuncBase<LabelInfo const*> {
public:
    UINT get_hash_value(LabelInfo const* li, UINT bucket_size) const
    { return ((UINT)computeLabelHashValue(li)) % bucket_size; }

    bool compare(LabelInfo const* li1, LabelInfo const* li2) const
    { return isSameLabel(li1, li2); }
};

} //namespace xoc
#endif

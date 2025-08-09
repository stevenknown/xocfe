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

ErrList g_err_msg_list;
WarnList g_warn_msg_list;

static void * xmalloc(size_t size)
{
    BYTE * p = (BYTE*)smpoolMalloc(size, g_pool_general_used);
    if (p == nullptr) { return nullptr; }
    ::memset((void*)p, 0, size);
    return (void*)p;
}


void show_err()
{
    if (!g_err_msg_list.has_msg()) { return; }
    fprintf(stdout, "\n");
    for (ErrMsg * e = g_err_msg_list.get_head();
         e != nullptr; e = g_err_msg_list.get_next()) {
        fprintf(stdout, "\nerror(%d):%s", ERR_MSG_lineno(e), ERR_MSG_msg(e));
    }
    fprintf(stdout, "\n");
}


void show_warn()
{
    if (!g_warn_msg_list.has_msg()) { return; }
    fprintf(stdout, "\n");
    for (WarnMsg * e = g_warn_msg_list.get_head();
         e != nullptr; e = g_warn_msg_list.get_next()) {
        fprintf(stdout, "\nwarning(%d):%s",
                WARN_MSG_lineno(e), WARN_MSG_msg(e));
    }
    fprintf(stdout, "\n");
}


//Report warning with line number.
void warn(INT line_num, CHAR const* msg, ...)
{
    if (msg == nullptr) { return; }
    WarnMsg * p = nullptr;
    StrBuf sbuf(64);
    va_list arg;
    va_start(arg, msg);
    sbuf.vsprint(msg, arg);
    p = (WarnMsg*)xmalloc(sizeof(WarnMsg));
    size_t l = sbuf.strlen();
    p->msg = (CHAR*)xmalloc(l + 1);
    ::memcpy(p->msg, sbuf.buf, l);
    p->msg[l] = 0;
    p->lineno = line_num;
    g_warn_msg_list.append_tail(p);
    va_end(arg);
}


//Report error with line number.
void err(INT line_num, CHAR const* msg, ...)
{
    if (msg == nullptr) { return; }
    ErrMsg * p = nullptr;
    StrBuf sbuf(64);
    va_list arg;
    va_start(arg, msg);
    sbuf.vsprint(msg, arg);
    p = (ErrMsg*)xmalloc(sizeof(ErrMsg));
    size_t l = sbuf.strlen();
    p->msg = (CHAR*)xmalloc(l + 1);
    ::memcpy(p->msg, sbuf.buf, l);
    p->msg[l] = 0;
    p->lineno = line_num;
    g_err_msg_list.append_tail(p);
    va_end(arg);
}


INT is_too_many_err()
{
    return g_err_msg_list.get_elem_count() > TOO_MANY_ERR;
}

} //namespace xfe

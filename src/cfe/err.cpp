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
#include "cfecom.h"

List<ERR_MSG*> g_err_msg_list;
List<WARN_MSG*> g_warn_msg_list;

static void * xmalloc(size_t size)
{
    void * p = smpoolMalloc(size, g_pool_general_used);
    if (p == NULL) return NULL;
    ::memset(p, 0, size);
    return p;
}


void show_err()
{
    if (g_err_msg_list.get_elem_count() == 0) return;
    fprintf(stdout,"\n");
    for (ERR_MSG * e = g_err_msg_list.get_head();
         e != NULL; e = g_err_msg_list.get_next()) {
        fprintf(stdout, "\nerror(%d):%s", ERR_MSG_lineno(e), ERR_MSG_msg(e));
    }
    fprintf(stdout,"\n");
}


void show_warn()
{
    if (g_warn_msg_list.get_elem_count() == 0) return;
    fprintf(stdout,"\n");
    for (WARN_MSG * e = g_warn_msg_list.get_head();
         e != NULL; e = g_warn_msg_list.get_next()) {
        fprintf(stdout, "\nwarning(%d):%s",
                WARN_MSG_lineno(e), WARN_MSG_msg(e));
    }
    fprintf(stdout,"\n");
}


//Report warning with line number.
void warn(INT line_num, CHAR * msg, ...)
{
    if (msg == NULL) { return; }

    WARN_MSG * p = NULL;
    StrBuf sbuf(64);

    va_list arg;
    va_start(arg, msg);
    sbuf.vsprint(msg, arg);
    p = (WARN_MSG*)xmalloc(sizeof(WARN_MSG));
    p->msg = (CHAR*)xmalloc(sbuf.strlen() + 1);
    ::memcpy(p->msg, sbuf.buf, sbuf.strlen() + 1);
    p->lineno = line_num;
    g_warn_msg_list.append_tail(p);
    va_end(arg);
}


//Report error with line number.
void err(INT line_num, CHAR * msg, ...)
{
    if (msg == NULL) { return; }

    ERR_MSG * p = NULL;
    StrBuf sbuf(64);

    va_list arg;
    va_start(arg, msg);
    sbuf.vsprint(msg, arg);
    p = (ERR_MSG*)xmalloc(sizeof(ERR_MSG));
    p->msg = (CHAR*)xmalloc(sbuf.strlen() + 1);
    ::memcpy(p->msg, sbuf.buf, sbuf.strlen() + 1);
    p->lineno = line_num;
    g_err_msg_list.append_tail(p);
    va_end(arg);
}


INT is_too_many_err()
{
    return g_err_msg_list.get_elem_count() > TOO_MANY_ERR;
}


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
#ifndef _ERR_H_
#define _ERR_H_

//record each error msg
class WARN_MSG {
public:
    CHAR * msg;
    INT lineno;
};
#define WARN_MSG_msg(e)        (e)->msg
#define WARN_MSG_lineno(e)    (e)->lineno


//record each error msg
class ERR_MSG {
public:
    CHAR * msg;
    INT lineno;
};
#define ERR_MSG_msg(e)        (e)->msg
#define ERR_MSG_lineno(e)    (e)->lineno

#define TOO_MANY_ERR 10
#define ERR_SHOW 1
#define WARN_SHOW 2
#endif


//Exported Variables
extern List<ERR_MSG*> g_err_msg_list;
extern List<WARN_MSG*> g_warn_msg_list;

//Exported Functions
void warn1(CHAR * msg, ...);
void err(INT line_num, CHAR * msg, ...);
void show_err();
void show_warn();
INT is_too_many_err();
void interwarn(CHAR * format, ...);

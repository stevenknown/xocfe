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

static List<CELL*> g_cell_free_list;

static void * xmalloc(size_t size)
{
    ASSERT0(g_pool_general_used != NULL);
    void * p = smpoolMalloc(size, g_pool_general_used);
    if (p == NULL) return NULL;
    memset(p, 0, size);
    return p;
}


//CELL operations
//If you intend to use CELL as a container to hold something, the follows should
//be noticed:
//When you need a new CELL , invoking 'newcell()', but is not 'get_free_cell()'.
void free_cell(CELL * c)
{
    if (c == NULL) {
        return;
    }
    g_cell_free_list.append_tail(c);
    return ;
}


CELL * get_free_cell()
{
    CELL * c = g_cell_free_list.remove_tail();
    if (c) {
        memset(c, 0 , sizeof(CELL));
        return c;
    }
    return NULL;
}


CELL * newcell(INT type)
{
    CELL * c = get_free_cell();
    if (!c) {
        c = (CELL*)xmalloc(sizeof(CELL));
    }
    CELL_type(c) = type;
    return c;
}

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

static List<Cell*> g_cell_free_list;

static void * xmalloc(size_t size)
{
    ASSERT0(g_pool_general_used != nullptr);
    void * p = smpoolMalloc(size, g_pool_general_used);
    if (p == nullptr) return nullptr;
    ::memset((void*)p, 0, size);
    return p;
}


//Cell operations
//If you intend to use Cell as a container to hold something, the follows should
//be noticed:
//When you require a new Cell , invoking 'newcell()', rather than
//'get_free_cell()'.
void free_cell(Cell * c)
{
    if (c == nullptr) {
        return;
    }
    g_cell_free_list.append_tail(c);
    return ;
}


Cell * get_free_cell()
{
    Cell * c = g_cell_free_list.remove_tail();
    if (c) {
        ::memset((void*)c, 0 , sizeof(Cell));
        return c;
    }
    return nullptr;
}


void clean_free_cell_list()
{
    g_cell_free_list.destroy();
    g_cell_free_list.init();
}


Cell * newcell(INT type)
{
    Cell * c = get_free_cell();
    if (!c) {
        c = (Cell*)xmalloc(sizeof(Cell));
    }
    CELL_type(c) = type;
    return c;
}

} //namespace xfe

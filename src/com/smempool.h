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
#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

//MEM POOL utilties.
#ifndef ST_SUCC
#define ST_SUCC 0
#endif

#ifndef ST_NO_SUCH_MEMPOOL_FIND
#define ST_NO_SUCH_MEMPOOL_FIND 1
#endif

#define WORD_ALIGN 1
#define MIN_MARGIN 0

typedef size_t MEMPOOLIDX;
typedef enum {
    MEM_NONE = 0,
    MEM_COMM,     //can be realloc, free
    MEM_CONST_SIZE, //the element in the pool should be in same size.
} MEMPOOLTYPE;


#define MEMPOOL_type(p)                 ((p)->pool_type)
#define MEMPOOL_next(p)                 ((p)->next)
#define MEMPOOL_prev(p)                 ((p)->prev)
#define MEMPOOL_id(p)                   ((p)->mpt_id)
#define MEMPOOL_grow_size(p)            ((p)->grow_size)
#define MEMPOOL_start_pos(p)            ((p)->start_pos)
#define MEMPOOL_pool_size(p)            ((p)->mem_pool_size)
#define MEMPOOL_pool_ptr(p)             ((p)->ppool)
#ifdef _DEBUG_
#define MEMPOOL_chunk_id(p)             ((p)->chunk_id)
#endif

typedef struct _MemPool {
    MEMPOOLTYPE pool_type;
    struct _MemPool * next;
    struct _MemPool * prev;
    MEMPOOLIDX mpt_id; //identification of mem pool
    size_t start_pos; //represent the alloca postion of mem pool
    size_t mem_pool_size; //represent mem pool size
    size_t grow_size;
    void * ppool; //start address of mem pool

    #ifdef _DEBUG_
    ULONG chunk_id;
    #endif
} SMemPool;


#ifdef __cplusplus
extern "C" {
#endif
//create mem pool
MEMPOOLIDX smpoolCreatePoolIndex(size_t size, MEMPOOLTYPE mpt = MEM_COMM);
SMemPool * smpoolCreate(size_t size, MEMPOOLTYPE mpt = MEM_COMM);

//delete mem pool
INT smpoolDeleteViaPoolIndex(MEMPOOLIDX mpt_idx);
INT smpoolDelete(SMemPool * handle);

//alloc memory from corresponding mem pool
void * smpoolMallocViaPoolIndex(size_t size, MEMPOOLIDX mpt_idx,
                                size_t grow_size = 0);
void * smpoolMalloc(size_t size, SMemPool * handle, size_t grow_size = 0);
void * smpoolMallocConstSize(size_t elem_size, IN SMemPool * handler);

//Get whole pool size with byte
size_t smpoolGetPoolSizeViaIndex(MEMPOOLIDX mpt_idx);
size_t smpoolGetPoolSize(SMemPool const* handle);

//This function do some initializations if you want to manipulate pool
//via pool index.
//Note if you just create pool and manipulate pool via handler,
//the initialization is dispensable.
void smpoolInitPool(); //Initializing pool utilities

//This function perform finialization works if you invoke the
//smpoolInitPool().
void smpoolFiniPool(); //Finializing pool

void dumpPool(SMemPool * handler, FILE * h);
#ifdef __cplusplus
}
#endif

extern ULONGLONG g_stat_mem_size;
#endif

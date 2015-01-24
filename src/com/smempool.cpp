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
#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"

#define msg
#define MAGIC_NUM 		0xFC
#define BOUNDARY_NUM 	0xAA
#define END_BOUND_BYTE 	4


class MEMPOOL_HASH : public SHASH<SMEM_POOL*> {
public:
#ifdef _VC6_
	MEMPOOL_HASH():SHASH<SMEM_POOL*>(1024){}
#else
	MEMPOOL_HASH():SHASH<SMEM_POOL*>::SHASH(1024){}
#endif
	~MEMPOOL_HASH(){}
};


static MEMPOOL_HASH * g_mem_pool_hash_tab = NULL;
static SMEM_POOL * g_mem_pool=NULL;
static ULONG g_mem_pool_count = 0;
#ifdef _DEBUG_
static ULONG g_mem_pool_chunk_count = 0;
#endif


//First of all marker of memory pool should be initialized
static bool g_is_pool_init = false;

//Build hash table of memory pool
static bool g_is_pool_hashed = true;
ULONG g_stat_mem_size = 0;


void dump_pool(SMEM_POOL * handler, FILE * h)
{
	if (h == NULL) { return; }
	fprintf(h, "\n= SMP, total size:%u ",
			(UINT)smpool_get_pool_size_handle(handler));
	while (handler != NULL) {
		fprintf(h, "<T%u R%u>",
				(UINT)MEMPOOL_pool_size(handler),
				(UINT)(MEMPOOL_pool_size(handler) -
					   MEMPOOL_start_pos(handler)));
		handler = MEMPOOL_next(handler);
		if (handler != NULL) {
			fprintf(h, ", ");
		}
	}
	fflush(h);
}


static SMEM_POOL * new_mem_pool(ULONG size, MEMPOOLTYPE mpt)
{
	SMEM_POOL * mp = NULL;
	INT size_mp = sizeof(SMEM_POOL);
	if (size_mp % WORD_ALIGN) {
		size_mp = (sizeof(SMEM_POOL) / WORD_ALIGN + 1 ) * WORD_ALIGN;
	}
	mp = (SMEM_POOL*)malloc(size_mp + size + END_BOUND_BYTE);
	IS_TRUE(mp, ("create mem pool failed, no enough memory"));
	memset(mp, 0, size_mp);
	memset(((CHAR*)mp) + size_mp + size, BOUNDARY_NUM, END_BOUND_BYTE);


	#ifdef _DEBUG_
	g_stat_mem_size += size_mp + size; //Only for statistic purpose
	MEMPOOL_chunk_id(mp) = ++g_mem_pool_chunk_count;
	#endif
	MEMPOOL_pool_ptr(mp) = ((CHAR*)mp) + size_mp;
	MEMPOOL_pool_size(mp) = size;
	MEMPOOL_start_pos(mp) = 0;
	MEMPOOL_grow_size(mp) = size;
	return mp;
}


inline static void remove_smp(SMEM_POOL * t)
{
	if (t == NULL) return;
	IS_TRUE(t->prev != NULL, ("t should not be first."));
	t->prev->next = t->next;
	if (t->next != NULL) {
		t->next->prev = t->prev;
	}
	t->next = t->prev = NULL;
}


inline static void append_head_smp(SMEM_POOL ** head, SMEM_POOL * t)
{
	IS_TRUE0(t && head);
	t->prev = NULL;
	t->next = *head;
	*head = t;
}


inline static void append_after_smp(SMEM_POOL * marker, SMEM_POOL * tlst)
{
	IS_TRUE0(marker && tlst && marker != tlst);
	if (marker->next != NULL) {
		SMEM_POOL * last = tlst;
		while (last != NULL && last->next != NULL) {
			last = last->next;
		}
		marker->next->prev = last;
		last->next = marker->next;
	}
	tlst->prev = marker;
	marker->next = tlst;
}


/*
Hash table must be initialized if one invoked
smpool_create_idx or smpool_malloc_h.
*/
void smpool_init_pool()
{
	if (g_is_pool_init) {
		return;
	}

	if (g_is_pool_hashed) {
		g_mem_pool_hash_tab = new MEMPOOL_HASH();
		SMEM_POOL * mp = g_mem_pool;

		//Record pool list into hash.
		while (mp != NULL) {
			/*
			Mainly hash addr of 'mp' into hash
			table corresponding to 'mpt_idx'.
			*/
			IS_TRUE(g_mem_pool_hash_tab->find(MEMPOOL_id(mp)) == NULL,
					("Repetitive pool idx"));
			g_mem_pool_hash_tab->append(mp);
			mp = MEMPOOL_next(mp);
		}

		//Clean up chain info
		mp = g_mem_pool;
		while (g_mem_pool != NULL) {
			mp = g_mem_pool;
			g_mem_pool = g_mem_pool->next;
			mp->prev = mp->next = NULL;
		}
	}
	g_is_pool_init = true;
}


void smpool_fini_pool()
{
	if (g_is_pool_init && g_is_pool_hashed) {
		IS_TRUE(g_mem_pool == NULL, ("illegal init process"));
		SMEM_POOL * next = NULL;
		INT c;
		for (SMEM_POOL * mp = g_mem_pool_hash_tab->get_first(c);
			 mp != NULL; mp = next) {
			next = g_mem_pool_hash_tab->get_next(c);
			g_mem_pool_hash_tab->removed(mp);
			add_next(&g_mem_pool, mp);
		}
		//Code must be placed here! The flag must be reset
		//before the call of pool_hash_tab.destroy().
		g_is_pool_init = false;
		delete g_mem_pool_hash_tab;
		g_mem_pool_hash_tab = NULL;
	}

	g_is_pool_init = false;
	SMEM_POOL * mp = g_mem_pool;
	while (mp != NULL) {
		SMEM_POOL * tmp = mp;
		mp = MEMPOOL_next(mp);
		smpool_free_idx(MEMPOOL_id(tmp));
	}
	g_mem_pool = NULL;
	g_mem_pool_count = 0;
}


/* Create new mem pool, return the pool handle.
NOTICE:
Since this type of pool will NOT to be recorded
in 'hash table of POOLs', pool index always be 0. */
SMEM_POOL * smpool_create_handle(ULONG size, MEMPOOLTYPE mpt)
{
	SMEM_POOL * mp = NULL;
	if (size == 0 || mpt == MEM_NONE) return NULL;
	mp = new_mem_pool(MAX(sizeof(SMEM_POOL)*2, size), mpt);
	return mp;
}


//Create new mem pool, return the pool idx.
#define MAX_TRY 1024
MEMPOOLIDX smpool_create_idx(ULONG size, MEMPOOLTYPE mpt)
{
	SMEM_POOL * mp = NULL;

	if (size <=0 || mpt == MEM_NONE)
		return 0;

	if (g_is_pool_hashed && g_is_pool_init) {
		MEMPOOLIDX idx,i = 0;
		idx = (MEMPOOLIDX)rand();
		do {
			if (idx != 0 && g_mem_pool_hash_tab->find(idx) == NULL) {
 				//Unique pool idx
				break;
			}
			idx = (MEMPOOLIDX)rand();
			i++;
		} while (i < MAX_TRY);

		if (i >= MAX_TRY) {
			IS_TRUE(0, ("Not any available mempool can be created."));
			return 0;
		}
		mp = smpool_create_handle(size, mpt);
		MEMPOOL_id(mp) = idx;
		g_mem_pool_hash_tab->append(mp);
	} else {
		mp = smpool_create_handle(size, mpt);
		MEMPOOL_id(mp) = ++g_mem_pool_count;
		if (g_mem_pool == NULL) {
			g_mem_pool = mp;
		} else {
			mp->next = g_mem_pool;
			g_mem_pool->prev = mp;
			g_mem_pool = mp;
		}
	}
	return MEMPOOL_id(mp);
}


//Free mem pool totally.
INT smpool_free_handle(SMEM_POOL * handler)
{
	if (handler == NULL) {
		return ST_NO_SUCH_MEMPOOL_FIND;
	}

	//Free local pool list
	SMEM_POOL * tmp = handler;
	while (tmp != NULL) {
		SMEM_POOL * d_tmp = tmp;
		tmp = MEMPOOL_next(tmp);
		free(d_tmp);
	}
	return ST_SUCC;
}


//Free mem pool totally.
INT smpool_free_idx(MEMPOOLIDX mpt_idx)
{
	//search the mempool which indicated with 'mpt_idx'
	SMEM_POOL * mp = g_mem_pool;
	if (mpt_idx == MEM_NONE) { return ST_SUCC; }

	//Searching the mempool which indicated with 'mpt_idx'
	if (g_is_pool_hashed && g_is_pool_init) {
		mp = g_mem_pool_hash_tab->find(mpt_idx);
		if (mp == NULL) {
			/*
			Sometimes, mem pool is manipulated by user, but
			is not due to destructer.
			Therefore, the same mem pool idx will be free
			serval times. The message may confuse users.
			fprintf(stdout, "No such mempool, removing failed.");
			*/
	  		return ST_NO_SUCH_MEMPOOL_FIND;
		}
		//Remove pool from pool table.
		g_mem_pool_hash_tab->removed(mp);
	} else {
		while (mp != NULL) {
			if (MEMPOOL_id(mp) == mpt_idx) {
				break;
			}
			mp = mp->next;
		}

		if (mp == NULL) {
	  		return ST_NO_SUCH_MEMPOOL_FIND;
		}

		if (mp->prev != NULL) {
	  		mp->prev->next = mp->next;
		}
		if (mp->next != NULL) {
	  		mp->next->prev = mp->prev;
		}
		if (mp == g_mem_pool) {
	  		if (mp->next != NULL) {
	    		g_mem_pool = mp->next;
	  		} else {
	    		g_mem_pool = NULL;
	  		}
		}
	}

	//Free local pool list
	return smpool_free_handle(mp);
}


//Query memory space from pool via handler.
void * smpool_malloc_h(ULONG size, IN SMEM_POOL * handler, UINT grow_size)
{
	if (size == 0 || handler == NULL) {
		return NULL;
	}
	if (size % WORD_ALIGN) {
		size = (size / WORD_ALIGN + 1) * WORD_ALIGN;
	}

	//Search free block in the pool.
	void * addr = NULL;
	SMEM_POOL * tmp_rest = handler, * last = NULL;
	SMEM_POOL * full_head = NULL;
	while (tmp_rest != NULL) {
    	IS_TRUE(MEMPOOL_pool_size(tmp_rest) >= MEMPOOL_start_pos(tmp_rest),
				("exception occurs during mempool function"));
		IS_TRUE(MEMPOOL_pool_size(tmp_rest) > 0,
				("exception occurs during mempool function"));
		ULONG s = MEMPOOL_pool_size(tmp_rest) - MEMPOOL_start_pos(tmp_rest);
		if (size <= s) {
			addr = (ULONG*)(((BYTE*)MEMPOOL_pool_ptr(tmp_rest)) +
							 MEMPOOL_start_pos(tmp_rest));
			goto FIN;
		}

		SMEM_POOL * cur = tmp_rest;
		tmp_rest = MEMPOOL_next(tmp_rest);
		if (s <= MIN_MARGIN && cur != handler) {
			remove_smp(cur);
			append_head_smp(&full_head, cur);
		} else {
			last = cur;
		}
	}
	IS_TRUE0(last && MEMPOOL_next(last) == NULL);

	//We lack free blocks, and quering new mem block.
	tmp_rest = last;
	if (grow_size == 0) {
		IS_TRUE0(MEMPOOL_grow_size(handler));
		grow_size = MEMPOOL_grow_size(handler) * 2;
		MEMPOOL_grow_size(handler) = grow_size;
	}
	if (size > grow_size) {
		MEMPOOL_next(tmp_rest) = new_mem_pool(
							(size / grow_size + 1) * grow_size,
							MEM_COMM);
	} else {
		MEMPOOL_next(tmp_rest) = new_mem_pool(grow_size, MEM_COMM);
	}
	MEMPOOL_prev(MEMPOOL_next(tmp_rest)) = tmp_rest;
	tmp_rest = MEMPOOL_next(tmp_rest);
	addr = (ULONG*)(((CHAR*)MEMPOOL_pool_ptr(tmp_rest)) +
					MEMPOOL_start_pos(tmp_rest));
FIN:
	if (full_head != NULL) {
		append_after_smp(tmp_rest, full_head);
	}
	MEMPOOL_start_pos(tmp_rest) += size;
	IS_TRUE(MEMPOOL_pool_size(tmp_rest) >= MEMPOOL_start_pos(tmp_rest),
			("\nexception occurs in handling of pool growing\n"));
	return addr;
}


//Quering memory space from pool via pool index.
void * smpool_malloc_i(ULONG size, MEMPOOLIDX mpt_idx, UINT grow_size)
{
	SMEM_POOL * mp = g_mem_pool;
	if (size <= 0) {
		return NULL;
	}

	//Searching the mempool which indicated with 'mpt_idx'
	if (g_is_pool_hashed && g_is_pool_init) {
		mp = g_mem_pool_hash_tab->find(mpt_idx);
	} else {
		while (mp != NULL) {
			if (MEMPOOL_id(mp) == mpt_idx) {
				break;
			}
			mp = mp->next;
		}
	}

	if (mp == NULL) {
		//Mem pool of Index %lu does not exist", mpt_idx);
		return NULL;
	}

	return smpool_malloc_h(size, mp, grow_size);
}


//Get total pool byte-size.
ULONG smpool_get_pool_size_handle(SMEM_POOL const* handle)
{
	if (handle == NULL) return 0;
	SMEM_POOL const* mp = handle;
	ULONG size = 0;
	while (mp != NULL) {
		size += mp->mem_pool_size;
		mp = MEMPOOL_next(mp);
	}
	return size;
}


//Get total pool byte-size.
ULONG smpool_get_pool_size_idx(MEMPOOLIDX mpt_idx)
{
	SMEM_POOL * mp = g_mem_pool;

	if (g_is_pool_hashed && g_is_pool_init) {
		mp = g_mem_pool_hash_tab->find(mpt_idx);
	} else {
		//Searching the mempool which indicated with 'mpt_idx'
		while (mp!=NULL) {
			if(MEMPOOL_id(mp) == mpt_idx ){
				break;
			}
			mp = mp->next;
		}
	}

	if (mp == NULL) {
		return 0;
	}

	/*
	Searching free mem block in the mempool
	which 'mpt_idx' refers to.
	*/
	return smpool_get_pool_size_handle(mp);
}


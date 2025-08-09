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
#include "xcominc.h"

namespace xcom {

template <class T>
class DumpHeap : public Graph {
    class HeapNode {
    public:
        UINT id;
        T val;
    };

    SMemPool * m_pool;
    void * xmalloc(INT size)
    {
        void * p = smpoolMalloc(size, m_pool);
        if (p == nullptr) { return nullptr; }
        ::memset((void*)p, 0, size);
        return p;
    }

    HeapNode * allocHeapNode() { return (HeapNode*)xmalloc(sizeof(HeapNode)); }
    UINT parent(UINT i) { return i / 2; }
    UINT lchild(UINT i) { return i * 2; }
    UINT rchild(UINT i) { return i * 2 + 1; }
public:
    DumpHeap(Vector<T> & data)
    {
        if (data.get_last_idx() < 0) { return; }
        HeapSort<INT>::HeapValVector<T> hdata(data);
        m_pool = smpoolCreate(64, MEM_COMM);
        for (UINT i = hdata.get_begin_idx(); i <= hdata.get_end_idx(); i++) {
            UINT l = lchild(i);
            UINT r = rchild(i);
            HeapNode * hn = allocHeapNode();
            hn->id = i;
            hn->val = hdata.get(i);
            Vertex * v = addVertex(hn->id);
            VERTEX_info(v) = hn;

            if (l <= hdata.get_end_idx()) {
                HeapNode * lhn = allocHeapNode();
                lhn->id = l;
                lhn->val = hdata.get(l);
                Vertex * v = addVertex(lhn->id);
                VERTEX_info(v) = lhn;
                addEdge(hn->id, lhn->id);
            }
            if (r <= hdata.get_end_idx()) {
                HeapNode * rhn = allocHeapNode();
                rhn->id = r;
                rhn->val = hdata.get(r);
                Vertex * v = addVertex(rhn->id);
                VERTEX_info(v) = rhn;
                addEdge(hn->id, rhn->id);
            }
        }
        dumpDOT();
        smpoolDelete(m_pool);
        m_pool = nullptr;
    }

    virtual CHAR const* formatValue(OUT StrBuf & buf, HeapNode const* n) const
    {
        buf.sprint("%d", n->val);
        return buf.buf;
    }

    virtual void dumpVertex(FILE * h, Vertex const* v) const
    {
        HeapNode * n = (HeapNode*)v->info();
        ASSERT0(n);
        bool dump_vex_id = false;
        StrBuf buf(8);
        fprintf(h, "\nnode%d [shape = Mrecord, label=\"{%s",
                v->id(), formatValue(buf, n));
        if (dump_vex_id) {
            fprintf(h, "(id:%d)", v->id());
        }
        fprintf(h, "}\"];");
    }
};

} //namespace xcom

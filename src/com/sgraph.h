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
#ifndef __GRAPH_H_
#define __GRAPH_H_

namespace xcom {

#define MAGIC_METHOD

class Vertex;
class Edge;
class Graph;

#define EDGE_next(e)        (e)->next
#define EDGE_prev(e)        (e)->prev
#define EDGE_from(e)        (e)->from
#define EDGE_to(e)          (e)->to
#define EDGE_info(e)        (e)->info
class Edge {
public:
    Edge()
    {
        prev = next = NULL;
        from = to = NULL;
        info = NULL;
    }
    Edge * prev; //used by FreeList and EdgeHash
    Edge * next; //used by FreeList and EdgeHash
    Vertex * from;
    Vertex * to;
    void * info;
};


//The container of EDEG.
#define EC_next(el)        (el)->next
#define EC_prev(el)        (el)->prev
#define EC_edge(el)        (el)->edge
class EdgeC {
public:
    EdgeC()
    {
        next = prev = NULL;
        edge = NULL;
    }
    EdgeC * next;
    EdgeC * prev;
    Edge * edge;
};



#define VERTEX_next(v)         (v)->next
#define VERTEX_prev(v)         (v)->prev
#define VERTEX_id(v)           (v)->id
#define VERTEX_rpo(v)          (v)->rpo
#define VERTEX_in_list(v)      (v)->in_list
#define VERTEX_out_list(v)     (v)->out_list
#define VERTEX_info(v)         (v)->info
class Vertex {
public:
    Vertex()
    {
        prev = next = NULL;
        in_list = out_list = NULL;
        info = NULL;
        id = 0;
    }

    Vertex * prev; //used by FreeList and HASH
    Vertex * next; //used by FreeList and HASH
    EdgeC * in_list; //incoming edge list
    EdgeC * out_list;//outgoing edge list
    UINT id;
    UINT rpo;
    void * info;
};


#define MAKE_VALUE(from, to) (((from)<<16)|(to))
class EdgeHashFunc {
public:
    UINT get_hash_value(Edge * e, UINT bs) const
    {
        ASSERT0(isPowerOf2(bs));
        return hash32bit(MAKE_VALUE(VERTEX_id(EDGE_from(e)),
                                    VERTEX_id(EDGE_to(e)))) & (bs - 1);
    }

    UINT get_hash_value(OBJTY val, UINT bs) const
    { return get_hash_value((Edge*)val, bs); }

    bool compare(Edge * e1, Edge * e2) const
    {
        return (VERTEX_id(EDGE_from(e1)) == VERTEX_id(EDGE_from(e2))) &&
               (VERTEX_id(EDGE_to(e1)) == VERTEX_id(EDGE_to(e2)));
    }

    bool compare(Edge * t1, OBJTY val) const
    {
        Edge * t2 = (Edge*)val;
        return VERTEX_id(EDGE_from(t1)) == VERTEX_id(EDGE_from(t2)) &&
               VERTEX_id(EDGE_to(t1)) == VERTEX_id(EDGE_to(t2));
    }
};


class EdgeHash : public Hash<Edge*, EdgeHashFunc> {
    Graph * m_g;
public:
    EdgeHash(UINT bsize = 64) : Hash<Edge*, EdgeHashFunc>(bsize) {}
    virtual ~EdgeHash() {}

    void init(Graph * g, UINT bsize)
    {
        m_g = g;
        Hash<Edge*, EdgeHashFunc>::init(bsize);
    }

    void destroy()
    {
        m_g = NULL;
        Hash<Edge*, EdgeHashFunc>::destroy();
    }

    virtual Edge * create(OBJTY v);
};


class VertexHashFunc {
public:
    UINT get_hash_value(OBJTY val, UINT bs) const
    {
        ASSERT0(isPowerOf2(bs));
        return hash32bit((UINT)(size_t)val) & (bs - 1);
    }

    UINT get_hash_value(Vertex const* vex, UINT bs) const
    {
        ASSERT0(isPowerOf2(bs));
        return hash32bit(VERTEX_id(vex)) & (bs - 1);
    }

    bool compare(Vertex * v1, Vertex * v2) const
    { return (VERTEX_id(v1) == VERTEX_id(v2)); }

    bool compare(Vertex * v1, OBJTY val) const
    { return (VERTEX_id(v1) == (UINT)(size_t)val); }
};


class VertexHash : public Hash<Vertex*, VertexHashFunc> {
protected:
    SMemPool * m_ec_pool;
public:
    VertexHash(UINT bsize = 64) : Hash<Vertex*, VertexHashFunc>(bsize)
    {
        m_ec_pool = smpoolCreate(sizeof(Vertex) * 4, MEM_CONST_SIZE);
    }

    virtual ~VertexHash()
    { smpoolDelete(m_ec_pool); }

    virtual Vertex * create(OBJTY v)
    {
        Vertex * vex =
            (Vertex*)smpoolMallocConstSize(sizeof(Vertex), m_ec_pool);
        ASSERT0(vex);
        memset(vex, 0, sizeof(Vertex));
        VERTEX_id(vex) = (UINT)(size_t)v;
        return vex;
    }
};


/* A graph G = (V, E), consists of a set of vertices, V, and a set of edges, E.
Each edge is a pair (v,w), where v,w belong to V. Edges are sometimes
referred to as arcs. If the pair is ordered, then the graph is directed.
Directed graphs are sometimes referred to as digraphs. Vertex w is adjacent
to v if and only if (v,w) belong to E. In an undirected graph with edge (v,w),
and hence (w,v), w is adjacent to v and v is adjacent to w.
Sometimes an edge has a third component, known as either a weight or a cost.

NOTICE: for accelerating perform operation of each vertex, e.g
compute dominator, please try best to add vertex with
topological order. */
class Graph {
    friend class EdgeHash;
    friend class VertexHash;
protected:
    //it is true if the number of edges between any two
    //vertices are not more than one.
    BYTE m_is_unique:1;
    BYTE m_is_direction:1; //true if graph is direction.
    UINT m_edge_hash_size;
    UINT m_vex_hash_size;
    EdgeHash m_edges; //record all edges.
    VertexHash m_vertices; //record all vertices.
    FreeList<Edge> m_e_free_list; //record freed Edge for reuse.
    FreeList<EdgeC> m_el_free_list; //record freed EdgeC for reuse.
    FreeList<Vertex> m_v_free_list; //record freed Vertex for reuse.
    SMemPool * m_vertex_pool;
    SMemPool * m_edge_pool;
    SMemPool * m_ec_pool;

    //record vertex if vertex id is densen distribution.
    //map vertex id to vertex.
    Vector<Vertex*> * m_dense_vertex;

protected:
    //Add 'e' into out-edges of 'vex'
    inline void addOutList(Vertex * vex, Edge * e)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        if (vex == NULL || e == NULL)return;

        EdgeC * el = VERTEX_out_list(vex);
        while (el != NULL) {
            if (EC_edge(el) == e) return;
            el = EC_next(el);
        }
        el = newEdgeC(e);
        add_next(&VERTEX_out_list(vex), el);
    }

    //Add 'e' into in-edges of 'vex'
    inline void addInList(Vertex * vex, Edge * e)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        if (vex == NULL || e == NULL) return;

        EdgeC * el = VERTEX_in_list(vex);
        while (el != NULL) {
            if (EC_edge(el) == e) return;
            el = EC_next(el);
        }
        el = newEdgeC(e);
        add_next(&VERTEX_in_list(vex), el);
    }

    virtual void * cloneEdgeInfo(Edge*)
    { ASSERT(0, ("should be overloaded")); return NULL; }

    virtual void * cloneVertexInfo(Vertex*)
    { ASSERT(0, ("should be overloaded")); return NULL; }

    inline Vertex * newVertex()
    {
        Vertex * vex = (Vertex*)smpoolMallocConstSize(sizeof(Vertex),
                                                      m_vertex_pool);
        ASSERT0(vex);
        memset(vex, 0, sizeof(Vertex));
        return vex;
    }

    inline Edge * newEdge(UINT from, UINT to)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        Vertex * fp = addVertex(from);
        Vertex * tp = addVertex(to);
        return newEdge(fp, tp);
    }
    Edge * newEdge(Vertex * from, Vertex * to);
    Vertex * newVertex(UINT vid);

    inline Edge * newEdgeImpl(Vertex * from, Vertex * to)
    {
        Edge * e = m_e_free_list.get_free_elem();
        if (e == NULL) {
            e = (Edge*)smpoolMallocConstSize(sizeof(Edge), m_edge_pool);
            memset(e, 0, sizeof(Edge));
        }
        EDGE_from(e) = from;
        EDGE_to(e) = to;
        addInList(to, e);
        addOutList(from, e);
        return e;
    }

    inline EdgeC * newEdgeC(Edge * e)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        if (e == NULL) { return NULL; }

        EdgeC * el = m_el_free_list.get_free_elem();
        if (el == NULL) {
            el = (EdgeC*)smpoolMallocConstSize(sizeof(EdgeC), m_ec_pool);
            memset(el, 0, sizeof(EdgeC));
        }
        EC_edge(el) = e;
        return el;
    }
public:
    Graph(UINT edge_hash_size = 64, UINT vex_hash_size = 64);
    Graph(Graph const& g);
    Graph const& operator = (Graph const&);

    virtual ~Graph() { destroy(); }
    void init();
    void destroy();
    inline Edge * addEdge(UINT from, UINT to)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return newEdge(from, to);
    }
    inline Edge * addEdge(Vertex * from, Vertex * to)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return newEdge(from, to);
    }
    inline Vertex * addVertex(UINT vid)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_vertices.append(newVertex(vid));
    }

    void computeRpoNoRecursive(IN OUT Vertex * root,
                               OUT List<Vertex const*> & vlst);
    bool clone(Graph const& src);
    UINT count_mem() const;

    void dump_dot(CHAR const* name = NULL);
    void dump_vcg(CHAR const* name = NULL);

    //Return true if 'succ' is successor of 'v'.
    bool is_succ(Vertex * v, Vertex * succ) const
    {
        EdgeC * e = VERTEX_out_list(v);
        while (e != NULL) {
            if (EDGE_to(EC_edge(e)) == succ) {
                return true;
            }
            e = EC_next(e);
        }
        return false;
    }

    //Return true if 'pred' is predecessor of 'v'.
    bool is_pred(Vertex * v, Vertex * pred) const
    {
        EdgeC * e = VERTEX_in_list(v);
        while (e != NULL) {
            if (EDGE_from(EC_edge(e)) == pred) {
                return true;
            }
            e = EC_next(e);
        }
        return false;
    }

    bool is_equal(Graph & g) const;
    bool is_unique() const { return m_is_unique; }
    bool is_direction() const { return m_is_direction; }

    //Is there exist a path connect 'from' and 'to'.
    inline bool is_reachable(UINT from, UINT to) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return is_reachable(get_vertex(from), get_vertex(to));
    }
    bool is_reachable(Vertex * from, Vertex * to) const;
    void insertVertexBetween(
            IN Vertex * v1, IN Vertex * v2,
            IN Vertex * newv, OUT Edge ** e1 = NULL,
            OUT Edge ** e2 = NULL);
    void insertVertexBetween(
            UINT v1, UINT v2, UINT newv,
            OUT Edge ** e1 = NULL, OUT Edge ** e2 = NULL);
    bool is_graph_entry(Vertex const* v) const
    { return VERTEX_in_list(v) == NULL;    }

    //Return true if vertex is exit node of graph.
    bool is_graph_exit(Vertex const* v) const
    { return VERTEX_out_list(v) == NULL; }

    void erase();

    bool get_neighbor_list(IN OUT List<UINT> & ni_list, UINT vid) const;
    bool get_neighbor_set(OUT DefSBitSet & niset, UINT vid) const;
    inline UINT get_degree(UINT vid) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return get_degree(get_vertex(vid));
    }
    UINT get_degree(Vertex const* vex) const;
    UINT get_in_degree(Vertex const* vex) const;
    UINT get_out_degree(Vertex const* vex) const;
    inline UINT get_vertex_num() const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_vertices.get_elem_count();
    }
    inline UINT get_edge_num() const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_edges.get_elem_count();
    }
    inline Vertex * get_vertex(UINT vid) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        if (m_dense_vertex != NULL) {
            return m_dense_vertex->get(vid);
        }
        return (Vertex*)m_vertices.find((OBJTY)(size_t)vid);
    }
    Edge * get_edge(UINT from, UINT to) const;
    Edge * get_edge(Vertex const* from, Vertex const* to) const;
    inline Edge * get_first_edge(INT & cur) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_edges.get_first(cur);
    }
    inline Edge * get_next_edge(INT & cur) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_edges.get_next(cur);
    }
    inline Vertex * get_first_vertex(INT & cur) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_vertices.get_first(cur);
    }
    inline Vertex * get_next_vertex(INT & cur) const
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return m_vertices.get_next(cur);
    }

    void resize(UINT vertex_hash_sz, UINT edge_hash_sz);
    Edge * reverseEdge(Edge * e); //Reverse edge direction.(e.g: a->b => b->a)
    void reverseEdges(); //Reverse all edges.
    Edge * removeEdge(Edge * e);
    void removeEdgeBetween(Vertex * v1, Vertex * v2);
    Vertex * removeVertex(Vertex * vex);
    inline Vertex * removeVertex(UINT vid)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        return removeVertex(get_vertex(vid));
    }
    void removeTransitiveEdge();

    bool sortInToplogOrder(OUT Vector<UINT> & vex_vec, bool is_topdown);
    void set_unique(bool is_unique)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        m_is_unique = (BYTE)is_unique;
    }
    void set_direction(bool has_direction)
    {
        ASSERT(m_ec_pool != NULL, ("not yet initialized."));
        m_is_direction = (BYTE)has_direction;
    }
    void set_dense(bool is_dense)
    {
        if (is_dense) {
            if (m_dense_vertex == NULL) {
                m_dense_vertex = new Vector<Vertex*>();
            }
            return;
        }
        if (m_dense_vertex == NULL) {
            delete m_dense_vertex;
            m_dense_vertex = NULL;
        }
    }
};


//This class indicate a Dominator Tree.
class DomTree : public Graph {
public:
    DomTree()
    { set_dense(true); }
};


//
//Graph with Dominator info.
//
class DGraph : public Graph {
protected:
    Vector<BitSet*> m_dom_set; //record dominator-set of each vertex.
    Vector<BitSet*> m_pdom_set; //record post-dominator-set of each vertex.
    Vector<INT> m_idom_set; //immediate dominator.
    Vector<INT> m_ipdom_set; //immediate post dominator.
    BitSetMgr * m_bs_mgr;
    void _removeUnreachNode(UINT id, BitSet & visited);
public:
    DGraph(UINT edge_hash_size = 64, UINT vex_hash_size = 64);
    DGraph(DGraph const& g);
    DGraph const& operator = (DGraph const&);

    inline bool clone(DGraph const& g)
    {
        m_bs_mgr = g.m_bs_mgr;
        return Graph::clone(g);
    }
    bool cloneDomAndPdom(DGraph const& src);
    bool computeDom3(List<Vertex const*> const* vlst, BitSet const* uni);
    bool computeDom2(List<Vertex const*> const& vlst);
    bool computeDom(List<Vertex const*> const* vlst = NULL,
                    BitSet const* uni = NULL);
    bool computePdomByRpo(Vertex * root, BitSet const* uni);
    bool computePdom(List<Vertex const*> const* vlst);
    bool computePdom(List<Vertex const*> const* vlst, BitSet const* uni);
    bool computeIdom();
    bool computeIdom2(List<Vertex const*> const& vlst);
    bool computeIpdom();
    UINT count_mem() const;

    void dump_dom(FILE * h, bool dump_dom_tree = true);

    //idom must be positive
    //NOTE: Entry does not have idom.
    //'id': vertex id.
    inline UINT get_idom(UINT id) { return (UINT)m_idom_set.get(id); }

    //ipdom must be positive
    //NOTE: Exit does not have idom.
    //'id': vertex id.
    inline UINT get_ipdom(UINT id) { return (UINT)m_ipdom_set.get(id); }

    void get_dom_tree(OUT Graph & dom);
    void get_pdom_tree(OUT Graph & pdom);

    BitSet * get_dom_set_c(UINT id) const { return m_dom_set.get(id); }

    //Get vertices who dominate vertex 'id'.
    //NOTE: set does NOT include 'v' itself.
    inline BitSet * get_dom_set(UINT id)
    {
        ASSERT0(m_bs_mgr != NULL);
        BitSet * set = m_dom_set.get(id);
        if (set == NULL) {
            set = m_bs_mgr->create();
            m_dom_set.set(id, set);
        }
        return set;
    }

    //Get vertices who dominate vertex 'v'.
    //NOTE: set does NOT include 'v' itself.
    BitSet * get_dom_set(Vertex const* v)
    {
        ASSERT0(v != NULL);
        return get_dom_set(VERTEX_id(v));
    }

    BitSet * get_pdom_set_c(UINT id) const { return m_pdom_set.get(id); }

    //Get vertices who post dominated by vertex 'id'.
    //NOTE: set does NOT include 'v' itself.
    inline BitSet * get_pdom_set(UINT id)
    {
        ASSERT0(m_bs_mgr != NULL);
        BitSet * set = m_pdom_set.get(id);
        if (set == NULL) {
            set = m_bs_mgr->create();
            m_pdom_set.set(id, set);
        }
        return set;
    }

    //Get vertices who post dominated by vertex 'v'.
    //NOTE: set does NOT include 'v' itself.
    BitSet * get_pdom_set(Vertex const* v)
    {
        ASSERT0(v != NULL);
        return get_pdom_set(VERTEX_id(v));
    }

    //Return true if 'v1' dominate 'v2'.
    bool is_dom(UINT v1, UINT v2) const
    {
        ASSERT0(get_dom_set_c(v2));
        return get_dom_set_c(v2)->is_contain(v1);
    }

    //Return true if 'v1' post dominate 'v2'.
    bool is_pdom(UINT v1, UINT v2) const
    {
        ASSERT0(get_pdom_set_c(v2));
        return get_pdom_set_c(v2)->is_contain(v1);
    }

    void sortInBfsOrder(Vector<UINT> & order_buf, Vertex * root,
                           BitSet & visit);
    void sortDomTreeInPreorder(IN Vertex * root,
                                   OUT List<Vertex*> & lst);
    void sortDomTreeInPostrder(IN Vertex * root,
                                    OUT List<Vertex*> & lst);
    void set_bs_mgr(BitSetMgr * bs_mgr) { m_bs_mgr = bs_mgr; }
    bool removeUnreachNode(UINT entry_id);
};

} //namespace xcom
#endif

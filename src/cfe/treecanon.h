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
DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/

#ifndef __TREECANON_H__
#define __TREECANON_H__

namespace xfe {

#define TCC_change(p) ((p)->m_change)

//This class represents the context informatin during tree canonicalization.
class TreeCanonCtx {
public:
    bool m_change;

public:
    TreeCanonCtx() { clean(); }

    void clean() { TCC_change(this) = false; }

    //Unify informations which propagated bottom up
    //during processing tree.
    void unionInfoBottomUp(TreeCanonCtx const& src)
    { TCC_change(this) |= TCC_change(&src); }
};


//This class represents tree canonicalization.
class TreeCanon {
    COPY_CONSTRUCTOR(TreeCanon);

    //Return original tree if there is no change, or new tree.
    bool handleParam(Decl * formalp, Decl * realp);
    Tree * handleAssign(Tree * t, TreeCanonCtx * ctx);
    Tree * handleLda(Tree * t, TreeCanonCtx * ctx);
    Tree * handleCvt(Tree * t, TreeCanonCtx * ctx);
    Tree * handleCall(Tree * t, TreeCanonCtx * ctx);
    Tree * handleTree(Tree * t, TreeCanonCtx * ctx);
    Tree * handleId(Tree * t, TreeCanonCtx * ctx);
    Tree * handleString(Tree * t, TreeCanonCtx * ctx);
    Tree * handleAggrAccess(Tree * t, TreeCanonCtx * ctx);
    Tree * handleArray(Tree * t, TreeCanonCtx * ctx);

public:
    TreeCanon() {}
    ~TreeCanon() {}

    //Return true if there is no error occur during handling tree list.
    Tree * handleTreeList(Tree * tl, TreeCanonCtx * ctx);
};

INT TreeCanonicalize();

} //namespace xfe
#endif

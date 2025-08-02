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
#ifndef __IR_HELPER_H__
#define __IR_HELPER_H__

namespace xoc {

typedef xcom::DefSEGIter * IRSetIter;

//The class repesents a set of IR stmt.
//Note the class will free resource before destruction and no need to call
//clean().
class IRSet : public xcom::DefSBitSet {
    COPY_CONSTRUCTOR(IRSet);
public:
    IRSet(DefSegMgr * sm) : DefSBitSet(sm) {}

    void append(IR const* v) { DefSBitSet::bunion(v->id()); }
    bool allElemBeExp(Region const* rg) const;

    void dump(Region const* rg) const;

    bool find(IR const* v) const
    {
        ASSERT0(v);
        return DefSBitSet::is_contain(v->id());
    }

    //The function remove all IRs in the 'set'.
    void remove(IRSet const& set) { diff(set); }
    void remove(IR const* v)
    {
        ASSERT0(v);
        DefSBitSet::diff(v->id());
    }
};


class VisitIRFuncBase {
public:
    //Inferface that should be overrided by user.
    //The function will be invoked by current class object when visiting each
    //IR. User can set visiting status to control whether the IR tree visiting
    //keep going or terminate.
    //is_terminate: If user set it to true, the visiting will terminate
    //              immediately.
    //Return true to keep processing the kid and sibling IR on tree.
    //
    //e.g: we are going to find LDA operation in IR tree.
    //  class VisitFunction {
    //  public:
    //    IR * lda;
    //    bool visitIR(IR * ir, OUT bool & is_terminate) {
    //      if (ir->is_lda()) {
    //        lda = ir;
    //        is_terminate = true;
    //      }
    //      //Note it is OK to return true of false here, because the
    //      //visiting will terminated immedately.
    //      return true;
    //    }
    //  };
    //  class MyVisit : public VisitIRTree<VisitFunction> {
    //  };
    //  VisitFunction vf;
    //  MyVisit my(vf);
    //  my.visit(root_ir);
    bool visitIR(IR const*, OUT bool & is_terminate)
    {
        DUMMYUSE(is_terminate);
        ASSERTN(0, ("Target Dependent Code"));
        return true;
    }
    bool visitIR(IR *, OUT bool & is_terminate)
    {
        DUMMYUSE(is_terminate);
        ASSERTN(0, ("Target Dependent Code"));
        return true;
    }
};

template <class VF = VisitIRFuncBase>
class VisitIRTree {
    COPY_CONSTRUCTOR(VisitIRTree);
protected:
    //Internal variable. No user attention required.
    bool m_is_terminate;
    VF & m_vf;
protected:
    //Internal function. No user attention required.
    template<class T> void iter(T ir)
    {
        if (ir == nullptr) { return; }
        if (!m_vf.visitIR(ir, m_is_terminate)) { return; }
        if (is_terminate()) { return; }
        for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
            T tmplst = ir->getKid(i);
            if (tmplst == nullptr) { continue; }
            for (T tmp = tmplst; tmp != nullptr; tmp = tmp->get_next()) {
                if (is_terminate()) { return; }
                iter(tmp);
            }
        }
    }

    //Internal function. No user attention required.
    template<class T> void iterWithSibling(T ir)
    {
        for (T t = ir; t != nullptr; t = t->get_next()) {
            if (!m_vf.visitIR(t, m_is_terminate)) {
                if (is_terminate()) { return; }
                continue;
            }
            if (is_terminate()) { return; }
            for (UINT i = 0; i < IR_MAX_KID_NUM(t); i++) {
                T tmplst = t->getKid(i);
                if (tmplst == nullptr) { continue; }
                for (T tmp = tmplst; tmp != nullptr; tmp = tmp->get_next()) {
                    if (is_terminate()) { return; }
                    iter(tmp);
                }
            }
        }
    }

    //Internal function. No user attention required.
    bool is_terminate() const { return m_is_terminate; }
public:
    VisitIRTree(VF & vf) : m_is_terminate(false), m_vf(vf) {}
    ~VisitIRTree() {}

    //API that can be invoked by user.
    //User can invoke the function when user expect to terminate the visiting
    //immediately.
    void setTerminate() { m_is_terminate = true; }

    //API that can be invoked by user.
    //The function will iterate the IR tree that rooted by 'ir'.
    //Note the function does NOT access the sibling IR of 'ir'.
    void visit(IR * ir) { iter<IR*>(ir); }

    //API that can be invoked by user.
    //The function will iterate the IR tree that rooted by 'ir'.
    //Note the function does NOT access the sibling IR of 'ir'.
    void visit(IR const* ir) { iter<IR const*>(ir); }

    //API that can be invoked by user.
    //The function will iterate the IR tree that rooted by 'ir'.
    //Note the function will access the sibling IR of 'ir'.
    void visitWithSibling(IR * ir) { iterWithSibling<IR*>(ir); }

    //API that can be invoked by user.
    //The function will iterate the IR tree that rooted by 'ir'.
    //Note the function will access the sibling IR of 'ir'.
    void visitWithSibling(IR const* ir) { iterWithSibling<IR const*>(ir); }
};


//The function clean the IR_parent for each elements in 'irlst'.
void cleanParentForIRList(IR * irlst);

//Iterative access ir tree. This funtion initialize the iterator.
//ir: the root ir of the tree.
//it: iterator. It should be clean already.
//iter_next: true to iterate the next IR of 'ir'.
//Readonly function.
IR const* iterInitC(IR const* ir, OUT ConstIRIter & it,
                    bool iter_next = true);

//Iterative access ir tree.
//This function return the next IR node according to 'it'.
//it: iterator.
//iter_next: true to iterate the next IR of ir in current iteration.
//Readonly function.
IR const* iterNextC(MOD ConstIRIter & it, bool iter_next = true);

//Iterative access the ir tree that start with 'ir'.
//This funtion initialize the iterator.
//ir: the root ir of the tree, it may be either stmt or expression.
//it: iterator. It should be clean already.
//iter_next: true to iterate the next IR of 'ir'.
//Note this function is NOT readonly, the returnd IR may be modified.
IR * iterInit(IN IR * ir, OUT IRIter & it, bool iter_next = true);

//Iterative access the ir tree.
//This funtion return the next IR node according to 'it'.
//it: iterator.
//iter_next: true to iterate the next IR of ir in current iteration.
//Note this function is NOT readonly, the returnd IR may be modified.
IR * iterNext(MOD IRIter & it, bool iter_next = true);

//Iterative access the expression of stmt.
//This funtion initialize the iterator.
//ir: the root ir of the tree, it must be stmt.
//it: iterator. It should be clean already.
//iter_next: true to iterate the next IR of 'ir'.
//The function is a READONLY function.
//Use iterExpNextC to iter next IR.
//Note the function does not iterate inner stmt, e.g:stmts in body of IR_IF.
IR const* iterExpInitC(IR const* ir, OUT ConstIRIter & it);

//Iterative access the right-hand-side expression of stmt.
//This function return the next IR node according to 'it'.
//it: iterator.
//iter_next: true to iterate the next IR of ir in current iteration.
//Readonly function.
inline IR const* iterExpNextC(MOD ConstIRIter & it, bool iter_next = true)
{
    return iterNextC(it, iter_next);
}

//Iterative access the right-hand-side expression of stmt.
//This funtion initialize the iterator.
//ir: the root ir of the tree, it must be stmt.
//it: iterator. It should be clean already.
//iter_next: true to iterate the next IR of 'ir'.
//Use iterExpNext to iter next IR.
IR * iterExpInit(IR const* ir, OUT IRIter & it);

//Iterative access the right-hand-side expression of stmt.
//The function return the next IR node according to 'it'.
//it: iterator.
//iter_next: true to iterate the next IR of ir in current iteration.
//This is a readonly function.
inline IR * iterExpNext(MOD IRIter & it, bool iter_next = true)
{
    return iterNext(it, iter_next);
}

//Iterative access the expressions that is consist of LHS of stmt itself.
//The funtion initialize the iterator.
//ir: the stmt ir.
//it: iterator. It should be clean by caller.
//Use iterExpOfStmtNext to iter next IR.
//e.g: base expression consist of the access of IST, thus the iteration does
//not include the RHS of IST.
IR * iterExpOfStmtInit(IR * ir, OUT IRIter & it);

//Iterative access the expressions that is consist of LHS of stmt itself.
//This function return the next IR node according to 'it'.
//'it': iterator.
//Readonly function.
//e.g: base expression consist of the access of IST, thus the iteration does
//not include the RHS of IST.
inline IR * iterExpOfStmtNext(MOD IRIter & it)
{
    return iterNext(it, true);
}

bool allBeExp(IR * irlst);
bool allBeStmt(IR * irlst);

UINT getArithPrecedence(IR_CODE ty);

inline bool isCommutative(IR_CODE irt)
{ return IRDES_is_commutative(irt); }

inline bool isTernaryOp(IR_CODE irt)
{ return IRDES_is_ter(irt); }

inline bool isBinaryOp(IR_CODE irt)
{ return IRDES_is_bin(irt); }

inline bool isUnaryOp(IR_CODE irt)
{ return IRDES_is_una(irt); }

//CASE:_$L9 is non-identifier char because of '$'.
bool isContainNonIdentifierChar(CHAR const* name);

//Return the last IR in given list, and free others.
//e.g: lst is {add, sub, mul}, the function return mul, and free 'add' and
//'sub'.
//change: return true if there is IR freed.
IR * onlyLeftLast(IR * lst, Region const* rg, OUT bool & change);

//The function set parent pointer for all elements in 'ir_list'.
void setParentPointerForIRList(IR * ir_list);

//The function is used to verify given IR list sanity and uniqueness.
//irh: an IR set that is used only inside the function.
//     used to guarantee the uniquess of IR.
bool verifyIRList(IR const* ir, BitSet * irh, Region const* rg);

//The function is used to verify given IR list sanity and uniqueness.
bool verifyIRList(IR const* ir, Region const* rg);

//The function is used to verify IR sanity after IR simplified.
bool verifySimp(IR * ir, SimpCtx & simp);

//The function verifies the consistency of PR operations and related MD
//reference.
//e.g: stpr $2:u64 id:1 = ... #S1
//     ... = $2:u32 id:2 #S2
//MD system will generate two different MDs to describe the $2 with different
//size, that will confuse the solver of DUMgr when computing REACH_DEF. And
//solver will give the wrong result, that is 'stpr $2 id:1' is NOT the DEF
//of '$2:u32 id:2'.
bool verifyPROpAndMDConsistency(Region const* rg);

} //namespace xoc
#endif

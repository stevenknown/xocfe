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
#ifndef __PASS_MGR_H__
#define __PASS_MGR_H__

namespace xoc {

class AliasAnalysis;
class DUMgr;

typedef xcom::TMap<PASS_TYPE, Pass*> PassTab;
typedef xcom::TMapIter<PASS_TYPE, Pass*> PassTabIter;
typedef xcom::List<PASS_TYPE> PassTypeList;
typedef xcom::List<PASS_TYPE>::Iter PassTypeListIter;

class PassMgr {
    COPY_CONSTRUCTOR(PassMgr);
protected:
    Region * m_rg;
    RegionMgr * m_rumgr;
    TypeMgr * m_tm;

    //Record all allocated pass.
    //When PassMgr destructed, the table guarantees objects that allocated
    //by the mananger are destroyed at all.
    xcom::TTab<Pass*> m_allocated_pass;
    PassTab m_registered_pass;
protected:
    virtual Pass * allocAA();
    virtual Pass * allocArgPasser();
    virtual Pass * allocBROpt();
    virtual Pass * allocCalcDerivative();
    virtual Pass * allocCallGraph();
    virtual Pass * allocCCP();
    virtual Pass * allocCDG();
    virtual Pass * allocCFG();
    virtual Pass * allocCfsMgr();
    virtual Pass * allocCopyProp();
    virtual Pass * allocBrCondProp();
    virtual Pass * allocDCE();
    virtual Pass * allocDSE();
    virtual Pass * allocDUMgr();
    virtual Pass * allocDynamicStack();
    virtual Pass * allocExprTab();
    virtual Pass * allocExtPass(PASS_TYPE passty)
    {
        DUMMYUSE(passty);
        ASSERTN(0, ("Target Dependent Code."));
        return nullptr;
    }
    virtual Pass * allocGCSE();
    virtual Pass * allocGPAdjustment();
    virtual Pass * allocGSCC();
    virtual Pass * allocGVN();
    virtual Pass * allocStackColoring();
    virtual Pass * allocInferType();
    virtual Pass * allocInliner();
    virtual Pass * allocInsertCvt();
    virtual Pass * allocInsertVecSet();
    virtual Pass * allocInstSched();
    virtual Pass * allocInvertBrTgt();
    virtual Pass * allocIPA();
    virtual Pass * allocIRFusion();
    virtual Pass * allocIRMgr();
    virtual Pass * allocIRReloc();
    virtual Pass * allocIRSimp();
    virtual Pass * allocIVR();
    virtual Pass * allocKernelAdjustment();
    virtual Pass * allocLCSE();
    virtual Pass * allocLFTR();
    virtual Pass * allocLICM();
    virtual Pass * allocLinearScanRA();
    virtual Pass * allocLivenessMgr();
    virtual Pass * allocLoopCvt();
    virtual Pass * allocLoopDepAna();
    virtual Pass * allocPRLivenessMgr();
    virtual Pass * allocMatchAndReplace();
    virtual Pass * allocMDLivenessMgr();
    virtual Pass * allocMDSSALiveMgr();
    virtual Pass * allocMDSSAMgr();
    virtual Pass * allocRegSSAMgr();
    virtual Pass * allocMemCheck();
    virtual Pass * allocMultiResConvert();
    virtual Pass * allocPRE();
    virtual Pass * allocPrologueEpilogue();
    virtual Pass * allocPRSSAMgr();
    virtual Pass * allocRCE();
    virtual Pass * allocRefine();
    virtual Pass * allocRefineDUChain();
    virtual Pass * allocRP();
    virtual Pass * allocScalarOpt();
    virtual Pass * allocTargInfoHandler();
    virtual Pass * allocTypeReviser();
    virtual Pass * allocVectorization();
    virtual Pass * allocAlgeReasscociate();
    virtual Pass * allocVRP();

protected:
    void checkAndRecomputeDUChain(
        OptCtx * oc, DUMgr * dumgr, BitSet const& opts);
    void checkAndRecomputeAA(
        OptCtx * oc, IRCFG * cfg, AliasAnalysis *& aa, BitSet const& opts);
    void checkAndRecomputeAAandDU(
        OptCtx * oc, IRCFG * cfg, AliasAnalysis *& aa, DUMgr *& dumgr,
        BitSet const& opts);
    void checkValidAndRecomputePass(
        PASS_TYPE pt, MOD OptCtx * oc, PassTypeList const& optlist,
        IRCFG *& cfg, AliasAnalysis *& aa, DUMgr *& dumgr, BitSet const& opts);
    void checkValidAndRecomputeImpl(
        MOD OptCtx * oc, PassTypeList const& optlist, BitSet const& opts);
public:
    PassMgr(Region * rg);
    virtual ~PassMgr() { destroyAllPass(); }

    //The function allocate pass object by given pass type.
    //Note the pass object returned by this function will NOT be registered in
    //registered-pass-set.
    virtual Pass * allocPass(PASS_TYPE passty);

    //This function check validation of options in oc, perform
    //recomputation if it is invalid.
    //...: the options/passes that anticipated to recompute.
    void checkValidAndRecompute(OptCtx * oc, ...);
    void checkValidAndRecompute(OptCtx * oc, PassTypeList const& optlist);

    void destroyAllPass();

    //Destory dedicated pass.
    //Note the function only destroy the given pass object, the registered
    //pass-set still not removed.
    void destroyPass(Pass * pass);

    //The function remove the pass object from registered pass set.
    //Note the user allocate pass
    void destroyRegisteredPass(PASS_TYPE passtype);
    void dump() const;

    PassTab const& getPassTab() const { return m_registered_pass; }

    virtual Pass * registerPass(PASS_TYPE passty);

    //The function will re-construct given pass object and do registration
    //for the pass again.
    Pass * reRegisterPass(PASS_TYPE passty)
    {
        destroyRegisteredPass(passty);
        return registerPass(passty);
    }

    virtual Pass * queryPass(PASS_TYPE passty)
    { return m_registered_pass.get(passty); }

    virtual Pass * replacePass(PASS_TYPE passty, Pass * newpass);
};

} //namespace xoc
#endif

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
#ifndef __IR_H__
#define __IR_H__

//For the compilation of XOC in limited memory space device, e.g: mobile phone.
//#define LIMITED_MEMORY_SPACE true

namespace xoc {

class SimpCtx;
class IRBB;
class DU;
class MD;
class SSAInfo;
class MDSSAInfo;
class MDPhi;
class IRCFG;
class IRMgr;

//Define the condition of isomophic checking.
typedef enum tagISOMO_COND {
    ISOMO_UNDEF = 0,
    ISOMO_CK_PRNO = 0x1, //Check the PRNO for each PR operation.
    ISOMO_CK_TYPE = 0x2, //Check the Data Type for each operation.

    //Check the IR code for each operation.
    //Note the flag is set, the IR code have to exactly equal.
    //e.g. the comparison of IST and ILD will return false.
    ISOMO_CK_CODE = 0x4,
    ISOMO_CK_CONST_VAL = 0x8, //Check the constant value.
    ISOMO_CK_IDINFO = 0x10, //Check the Idinfo for each direct operation.

    //Check the prno and idinfo for memory reference operation.
    //Both the checking information must be strictly identical.
    ISOMO_CK_MEMREF_NAME = ISOMO_CK_PRNO|ISOMO_CK_IDINFO,
    ISOMO_CK_ALL = 0xFFFFffff, //Perform all kind of checking.
} ISOMO_COND;

class IsomoFlag : public UFlag {
public:
    IsomoFlag(UINT v) : UFlag(v) {}
};

class ReplaceKidCompareFunc {
public:
    bool m_is_replace_whole_ir_tree;
public:
    ReplaceKidCompareFunc(bool replace_whole_ir_tree = true)
    { m_is_replace_whole_ir_tree = replace_whole_ir_tree; }
    virtual ~ReplaceKidCompareFunc() {}

    //Return true if oldir has to be replaced by 'newir'.
    virtual bool is_replace(IR const* oldir, IR const* newir) const
    { return oldir == newir; }

    //Return true if oldir has to be replaced, and record the anticepated
    //replacer in 'newir'.
    //newir: return as a result that recorded the anticipated IR to replace.
    virtual bool is_replace(IR const* oldir, OUT IR ** newir) const
    {
        DUMMYUSE(newir && oldir);
        ASSERTN(0, ("Target Dependent Code"));
        return false;
    }

    //Return true if the replacement will scan whole IR tree, otherwise the
    //scanning will stopped right after the first replacing happened.
    virtual bool isReplaceWholeIRTree() const
    { return m_is_replace_whole_ir_tree; }
};

//The maximum integer value that can be described by bits of IR_CODE_BIT_SIZE
//should larger than IR_CODE_NUM.
#define IR_CODE_BIT_SIZE 9

//Each IR at same Region has it own unique id.
#define IR_id(ir) ((ir)->uid)

//Record IR's result data type.
#define IR_dt(ir) ((ir)->result_data_type)

//Record if ir might throw exception. If ir throw exception, destructor of
//current scope must be executed.
//e.g: ist may throw exception if p is nullptr.
//  ist:i8:4:(throw(ExceptionHandlerLabel)) = ld p, 10;
//  ...
//  label ExceptionHandlerLabel;
//  call ObjectDestructor(ld:*<4> this_pointer);
//  The control flow will jump to label ExceptionHandlerLabel when exception
//  occurred, then destructor executed.
#define IR_may_throw(ir) ((ir)->may_throw_exception)

//Indicates IR will terminate current control flow in the region.
//If this flag is true, the code that followed subsequently is unreachable.
#define IR_is_terminate(ir) ((ir)->is_terminate_control_flow)

//Record IR code.
#define IR_code(ir) ((ir)->code)

//Access parent IR.
#define IR_parent(ir) ((ir)->parent)

//Access next IR.
#define IR_next(ir) ((ir)->next)

//Access prev IR.
#define IR_prev(ir) ((ir)->prev)

//Record attached info container.
#define IR_ai(ir) ((ir)->attach_info_container)

//This flag describes concurrency semantics.
//It is true if current operation is atomic.
//IR can be atomic load, atomic write or atomic read-modify-write.
//Read barrier: such as ID/LD/ILD/PR/ARRAY may be regarded as read
//barrier if the flag is true.
//Analogously, ST/STPR/IST/STARRAY/CALL may be regarded as write barrier.
//NOTE: do NOT replace an atomic operation with a non-atomic operation.
#define IR_is_atomic(ir) ((ir)->is_atomic_op)

//This flag describes concurrency semantics.
//Set to true if current operation is atomic read-modify-write.
//For given variable, Read-Modify-Write(RMW) operation read the old value,
//then compare with new value, then write the new value to the variable,
//finally return old value.
//NOTE: The write operation may be failed.
//
//If the variable is volatile, one should not change the order of this
//operation with other memory operations.
//The flag can be used to represent safepoint in code generation, and
//if it is, the IR modified/invalided each pointers previous defined,
//and this cuts off the Def-Use chain of those pointers immediately
//after the IR.
//By default, RMW could be simulated by IR_CALL with 3 arguments,
//e.g: $x = call Opcode:i32, OldValueMemory:<valuetype>, NewValue:valuetype;
//where Opcode defined the RMW operations, and OldValueMemory indicates
//the memory location with valuetype that hold old-value, and NewValue
//is the value to be set.
#define IR_is_read_mod_write(ir) ((ir)->is_read_mod_write)

//Set to true if IR has sideeffect. This flag often be used to prevent
//the user from performing incorrect optimization.
//If IR has sideeffect, that means IR can not be removed, but it still
//can be moved.
#define IR_has_sideeffect(ir) ((ir)->has_sideeffect)

//Set to true if ir can not be moved.
//This flag often be used to prevent the user from performing incorrect
//optimization, e.g: LICM.
//If IR is immovable, it also can not be removed
#define IR_no_move(ir) ((ir)->no_move)

//Define the marco if we try to do fast searching IR in
//free_ir_tab while invoking IRMgr::allocIR(). But by default, it is disabled.
//#define CONST_IRC_SZ

#ifdef CONST_IRC_SZ
#define IR_irc_size(ir) ((ir)->irc_size)
#endif

//The class represents IR, the intermediate language for the XOC compiler,
//serves as the common interface among almost all the components.
//IR is defined to be capable of representing any level of semantics except
//the level that corresponds to the machine instructions.
//Two different levels of IR are defined, and each optimization phase is
//defined to work at a specific level of IR.
//
//The front-ends may generate the highest level of IR. Optimization proceeds
//together with the process of continuous simplification, in which a
//simplification of IR is called to translate IR from the current level to
//the next lower level.
//
//High level IR preserves the high level Control-Flow-Structure, such as
//DO_LOOP, DO_WHILE, WHILE_DO, SWITCH, IF, and Scope-Control-Operation,
//such as BREAK and CONTINUE.
//Operations can be divided into two categories: statements, and expressions.
//Statement implies which variable is defined, or control flow transfering.
//Expression implies which variable is used, or operation without sideeffect,
//and expression does not transfer control flow.
//Both statement and expression node have NEXT and PREV pointers which
//link them together.
//
//Statment can not be kid of other statement except control flow structure IR,
//and expression can be kid of both expression and statement.
//
//In a simple word, statements have side effects, and can be reordered only
//if dependencies preserved. Expressions do not have side effect, expression
//trees hung from statement, and they contain only uses and can be
//aggressively optimized.
//
//Note IR should NOT have virtual table because all operations are
//distinguished by IR_CODE. An IR object might represent differet operation
//in specific scene when it is continually freed and allocated. Diverse
//description should be placed in attach-info.
class IR {
    COPY_CONSTRUCTOR(IR);
public:
    #ifdef LIMITED_MEMORY_SPACE
    USHORT code:IR_CODE_BIT_SIZE;

    //True if IR may throw excetion.
    USHORT may_throw_exception:1;

    //True if IR is atomic operation.
    USHORT is_atomic_op:1;

    //True if IR behaved as if it is an atomic operation consist of
    //sequential read, modify, and write.
    USHORT is_read_mod_write:1;

    //True if IR may terminate the control flow, such as throwing an excetion.
    USHORT is_terminate_control_flow:1;

    //True if IR may have side effect.
    USHORT has_sideeffect:1;

    //True if IR can not be moved.
    USHORT no_move:1;
    #else
    //True if IR may throw excetion.
    bool may_throw_exception;

    //True if IR is atomic operation.
    bool is_atomic_op;

    //True if IR behaved as if it is an atomic operation consist of
    //sequential read, modify, and write.
    bool is_read_mod_write;

    //True if IR may terminate the control flow, such as throwing an excetion.
    bool is_terminate_control_flow;

    //True if IR may have side effect.
    bool has_sideeffect;

    //True if IR can not be moved.
    bool no_move;
    IR_CODE code;
    #endif

    #ifdef CONST_IRC_SZ
    //Record the specific IR byte size.
    UINT irc_size:6;
    #endif

    UINT uid; //Each IR has unique id.

    //The type of IR can be ANY that depend on the dynamic behavior of program.
    Type const* result_data_type;

    //Both of 'next' and 'prev' used by the processing of
    //complicated tree level IR construction.
    IR * next;
    IR * prev;

    //Used in all processing at all level IR.
    //This field should be nullptr if IR is the top level of stmt.
    IR * parent;

    //IR may have an unique attach-info container.
    AIContainer * attach_info_container;
public:
    IR() {}

    //Calculate the accumulated offset value from the base of array.
    //e.g: For given array long long p[10][20],
    //the offset of p[i][j] can be computed by i*20 + j, and
    //the offset of p[i] can be computed by i*20.
    //If all the indice are constant value, calcuate the value, storing
    //in 'ofst_val' and return True, otherwise return False that means the
    //Offset can not be predicated.
    bool calcArrayOffset(TMWORD * ofst, TypeMgr * tm) const;

    //Set ir's DU to be nullptr, return the DU pointer.
    DU * cleanDU();

    //Set ir's PR SSA Info to be nullptr.
    //For convenient purpose, this function does not assert
    //when current IR object is not operate on PR.
    void cleanSSAInfo();
    void cleanRefMD()
    {
        DU * du = getDU();
        if (du == nullptr) { return; }
        DU_md(du) = nullptr;
    }

    void cleanRefMDSet()
    {
        DU * du = getDU();
        if (du == nullptr) { return; }
        DU_mds(du) = nullptr;
    }

    void cleanMayRef() { cleanRefMDSet(); }
    void cleanMustRef() { cleanRefMD(); }

    void cleanRef()
    {
        DU * du = getDU();
        if (du == nullptr) { return; }
        DU_mds(du) = nullptr;
        DU_md(du) = nullptr;
    }

    //Count memory usage for current IR.
    size_t count_mem() const;

    //Copy memory reference only for current IR.
    //src: copy MD reference from 'src', it may be different to current ir.
    void copyRef(IR const* src, Region const* rg);

    //Copy AttachInfo from 'src' to current ir, not include kid and sibling.
    void copyAI(IR const* src, Region const* rg);

    //Copy data-type from 'src' to current ir, not include kid and sibling.
    void copyType(IR const* src) { setType(src->getType()); }

    //Copy each memory reference for whole ir tree.
    //src: copy MD reference from 'src', it must be equal to current ir tree.
    //copy_kid_ref: copy MD reference for kid recursively.
    void copyRefForTree(IR const* src, Region const* rg);

    //The function collects the LabelInfo for each branch-target.
    void collectLabel(OUT List<LabelInfo const*> & lst) const;

    //Return true if 'ty' can be the data type of Const-Op.
    static bool canBeTypeOfConstOp(Type const* ty);

    //Dump MD reference.
    void dumpRefOnly(Region const* rg) const;
    void dumpRef(Region const* rg, UINT indent) const;

    //Clean all DU-Chain and Defined/Used-MD reference info.
    void freeDUset(DUMgr * dumgr);

    IR_CODE getCode() const { return (IR_CODE)IR_code(this); }
    IR * get_next() const { return IR_next(this); }
    IR * get_prev() const { return IR_prev(this); }
    IR * getBase() const; //Get base expression if exist.
    TMWORD getOffset() const; //Get byte offset if any.
    IR * getOffsetOfPartialPROp() const; //Get byte offset if any.
    Var * getIdinfo() const; //Get idinfo if any.
    IR * getParent() const { return IR_parent(this); }
    IR * getKid(UINT idx) const;
    UINT getKidNum() const { return IR_MAX_KID_NUM(this); }
    IRBB * getBB() const;
    DU * getDU() const;

    //Return rest part of multip-result list if exist.
    //e.g:some IR may have multiple-results, e.g: broadcast. the result IR
    //list is stpr $1, stpr $2, stpr $3, the function will return stpr $2 and
    //stpr $3, whereas stpr $1 will be the result IR of broadcast's parent stmt.
    IR * getResList() const;

    //Return STMT if current ir is expression.
    //e.g: st(i32 a)
    //       ld(i32 b)
    //If given expression is ld, this function return st stmt.
    //Note if there are high level stmts, such as:
    //    if (det)
    //      st:i32 a
    //        ld:i32 b
    //    endif
    //This function only return the nearest stmt to ld:i32 b, namely, st:i32 a.
    inline IR * getStmt() const
    {
        ASSERT0(!is_undef());
        ASSERTN(!is_stmt(), ("IR already be stmt, it is performance bug."));
        IR const* ir = this;
        while (IR_parent(ir) != nullptr) {
            ir = IR_parent(ir);
            if (ir->is_stmt()) { break; }
        }
        ASSERTN(ir->is_stmt(), ("can not get stmt for given exp"));
        return (IR*)ir;
    }

    //Return label info if exist.
    LabelInfo const* getLabel() const;

    //Return the byte size of array element.
    UINT getArrayElemDtSize(TypeMgr const* tm) const;

    //Return byte size of ir data type.
    UINT getTypeSize(TypeMgr const* tm) const
    { return tm->getByteSize(getType()); }

    //Return bit size of ir data type.
    UINT getTypeBitSize(TypeMgr const* tm) const
    { return tm->getByteSize(getType()) * BIT_PER_BYTE; }

    DATA_TYPE getDType() const { return TY_dtype(getType()); }

    //Return data type descriptor.
    Type const* getType() const { return IR_dt(this); }

    AIContainer const* getAI() const { return IR_ai(this); }

    //Return rhs if exist. Some stmt has rhs,
    //such as IR_ST, IR_STPR and IR_IST.
    IR * getRHS() const;

    //Return the PRNO if exist.
    PRNO getPrno() const;

    //Return the storage space if exist.
    StorageSpace getStorageSpace() const;

    //Return the SSAInfo if exist.
    SSAInfo * getSSAInfo() const;

    //Return stmt if it writes PR as result. Otherwise return nullptr.
    IR * getResultPR();

    //Get the stmt according to given prno if the stmt writes PR as a result.
    //Otherwise return nullptr.
    //This function can not be const because it will return itself.
    IR * getResultPR(PRNO prno);

    //Find the first PR related to 'prno'. Otherwise return nullptr.
    //This function iterate IR tree nonrecursively.
    IR * getOpndPRList(PRNO prno) const;

    //Find the first PR related to 'prno'.
    //This function iterate IR tree nonrecursively.
    //it: iterator.
    IR * getOpndPR(PRNO prno, IRIter & it) const; //Nonrecursively.

    //This function recursively iterate the IR tree to
    //retrieve the PR whose PR_no is equal to given 'prno'.
    //Otherwise return nullptr.
    IR * getOpndPR(PRNO prno) const;

    //This function recursively iterate the IR tree to
    //retrieve the memory-ref IR whose MD is equal to given 'md'.
    //Otherwise return nullptr.
    IR * getOpndMem(MD const* md) const;

    //Get the MD DefUse Set.
    DUSet * getDUSet() const
    {
        DU * const du = getDU();
        return du == nullptr ? nullptr : DU_duset(du);
    }

    //Return the Memory Descriptor Set for given ir may describe.
    MDSet const* getMayRef() const { return getRefMDSet(); }

    //Return the MemoryAddr for 'ir' must be.
    MD const* getMustRef() const { return getRefMD(); }

    //Get the MD that IR referrenced.
    MD const* getRefMD() const
    {
        DU * du = getDU();
        return du == nullptr ? nullptr : DU_md(du);
    }

    //Get the MDSet that IR referrenced.
    MDSet const* getRefMDSet() const
    {
        DU * du = getDU();
        return du == nullptr ? nullptr : DU_mds(du);
    }

    //Return exact MD if ir defined.
    MD const* getEffectRef() const
    {
        MD const* md = getRefMD();
        return (md == nullptr || !md->is_effect()) ? nullptr : md;
    }

    //Return exact MD if ir defined.
    MD const* getExactRef() const
    {
        MD const* md = getRefMD();
        return (md == nullptr || !md->is_exact()) ? nullptr : md;
    }

    //Return determinate expression if any.
    IR * getJudgeDet() const;

    //Return the value-expression of multiple branch operations.
    IR * getValExp() const;

    //Return expression if stmt has CASE list.
    IR * getCaseList() const;

    //Return the number of alignment.
    UINT getAlign() const;

    static CHAR const* getIRName(IR const* ir)
    { return getIRCodeName(ir->getCode()); }
    static CHAR const* getIRCodeName(IR_CODE irc) { return IRCNAME(irc); }
    static UINT getIRCodeSize(IR const* ir)
    {
        #ifdef CONST_IRC_SZ
        return IR_irc_size(ir);
        #else
        return IRCSIZE(ir->getCode());
        #endif
    }

    //Return true if ir has RHS. The ir always be store operation.
    bool hasRHS() const { return IRDES_has_rhs(getCode()); }

    //Return true if ir has Multi-Result-List.
    bool hasResList() const { return IRDES_has_res_list(getCode()); }

    //This function recursively iterate the IR tree to
    //retrieve whether the IR has side effect.
    //Return true if ir carried sideeffect property.
    bool hasSideEffect(bool recur) const;

    //Return true if ir compute produce a result.
    bool hasResult() const { return IRDES_has_result(getCode()); }

    //Return true if ir has constant offset.
    bool hasOffset() const { return IRDES_has_offset(getCode()); }

    //Return true if ir has address-alignment property.
    bool hasAlign() const;

    //Return true if ir has idinfo.
    bool hasIdinfo() const { return IRDES_has_idinfo(getCode()); }

    //Return true if ir has stroage space.
    bool hasStorageSpace() const
    { return IRDES_has_storage_space(getCode()); }

    //Return true if ir has DU Info.
    bool hasDU() const { return IRDES_has_du(getCode()); }

    //Return true if stmt has judge determinate expression.
    bool hasJudgeDet() const
    { return IRDES_has_judge_target(getCode()); }

    //Return true if stmt has CASE list as kid.
    bool hasCaseList() const
    { return IRDES_has_case_list(getCode()); }

    //Return true if ir is call and does have a return value.
    bool hasReturnValue() const;

    //Return true if ir is branch-op and has multiple jump targets.
    bool hasMultiTarget() const;

    UINT id() const { return IR_id(this); }
    void invertLand(Region * rg);
    void invertLor(Region * rg);

    //The function compare the memory object that 'this' and 'ir2' accessed,
    //and return true if 'this' object is NOT overlapped with 'ir2',
    //otherwise return false.
    //ir2: stmt or expression to be compared.
    //e.g: this and ir2 are overlapped:
    //     'this' object: |--------|
    //     'ir2'  object:        |----|
    //e.g: this and ir2 are NOT overlapped:
    //     'this' object: |------|
    //     'ir2'  object:        |----|
    //
    //Note: The function will NOT consider the different pattern
    // of 'this' and ir2.
    // The function does not require RefMD information.
    // The function just determine overlapping of given two IR according to
    // their data-type and offset.
    bool isNotOverlap(IR const* ir2, Region const* rg) const;
    bool isNotOverlapByMDRef(IR const* ir2, Region const* rg) const;

    //This function recursively iterate the IR tree to
    //retrieve whether the IR is no-movable.
    bool isNoMove(bool recur) const;

    //The function compare the memory object that 'this' and 'ir2' accessed,
    //and return true if 'this' object is conver 'ir2',
    //otherwise return false.
    //ir2: stmt or expression to be compared.
    //e.g: 'this' covers ir2:
    //     'this' object: |------|
    //     'ir2'  object:   |----|
    //e.g: this is NOT cover ir2:
    //     'this' object: |------|
    //     'ir2'  object:        |----|
    //
    //Note: The function will NOT consider the different pattern
    // of 'this' and ir2.
    // The function does not require RefMD information.
    // The function just determine overlapping of given two IR according to
    // their data-type and offset.
    bool isCover(IR const* ir2, Region const* rg) const;

    //Return true if ir's data type is vector.
    bool is_vec() const { return IR_dt(this)->is_vector(); }

    //Return true if ir's data type is pointer.
    bool is_ptr() const { return IR_dt(this)->is_pointer(); }

    //Return true if ir's data type can be regarded as pointer.
    bool isPtr() const { return is_ptr() || is_any(); }

    //Return true if ir's data type can be regarded as integer.
    bool isInt() const { return is_int() || is_bool() || is_ptr(); }

    //Return true if ir's data type can be regarded as float.
    bool isFP() const { return IR_dt(this)->isFP(); }

    //Return true if ir's data type can be regarded as unsigned integer.
    bool isUInt() const { return is_uint() || is_bool() || is_ptr(); }

    //Return true if ir's data type is string.
    bool is_str() const { return IR_dt(this)->is_string(); }

    //Return true if ir's data type is memory chunk.
    bool is_mc() const { return IR_dt(this)->is_mc(); }

    bool is_any() const { return IR_dt(this)->is_any(); }

    //Return true if ir data type is signed, and the type
    //may be integer or float.
    bool is_signed() const { return IR_dt(this)->is_signed(); }

    //Return true if ir data type is unsigned, and the type
    //may be integer, string, vector.
    bool is_unsigned() const { return IR_dt(this)->is_unsigned(); }

    //Return true if the type can be regarded as signed.
    bool isSigned() const { return IR_dt(this)->isSigned(); }

    //Return true if the type can be regarded as unsigned.
    bool isUnsigned() const { return IR_dt(this)->isUnsigned(); }

    //Return true if ir data type is signed integer.
    bool is_sint() const { return IR_dt(this)->is_sint(); }

    //Return true if ir data type is unsigned integer.
    bool is_uint() const { return IR_dt(this)->is_uint(); }

    //Return true if ir data type is signed/unsigned integer.
    bool is_int() const { return IR_dt(this)->is_int(); }

    //Return true if ir data type is float.
    bool is_fp() const { return IR_dt(this)->is_fp(); }

    //Return true if ir data type is boolean.
    bool is_bool() const { return IR_dt(this)->is_bool(); }

    //Return true if ir data type is tensor.
    bool is_tensor() const { return IR_dt(this)->is_tensor(); }

    //Return true if ir data type is scalar type.
    bool is_scalar() const { return IR_dt(this)->is_scalar(); }

    //Return true if ir is label.
    bool is_lab() const { return getCode() == IR_LABEL; }

    //Return true if current ir tree is equivalent to src.
    //src: root of IR tree.
    //is_cmp_kid: it is true if comparing kids as well.
    //Note the function does not compare the siblings of 'src'.
    bool isIREqual(IR const* src, IRMgr const* mgr,
                   bool is_cmp_kid = true) const;

    //Return true if current ir tree is isomorphic to src.
    //src: root of IR tree.
    //is_cmp_kid: it is true if comparing kids as well.
    //Note the function does not compare the siblings of 'src'.
    //e.g:ist (ld base) is isomorphic to ild (ld base)
    //    ist (ld base) is NOT isomorphic to ild ($base)
    bool isIsomoTo(IR const* src, IRMgr const* mgr, bool is_cmp_kid = true,
                   IsomoFlag const& flag = IsomoFlag(ISOMO_UNDEF)) const;

    //Return true if current ir is PR and equal to src.
    bool isPREqual(IR const* src) const
    {
        ASSERT0(isWritePR() && src->isReadPR());
        return getType() == src->getType() && getPrno() == src->getPrno();
    }

    //Return true if ir-list are equivalent.
    bool isIRListEqual(IR const* irs, IRMgr const* mgr,
                       bool is_cmp_kid = true) const;

    //Return true if IR tree is exactly congruent, or
    //they are parity memory reference.
    //NOTE: the function performs the comparison from the perspective of IR
    //tree.
    bool isMemRefEqual(IR const* src, IRMgr const* mgr) const;

    //Return true if IR tree is exactly congruent, or
    //they are parity memory reference.
    //NOTE: the function performs the comparison from the perspective of
    //MustRef and MayRef.
    bool isMDRefEqual(IR const* src) const;

    //Return true if the expression or stmt references given 'mdid'.
    //The function checks both MustRef and MayRef.
    //NOTE: the function only checks the overlaping effect or delegation effect.
    //e.g: given mdid is MD_GLOBAL, if current ir's MayRef is {MD16}
    //even if MD16 is global MD, the function also return false.
    bool isRefMD(MDIdx mdid) const;

    //Return true if ir does not have any sibling.
    bool is_single() const
    { return get_next() == nullptr && get_prev() == nullptr; }

    //Return true if current ir is memory store operation.
    bool isStoreStmt() const
    { return is_st() || is_stpr() || is_ist() || is_starray(); }

    //Return true if current ir is valid type to be phi operand.
    bool isPhiOpnd() const { return is_pr() || is_lda() || isConstExp(); }

    //Return true if current ir is stmt.
    //Only statement can be chained.
    bool is_stmt() const { return IRDES_is_stmt(getCode()); }

    //Return true if current ir is expression.
    bool is_exp() const { ASSERT0(!is_undef()); return !is_stmt(); }

    //Return true if k is kid of right-hand-side of current ir.
    bool is_rhs(IR const* k) const { return !is_lhs(k) && k != this; }

    //Return true if k is the lhs of current ir.
    bool is_lhs(IR const* k) const;

    //Return true if ir terminates the control flow.
    bool is_terminate() const { return IR_is_terminate(this); }

    //Return true if ir is dummy operation.
    //The dummy operation includes both stmt and expression.
    bool isDummyOp() const { return isVirtualOp() || is_dummyuse(); }

    //Return true if ir is volatile.
    bool is_volatile() const;

    //Return true if given array has same dimension structure with current ir.
    bool isSameArrayStruct(IR const* ir) const;

    //Return true if may throw exceptions.
    bool is_may_throw() const { return IR_may_throw(this); }

    //This function recursively iterate the IR tree to
    //retrieve whether the IR is may-throw.
    //Record if ir might throw exception.
    bool isMayThrow(bool recur) const;

    //Return true if current ir is ternary operation.
    bool isTernaryOp() const { return IRDES_is_ter(getCode()); }
    static bool isTernaryOp(IR_CODE c) { return IRDES_is_ter(c); }

    //Return true if current ir is binary operation.
    bool isBinaryOp() const { return IRDES_is_bin(getCode()); }
    static bool isBinaryOp(IR_CODE c) { return IRDES_is_bin(c); }

    //Return true if current ir is unary operation.
    bool isUnaryOp() const { return IRDES_is_una(getCode()); }
    static bool isUnaryOp(IR_CODE c) { return IRDES_is_una(c); }

    //Return true if ir is constant expression.
    bool isConstExp() const;

    //Return true if ir is integer immediate.
    bool isConstInt() const { return is_const() && is_int(); }

    //Return true if ir is float-point immediate.
    bool isConstFP() const { return is_const() && is_fp(); }

    //Return true if ir is string.
    bool isConstStr() const { return is_const() && is_str(); }

    //Return true if ir is immutable expression, includes immediate, lda.
    bool isImmutExp() const;

    //Return true if ir is readonly expression or readonly call stmt.
    //If ir is expression, this function indicates that the expression does
    //not modify any memory.
    //If ir is call, this function indicates that function does not modify any
    //global memory or any memory object that passed through pointer
    //arguments.
    bool isReadOnly() const;

    //True if store to specified element of pseduo register.
    //The pseduo register must be D_MC or vector type.
    bool is_setelem() const { return getCode() == IR_SETELEM; }
    bool is_alloca() const { return getCode() == IR_ALLOCA; }
    bool is_pow() const { return getCode() == IR_POW; }
    bool is_nroot() const { return getCode() == IR_NROOT; }
    bool is_log() const { return getCode() == IR_LOG; }
    bool is_exponent() const { return getCode() == IR_EXPONENT; }
    bool is_abs() const { return getCode() == IR_ABS; }
    bool is_sin() const { return getCode() == IR_SIN; }
    bool is_cos() const { return getCode() == IR_COS; }
    bool is_tan() const { return getCode() == IR_TAN; }
    bool is_asin() const { return getCode() == IR_ASIN; }
    bool is_acos() const { return getCode() == IR_ACOS; }
    bool is_atan() const { return getCode() == IR_ATAN; }

    //Return true if picking up specified element of givne PR and store the
    //value to a new PR. The base PR must be D_MC or vector type.
    //And the result PR must be element type of base PR.
    bool is_getelem() const { return getCode() == IR_GETELEM; }
    bool is_undef() const { return getCode() == IR_UNDEF; }
    bool is_dowhile() const { return getCode() == IR_DO_WHILE; }
    bool is_whiledo() const { return getCode() == IR_WHILE_DO; }
    bool is_doloop() const { return getCode() == IR_DO_LOOP; }
    bool is_if() const { return getCode() == IR_IF; }
    bool is_label() const { return getCode() == IR_LABEL; }
    bool is_case() const { return getCode() == IR_CASE; }
    bool is_dummyuse() const { return getCode() == IR_DUMMYUSE; }
    bool is_id() const { return getCode() == IR_ID; }
    bool is_break() const { return getCode() == IR_BREAK; }
    bool is_continue() const { return getCode() == IR_CONTINUE; }
    bool is_const() const { return getCode() == IR_CONST; }
    bool is_ld() const { return getCode() == IR_LD; }
    bool is_st() const { return getCode() == IR_ST; }
    bool is_call() const { return getCode() == IR_CALL; }
    bool is_icall() const { return getCode() == IR_ICALL; }
    bool is_starray() const { return getCode() == IR_STARRAY; }
    bool is_ild() const { return getCode() == IR_ILD; }
    bool is_array() const { return getCode() == IR_ARRAY; }
    bool is_ist() const { return getCode() == IR_IST; }
    bool is_lda() const { return getCode() == IR_LDA; }
    bool is_switch() const { return getCode() == IR_SWITCH; }
    bool is_return() const { return getCode() == IR_RETURN; }
    bool is_cvt() const { return getCode() == IR_CVT; }
    bool is_truebr() const { return getCode() == IR_TRUEBR; }
    bool is_falsebr() const { return getCode() == IR_FALSEBR; }
    bool is_select() const { return getCode() == IR_SELECT; }
    bool is_phi() const { return getCode() == IR_PHI; }
    bool is_region() const { return getCode() == IR_REGION; }
    bool is_goto() const { return getCode() == IR_GOTO; }
    bool is_igoto() const { return getCode() == IR_IGOTO; }
    bool is_add() const { return getCode() == IR_ADD; }
    bool is_sub() const { return getCode() == IR_SUB; }
    bool is_mul() const { return getCode() == IR_MUL; }
    bool is_div() const { return getCode() == IR_DIV; }
    bool is_rem() const { return getCode() == IR_REM; }
    bool is_mod() const { return getCode() == IR_MOD; }
    bool is_land() const { return getCode() == IR_LAND; }
    bool is_lor() const { return getCode() == IR_LOR; }
    bool is_band() const { return getCode() == IR_BAND; }
    bool is_bor() const { return getCode() == IR_BOR; }
    bool is_xor() const { return getCode() == IR_XOR; }
    bool is_asr() const { return getCode() == IR_ASR; }
    bool is_lsr() const { return getCode() == IR_LSR; }
    bool is_lsl() const { return getCode() == IR_LSL; }
    bool is_lt() const { return getCode() == IR_LT; }
    bool is_le() const { return getCode() == IR_LE; }
    bool is_gt() const { return getCode() == IR_GT; }
    bool is_ge() const { return getCode() == IR_GE; }
    bool is_eq() const { return getCode() == IR_EQ; }
    bool is_ne() const { return getCode() == IR_NE; }
    bool is_bnot() const { return getCode() == IR_BNOT; }
    bool is_lnot() const { return getCode() == IR_LNOT; }
    bool is_neg() const { return getCode() == IR_NEG; }
    bool is_pr() const { return getCode() == IR_PR; }
    bool is_stpr() const { return getCode() == IR_STPR; }
    bool is_cfi_def_cfa() const { return getCode() == IR_CFI_DEF_CFA; }
    bool is_cfi_same_value() const { return getCode() == IR_CFI_SAME_VALUE; }
    bool is_cfi_offset() const { return getCode() == IR_CFI_OFFSET; }
    bool is_cfi_restore() const { return getCode() == IR_CFI_RESTORE; }
    bool is_cfi_def_cfa_offset() const {
        return getCode() == IR_CFI_DEF_CFA_OFFSET;
    }

    bool isDwarf() const {
        return is_cfi_def_cfa() || is_cfi_same_value() ||
               is_cfi_offset() || is_cfi_restore() ||
               is_cfi_def_cfa_offset();
    }

    //Return true if ir indicate conditional branch to a label.
    bool isConditionalBr() const { return IRDES_is_conditional_br(getCode()); }

    //Return true if current IR is a constant zero.
    //Note that the constant zero may be integer or positive float.
    bool isConstZero() const;

    //Return true if ir is operation that read or write to an array element.
    bool isArrayOp() const { return IRDES_is_array_op(getCode()); }

    //Return true if ir is base expression of array operation.
    bool isArrayBase(IR const* ir) const;

    //Return true if ir may jump to multiple targets.
    bool isMultiConditionalBr() const { return is_switch(); }

    //Return true if ir is unconditional branch.
    bool isUnconditionalBr() const
    { return IRDES_is_unconditional_br(getCode()); }

    //Return true if ir is indirect jump to multiple targets.
    bool isIndirectBr() const { return is_igoto(); }

    //Return true if ir is branch stmt.
    bool isBranch() const
    { return isConditionalBr() || isMultiConditionalBr() ||
             isUnconditionalBr() || isIndirectBr(); }

    //Return true if ir is trigonometric function.
    bool isTrig() const
    {
        return is_sin() || is_cos() || is_tan() || is_asin() ||
               is_acos() || is_atan();
    }

    //Return true if ir is indirect memory operation.
    bool isIndirectMemOp() const
    { return IRDES_is_indirect_mem_op(getCode()); }

    //Return true if ir is direct memory operation.
    bool isDirectMemOp() const { return IRDES_is_direct_mem_op(getCode()); }

    //Return true if ir is direct-call or indirect-call operation.
    bool isCallStmt() const { return is_call() || is_icall(); }

    //Return true if ir is a readonly call stmt.
    bool isCallReadOnly() const { return isCallStmt() && isReadOnly(); }

    //Return true if ir is a call and has a return value.
    bool isCallHasRetVal() const { return isCallStmt() && hasReturnValue(); }

    //Return true if ir is intrinsic operation.
    bool isIntrinsicOp() const;

    //Return true if stmt modify PR.
    //CALL/ICALL may modify PR if it has a return value.
    bool isWritePR() const { return IRDES_is_write_pr(getCode()); }

    //Return true if current stmt exactly modifies a PR.
    //CALL/ICALL may modify PR if it has a return value.
    //IR_SETELEM may modify part of PR rather than whole IR.
    //IR_GETELEM may get part of PR rather than whole IR.
    bool isWriteWholePR() const { return IRDES_is_write_whole_pr(getCode()); }

    //Return true if current stmt partailly modifies the value of PR.
    //e.g:IR_SETELEM may modify part of PR rather than whole IR.
    bool isWritePartialPR() const { return !isWriteWholePR(); }

    //Return true if current stmt may modifies a PR.
    //Note CALL/ICALL may modify PR if it has a return value.
    //IR_SETELEM and IR_GETELEM may modify part of PR rather than whole IR.
    bool isMayWritePR() const { return isWritePR() || isCallStmt(); }

    //Return true if current expression read value from PR.
    bool isReadPR() const { return is_pr(); }

    //Return true if current expression read value from part of PR.
    bool isReadPartialPR() const { return is_getelem(); }

    //Return true if current stmt/expression operates PR.
    bool isPROp() const
    { return isReadPR() || isWritePR() || isCallHasRetVal(); }

    //Return true if current stmt/expression operates part of PR.
    bool isPartialPROp() const
    { return isWritePartialPR() || isReadPartialPR(); }

    //Return true if ir might be control-flow-structure.
    bool isCFS() const
    {
        return is_switch() || is_if() || is_dowhile() || is_whiledo() ||
               is_doloop();
    }

    //Return true if ir might be control-flow-loop-structure.
    bool isCFSLoop() const
    { return is_dowhile() || is_whiledo() || is_doloop(); }

    //Return true if ir is scope-control-operation that is used to operate on
    //control-flow-structure.
    bool isScopeControlOp() const { return is_continue() || is_break(); }

    //Return true if ir is control-flow-structure and can be controlled by
    //scope-control-operation.
    bool isCFSThatControlledBySCO() const
    { return is_switch() || is_dowhile() || is_whiledo() || is_doloop(); }

    //Return true if current operation references memory.
    //These kinds of operation always define or use MD, thus include both PR
    //and NonPR operations.
    bool isMemRef() const { return IRDES_is_mem_ref(getCode()); }

    //Return true if current operation references memory, and
    //it is the rhs of stmt.
    //These kinds of operation always use MD.
    bool isMemOpnd() const { return IRDES_is_mem_opnd(getCode()); }

    //Return true if current ir is integer constant, and the number
    //is equal to 'value'.
    bool isConstIntValueEqualTo(HOST_INT value) const;

    //Return true if current ir is integer constant, and the number
    //is equal to 'value'.
    bool isConstFPValueEqualTo(HOST_FP value) const;

    //Return true if current operation references memory except the PR memory.
    bool isMemRefNonPR() const
    { return IRDES_is_non_pr_memref(getCode()); }

    //True if ir is atomic operation.
    bool is_atomic() const { return IR_is_atomic(this); }

    //True if ir is read-modify-write.
    bool is_rmw() const { return IR_is_read_mod_write(this); }

    //True if ir is judgement operation.
    bool is_judge() const { return is_relation() || is_logical(); }

    //True if ir code is judgement operation.
    static bool is_judge(IR_CODE c) { return is_relation(c) || is_logical(c); }

    //True if ir is logical operation.
    bool is_logical() const { return IRDES_is_logical(getCode()); }

    //True if ir code is logical operation.
    static bool is_logical(IR_CODE c) { return IRDES_is_logical(c); }

    //True if ir is relation operation.
    bool is_relation() const { return IRDES_is_relation(getCode()); }

    //True if ir code is relation operation.
    static bool is_relation(IR_CODE c)
    { return IRDES_is_relation(c); }

    //IR meet commutative, e.g: a+b = b+a
    bool is_commutative() const
    { return IRDES_is_commutative(getCode()); }

    //IR meet associative, e.g: (a+b)+c = a+(b+c)
    bool is_associative() const
    { return IRDES_is_associative(getCode()); }

    //Return true if current ir is leaf node at IR tree.
    //Leaf node must be expression node and it does not have any kids.
    bool is_leaf() const { return IRDES_is_leaf(getCode()); }

    //Return true if 'k' is kid of current IR tree.
    //Check if 'k' is child or grandchildren of current IR.
    //The function only compares the equality of two IR pointer
    //to determine and apply the DFS searching in whole IR tree.
    bool isKids(IR const* k) const;

    //Return true if 'k' is the immeidate kid of current IR.
    bool isImmKid(IR const* k) const;

    //Return true if array base is IR_LDA. This exactly clerifies which array
    //we are accessing. In contrast to direct array reference,
    //one can access array via computational expression, which return a pointer,
    //that record the base address of array accessing. We call this
    //indirect array accessing.
    bool isDirectArrayRef() const;

    //Return true if current ir is extended IR operation.
    bool isExtOp() const;

    //Return true if given IR_CODE is extended IR operation.
    static bool isExtOp(IR_CODE code);

    //This function invert the operation according to it semantics.
    static IR * invertIRCode(IR * ir, Region * rg);
    static IR_CODE invertIRCode(IR_CODE src);

    //Return true if current ir can be placed in BB.
    bool isStmtInBB() const;

    //Return true if current ir operates on memory object with alignged address
    //or has the attribute of aligned.
    bool isAligned() const { return getAlign() != 0 || hasAlignedAttr(); }

    //Return true if current ir has the attribute of aligned.
    bool hasAlignedAttr() const;

    //Return true if current stmt must modify 'md'.
    bool isExactDef(MD const* md) const;
    bool isExactDef(MD const* md, MDSet const* mds) const;

    //Return true if RHS of current stmt reference PR with given 'prno'.
    bool isRHSUsePR(PRNO prno) const { return getOpndPR(prno) != nullptr; }

    //Return true if IR or its kid expression is isomorphic to 'exp'.
    //e.g: if given exp is ld 'x', current ir is (add ld 'x', ld 'y')
    //the function returns true.
    bool isUseIsomoExp(IR const* exp, IRMgr const* mgr) const;

    //Return true if ir's data type must be bool.
    bool mustBeBoolType() const { return is_judge(); }

    //Return true if ir's data type must be used to represent constant.
    bool mustBeConstDataType() const;

    //Return true if ir code's data type must be bool.
    static bool mustBeBoolType(IR_CODE c) { return is_judge(c); }

    //Return true if ir's data type must be pointer.
    bool mustBePointerType() const
    {
        return is_lda() ||
               //Base of indirect operation should be pointer.
               (getParent() != 0 && getParent()->getBase() == this);
    }

    //Set prno, and update SSAInfo meanwhile.
    void setPrnoAndUpdateSSAInfo(PRNO prno);
    void setRHS(IR * rhs);
    void setPrno(PRNO prno);
    void setStorageSpace(StorageSpace ss);
    void setOffset(TMWORD ofst);
    void setIdinfo(Var * idinfo);
    void setLabel(LabelInfo const* li);
    void setBB(IRBB * bb);
    void setResList(IR * reslist);
    void setSSAInfo(SSAInfo * ssa);
    void setDU(DU * du);
    void setValExp(IR * exp);
    void setBase(IR * exp);
    void setJudgeDet(IR * exp);
    void setAlign(UINT align_bytenum);
    void setAligned(bool is_aligned);

    //Set 'kid' to be 'idx'th child of current ir.
    void setKid(UINT idx, IR * kid);

    //Set the relationship between parent and its kid.
    void setParentPointer(bool recur = true);

    //Set current ir to be parent of 'kid'.
    void setParent(IR * kid)
    {
        ASSERT0(kid);
        for (IR * k = kid; k != nullptr; k = IR_next(k)) {
            IR_parent(k) = this;
        }
    }

    //The current ir is set to pointer type.
    //Note pointer_base_size may be 0.
    inline void setPointerType(UINT pointer_base_size, TypeMgr * tm)
    {
        PointerType d;
        TY_dtype(&d) = D_PTR;
        TY_ptr_base_size(&d) = pointer_base_size;
        setType(TC_type(tm->registerPointer(&d)));
    }
    void setType(Type const* ty)
    {
        ASSERT0(ty);
        ASSERT0(!mustBePointerType() || ty->is_pointer());
        ASSERT0(!mustBeBoolType() || ty->is_bool());
        IR_dt(this) = ty;
    }
    void setRefMD(MD const* md, Region const* rg);
    void setRefMDSet(MDSet const* mds, Region const* rg);
    void setMustRef(MD const* md, Region * rg)
    {
        ASSERT0(md);
        setRefMD(md, rg);
    }

    //The function record the May-Reference MDSet of ir.
    //mds: record MayMDSet that has been hashed.
    void setMayRef(MDSet const* mds, Region const* rg)
    {
        ASSERT0(mds && !mds->is_empty());
        setRefMDSet(mds, rg);
    }

    static inline void setIRCodeSize(IR * ir, UINT ircsz)
    {
        #ifdef CONST_IRC_SZ
        IR_irc_size(ir) = irc_sz;
        #else
        DUMMYUSE((ir, ircsz));
        #endif
    }

    //Find and substitute 'newk' for 'oldk'.
    //Return true if replaced the 'oldk'.
    //recur: set to true if function recusively perform
    //replacement for 'oldk'.
    bool replaceKid(IR * oldk, IR * newk, bool recur);

    //Find and substitute 'newk_list' for 'oldk'.
    //Return true if replaced the 'oldk'.
    //recur: set to true if function recusively perform
    //replacement for 'oldk'.
    //NOTE:the function will replace 'oldk' with a list of IR.
    bool replaceKidWithIRList(IR * oldk, IR * newk_list, bool recur);

    //Find and substitute 'newk' by the strategy that defined in compare
    //function 'cmp'.
    //Return true if substitution happened.
    //newk: since newk may satisifed 'cmp' multiple times, the function will
    //      duplicate newk when replacing happened.
    //recur: set to true if function recusively perform the replacement for
    //cmp: a strategy comparison function that determined which IR should be
    //     replaced by 'newk'.
    bool replaceKid(
        IR const* newk, bool recur, ReplaceKidCompareFunc const& cmp,
        MOD Region * rg);

    //Find and substitute the related IR by the strategy that defined in compare
    //function 'cmp'.
    //Return true if substitution happened.
    //recur: set to true if function recusively perform the replacement for
    //cmp: a strategy comparison function that determined which IR should be
    //     replaced by new IR. Note the new IR is also given by 'cmp' function.
    bool replaceKid(
        bool recur, ReplaceKidCompareFunc const& cmp, MOD Region * rg);

    //Get the MD DefUse Set. This function is readonly.
    DUSet const* readDUSet() const { return getDUSet(); }

    //Iterate IR tree to remove SSA du.
    //    e.g: pr1 = ...
    //             = pr1 //S1
    //If S1 will be deleted, pr1 should be removed from its SSA_uses.
    void removeSSAUse();

    bool verify(Region const* rg) const;

    //The file includes the extended APIs and interfaces that defined by user
    //to access IR's miscellaneous attributes and operands.
    #include "ir_ext.inc"
};

class CompareIRFunc {
public:
    bool is_less(IR * t1, IR * t2) const { return t1->id() < t2->id(); }
    bool is_equ(IR * t1, IR * t2) const { return t1 == t2; }
    IR * createKey(IR * t) { return t; }
};

class CompareConstIRFunc {
public:
    bool is_less(IR const* t1, IR const* t2) const
    { return t1->id() < t2->id(); }
    bool is_equ(IR const* t1, IR const* t2) const { return t1 == t2; }
    IR const* createKey(IR const* t) { return t; }
};

} //namespace xoc

#endif

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
#ifndef _ARM_ELF_MGR_H_
#define _ARM_ELF_MGR_H_

//
//START ARMSectionInfoMgr
//
class ARMSectionInfoMgr : public elf::SectionInfoMgr {
    COPY_CONSTRUCTOR(ARMSectionInfoMgr);
public:
    ARMSectionInfoMgr() {}
    virtual ~ARMSectionInfoMgr() {}
    virtual void initExtSectionInfo(
        MOD SectionInfo * si, SECTION_TYPE sect_type) override
    {
        ASSERT0(si);
        switch (sect_type) {
        case SH_TYPE_LDM: //Use LDM initialization as an example.
            SECTINFO_bytevec(si) = allocByteVec();
            break;
        default:
            UNREACHABLE();
            break;
        }
    }
};
//END ARMSectionInfoMgr


//
//START ARMELFMgr
//
class ARMELFMgr : public elf::ELFMgr {
    COPY_CONSTRUCTOR(ARMELFMgr);
public:
    ARMELFMgr() {}
    virtual ~ARMELFMgr();
protected:
    virtual void allocTargInfo() override;

    //Allocate TECOSectionInfoMgr object.
    virtual SectionInfoMgr * allocSectionInfoMgr() override
    { return new ARMSectionInfoMgr(); }

    //Get relocation addend value of relocation type for ARM arch.
    virtual UINT getRelocAddend(elf::Word reloc_type) override;
};
//END ARMELFMgr


//
//START ARMLinkerMgr
//
class ARMLinkerMgr : public elf::LinkerMgr {
    COPY_CONSTRUCTOR(ARMLinkerMgr);
public:
    //Record spm SymbolInfo for sorted by the size later.
    xcom::List<elf::SymbolInfo*> m_spm_symbol_info;
public:
    ARMLinkerMgr() {}
    virtual ~ARMLinkerMgr() {}

    //Allocate ARMELFMgr object.
    virtual elf::ELFMgr * allocELFMgr() override;

    //Allocate output ELFMgr.
    virtual elf::ELFMgr * allocOutputELFMgr() override
    { return new ARMELFMgr(); }

    //It is entry function of relocating. After completing resolved RelocInfo,
    //merged ELFMgr and setting section address operations, the empty location
    //of relocatable RelocInfo need to be refilled by the actual address of
    //target SymbolInfo. The target address will be calculated according to
    //the different relocating type in ARM-architecture.
    //e.g.: Given a RelocInfo that has been resolved by SymbolInfo. The
    //      RELOCINFO_called_loc(RelocInfo) needs to be refilled by a actual
    //      address. And the formula of this actual address:
    //        'address = address of SymbolInfo + RELOCINFO_addend(RelocInfo)'
    //      Different formula will be chosen according to the different
    //      RELOCINFO_type(RelocInfo).
    //'elf_mgr': the output ELFMgr.
    virtual void doRelocate(MOD elf::ELFMgr * elf_mgr) override;

    //Merge ELFSHdr with no-bits type to the
    //corresponded section of output ELFMgr.
    //'elf_mgr': ELFMgr that is currently being merged.
    //'shdr': ELFSHdr with no-bits type.
    //'shdr_idx': the index of 'shdr' in ELF.
    //'shdr_subname': the name of 'shdr'.
    virtual void mergedShdrWithNoBitsType(
        MOD elf::ELFMgr * elf_mgr, elf::ELFSHdr const* shdr, UINT shdr_idx,
        Sym const* shdr_subname) override;

    //Merge ELFShdr with prog-bits type to the
    //corresponded section of output ELFMgr.
    //'shdr': ELFMgr with prog-bits type.
    //'shdr_subname': the name of 'shdr'.
    virtual void mergedShdrWithProgBitsType(
        elf::ELFSHdr const* shdr, Sym const* shdr_subname) override;

    //Merge code in 'symbol_info' into .text
    //section of output ELFMgr with ET_DYN type.
    virtual void mergeDynTypeCodeImpl(
        MOD elf::SymbolInfo * symbol_info) override;

    //Merge code in 'symbol_info' into .text
    //section of output ELFMgr with ET_REL type.
    virtual void mergeRelTypeCodeImpl(
        MOD elf::SymbolInfo * symbol_info) override;

    //Merge data in 'symbol_info' into .data section of output ELFMgr.
    virtual void mergeData(MOD elf::SymbolInfo * symbol_info) override;

    //The implement function of recording code and data
    //info into the corresponded section of output ELFMgr.
    //'elf_mgr': ELFMgr currently being operated on.
    //'shdr': ELFSHdr currently being operated on.
    //'shdr_name': the name of 'shdr'.
    //'shdr_idx': the index of 'shdr' in ELF.
    virtual void mergeELFMgrImpl(
        MOD elf::ELFMgr * elf_mgr, elf::ELFSHdr const* shdr,
        CHAR const* shdr_name, UINT shdr_idx) override;

    //There is the meaning of reloc_type of specific 'reloc_info'
    //that depended on the reloc_type of other related RelocInfo
    //in ARM-architecture. It needs to record the index of related
    //RelocInfo to the specific 'reloc_info'. The related RelocInfo
    //is next to the 'reloc_info'.
    //'index': the index of 'reloc_info' in 'm_reloc_symbol_vec'.
    virtual void processRelateRelocInfo(
        elf::RelocInfo * reloc_info, UINT index) override;

    //There is post-processing operation after ELFMgr that collected from
    //xoc::Var has been merged according to ARM-architecture.
    //e.g.: SPM variables need to be sorted before written into output ELFMgr.
    //      Thus these variables need to be collected to Vector or Map during
    //      the processing of merged ELFMgr firstly. Then their will be written
    //      into .dl_ldm section content after completing the sorting operation
    //      in this post-processing function.
    virtual void postMergeELFMgrCollectedFromVar() override;

    //Update RELOCINFO_called_loc(reloc_info) after corresponded section merged.
    //It will be updated according to the reloc type in ARM-architecture.
    virtual void updateRelaOfst(MOD elf::RelocInfo * reloc_info) override;
};
//END ARMLinkerMgr

#endif

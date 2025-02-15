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
#include "../elf/elfinc.h"
#include "../opt/cominc.h"
#include "arm_elf_targinfo.h"
#include "arm_elf_mgr.h"

//
//START ARMELFMgr
//
ARMELFMgr::~ARMELFMgr()
{
}


void ARMELFMgr::allocTargInfo()
{
    ASSERT0(m_elf_hdr.e_machine == EM_ARM || m_elf_hdr.e_machine == EM_AARCH64);
    m_ti = new ARMELFTargInfo(this);
}


UINT ARMELFMgr::getRelocAddend(elf::Word reloc_type)
{
    switch (reloc_type) {
    case R_ARM_THM_CALL:
    case R_ARM_CALL: return elf::RELOC_ADDEND_GPDISP;
    default: ASSERTN(0, ("TODO"));
    }
    return elf::RELOC_ADDEND_DEFAULT;
}
//END ARMELFMgr


//
//Start ARMLinkerMgr
//
ELFMgr * ARMLinkerMgr::allocELFMgr()
{
    ELFMgr * em = new ARMELFMgr();
    ASSERT0(em);

    //For link and generated ELF.
    m_elf_mgr_list.append_tail(em);

    //For managed ELFMgr object resources.
    m_elf_mgr_meta_list.append_tail(em);

    return em;
}


void ARMLinkerMgr::doRelocate(MOD ELFMgr * elf_mgr)
{
    ASSERT0(elf_mgr);
    ARMELFMgr * em = (ARMELFMgr*)elf_mgr;
    ASSERT0(em);

    if (g_elf_opt.isDumpLink()) {
        m_dump->prt("\n\n==== Do Relocate ==== \n\n");
    }

    for (UINT i = 0; i < m_reloc_symbol_vec.get_elem_count(); i++) {
        RelocInfo * reloc_info = m_reloc_symbol_vec[i];
        ASSERT0(reloc_info);
        switch (reloc_info->m_reloc_type) {
        case SH_TYPE_SPM:
        default:
            //TODO: Process other reloc type.
            UNREACHABLE();
            break;
        }
        //Dump info.
        if (g_elf_opt.isDumpLink()) { dumpLinkRelocate(reloc_info, i); }
    }
}


void ARMLinkerMgr::mergeDynTypeCodeImpl(MOD SymbolInfo * symbol_info)
{
    ASSERT0(symbol_info && SYMINFO_func(symbol_info));
    FunctionInfo * fi = SYMINFO_func(symbol_info);
    switch (FUNCINFO_sect_type(fi)) {
    case SH_TYPE_TEXT: //Use TEXT as an example.
    default:
        LinkerMgr::mergeDynTypeCodeImpl(symbol_info);
        break;
    }
}


void ARMLinkerMgr::mergeRelTypeCodeImpl(MOD SymbolInfo * symbol_info)
{
    ASSERT0(symbol_info && SYMINFO_func(symbol_info));

    FunctionInfo * fi = SYMINFO_func(symbol_info);
    switch (FUNCINFO_sect_type(fi)) {
    case SH_TYPE_TEXT: //Use TEXT as an example.
    default:
        LinkerMgr::mergeRelTypeCodeImpl(symbol_info);
        break;
    }
}


void ARMLinkerMgr::mergeData(MOD SymbolInfo * symbol_info)
{
    ASSERT0(symbol_info);
    switch (SYMINFO_sect_type(symbol_info)) {
    case SH_TYPE_SBSS:
        m_output_elf_mgr->mergeBssData(symbol_info);
        break;
    case SH_TYPE_SDATA:
        m_output_elf_mgr->mergeUnullData(symbol_info);
        break;
    default:
        LinkerMgr::mergeData(symbol_info);
        break;
    }
}


void ARMLinkerMgr::postMergeELFMgrCollectedFromVar()
{
}


void ARMLinkerMgr::processRelateRelocInfo(
    MOD RelocInfo * reloc_info, UINT index)
{
    ASSERT0(reloc_info);
    ASSERT0(0);
    //RELOCINFO_next(reloc_info) =
    //    (RELOCINFO_type(reloc_info) == R_SWAI_64_LITERAL) ?
    //    (ARM_GET_NEXT_RELOCINFO_INDEX(index)) : 0;
}


void ARMLinkerMgr::mergedShdrWithProgBitsType(
    ELFSHdr const* shdr, Sym const* shdr_subname)
{
    ASSERT0(shdr && shdr_subname);

    switch (m_output_elf_mgr->getSectionType(shdr_subname)) {
    case SH_TYPE_SPM:
    case SH_TYPE_SDATA:
        //'false' represents the shdr isn't BSS section.
        mergeShdrImpl(shdr, shdr_subname, false);
        break;
    default:
        LinkerMgr::mergedShdrWithProgBitsType(shdr, shdr_subname);
        break;
    }
}


void ARMLinkerMgr::mergedShdrWithNoBitsType(MOD ELFMgr * elf_mgr,
    ELFSHdr const* shdr, UINT shdr_idx, Sym const* shdr_subname)
{
    ASSERT0(shdr && shdr_subname);

    SECTION_TYPE sect_type = m_output_elf_mgr->getSectionType(shdr_subname);
    switch (sect_type) {
    case SH_TYPE_SBSS:
        //'true' represents the shdr is BSS section.
        if (shdr->s_size != 0) { mergeShdrImpl(shdr, shdr_subname, true); }
        break;
    default:
        LinkerMgr::mergedShdrWithNoBitsType(
            elf_mgr, shdr, shdr_idx, shdr_subname);
        break;
    }
}


void ARMLinkerMgr::updateRelaOfst(MOD RelocInfo * reloc_info)
{
    ASSERT0(reloc_info);
    ASSERT0(0);
    switch (RELOCINFO_type(reloc_info)) {
    case SH_TYPE_TEXT: //Use TEXT as an example.
    default:
        LinkerMgr::updateRelaOfst(reloc_info);
        break;
    }
}


void ARMLinkerMgr::mergeELFMgrImpl(MOD ELFMgr * elf_mgr,
    ELFSHdr const* shdr, CHAR const* shdr_name, UINT shdr_idx)
{
    ASSERT0(elf_mgr && shdr && shdr_name);
    switch (shdr->s_type) {
    case SH_TYPE_TEXT: //Use TEXT as an example.
    default:
        LinkerMgr::mergeELFMgrImpl(elf_mgr, shdr, shdr_name, shdr_idx);
        break;
    }
}
//END ARMLinkerMgr

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
#include "cfeinc.h"
#include "cfecommacro.h"

namespace xfe {

SMemPool * g_pool_general_used = nullptr;
SMemPool * g_pool_st_used = nullptr;
SMemPool * g_pool_tree_used = nullptr;
SymTab * g_fe_sym_tab = nullptr;
CHAR * g_real_token_string = nullptr;
TOKEN g_real_token = T_UNDEF;

Tree * buildDeref(Tree * base)
{
    //The basetype of pointer is an array. Convert a[] to (*a)[].
    Tree * deref = allocTreeNode(TR_DEREF, base->getLineno());
    TREE_lchild(deref) = base;
    Tree::setParent(deref, TREE_lchild(deref));
    return deref;
}


Tree * buildIndmem(Tree * base, Decl const* fld)
{
    Tree * t = NEWTN(TR_INDMEM);
    TREE_lineno(t) = fld->getLineno();
    TREE_token(t) = T_ARROW;
    TREE_base_region(t) = base;
    TREE_field(t) = buildId(fld);
    Tree::setParent(t, TREE_base_region(t));
    Tree::setParent(t, TREE_field(t));
    return t;
}


//Build aggregate reference tree node.
//decl: aggregate declaration.
//fldvec: record a list of indices in aggregate.
//  e.g: struct T { unsigned int b, c; struct Q {int m,n;} char e; };
//  The accessing to m's fldvec is <2, 0>, whereas e's fldvec is <3>.
Tree * buildAggrFieldRef(Decl const* decl, xcom::Vector<UINT> & fldvec)
{
    ASSERT0(decl->is_aggr());
    ASSERTN(fldvec.get_last_idx() != VEC_UNDEF, ("miss field index"));
    Tree * base = buildId(decl);
    Decl const* basedecl = decl;
    for (VecIdx i = 0; i <= fldvec.get_last_idx(); i++) {
        Decl * flddecl = nullptr;
        if (basedecl->is_aggr()) {
            get_aggr_field(basedecl->getTypeAttr(), fldvec.get(i),
                           &flddecl, nullptr);
        } else {
            ASSERTN(0, ("the function only handle aggregate reference"));
        }
        ASSERTN(flddecl, ("not find field"));

        if (basedecl->is_pointer()) {
            base = buildIndmem(base, flddecl);
        } else {
            base = buildDmem(base, flddecl);
        }

        basedecl = flddecl;
    }
    return base;
}


Tree * buildDmem(Tree * base, Decl const* fld)
{
    Tree * t = NEWTN(TR_DMEM);
    TREE_lineno(t) = fld->getLineno();
    TREE_token(t) = T_DOT;
    TREE_base_region(t) = base;
    TREE_field(t) = buildId(fld);
    Tree::setParent(t, TREE_base_region(t));
    Tree::setParent(t, TREE_field(t));
    return t;
}


Tree * buildLda(Tree * base)
{
    Tree * t = NEWTN(TR_LDA);
    TREE_token(t) = T_BITAND;
    TREE_lchild(t) = base;
    Tree::setParent(t, TREE_lchild(t));
    return t;
}


Tree * buildId(Decl const* decl)
{
    Tree * t = NEWTN(TR_ID);
    TREE_lineno(t) = decl->getLineno();
    TREE_token(t) = T_ID;
    Sym const* sym = decl->getDeclSym();
    ASSERT0(sym);
    TREE_id_name(t) = sym;
    TREE_id_decl(t) = const_cast<Decl*>(decl);
    return t;
}


Tree * buildAggrRef(Tree * base, Decl const* fld)
{
    if (fld->is_pointer()) {
        return buildIndmem(base, fld);
    }
    return buildDmem(base, fld);
}


Tree * buildUInt(HOST_UINT val)
{
    Tree * t = NEWTN(TR_IMMU);
    //If the target integer hold in 'g_real_token_string' is longer than
    //host size_t type, it will be truncated now.
    TREE_token(t) = T_IMMU;
    TREE_imm_val(t) = (HOST_INT)val;
    return t;
}


Tree * buildInitvalScope(Tree * exp_list)
{
    Tree * t = NEWTN(TR_INITVAL_SCOPE);
    TREE_initval_scope(t) = exp_list;
    TREE_lineno(t) = exp_list->getLineno();
    return t;
}


Tree * buildInt(HOST_INT val)
{
    Tree * t = NEWTN(TR_IMM);
    //If the target integer hold in 'g_real_token_string' is longer than
    //host size_t type, it will be truncated now.
    TREE_token(t) = T_IMM;
    TREE_imm_val(t) = val;
    return t;
}


Tree * buildString(Sym const* str)
{
    Tree * t = NEWTN(TR_STRING);
    TREE_token(t) = T_STRING;
    TREE_string_val(t) = str;
    return t;
}


Tree * buildAssign(Decl const* decl, Tree * rhs)
{
    Tree * id = buildId(decl);
    Tree * p = NEWTN(TR_ASSIGN);
    TREE_token(p) = T_ASSIGN;
    TREE_lchild(p) = id;
    Tree::setParent(p, TREE_lchild(p));
    TREE_rchild(p) = rhs;
    TREE_lineno(p) = rhs->getLineno();
    Tree::setParent(p, TREE_rchild(p));
    return p;
}


Tree * buildAssign(Tree * lhs, Tree * rhs)
{
    Tree * p = NEWTN(TR_ASSIGN);
    TREE_lineno(p) = rhs->getLineno();
    TREE_token(p) = T_ASSIGN;
    TREE_lchild(p) = lhs;
    Tree::setParent(p, TREE_lchild(p));
    TREE_rchild(p) = rhs;
    Tree::setParent(p, TREE_rchild(p));
    return p;
}


Tree * buildArray(Tree * base, xcom::Vector<UINT> & subexp_vec)
{
    ASSERTN(subexp_vec.get_last_idx() != VEC_UNDEF, ("miss dimension exp"));
    for (VecIdx i = 0; i <= subexp_vec.get_last_idx(); i++) {
        Tree * array = NEWTN(TR_ARRAY);
        TREE_lineno(array) = base->getLineno();
        TREE_array_base(array) = base;
        Tree::setParent(array, base);
        TREE_array_indx(array) = buildInt(subexp_vec.get(i));
        Tree::setParent(array, TREE_array_indx(array));
        base = array;
    }
    return base;
}


//Build array tree node.
//subexp_vec: record a list of subscript expressions.
//            e.g: arr[3][5], subexp_vec is <3, 5>.
Tree * buildArray(Decl const* decl, xcom::Vector<UINT> & subexp_vec)
{
    ASSERTN(subexp_vec.get_last_idx() != VEC_UNDEF, ("miss dimension exp"));
    return buildArray(buildId(decl), subexp_vec);
}


Tree * buildCvt(Tree * tgt_type, Tree * src)
{
    ASSERT0(tgt_type && tgt_type->getCode() == TR_TYPE_NAME && src);
    Tree * t = NEWTN(TR_CVT);
    TREE_cvt_type(t) = tgt_type;
    TREE_cvt_exp(t) = src;
    Tree::setParent(t, TREE_cvt_type(t));
    Tree::setParent(t, TREE_cvt_exp(t));
    return t;
}


Tree * buildCvt(Decl const* tgt_type, Tree * src)
{
    ASSERT0(tgt_type && src);
    Decl * dup = genTypeName(tgt_type);
    return buildCvt(buildTypeName(dup), src);
}


Tree * buildTypeName(Decl * decl)
{
    Tree * t = NEWTN(TR_TYPE_NAME);
    TREE_type_name(t) = decl;
    return t;
}


Tree * copyTreeList(Tree const* t)
{
    Tree * new_list = nullptr;
    while (t != nullptr) {
        Tree * newt = copyTree(t);
        xcom::add_next(&new_list, newt);
        t = TREE_nsib(t);
    }
    return new_list;
}


//Duplicate 't' and its kids, but without ir's sibiling node.
Tree * copyTree(Tree const* t)
{
    if (t == nullptr) { return nullptr; }
    Tree * newt = NEWTN(t->getCode());
    UINT id = newt->id();
    ::memcpy(newt, t, sizeof(Tree));
    TREE_id(newt) = id;
    TREE_parent(newt) = nullptr;
    TREE_psib(newt) = nullptr;
    TREE_nsib(newt) = nullptr;
    for (UINT i = 0; i < MAX_TREE_FLDS; i++) {
        Tree * kid = TREE_fld(t, i);
        if (kid == nullptr) { continue; }

        Tree * newkid_list = copyTreeList(kid);
        TREE_fld(newt, i) = newkid_list;
        for (Tree * xt = newkid_list; xt != nullptr; xt = TREE_nsib(xt)) {
            TREE_parent(xt) = newt;
        }
    }
    return newt;
}

} //namespace xfe

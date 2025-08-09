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

static INT TypeTranInitValScope(Tree * t, TYCtx * cont);

#define BUILD_TYNAME(T)  buildTypeName(buildBaseTypeSpec(T))

static TypeAttr * g_schar_type;
static TypeAttr * g_sshort_type;
static TypeAttr * g_sint_type;
static TypeAttr * g_slong_type;
static TypeAttr * g_slonglong_type;
static TypeAttr * g_uchar_type;
static TypeAttr * g_ushort_type;
static TypeAttr * g_uint_type;
static TypeAttr * g_ulong_type;
static TypeAttr * g_ulonglong_type;
static TypeAttr * g_float_type;
static TypeAttr * g_double_type;
static TypeAttr * g_void_type;
static TypeAttr * g_enum_type;

static INT process_pointer_init(Decl const* dcl, TypeAttr * ty, Tree ** init);
static INT process_struct_init(TypeAttr * ty, Tree ** init);
static INT process_union_init(TypeAttr * ty, Tree ** init);
static INT process_base_init(TypeAttr * ty, Tree ** init);
static TypeAttr * buildBaseTypeSpec(INT des);

//Go through the init tree , 'dcl' must be DCL_ARRAY
//NOTE: compute_array_dim() should be invoked.
//      dcl might be changed.
static INT process_array_init_recur(Decl * dcl, TypeAttr * ty, Tree ** init)
{
    if (dcl->is_dt_id()) {
        dcl = DECL_next(dcl);
    }
    ASSERTN(dcl->is_dt_array(), ("ONLY can be DCL_ARRAY"));
    ULONGLONG dim = DECL_array_dim(dcl);
    ULONGLONG count = 0;

    Decl * head = dcl;
    Decl * tail = nullptr;
    while (DECL_next(dcl) != nullptr && DECL_next(dcl)->is_dt_array()) {
        dcl = DECL_next(dcl);
    }
    tail = dcl;

    INT st = ST_SUCC;
    if (head != tail) {
        //multipul dimension array.
        while (*init != nullptr && st == ST_SUCC) {
            st = process_array_init_recur(DECL_next(head), ty, init);
            count++;
            if (dim > 0 && count >= dim) {
                break;
            }
        }
    } else {
        //Single dimension array.
        if ((*init)->getCode() == TR_INITVAL_SCOPE) {
            //When we meet TR_INITVAL_SCOPE, because we are
            //initializing an array, thus the initial value set begin at
            //subset of TR_INITVAL_SCOPE.
            Tree * t = TREE_initval_scope(*init);
            ty = ty->getPureTypeAttr();
            while (t != nullptr && st == ST_SUCC) {
                if (ty->is_struct()) {
                    st = process_struct_init(ty, &t);
                } else if (ty->is_union()) {
                    st = process_union_init(ty, &t);
                } else if (dcl->is_pointer()) {
                    st = process_pointer_init(const_cast<Decl*>(dcl), ty, &t);
                } else {
                    //simple type init. e.g INT/SHORT/CHAR
                    st = process_base_init(ty, &t);
                }
                count++;
                if (dim > 0 && count >= dim) {
                    break;
                }
            }
            *init = TREE_nsib(*init);
        } else {
            ty = ty->getPureTypeAttr();
            while (*init != nullptr && st == ST_SUCC) {
                if (ty->is_struct()) {
                    st = process_struct_init(ty, init);
                } else if (ty->is_union()) {
                    st = process_union_init(ty, init);
                } else if (dcl->is_pointer()) {
                    st = process_pointer_init(const_cast<Decl*>(dcl), ty, init);
                } else {
                    //simple type init. e.g INT/SHORT/CHAR
                    st = process_base_init(ty, init);
                }
                count++;
                if (dim > 0 && count >= dim) {
                    break;
                }
            }
        }
    } //end else

    if (dim == 0) {
        DECL_array_dim(head) = count;
    }
    return st;
}


//Go through the init tree , 'decl' must be DCL_DECLARATION
//NOTE: compute_array_dim() should be invoked.
static INT process_array_init(Decl * decl, Tree ** init)
{
    ASSERT0(decl->is_array());
    Decl * dcl = const_cast<Decl*>(decl->getTraitList());
    TypeAttr * ty = decl->getTypeAttr(); //get TypeSpecifier
    ASSERTN(dcl && ty,
            ("DCLARATION must have a DECRLARATOR node and TypeAttr node"));
    return process_array_init_recur(dcl, ty, init);
}


static INT process_pointer_init(Decl const* dcl, TypeAttr * ty, Tree ** init)
{
    Tree * t = TREE_initval_scope(*init);
    if ((*init)->getCode() == TR_INITVAL_SCOPE) {
        process_pointer_init(dcl, ty, &t);
        *init = TREE_nsib(*init);
        return ST_SUCC;
    }

    //TODO: type check
    //...

    *init = TREE_nsib(*init);
    return ST_SUCC;
}


static INT process_struct_init(TypeAttr * ty, Tree ** init)
{
    ASSERTN(ty->is_struct(), ("ONLY must be struct type-spec"));
    Aggr const* s = Scope::retrieveCompleteType(ty->getAggrType(), true);
    if (s == nullptr) {
        err(g_real_line_num, "uses incomplete struct %s",
            ty->getAggrType()->getTag() != nullptr ?
                ty->getAggrType()->getTag()->getStr() : "");
        return ST_ERR;
    }

    if (*init == nullptr) {
        //There is no initial value, nothing to check.
        return ST_SUCC;
    }

    if ((*init)->getCode() == TR_INITVAL_SCOPE) {
        //Strip off the initval-scope.
        Tree * fldval = TREE_initval_scope(*init);
        for (Decl * flddcl = s->getDeclList();
             flddcl != nullptr && fldval != nullptr;
             flddcl = DECL_next(flddcl)) {
            INT st = ST_SUCC;
            if ((st = process_init_by_extra_val(flddcl, &fldval)) != ST_SUCC) {
                return st;
            }
        }
    } else if ((*init)->isRHS()) {
        ;
    } else {
        err(g_real_line_num, "unmatch initial value type to struct %s",
            s->getTag() != nullptr ? s->getTag()->getStr() : "");
    }

    *init = TREE_nsib(*init);
    return ST_SUCC;
}


static INT process_union_init(TypeAttr * ty, Tree ** init)
{
    ASSERTN(ty->is_union(), ("ONLY must be union type-spec"));
    Aggr * s = TYPE_aggr_type(ty);
    if (!s->is_complete()) {
        err(g_real_line_num, "uses incomplete union %s",
            s->getTag()->getStr());
        return ST_ERR;
    }
    if (*init == nullptr) {
        //There is no initial value, nothing to check.
        return ST_SUCC;
    }
    Tree * fldval = nullptr;
    if ((*init)->getCode() == TR_INITVAL_SCOPE) {
        //Strip off the initval-scope.
        fldval = TREE_initval_scope(*init);
    } else {
        //*init is already initial-value which has stripped SCOPE token.
        fldval = *init;
    }
    for (Decl * flddcl = s->getDeclList();
         flddcl != nullptr && fldval != nullptr; flddcl = DECL_next(flddcl)) {
        INT st = ST_SUCC;
        if ((st = process_init_by_extra_val(flddcl, &fldval)) != ST_SUCC) {
            return st;
        }
    }
    if ((*init)->getCode() == TR_INITVAL_SCOPE) {
        //Update *init pointer to subsequent token that may be value or scope.
        *init = TREE_nsib(*init);
    }
    return ST_SUCC;
}


//C base type
static INT process_base_init(TypeAttr * ty, Tree ** init)
{
    Tree * t = TREE_initval_scope(*init);
    if ((*init)->getCode() == TR_INITVAL_SCOPE) {
        process_base_init(ty, &t);
        *init = TREE_nsib(*init);
        return ST_SUCC;
    }
    //TODO: type check
    *init = TREE_nsib(*init);
    return ST_SUCC;
}


//Initialization verification.
//The function:
//  1. computes the exactly array index for lowest dimension.
//  2. checks compatibility between the type of initial-value and
//     type-specifer.
//decl can ONLY be DCL_DECLARATION.
INT process_init_of_declaration(Decl * decl)
{
    if (decl == nullptr) { return ST_SUCC; }
    ASSERTN(decl->is_dt_declaration(), ("ONLY can be DCRLARATION"));
    if (!decl->is_initialized()) { return ST_SUCC; }

    Decl * dcl = DECL_decl_list(decl); //get DCL_DECLARATOR
    TypeAttr * ty = decl->getTypeAttr(); //get TypeSpecifier
    ASSERTN(dcl && ty,
            ("DCLARATION must have a DECRLARATOR node and TypeAttr node"));
    ASSERTN(dcl->is_dt_declarator(), ("ONLY can be DECLARATOR"));
    ASSERT0(DECL_is_init(dcl));
    ASSERT0(DECL_init_tree(dcl));

    Tree * initval = decl->getDeclInitTree();
    if (initval == nullptr) {
        err(g_real_line_num, "initializing expression is illegal");
        return ST_ERR;
    }
    INT st = ST_SUCC;
    if (dcl->is_pointer()) {
        //First, determine whether 'dcl' is aggregate or array type.
        //e.g: basic type, pointer type.
        st = process_pointer_init(dcl->getTraitList(), ty, &initval);
    } else if (dcl->is_array()) {
        st = process_array_init(decl, &initval);
    } else if (ty->is_struct()) {
        st = process_struct_init(ty, &initval);
    } else if (ty->is_union()) {
        st = process_union_init(ty, &initval);
    } else {
        //simple type init. e.g INT SHORT
        st = process_base_init(ty, &initval);
    }

    if (initval != nullptr) {
        ASSERT0(decl->getDeclSym());
        err(g_real_line_num,
            "there are too many initializers than var '%s' declared",
            decl->getDeclSym()->getStr());
        st = ST_ERR;
    }
    return st;
}


//'decl' does not have its own initializing form tree,
//therefore 'init' will be recorded as the initialization tree.
INT process_init_by_extra_val(Decl * decl, Tree ** init)
{
    if (decl == nullptr) { return ST_SUCC; }
    ASSERTN(decl->is_dt_declaration(), ("ONLY can be DCRLARATION"));

    Decl * dcl = DECL_decl_list(decl); //get DCRLARATOR
    TypeAttr * ty = decl->getTypeAttr(); //get TypeSpecifier
    ASSERTN((dcl && ty),
            ("DCLARATION must have a DCRLARATOR node and TypeAttr node"));
    ASSERTN((*init) != nullptr,
            ("'init' initialization tree cannot be nullptr"));

    INT st = ST_SUCC;
    if (dcl->is_array()) {
        st = process_array_init(decl, init);
    } else if (ty->is_struct()) {
        st = process_struct_init(ty, init);
    } else if (ty->is_union()) {
        st = process_union_init(ty, init);
    } else if (dcl->is_pointer()) {
        st = process_pointer_init(dcl, ty, init);
    } else {
        //simple type init. e.g INT SHORT
        st = process_base_init(ty,init);
    }

    return st;
}


#ifdef _DEBUG_
//Check 'dcl' is DCL_TYPE_NAME.
//type name often be used in type-cast expression, such as
static bool is_valid_type_name(Decl const* dcl)
{
    ASSERT0(dcl && DECL_decl_list(dcl));
    if (!(dcl->is_dt_typename() &&
          DECL_decl_list(dcl)->is_dt_abs_declarator())) {
        return false;
    }

    dcl = dcl->getTraitList();

    //type name does not include ID.
    if (dcl == nullptr || dcl->is_dt_array() ||
        dcl->is_dt_pointer() || dcl->is_dt_fun()) {
        return true;
    }
    return false;
}
#endif


//Constructing TypeAttr-NAME declaration
static Decl * buildTypeName(TypeAttr * ty)
{
    ASSERT0(ty);
    Decl * decl = newDecl(DCL_TYPE_NAME);
    DECL_decl_list(decl) = newDecl(DCL_ABS_DECLARATOR);
    DECL_spec(decl) = ty;
    return decl;
}


//Only construct simply base type-spec
static TypeAttr * buildBaseTypeSpec(INT des)
{
    if (!TypeAttr::isSimpleType(des)) {
        ASSERTN(0,("expect base type"));
        return nullptr;
    }

    if (IS_TYPE(des, T_SPEC_SIGNED)) {
        if (IS_TYPE(des, T_SPEC_CHAR)) {
            return g_schar_type;
        } else if (IS_TYPE(des, T_SPEC_SHORT)) {
            return g_sshort_type;
        } else if (IS_TYPE(des, T_SPEC_INT)) {
            return g_sint_type;
        } else if (IS_TYPE(des, T_SPEC_LONG)) {
            return g_slong_type;
        } else if (IS_TYPE(des, T_SPEC_LONGLONG)) {
            return g_slonglong_type;
        } else if (IS_TYPE(des, T_SPEC_FLOAT)) {
            return g_float_type;
        } else if (IS_TYPE(des, T_SPEC_DOUBLE)) {
            return g_double_type;
        } else if (IS_TYPE(des, T_SPEC_VOID)) {
            return g_sint_type;
        }
    } else if (IS_TYPE(des, T_SPEC_UNSIGNED)) {
        if (IS_TYPE(des, T_SPEC_CHAR)) {
            return g_uchar_type;
        } else if (IS_TYPE(des, T_SPEC_SHORT)) {
            return g_ushort_type;
        } else if (IS_TYPE(des, T_SPEC_INT)) {
            return g_uint_type;
        } else if (IS_TYPE(des, T_SPEC_LONG)) {
            return g_ulong_type;
        } else if (IS_TYPE(des, T_SPEC_LONGLONG)) {
            return g_ulonglong_type;
        } else if (IS_TYPE(des, T_SPEC_FLOAT)) {
            return g_float_type;
        } else if (IS_TYPE(des, T_SPEC_DOUBLE)) {
            return g_double_type;
        } else if (IS_TYPE(des, T_SPEC_VOID)) {
            return g_uint_type;
        }
    } else {
        if (IS_TYPE(des, T_SPEC_CHAR)) {
            return g_schar_type;
        } else if (IS_TYPE(des, T_SPEC_SHORT)) {
            return g_sshort_type;
        } else if (IS_TYPE(des, T_SPEC_INT)) {
            return g_sint_type;
        } else if (IS_TYPE(des, T_SPEC_LONG)) {
            return g_slong_type;
        } else if (IS_TYPE(des, T_SPEC_LONGLONG)) {
            return g_slonglong_type;
        } else if (IS_TYPE(des, T_SPEC_FLOAT)) {
            return g_float_type;
        } else if (IS_TYPE(des, T_SPEC_DOUBLE)) {
            return g_double_type;
        } else if (IS_TYPE(des, T_SPEC_VOID)) {
            return g_void_type;
        } else if (IS_TYPE(des, T_SPEC_ENUM)) {
            return g_enum_type;
        }
    }
    ASSERTN(0,("TODO"));
    return nullptr;
}


//Conversion rank. Accroding C99 binary operation converting rules.
//l                r                       standard C convert
//double           any                     double
//float            any                     float
//unsigned         unsigned                upper rank unsigned
//signed           signed                  upper rank signed
//unsigned         lower rank signed       unsigned
//unsigned         upper rank signed       upper rank signed
//any              any                     no-convert
static UINT getCvtRank(UINT des)
{
    if (HAVE_FLAG(des, T_SPEC_LONGLONG) || HAVE_FLAG(des, T_SPEC_DOUBLE)) {
        if (HAVE_FLAG(des, T_SPEC_DOUBLE)) {
            return 90;
        }

        //long long
        //long long int
        //signed/unsiged long long int
        return 89;
    }

    if (HAVE_FLAG(des, T_SPEC_LONG) ||
        HAVE_FLAG(des, T_SPEC_DOUBLE) ||
        HAVE_FLAG(des, T_SPEC_FLOAT)) {
        if (HAVE_FLAG(des, T_SPEC_DOUBLE) ||
            HAVE_FLAG(des, T_SPEC_FLOAT)) {
            return 88;
        }

        //long
        //long int
        //signed/unsiged long int
        return 87;
    }

    if (HAVE_FLAG(des, T_SPEC_SHORT)) {
        //short
        //short int
        //signed/unsiged short
        //signed/unsiged short int
        return 84;
    }

    if (HAVE_FLAG(des, T_SPEC_INT) ||
        HAVE_FLAG(des, T_SPEC_ENUM) ||
        ONLY_HAVE_FLAG(des, T_SPEC_SIGNED) ||
        ONLY_HAVE_FLAG(des, T_SPEC_UNSIGNED)) {
        //signed/unsigned
        //int
        //signed/unsiged int
        return 85;
    }

    if (HAVE_FLAG(des, T_SPEC_CHAR) || HAVE_FLAG(des, T_SPEC_BOOL)) {
        //char
        //signed/unsiged char
        return 83;
    }

    UNREACHABLE();
    return 0;
}


//Accroding C99 integer/float conversion rank to build binary operation type.
//l                r                       standard C convert
//double           any                     double
//float            any                     float
//unsigned         unsigned                upper rank unsigned
//signed           signed                  upper rank signed
//unsigned         lower rank signed       unsigned
//unsigned         upper rank signed       upper rank signed
//any              any                     no-convert
static Decl * buildBinaryOpType(TREE_CODE tok, Decl * l, Decl * r)
{
    TypeAttr * lty = l->getTypeAttr();
    TypeAttr * rty = r->getTypeAttr();
    UINT bankl = getCvtRank(TYPE_des(lty));
    UINT bankr = getCvtRank(TYPE_des(rty));
    if (bankl > bankr || tok == TR_SHIFT) {
        return l;
    }
    if (bankl == bankr) {
        if (lty->is_unsigned()) {
            return l;
        }
    }
    return r;
}


static Decl * buildPointerType(TypeAttr * ty)
{
    ASSERT0(ty);
    Decl * newdecl = buildTypeName(ty);
    ASSERT0(DECL_decl_list(newdecl) &&
            DECL_decl_list(newdecl)->is_dt_abs_declarator());
    DECL_trait(newdecl) = newDecl(DCL_POINTER);
    return newdecl;
}


//Checking type-convert of modifier
static bool checkAssign(Tree const* t, Decl * ld, Decl *)
{
    ASSERT0(t && ld);
    xcom::DefFixedStrBuf buf;
    if (ld->is_array()) {
        format_declaration(buf, ld, true);
        err(t->getLineno(), "illegal '%s', left operand must be l-value",
            buf.getBuf());
        return false;
    }

    if (ld->getTypeAttr()->is_const()) {
        format_declaration(buf, ld, true);
        err(t->getLineno(),
            "illegal '%s', l-value specifies const object", buf.getBuf());
        return false;
    }

    //TODO: we should check struct/union compatibility in 'ld' and 'rd'
    //Here look lchild of '=' as default type
    return true;
}


//Check parameters type and insert CVT if necessary.
//arg: real arguments of function call.
//param_decl: the declaration of formal parameter of function call.
static Tree * insertCvtForParam(Tree * arg, Decl const* param_decl)
{
    Decl * arg_decl = TREE_result_type(arg);
    ASSERTN(arg_decl, ("miss result-type"));
    if (arg_decl->is_double() && param_decl->is_float()) {
        //Insert convertion operation: truncate double to float.
        arg = buildCvt(param_decl, arg);
        TypeTranList(arg, nullptr);
        return arg;
    }
    if (arg_decl->is_integer() && param_decl->is_integer()) {
        if (arg_decl->getDeclByteSize() == param_decl->getDeclByteSize()) {
            //The arguments will be expended safely to the size of parameter.
            return arg;
        }
        //Insert convertion operation: truncate bigger simple type to
        //smaller one.
        //e.g:The function declaration is: void foo(unsigned int p);
        //  The call-site is:
        //    unsigned long long lx;
        //    foo(lx);
        //  after insertion:
        //    unsigned long long lx;
        //    foo((unsigned int)lx);
        //e.g2:The function declaration is:
        //   void foo(unsigned long long p, unsigned long long q);
        //  The call-site is:
        //    unsigned int x,y;
        //    foo(x, y);
        //  after insertion:
        //    unsigned int x,y;
        //    foo((unsigned long long)x, (unsigned long long)y);
        arg = buildCvt(param_decl, arg);
        TypeTranList(arg, nullptr);
        return arg;
    }
    //Should be verified during type-checking.
    return arg;
}


//Check parameters type and insert CVT if necessary.
static void insertCvtForParams(Tree * t)
{
    ASSERT0(t && t->getCode() == TR_CALL);
    Decl * funcdecl = TREE_result_type(TREE_fun_exp(t));
    ASSERTN(funcdecl->is_dt_typename(), ("expect Type-NAME"));
    ASSERT0(DECL_decl_list(funcdecl));
    ASSERTN(DECL_decl_list(funcdecl)->is_dt_abs_declarator(),
            ("expect abs-declarator"));
    Decl const* formalp_decl = get_parameter_list(funcdecl);
    Tree * newparamlist = nullptr;
    Tree * last = nullptr;
    for (Tree * realp = xcom::removehead(&TREE_para_list(t));
         realp != nullptr && formalp_decl != nullptr;
         realp = xcom::removehead(&TREE_para_list(t))) {
        if (formalp_decl->is_dt_var())  {
            xcom::add_next(&newparamlist, &last, realp);
            continue;
        }
        realp = insertCvtForParam(realp, formalp_decl);
        xcom::add_next(&newparamlist, &last, realp);
        formalp_decl = DECL_next(formalp_decl);
    }
    TREE_para_list(t) = newparamlist;
}


static bool findAndRefillAggrField(Decl const* base, Sym const* field_name,
                                   OUT Decl ** field_decl, INT lineno)
{
    ASSERT0(base->is_dt_declaration() ||
            base->is_dt_typename());
    TypeAttr * base_spec = base->getTypeAttr();
    Aggr * s = TYPE_aggr_type(base_spec);

    //Search for matched field.
    Decl * field_list = s->getDeclList();
    if (field_list == nullptr) {
        //Declaration of base type may be defined afterward to
        //reference point, find the complete declaration and backfilling.
        Aggr * findone = nullptr;
        if (!s->is_complete() && s->getTag() != nullptr &&
            isAggrExistInOuterScope(s->getScope(), s->getTag(), true,
                                    base_spec, &findone)) {
            //Try to find complete aggr declaration in outer scope.
            ASSERT0(findone && findone->is_complete());
            TYPE_aggr_type(base_spec) = findone;
            field_list = findone->getDeclList();
        }

        if (field_list == nullptr) {
            //Not find field.
            xcom::DefFixedStrBuf buf;
            format_aggr_complete(buf, base_spec);
            err(lineno,
                " '%s' is an empty %s, '%s' is not its field",
                buf.getBuf(), base_spec->getAggrTypeName(),
                SYM_name(field_name));
            return false;
        }
    }

    while (field_list != nullptr) {
        Sym const* sym = field_list->getDeclSym();
        if (sym == field_name) {
            *field_decl = field_list;
            break;
        }
        if (field_list->is_anony_aggr()) {
            if (findAndRefillAggrField(field_list, field_name,
                                       field_decl, lineno)) {
                break;
            }
        }
        field_list = DECL_next(field_list);
    }

    if (*field_decl == nullptr) { return false; }

    if ((*field_decl)->is_aggr() &&
        !(*field_decl)->getTypeAttr()->isAggrComplete()) {
        //Try to find complete aggr declaration for field.
        Aggr * s2 = (*field_decl)->getTypeAttr()->getAggrType();
        Aggr * findone2 = nullptr;
        if (AGGR_tag(s2) != nullptr &&
            isAggrExistInOuterScope(AGGR_scope(s2), AGGR_tag(s2), true,
                                    (*field_decl)->getTypeAttr(), &findone2)) {
            //Update and refill current field's type-specifier.
            TYPE_aggr_type((*field_decl)->getTypeAttr()) = findone2;
        }
    }
    return true;
}


//Return true if t is valid field of given struct/union, otherwise false.
static INT TypeTranIDField(Tree * t, Decl ** field_decl, TYCtx * cont)
{
    ASSERT0(t && t->getCode() == TR_ID);
    //At present, the ID may be a field of struct/union,
    //and you need to check out if it is the correct field.
    //Get the aggregate type base.
    Decl * base = TREE_result_type(cont->base_tree_node);
    ASSERTN(base, ("miss base tree node of aggregate"));
    if (!findAndRefillAggrField(base, TREE_id_name(t), field_decl,
                                t->getLineno())) {
        xcom::DefFixedStrBuf buf;
        format_aggr_complete(buf, base->getTypeAttr());
        err(t->getLineno(), " '%s' : is not a member of type '%s'",
            TREE_id_name(t)->getStr(), buf.getBuf());
        return ST_ERR;
    }
    ASSERT0(*field_decl);
    TREE_id_decl(t) = *field_decl;
    return ST_SUCC;
}


//Return sub-dimension type if 'decl' is multi-dimensional array or multi-level
//pointer.
//e.g: given ID->ARRAY[10]->ARRAY[20], return ID->ARRAY[10].
static void reduceDimForArrayOrPointer(Decl * decl)
{
    ASSERT0(decl);
    ASSERT0(decl->is_dt_declaration() || decl->is_dt_typename());
    Decl * dcl = DECL_trait(decl); //dcl will changed.
    if (dcl->is_dt_id()) {
        dcl = DECL_next(dcl);
    }
    if (dcl->is_dt_array() || dcl->is_dt_pointer()) {
        xcom::remove(&DECL_trait(decl), dcl);
    }
}


//Reduce the number of DCL_POINTER of function-pointer, and remain only
//one.
//e.g: In C, you can define function pointer like: int (*******fun)();
//     We will simply it to be 'int (*fun)()'.
static Decl * refineFuncPtr(Decl * dcl_list)
{
    Decl * tmp = dcl_list;
    while (tmp != nullptr) {
        if (tmp->is_dt_pointer()) {
            tmp = DECL_next(tmp);
        } else {
            if (tmp->is_dt_fun()) {
                break;
            } else {
                tmp = dcl_list;
                break;
            }
        }
    }

    if (tmp != nullptr && tmp->is_dt_fun() &&
        DECL_prev(tmp) != nullptr && DECL_prev(tmp)->is_dt_pointer()) {
        //CASE: tmp is PTR->FUN.
        //Change traits of result-type of 't' to be function pointer.
        tmp = DECL_prev(tmp);
        DECL_prev(tmp) = nullptr;
    }
    return tmp != nullptr ? tmp : dcl_list;
}


static INT TypeTranID(Tree * t, TYCtx * cont)
{
    ASSERT0(t && t->getCode() == TR_ID);
    //Construct type-name and expand user type if it was declared.
    Decl * id_decl = nullptr;
    Tree * parent = t->parent();
    if (parent != nullptr && cont->is_field &&
        (parent->getCode() == TR_DMEM || parent->getCode() == TR_INDMEM)) {
        if (ST_SUCC != TypeTranIDField(t, &id_decl, cont)) {
            return ST_ERR;
        }
    } else {
        id_decl = TREE_id_decl(t);
    }
    ASSERT0(id_decl);

    if (id_decl->is_user_type_ref()) {
        //ID has been defined with typedef type.
        //we must expand the combined type here.
        id_decl = expandUserType(id_decl);
    }

    //Construct TYPE_NAME for ID, that would
    //be used to infer decl-type of tree node.
    Decl * res_ty = buildTypeName(id_decl->getTypeAttr());
    TREE_result_type(t) = res_ty;
    Decl * declarator = id_decl->getPureDeclaratorList();
    if (DECL_is_bit_field(declarator)) {
        //Set bit info if idenifier is bitfield.
        //Bitfield info is stored at declarator list.
        DECL_is_bit_field(DECL_decl_list(res_ty)) = true;
        DECL_bit_len(DECL_decl_list(res_ty)) = DECL_bit_len(declarator);

        //Check bitfield properties.
        if (id_decl->is_pointer()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, id_decl, true);
            err(t->getLineno(),
                "'%s' : pointer cannot assign bit length", buf.getBuf());
            return ST_ERR;
        }

        if (id_decl->is_array()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, id_decl, true);
            err(t->getLineno(),
                "'%s' : array type cannot assign bit length", buf.getBuf());
            return ST_ERR;
        }

        if (!id_decl->is_integer()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, id_decl, true);
            err(t->getLineno(), "'%s' : bit field must have integer type",
                buf.getBuf());
            return ST_ERR;
        }

        //Check bitfield's base type is big enough to hold it.
        UINT size = id_decl->getDeclByteSize() * BIT_PER_BYTE;
        if (size < (UINT)DECL_bit_len(declarator)) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, id_decl, true);
            err(t->getLineno(),
                "'%s' : type of bit field too small for number of bits",
                buf.getBuf());
            return ST_ERR;
        }
    }

    Decl * traits = const_cast<Decl*>(id_decl->getTraitList());
    ASSERTN(traits->is_dt_id(),
            ("'id' should be declarator-list-head. Illegal declaration"));

    //Because we are building Type-Name, neglect the first DCL_ID node,
    //we only need the rest decl-type to build Abs-Declarator.
    traits = dupDeclBeginAt(DECL_next(traits));

    //Reduce the number of DCL_POINTER of function-pointer, and remain only
    //one.
    //e.g: In C, you can define function pointer like: int (*******fun)();
    //     We will simply it to be 'int (*fun)()'.
    //Update traits of result-type of 't' if it has been changed.
    DECL_trait(res_ty) = refineFuncPtr(traits);

    //The result-type of 't' has been genereted.
    return ST_SUCC;
}


//The function handles dereference of pointer.
static INT TypeTranDeref(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) {
        return ST_ERR;
    }

    Decl * ld = TREE_result_type(t->lchild());
    if (!ld->is_pointer() && !ld->is_array()) {
        err(t->getLineno(), "Illegal dereferencing operation, "
            "indirection operation should operate on pointer type.");
        return ST_ERR;
    }

    Decl * td = dupTypeName(ld);
    ld = DECL_trait(td); //ld may changed.
    ASSERTN(ld, ("left child must be pointer type"));
    if (ld->is_dt_pointer() || ld->is_dt_array()) {
        //In C, base of array only needs address, so the DEREF
        //operator has alias effect. It means ARRAY(LD(p)) for
        //given declaration: int (*p)[].
        //
        //The value is needed if there is not an ARRAY operator,
        //e.g: a = *p, should generate a=LD(LD(p)).
        xcom::remove(&DECL_trait(td), ld);
    } else if (ld->is_dt_fun()) {
        //ACCEPT
    } else {
        err(t->getLineno(), "illegal indirection");
        return ST_ERR;
    }
    TREE_result_type(t) = td;
    return ST_SUCC;
}


static INT TypeTranMulti(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    ASSERT0(TREE_token(t) == T_ASTERISK || TREE_token(t) == T_DIV ||
            TREE_token(t) == T_MOD);
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) { return ST_ERR; }
    if (ST_SUCC != TypeTranList(t->rchild(), cont)) { return ST_ERR; }

    Decl * ld = TREE_result_type(t->lchild());
    Decl * rd = TREE_result_type(t->rchild());
    if (TREE_token(t) == T_ASTERISK || TREE_token(t) == T_DIV) {
        //Deal with perand type of MUL, DIV.
        if (!ld->is_arith()) {
            err(t->getLineno(),
                "illegal operation for '%s', left operand"
                " must be arithmetic type",
                getTokenName(TREE_token(t)));
            return ST_ERR;
        }

        if (!rd->is_arith()) {
            err(t->getLineno(),
                "illegal operation for '%s', right operand"
                " must be arithmetic type",
                getTokenName(TREE_token(t)));
            return ST_ERR;
        }

        //arithmetic operation
        TREE_result_type(t) = buildBinaryOpType(t->getCode(), ld, rd);
        return ST_SUCC;
    }

    //Deal with perand type of MOD.
    if (!ld->is_integer()) {
        err(t->getLineno(),
            "illegal operation for '%s', left operand must be integer",
            getTokenName(TREE_token(t)));
        return ST_ERR;
    }
    if (!rd->is_integer()) {
        err(t->getLineno(),
            "illegal operation for '%s', right operand must be integer",
            getTokenName(TREE_token(t)));
        return ST_ERR;
    }

    //arithmetic operation
    TREE_result_type(t) = buildBinaryOpType(t->getCode(), ld, rd);
    return ST_SUCC;
}


static INT TypeTranCond(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_det(t), cont)) { return ST_ERR; }
    if (ST_SUCC != TypeTranList(TREE_true_part(t), cont)) { return ST_ERR; }
    if (ST_SUCC != TypeTranList(TREE_false_part(t), cont)) { return ST_ERR; }
    Decl * td = xcom::get_last(TREE_true_part(t))->getResultType();
    Decl * fd = xcom::get_last(TREE_false_part(t))->getResultType();
    ASSERT0(td && fd);
    Tree const* falsepart = TREE_false_part(t);
    if (td->is_pointer() && !fd->is_pointer()) {
        if (falsepart->is_imm_int() && TREE_imm_val(falsepart) == 0) {
            //This is valid case:0 can be treated as NULL pointer.
            //CASE: compare a pointer to 0.
            ;
        } else if (fd->isCharArray()) {
            //This is valid case:char array can be treated as a pointer that
            //point to a string.
            //CASE: compare a pointer to a char-array, and leave the type
            //checking to type-check pass.
            ;
        } else {
            err(t->getLineno(),
                "no conversion from pointer to non-pointer");
            return ST_ERR;
        }
    } else if (!td->is_pointer() && fd->is_pointer()) {
        if (!TREE_true_part(t)->is_imm_int() ||
            TREE_imm_val(TREE_true_part(t)) != 0) {
            err(t->getLineno(), "no conversion from pointer to non-pointer");
            return ST_ERR;
        }
    } else if (td->is_array() && !fd->is_array()) {
        err(t->getLineno(), "no conversion from array to non-array");
        return ST_ERR;
    } else if (!td->is_array() && fd->is_array()) {
        err(t->getLineno(), "no conversion from non-array to array");
        return ST_ERR;
    } else if (td->is_struct() && !fd->is_struct()) {
        err(t->getLineno(),
            "can not select between struct and non-struct");
        return ST_ERR;
    } else if (td->is_union() && !fd->is_union()) {
        err(t->getLineno(),
            "can not select between union and non-union");
        return ST_ERR;
    }

    //Record true-part type as the result type.
    TREE_result_type(t) = td;
    return ST_SUCC;
}


static INT TypeTranPreAndPostInc(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_inc_exp(t), cont)) { return ST_ERR; }

    Decl * d = TREE_result_type(TREE_inc_exp(t));
    if (!d->is_arith() && !d->is_pointer()) {
        xcom::DefFixedStrBuf buf;
        format_declaration(buf, d, true);
        if (t->getCode() == TR_INC) {
            err(t->getLineno(),
                "illegal prefixed '++', for type '%s'", buf.getBuf());
        } else {
            err(t->getLineno(),
                "illegal postfix '++', for type '%s'", buf.getBuf());
        }
    }
    TREE_result_type(t) = d;
    return ST_SUCC;
}


static INT TypeTranPreAndPostDec(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_dec_exp(t), cont)) { return ST_ERR; }

    Decl * d = TREE_result_type(TREE_dec_exp(t));
    if (!d->is_arith() && !d->is_pointer()) {
        xcom::DefFixedStrBuf buf;
        format_declaration(buf, d, true);
        if (t->getCode() == TR_DEC) {
            err(t->getLineno(),
                "illegal prefixed '--' for type '%s'", buf.getBuf());
        } else {
            err(t->getLineno(),
                "illegal postfix '--' for type '%s'", buf.getBuf());
        }
    }
    TREE_result_type(t) = d;
    return ST_SUCC;
}


static INT TypeTranSizeof(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    Tree * exp = TREE_sizeof_exp(t);
    if (exp == nullptr) {
        err(t->getLineno(), "miss expression after sizeof");
        return ST_ERR;
    }

    if (exp->getCode() == TR_TYPE_NAME) {
        Decl * type_name = TREE_type_name(exp);
        if (type_name->getTypeAttr()->is_user_type_ref()) {
            //Expand the combined type here.
            type_name = expandUserType(type_name);
            ASSERTN(is_valid_type_name(type_name),
                    ("Illegal expanding user-type"));
            TREE_type_name(exp) = type_name;
        }
    }

    UINT size;
    if (exp->getCode() == TR_TYPE_NAME) {
        ASSERT0(TREE_type_name(exp));
        size = TREE_type_name(exp)->getDeclByteSize();
    } else {
        if (ST_SUCC != TypeTranList(exp, cont)) {
            return ST_ERR;
        }
        ASSERT0(TREE_result_type(exp));
        size = TREE_result_type(exp)->getDeclByteSize();
    }
    ASSERT0(size != 0);
    TREE_code(t) = TR_IMMU;
    TREE_imm_val(t) = size;
    return TypeTranList(t, cont);
}


static INT TypeTranInDMem(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_base_region(t), cont)) {
        return ST_ERR;
    }

    Decl * ld = TREE_result_type(TREE_base_region(t));
    ASSERTN(TREE_field(t)->getCode() == TR_ID, ("illegal TR_INDMEM node!!"));
    if (!ld->getTypeAttr()->is_aggr()) {
        err(t->getLineno(), "left of '->' must have struct/union type");
        return ST_ERR;
    }

    cont->is_field = true;
    cont->base_tree_node = TREE_base_region(t);
    if (ST_SUCC != TypeTranList(TREE_field(t), cont)) {
        return ST_ERR;
    }

    Decl const* rd = TREE_result_type(TREE_field(t));
    cont->base_tree_node = nullptr;
    cont->is_field = false;

    if (!ld->is_pointer()) {
        xoc::Sym const* sym = TREE_id_decl(TREE_field(t))->getDeclSym();
        err(t->getLineno(),
            "'->%s' : left operand has 'struct' type, should use '.'",
            sym->getStr());
        return ST_ERR;
    }

    TREE_result_type(t) = buildTypeName(rd->getTypeAttr());
    DECL_trait(TREE_result_type(t)) = dupDeclBeginAt(DECL_trait(rd));
    return ST_SUCC;
}


static INT TypeTranDMem(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_base_region(t), cont)) {
        return ST_ERR;
    }

    Decl * ld = TREE_result_type(TREE_base_region(t));
    ASSERTN(TREE_field(t)->getCode() == TR_ID, ("illegal TR_DMEM node!!"));
    if (!ld->getTypeAttr()->is_aggr()) {
        err(t->getLineno(),
            "left of field access operation '.' must be struct/union type");
        return ST_ERR;
    }

    cont->is_field = true;
    cont->base_tree_node = TREE_base_region(t);
    if (ST_SUCC != TypeTranList(TREE_field(t), cont)) {
        return ST_ERR;
    }

    Decl * rd = TREE_result_type(TREE_field(t));
    cont->base_tree_node = nullptr;
    cont->is_field = false;

    if (ld->is_pointer()) {
        Sym const* sym = TREE_id_decl(TREE_field(t))->getDeclSym();
        err(t->getLineno(),
            "'.%s' : left operand points to 'struct' type, should use '->'",
            sym->getStr());
        return ST_ERR;
    }

    TREE_result_type(t) = buildTypeName(rd->getTypeAttr());
    DECL_trait(TREE_result_type(t)) = dupDeclBeginAt(DECL_trait(rd));
    return ST_SUCC;
}


static INT TypeTranArray(Tree * t, TYCtx * cont)
{
    //Note array base type can ONLY either ARRAY or POINTER.
    ASSERT0(t);
    //Under C specification, user can derefence pointer
    //utilizing array-operator, thus the function also transfering type for
    //multi-level pointer, even if t is not array type.
    //e.g:
    //  int ** p;
    //  p[i][j] = 10;
    if (ST_SUCC != TypeTranList(TREE_array_base(t), cont)) {
        return ST_ERR;
    }
    if (ST_SUCC != TypeTranList(TREE_array_indx(t), cont)) {
        return ST_ERR;
    }
    Decl * basetype = TREE_array_base(t)->getResultType();

    //Return sub-dimension type if 'basetype' is
    //multi-dimensional array or multi-level pointer.
    Decl * resty = dupTypeName(basetype);
    if (resty->getTraitList() == nullptr) {
        err(t->getLineno(),
            "The referrence of array is not match with its declaration.");
    } else {
        reduceDimForArrayOrPointer(resty);
    }
    TREE_result_type(t) = resty;
    return ST_SUCC;
}


static INT TypeTranCall(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(TREE_para_list(t), cont)) {
        return ST_ERR;
    }
    if (ST_SUCC != TypeTranList(TREE_fun_exp(t), cont)) {
        return ST_ERR;
    }

    insertCvtForParams(t);
    Decl * ld = TREE_result_type(TREE_fun_exp(t));
    ASSERTN(ld->is_dt_typename(), ("expect TypeAttr-NAME"));
    ASSERT0(DECL_decl_list(ld));
    ASSERTN(DECL_decl_list(ld)->is_dt_abs_declarator(),
            ("expect abs-declarator"));

    //Return value type is the CALL's result-type.
    //So constructing return value type.
    TypeAttr * ty = ld->getTypeAttr();
    Decl * pure = DECL_trait(ld); //pure may changed.
    if (pure != nullptr) {
        if (pure != nullptr && pure->is_dt_fun()) {
            //The next DCL of DCL_FUN is the return-value type if exist.
            pure = DECL_next(pure);
        } else if (pure->is_dt_pointer() &&
                   DECL_next(pure) != nullptr &&
                   DECL_next(pure)->is_dt_fun()) {
            //'t' is FUN_POINTER
            //The next of next of DCL of DCL_FUN is the return-value type
            // if exist.
            pure = DECL_next(DECL_next(pure));
        }
    }
    ASSERTN(pure == nullptr || !pure->is_dt_fun(), ("Illegal dcl list"));
    TREE_result_type(t) = buildTypeName(ty);
    DECL_trait(TREE_result_type(t)) = pure;
    return ST_SUCC;
}


//The function handles token '+' '-'.
static INT TypeTranAdditive(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) {
        return ST_ERR;
    }

    if (ST_SUCC != TypeTranList(t->rchild(), cont)) {
        return ST_ERR;
    }

    Decl * ld = TREE_result_type(t->lchild());
    Decl * rd = TREE_result_type(t->rchild());
    if (TREE_token(t) == T_ADD) { // '+'
        if (ld->is_pointer() && rd->is_pointer()) {
            err(t->getLineno(), "can not add two pointers");
            return ST_ERR;
        }

        if (ld->is_array() && rd->is_array()) {
            err(t->getLineno(), "can not add two arrays");
            return ST_ERR;
        }

        if (!ld->is_pointer()) {
            if (ld->is_struct() || rd->is_union()) {
                err(t->getLineno(), "illegal '%s' for struct/union",
                    getTokenName(TREE_token(t)));
                return ST_ERR;
            }
        }

        if (rd->is_pointer()) {
            //Swap operands.
            Decl * tmp = ld;
            ld = rd;
            rd = tmp;
            TREE_result_type(t->lchild()) = ld;
            TREE_result_type(t->rchild()) = rd;
        }

        if (ld->is_array() && rd->is_integer()) {
            //Regard array type as pointer,
            //the result type is pointer.
            //e.g: int a[]; b=a+2; b is pointer.
            TREE_result_type(t) = buildPointerType(ld->getTypeAttr());
        } else if (ld->is_pointer() && rd->is_integer()) {
            //Pointer arith.
            TREE_result_type(t) = ld;
        } else if (ld->is_arith() && rd->is_arith()) {
            //Arithmetic type.
            TREE_result_type(t) = buildBinaryOpType(t->getCode(), ld, rd);
        } else {
            ASSERTN(0, ("illegal type for '%s'", getTokenName(TREE_token(t))));
        }
        return ST_SUCC;
    }

    if (TREE_token(t) == T_SUB) { // '-'
        if (!ld->is_pointer() && rd->is_pointer()) {
            err(t->getLineno(),
                "pointer can only be subtracted from another pointer");
            return ST_ERR;
        }

        if (!ld->is_pointer()) {
            if (ld->getTypeAttr()->isAggrExpanded()) {
                err(t->getLineno(), "illegal '%s' for struct/union",
                    getTokenName(TREE_token(t)));
                return ST_ERR;
            }
        }

        if (!rd->is_pointer()) {
            if (rd->getTypeAttr()->isAggrExpanded()) {
                err(t->getLineno(), "illegal '%s' for struct/union",
                    getTokenName(TREE_token(t)));
                return ST_ERR;
            }
        }

        if (ld->is_pointer() && rd->is_pointer()) {
            //pointer - pointer
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_UNSIGNED | T_SPEC_LONG);
        } else if (ld->is_pointer() && rd->is_integer()) {
            //pointer - integer
            TREE_result_type(t) = ld;
        } else if (ld->is_arith() && rd->is_arith()) {
            //Arithmetic type
            TREE_result_type(t) = buildBinaryOpType(t->getCode(), ld, rd);
        } else {
            err(t->getLineno(), "illegal operand type for '%s'",
                getTokenName(TREE_token(t)));
            return ST_ERR;
        }
        return ST_SUCC;
    }

    ASSERTN(0, ("illegal type for '%s'", getTokenName(TREE_token(t))));
    return ST_ERR;
}


static INT TypeTranDeclInit(Decl * decl, TYCtx * cont)
{
    ASSERT0(decl->is_initialized());
    Tree * inittree = decl->getDeclInitTree();
    ASSERT0(inittree);
    if (inittree->getCode() == TR_INITVAL_SCOPE) {
        ASSERT0(cont);
        TYCtx tc(*cont);
        tc.current_initialized_declaration = decl;
        return TypeTranInitValScope(inittree, &tc);
    }
    return TypeTranList(inittree, cont);
}


static INT TypeTranDeclInitList(Decl const* decl, TYCtx * cont)
{
    for (Decl const* dcl = decl; dcl != nullptr; dcl = DECL_next(dcl)) {
        ASSERT0(dcl->is_dt_declaration());
        if (!dcl->is_initialized()) { continue; }
        Tree * inittree = dcl->getDeclInitTree();
        ASSERT0(inittree);
        if (ST_SUCC != TypeTranDeclInit(const_cast<Decl*>(dcl), cont)) {
            return ST_ERR;
        }
    }
    return ST_SUCC;
}


static INT TypeTranScope(Scope * sc, TYCtx * cont)
{
    ASSERT0(sc);
    if (ST_SUCC != TypeTranDeclInitList(sc->getDeclList(), cont)) {
        return ST_ERR;
    }
    if (ST_SUCC != TypeTranList(sc->getStmtList(), cont)) {
        return ST_ERR;
    }
    return ST_SUCC;
}


static INT TypeTranAssign(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    // one of   '='   '*='   '/='   '%='  '+='
    //          '-='  '<<='  '>>='  '&='  '^='  '|='
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) {
        return ST_ERR;
    }
    if (ST_SUCC != TypeTranList(t->rchild(), cont)) {
        return ST_ERR;
    }
    if (!checkAssign(t, TREE_result_type(t->lchild()),
                     TREE_result_type(t->rchild()))) {
        return ST_ERR;
    }
    TREE_result_type(t) = TREE_result_type(t->lchild());
    return ST_SUCC;
}


static INT TypeTranBinaryLogical(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) { return ST_ERR; }
    if (ST_SUCC != TypeTranList(t->rchild(), cont)) { return ST_ERR; }

    Decl * ld = TREE_result_type(t->lchild());
    Decl * rd = TREE_result_type(t->rchild());
    if (ld->is_pointer() || ld->is_array()) {
        xcom::DefFixedStrBuf buf;
        format_declaration(buf, ld, true);
        err(t->getLineno(), "illegal '%s', left operand has type '%s'",
            getTokenName(TREE_token(t->lchild())), buf.getBuf());
        return ST_ERR;
    }

    if (rd->is_pointer() || rd->is_array()) {
        xcom::DefFixedStrBuf buf;
        format_declaration(buf, rd, true);
        err(t->getLineno(), "illegal '%s', right operand has type '%s'",
            getTokenName(TREE_token(t->rchild())), buf.getBuf());
        return ST_ERR;
    }

    if (ld->getTypeAttr()->isStructExpanded() ||
        rd->getTypeAttr()->isStructExpanded() ||
        ld->getTypeAttr()->isUnionExpanded() ||
        rd->getTypeAttr()->isUnionExpanded()) {
        err(t->getLineno(), "illegal '%s' for struct/union",
            getTokenName(TREE_token(t->rchild())));
        return ST_ERR;
    }
    TREE_result_type(t) = buildBinaryOpType(t->getCode(), ld, rd);
    return ST_SUCC;
}


static INT TypeTranBinaryRelation(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    if (ST_SUCC != TypeTranList(t->lchild(), cont)) { return ST_ERR; }
    if (ST_SUCC != TypeTranList(t->rchild(), cont)) { return ST_ERR; }

    Decl * ld = TREE_result_type(t->lchild());
    Decl * rd = TREE_result_type(t->rchild());
    ASSERT0(ld && rd);
    if (ld->getTypeAttr()->isAggrExpanded() && !ld->is_pointer()) {
        err(t->getLineno(),
            "can not do '%s' operation for %s.",
            getTokenName(TREE_token(t)),
            ld->getTypeAttr()->getAggrTypeName());
        return ST_ERR;
    }
    if (rd->getTypeAttr()->isAggrExpanded() && !rd->is_pointer()) {
        err(t->getLineno(),
            "can not do '%s' operation for %s.",
            getTokenName(TREE_token(t)),
            ld->getTypeAttr()->getAggrTypeName());
        return ST_ERR;
    }
    TREE_result_type(t) = BUILD_TYNAME(T_SPEC_UNSIGNED | T_SPEC_CHAR);
    return ST_SUCC;
}


static bool hasScopeInitVal(Decl const* decl)
{
    return decl->is_array() || decl->is_struct() || decl->is_union();
}


static INT TypeTranInitValScope(Tree * t, TYCtx * cont)
{
    ASSERT0(t);
    Decl * decl = nullptr;
    if (t->parent() != nullptr) {
        //Current type-tran is processing assignment.
        //e.g: char a[]={'x'};
        ASSERT0(t->parent()->getCode() == TR_ASSIGN);
        ASSERT0(TREE_lchild(t->parent())->getCode() == TR_ID);
        decl = TREE_id_decl(TREE_lchild(t->parent()));
    } else {
        //Current type-tran is processing declaration init-value trees.
        //These init-value trees are not recorded in the rchild of assignment.
        ASSERT0(cont);
        decl = cont->current_initialized_declaration;
    }
    ASSERTN(decl, ("none of senarios provides enough info"));
    if (!hasScopeInitVal(decl)) {
        xcom::DefFixedStrBuf buf;
        format_declaration(buf, decl, false);
        err(t->getLineno(),
            "'%s' can not be initialized via scoped initial value",
            buf.getBuf());
        return ST_ERR;
    }
    TypeTranList(TREE_initval_scope(t), cont);
    TREE_result_type(t) = decl;
    return ST_SUCC;
}


//Transfering type declaration for all AST nodes.
INT TypeTranList(Tree * t, TYCtx * cont)
{
    while (t != nullptr) {
        if (TypeTran(t, cont) != ST_SUCC) {
            return ST_ERR;
        }
        t = TREE_nsib(t);
    }
    return ST_SUCC;
}


//Transfering type declaration for all AST nodes.
INT TypeTran(Tree * t, TYCtx * cont)
{
    TYCtx ct;
    if (cont == nullptr) {
        cont = &ct;
    }
    g_src_line_num = t->getLineno();
    switch (t->getCode()) {
    case TR_ASSIGN:
        if (ST_SUCC != TypeTranAssign(t, cont)) { goto FAILED; }
        break;
    case TR_ID:
        if (ST_SUCC != TypeTranID(t, cont)) { goto FAILED; }
        break;
    case TR_IMM:
        if (xcom::get64BitValueHighNBit((UINT64)TREE_imm_val(t), 32) != 0) {
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_LONGLONG|T_QUA_CONST);
        } else {
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_INT|T_QUA_CONST);
        }
        break;
    case TR_IMMU:
        if (xcom::get64BitValueHighNBit((UINT64)TREE_imm_val(t), 32) != 0) {
            TREE_result_type(t) = BUILD_TYNAME(
                T_SPEC_UNSIGNED|T_SPEC_LONGLONG|T_QUA_CONST);
        } else {
            TREE_result_type(t) = BUILD_TYNAME(
                T_SPEC_UNSIGNED|T_SPEC_INT|T_QUA_CONST);
        }
        break;
    case TR_IMML:
        TREE_result_type(t) = BUILD_TYNAME(T_SPEC_LONGLONG|T_QUA_CONST);
        break;
    case TR_IMMUL:
        TREE_result_type(t) = BUILD_TYNAME(
            T_SPEC_UNSIGNED|T_SPEC_LONGLONG|T_QUA_CONST);
        break;
    case TR_FP:
    case TR_FPLD:
        TREE_result_type(t) = BUILD_TYNAME(T_SPEC_DOUBLE|T_QUA_CONST);
        break;
    case TR_FPF:
        TREE_result_type(t) = BUILD_TYNAME(T_SPEC_FLOAT|T_QUA_CONST);
        break;
    case TR_ENUM_CONST:
        TREE_result_type(t) = BUILD_TYNAME(T_SPEC_ENUM|T_QUA_CONST);
        break;
    case TR_STRING: {
        Decl * tn = BUILD_TYNAME(T_SPEC_CHAR|T_QUA_CONST);
        Decl * d = newDecl(DCL_ARRAY);
        ASSERT0(TREE_string_val(t));
        DECL_array_dim(d) = TREE_string_val(t)->getLen() + 1;
        xcom::add_next(&DECL_trait(tn), d);
        TREE_result_type(t) = tn;
        break;
    }
    case TR_LOGIC_OR: //logical or ||
    case TR_LOGIC_AND: //logical and &&
        if (ST_SUCC != TypeTranList(t->lchild(), cont)) { goto FAILED; }
        if (ST_SUCC != TypeTranList(t->rchild(), cont)) { goto FAILED; }
        TREE_result_type(t) = BUILD_TYNAME(T_SPEC_UNSIGNED|T_SPEC_CHAR);
        break;
    case TR_INCLUSIVE_OR: //inclusive or |
    case TR_XOR: //exclusive or
    case TR_INCLUSIVE_AND: //inclusive and &
    case TR_SHIFT: // >> <<
        if (ST_SUCC != TypeTranBinaryLogical(t, cont)) { goto FAILED; }
        break;
    case TR_EQUALITY: // == !=
    case TR_RELATION: // < > >= <=
        if (ST_SUCC != TypeTranBinaryRelation(t, cont)) { goto FAILED; }
        break;
    case TR_ADDITIVE: // '+' '-'
        if (ST_SUCC != TypeTranAdditive(t, cont)) { goto FAILED; }
        break;
    case TR_MULTI: // '*' '/' '%'
        if (ST_SUCC != TypeTranMulti(t, cont)) { goto FAILED; }
        break;
    case TR_INITVAL_SCOPE:
        if (ST_SUCC != TypeTranInitValScope(t, cont)) { goto FAILED; }
        break;
    case TR_SCOPE: {
        TYCtx tc;
        if (ST_SUCC != TypeTranScope(TREE_scope(t), &tc)) {
            goto FAILED;
        }
        break;
    }
    case TR_IF:
        if (ST_SUCC != TypeTranList(TREE_if_det(t), cont)) { goto FAILED; }
        if (ST_SUCC != TypeTranList(TREE_if_true_stmt(t), cont)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_if_false_stmt(t), cont)) {
            goto FAILED;
        }
        break;
    case TR_DO:
        if (ST_SUCC != TypeTranList(TREE_dowhile_det(t), cont)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_dowhile_body(t), cont)) {
            goto FAILED;
        }
        break;
    case TR_WHILE:
        if (ST_SUCC != TypeTranList(TREE_whiledo_det(t), cont)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_whiledo_body(t), cont)) {
            goto FAILED;
        }
        break;
    case TR_FOR:
        if (TREE_for_scope(t) != nullptr &&
            ST_SUCC != TypeTranDeclInitList(
                TREE_for_scope(t)->getDeclList(), nullptr)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_for_init(t), nullptr)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_for_det(t), nullptr)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_for_step(t), nullptr)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_for_body(t), nullptr)) {
            goto FAILED;
        }
        break;
    case TR_SWITCH:
        if (ST_SUCC != TypeTranList(TREE_switch_det(t), nullptr)) {
            goto FAILED;
        }
        if (ST_SUCC != TypeTranList(TREE_switch_body(t), nullptr)) {
            goto FAILED;
        }
        break;
    case TR_BREAK:
    case TR_CONTINUE:
    case TR_GOTO:
    case TR_LABEL:
    case TR_DEFAULT:
    case TR_CASE:
        break;
    case TR_RETURN:
        if (ST_SUCC != TypeTranList(TREE_ret_exp(t), cont)) { goto FAILED; }
        break;
    case TR_COND: //formulized log_OR_exp?exp:cond_exp
        if (ST_SUCC != TypeTranCond(t, cont)) { goto FAILED; }
        break;
    case TR_CVT: { //type convertion
        if (ST_SUCC != TypeTranList(TREE_cvt_exp(t), cont)) { goto FAILED; }

        Decl * type_name = TREE_type_name(TREE_cvt_type(t));
        if (type_name->getTypeAttr()->is_user_type_ref()) {
            //Expand the combined type here.
            type_name = expandUserType(type_name);
            ASSERTN(is_valid_type_name(type_name),
                    ("Illegal expanding user-type"));
        }
        TREE_result_type(t) = type_name;
        break;
    }
    case TR_TYPE_NAME: //user defined type or C standard type
        //TR_TYPE_NAME node should be process by its parent node directly.
        ASSERTN(0, ("Should not be arrival"));
        break;
    case TR_LDA: {  // &a get address of 'a'
        if (ST_SUCC != TypeTranList(t->lchild(), cont)) { goto FAILED; }
        TREE_result_type(t) = convertToPointerTypeName(
            t->lchild()->getResultType());
        break;
    }
    case TR_DEREF: // *p dereferencing the pointer 'p'
        if (ST_SUCC != TypeTranDeref(t, cont)) { goto FAILED; }
        break;
    case TR_PLUS: // +123
    case TR_MINUS: { // -123
        if (ST_SUCC != TypeTranList(t->lchild(), cont)) { goto FAILED; }
        Decl * ld = t->lchild()->getResultType();
        if (!ld->is_arith() || ld->is_array() || ld->is_pointer()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, ld, true);
            if (t->getCode() == TR_PLUS) {
                err(t->getLineno(),
                    "illegal positive '+' for type '%s'", buf.getBuf());
            } else {
                err(t->getLineno(),
                    "illegal minus '-' for type '%s'", buf.getBuf());
            }
        }
        TREE_result_type(t) = ld;
        break;
    }
    case TR_REV: { // reverse
        if (ST_SUCC != TypeTranList(t->lchild(), cont)) { goto FAILED; }

        Decl * ld = t->lchild()->getResultType();
        if (!ld->is_integer() || ld->is_array() || ld->is_pointer()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, ld, true);
            err(t->getLineno(),
                "illegal bit reverse operation for type '%s'", buf.getBuf());
        }
        TREE_result_type(t) = ld;
        break;
    }
    case TR_NOT: { // get non-value
        if (ST_SUCC != TypeTranList(t->lchild(), cont)) goto FAILED;

        Decl * ld = t->lchild()->getResultType();
        if (!ld->is_arith() && !ld->is_pointer() && !ld->is_bool()) {
            xcom::DefFixedStrBuf buf;
            format_declaration(buf, ld, true);
            err(t->getLineno(),
                "illegal logical not operation for type '%s'", buf.getBuf());
        }
        TREE_result_type(t) = ld;
        break;
    }
    case TR_INC:   //++a
    case TR_POST_INC: //a++
        if (ST_SUCC != TypeTranPreAndPostInc(t, cont)) { goto FAILED; }
        break;
    case TR_DEC: //--a
    case TR_POST_DEC: //a--
        if (ST_SUCC != TypeTranPreAndPostDec(t, cont)) { goto FAILED; }
        break;
    case TR_SIZEOF: // sizeof(a)
        if (ST_SUCC != TypeTranSizeof(t, cont)) { goto FAILED; }
        break;
    case TR_CALL:
        if (ST_SUCC != TypeTranCall(t, cont)) { goto FAILED; }
        break;
    case TR_ARRAY:
        if (ST_SUCC != TypeTranArray(t, cont)) { goto FAILED; }
        break;
    case TR_DMEM: // a.b
        if (ST_SUCC != TypeTranDMem(t, cont)) { goto FAILED; }
        break;
    case TR_INDMEM: // a->b
        if (ST_SUCC != TypeTranInDMem(t, cont)) { goto FAILED; }
        break;
    case TR_PRAGMA:
    case TR_PREP:
    case TR_DECL:
        break;
    default: ASSERTN(0, ("unknown tree type:%d", t->getCode()));
    }
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


void initTypeTran()
{
    g_schar_type = newTypeAttr(T_SPEC_SIGNED | T_SPEC_CHAR);
    g_sshort_type = newTypeAttr(T_SPEC_SIGNED | T_SPEC_SHORT);
    g_sint_type = newTypeAttr(T_SPEC_SIGNED | T_SPEC_INT);
    g_slong_type = newTypeAttr(T_SPEC_SIGNED | T_SPEC_LONG);
    g_slonglong_type = newTypeAttr(T_SPEC_SIGNED | T_SPEC_LONGLONG);

    g_uchar_type = newTypeAttr(T_SPEC_UNSIGNED | T_SPEC_CHAR);
    g_ushort_type = newTypeAttr(T_SPEC_UNSIGNED | T_SPEC_SHORT);
    g_uint_type = newTypeAttr(T_SPEC_UNSIGNED | T_SPEC_INT);
    g_ulong_type = newTypeAttr(T_SPEC_UNSIGNED | T_SPEC_LONG);
    g_ulonglong_type = newTypeAttr(T_SPEC_UNSIGNED | T_SPEC_LONGLONG);

    g_float_type = newTypeAttr(T_SPEC_FLOAT);
    g_double_type = newTypeAttr(T_SPEC_DOUBLE);
    g_void_type = newTypeAttr(T_SPEC_VOID);
    g_enum_type = newTypeAttr(T_SPEC_ENUM);
}


//Infer type to tree nodes.
INT TypeTransform()
{
    initTypeTran();
    Scope * s = get_global_scope();
    if (s == nullptr) { return ST_SUCC; }
    for (Decl * dcl = s->getDeclList(); dcl != nullptr; dcl = DECL_next(dcl)) {
        ASSERT0(dcl->getDeclScope() == s);
        TYCtx cont;
        if (dcl->is_fun_def()) {
            if (ST_SUCC != TypeTranScope(dcl->getFunBody(), &cont) ||
                g_err_msg_list.has_msg()) {
                return ST_ERR;
            }
        }
        if (dcl->is_initialized()) {
            if (ST_SUCC != TypeTranDeclInit(dcl, &cont) ||
                g_err_msg_list.has_msg()) {
                return ST_ERR;
            }
        }
    }
    TYCtx cont;
    if (ST_SUCC != TypeTranList(s->getStmtList(), &cont) ||
        g_err_msg_list.has_msg()) {
        return ST_ERR;
    }
    return ST_SUCC;
}

} //namespace xfe

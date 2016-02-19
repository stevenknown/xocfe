/*@
Copyright (c) 2013-2014, Su Zhenyu steven.known@gmail.com
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
#include "cfecom.h"
#include "cfecommacro.h"

class TYCtx {
public:
    //for lvalue expression , TR_ID should corresponding with IR_ID, but IR_LD.
    bool is_lvalue;

    //record current id whether one field of struct or union
    bool is_field;

    //record base of current memory . e.g field of struct/union
    Tree * base_tree_node;
};


static TypeSpec * g_schar_type ;
static TypeSpec * g_sshort_type ;
static TypeSpec * g_sint_type ;
static TypeSpec * g_slong_type ;
static TypeSpec * g_slonglong_type ;

static TypeSpec * g_uchar_type ;
static TypeSpec * g_ushort_type ;
static TypeSpec * g_uint_type ;
static TypeSpec * g_ulong_type ;
static TypeSpec * g_ulonglong_type ;

static TypeSpec * g_float_type ;
static TypeSpec * g_double_type ;
static TypeSpec * g_void_type ;
static TypeSpec * g_enum_type ;

static INT process_array_init(Decl * dcl, TypeSpec * ty, Tree ** init);
static INT process_pointer_init(Decl * dcl, TypeSpec * ty, Tree ** init);
static INT process_struct_init(TypeSpec * ty, Tree ** init);
static INT process_union_init(TypeSpec * ty, Tree ** init);
static INT process_base_init(TypeSpec * ty, Tree ** init);
static TypeSpec * build_base_type_spec(INT des);
static INT c_type_ck(Tree * t, TYCtx * cont);

#define BUILD_TYNAME(T)  build_type_name(build_base_type_spec(T))


//Go through the init tree , 'dcl' must be DCL_ARRAY
//NOTE: compute_array_dim() should be invoked.
static INT process_array_init(Decl * dcl, TypeSpec * ty, Tree ** init)
{
    INT st = ST_SUCC;
    if (DECL_dt(dcl) == DCL_ID) {
        dcl = DECL_next(dcl);
    }
    ASSERT(DECL_dt(dcl) == DCL_ARRAY, ("ONLY can be DCL_ARRAY"));
    ULONGLONG dim = DECL_array_dim(dcl), count = 0;

    Decl * head = dcl, * tail = NULL;
    while (DECL_next(dcl) != NULL && DECL_dt(DECL_next(dcl)) == DCL_ARRAY) {
        dcl = DECL_next(dcl);
    }
    tail = dcl;

    if (head != tail) {
        //multipul dimension array.
        while (*init != NULL && st == ST_SUCC) {
            st = process_array_init(DECL_next(head), ty, init);
            count++;
            if (dim > 0 && count >= dim) {
                break;
            }
        }
    } else {
        //single dimension array
        /* When we meet a TR_EXP_SCOPE, because now we are
        initializing a array, so the initialization set up
        from subset of EXP_SCOPE */
        if (TREE_type(*init) == TR_EXP_SCOPE) {
            Tree * t = TREE_exp_scope(*init);
            ty = get_pure_type_spec(ty);
            while (t != NULL && st == ST_SUCC) {
                if (is_struct(ty)) {
                    st = process_struct_init(ty, &t);
                } else if (is_union(ty)) {
                    st = process_union_init(ty, &t);
                } else if (is_pointer(dcl)) {
                    st = process_pointer_init(dcl, ty, &t);
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
            ty = get_pure_type_spec(ty);
            while (*init != NULL && st == ST_SUCC) {
                if (is_struct(ty)) {
                    st = process_struct_init(ty, init);
                } else if (is_union(ty)) {
                    st = process_union_init(ty, init);
                } else if (is_pointer(dcl)) {
                    st = process_pointer_init(dcl, ty, init);
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


static INT process_pointer_init(Decl * dcl, TypeSpec * ty, Tree ** init)
{
    Tree * t = TREE_exp_scope(*init);
    if (TREE_type(*init) == TR_EXP_SCOPE) {
        process_pointer_init(dcl, ty, &t);
        *init = TREE_nsib(*init);
        return ST_SUCC;
    }
    //TODO type check
    //...

    *init = TREE_nsib(*init);
    return ST_SUCC;
}


static INT process_struct_init(TypeSpec * ty, Tree ** init)
{
    INT st = ST_SUCC;
    Struct * s = TYPE_struct_type(ty);
    ASSERT(IS_STRUCT(ty), ("ONLY must be struct type-spec"));
    if (STRUCT_is_complete(s)) {
        err(g_real_line_num, "uses undefined struct %s",
            SYM_name(STRUCT_tag(s)));
        return ST_ERR;
    }

    Decl * sdcl = STRUCT_decl_list(s);
    if (TREE_type(*init) == TR_EXP_SCOPE) {
        Tree * t = TREE_exp_scope(*init);
        while (sdcl != NULL && t != NULL) {
            if ((st=process_init(sdcl, &t)) != ST_SUCC) {
                return st;
            }
            sdcl = DECL_next(sdcl);
        }
        *init = TREE_nsib(*init);
    } else {
        while (sdcl != NULL && *init != NULL) {
            if ((st = process_init(sdcl, init)) != ST_SUCC) {
                return st;
            }
            sdcl = DECL_next(sdcl);
        }
    }
    return st;
}


static INT process_union_init(TypeSpec * ty, Tree ** init)
{
    INT st = ST_SUCC;
    Union * s = TYPE_union_type(ty);
    ASSERT(IS_UNION(ty), ("ONLY must be union type-spec"));
    if (!UNION_is_complete(s)) {
        err(g_real_line_num, "uses undefined union %s",
            SYM_name((UNION_tag(s))));
        return ST_ERR;
    }
    Decl * sdcl = UNION_decl_list(s);
    while (sdcl != NULL && *init != NULL) {
        if ((st = process_init(sdcl,init)) != ST_SUCC) {
            return st;
        }
        sdcl = DECL_next(sdcl);
    }
    return st;
}


//C base type
static INT process_base_init(TypeSpec * ty, Tree ** init)
{
    Tree * t = TREE_exp_scope(*init);
    if (TREE_type(*init) == TR_EXP_SCOPE) {
        process_base_init(ty, &t);
        *init = TREE_nsib(*init);
        return ST_SUCC;
    }
    //TODO type check
    *init = TREE_nsib(*init);
    return ST_SUCC;
}


/*
Initializing check.
function aim:
  1. compute the exactly array index for zero count dimension
  2. check compatibility between init value type and type-spec info

'declaration' must ONLY be DCL_DECLARATION

*/
INT process_init(Decl * decl)
{
    Decl * dcl = NULL;
    TypeSpec * ty = NULL;
    Tree * init = NULL;
    INT st = ST_SUCC;
    if (decl == NULL) { return ST_SUCC; }
    ASSERT(DECL_dt(decl) == DCL_DECLARATION, ("ONLY can be DCRLARATION"));

    dcl = DECL_decl_list(decl);//get DCRLARATOR
    ty = DECL_spec(decl);//get TypeSpec-SPEC
    ASSERT(dcl && ty, ("DCLARATION must have a "
                        "DECRLARATOR node and TypeSpec node"));
    ASSERT(DECL_dt(dcl) == DCL_DECLARATOR, ("ONLY can be DECLARATOR"));
    if (!DECL_is_init(dcl)) { return ST_SUCC; }

    init = DECL_init_tree(dcl);
    if (init == NULL) {
        err(g_real_line_num, "initializing expression is illegal");
        return ST_ERR;
    }

    if (TREE_type(init) == TR_EXP_SCOPE) {
        init = TREE_exp_scope(init);
    }
    if (is_array(dcl)) {
        st = process_array_init(DECL_child(dcl),ty,&init);
    } else if (is_struct(ty)) {
        st = process_struct_init(ty, &init);
    } else if (is_union(ty)) {
        st = process_union_init(ty, &init);
    } else if (is_pointer(dcl)) {
        st = process_pointer_init(DECL_child(dcl), ty, &init);
    } else {
        //simple type init. e.g INT SHORT
        st = process_base_init(ty, &init);
    }

    if (init != NULL) {
        ASSERT0(get_decl_sym(decl));
        err(g_real_line_num,
            "you write too many initializers than var '%s' declared",
            SYM_name(get_decl_sym(decl)));
        st = ST_ERR;
    }
    return st;
}


/*
'decl' does not have its own initializing form tree,
therefore 'init' will be recorded as the initialization tree.
*/
INT process_init(Decl * decl, Tree ** init)
{
    Decl * dcl = NULL;
    TypeSpec * ty = NULL;
    INT st = ST_SUCC;
    if (decl == NULL) return ST_SUCC;
    ASSERT(DECL_dt(decl) == DCL_DECLARATION,
            ("ONLY can be DCRLARATION"));

    dcl = DECL_decl_list(decl); //get DCRLARATOR
    ty = DECL_spec(decl); //get TypeSpec-SPEC
    ASSERT((dcl && ty),
            ("DCLARATION must have a DCRLARATOR node and TypeSpec node"));
    ASSERT((*init) != NULL,
            ("'init' initialization tree cannot be NULL"));
    if (is_array(dcl)) {
        st = process_array_init(dcl, ty, init);
    } else if (is_struct(ty)) {
        st = process_struct_init(ty, init);
    } else if (is_union(ty)) {
        st = process_union_init(ty, init);
    } else if (is_pointer(dcl)) {
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
static bool is_valid_type_name(Decl * dcl)
{
    ASSERT0(dcl && DECL_decl_list(dcl));
    if (!(DECL_dt(dcl) == DCL_TYPE_NAME &&
          DECL_dt(DECL_decl_list(dcl)) == DCL_ABS_DECLARATOR)) {
        return false;
    }

    dcl = DECL_child(DECL_decl_list(dcl));

    //type name does not include ID.
    if (dcl == NULL || DECL_dt(dcl) == DCL_ARRAY ||
        DECL_dt(dcl) == DCL_POINTER || DECL_dt(dcl) == DCL_FUN) {
        return true;
    }
    return false;
}
#endif


//Constructing TypeSpec-NAME declaration
static Decl * build_type_name(TypeSpec * ty)
{
    Decl * decl = new_decl(DCL_TYPE_NAME);
    DECL_decl_list(decl) = new_decl(DCL_ABS_DECLARATOR);
    DECL_spec(decl) = ty;
    return decl;
}


//Only construct simply base type-spec
static TypeSpec * build_base_type_spec(INT des)
{
    if (!is_simple_base_type(des)) {
        ASSERT(0,("expect base type"));
        return NULL;
    }

    if (IS_TYPED(des, T_SPEC_SIGNED)) {
        if (IS_TYPED(des, T_SPEC_CHAR)) {
            return g_schar_type;

        } else if (IS_TYPED(des, T_SPEC_SHORT)) {
            return g_sshort_type;

        } else if (IS_TYPED(des, T_SPEC_INT)) {
            return g_sint_type;

        } else if (IS_TYPED(des, T_SPEC_LONG)) {
            return g_slong_type;

        } else if (IS_TYPED(des, T_SPEC_LONGLONG)) {
            return g_slonglong_type;

        } else if (IS_TYPED(des, T_SPEC_FLOAT)) {
            return g_float_type;

        } else if (IS_TYPED(des, T_SPEC_DOUBLE)) {
            return g_double_type;

        } else if (IS_TYPED(des, T_SPEC_VOID)) {
            return g_sint_type;

        }
    } else if (IS_TYPED(des, T_SPEC_UNSIGNED)) {
        if (IS_TYPED(des, T_SPEC_CHAR)) {
            return g_uchar_type;

        } else if (IS_TYPED(des, T_SPEC_SHORT)) {
            return g_ushort_type;

        } else if (IS_TYPED(des, T_SPEC_INT)) {
            return g_uint_type;

        } else if (IS_TYPED(des, T_SPEC_LONG)) {
            return g_ulong_type;

        } else if (IS_TYPED(des, T_SPEC_LONGLONG)) {
            return g_ulonglong_type;

        } else if (IS_TYPED(des, T_SPEC_FLOAT)) {
            return g_float_type;

        } else if (IS_TYPED(des, T_SPEC_DOUBLE)) {
            return g_double_type;

        } else if (IS_TYPED(des, T_SPEC_VOID)) {
            return g_uint_type;

        }
    } else {
        if (IS_TYPED(des, T_SPEC_CHAR)) {
            return g_schar_type;

        } else if (IS_TYPED(des, T_SPEC_SHORT)) {
            return g_sshort_type;

        } else if (IS_TYPED(des, T_SPEC_INT)) {
            return g_sint_type;

        } else if (IS_TYPED(des, T_SPEC_LONG)) {
            return g_slong_type;

        } else if (IS_TYPED(des, T_SPEC_LONGLONG)) {
            return g_slonglong_type;

        } else if (IS_TYPED(des, T_SPEC_FLOAT)) {
            return g_float_type;

        } else if (IS_TYPED(des, T_SPEC_DOUBLE)) {
            return g_double_type;

        } else if (IS_TYPED(des, T_SPEC_VOID)) {
            return g_void_type;
        } else if (IS_TYPED(des, T_SPEC_ENUM)) {
            return g_enum_type;
        }
    }
    ASSERT(0,("TODO"));
    return NULL;
}


static INT get_cvt_rank(INT des)
{
    if (IS_TYPED(des, T_SPEC_CHAR)) {
        return 20;
    } else if (IS_TYPED(des, T_SPEC_SHORT)) {
        return 30;
    } else if (IS_TYPED(des, T_SPEC_INT) ||
               IS_TYPED(des, T_SPEC_UNSIGNED) ||
               IS_TYPED(des, T_SPEC_ENUM)) {
        return 40;
    } else if (IS_TYPED(des, T_SPEC_LONG)) {
        return 50;
    } else if (IS_TYPED(des, T_SPEC_LONGLONG)) {
        return 60;
    } else if (IS_TYPED(des, T_SPEC_FLOAT)) {
        return 70;
    } else if (IS_TYPED(des, T_SPEC_DOUBLE)) {
        return 80;
    }
    ASSERT(0,("get_cvt_rank"));
    return 0;
}


/*
Accroding C89 binary operation converting rules
l                r                       standard C convert
double           any                     double
float            any                     float
unsigned         unsigned                upper rank unsigned
signed           signed                  upper rank signed
unsigned         lower rank signed       unsigned
unsigned         upper rank signed       upper rank signed
any              any                     no-convert
*/
static Decl * build_binary_op_type(Decl * l, Decl * r)
{
    TypeSpec * lty = DECL_spec(l);
    TypeSpec * rty = DECL_spec(r);
    if (get_cvt_rank(TYPE_des(lty)) > get_cvt_rank(TYPE_des(rty))) {
        return l;
    }
    return r;
}


//Checking type-convert of modifier
static bool ck_assign(Tree * t, Decl * ld, Decl *)
{
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    if (is_array(ld)) {
        format_declaration(buf, ld);
        err(TREE_lineno(t), "illegal '%s', left operand must be l-value", buf);
        return false;
    }
    if (IS_CONST(DECL_spec(ld))) {
        format_declaration(buf, ld);
        err(TREE_lineno(t),
             "illegal '%s', l-value specifies const object", buf);
        return false;
    }
    //TODO: we should check struct/union compatibility in 'ld' and 'rd'
    //Here look lchild of '=' as default type
    return true;
}


static bool type_tran_id(Tree * t, TYCtx * cont, CHAR buf[])
{
    //Construct type-name and expand user type if it was declared.
    Decl * tmp_decl = NULL;
    Tree * parent = TREE_parent(t);
    if (parent != NULL && cont->is_field &&
        (TREE_type(parent) == TR_DMEM ||
         TREE_type(parent) == TR_INDMEM)) {
        //At present, the ID may be a field of struct/union,
        //and you need to check out if it is not the correct field.

        //Get the struct/union type base.
        Decl * base = TREE_result_type(cont->base_tree_node);
        ASSERT(base, ("should be struct/union type"));
        if (is_struct(base)) {
            Struct * s = TYPE_struct_type(DECL_spec(base));
            //Search for matched field.
            Decl * field = STRUCT_decl_list(s);
            if (field == NULL) {
                buf[0] = 0;
                format_struct_union_complete(buf, DECL_spec(base));
                err(TREE_lineno(t),
                    " '%s' is an empty struct, '%s' is not its field",
                    buf, SYM_name(TREE_id(t)));
                return false;
            }

            while (field != NULL) {
                SYM * sym = get_decl_sym(field);
                if (!strcmp(SYM_name(sym), SYM_name(TREE_id(t)))) {
                    TREE_id_decl(t) = field;
                    tmp_decl = field;
                    break;
                }
                field = DECL_next(field);
            }
        } else {
            ASSERT0(is_union(base));
            Union * s = TYPE_union_type(DECL_spec(base));
            //Search for matched field.
            Decl * field = UNION_decl_list(s);
            if (field == NULL) {
                buf[0] = 0;
                format_struct_union_complete(buf, DECL_spec(base));
                err(TREE_lineno(t),
                    " '%s' is an empty struct, '%s' is not its field",
                    buf, SYM_name(TREE_id(t)));
                return false;
            }

            while (field != NULL) {
                SYM * sym = get_decl_sym(field);
                if (!strcmp(SYM_name(sym), SYM_name(TREE_id(t)))) {
                    TREE_id_decl(t) = field;
                    tmp_decl = field;
                    break;
                }
                field = DECL_next(field);
            }
        }

        if (tmp_decl == NULL) {
            buf[0] = 0;
            format_struct_union_complete(buf, DECL_spec(base));
            err(TREE_lineno(t),
                " '%s' : is not a member of type '%s'",
                SYM_name(TREE_id(t)), buf);
            return false;
        }
    } else {
        tmp_decl = TREE_id_decl(t);
    }

    if (is_user_type_ref(tmp_decl)) {
        //ID has been defined with typedef type.
        //we must expand the combined type here.
        tmp_decl = expand_user_type(tmp_decl);
    }

    //Construct TYPE_NAME for ID, that would
    //be used to infer type for tree node.
    TREE_result_type(t) = build_type_name(DECL_spec(tmp_decl));
    Decl * res_ty = TREE_result_type(t);


    //Set bit info if idenifier is bitfield.
    //Bit info stored at declarator list.
    Decl * declarator = DECL_decl_list(tmp_decl);
    DECL_is_bit_field(DECL_decl_list(res_ty)) = DECL_is_bit_field(declarator);
    DECL_bit_len(DECL_decl_list(res_ty)) = DECL_bit_len(declarator);

    if (DECL_is_bit_field(declarator)) {
        if (is_pointer(tmp_decl)) {
            format_declaration(buf, tmp_decl);
            err(TREE_lineno(t),
                "'%s' : pointer cannot assign bit length", buf);
            return false;
        }

        if (is_array(tmp_decl)) {
            format_declaration(buf, tmp_decl);
            err(TREE_lineno(t),
                "'%s' : array type cannot assign bit length", buf);
            return false;
        }

        if (!is_integer(tmp_decl)) {
            format_declaration(buf, tmp_decl);
            err(TREE_lineno(t), "'%s' : bit field must have integer type", buf);
            return false;
        }

        //Check bitfield's base type is big enough to hold it.
        INT size = get_decl_size(tmp_decl) * BIT_PER_BYTE;
        if (size < DECL_bit_len(declarator)) {
            format_declaration(buf, tmp_decl);
            err(TREE_lineno(t),
                "'%s' : type of bit field too small for number of bits", buf);
            return false;
        }
    }

    Decl * dcl_list = get_pure_declarator(tmp_decl);
    ASSERT(DECL_dt(dcl_list) == DCL_ID,
           ("'id' should be declarator-list-head. Illegal declaration"));

    //neglect the first DCL_ID node, we only need the rest.
    dcl_list = DECL_next(dcl_list);
    dcl_list = cp_decl_begin_at(dcl_list);

    //Reduce the number of POINTER of function-pointer to 1.
    //e.g: In C, you can define function pointer like:
    //  int (****fun)();
    //We simply it to be 'int (*fun)()'.
    Decl * tmp = dcl_list;
    while (tmp != NULL) {
        if (DECL_dt(tmp) == DCL_POINTER) {
            tmp = DECL_next(tmp);
        } else {
            if (DECL_dt(tmp) == DCL_FUN) {
                break;
            } else {
                tmp = dcl_list;
                break;
            }
        }
    }

    if (tmp != NULL &&
        DECL_dt(tmp) == DCL_FUN &&
        DECL_prev(tmp) != NULL &&
        DECL_dt(DECL_prev(tmp)) == DCL_POINTER) {
        PURE_DECL(res_ty) = DECL_prev(tmp);
        DECL_prev(DECL_prev(tmp)) = NULL;
    } else {
        PURE_DECL(res_ty) = tmp != NULL ? tmp : dcl_list;
    }
    return true;
}


//Transfering type declaration for all AST nodes.
static INT c_type_tran(Tree * t, TYCtx * cont)
{
    CHAR buf[MAX_BUF_LEN];
    buf[0] = 0;
    if (cont == NULL) {
        TYCtx ct = {0};
        cont = &ct;
    }
    while (t != NULL) {
        g_src_line_num = TREE_lineno(t);
        switch (TREE_type(t)) {
        case TR_ASSIGN:
            // one of   '='   '*='   '/='   '%='  '+='
            //          '-='  '<<='  '>>='  '&='  '^='  '|='
            if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) {
                goto FAILED;
            }
            if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) {
                goto FAILED;
            }
            if (!ck_assign(t, TREE_result_type(TREE_lchild(t)),
                           TREE_result_type(TREE_rchild(t)))) {
                goto FAILED;
            }
            TREE_result_type(t) = TREE_result_type(TREE_lchild(t));
            break;
        case TR_ID:
            if (!type_tran_id(t, cont, buf)) { goto FAILED; }
            break;
        case TR_IMM:
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_INT|T_QUA_CONST);
            break;
        case TR_IMML:
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_LONGLONG|T_QUA_CONST);
            break;
        case TR_FP:  //3.1415926
            if (BYTE_PER_CONST_FP == BYTE_PER_FLOAT) {
                TREE_result_type(t) = BUILD_TYNAME(T_SPEC_FLOAT|T_QUA_CONST);
            } else if (BYTE_PER_CONST_FP == BYTE_PER_DOUBLE) {
                TREE_result_type(t) = BUILD_TYNAME(T_SPEC_DOUBLE|T_QUA_CONST);
            } else {
                TREE_result_type(t) = BUILD_TYNAME(T_SPEC_FLOAT|T_QUA_CONST);
            }
            break;
        case TR_ENUM_CONST:
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_ENUM|T_QUA_CONST);
            break;
        case TR_STRING:
            {
                Decl * tn = BUILD_TYNAME(T_SPEC_CHAR|T_QUA_CONST);
                Decl * d = new_decl(DCL_ARRAY);
                ASSERT0(TREE_string_val(t));
                DECL_array_dim(d) = strlen(SYM_name(TREE_string_val(t))) + 1;
                add_next(&PURE_DECL(tn), d);
                TREE_result_type(t) = tn;
                break;
            }
        case TR_LOGIC_OR:  //logical or       ||
        case TR_LOGIC_AND: //logical and      &&
            if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) goto FAILED;
            TREE_result_type(t) = BUILD_TYNAME(T_SPEC_UNSIGNED | T_SPEC_CHAR);
            break;
        case TR_INCLUSIVE_OR: //inclusive or  |
        case TR_XOR: //exclusive or
        case TR_INCLUSIVE_AND: //inclusive and &
        case TR_SHIFT:   // >> <<
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) goto FAILED;

                Decl * ld = TREE_result_type(TREE_lchild(t));
                Decl * rd = TREE_result_type(TREE_rchild(t));
                if (is_pointer(ld) || is_array(ld)) {
                    format_declaration(buf,ld);
                    err(TREE_lineno(t),
                         "illegal '%s', left operand has type '%s'",
                         get_token_name(TREE_token(TREE_lchild(t))), buf);
                    goto FAILED;
                }

                if (is_pointer(rd) || is_array(rd)) {
                    format_declaration(buf,rd);
                    err(TREE_lineno(t),
                        "illegal '%s', right operand has type '%s'",
                        get_token_name(TREE_token(TREE_rchild(t))), buf);
                    goto FAILED;
                }

                if (is_struct(DECL_spec(ld)) ||
                    is_struct(DECL_spec(rd)) ||
                    is_union(DECL_spec(ld)) ||
                    is_union(DECL_spec(rd))) {
                    err(TREE_lineno(t), "illegal '%s' for struct/union",
                        get_token_name(TREE_token(TREE_rchild(t))));
                    goto FAILED;
                }
                TREE_result_type(t) = build_binary_op_type(ld, rd);
                break;
            }
        case TR_EQUALITY: // == !=
        case TR_RELATION: // < > >= <=
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t)),
                     * rd = TREE_result_type(TREE_rchild(t));
                ASSERT0(ld && rd);
                if ((is_struct(DECL_spec(ld)) || is_union(DECL_spec(ld))) &&
                    !is_pointer(ld)) {
                    err(TREE_lineno(t),
                         "can not do '%s' operation for struct/union.",
                         get_token_name(TREE_token(t)));
                    goto FAILED;
                } else if ((is_struct(DECL_spec(rd)) ||
                            is_union(DECL_spec(rd))) &&
                           !is_pointer(rd)) {
                    err(TREE_lineno(t),
                         "can not do '%s' operation for struct/union.",
                         get_token_name(TREE_token(t)));
                    goto FAILED;
                }
                TREE_result_type(t) =
                    BUILD_TYNAME(T_SPEC_UNSIGNED | T_SPEC_CHAR);
                break;
            }
        case TR_ADDITIVE: // '+' '-'
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) {
                    goto FAILED;
                }
                if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) {
                    goto FAILED;
                }
                Decl * ld = TREE_result_type(TREE_lchild(t)),
                     * rd = TREE_result_type(TREE_rchild(t));

                if (TREE_token(t) == T_ADD) { // '+'
                    if ((is_pointer(ld) && is_pointer(rd)) ||
                        (is_array(ld) && is_array(rd))) {
                        err(TREE_lineno(t), "cannot add two pointers");
                        goto FAILED;
                    }

                    if (is_pointer(rd)) {
                        Decl * tmp = ld;
                        ld = rd;
                        rd = tmp;
                        TREE_result_type(TREE_lchild(t)) = ld;
                        TREE_result_type(TREE_rchild(t)) = rd;
                    }

                    if (!is_pointer(ld)) {
                        if (is_struct(DECL_spec(ld)) ||
                            is_union(DECL_spec(ld))) {
                            err(TREE_lineno(t),
                                 "illegal '%s' for struct/union",
                                 get_token_name(TREE_token(t)));
                            goto FAILED;
                        }
                    }

                    if (is_pointer(ld) && is_integer(rd)) {
                        TREE_result_type(t) = ld;
                    } else if (is_arith(ld) && is_arith(rd)) {
                        //arithmetic operation
                        TREE_result_type(t) = build_binary_op_type(ld, rd);
                    } else {
                        ASSERT(0,("TODO"));
                    }
                } else if (TREE_token(t) == T_SUB) { // '-'
                    if (!is_pointer(ld) && is_pointer(rd)) {
                        err(TREE_lineno(t),
                             "pointer can only be "
                             "subtracted from another pointer");
                        goto FAILED;
                    }

                    if (!is_pointer(ld)) {
                        if (is_struct(DECL_spec(ld)) ||
                            is_union(DECL_spec(ld))) {
                            err(TREE_lineno(t),
                                 "illegal '%s' for struct/union",
                                 get_token_name(TREE_token(t)));
                            goto FAILED;
                        }
                    }

                    if (!is_pointer(rd)) {
                        if (is_struct(DECL_spec(rd)) ||
                            is_union(DECL_spec(rd))) {
                            err(TREE_lineno(t),
                                 "illegal '%s' for struct/union",
                                 get_token_name(TREE_token(t)));
                            goto FAILED;
                        }
                    }

                    if (is_pointer(ld) && is_pointer(rd)) {
                        //pointer - pointer
                        TREE_result_type(t) =
                            BUILD_TYNAME(T_SPEC_UNSIGNED | T_SPEC_LONG);
                    } else if (is_pointer(ld) && is_integer(rd)) {
                        //pointer - integer
                        TREE_result_type(t) = ld;
                    } else if (is_arith(ld) && is_arith(rd)) {
                        //arithmetic operation
                        TREE_result_type(t) = build_binary_op_type(ld, rd);
                    } else {
                        ASSERT(0,("TODO"));
                    }

                } else ASSERT(0,("TODO"));
                 break;
            }
        case TR_MULTI:    // '*' '/' '%'
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                if (ST_SUCC != c_type_tran(TREE_rchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t));
                Decl * rd = TREE_result_type(TREE_rchild(t));

                if (TREE_token(t) == T_ASTERISK || TREE_token(t) == T_DIV) {
                    if (is_arith(ld) && is_arith(rd)) {
                        //arithmetic operation
                        TREE_result_type(t) = build_binary_op_type(ld, rd);
                    } else {
                        err(TREE_lineno(t), "illegal operation for '%s'",
                             get_token_name(TREE_token(TREE_rchild(t))));
                        goto FAILED;
                    }
                } else {
                    if (is_integer(ld) && is_integer(rd)) {
                        //arithmetic operation
                        TREE_result_type(t) = build_binary_op_type(ld, rd);
                    } else {
                        err(TREE_lineno(t), "illegal operation for '%%'");
                        goto FAILED;
                    }
                }
                break;
            }
        case TR_SCOPE:
            {
                SCOPE * sc = TREE_scope(t);
                if (ST_SUCC != c_type_tran(SCOPE_stmt_list(sc), NULL)) goto FAILED;
                break;
            }
        case TR_IF:
            if (ST_SUCC != c_type_tran(TREE_if_det(t), cont)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_if_true_stmt(t), cont)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_if_false_stmt(t), cont)) goto FAILED;
            break;
        case TR_DO:
            if (ST_SUCC != c_type_tran(TREE_dowhile_det(t), cont)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_dowhile_body(t), cont)) goto FAILED;
            break;
        case TR_WHILE:
            if (ST_SUCC != c_type_tran(TREE_whiledo_det(t), cont)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_whiledo_body(t), cont)) goto FAILED;
            break;
        case TR_FOR:
            if (ST_SUCC != c_type_tran(TREE_for_init(t), NULL)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_for_det(t), NULL)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_for_step(t), NULL)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_for_body(t), NULL)) goto FAILED;
            break;
        case TR_SWITCH:
            if (ST_SUCC != c_type_tran(TREE_switch_det(t), NULL)) goto FAILED;
            if (ST_SUCC != c_type_tran(TREE_switch_body(t), NULL)) goto FAILED;
            break;
        case TR_BREAK:
        case TR_CONTINUE:
        case TR_GOTO:
        case TR_LABEL:
        case TR_DEFAULT:
        case TR_CASE:
            break;
        case TR_RETURN:
            if (ST_SUCC != c_type_tran(TREE_ret_exp(t), cont)) goto FAILED;
            break;
        case TR_COND: //formulized log_OR_exp?exp:cond_exp
            {
                if (ST_SUCC != c_type_tran(TREE_det(t), cont)) goto FAILED;
                if (ST_SUCC != c_type_tran(TREE_true_part(t), cont)) {
                    goto FAILED;
                }
                if (ST_SUCC != c_type_tran(TREE_false_part(t), cont)) {
                    goto FAILED;
                }
                Decl * td = TREE_result_type(TREE_true_part(t));
                Decl * fd = TREE_result_type(TREE_false_part(t));
                ASSERT0(td && fd);
                if (is_pointer(td) && !is_pointer(fd)) {
                    err(TREE_lineno(t), "no conversion from pointer to non-pointer");
                    goto FAILED;
                }
                if (!is_pointer(td) && is_pointer(fd)) {
                    err(TREE_lineno(t), "no conversion from non-pointer to pointer");
                    goto FAILED;
                }
                if (is_array(td) && !is_array(fd)) {
                    err(TREE_lineno(t), "no conversion from array to non-array");
                    goto FAILED;
                }
                if (!is_array(td) && is_array(fd)) {
                    err(TREE_lineno(t), "no conversion from non-array to array");
                    goto FAILED;
                }
                if (is_struct(td) && !is_struct(fd)) {
                    err(TREE_lineno(t), "can not select between struct and non-struct");
                    goto FAILED;
                }
                if (is_union(td) && !is_union(fd)) {
                    err(TREE_lineno(t), "can not select between union and non-union");
                    goto FAILED;
                }
                TREE_result_type(t) = td; //record true-part type as the result type.
            }
            break;
        case TR_CVT: //type convertion
            {
                if (ST_SUCC != c_type_tran(TREE_cast_exp(t), cont)) goto FAILED;

                Decl * type_name = TREE_type_name(TREE_cvt_type(t));

                if (IS_USER_TYPE_REF(DECL_spec(type_name))) {
                    //Expand the combined type here.
                    type_name = expand_user_type(type_name);
                    ASSERT(is_valid_type_name(type_name),
                            ("Illegal expanding user-type"));
                }

                TREE_result_type(t) = type_name;
            }
            break;
        case TR_TYPE_NAME: //user defined type or C standard type
            //TR_TYPE_NAME node should be process by its parent node directly.
            ASSERT(0, ("Should not be arrival"));
            break;
        case TR_LDA:   // &a get address of 'a'
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t));
                Decl * td = cp_type_name(ld);
                insertafter(&PURE_DECL(td), new_decl(DCL_POINTER));
                TREE_result_type(t) = td;
                break;
            }
        case TR_DEREF: //*p  dereferencing the pointer 'p'
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) {
                    goto FAILED;
                }
                Decl * ld = TREE_result_type(TREE_lchild(t));
                if (!is_pointer(ld) && !is_array(ld)) {
                    err(TREE_lineno(t), "Illegal dereferencing operation, "
                        "indirection operation should operate on pointer type.");

                    goto FAILED;
                }
                Decl * td = cp_type_name(ld);
                ld = PURE_DECL(td);
                ASSERT(ld, ("lchild must be pointer type"));
                if (DECL_dt(ld) == DCL_POINTER ||
                    DECL_dt(ld) == DCL_ARRAY) {
                    ASSERT0(TREE_parent(t));
                    //if (TREE_type(parent) != TR_ARRAY)
                    {
                        /*
                        In C, base of array only needs address, so the DEREF
                        operator has alias effect. It means ARRAY(LD(p)) for
                        given declaration: int (*p)[].

                        The value is needed if there is not an ARRAY operator,
                        e.g: a = *p, should generate a=LD(LD(p)).
                        */
                        remove(&PURE_DECL(td), ld);
                    }
                } else if (DECL_dt(ld) == DCL_FUN) {
                    //ACCEPT
                } else {
                    err(TREE_lineno(t), "illegal indirection");
                    goto FAILED;
                }
                TREE_result_type(t) = td;
                break;
            }
        case TR_PLUS: // +123
        case TR_MINUS:  // -123
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t));
                if (!is_arith(ld) || is_array(ld) || is_pointer(ld)) {
                    format_declaration(buf,ld);
                    if (TREE_type(t) == TR_PLUS) {
                        err(TREE_lineno(t),
                            "illegal positive '+' for type '%s'", buf);
                    } else {
                        err(TREE_lineno(t),
                            "illegal minus '-' for type '%s'", buf);
                    }
                }
                TREE_result_type(t) = ld;
                break;
            }
        case TR_REV:  // Reverse
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t));
                if (!is_integer(ld) || is_array(ld) || is_pointer(ld)) {
                    format_declaration(buf,ld);
                    err(TREE_lineno(t),
                        "illegal bit reverse operation for type '%s'", buf);
                }
                TREE_result_type(t) = ld;
                break;
            }
        case TR_NOT:  // get non-value
            {
                if (ST_SUCC != c_type_tran(TREE_lchild(t), cont)) goto FAILED;
                Decl * ld = TREE_result_type(TREE_lchild(t));
                if (!is_arith(ld)) {
                    format_declaration(buf,ld);
                    err(TREE_lineno(t),
                        "illegal logical not operation for type '%s'", buf);
                }
                TREE_result_type(t) = ld;
                break;
            }
        case TR_INC:   //++a
        case TR_POST_INC: //a++
            {
                if (ST_SUCC != c_type_tran(TREE_inc_exp(t), cont)) goto FAILED;
                Decl * d = TREE_result_type(TREE_inc_exp(t));
                if (!is_arith(d) && !is_pointer(d)) {
                    format_declaration(buf, d);
                    if (TREE_type(t) == TR_INC) {
                        err(TREE_lineno(t),
                            "illegal prefixed '++', for type '%s'", buf);
                    } else {
                        err(TREE_lineno(t),
                            "illegal postfix '++', for type '%s'", buf);
                    }
                }
                TREE_result_type(t) = d;
                break;
            }
        case TR_DEC:   //--a
        case TR_POST_DEC: //a--
            {
                if (ST_SUCC != c_type_tran(TREE_dec_exp(t), cont)) goto FAILED;

                Decl * d = TREE_result_type(TREE_dec_exp(t));
                if (!is_arith(d) && !is_pointer(d)) {
                    format_declaration(buf, d);
                    if (TREE_type(t) == TR_DEC) {
                        err(TREE_lineno(t),
                            "illegal prefixed '--' for type '%s'", buf);
                    } else {
                        err(TREE_lineno(t),
                            "illegal postfix '--' for type '%s'", buf);
                    }
                }
                TREE_result_type(t) = d;
                break;
            }
        case TR_SIZEOF: // sizeof(a)
            {
                Tree * kid = TREE_sizeof_exp(t);
                if (kid == NULL) {
                    err(TREE_lineno(t), "miss expression after sizeof");
                    break;
                }
                INT size;
                if (TREE_type(kid) == TR_TYPE_NAME) {
                    ASSERT0(TREE_type_name(kid));
                    size = get_decl_size(TREE_type_name(kid));
                } else {
                    if (ST_SUCC != c_type_tran(kid, cont)) goto FAILED;
                    ASSERT0(TREE_result_type(kid));
                    size = get_decl_size(TREE_result_type(kid));
                }
                TREE_type(t) = TR_IMM;
                TREE_imm_val(t) = size;
                if (ST_SUCC != c_type_tran(t, cont)) goto FAILED;
                break;
            }
        case TR_CALL:
            {
                if (ST_SUCC != c_type_tran(TREE_para_list(t), cont)) {
                    goto FAILED;
                }
                if (ST_SUCC != c_type_tran(TREE_fun_exp(t), cont)) {
                    goto FAILED;
                }
                Decl * ld = TREE_result_type(TREE_fun_exp(t));
                ASSERT(DECL_dt(ld) == DCL_TYPE_NAME, ("expect TypeSpec-NAME"));

                //Return value type is the CALL node type.
                //So constructing return value type.
                TypeSpec * ty = DECL_spec(ld);
                ld = PURE_DECL(ld);
                if (DECL_dt(ld) == DCL_FUN) {
                    ld = DECL_next(ld);
                }

                if (ld != NULL) {
                    ASSERT(DECL_dt(ld) != DCL_FUN, ("Illegal dcl list"));
                }

                TREE_result_type(t) = build_type_name(ty);
                PURE_DECL(TREE_result_type(t)) = ld;
                break;
            }
        case TR_ARRAY:
            {
                /*
                Under C specification, user can derefence pointer
                utilizing array-operator,
                e.g:
                    int ** p;
                    p[i][j] = 10;
                */
                if (ST_SUCC != c_type_tran(TREE_array_base(t), cont)) {
                    goto FAILED;
                }
                if (ST_SUCC != c_type_tran(TREE_array_indx(t), cont)) {
                    goto FAILED;
                }

                Decl * ld = TREE_result_type(TREE_array_base(t));

                //Return sub-dimension of base if 'ld' is
                //multi-dimensional array.
                Decl * td = cp_type_name(ld);
                if (PURE_DECL(td) == NULL) {
                    err(TREE_lineno(t),
                        "The referrence of array is not match with its declaration.");
                }
                if (DECL_dt(PURE_DECL(td)) == DCL_ARRAY ||
                    DECL_dt(PURE_DECL(td)) == DCL_POINTER) {
                    removehead(&PURE_DECL(td));
                }
                TREE_result_type(t) = td;
                break;
            }
        case TR_DMEM: // a.b
            {
                Decl * rd, * ld;
                if (ST_SUCC != c_type_tran(TREE_base_region(t), cont)) {
                    goto FAILED;
                }
                ld = TREE_result_type(TREE_base_region(t));
                ASSERT(TREE_type(TREE_field(t)) == TR_ID,
                        ("illegal TR_DMEM node!!"));
                if (!IS_STRUCT(DECL_spec(ld)) && !IS_UNION(DECL_spec(ld))) {
                    err(TREE_lineno(t),
                        "left of field access operation '.' must be struct/union type");
                    goto FAILED;
                }

                cont->is_field = true;
                cont->base_tree_node = TREE_base_region(t);
                if (ST_SUCC != c_type_tran(TREE_field(t), cont)) goto FAILED;
                rd = TREE_result_type(TREE_field(t)),
                cont->base_tree_node = NULL;
                cont->is_field = false;

                if (is_pointer(ld)) {
                    SYM * sym = get_decl_sym(TREE_id_decl(TREE_field(t)));
                    err(TREE_lineno(t),
                        "'.%s' : left operand points to 'struct' type, should use '->'",
                        SYM_name(sym));
                    goto FAILED;
                }
                TREE_result_type(t) = build_type_name(DECL_spec(rd));
                PURE_DECL(TREE_result_type(t)) = cp_decl_begin_at(PURE_DECL(rd));
                break;
            }
        case TR_INDMEM: // a->b
            {
                Decl * rd,*ld;
                if (ST_SUCC != c_type_tran(TREE_base_region(t), cont)) goto FAILED;
                ld = TREE_result_type(TREE_base_region(t));

                ASSERT(TREE_type(TREE_field(t)) == TR_ID,
                        ("illegal TR_INDMEM node!!"));
                if (!IS_STRUCT(DECL_spec(ld)) && !IS_UNION(DECL_spec(ld))) {
                    err(TREE_lineno(t),
                        "left of '->' must have struct/union type");
                    goto FAILED;
                }

                cont->is_field = true;
                cont->base_tree_node = TREE_base_region(t);
                if (ST_SUCC != c_type_tran(TREE_field(t), cont)) goto FAILED;
                rd = TREE_result_type(TREE_field(t)),
                cont->base_tree_node = NULL;
                cont->is_field = false;

                if (!is_pointer(ld)) {
                    SYM * sym = get_decl_sym(TREE_id_decl(TREE_field(t)));
                    err(TREE_lineno(t),
                        "'->%s' : left operand has 'struct' type, use '.'",
                        SYM_name(sym));
                    goto FAILED;
                }
                TREE_result_type(t) = build_type_name(DECL_spec(rd));
                PURE_DECL(TREE_result_type(t)) =
                    cp_decl_begin_at(PURE_DECL(rd));
                break;
            }
        case TR_PRAGMA:    break;
        default: ASSERT(0,("unknown tree type:%d",TREE_type(t)));
        }//end switch
        t = TREE_nsib(t);
    }//end while
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


/*
Checking compatible between formal parameter and real parameter.
'formalp': formal parameter
'realp': real parameter
*/
static bool ck_para_type_compatible(Decl * formalp, Decl * realp)
{
    UNUSED(realp);
    UNUSED(formalp);
    //TODO
    return true;
}


/*
Declaration checking
*/
static INT c_declaration_ck(Decl * d)
{
    ASSERT0(DECL_dt(d) == DCL_DECLARATION);
    Decl * dclor = get_pure_declarator(d);
    ASSERT0(dclor);
    while (dclor != NULL) {
        if (DECL_dt(dclor) == DCL_FUN) {
            Decl * ret_value_type = DECL_next(dclor);
            if (ret_value_type) {
                if (DECL_dt(ret_value_type) == DCL_FUN) {
                    err(g_real_line_num,
                        "return value type of function can not be a function");
                    return ST_ERR;
                }
                if (DECL_dt(ret_value_type) == DCL_ARRAY) {
                    err(g_real_line_num,
                        "return value type of function can not be an array");
                    return ST_ERR;
                }
            }
        }
        dclor = DECL_next(dclor);
    }
    return ST_SUCC;
}


static bool type_ck_call(Tree * t, TYCtx * cont)
{
    if (ST_SUCC != c_type_ck(TREE_para_list(t), cont)) return false;
    if (ST_SUCC != c_type_ck(TREE_fun_exp(t), cont)) return false;
    Decl * fun_decl = TREE_result_type(TREE_fun_exp(t));

    //Return type is the call type.
    //And here constructing return value type.
    //TypeSpec * ty = DECL_spec(fun_decl);
    Decl * pure_decl = PURE_DECL(fun_decl);
    if (DECL_dt(pure_decl) == DCL_FUN) {
        pure_decl = DECL_next(pure_decl);
    }

    //Do legality checking first for the return value type.
    if (pure_decl) {
        if (is_array(pure_decl)) {
            err(TREE_lineno(t), "function cannot returns array");
        }
        if (is_fun_decl(pure_decl)) {
            err(TREE_lineno(t), "function cannot returns function");
        }
    }

    //Check parameter list
    Decl * param_decl = get_parameter_list(TREE_result_type(TREE_fun_exp(t)));
    Tree * para = TREE_para_list(t);
    INT count = 0;
    if (param_decl != NULL) {
        while (param_decl != NULL && para != NULL) {
            count++;
            Decl * pld = TREE_result_type(para);
            if (!ck_para_type_compatible(param_decl, pld)) {
                err(TREE_lineno(t), "%dth parameter type incompatible", count);
                return false;
            }
            param_decl = DECL_next(param_decl);
            para = TREE_nsib(para);
            if (param_decl && DECL_dt(param_decl) == DCL_VARIABLE) {
                ASSERT(!DECL_next(param_decl),
                        ("DCL_VARIABLE must be last formal-parameter"));
                param_decl = NULL;
                para = NULL;
            }
        }
    }
    if (param_decl != NULL || para != NULL) {
        CHAR * name = NULL;
        if (TREE_type(TREE_fun_exp(t)) == TR_ID) {
            name = SYM_name(TREE_id(TREE_fun_exp(t)));
        }
        Decl * p = get_parameter_list(TREE_result_type(TREE_fun_exp(t)));
        UINT c = 0;
        while (p != NULL) {
            c++;
            p = DECL_next(p);
        }
        if (count == 0) {
            err(TREE_lineno(t),
                "function '%s' cannot take any parameter",
                name != NULL ? name : "");
        } else {
            err(TREE_lineno(t),
                "function '%s' should take %d parameters",
                name != NULL ? name : "", c);
        }
        return false;
    }
    return true;
}


//Perform type checking.
static INT c_type_ck(Tree * t, TYCtx * cont)
{
    if (cont == NULL) {
        TYCtx ct = {0};
        cont = &ct;
    }

    while (t != NULL) {
        g_src_line_num = TREE_lineno(t);
        switch (TREE_type(t)) {
        case TR_ASSIGN:
        case TR_ID:
        case TR_IMM:
        case TR_IMML:
        case TR_FP:  //3.1415926
        case TR_ENUM_CONST:
        case TR_STRING:
        case TR_LOGIC_OR:  //logical or       ||
        case TR_LOGIC_AND: //logical and      &&
        case TR_INCLUSIVE_OR: //inclusive or  |
        case TR_XOR: //exclusive or
        case TR_INCLUSIVE_AND: //inclusive and &
        case TR_SHIFT:   // >> <<
        case TR_EQUALITY: // == !=
        case TR_RELATION: // < > >= <=
        case TR_ADDITIVE: // '+' '-'
        case TR_MULTI:    // '*' '/' '%'
        case TR_SCOPE:
        case TR_IF:
        case TR_DO:
        case TR_WHILE:
        case TR_FOR:
        case TR_SWITCH:
        case TR_BREAK:
        case TR_CONTINUE:
        case TR_GOTO:
        case TR_LABEL:
        case TR_DEFAULT:
        case TR_CASE:
        case TR_RETURN:
        case TR_COND: //formulized log_OR_exp?exp:cond_exp
        case TR_CVT: //type convertion
        case TR_TYPE_NAME: //user defined type or C standard type
        case TR_LDA:   // &a get address of 'a'
        case TR_DEREF: //*p  dereferencing the pointer 'p'
        case TR_PLUS: // +123
        case TR_MINUS:  // -123
        case TR_REV:  // Reverse
        case TR_NOT:  // get non-value
        case TR_INC:   //++a
        case TR_POST_INC: //a++
        case TR_DEC:   //--a
        case TR_POST_DEC: //a--
        case TR_SIZEOF: // sizeof(a)
            break;
        case TR_CALL:
            if (!type_ck_call(t, cont)) {
                goto FAILED;
            }
            break;
        case TR_ARRAY:
        case TR_DMEM: // a.b
        case TR_INDMEM: // a->b
        case TR_PRAGMA:
            break;
        default:
            ASSERT(0, ("unknown tree type:%d", TREE_type(t)));
        } //end switch
        t = TREE_nsib(t);
    } //end while
    return ST_SUCC;
FAILED:
    return ST_ERR;
}


static void type_trans_init()
{
    g_schar_type = new_type(T_SPEC_SIGNED | T_SPEC_CHAR);
    g_sshort_type = new_type(T_SPEC_SIGNED | T_SPEC_SHORT);
    g_sint_type = new_type(T_SPEC_SIGNED | T_SPEC_INT);
    g_slong_type = new_type(T_SPEC_SIGNED | T_SPEC_LONG);
    g_slonglong_type = new_type(T_SPEC_SIGNED | T_SPEC_LONGLONG);

    g_uchar_type = new_type(T_SPEC_UNSIGNED | T_SPEC_CHAR);
    g_ushort_type = new_type(T_SPEC_UNSIGNED | T_SPEC_SHORT);
    g_uint_type = new_type(T_SPEC_UNSIGNED | T_SPEC_INT);
    g_ulong_type = new_type(T_SPEC_UNSIGNED | T_SPEC_LONG);
    g_ulonglong_type = new_type(T_SPEC_UNSIGNED | T_SPEC_LONGLONG);

    g_float_type = new_type(T_SPEC_FLOAT);
    g_double_type = new_type(T_SPEC_DOUBLE);
    g_void_type = new_type(T_SPEC_VOID);
    g_enum_type = new_type(T_SPEC_ENUM);
}


//Decl hash table
class DECL_HASH : public Hash<Decl*> {
public:
       UINT compute_hash_value(CHAR const* s) const
    {
        UINT v = 0 ;
        do {
            v += (UINT)(*s++);
        } while (*s != '\0');
        v %= m_bucket_size;
        return v;
    }

    UINT get_hash_value(Decl * decl) const
    {
        CHAR const* s = SYM_name(get_decl_sym(decl));
        return compute_hash_value(s);
    }

    bool compare(Decl * d1, Decl * d2) const
    { return is_decl_equal(d1, d2); }

};


//Infer type to Tree nodes.
INT type_trans()
{
    type_trans_init();
    SCOPE * s = get_global_scope();
    Decl * dcl = SCOPE_decl_list(s);
    while (dcl != NULL) {
        ASSERT0(DECL_decl_scope(dcl) == s);
        if (DECL_is_fun_def(dcl)) {
            Tree * stmt = SCOPE_stmt_list(DECL_fun_body(dcl));
            if (ST_SUCC != c_type_tran(stmt, NULL)) {
                return ST_ERR;
            }
            if (g_err_msg_list.get_elem_count() > 0) {
                return ST_ERR;
            }
        }
        dcl = DECL_next(dcl);
    }
    return ST_SUCC;
}


INT type_ck()
{
    SCOPE * s = get_global_scope();
    Decl * dcl = SCOPE_decl_list(s);
    INT st = ST_SUCC;
    while (dcl != NULL) {
        ASSERT0(DECL_decl_scope(dcl) == s);
        c_declaration_ck(dcl);
        if (DECL_is_fun_def(dcl)) {
            Tree * stmt = SCOPE_stmt_list(DECL_fun_body(dcl));
            if (c_type_ck(stmt, NULL) != ST_SUCC) {
                st = ST_ERR;
                break;
            }
            if (g_err_msg_list.get_elem_count() > 0) {
                st = ST_ERR;
                break;
            }
        }
        dcl = DECL_next(dcl);
    }
    tfree();
    return st;
}

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

author: Su Zhenyu
@*/
#include "../xgen/xgeninc.h"

bool ARMIRMgr::isIRIsomorphicExtOp(
    IR const* ir, IR const* src, bool is_cmp_kid,
    IsomoFlag const& flag) const
{
    return false;
}


IR * ARMIRMgr::buildConvOpndGrad(
    IR * input, IR * weight, IR * grad, Type const* type,
    CConvOpndGrad::OpndKind ok, UINT stride_h, UINT stride_w,
    StorageSpace ss)
{
    ASSERT0(input && weight && grad);
    ASSERT0(ok != CConvOpndGrad::OK_UNDEF);
    IR * ir = allocIR(IR_CONV_OPND_GRAD);
    CONVOPNDGRAD_input(ir) = input;
    CONVOPNDGRAD_weight(ir) = weight;
    CONVOPNDGRAD_grad(ir) = grad;
    CONVOPNDGRAD_stride_h(ir) = stride_h;
    CONVOPNDGRAD_stride_w(ir) = stride_w;
    CONVOPNDGRAD_opnd_kind(ir) = ok;
    IR_dt(ir) = type;
    IR_parent(input) = ir;
    IR_parent(weight) = ir;
    IR_parent(grad) = ir;
    CONVOPNDGRAD_storage_space(ir) = ss;
    return ir;
}


#ifdef FOR_IP
static bool isConvResTypeValid(IR const* input, IR const* weight,
                               Type const* resty, UINT stride_h, UINT stride_w)
{
    TenType const* t_input = nullptr;
    if (input->is_const()) { t_input = (TenType*)CONST_tensor_val(input); }
    else { t_input = CalcDerivative::getTensor(input); }
    if (t_input == nullptr) {
        //No enough information to do verification.
        return true;
    }
    TenType const* t_weight = CalcDerivative::getTensor(weight);
    if (weight->is_const()) { t_weight = (TenType*)CONST_tensor_val(weight); }
    else { t_weight = CalcDerivative::getTensor(weight); }
    if (t_weight == nullptr) {
        //No enough information to do verification.
        return true;
    }
    TensorType const* res_tty = (TensorType const*)resty;
    ASSERT0(res_tty->getDim() == 2); //TODO:
    UINT dimh, dimw;
    xoc::TenType::computeConvResSize(*t_input, *t_weight, stride_h, stride_w,
                                      dimh, dimw);
    ASSERT0(dimh == res_tty->getDegreeOfDim(0));
    ASSERT0(dimw == res_tty->getDegreeOfDim(1));
    return true;
}
#endif


IR * ARMIRMgr::buildConv(IR * input, IR * weight, Type const* type,
                         UINT stride_h, UINT stride_w, StorageSpace ss)
{
    ASSERT0(input && input->is_tensor());
    ASSERT0(weight && weight->is_tensor());
    #ifdef FOR_IP
    ASSERT0(isConvResTypeValid(input, weight, type, stride_h, stride_w));
    #endif
    IR * ir = allocIR(IR_CONV);
    CONV_input(ir) = input;
    CONV_weight(ir) = weight;
    CONV_stride_h(ir) = stride_h;
    CONV_stride_w(ir) = stride_w;
    IR_dt(ir) = type;
    IR_parent(input) = ir;
    IR_parent(weight) = ir;
    CONV_storage_space(ir) = ss;
    return ir;
}

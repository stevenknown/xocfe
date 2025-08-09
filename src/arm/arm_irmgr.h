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
#ifndef _ARM_IRMGR_H_
#define _ARM_IRMGR_H_

class ARMIRMgr : public IRMgrExt {
public:
    ARMIRMgr(Region * rg) : IRMgrExt(rg) {}
    virtual ~ARMIRMgr() {}

    //Build convolution operation.
    //input: the input operand with constant tensor type.
    //weight: the weight operand with constant tensor type.
    //stride_w: the stride on width-dimension of input.
    //stride_h: the stride on height-dimension of input.
    //  e.g:given input is tensor<2*3>, stride_w is 2, stride_h is 1,
    //  |----------> width
    //  | a1 a2 a3
    //  | a4 a5 a6
    //  V
    //  height
    //ss: the storage for both input and weight data.
    IR * buildConv(IR * input, IR * weight, Type const* type,
                   UINT stride_h, UINT stride_w, StorageSpace ss);
    IR * buildConv(IR * input, IR * weight, Type const* type)
    { return buildConv(input, weight, type, 1, 1, SS_UNDEF); }

    //Build convolution gradient operation.
    //input: the input operand with constant tensor type.
    //weight: the weight operand with constant tensor type.
    //stride_w: the stride on width-dimension of input.
    //stride_h: the stride on height-dimension of input.
    //dx: the DX variable.
    IR * buildConvOpndGrad(
        IR * input, IR * weight, IR * grad, Type const* type,
        CConvOpndGrad::OpndKind ok, UINT stride_h, UINT stride_w,
        StorageSpace ss);
    IR * buildConvOpndGrad(IR * input, IR * weight, IR * grad,
                           Type const* type, CConvOpndGrad::OpndKind ok)
    { return buildConvOpndGrad(input, weight, grad, type, ok, 1, 1, SS_UNDEF); }

    virtual bool isIRIsomorphicExtOp(
        IR const* ir, IR const* src, bool is_cmp_kid,
        IsomoFlag const& flag) const override;
};

#endif

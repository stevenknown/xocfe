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
#ifndef _ARM_DECL_EXT_H_
#define _ARM_DECL_EXT_H_

namespace xoc {

////////////////////////////////////////////////////////////////////////////////
//NOTE DECL-EXT IS ALREADY IN XOC NAMESPACE. USER CAN USE XOC DATA            //
//STRUCTURES DIRECTLY.                                                        //
////////////////////////////////////////////////////////////////////////////////

//The class represents convolution operation.
//The macro defines the input tensor operand.
#define CONV_input(ir) CONV_kid(ir, 0)

//The macro defines the weight tensor operand.
#define CONV_weight(ir) CONV_kid(ir, 1)

//The macro defines the accessing interface to CONV's kids.
#define CONV_kid(ir, idx) \
    (((CConv*)ir)->opnd[CK_KID_IRC(ir, IR_CONV, idx)])

//The macro defines the storage space of input and weight data.
//Note both input and weight data should be in same type storage space.
#define CONV_storage_space(ir) \
    (((CConv*)CK_IRC(ir, IR_CONV))->storage_space)

//These macros define strides on width and height directions of input data.
//  e.g:given input is tensor<2*3>, stride_w is 2, stride_h is 1,
//  |----------> width
//  | a1 a2 a3
//  | a4 a5 a6
//  V
//  height
#define CONV_stride_w(ir) \
    (((CConv*)CK_IRC(ir, IR_CONV))->stride_width)
#define CONV_stride_h(ir) \
    (((CConv*)CK_IRC(ir, IR_CONV))->stride_height)
class CConv : public IR {
    COPY_CONSTRUCTOR(CConv);
public:
    static BYTE const kid_map = 0x3;
    static BYTE const kid_num = 2;
    StorageSpace storage_space;
    UINT stride_width;
    UINT stride_height;
    IR * opnd[kid_num];
public:
    //Access Kids.
    static inline IR *& accKid(IR * ir, UINT idx) { return CONV_kid(ir, idx); }

    //Dump function.
    static void accDump(IR const* ir, Region const* rg, IRDumpCtx<> & ctx);

    //Access storage space.
    static inline StorageSpace & accSS(IR * ir)
    { return CONV_storage_space(ir); }

    IR * getKid(UINT idx) const { return CONV_kid(this, idx); }
};


//The class represents convolution gradient operation.
//The macro defines the input tensor operand.
#define CONVOPNDGRAD_input(ir) CONVOPNDGRAD_kid(ir, 0)

//The macro defines the weight tensor operand.
#define CONVOPNDGRAD_weight(ir) CONVOPNDGRAD_kid(ir, 1)

//The macro defines the gradient of CONV which is used to compute the
//gradient of operand in reverse mode.
#define CONVOPNDGRAD_grad(ir) CONVOPNDGRAD_kid(ir, 2)

//The macro defines the stride in width.
#define CONVOPNDGRAD_stride_w(ir) \
    (((CConvOpndGrad*)CK_IRC(ir, IR_CONV_OPND_GRAD))->stride_width)

//The macro defines the stride in height.
#define CONVOPNDGRAD_stride_h(ir) \
    (((CConvOpndGrad*)CK_IRC(ir, IR_CONV_OPND_GRAD))->stride_height)

//The macro defines the operand that corresponding to CONV.
#define CONVOPNDGRAD_opnd_kind(ir) \
    (((CConvOpndGrad*)CK_IRC(ir, IR_CONV_OPND_GRAD))->opnd_kind)

#define CONVOPNDGRAD_is_compute_input(ir) \
    (CONVOPNDGRAD_opnd_kind(ir) == CConvOpndGrad::OK_INPUT)

#define CONVOPNDGRAD_is_compute_weight(ir) \
    (CONVOPNDGRAD_opnd_kind(ir) == CConvOpndGrad::OK_WEIGHT)

//Storage Space.
#define CONVOPNDGRAD_storage_space(ir) \
    (((CConvOpndGrad*)CK_IRC(ir, IR_CONV_OPND_GRAD))->storage_space)

//The macro defines the accessing interface to CONVOPNDGRAD's kids.
#define CONVOPNDGRAD_kid(ir, idx) \
    (((CConvOpndGrad*)ir)->opnd[CK_KID_IRC(ir, IR_CONV_OPND_GRAD, idx)])

class CConvOpndGrad : public IR {
    COPY_CONSTRUCTOR(CConvOpndGrad);
public:
    typedef enum tagOpndType {
        OK_UNDEF = 0,
        OK_INPUT,
        OK_WEIGHT,
    } OpndKind;
public:
    static BYTE const kid_map = 0x7;
    static BYTE const kid_num = 3;
    OpndKind opnd_kind;
    StorageSpace storage_space;
    UINT stride_width;
    UINT stride_height;
    IR * opnd[kid_num];
public:
    //Access Kids.
    static inline IR *& accKid(IR * ir, UINT idx)
    { return CONVOPNDGRAD_kid(ir, idx); }

    //Dump function.
    static void accDump(IR const* ir, Region const* rg, IRDumpCtx<> & ctx);

    IR * getKid(UINT idx) const { return CONVOPNDGRAD_kid(this, idx); }
};

} //namespace xoc

#endif

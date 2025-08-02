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
//Defined the entry for extended ir code.
//In order to conform the compatibility of origin IR code, user can undef
//original SWITCH_CASE_<NAME>, then redefine the same MACRO with new IR code
//and followed by origin IR code.
#define SWITCH_CASE_ML_CODE \
    SWITCH_CASE_ML_EXT_UNA: \
    case IR_CONV: \
    case IR_CONV_OPND_GRAD

//Defined the entry for extended expression ir code.
#undef SWITCH_CASE_EXT_EXP
#define SWITCH_CASE_EXT_EXP \
    SWITCH_CASE_EXT_ATOM: \
    case IR_PHYREG: \
    case IR_BROADCAST: \
    SWITCH_CASE_ML_CODE

#undef SWITCH_CASE_EXT_UNA
#define SWITCH_CASE_EXT_UNA SWITCH_CASE_ML_EXT_UNA

//Defined the entry for extended expression unary code.
#define SWITCH_CASE_ML_EXT_UNA \
    case IR_RELU: \
    case IR_SOFTMAX: \
    case IR_SIGMOID: \
    case IR_TANH

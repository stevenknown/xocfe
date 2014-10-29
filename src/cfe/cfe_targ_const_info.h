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
#ifndef __CFE_TARG_CONST_INFO_H__
#define __CFE_TARG_CONST_INFO_H__

//Define machine word/half-word/byte/bit size
#define BIT_PER_BYTE         8
#define BYTE_PER_CHAR        1
#define BYTE_PER_SHORT       2
#define BYTE_PER_INT         4
#define BYTE_PER_LONG        4
#define BYTE_PER_LONGLONG    8
#define BYTE_PER_FLOAT       4
#define BYTE_PER_DOUBLE      8
#define BYTE_PER_ENUM        4
#define BYTE_PER_POINTER     4
#define GENERAL_REGISTER_SIZE (BYTE_PER_POINTER)


/*
Host integer and float point type.
e.g: Build XOCC on x8664, HOST_INT_TYPE should be 64bit.
Or build XOCC on ARM, HOST_INT_TYPE should be 32bit,
of course 64bit is ok if you want.
*/
#define HOST_INT_TYPE LONGLONG
#define HOST_FP_TYPE double


//Const float-point byte size.
#define BYTE_PER_CONST_FP		4

//Alignment with byte.
#define PRAGMA_ALIGN		1

/*
Setting for compiler build-environment.
Byte length.
*/
#define HOST_BITS_PER_BYTE		8

#endif

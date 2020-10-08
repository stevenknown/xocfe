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

//Represent target machine word with host type.
#ifndef TMWORD
#define TMWORD UINT
#endif

//Define machine word/half-word/byte/bit size
#ifndef BIT_PER_BYTE
#define BIT_PER_BYTE 8
#endif

#ifndef BYTE_PER_CHAR
#define BYTE_PER_CHAR 1
#endif

#ifndef BYTE_PER_SHORT
#define BYTE_PER_SHORT 2
#endif

#ifndef BYTE_PER_INT
#define BYTE_PER_INT 4
#endif

#ifndef BYTE_PER_LONG
#define BYTE_PER_LONG 4
#endif

#ifndef BYTE_PER_LONGLONG
#define BYTE_PER_LONGLONG 8
#endif

#ifndef BYTE_PER_FLOAT
#define BYTE_PER_FLOAT 4
#endif

#ifndef BYTE_PER_DOUBLE
#define BYTE_PER_DOUBLE 8
#endif

#ifndef BYTE_PER_ENUM
#define BYTE_PER_ENUM 4
#endif

#ifndef BYTE_PER_POINTER
#define BYTE_PER_POINTER 4
#endif

#ifndef GENERAL_REGISTER_SIZE
#define GENERAL_REGISTER_SIZE (BYTE_PER_POINTER)
#endif

//Define signed and unsigned integer type on host machine.
#define INT8 CHAR
#define UINT8 UCHAR
#define INT16 SHORT
#define UINT16 USHORT
#define INT32 INT
#define UINT32 UINT
#define INT64 LONGLONG
#define UINT64 ULONGLONG

//Host integer and float point type.
//e.g: Build XOCC on x8664, HOST_INT should be 64bit.
//Or build XOCC on ARM, HOST_INT should be 32bit,
//of course 64bit is ok if you want.
#ifndef HOST_INT
#define HOST_INT LONGLONG
#endif

//Host unsigned integer.
//e.g: Build XOCC on x8664, HOST_UINT should be 64bit.
//Or build XOCC on ARM, HOST_UINT should be 32bit,
//of course 64bit is ok if you want.
#ifndef HOST_UINT
#define HOST_UINT ULONGLONG
#endif

#ifndef HOST_FP
#define HOST_FP double
#endif

//Define target machine alignment with byte.
//This marco used by pragma directive.
#ifndef PRAGMA_ALIGN
#define PRAGMA_ALIGN 4
#endif

//Setting for compiler build-environment.
#ifndef HOST_BIT_PER_BYTE
#define HOST_BIT_PER_BYTE 8
#endif

#endif
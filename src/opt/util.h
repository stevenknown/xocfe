/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

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

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#ifndef __UTIL_H__
#define __UTIL_H__

namespace xoc {

class Type;
class TypeMgr;

class BYTEVec : public xcom::Vector<BYTE> {
    COPY_CONSTRUCTOR(BYTEVec);
public:
    BYTEVec() {}
    BYTEVec(UINT size) : xcom::Vector<BYTE>(size) {}
    void dump(Region const* rg) const;
};

//Dump host-machine integer value.
void dumpHostInteger(HOST_UINT intval, Type const* ty, Region const* rg,
                     TypeMgr const* tm, bool is_sign);

//Dump host-machine float-point value.
void dumpHostFP(HOST_FP fpval, Type const* ty, Region const* rg,
                TypeMgr const* tm);

//Report internal warning.
void interwarn(CHAR const* format, ...);

//Print message to console.
void prt2C(CHAR const* format, ...);

//Return signed integer placeholder string that used in format.
CHAR const* getHostIntFormat(bool hex);

//Return unsigned integer placeholder string that used in format.
CHAR const* getHostUIntFormat(bool hex);

//Return float-point placeholder string that used in format.
CHAR const* getHostFPFormat();

} //namespace xoc
#endif

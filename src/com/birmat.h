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
#ifndef __BIRMAT_H__
#define __BIRMAT_H__

namespace xcom {

class IMat;
class BIRMat;
class RMat;

///BIRational
BIRMat operator * (BIRMat const& a, BIRMat const& b);
BIRMat operator + (BIRMat const& a, BIRMat const& b);
BIRMat operator - (BIRMat const& a, BIRMat const& b);

class BIRMat : public Matrix<BIRational> {
    friend BIRMat operator * (BIRMat const& a, BIRMat const& b);
    friend BIRMat operator + (BIRMat const& a, BIRMat const& b);
    friend BIRMat operator - (BIRMat const& a, BIRMat const& b);
    friend class IMat;
    friend class RMat;
public:
    BIRMat();
    BIRMat(BigInt const& v); //used by template call of T(0) in Vector<Mat>
    BIRMat(BIRMat const& m);
    BIRMat(IMat const& m);
    BIRMat(RMat const& m);
    BIRMat(UINT row, UINT col);
    ~BIRMat();
    void init();
    void init(UINT row, UINT col);
    void init(BIRMat const& m);
    void init(RMat const& m);
    void init(IMat const& m);
    void destroy();
    bool is_imat(UINT * row = nullptr, UINT * col = nullptr);
    void sete(UINT num, ...);
    void setr(UINT row, UINT col, BigInt const& numer, BigInt const& denom = 1);
    void setr(UINT row, UINT col, BIRational const& rat);
    //Set value to numerator and denomiator.
    void setr(UINT row, UINT col, Rational::FType numer,
              Rational::FType denom);
    void getr(UINT row, UINT col, BigInt * numer, BigInt * denom);
    BIRational getr(UINT row, UINT col);
    void ds(BIRMat const& c);
    void copy(BIRMat const& r);
    void copy(IMat const& r);
    void copy(RMat const& m);
    UINT comden(UINT row, UINT col); //Common denominator
    void substit(BIRMat const& exp,
                 UINT v,
                 bool is_eq = true,
                 INT cst_col = CST_COL_UNDEF);
    void intlize(INT row = -1); //Converting rational element to integer.
    BIRMat & operator = (BIRMat const& m);
    BIRational reduce(UINT row, UINT col);
    void reduce();
};


class BIRMatMgr : public MatMgr<BIRational> {
public:
    virtual Matrix<BIRational> * allocMat() override
    { return new BIRMat(); }
    virtual Matrix<BIRational> * allocMat(UINT row, UINT col) override
    { return new BIRMat(row, col); }
};

} //namespace xcom
#endif

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

#ifndef __LIN_SYS_H_
#define __LIN_SYS_H_

namespace xcom {

//
//START ZPoly
//
//e.g: Integer Polyhedron Z=Q��L
//    domain  Q = {i,j|0<=i<=5, 0<=3j<=20}
//    lattice L = {i,j|2i+1, 3j+5|i,j��Z}
class ZPoly {
    RMat m_domain;
    IMat m_lattice; //an affine, invertible matrix.
public:

};
//END ZPoly


//
//START Lineq
//
//System of Linear Inequalities and Equalities.
#define CST_UNK 1 //unknown result of comparation of const term
#define CST_LT 2 //less than
#define CST_GT 3 //great than
#define CST_EQ 4 //equal to

class Lineq {
protected:
    BYTE m_is_init:1; //To make sure functions are idempotent.
    BYTE m_is_dump:1;

    //Index of right-hand-side, also the column index of constant coefficient
    //vector, start from zero, named by Mathematical Programming System.
    //If 'cst_col' does not equal to 'm_col_size - 1', it means,
    //each column from 'cst_col + 1' to 'm_col_size -1' represent one
    //constant symbol.
    //e.g: x+y <= 10 + M + N, where M, N represent constant symbols
    //respectively.
    //Default value is -1, indicate last column is constant vector.
    INT m_cst_col;

    //Record coeff of inequality: Ax <= b+C(x), where C(x) is function of
    //symbolic constant.
    RMat * m_coeff;
    RMatMgr m_rmmgr;
    IMatMgr m_immgr;
protected:
    INT compareConstIterm(RMat const& m, UINT cst_col, INT idx_of_eqt1,
                          Rational v);
    INT compareConstIterm(RMat const& m, UINT cst_col, INT idx_of_eqt1,
                          INT idx_of_eqt2);
    void combine(OUT IMat & res, IMat const& coeff,
                 UINT nc, UINT pc, UINT lc, UINT pos);
    void combineRays(OUT IMat & res, MOD IMat & coeff,
                     UINT r1, UINT r2, UINT lc, UINT pos);

    RMatMgr & getRMatMgr() { return m_rmmgr; }
    IMatMgr & getIMatMgr() { return m_immgr; }

    bool omit(IMat const& coeff, UINT ncv, UINT pcv, UINT rhs_part,
              Vector<UINT> const& combined, Vector<UINT> const& noneg);

    void removeRedRow(MOD IMat & cs, IMat const& org_cone, UINT rhs_part);

    INT selectLeadingColumn(IMat const& coeff, Vector<bool> const& is_noneg,
                            UINT rhs_part);
public:
    Lineq(RMat * m, INT cst_col = CST_COL_UNDEF);
    ~Lineq();

    void appendEquation(RMat const& eq);

    bool convertRay2Constraint(
        IMat const& gmat, OUT IMat & cs, UINT cslimit = 100);
    bool convertConstraint2Ray(
        OUT IMat & gmat, IMat const& cs, UINT cst_col,
        UINT raylimit = 1000);
    bool calcBound(MOD List<RMat*> & limits);
    void ConvexHullUnionAndIntersect(
        OUT RMat & res, IN List<RMat*> & chulls, UINT cst_col,
        bool is_intersect);

    //Dumps variable, forms as
    //  ak*xk <= const + F(x) + a0x0 + a1x1 + ... + a(k-1)x(k-1) +
    //           a(k+1)x(k+1) + ... + anxn.
    void dumps_var_bound(UINT u);
    void destroy();

    void EhartPoly(OUT RMat & res, IN RMat & a, UINT cst_col);

    //Represent variable, forms as
    //    ak*xk <= const + F(x) + a0x0 + a1x1 + ... + a(k-1)x(k-1) +
    //             a(k+1)x(k+1) + ... + anxn.
    void formatBound(UINT u, OUT RMat & ineqt_of_u);

    //Fourier-Motzkin elimination
    bool fme(UINT const u, OUT RMat & res, bool const darkshadow = false);

    bool is_consistent();
    void init(RMat * m, INT cst_col = CST_COL_UNDEF);
    void initVarConstraint(Vector<INT> const& sign, MOD RMat & vc,
                           UINT cst_col);

    bool has_solution(RMat const& leq, RMat const& eq, RMat const& vc,
                      UINT cst_col, bool is_int_sol, bool is_unique_sol);
    void move2cstsym(IN RMat & ieq, UINT cst_col, UINT first_var, UINT last_var,
                     OUT UINT * first_sym_idx, OUT UINT * last_sym_idx);
    void move2var(IN RMat & ieq, UINT cst_col, UINT first_sym, UINT last_sym,
                  OUT UINT * first_var_idx, OUT UINT * last_var_idx);

    //Polyhedra operation
    void PolyDiff(OUT RMat & res, IN RMat & a, IN RMat & b, UINT cst_col);
    void PolyImage(OUT RMat & res, IN RMat & a, UINT cst_col);

    bool reduce(MOD RMat & m, UINT cst_col, bool is_intersect);
    void removeIdenRow(MOD RMat & m);

    //Set index of const column and coeff matrix.
    void setParam(RMat * m, INT cst_col = CST_COL_UNDEF);
    void set_dump(bool is_dump) { m_is_dump = is_dump; }
    void substituteAndExpand(MOD RMat & coeff, UINT cst_col, RMat const& p,
                             UINT sub_var);
};

} //namespace xcom
#endif

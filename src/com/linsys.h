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

#ifndef __LIN_SYS_H_
#define __LIN_SYS_H_

//
//START ZPOLY
//
/*
e.g: Integer Polyhedron Z=Q¡ÉL
	domain  Q = {i,j|0<=i<=5, 0<=3j<=20}
	lattice L = {i,j|2i+1, 3j+5|i,j¡ÊZ}
*/
class ZPOLY {
	RMAT m_domain;
	INTMAT m_lattice; //an affine, invertible matrix.
public:
	
};
//END ZPOLY



//
//START LINEQ
//
/*
System of Linear Inequalities and Equalities.
*/
#define CST_UNK		1 //unknown result of comparation of const term 
#define CST_LT		2 //less than
#define CST_GT		3 //great than
#define CST_EQ		4 //equal to

class LINEQ {
	bool m_is_init;

	/*
	Index of right-hand-side, also the column index of constant coefficient
	vector, start from zero, named by Mathematical Programming System.
	If 'rhs_idx' does not equal to 'm_col_size - 1', it means, 
	each column from 'rhs_idx + 1' to 'm_col_size -1' represent one 
	constant symbol.
	e.g: x+y <= 10 + M + N, where M, N represent constant symbols respectively.
	Default value is -1, indicate last column is constant vector.	
	*/
	INT m_rhs_idx; 

	/*
	Record coeff of inequality: Ax <= b+C(x), where C(x) is function of 
	symbolic constant.
	*/
	RMAT * m_coeff; 
	INT _cmp_const_term(RMAT const& m, UINT rhs_idx,
						INT idx_of_eqt1, RATIONAL v);
	INT _cmp_const_term(RMAT const& m, UINT rhs_idx,
						INT idx_of_eqt1, INT idx_of_eqt2);	
	INT _select_leading_column(IN INTMAT const& coeff, 
							IN SVECTOR<bool> const& is_noneg, 
							UINT rhs_part);
	void _combine(OUT INTMAT & res, IN INTMAT const& coeff, 
					UINT nc, UINT pc, UINT lc, UINT pos);
	void _combine_rays(OUT INTMAT & res, IN OUT INTMAT & coeff, 
						UINT r1, UINT r2, UINT lc, UINT pos);
	bool _omit(IN INTMAT const& coeff, 
				UINT ncv, 
				UINT pcv, 
				UINT rhs_part, 
				IN SVECTOR<UINT> const& combined,
				IN SVECTOR<UINT> const& noneg);
	void _rm_red_row(IN OUT INTMAT & cs, 
					IN INTMAT const& org_cone, 
					UINT rhs_part);
public:
	LINEQ(RMAT * m, INT rhs_idx = -1);
	~LINEQ();	
	void init(RMAT * m, INT rhs_idx = -1);
	void destroy();

	void append_eq(IN RMAT const& eq);	

	//Set index of const column and coeff matrix.
	void set_param(RMAT * m, INT rhs_idx = -1); 
	bool reduce(IN OUT RMAT & m, UINT rhs_idx, bool is_intersect);
	void convex_hull_union_intersect(OUT RMAT & res,
									IN LIST<RMAT*> & chulls,
									IN UINT rhs_idx,
									bool is_intersect);

	//Fourier-Motzkin elimination
	bool fme(IN UINT const u, OUT RMAT & res, 
			IN bool const darkshadow = false); 
	bool is_consistent();
	bool has_solution(IN RMAT const& leq, 
					IN RMAT const& eq, 
					IN OUT RMAT & vc, 
					UINT rhs_idx, 
					bool is_int_sol, 
					bool is_unique_sol);
	void init_vc(IN SVECTOR<INT> const* sign, 
				IN OUT RMAT & vc, UINT rhs_idx);
	void substi_and_expand(IN OUT RMAT & coeff, 
						IN UINT rhs_idx, IN RMAT const& p, 
						UINT sub_var);
	/*
	Represent variable, forms as 
		ak*xk <= const + F(x) + a0x0 + a1x1 + ... + a(k-1)x(k-1) + 
					a(k+1)x(k+1) + ... + anxn.
	*/
	void format_bound(IN UINT u, OUT RMAT & ineqt_of_u); 
	bool calc_bound(IN OUT LIST<RMAT*> & limits);

	void move2cstsym(IN RMAT & ieq, UINT rhs_idx, 
					UINT first_var, UINT last_var,
					OUT UINT * first_sym_idx, OUT UINT * last_sym_idx);
	void move2var(IN RMAT & ieq, UINT rhs_idx, 
					UINT first_sym, UINT last_sym,
					OUT UINT * first_var_idx, OUT UINT * last_var_idx);
	void remove_iden_row(IN OUT RMAT & m);

	//Polyhedra operation
	bool cvt_cs2ray(OUT INTMAT & gmat, IN INTMAT const& cs, 
					IN UINT rhs_idx, UINT raylimit = 1000);
	bool cvt_ray2cs(IN INTMAT const& gmat, 
					OUT INTMAT & cs, UINT cslimit = 100);
	void poly_diff(OUT RMAT & res, IN RMAT & a, IN RMAT & b, IN UINT rhs_idx);
	void poly_image(OUT RMAT & res, IN RMAT & a, IN UINT rhs_idx);
	void ehart_poly(OUT RMAT & res, IN RMAT & a, IN UINT rhs_idx);

	/*
	Dumps variable, forms as 
		ak*xk <= const + F(x) + a0x0 + a1x1 + ... + a(k-1)x(k-1) + 
					a(k+1)x(k+1) + ... + anxn.
	*/
	void dumps_var_bound(UINT u); 

};
#endif


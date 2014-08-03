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
#ifndef __XMAT_H__
#define __XMAT_H__

class INTMAT;
class RMAT;
class FLMAT;

///Rational
RMAT operator * (RMAT const& a, RMAT const& b);
RMAT operator + (RMAT const& a, RMAT const& b);
RMAT operator - (RMAT const& a, RMAT const& b);

class RMAT : public MATRIX<RATIONAL> {
	friend RMAT operator * (RMAT const& a, RMAT const& b);
	friend RMAT operator + (RMAT const& a, RMAT const& b);
	friend RMAT operator - (RMAT const& a, RMAT const& b);
	friend class INTMAT;
	bool m_is_init;

	void _init_hook();
public:
	RMAT();
	RMAT(FRAC_TYPE v); //used by template call of T(0) in SVECTOR<MAT>
	RMAT(RMAT const& m);
	RMAT(INTMAT const& m);
	RMAT(UINT row, UINT col);
	~RMAT();
	void init();
	void init(UINT row, UINT col);
	void init(RMAT const& m);
	void init(INTMAT const& m);
	void destroy();
	bool is_init() const { return m_is_init; }
	bool is_imat(UINT * row = NULL, UINT * col = NULL);
	void sete(UINT num,...);  
	void setr(UINT row, UINT col, FRAC_TYPE numer, FRAC_TYPE denom = 1);
	void setr(UINT row, UINT col, RATIONAL rat);
	void getr(UINT row, UINT col, FRAC_TYPE * numer, FRAC_TYPE * denom);
	RATIONAL getr(UINT row, UINT col);
	bool inv(RMAT & e);
	void ds(IN RMAT const& c);
	void copy(RMAT const& r);
	void copy(INTMAT const& r);
	UINT comden(UINT row, UINT col); //Common denominator
	void substit(IN RMAT const& exp, 
				IN UINT v, 
				bool is_eq = true, 
				INT rhs_idx = -1);
	void intliz(INT row = -1); //Converting rational element to integer.
	RMAT & operator = (RMAT const& m);
	RATIONAL reduce(UINT row, UINT col);
	void reduce();
};


///Integer
INTMAT operator * (INTMAT const& a, INTMAT const& b);
INTMAT operator + (INTMAT const& a, INTMAT const& b);
INTMAT operator - (INTMAT const& a, INTMAT const& b);

class INTMAT : public MATRIX<INT> {
	friend class RMAT;
	bool m_is_init;
	void _verify_hnf(INTMAT &h);
	friend INTMAT operator * (INTMAT const& a, INTMAT const& b);
	friend INTMAT operator + (INTMAT const& a, INTMAT const& b);
	friend INTMAT operator - (INTMAT const& a, INTMAT const& b);
public:
	INTMAT();

	//Used by template call of T(0) in SVECTOR<MAT>.
	INTMAT(INT v); 
	INTMAT(UINT row, UINT col);
	~INTMAT();
	void init();
	void destroy();
	bool is_init() const { return m_is_init; }

	//Set entry value one by one, 'num' indicate entry number.
	void sete(UINT num,...);  

	//Invering of Integer Matrix will be transformed 
	//to Rational Matrix, and one exception will be thrown 
	//if there are some element's denomiator is not '1'.
	bool inv(OUT INTMAT &e);
	INT det();

	//Generate unimodular matrix to elimnate element.
	void gen_elim_mat(IN UINT row, IN UINT col, OUT INTMAT &elim); 
	
	//Hermite Normal Form decomposition.
	void hnf(OUT INTMAT &h, OUT INTMAT &u); 

	//Reduce matrix by GCD operation.
	void gcd();

	//Find convex hull of a set of points.
	void cvexhull(OUT INTMAT &hull); 
	INTMAT & operator = (INTMAT const& m);
	void copy(RMAT const& r);
	void dumpf(CHAR const* name = NULL, bool is_del = false) const; 
	void dumps() const;
};


///Float
#define DEFAULT_SD			6
FLMAT operator * (FLMAT const& a, FLMAT const& b);
FLMAT operator - (FLMAT const& a, FLMAT const& b);
float flt_fast_sqrt(float n);
class FLMAT : public MATRIX<FLTY> {
	bool m_is_init;
	CHAR * m_sd_str; //Descripte significant digit string.
public:
	FLMAT();
	FLMAT(INT v); //used by template call of T(0) in SVECTOR<MAT>
	FLMAT(UINT row, UINT col);
	~FLMAT();
	void init();
	void destroy();
	bool is_init() const { return m_is_init; }
	void sete(UINT num,...);
	void setie(UINT num, ...);
	FLMAT& operator = (FLMAT const& m);
	void set_sd(UINT sd); //Redefine the significant digit.
	void substit(IN FLMAT const& exp, IN UINT v, bool is_eq, INT rhs_idx);
	bool is_imat(UINT * row, UINT * col);

	//Get the significant digit description string.
	CHAR const* get_sd() const; 
	FLTY reduce(UINT row, UINT col) { return get(row, col); }
	void reduce() {}
};


///Boolean
class BMAT : public MATRIX<bool> {
	bool m_is_init;
public:
	BMAT();
	BMAT(INT v); //used by template call of T(0) in SVECTOR<MAT>
	BMAT(UINT row, UINT col);
	~BMAT();
	void init();
	void destroy();
	bool is_init() const { return m_is_init; }

	//Set entry value one by one, 'num' indicate entry number.
	void sete(UINT num, ...);  
	BMAT & operator = (BMAT const& m);
};
#endif

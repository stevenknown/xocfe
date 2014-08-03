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
#include "ltype.h"
#include "comf.h"
#include "math.h"
#include "smempool.h"
#include "rational.h"
#include "flty.h"
#include "sstl.h"
#include "matt.h"
#include "xmat.h"


MATRIX<RATIONAL> operator * (MATRIX<RATIONAL> const& a, MATRIX<RATIONAL> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size > 0 && a.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(b.m_row_size > 0 && b.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(a.m_col_size == b.m_row_size, ("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL,
			("strange matrix member function"));
	MATRIX<RATIONAL> c(a.m_row_size, b.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < b.m_col_size; j++) {
			RATIONAL tmp = 0;
			for (UINT k = 0; k < a.m_col_size; k++) {
				tmp = tmp + a.get(i,k) * b.get(k,j);
			}
			c.set(i,j,tmp);	
		}
	}
	return c;
}


MATRIX<RATIONAL> operator + (MATRIX<RATIONAL> const& a, MATRIX<RATIONAL> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<RATIONAL> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) + b.get(i,j));	
		}
	}
	return c;
}


MATRIX<RATIONAL> operator - (MATRIX<RATIONAL> const& a, MATRIX<RATIONAL> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<RATIONAL> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) - b.get(i,j));
		}
	}
	return c;
}


MATRIX<INT> operator * (MATRIX<INT> const& a, MATRIX<INT> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size > 0 && a.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(b.m_row_size > 0 && b.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(a.m_col_size == b.m_row_size, ("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL,
			("strange matrix member function"));
	MATRIX<INT> c(a.m_row_size, b.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < b.m_col_size; j++) {
			INT tmp = 0;
			for (UINT k = 0; k < a.m_col_size; k++) {
				tmp = tmp + a.get(i,k) * b.get(k,j);
			}
			c.set(i,j,tmp);	
		}
	}
	return c;
}


MATRIX<INT> operator + (MATRIX<INT> const& a, MATRIX<INT> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<INT> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) + b.get(i,j));	
		}
	}
	return c;
}


MATRIX<INT> operator - (MATRIX<INT> const& a, MATRIX<INT> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<INT> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) - b.get(i,j));
		}
	}
	return c;
}


MATRIX<PRECISION_TYPE> operator * (MATRIX<PRECISION_TYPE> const& a, MATRIX<PRECISION_TYPE> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size > 0 && a.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(b.m_row_size > 0 && b.m_col_size > 0, ("invalid matrix"));
	IS_TRUE(a.m_col_size == b.m_row_size, ("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL,
			("strange matrix member function"));
	MATRIX<PRECISION_TYPE> c(a.m_row_size, b.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < b.m_col_size; j++) {
			PRECISION_TYPE tmp = 0;
			for (UINT k = 0; k < a.m_col_size; k++) {
				tmp = tmp + a.get(i,k) * b.get(k,j);
			}
			c.set(i,j,tmp);	
		}
	}
	return c;
}


MATRIX<PRECISION_TYPE> operator + (MATRIX<PRECISION_TYPE> const& a, MATRIX<PRECISION_TYPE> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<PRECISION_TYPE> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) + b.get(i,j));	
		}
	}
	return c;
}


MATRIX<PRECISION_TYPE> operator - (MATRIX<PRECISION_TYPE> const& a, MATRIX<PRECISION_TYPE> const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, ("not yet initialize."));
	IS_TRUE(a.m_row_size == b.m_row_size && a.m_col_size == b.m_col_size, 
			("invalid matrix type of mul"));
	IS_TRUE(a.HOOK_INIT == b.HOOK_INIT && a.HOOK_EQUAL == b.HOOK_EQUAL, 
			("strange matrix member function"));
	MATRIX<PRECISION_TYPE> c(a.m_row_size, a.m_col_size, &a.m_inhr);
	for (UINT i = 0; i < a.m_row_size; i++) {
		for (UINT j = 0; j < a.m_col_size; j++) {
			c.set(i,j,a.get(i,j) - b.get(i,j));
		}
	}
	return c;
}


//
//Rational Matrix
//
static void init_den(MATRIX<RATIONAL> * pbasis)
{	
	RATIONAL v(0);
	for (UINT i = 0; i < pbasis->get_row_size(); i++) {
		for (UINT j = 0; j < pbasis->get_col_size(); j++) {
			pbasis->set(i, j, v);
		}
	}
}


//Rational square root.
static RATIONAL rat_sqrt(RATIONAL v)
{
	IS_TRUE(0, ("NYI"));
	return 0;
}


static void rmat_dumpf_by_handle(void const* pbasis, FILE * h)
{
	IS_TRUE(h, ("dump file handle is NULL"));
	RMAT * pthis = (RMAT*)pbasis;
	fprintf(h, "\nMATRIX(%d,%d)\n", pthis->get_row_size(), pthis->get_col_size());	
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			RATIONAL rat = pthis->get(i, j);
			CHAR const* blank = "      ";
			if (rat.den() == 1) {
				fprintf(h, "%5d%s", (INT)rat.num(), blank);
			} else if (rat.den() == -1) {
				fprintf(h, "%5d%s", -((INT)rat.num()), blank);	
			} else if (rat.num() == 0) {
				if (rat.den() == 1) {
					fprintf(h, "%5d %s", 0, blank);	
				} else {
					fprintf(h, "%5d/%-5d", (INT)rat.num(), (INT)rat.den());	
				}
			} else {
				fprintf(h, "%5d/%-5d", (INT)rat.num(), (INT)rat.den());	
			}
		}	
		fprintf(h, "\n");	
	}
	fprintf(h, "\n");	
}


static void rmat_dumpf(void const* pbasis, CHAR const* name, bool is_del)
{
	if (name == NULL) {
		name = "matrix.tmp";
	}
	if (is_del) {
		unlink(name);
	}

	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));
	rmat_dumpf_by_handle(pbasis, h);
	fclose(h);
}


#define PRT_COMMA ","
static void rmat_dumps(void const* pbasis)
{
	printf("\n");	
	RMAT * pthis = (RMAT*)pbasis;
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			RATIONAL rat = pthis->get(i, j);
			CHAR const* blank = "      ";
			if (rat.den() == 1) {
				printf("%5d%s%s", (INT)rat.num(), PRT_COMMA, blank);
			} else if (rat.den() == -1) {
				printf("%5d%s%s", 
							-((INT)rat.num()), PRT_COMMA, blank);	
			} else if (rat.num() == 0) {
				if (rat.den() == 1) {
					printf("%5d %s%s", 0, PRT_COMMA, blank);	
				} else {
					printf("%5d/%-5d%s", 
							  (INT)rat.num(), (INT)rat.den(), PRT_COMMA);
				}
			} else {
				printf("%5d/%-5d%s", 
						  (INT)rat.num(), (INT)rat.den(), PRT_COMMA);	
			}
		}	
		printf("\n");	
	}
	printf("\n");
}


RMAT::RMAT()
{
	m_is_init = false;
	init();
}


//used by template call of T(0) in SVECTOR<MAT>
RMAT::RMAT(FRAC_TYPE v)
{
	RMAT();
}


RMAT::RMAT(RMAT const& m) : MATRIX<RATIONAL>(m)
{
	m_is_init = true;
	_init_hook();
}


RMAT::RMAT(INTMAT const& m) //DO NOT explicitly initialize the base-class copy-constructor.
{
	m_is_init = false;
	init(m);
}


RMAT::RMAT(UINT row, UINT col) : MATRIX<RATIONAL>(row, col)
{
	m_is_init = true;
	_init_hook();	
	init_den(this);
}


RMAT::~RMAT()
{
	destroy();
}


void RMAT::_init_hook()
{
	INHR i; memset(&i, 0, sizeof(INHR)); 
	i.hi = init_den;
	i.hs = rat_sqrt;
	i.hds = rmat_dumps;
	i.hdf = rmat_dumpf;
	i.hdfh = rmat_dumpf_by_handle;
	set_hook(&i);
}


void RMAT::init()
{
	if (m_is_init) return;
	((MATRIX<RATIONAL>*)this)->init(); //Do initialization of base class here.
	m_is_init = true;
	_init_hook();	
	init_den(this);
}


void RMAT::init(UINT row, UINT col)
{
	if (m_is_init) return;
	((MATRIX<RATIONAL>*)this)->init(row, col);
	m_is_init = true;
	_init_hook();	
	init_den(this);
}


void RMAT::init(RMAT const& m)
{
	if (m_is_init) return;
	((MATRIX<RATIONAL>*)this)->init();
	m_is_init = true;
	copy(m);
	_init_hook();	
}


void RMAT::init(INTMAT const& m)
{
	if (m_is_init) return;
	((MATRIX<RATIONAL>*)this)->init();
	m_is_init = true;
	copy(m);
	_init_hook();	
}


void RMAT::destroy()
{
	if (!m_is_init) return;
	MATRIX<RATIONAL>::destroy();
	m_is_init = false;
}


void RMAT::getr(UINT row, UINT col, FRAC_TYPE * numer, FRAC_TYPE * denom)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	RATIONAL rat = MATRIX<RATIONAL>::get(row, col);
	*numer = rat.num();
	*denom = rat.den();
}


RATIONAL RMAT::getr(UINT row, UINT col)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	return MATRIX<RATIONAL>::get(row, col);
}


void RMAT::sete(UINT num, ...)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(num <= m_col_size*m_row_size, ("set out of boundary."));
	if (num <= 0) {
		return;
	}
	INT * ptr = (INT*) (((BYTE*)(&num)) + sizeof(UINT));	
	UINT i = 0;
	UINT row = 0, col = 0;
	while (i < num) {
		INT numer = *ptr;
		set(row, col++, numer);
		if (col >= m_col_size) {
			row++;
			col = 0;
		}
		i++;
		ptr++; //stack growing down
	}
}


//Set value to numerator and denomiator.
void RMAT::setr(UINT row, UINT col, FRAC_TYPE numer, FRAC_TYPE denom)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(denom != 0, ("denominator is 0!"));
	RATIONAL rat;
	rat.num() = numer;
	rat.den() = denom;
	MATRIX<RATIONAL>::set(row, col, rat);
}


void RMAT::setr(UINT row, UINT col, RATIONAL rat)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(rat.den() != 0, ("denominator is 0!"));
	MATRIX<RATIONAL>::set(row, col, rat);
}


RMAT & RMAT::operator = (RMAT const& m)
{
	((MATRIX<RATIONAL>*)this)->copy(*((MATRIX<RATIONAL>*)&m));
	return *this;
}


void RMAT::copy(RMAT const& r)
{
	IS_TRUE(m_is_init && r.m_is_init, ("not yet initialize."));
	MATRIX<RATIONAL>::copy(*((MATRIX<RATIONAL>*)&r));
}


//Copy elements in Integer Matrix.
void RMAT::copy(INTMAT const& m)
{
	IS_TRUE(m_is_init && m.m_is_init, ("not yet initialize."));
	if (this == (RMAT*)&m) return;
	if (m_mat) {
		::free(m_mat);
	}
	m_row_size = m.m_row_size;
	m_col_size = m.m_col_size;
	m_mat = (RATIONAL*)::malloc(m_row_size * m_col_size * sizeof(RATIONAL));
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			setr(i, j, m.get(i, j), 1);
		}
	}
}


//Return true if matrix is nonsingular, otherwise return false.
bool RMAT::inv(RMAT & e)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	bool is_nonsingular = MATRIX<RATIONAL>::inv(e);
	//for (UINT i = 0; i < m_row_size; i++) {
	//	for (UINT j = 0; j < m_col_size; j++) {
	//		set(i, j, reduce(get(i,j)));			
	//	}
	//}
	return is_nonsingular;
}


/*
Reduction to a common denominator.
Return the common denominator.

'row': Row to reduce
'col': The starting column to reduce.
*/
UINT RMAT::comden(UINT row, UINT col) 
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(row < m_row_size, ("out of boundary"));

	bool is_int = true; //All elem is integer.
	INT lcm  = 1;
	for (UINT j= col; j < m_col_size; j++) {
			RATIONAL v = get(row, j);
			IS_TRUE(v.den() > 0, 
						("should be converted positive in rational file"));
			if (v.num() == 0) {
				continue;
			}
			if (v.den() != 1) {
				lcm = slcm(lcm, v.den());
				is_int = false;
			}	
	} //end for	

	if (!is_int) {
		for (UINT j= col; j < m_col_size; j++) {
			RATIONAL v = get(row, j);
			if (v.den() != lcm) {
				INT num = v.num() * (lcm / v.den());
				IS_TRUE(num < 0x7FFFffff, ("out of boundary"));
				setr(row, j, num, lcm);
			}	
		}	
	} //end if	
	return lcm;
}


/*
Substituting variable 'v' with expression 'exp'.

'exp': system of equations
'v': varible id, the column number.
'is_eq': set to true indicate that each rows of matrix represents
	an equations, or the false value only represent a right-hand of 
	a linear function, 
	e.g: if 'is_eq' is false, matrix is a vector, [1, 7, -2, -10], that 
	represents the right-hand of 
		f(x) = x1 + 7x2 - 2x3 - 10, 
	and if 'is_eq' is true, matrix repesented an equation, 
		x1 + 7x2 - 2x3  = -10 
*/
void RMAT::substit(IN RMAT const& exp, IN UINT v, bool is_eq, INT rhs_idx)
{
	IS_TRUE(m_is_init && exp.m_is_init, 
			("not yet initialize."));
	IS_TRUE(m_col_size == exp.m_col_size && v < m_col_size && 
			exp.is_rowvec(), ("unmatch matrix"));	
	
	if (!is_eq) {
		IS_TRUE0(rhs_idx >= 1 && rhs_idx < (INT)m_col_size);
		mul_of_cols(rhs_idx, m_col_size - 1, -1);
	}
	for (UINT i = 0; i < m_row_size; i++) {
		if (get(i, v) != 0) {
			RMAT tmp = exp;
			if (tmp.get(0, v) == 0) {
				continue;
			}
			if (get(i, v) != tmp.get(0, v)) {
				tmp.mul(-get(i, v)/tmp.get(0, v));
			} else {
				tmp.mul(-1);
			}
			add_row_to_row(tmp, 0, i);
		}
	}
	if (!is_eq) {
		mul_of_cols(rhs_idx, m_col_size - 1, -1);
	}
}


//elements all be integer
bool RMAT::is_imat(UINT * row, UINT * col)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			if (get(i, j).den() != 1) {
				if (row) *row = i;
				if (col) *col = j;
				return false;
			}
		}
	}
	return true;
}


RATIONAL RMAT::reduce(UINT row, UINT col)
{
	RATIONAL v = get(row, col);
	v.reduce();
	set(row, col, v);
	return v;
}


void RMAT::reduce()
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			RATIONAL v = get(i, j);
			v.reduce();
			set(i, j, v);
		}
	}
}


RMAT operator * (RMAT const& a, RMAT const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, 
			("not yet initialize."));
	RMAT c;	
	MATRIX<RATIONAL> * cp = (MATRIX<RATIONAL>*)&c;
	MATRIX<RATIONAL> const* ap = (MATRIX<RATIONAL> const*)&a;
	MATRIX<RATIONAL> const* bp = (MATRIX<RATIONAL> const*)&b;
	*cp = *ap * *bp;
	return c;
}


RMAT operator + (RMAT const& a, RMAT const& b)
{
	IS_TRUE(a.m_is_init && b.m_is_init, 
			("not yet initialize."));
	RMAT c;
	MATRIX<RATIONAL> *cp = (MATRIX<RATIONAL>*)&c;
	MATRIX<RATIONAL> *ap = (MATRIX<RATIONAL>*)&a;
	MATRIX<RATIONAL> *bp = (MATRIX<RATIONAL>*)&b;
	*cp = *ap + *bp;
	return c;
}


RMAT operator - (RMAT const& a, RMAT const& b)
{
	IS_TRUE(a.m_is_init && 
			b.m_is_init, 
			("not yet initialize."));
	RMAT c;
	MATRIX<RATIONAL> *cp = (MATRIX<RATIONAL>*)&c;
	MATRIX<RATIONAL> *ap = (MATRIX<RATIONAL>*)&a;
	MATRIX<RATIONAL> *bp = (MATRIX<RATIONAL>*)&b;
	*cp = *ap - *bp;
	return c;
}


/*
Dark Shadow elimination, inequlities form as: Ax <= c, 
and region of x(unknowns) for which gap between upper 
and lower bounds of x is guaranteed to be greater than 
or equals 1.
e.g: L <= x <= U , to ( L + 1) <= U.

'c': constant vector.
*/
void RMAT::ds(IN RMAT const& c)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
}


/*
Converting rational element to integer for row vector.

'row': number of row to integral

NOTICE: This function uses row convention.
*/
void RMAT::intliz(INT row)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(row < (INT)m_row_size, ("out of range"));
	if (row != -1) { 
		if (comden(row, 0) != 1) {
			for (UINT j = 0; j < m_col_size; j++) {
				setr(row, j, get(row, j).num(), 1);
			}
		}
	} else {
		for (UINT i = 0; i < m_row_size; i++) {
			if (comden(i, 0) != 1) {
				for (UINT j = 0; j < m_col_size; j++) {
					setr(i, j, get(i, j).num(), 1);
				}
			}
		}
	}
}



//
//Integer Matrix
//
INTMAT::INTMAT()
{
	m_is_init = false;
	init();
}


//used by template call of T(0) in SVECTOR<MAT>
INTMAT::INTMAT(INT v)
{
	INTMAT();
}


INTMAT::INTMAT(UINT row, UINT col)
{
	m_is_init = false;
	init();
	grow_all(row, col);
}


INTMAT::~INTMAT()
{
	destroy();
}


void INTMAT::init()
{
	if(m_is_init) return;
	((MATRIX<INT>*)this)->init();
	m_is_init = true;
}


void INTMAT::destroy()
{
	if(!m_is_init) return;
	m_is_init = false;
}


//Pamaters after 'num' must be integer.
void INTMAT::sete(UINT num, ...)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(num <= m_col_size*m_row_size, ("set out of boundary."));
	if (num <= 0) {
		return;
	}
	INT * ptr = (INT*) (((BYTE*)(&num)) + sizeof(UINT));	
	UINT i = 0;
	UINT row = 0, col = 0;
	while (i < num) {
		INT numer = *ptr;
		set(row, col++, numer);
		if (col >= m_col_size) {
			row++;
			col = 0;
		}
		i++;
		ptr++; //stack growing down
	}
}


INTMAT & INTMAT::operator = (INTMAT const& m)
{
	((MATRIX<INT>*)this)->copy(*((MATRIX<INT>*)&m));
	return *this;
}


//Copy elements in RMAT. Convert value type from rational to integral.
void INTMAT::copy(RMAT const& r)
{
	IS_TRUE(m_is_init && r.m_is_init, 
			("not yet initialize."));
	if (this == (INTMAT*)&r) return;
	if (m_mat) {
		::free(m_mat);
	}
	m_row_size = r.m_row_size;
	m_col_size = r.m_col_size;
	m_mat = (INT*)::malloc(m_row_size * m_col_size * sizeof(INT));
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			IS_TRUE(r.get(i, j).den() == 1, ("illegal rmat"));
			set(i, j, r.get(i, j).num());
		}
	}
}


/*
Invering of Integer Matrix will be transformed to Rational 
Matrix, and one exception will be thrown if there are some 
element's denomiator is not '1'.
*/
bool INTMAT::inv(OUT INTMAT & e)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	RMAT tmp, v;
	tmp.copy(*this);
	bool is_nonsingular = tmp.inv(tmp);
	e.reinit(m_row_size, m_col_size);
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			RATIONAL v = tmp.get(i, j);
			IS_TRUE(v.den() == 1, 
					("Should converts INTMAT to RMAT firstly"));
			e.set(i, j, v.num());			
		}
	}
	return is_nonsingular;
}


/*
Determinant of Integer Matrix will be transformed to Rational 
Matrix, and one exception will be thrown if there are some 
element's denomiator is not '1'.
*/
INT INTMAT::det()
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	RMAT tmp;
	tmp.copy(*this);
	RATIONAL v = tmp.det();
	IS_TRUE(v.den() == 1, ("Should converts INTMAT to RMAT firstly"));
	return v.num();
}


/*
Generate unimodular matrix to elimnate element.
*/
void INTMAT::gen_elim_mat(IN UINT row, IN UINT col, OUT INTMAT & elim)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	INT aii = get(row, row), aij = get(row, col), x, y;
	INT gcd = exgcd(aii, aij, x, y);
	IS_TRUE(gcd == aii*x + aij*y, ("illegal computation"));

	/*
	Construct unimodular to eliminate aij.
	Satisfied: det(uni) = x * ((x*aii+y*aij)/x*gcd) = 1
	*/
	elim.reinit(m_col_size, m_col_size, &m_inhr); 
	elim.eye(1);
	elim.set(row, row, x);
	elim.set(col, row, y);
	elim.set(row, col, -aij/gcd);
	elim.set(col, col, aii/gcd);
}


void INTMAT::_verify_hnf(INTMAT & h)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	for (UINT i = 0; i < MIN(h.m_row_size,h.m_col_size); i++) {
		//1. Eliminate element from i+1 to n to single zero.
		UINT j;
		INT v = h.get(i, i);
		for (j = 0; j < i; j++) {
			IS_TRUE(h.get(i,j) >= 0, ("negtive element"));
			IS_TRUE(h.get(i,j) < v, ("large than diagnal element"));
		}
		for (j = i + 1; j < h.m_col_size; j++) {
			IS_TRUE(h.get(i,j) == 0, ("should be low triangular"));
		}	
	}//end for
}


/*
Hermite Normal Form decomposition.
Given a m*n matrix A, there exists an n*n unimodular matrix U and 
an m*n lowtriangular matrix H such that:
	A = H*inv(U)
Hermite Normal Form:
	One possible set of conditions (corresponding to the col 
	convention and making H lower triangular) is given by
			1. h(ij)=0 for j>i,
			2. h(ii)>0 for all i, and
			3. h(ij)<=0 and |h(ij)|<|h(ii)| for j<i

Return true if succ.

h: hermite matrix, it is a lower triangular matrix, row convention.
u: unimodular matrix, so 'this' and h and u satisfied: 
	h = 'this' * u and 
	'this' = h * inv(u)

NOTICE:
	1. 'this' uses row convention.
	2. 'this' may be singular.	
*/
void INTMAT::hnf(OUT INTMAT & h, OUT INTMAT & u)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	u.reinit(m_col_size, m_col_size); 
	u.eye(1); //unimodular matrix
	h = *this;
	
	//Eliminiate non-zero upper diagonal elements.
	for (INT i = 0; i < (INT)MIN(h.m_row_size,h.m_col_size); i++) {
		//1. Eliminate element from i+1 to n to zero.
		INT j;
		for (j = i + 1; j < (INT)h.m_col_size; j++) {
			if (h.get(i, j) != 0) {
				INTMAT elim; 
				h.gen_elim_mat(i, j, elim);
				
				//Compounding unimodular postmultiply matrix
				//for each constituent transformation.
				u = u * elim; 

				//HNF(notes as H): H=A*U
				h = h * elim; 
			}//end if
		}//end for

		//2. Make diagonal element positive.
		if (h.get(i, i) < 0) {
			INTMAT neg(h.m_row_size, h.m_col_size);
			neg.eye(1);
			neg.set(i, i, -1);
			h = h * neg;
			u = u * neg;
		}

		/*
		3. Before performing the following operation, the 
		diagonal element must be positive!

		Make elements below diagonal in row from 0 to i-1 are non-negative.
		e.g: If aij is neative, and if abs(aij) <= abs(aii), 
		set aij = aij + aii or else set aij = aij + (d+1)*aii, 
		where d is abs(aij/aii).
		*/
		for (j = 0; j < i; j++) {
			if (h.get(i, j) < 0) {
				INT v;
				if (abs(h.get(i,j)) <= abs(h.get(i,i))) {
					v = 1;
				} else {
					v = abs(h.get(i,j) / h.get(i,i)) + 1;
				}
				INTMAT elim(m_col_size, m_col_size); //'this' may be m*n
				elim.eye(1);
				elim.set(i, j, v); 
				h = h * elim;
				u = u * elim;
			} //end if
		} //end for

		/*
		4. Reduce below diagonal elements which their value larger 
		than diagonal element in row.
		Make sure 'elim' is triangular matrix to ensure |det(elim)|=1

		e.g: If a(i,j) > a(i,i)
					d  = a(i,j)/a(i,i)
					a(i,j) = a(i,j) + (-d)*a(i,i)
		Generate 'elim' such as, at row convention.
				[1 0 0]
				[0 1 0]
				[-d 0 1]
			to reduce element a(i,j) less than a(i,i).
		*/
		for (j = 0; j < i; j++) {
			if (h.get(i, j) >= h.get(i, i)) {
				INT d = h.get(i, j) / h.get(i, i); //Get 
				INTMAT elim(m_col_size, m_col_size);
				elim.eye(1);
				elim.set(i, j, -d); 
				h = h * elim;
				u = u * elim;
			}
		}
	} //end for
	_verify_hnf(h);
}


//Reduce matrix by GCD operation.
void INTMAT::gcd()
{
	if (get_col_size() == 1) return;
	for (UINT i = 0; i < get_row_size(); i++) {
		UINT min = (UINT)-1;
		UINT j;
		bool allzero = true;
		for (j = 0; j < get_col_size(); j++) {
			UINT x = abs(get(i, j));
			if (x != 0) {
				min = MIN(min, x);
				allzero = false;
			}
		}
		if (min == 1 || min == 0 || allzero) {
			continue;	
		}
		UINT mingcd = min;
		for (j = 0; j < get_col_size(); j++) {
			UINT q = abs(get(i, j));
			if (q != 0 && q != mingcd) {
				mingcd = sgcd(mingcd, q);
				if (mingcd == 1) {
					break;
				}
			}
		}
		if (mingcd == 1) {
			continue;
		}
		for (j = 0; j < get_col_size(); j++) {
			set(i, j, get(i, j)/(INT)mingcd);		
		}
	}
}


/*
Find maximum convex hull of a set of 2-dimension points.(Graham scan)

'c': coordinates of a set of points.
'idx': 1*n matrix, indices of coordinates of convex hull.

NOTICE:
	1.'this' is a n*2 matrix that each row indicate one coordinate as (x,y).
*/
void INTMAT::cvexhull(OUT INTMAT & hull)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(m_col_size == 2, ("need n*2 matrix"));
	INT p0_x, p0_y;
	UINT p0_idx, p1_idx;
	UINT i;
	for (i = 0; i < m_row_size; i++) {
		if (i == 0) {
			p0_x = get(i, 0);
			p0_y = get(i, 1);
			p0_idx = i;
		} else if (get(i, 1) < p0_y) { //Get minimum coordinate in y axis.
			p0_x = get(i, 0);
			p0_y = get(i, 1);
			p0_idx = i;
		} else if (get(i, 1) == p0_y) {
			if (get(i, 0) < p0_x) { //Get most left point in x axis.
				p0_x = get(i, 0);
				p0_y = get(i, 1);
				p0_idx = i;
			}
		}
	}
	
	//Sort points with increasing polar angle, insertion sort.
	LIST<INT> order;
	for (i = 0; i < m_row_size; i++) {
		if (i == p0_idx) {
			continue;
		}
		bool inserted = false;
		INT next_idx = -1;
		INT idx;
		for (idx = order.get_head(); idx != 0; idx = next_idx) {
			next_idx = order.get_next();
			INT p1_x = get(i, 0) - p0_x;
			INT p1_y = get(i, 1) - p0_y;
			INT p2_x = get(idx - 1, 0) - p0_x;
			INT p2_y = get(idx - 1, 1) - p0_y;
			INT cross =  p1_x * p2_y -  p2_x * p1_y;	
			if (cross > 0) { 
				//'i' is in deasil order of 'idx', 
				//indicate angle of 'i-p0' less than 'idx-p0'.
				order.insert_before(i + 1, idx);
				inserted = true;
				break;
			} else if (cross == 0) { 
				//collinear, keep the point most far away from 'p0'
				if (p1_x != 0) {
					if (abs(p1_x) > abs(p2_x)) { //remove p2.
						order.insert_before(i + 1, idx);
						order.remove(idx);
						inserted = true; 
					} else { //keep p2.
						inserted = true; 
					}
				} else { //line p1,p2 is perpendicular to x-axis
					if (abs(p1_y) > abs(p2_y)) { //remove p2.
						order.insert_before(i + 1, idx);
						order.remove(idx);
						inserted = true; 
					} else { //keep p2.
						inserted = true; 
					}
				}
				if (inserted) {
					break;
				}
			}//end else ...
		}//end for

		if (!inserted) {
			IS_TRUE(idx == 0, ("illegal list"));
			order.append_tail(i + 1); //The first index in list starting at '1'.
		}
	}//end for 

	SSTACK<INT> s;
	INT next_idx = -1; 
	s.push(p0_idx + 1);
	s.push(order.get_head_nth(0)); 
	s.push(order.get_head_nth(1)); 
	p0_idx = 1; 
	p1_idx = 2;

	//Processing node in order list.
	for (INT idx = order.get_head_nth(2); idx != 0; 
		 idx = order.get_next()) {
		INT cross = 0;

		//If vector TOP(1)->idx is not turn left corresponding to 
		//TOP(1)->TOP(0), pop the element TOP(0) from stack.
		do {
			INT p0_idx = s.get_top_nth(1);
			INT p1_idx = s.get_top_nth(0);
			p0_x = get(p0_idx - 1, 0); 
			p0_y = get(p0_idx - 1, 1);

			INT p1_x = get(p1_idx - 1, 0) - p0_x;
			INT p1_y = get(p1_idx - 1, 1) - p0_y;
			INT p2_x = get(idx - 1, 0) - p0_x;
			INT p2_y = get(idx - 1, 1) - p0_y;
			cross =  p1_x * p2_y -  p2_x * p1_y;	
			if (cross < 0) {
				s.pop();
			} else {
				break;
			}
		} while (1);
		IS_TRUE(cross > 0, ("exception!"));
		s.push(idx); //point 'idx - 1' turn left.
	}

	//Record index of vertex of hull.
	hull.reinit(1, s.get_elem_count());
	for (INT j = s.get_elem_count() - 1; j >=0; j--) {
		INT idx = s.pop();
		hull.set(0, j,  idx-1);
	}
}


void INTMAT::dumps() const
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	printf("\n");	
	for (UINT i = 0; i < m_row_size; i++) {
		printf("\t");	
		for (UINT j = 0; j < m_col_size; j++) {
			CHAR const* blank = "           ";
			printf("%5d%s", get(i,j), blank);	
		}	
		printf("\n");	
	}
	printf("\n");	
}


void INTMAT::dumpf(CHAR const* name, bool is_del) const
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	if (name == NULL) {
		name = "matrix.tmp";
	}
	if (is_del) {
		unlink(name);
	}
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));
	fprintf(h, "\nMATRIX(%d,%d)\n", this->get_row_size(), this->get_col_size());	
	for (UINT i = 0; i < m_row_size; i++) {
		fprintf(h, "\t");	
		for (UINT j = 0; j < m_col_size; j++) {
			CHAR const* blank = "           ";
			fprintf(h, "%5d%s", get(i,j), blank);	
		}	
		fprintf(h, "\n");	
	}
	fprintf(h, "\n");	
	fclose(h);
}


INTMAT operator * (INTMAT const& a, INTMAT const& b)
{
	INTMAT c;
	MATRIX<INT> * cp = (MATRIX<INT>*)&c;
	MATRIX<INT> * ap = (MATRIX<INT>*)&a;
	MATRIX<INT> * bp = (MATRIX<INT>*)&b;
	*cp = *ap * *bp;
	return c;
}


INTMAT operator + (INTMAT const& a, INTMAT const& b)
{
	INTMAT c;
	MATRIX<INT> * cp = (MATRIX<INT>*)&c;
	MATRIX<INT> * ap = (MATRIX<INT>*)&a;
	MATRIX<INT> * bp = (MATRIX<INT>*)&b;
	*cp = *ap + *bp;
	return c;
}


INTMAT operator - (INTMAT const& a, INTMAT const& b)
{
	INTMAT c;
	MATRIX<INT> *cp = (MATRIX<INT>*)&c;
	MATRIX<INT> *ap = (MATRIX<INT>*)&a;
	MATRIX<INT> *bp = (MATRIX<INT>*)&b;
	*cp = *ap - *bp;
	return c;
}



//
//Float Matrix, default precision is double.
//
static CHAR const* g_sd_str = "%f";
static void val_adjust(MATRIX<FLTY> * pbasis)
{
	CHAR buf[256];
	for (UINT i = 0; i < pbasis->get_row_size(); i++) {
		for (UINT j = 0; j < pbasis->get_col_size(); j++) {
			sprintf(buf, g_sd_str, pbasis->get(i,j).f());
			pbasis->set(i, j, atof(buf));
		}
	}
}


static FLTY val_sqrt(FLTY a)
{
	FLTY v = ::sqrt(a.f());
	return v;
}


static void flt_dumpf_by_handle(void const* pbasis, FILE * h)
{
	IS_TRUE(h, ("file handle is NULL"));
	FLMAT * pthis = (FLMAT*)pbasis;
	fprintf(h, "\nMATRIX(%d,%d)\n", pthis->get_row_size(), pthis->get_col_size());	
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		fprintf(h, "\t");	
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			CHAR const* blank = " ";
			fprintf(h, "%10f%s", pthis->get(i,j).f(), blank);	
		}	
		fprintf(h, "\n");	
	}
	fprintf(h, "\n");	
}


static void flt_dumpf(void const* pbasis, CHAR const* name, bool is_del)
{
	if (name == NULL) {
		name = "matrix.tmp";
	}
	if (is_del) {
		unlink(name);
	}
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));
	flt_dumpf_by_handle(pbasis, h);
	fclose(h);
}


//Print as real number even though T is integer.
static void flt_dumps(void const* pbasis)
{
	FLMAT * pthis = (FLMAT*)pbasis;
	printf("\n");	
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		printf("\t");	
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			CHAR const* blank = "           ";
			printf("%5f%s", pthis->get(i,j).f(), blank);	
		}	
		printf("\n");	
	}
	printf("\n");	
}


float flt_fast_sqrt(float n) 
{	   
	float x, y;    
	const float f = 1.5f;    
	x = n * 0.5f;    
	y  = n;
	//WARNING: dereferencing type-punned pointer will break
	//strict-aliasing rules [-Wstrict-aliasing]
	LONG i = *(LONG*)&y;    
	i  = 0x5f3759df - (i >> 1);
	y  = *(float*)&i;    

	//May be need more iters for higher precision?
	y  = y * (f - ( x * y * y ));
	y  = y * (f - ( x * y * y ));
	y  = y * (f - ( x * y * y ));
	return n * y;
}


void FLMAT::set_sd(UINT sd)
{
	if (HOOK_ADJUST == NULL) {
		INHR i; 
		memset(&i, 0, sizeof(INHR)); 
		i.he = NULL;
		i.ha = val_adjust;
		i.hs = val_sqrt;
		i.hds = flt_dumps;
		i.hdf = flt_dumpf;
		i.hdfh = flt_dumpf_by_handle;
		set_hook(&i);
	}
	switch (sd) {
	case 0:	g_sd_str = "%.0f"; break;			
	case 1:	g_sd_str = "%.1f"; break;			
	case 2:	g_sd_str = "%.2f"; break;			
	case 3:	g_sd_str = "%.3f"; break;			
	case 4:	g_sd_str = "%.4f"; break;			
	case 5:	g_sd_str = "%.5f"; break;			
	case 6:	g_sd_str = "%.6f"; break;			
	default:
		IS_TRUE(0, ("unsupport significant digit!"));
	}
}


CHAR const* FLMAT::get_sd() const
{
	return g_sd_str;
}


FLMAT::FLMAT()
{
	m_is_init = false;
	init();
	
}


//used by template call of T(0) in SVECTOR<MAT>
FLMAT::FLMAT(INT v)
{
	FLMAT();
}


FLMAT::FLMAT(UINT row, UINT col)
{
	m_is_init = false;
	init();
	grow_all(row, col);
}


FLMAT::~FLMAT()
{
	destroy();
}


void FLMAT::init()
{
	if (m_is_init) return;
	((MATRIX<PRECISION_TYPE>*)this)->init();
	INHR i; memset(&i, 0, sizeof(INHR)); 
	i.he = NULL;
	i.ha = val_adjust;
	i.hs = val_sqrt; //an alternative: flt_fast_sqrt
	i.hds = flt_dumps;
	i.hdf = flt_dumpf;
	i.hdfh = flt_dumpf_by_handle;
	set_hook(&i);
	m_is_init = true;
}


void FLMAT::destroy()
{
	if(!m_is_init) return;
	m_is_init = false;
}


/*
Set value of elements one by one.
'num': indicate the number of variant parameters.
NOTICE:
	Pamaters after 'num' must be float/double.
	e.g: sete(NUM, 2.0, 3.0...)
*/
void FLMAT::sete(UINT num, ...)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(num <= m_col_size*m_row_size, ("set out of boundary."));
	if (num <= 0) {
		return;
	}
	UINT i = 0;
	UINT row = 0, col = 0;
	BYTE * ptr =(BYTE*) (((BYTE*)(&num)) + sizeof(num));	
	while (i < num) {
		PRECISION_TYPE numer = (PRECISION_TYPE)*(double*)ptr;
		MATRIX<FLTY>::set(row, col++, FLTY(numer));
		if (col >= m_col_size) {
			row++;
			col = 0;
		}
		i++;

		//C++ default regard type of the parameter as double.
		ptr+=sizeof(double); //stack growing down
	}
}


/*
Set value of elements one by one.
'num': indicate the number of variant parameters.
NOTICE:
	Pamaters after 'num' must be integer.
	e.g: setie(NUM, 2, 3...)
*/
void FLMAT::setie(UINT num, ...)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(num <= m_col_size*m_row_size, ("set out of boundary."));
	if (num <= 0) {
		return;
	}
	UINT i = 0;
	UINT row = 0, col = 0;
	BYTE * ptr =(BYTE*) (((BYTE*)(&num)) + sizeof(num));	
	while (i < num) {
		INT numer = *(INT*)ptr;
		MATRIX<FLTY>::set(row, col++, FLTY(numer));
		if (col >= m_col_size) {
			row++;
			col = 0;
		}
		i++;

		//C++ default regard type of the parameter as double.
		ptr += sizeof(INT); //stack growing down
	}
}


FLMAT& FLMAT::operator = (FLMAT const& m)
{
	((MATRIX<PRECISION_TYPE>*)this)->copy(*((MATRIX<PRECISION_TYPE>*)&m));
	return *this;
}


void FLMAT::substit(IN FLMAT const& exp, IN UINT v, bool is_eq, INT rhs_idx)
{
	IS_TRUE(m_is_init && exp.m_is_init, 
							("not yet initialize."));
	IS_TRUE(m_col_size == exp.m_col_size && v < m_col_size && 
		exp.is_rowvec(), ("unmatch matrix"));	
	
	if (!is_eq) {
		IS_TRUE0(rhs_idx >= 1 && rhs_idx < (INT)m_col_size);
		mul_of_cols(rhs_idx, m_col_size - 1, -1);
	}
	for (UINT i = 0; i < m_row_size; i++) {
		if (get(i, v) != 0) {
			FLMAT tmp = exp;
			if (tmp.get(0, v) == 0) {
				continue;
			}
			if (get(i, v) != tmp.get(0, v)) {
				tmp.mul(-get(i, v) / tmp.get(0, v));
			} else {
				tmp.mul(-1);
			}
			add_row_to_row(tmp, 0, i);
		}
	}
	if (!is_eq) {
		mul_of_cols(rhs_idx, m_col_size - 1, -1);
	}
}


//All elements are integer
bool FLMAT::is_imat(UINT * row, UINT * col)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	for (UINT i = 0; i < m_row_size; i++) {
		for (UINT j = 0; j < m_col_size; j++) {
			if (!get(i, j).is_int()) {
				if (row) *row = i;
				if (col) *col = j;
				return false;
			}
		}
	}
	return true;
}


FLMAT operator * (FLMAT const& a, FLMAT const& b)
{
	FLMAT c;
	MATRIX<PRECISION_TYPE> * cp = (MATRIX<PRECISION_TYPE>*)&c;
	MATRIX<PRECISION_TYPE> * ap = (MATRIX<PRECISION_TYPE>*)&a;
	MATRIX<PRECISION_TYPE> * bp = (MATRIX<PRECISION_TYPE>*)&b;
	*cp = *ap * *bp;
	return c;
}


FLMAT operator - (FLMAT const& a, FLMAT const& b)
{
	FLMAT c;
	MATRIX<PRECISION_TYPE> * cp = (MATRIX<PRECISION_TYPE>*)&c;
	MATRIX<PRECISION_TYPE> * ap = (MATRIX<PRECISION_TYPE>*)&a;
	MATRIX<PRECISION_TYPE> * bp = (MATRIX<PRECISION_TYPE>*)&b;
	*cp = *ap - *bp;
	return c;
}



//
//Boolean Matrix
//
static void bool_dumpf_by_handle(void const* pbasis, FILE * h)
{
	IS_TRUE(h != NULL, ("file handle is NULL"));
	BMAT * pthis = (BMAT*)pbasis;
	fprintf(h, "\nMATRIX(%d,%d)\n", pthis->get_row_size(), pthis->get_col_size());	
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		fprintf(h, "\t");	
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			CHAR const* blank = "           ";
			fprintf(h, "%10d%s", pthis->get(i,j), blank);	
		}	
		fprintf(h, "\n");	
	}
	fprintf(h, "\n");	
}


static void bool_dumpf(void const* pbasis, CHAR const* name, bool is_del)
{
	if (name == NULL) {
		name = "matrix.tmp";
	}
	if (is_del) {
		unlink(name);
	}
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));
	bool_dumpf_by_handle(pbasis, h);	
	fclose(h);
}


//Print as real number even though T is integer.
static void bool_dumps(void const* pbasis)
{
	BMAT * pthis = (BMAT*)pbasis;
	printf("\n");	
	for (UINT i = 0; i < pthis->get_row_size(); i++) {
		printf("\t");	
		for (UINT j = 0; j < pthis->get_col_size(); j++) {
			CHAR const* blank = "           ";
			printf("%5d%s", pthis->get(i,j), blank);	
		}	
		printf("\n");	
	}
	printf("\n");	
}


BMAT::BMAT()
{
	m_is_init = false;
	init();
}


//used by template call of T(0) in SVECTOR<MAT>
BMAT::BMAT(INT v)
{
	BMAT();
}


BMAT::BMAT(UINT row, UINT col)
{
	m_is_init = false;
	init();
	grow_all(row, col);
}


BMAT::~BMAT()
{
	destroy();
}


void BMAT::init()
{
	if (m_is_init) return;
	((MATRIX<bool>*)this)->init();
	INHR i; memset(&i, 0, sizeof(INHR)); 
	i.hds = bool_dumps;
	i.hdf = bool_dumpf;
	i.hdfh = bool_dumpf_by_handle;
	set_hook(&i);
	m_is_init = true;
}


void BMAT::destroy()
{
	if(!m_is_init) return;
	m_is_init = false;
}


/*
Assignment value of matrix element
e.g: sete(3, true, true, false)
*/
void BMAT::sete(UINT num, ...)
{
	IS_TRUE(m_is_init, ("not yet initialize."));
	IS_TRUE(num <= m_col_size*m_row_size, ("set out of boundary."));
	if (num <= 0) {
		return;
	}
	UINT i = 0;
	UINT row = 0, col = 0;
	bool * ptr = (bool*)(((BYTE*)(&num)) + sizeof(num));	
	while (i < num) {
		set(row, col++, *ptr);
		if (col >= m_col_size) {
			row++;
			col = 0;
		}
		i++;

		//Default regard type of the parameter as double.
		ptr++; //stack growing down
	}
}


BMAT& BMAT::operator = (BMAT const& m)
{
	((MATRIX<bool>*)this)->copy(*((MATRIX<bool>*)&m));
	return *this;
}

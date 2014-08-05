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
#ifndef _COMF_H_
#define _COMF_H_

template <class T> class SVECTOR;

UINT arra(UINT n, UINT m); //Arrangement
void af2i(IN CHAR * f, OUT BYTE * buf, INT buflen, bool is_double);

bool binsearch(INT array[], UINT n, INT v, IN OUT UINT * ppos);

UINT combin(UINT n, UINT m); //Combination
LONGLONG ceil_align(LONGLONG v, LONGLONG align);

void dumpf_svec(void * vec, UINT ty, CHAR const* name, bool is_del);
void dumps_svec(void * vec, UINT ty);

INT exgcd(IN INT a, IN INT b, OUT INT & x, OUT INT & y);

UINT fact(UINT n);
INT findstr(CHAR * src, CHAR * s);

INT gcdm(UINT num, ...);
INT gcdm(UINT num, SVECTOR<INT> const& a);
UINT get_power_of_2(ULONGLONG v);
UINT get_const_bit_len(LONGLONG v);
CHAR * getfilesuffix(CHAR * n, OUT CHAR * buf);
CHAR * getfilepath(CHAR * n, OUT CHAR * buf, UINT bufl);
CHAR * getfilename(CHAR * n, OUT CHAR * buf, UINT bufl);
ULONGLONG getusec();
INT get_first_one_pos(INT m);

bool is_integer(float f);
bool is_integerd(double d);
bool is_power_of_5(double f);
inline bool is_power_of_2(ULONGLONG x)
{ return (x != 0 && (x & (x-1)) == 0); }

void prim(INT m, OUT INT * buf);
LONG revlong(LONG d);
UCHAR * reverse_string(UCHAR * v);
CHAR * upper(CHAR * n);
CHAR * lower(CHAR * n);
INT sgcd(INT x, INT y);
INT slcm(INT x, INT y);
void strshift(CHAR * src, INT ofst);

CHAR * xstrcat(CHAR * buf, UINT bufl, CHAR const* info, ...);
UINT xstrlen(CHAR const* p);
bool xstrcmp(CHAR const* p1, CHAR const* p2, INT n);
CHAR * xsprintf(IN OUT CHAR * buf, 
				IN UINT buflen, 
				IN CHAR const* format, 
				...);
LONG xatol(CHAR const* nptr, bool is_oct);
INT xctoi(CHAR const* cl);
UCHAR * xltoa(IN LONG v, OUT UCHAR * buf);
INT xceiling(INT a, INT b);
INT xfloor(INT a, INT b);
INT xstrstr(CHAR const* src, CHAR const* par, INT i);
void xstrcpy(CHAR * tgt, CHAR const* src, UINT size);
inline bool xisspace(CHAR c) { return c == ' ' || c == '\t'; }
inline bool xisdigit(CHAR c) { return c >= '0' && c <= '9'; }
inline bool xisdigithex(CHAR d)
{
	if (xisdigit(d)) return true;
	else if ((d >= 'a' && d <= 'f') || (d >= 'A' && d <= 'F')) return true;
	return false;
}
inline bool xisalpha(CHAR c) 
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
LONGLONG xabs(LONGLONG a);

//Exported Data Structures
class ASCII {
public:
	UCHAR val;
	CHAR ch;
};
#endif


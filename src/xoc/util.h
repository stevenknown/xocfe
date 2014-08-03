#ifndef __UTIL_H__
#define __UTIL_H__
 
#define MAX_BUF_LEN 1024
#define ST_ERR  1
#define ST_SUCC 0
#define ST_EOF 2

//Exported Variables
extern INT g_indent; //Only for dump.
extern FILE * g_tfile;

void dump_vec(SVECTOR<UINT> & v);
void initdump(CHAR const* f, bool is_del);
void interwarn(CHAR const* format, ...);
void finidump();
UINT get_const_bit_len(LONGLONG v);
void scr(CHAR const* format , ...) ;
void * tlloc(LONG size);
void tfree();
void note(CHAR const* format, ...);
INT prt(CHAR const* format , ...);
#endif


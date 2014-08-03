#include "ltype.h"
#include "smempool.h"
#include "sstl.h"
#include "errno.h"

#define ERR_BUF_LEN 1024

INT g_indent = 0;
CHAR * g_indent_chars = (CHAR*)" ";
SMEM_POOL * g_pool_tmp_used = NULL;
FILE * g_tfile = NULL;

void interwarn(CHAR const* format, ...)
{
	CHAR sbuf[ERR_BUF_LEN];
	if (strlen(format) > ERR_BUF_LEN) {
		IS_TRUE(0, ("internwarn message is too long to print"));
	}
	//CHAR * arg = (CHAR*)((CHAR*)(&format) + sizeof(CHAR*));	
	va_list arg;
	va_start(arg, format);
	vsprintf(sbuf, format, arg);
	printf("\n!!!XOC INTERNAL WARNING:%s\n\n", sbuf); 	
	va_end(arg);
}


//Print string.
INT prt(CHAR const* format, ...)
{
	if (format == NULL) return 0;
	CHAR buf[MAX_BUF_LEN];
	if (strlen(format) > MAX_BUF_LEN) {
		IS_TRUE(0, ("prt message is too long to print"));
	}
	//CHAR * arg = (CHAR*)((CHAR*)(&format) + sizeof(CHAR*));
	va_list arg;
	va_start(arg, format);
	vsprintf(buf, format, arg);
    if (g_tfile != NULL) {
		fprintf(g_tfile, "%s", buf);
		fflush(g_tfile);
	} else {
		fprintf(stdout, "%s", buf);
	}
	va_end(arg);
	return 0;
}


void scr(CHAR const* format, ...) 
{
	return;
	
	//CHAR * arg = (CHAR*)((CHAR*)(&format) + sizeof(CHAR*));
	va_list arg;
	va_start(arg, format);
	CHAR buf[MAX_BUF_LEN];		
	vsprintf(buf, format, arg);	
	fprintf(stdout, "%s", buf);
	va_end(arg);
}


void finidump()
{
	if (g_tfile != NULL) {
		fclose(g_tfile);
		g_tfile = NULL;
	}
}


void initdump(CHAR const* f, bool is_del)
{
	if (is_del && f != NULL) {
		unlink(f);
	}
	if (f != NULL) {
		if (is_del) {
			unlink(f);
		}
		g_tfile = fopen(f, "a+");
		if (g_tfile == NULL) {
			IS_TRUE(0, ("can not open %s, errno:%d, errstring is %s", 
						f, errno, strerror(errno)));
			return;
		}
	} else {
		//g_tfile = stdout;
	}
}


//Print string with indent chars.
void note(CHAR const* format, ...)
{
	if (g_tfile == NULL) {
		return;
	}
	if (format == NULL) return;
	CHAR buf[MAX_BUF_LEN];
	CHAR * real_buf = buf;
	//CHAR * arg = (CHAR*)((CHAR*)(&format) + sizeof(CHAR*));
	va_list arg;
	va_start(arg, format);
	vsnprintf(buf, MAX_BUF_LEN, format, arg);
	buf[MAX_BUF_LEN - 1] = 0;
	UINT len = strlen(buf);
	IS_TRUE0(len < MAX_BUF_LEN);
	UINT i = 0;
	while (i < len) {
		if (real_buf[i] == '\n') {
			fprintf(g_tfile, "\n");
		} else {
			break;
		}
		i++;
	}
	
	//Append indent chars ahead of string.	
	INT w = 0;
	while (w < g_indent) {
		fprintf(g_tfile, "%s", g_indent_chars);
		w++;
	}

	if (i == len) { 		
		fflush(g_tfile);
		goto FIN;
	}
	
	fprintf(g_tfile, "%s", real_buf + i);
	fflush(g_tfile);
FIN:
	va_end(arg);
	return;
}


//Malloc memory for tmp used. 
void * tlloc(LONG size)
{
	if (size < 0 || size == 0) return NULL;
	if (g_pool_tmp_used == NULL) {
		g_pool_tmp_used = smpool_create_handle(8, MEM_COMM);
	}
	void * p = smpool_malloc_h(size, g_pool_tmp_used);
	if (p == NULL) return NULL;
	memset(p, 0, size);
	return p;
}


void tfree()
{
	if (g_pool_tmp_used != NULL) {
		smpool_free_handle(g_pool_tmp_used);
		g_pool_tmp_used = NULL;
	}
}


void dump_vec(SVECTOR<UINT> & v)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n");
	for (INT i = 0; i <= v.get_last_idx(); i++) {
		UINT x = v.get(i);
		if (x == 0) {
			fprintf(g_tfile, "0,");
		} else {
			fprintf(g_tfile, "0x%x,", x);
		}
	}
	fflush(g_tfile);
}

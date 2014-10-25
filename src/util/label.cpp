#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"
#include "util.h"
#include "symtab.h"
#include "label.h"

LABEL_INFO * new_clabel(SYM * st, SMEM_POOL * pool)
{
	LABEL_INFO * li = new_label(pool);
	LABEL_INFO_name(li) = st;
	LABEL_INFO_type(li) = L_CLABEL;
	return li;
}


LABEL_INFO * new_ilabel(SMEM_POOL * pool)
{
	LABEL_INFO * n = new_label(pool);
	LABEL_INFO_type(n) = L_ILABEL;
	return n;
}


LABEL_INFO * new_label(SMEM_POOL * pool)
{
	LABEL_INFO * p = (LABEL_INFO*)smpool_malloc_h(sizeof(LABEL_INFO), pool);
	IS_TRUE0(p);
	memset(p, 0, sizeof(LABEL_INFO));
	return p;
}


bool is_same_label(LABEL_INFO const* li1, LABEL_INFO const* li2)
{
	IS_TRUE0(li1 && li2);
	if (li1 == li2) return true;
	if (LABEL_INFO_type(li1) == LABEL_INFO_type(li2) &&
		LABEL_INFO_num(li1) == LABEL_INFO_num(li2)) {
		return true;
	}
	return false;
}


void dump_lab(LABEL_INFO const* li)
{
	if (g_tfile == NULL) return;
	if (LABEL_INFO_type(li) == L_ILABEL) {
		fprintf(g_tfile, "\nilabel(" ILABEL_STR_FORMAT ")",
				ILABEL_CONT(li));
	} else if (LABEL_INFO_type(li) == L_CLABEL) {
		fprintf(g_tfile, "\nclabel(" CLABEL_STR_FORMAT ")",
				CLABEL_CONT(li));
	} else if (LABEL_INFO_type(li) == L_PRAGMA) {
		fprintf(g_tfile, "\npragms(%s)",
				SYM_name(LABEL_INFO_pragma(li)));
	} else { IS_TRUE0(0); }

	if (LABEL_INFO_b1(li) != 0) {
		fprintf(g_tfile, "(");
	}
	if (LABEL_INFO_is_try_start(li)) {
		fprintf(g_tfile, "try_start ");
	}
	if (LABEL_INFO_is_try_end(li)) {
		fprintf(g_tfile, "try_end ");
	}
	if (LABEL_INFO_is_catch_start(li)) {
		fprintf(g_tfile, "catch_start ");
	}
	if (LABEL_INFO_is_used(li)) {
		fprintf(g_tfile, "used ");
	}
	if (LABEL_INFO_b1(li) != 0) {
		fprintf(g_tfile, ")");
	}
	fflush(g_tfile);
}


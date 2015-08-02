#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"
#include "util.h"
#include "symtab.h"
#include "label.h"

LabelInfo * newCustomerLabel(SYM * st, SMemPool * pool)
{
	LabelInfo * li = newLabel(pool);
	LABEL_INFO_name(li) = st;
	LABEL_INFO_type(li) = L_CLABEL;
	return li;
}


LabelInfo * newInternalLabel(SMemPool * pool)
{
	LabelInfo * n = newLabel(pool);
	LABEL_INFO_type(n) = L_ILABEL;
	return n;
}


LabelInfo * newLabel(SMemPool * pool)
{
	LabelInfo * p = (LabelInfo*)smpoolMalloc(sizeof(LabelInfo), pool);
	ASSERT0(p);
	memset(p, 0, sizeof(LabelInfo));
	return p;
}


void dumpLabel(LabelInfo const* li)
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
	} else { ASSERT0(0); }

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

$(info "CONDBR:arm/Makefile.opt.ext.inc:TARG=$(TARG),TARG_DIR=$(TARG_DIR)")
OPT_OBJS+=\
regset_impl.o\
regalloc_mgr.o\
lsra_impl.o\
lsra_post_opt.o\
lsra_scan_in_pos.o\
lsra_scan_in_prio.o\
lt_interf_graph.o\
lt_prio_mgr.o\
lifetime.o\
targinfo_mgr.o\
linear_scan.o\
ir_reloc_mgr.o\
var_reloc_mgr.o

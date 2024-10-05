#DO NOT ADD VAR BEFORE FILES

ARM_OBJS+=\
armasmprinter.o\
arm_cg.o\
arm_sim.o\
arm_cgmgr.o\
arm.o\
armir2or.o\
arm_lis.o\
arm_lra.o\
arm_or.o\
arm_passmgr.o\
arm_ramgr.o\
arm_region.o\
arm_region_mgr.o\
arm_scalar_opt.o\
arm_simp.o\
arm_refine.o\
arm_sr.o\
arm_var.o\
arm_ddg.o\
arm_targinfo.o\
arm_decl_ext.o\
arm_ir_dump.o\
arm_elf_mgr.o

ifeq ($(FOR_IP), true)
  $(info "CONDBR:arm/Makefile.inc:FOR_IP=$(FOR_IP)")
  ARM_OBJS+=\
  arm_derivative.o\
  arm_irmgr.o\
  arm_elf_targinfo.o
endif

ifneq ($(findstring $(REF_TARGMACH_INFO), true),)
  $(info "CONDBR:arm/Makefile.inc:REF_TARGMACH_INFO=$(REF_TARGMACH_INFO)")
  ARM_OBJS+=\
  arm_targinfo_mgr.o\
  arm_linear_scan.o
endif

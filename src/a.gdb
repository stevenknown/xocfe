#compiler
file xocfe.exe

b m518087
b m116038

run ../test/test_ansic.c -dump xx


#b main
b m518087
b m116038



set print pretty on
set listsize 20

def pru
p m_ru->get_ru_name()
end

def fss
fs src 
end
def fsc
fs cmd
end

def ff
call fflush($arg0)
end

def ccfg
call $arg0->dump_cfg(0,0)
end

def pcfg
call trace_cfg(0,1)
end

def pir
p dump_ir($arg0, 0, 1, 1, 0)
end


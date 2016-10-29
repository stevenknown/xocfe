xocfe
=====

XOCFE is an concise, clearly, C compiler frontend, it is easy to understand and 
modify. XOCFE does not include preprocessor, so its input is pure C, output is 
an AST tree. XOCFE only support C99.

A concise and readable C++ Template Library is under 'src/com' directory. It can be used to 
build whole compiler, or anyother project.

BUILD: cd src & make -f Makefile.cfe

RUN: cd src & ./xocfe ../test/test_ansic.c -dump asttree.log

Enjoy the code.

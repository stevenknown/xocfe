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
#ifndef __TYPECK_H__
#define __TYPECK_H__

class TYCtx {
public:
    //When it comes to lvalue expression of assignment,
    //TR_ID should corresponding with IR_ID, rather than IR_LD.
    BYTE is_lvalue:1;

    //Set to true if current TR_ID indicate field one of
    //struct/union contained.
    BYTE is_field:1;

    //Record base of current memory accessing.
    //e.g: it records the struct/union name if we meet a field.
    Tree * base_tree_node;

    TYCtx() { is_lvalue = false; is_field = false; base_tree_node = nullptr; }
};

bool isConsistentWithPointer(Tree * t);
INT process_init(Decl * decl, Tree ** init);
INT process_init(Decl * decl);
INT TypeCheckTreeList(Tree * t, TYCtx * cont);
INT TypeCheck();
INT TypeTransform();
INT TypeTran(Tree * t, TYCtx * cont);

#endif

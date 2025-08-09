The software is licensed under the BSD License.

NOTE: You should enable -D_DEBUG_ if you want to dump AST tree.

Building
------------
    ./build_xocfe.sh
   or
    make xocfe -f Makefile.cfe


Examples
------------
    ./xocfe.exe  examples.c -dump a.tmp

Enjoy!


How to use cfe?
------------
    The most effective way to understand xocfe is step one step debug how
    'dump_scope()' and 'dump_scope_tree()' works.

    Here, I explain some concepts.

    Firstly, CFE(C Front End) generate a top scope.
    The top scope means all compile unit, namely, it is file scope.
    The scope includes a number of list, include type list,  statement list,
    and function definitions, and function declarations.
    So walk through these data-structure, you will get the informations of :
        * variable declarations
        * function definition(it has function body)
        * function declaration(no function body)
        * typedef declarations(I call them 'user type')
        * struct/union/enum declaration.
    See scope.h for details.


Declaration List
------------
    Declaration represent the type for each variables and each functions,
    it is structured as tree style.

    You could invoke API to check the properties of declaration,
    these APIs defined in cfe/decl.h

    E.g: The structure of declaration:
    int a;
    'a' has declaration such as:
        declaration----
            |          |--type-spec (int)
            |          |--declarator
            |                |---decl-type (identifier:a)

    and more complex examples:
        int * a, * const * volatile b[10];
        declaration----
            |          |--type-spec (int)
            |          |--declarator1
            |                |---decl-type (identifier:a)
            |                |---decl-type (pointer)
            |
            |          |--declarator2
            |                |---decl-type (identifier:b)
            |                |---decl-type (array:dim=10)
            |                |---decl-type (pointer:volatile)
            |                |---decl-type (pointer:const)

        int (*q)[30];
        declaration----
            |          |--type-spec (int)
            |          |--declarator1
            |                |---decl-type (identifier:q)
            |                |---decl-type (pointer)
            |                |---decl-type (array:dim=30)

        unsigned long const (* const c)(void);
        declaration----
                      |--type-spec (unsigned long const)
                      |--declarator1
                            |---decl-type (identifier:c)
                            |---decl-type (pointer:const)
                            |---decl-type (function)

    Declaration consist of Type, and a list of Declators.
        DCL_DECLARATION
            |->Scope
            |->TYPE
                |->const|volatile
                |-> void|long|int|short|char|float|double|signed|unsigned|struct|union
                |->auto|register|static|extern|typedef
            |->DCL_DECLARATOR|DCL_ABS_DECLARATOR
                |->DCL_ID(optional)->DCL_FUN->DCL_POINTER->...


Statement List
------------
    Statement includes:
        TR_ASSIGN
        TR_INTRI_FUN
        TR_IF
        TR_ELSE
        TR_DO
        TR_WHILE
        TR_FOR
        TR_SWITCH
        TR_BREAK
        TR_CONTINUE
        TR_RETURN
        TR_GOTO
        TR_LABEL
        TR_DEFAULT
        TR_CASE
        TR_POST_INC        // a++
        TR_POST_DEC        // a--
        TR_REV                // Reverse
        TR_NOT                // get non-value
        TR_SIZEOF            // sizeof(a)
        TR_CALL            // function call
        TR_SCOPE            // record a scope
        TR_EXP_SCOPE        // record a scope only permit expression-list.
        TR_PRAGMA            // pragma


How to walk through AST.
------------
    If we have a frontend, we need to generate some IR that consist of our
    compiler middle end.

    The most important things is to walk through AST, and convert the AST TREE
    node to various compiler middle end IR. Of course, how to convert the TREE
    node depends on your IR.

    Here is an example code to walk through AST:

    void walk_through_ast()
    {
        Scope * scope = get_global_scope(); //The most high level scope.

        //Find function definition. Because the function body always belong to
        //a function definition.
        DECL * dcl = SCOPE_decl_list(scope);
        while (dcl != NULL) {
            if (is_fun_decl(dcl) && DECL_is_fun_def(dcl)) {
                break;
            }
            dcl = dcl->next;
        }
        if (dcl == NULL) return;

        //Now dcl is func definition. Get its body.
        TREE * statement_tree_list = SCOPE_stmt_list(DECL_fun_body(dcl));
        while (statement_tree_list != NULL) {

            //Access statement, do convertion, or anything you want.

            statement_tree_list = statement_tree_list->next;
        }
    }

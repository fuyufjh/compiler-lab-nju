#include <stdio.h>

#include "syntax.tab.h"
#include "ast.h"

extern void yyrestart(FILE * input_file);

int no_error = 1;

int main(int argc, char* argv[]) {
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if (no_error) print_ast(root, 0);

    return 0;
}

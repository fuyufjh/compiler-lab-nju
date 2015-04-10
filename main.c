#include <stdio.h>

#include "syntax.tab.h"
#include "grammer_tree.h"

extern void yyrestart(FILE * input_file);

int main(int argc, char* argv[]) {
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    print_tree(root, 0);

    return 0;
}

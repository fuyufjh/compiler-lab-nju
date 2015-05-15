#include <stdlib.h>
#include <stdio.h>

#include "syntax.tab.h"
#include "ast.h"
#include "read_symbols.h"

extern void yyrestart(FILE * input_file);

int no_error = 1;

int main(int argc, char* argv[]) {
    if (argc <= 1) return 1;
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(fp);
    yyparse();
    if (!no_error) {
        fclose(fp);
        return -1;
    }
    rewind(fp);
    fseek(fp ,0L ,SEEK_END);
    int fsize = ftell(fp);
    source_code = (char*) malloc(fsize + 1);

    rewind(fp);
    fread(source_code, fsize, 1, fp);
    fclose(fp);

    print_ast(ast_root, 0);
    read_symbols();
    check_declared_fun();
    return 0;
}

#include "common.h"
#include "syntax.tab.h"
#include "ast.h"
#include "translate.h"
#include "block_opt.h"

extern void yyrestart(FILE * input_file);

bool no_error = true;

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Usage: parser [-p] [-v] [-B] source [out]\n");
        printf("  -p  print the abstract syntax tree (AST)\n");
        printf("  -v  verbose mode\n");
        printf("  -B  disable basic block optimizing\n");
        printf("Copyright: Fu Yu @ NJU\n");
        return -1;
    }
    int i;
    char *filename = NULL;
    char *out = NULL;
    for (i=1; i<argc; i++) {
        if (*argv[i] != '-') {
            if (filename == NULL) filename = argv[i];
            else if (out == NULL) out = argv[i];
        }
        else if (strcmp(argv[i], "-p") == 0) flag_print_ast = true;
        else if (strcmp(argv[i], "-v") == 0) flag_verbose = true;
        else if (strcmp(argv[i], "-B") == 0) flag_disable_block_optimize = true;
        else printf("Invalid parameter \"%s\"\n", argv[i]);
    }
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return 1;
    }
    yyrestart(fp);
    yyparse();
    if (!no_error) {
        fclose(fp);
        return -1;
    }

    // Read source code to memery
    rewind(fp);
    fseek(fp ,0L ,SEEK_END);
    int fsize = ftell(fp);
    source_code = (char*) malloc(fsize + 1);
    rewind(fp);
    fread(source_code, fsize, 1, fp);
    fclose(fp);

    if (flag_print_ast) {
        print_ast(ast_root, 0);
    }
    init_read_write();
    translate();
    check_declared_fun();
    if (!no_error) return 0;
    if (!flag_disable_block_optimize) {
        block_optimize();
    }

    if (out) {
        fp = fopen(out, "w");
        if (fp == NULL) {
            perror(out);
            return 1;
        }
    } else {
        fp = stdout;
    }
    print_ir_list(fp);
    fclose(fp);
    free(source_code);
    return 0;
}

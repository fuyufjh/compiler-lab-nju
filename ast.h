#ifndef _AST_H
#define _AST_H

enum ast_value_type {
    NO_VALUE,
    INT_VALUE,
    FLOAT_VALUE,
    DOUBLE_VALUE,
    STR_VALUE
};

enum ast_nonterminal_symbol {
    Program, ExtDefList, ExtDef, ExtDecList, Specifier,
    StructSpecifier, OptTag, Tag, VarDec, FunDec, VarList,
    ParamDec, CompSt, StmtList, Stmt, DefList, Def, DecList,
    Dec, Exp, Args
};

/* yytokentype: defined in syntax.tab.h */

extern const char* nt_symbol_name[];
extern const char* t_symbol_name[];

#define get_symbol_name(symbol) \
    (symbol<ASSIGNOP?nt_symbol_name[symbol]:t_symbol_name[symbol-ASSIGNOP])

int get_value_type(int node_type);

union ast_value {
    int int_value;
    float float_value;
    double double_value;
    char* str_value;
};

struct ast_node {
    int symbol;
    union ast_value value;
    int lineno;

    struct ast_node *child;
    struct ast_node *peer;
    struct ast_node *parent;
};

struct ast_node *root;

void add_children_ast_node(struct ast_node *parent, int n, ...);

void print_ast(struct ast_node *root, int space);
struct ast_node *make_ast_node_terminal(int st, union ast_value value);
struct ast_node *make_ast_node_nonterminal(int st);

#endif

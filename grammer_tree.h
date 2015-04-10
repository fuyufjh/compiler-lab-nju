#ifndef GRAMMER_TREE_H
#define GRAMMER_TREE_H

#include <stdarg.h>

typedef enum ValueType {
    NO_VALUE,
    INT_VALUE,
    FLOAT_VALUE,
    DOUBLE_VALUE,
    STR_VALUE
} ValueType;

typedef enum NonterminalSymbolType {
    Program, ExtDefList, ExtDef, ExtDecList, Specifier,
    StructSpecifier, OptTag, Tag, VarDec, FunDec, VarList,
    ParamDec, CompSt, StmtList, Stmt, DefList, Def, DecList,
    Dec, Exp, Args
} NonterminalSymbolType;

/* yytokentype: defined in syntax.tab.h */
typedef enum yytokentype TerminalSymbolType;

extern const char* nt_symbol_name[];
extern const char* t_symbol_name[];

#define get_name(s) \
    (s<ASSIGNOP?nt_symbol_name[s]:t_symbol_name[s-ASSIGNOP])

typedef union {
    int int_value;
    float float_value;
    double double_value;
    char* str_value;
} Value;

typedef struct Node {
    int symbol;
    ValueType type;
    Value value;
    int lineno;

    struct Node* child;
    struct Node* peer;
    struct Node* parent;
} Node;

Node* root;

void add_child(struct Node* parent, Node* node);
void add_children(Node* parent, int n, ...);

void print_tree(struct Node* root, int space);
Node* make_node_terminal(int st, int vt, Value v);
Node* make_node_nonterminal(int st, int lineno);

#endif

%{
#include "common.h"
#include "ast.h"
typedef struct ast_node* AST_NODE;
#define YYSTYPE AST_NODE

int yylex (void);
void yyerror (const char *);

%}
%locations
%define parse.error verbose

/* Level 8 */
%right ASSIGNOP

/* Level 7 */
%left OR

/* Level 6 */
%left AND

/* Level 5 */
%left RELOP

/* Level 4 */
%left MINUS PLUS

/* Level 3 */
%left STAR DIV

/* Level 2 */
%right NOT

/* Level 1 */
%left LP RP LB RB DOT 

/* Others */
%nonassoc LOWER_THAN_ELSE
%token INT
%token FLOAT
%token TYPE
%token ID
%nonassoc SEMI COMMA
%nonassoc STRUCT RETURN IF ELSE WHILE
%left LC RC

/* Nonterminal Symbols */

%%
/* High-level Definitions */
Program: ExtDefList {
    $$ = make_ast_node_nonterminal(Program);
    ast_root = $$;
    add_children_ast_node($$, 1, $1);
    }
    ;
ExtDefList: ExtDef ExtDefList {
    $$ = make_ast_node_nonterminal(ExtDefList);
    add_children_ast_node($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
ExtDef: Specifier ExtDecList SEMI {
    $$ = make_ast_node_nonterminal(ExtDef);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Specifier SEMI {
    $$ = make_ast_node_nonterminal(ExtDef);
    add_children_ast_node($$, 2, $1, $2);
    }
    | Specifier FunDec CompSt {
    $$ = make_ast_node_nonterminal(ExtDef);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Specifier FunDec SEMI {
    $$ = make_ast_node_nonterminal(ExtDef);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | error SEMI { yyerrok; }
    ;
ExtDecList: VarDec {
    $$ = make_ast_node_nonterminal(ExtDecList);
    add_children_ast_node($$, 1, $1);
    }
    | VarDec COMMA ExtDecList {
    $$ = make_ast_node_nonterminal(ExtDecList);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    ;

/* Specifiers */
Specifier: TYPE {
    $$ = make_ast_node_nonterminal(Specifier);
    add_children_ast_node($$, 1, $1);
    }
    | StructSpecifier {
    $$ = make_ast_node_nonterminal(Specifier);
    add_children_ast_node($$, 1, $1);
    }
    ;
StructSpecifier: STRUCT OptTag LC DefList RC {
    $$ = make_ast_node_nonterminal(StructSpecifier);
    add_children_ast_node($$, 5, $1, $2, $3, $4, $5);
    }
    | STRUCT Tag {
    $$ = make_ast_node_nonterminal(StructSpecifier);
    add_children_ast_node($$, 2, $1, $2);
    }
    ;
OptTag: ID {
    $$ = make_ast_node_nonterminal(OptTag);
    add_children_ast_node($$, 1, $1);
    }
    | /* empty */ { $$ = NULL; }
    ;
Tag: ID {
    $$ = make_ast_node_nonterminal(Tag);
    add_children_ast_node($$, 1, $1);
    }
    ;

/* Declarators */
VarDec: ID {
    $$ = make_ast_node_nonterminal(VarDec);
    add_children_ast_node($$, 1, $1);
    }
    | VarDec LB INT RB {
    $$ = make_ast_node_nonterminal(VarDec);
    add_children_ast_node($$, 4, $1, $2, $3, $4);
    }
    ;
FunDec: ID LP VarList RP {
    $$ = make_ast_node_nonterminal(FunDec);
    add_children_ast_node($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
    $$ = make_ast_node_nonterminal(FunDec);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    ;
VarList: ParamDec COMMA VarList {
    $$ = make_ast_node_nonterminal(VarList);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | ParamDec {
    $$ = make_ast_node_nonterminal(VarList);
    add_children_ast_node($$, 1, $1);
    }
    ;
ParamDec: Specifier VarDec {
    $$ = make_ast_node_nonterminal(ParamDec);
    add_children_ast_node($$, 2, $1, $2);
    }
    | Specifier {
    $$ = make_ast_node_nonterminal(ParamDec);
    add_children_ast_node($$, 2, $1);
    }
    ;

/* Statements */
CompSt: LC DefList StmtList RC {
    $$ = make_ast_node_nonterminal(CompSt);
    add_children_ast_node($$, 4, $1, $2, $3, $4);
    }
    | error RC { yyerrok; }
    ;
StmtList: Stmt StmtList {
    $$ = make_ast_node_nonterminal(StmtList);
    add_children_ast_node($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Stmt: Exp SEMI {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 2, $1, $2);
    }
    | CompSt {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 1, $1);
    }
    | RETURN Exp SEMI {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | IF LP Exp RP Stmt {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 5, $1, $2, $3, $4, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt %prec LOWER_THAN_ELSE {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 7, $1, $2, $3, $4, $5, $6, $7);
    }
    | WHILE LP Exp RP Stmt {
    $$ = make_ast_node_nonterminal(Stmt);
    add_children_ast_node($$, 5, $1, $2, $3, $4, $5);
    }
    | error SEMI { yyerrok; }
    ;

/* Local Definitions */
DefList: Def DefList {
    $$ = make_ast_node_nonterminal(DefList);
    add_children_ast_node($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Def: Specifier DecList SEMI {
    $$ = make_ast_node_nonterminal(Def);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | error SEMI { yyerrok; }
    ;
DecList: Dec {
    $$ = make_ast_node_nonterminal(DecList);
    add_children_ast_node($$, 1, $1);
    }
    | Dec COMMA DecList {
    $$ = make_ast_node_nonterminal(DecList);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    ;
Dec: VarDec {
    $$ = make_ast_node_nonterminal(Dec);
    add_children_ast_node($$, 1, $1);
    }
    | VarDec ASSIGNOP Exp {
    $$ = make_ast_node_nonterminal(Dec);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    ;

/* Expressions */
Exp: Exp ASSIGNOP Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp AND Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp OR Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp RELOP Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp PLUS Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp MINUS Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp STAR Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp DIV Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | LP Exp RP {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    $$->left_value = DEPENDS_ON_CHILD;
    $$->left_value_depends_child = 1;
    }
    | MINUS Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 2, $1, $2);
    }
    | NOT Exp {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 2, $1, $2);
    }
    | ID LP Args RP {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp LB Exp RB {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 4, $1, $2, $3, $4);
    $$->left_value = true;
    }
    | Exp DOT ID {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 3, $1, $2, $3);
    $$->left_value = DEPENDS_ON_CHILD;
    $$->left_value_depends_child = 0;
    }
    | ID {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 1, $1);
    $$->left_value = DEPENDS_ON_CHILD;
    $$->left_value_depends_child = 0;
    }
    | INT {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 1, $1);
    }
    | FLOAT {
    $$ = make_ast_node_nonterminal(Exp);
    add_children_ast_node($$, 1, $1);
    }
    | error RP { yyerrok; }
    ;
Args: Exp COMMA Args {
    $$ = make_ast_node_nonterminal(Args);
    add_children_ast_node($$, 3, $1, $2, $3);
    }
    | Exp {
    $$ = make_ast_node_nonterminal(Args);
    add_children_ast_node($$, 1, $1);
    }
    ;

%%
/*
#include "lex.yy.c"
int main() {
    yyparse();
}
yyerror(char* msg) {
    printf("Error: %s\n", msg);
}*/

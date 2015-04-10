%{
#include <stdlib.h>
#define YYSTYPE Node*
#include "grammer_tree.h"
extern int yylineno;
%}
%locations
%define parse.lac full
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
    $$ = make_node_nonterminal(Program, yylineno);
    root = $$;
    add_child($$, $1);
    }
    ;
ExtDefList: ExtDef ExtDefList {
    $$ = make_node_nonterminal(ExtDefList, yylineno);
    add_children($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
ExtDef: Specifier ExtDecList SEMI {
    $$ = make_node_nonterminal(ExtDef, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Specifier SEMI {
    $$ = make_node_nonterminal(ExtDef, yylineno);
    add_children($$, 2, $1, $2);
    }
    | Specifier FunDec CompSt {
    $$ = make_node_nonterminal(ExtDef, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;
ExtDecList: VarDec {
    $$ = make_node_nonterminal(ExtDecList, yylineno);
    add_child($$, $1);
    }
    | VarDec COMMA ExtDecList {
    $$ = make_node_nonterminal(ExtDecList, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;

/* Specifiers */
Specifier: TYPE {
    $$ = make_node_nonterminal(Specifier, yylineno);
    add_child($$, $1);
    }
    | StructSpecifier {
    $$ = make_node_nonterminal(Specifier, yylineno);
    add_child($$, $1);
    }
    ;
StructSpecifier: STRUCT OptTag LC DefList RC {
    $$ = make_node_nonterminal(StructSpecifier, yylineno);
    add_children($$, 5, $1, $2, $3, $4, $5);
    }
    | STRUCT Tag {
    $$ = make_node_nonterminal(StructSpecifier, yylineno);
    add_children($$, 2, $1, $2);
    }
    ;
OptTag: ID {
    $$ = make_node_nonterminal(ID, yylineno);
    add_child($$, $1);
    }
    | /* empty */ { $$ = NULL; }
    ;
Tag: ID {
    $$ = make_node_nonterminal(Tag, yylineno);
    add_child($$, $1);
    }
    ;

/* Declarators */
VarDec: ID {
    $$ = make_node_nonterminal(VarDec, yylineno);
    add_child($$, $1);
    }
    | VarDec LB INT RB {
    $$ = make_node_nonterminal(VarDec, yylineno);
    add_children($$, 4, $1, $2, $3, $4);
    }
    ;
FunDec: ID LP VarList RP {
    $$ = make_node_nonterminal(FunDec, yylineno);
    add_children($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
    $$ = make_node_nonterminal(FunDec, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;
VarList: ParamDec COMMA VarList {
    $$ = make_node_nonterminal(VarList, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | ParamDec {
    $$ = make_node_nonterminal(VarList, yylineno);
    add_child($$, $1);
    }
    ;
ParamDec: Specifier VarDec {
    $$ = make_node_nonterminal(ParamDec, yylineno);
    add_children($$, 2, $1, $2);
    }
    ;

/* Statements */
CompSt: LC DefList StmtList RC {
    $$ = make_node_nonterminal(CompSt, yylineno);
    add_children($$, 4, $1, $2, $3, $4);
    }
    ;
StmtList: Stmt StmtList {
    $$ = make_node_nonterminal(StmtList, yylineno);
    add_children($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Stmt: Exp SEMI {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_children($$, 2, $1, $2);
    }
    | CompSt {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_child($$, $1);
    }
    | RETURN Exp SEMI {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | IF LP Exp RP Stmt {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_children($$, 5, $1, $2, $3, $4, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt %prec LOWER_THAN_ELSE {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_children($$, 7, $1, $2, $3, $4, $5, $6, $7);
    }
    | WHILE LP Exp RP Stmt {
    $$ = make_node_nonterminal(Stmt, yylineno);
    add_children($$, 5, $1, $2, $3, $4, $5);
    }
    ;

/* Local Definitions */
DefList: Def DefList {
    $$ = make_node_nonterminal(DefList, yylineno);
    add_children($$, 2, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Def: Specifier DecList SEMI {
    $$ = make_node_nonterminal(Def, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;
DecList: Dec {
    $$ = make_node_nonterminal(DecList, yylineno);
    add_children($$, 1, $1);
    }
    | Dec COMMA DecList {
    $$ = make_node_nonterminal(DecList, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;
Dec: VarDec {
    $$ = make_node_nonterminal(Dec, yylineno);
    add_children($$, 1, $1);
    }
    | VarDec ASSIGNOP Exp {
    $$ = make_node_nonterminal(Dec, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    ;

/* Expressions */
Exp: Exp ASSIGNOP Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp AND Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp OR Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp RELOP Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp PLUS Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp MINUS Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp STAR Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp DIV Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | LP Exp RP {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | MINUS Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 2, $1, $2);
    }
    | NOT Exp {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 2, $1, $2);
    }
    | ID LP Args RP {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp LB Exp RB {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 4, $1, $2, $3, $4);
    }
    | Exp DOT ID {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | ID {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 1, $1);
    }
    | INT {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 1, $1);
    }
    | FLOAT {
    $$ = make_node_nonterminal(Exp, yylineno);
    add_children($$, 1, $1);
    }
    ;
Args: Exp COMMA Args {
    $$ = make_node_nonterminal(Args, yylineno);
    add_children($$, 3, $1, $2, $3);
    }
    | Exp {
    $$ = make_node_nonterminal(Args, yylineno);
    add_children($$, 1, $1);
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

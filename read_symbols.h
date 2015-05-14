#ifndef _READ_SYMBOLS_H
#define _READ_SYMBOLS_H

#include "symbol_table.h"
#include "ast.h"
#include "symbol_type.h"

void read_symbols();
void dfs(struct ast_node *root);
void dfs_ext_def(struct ast_node *root);
void dfs_ext_dec_list(struct ast_node *root, struct var_type *t);
void dfs_fun_dec(struct ast_node *root, struct var_type *ret);
struct func_param_list *dfs_var_list(struct ast_node *root);
struct var_type *dfs_param_dec(struct ast_node *root);
struct var_type *dfs_var_dec(struct ast_node *root, char **name, struct var_type *vt);
struct var_type *dfs_specifier(struct ast_node *root);
struct struct_type *dfs_struct_specifier(struct ast_node *root);
char* dfs_tag(struct ast_node* root);
char* dfs_opt_tag(struct ast_node* root);
char* dfs_id(struct ast_node* root);
struct field_list *dfs_def_list(struct ast_node *root);
struct field_list *dfs_def(struct ast_node *root);
struct field_list *dfs_dec_list(struct ast_node *root, struct var_type *vt);
struct field_list *dfs_dec(struct ast_node *root, struct var_type *vt);

#endif

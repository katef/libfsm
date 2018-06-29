#ifndef RE_PRINT_H
#define RE_PRINT_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "re_ast.h"

#define PRETTYPRINT_AST 1

void
re_ast_print(FILE *f, struct ast_re *ast);

#endif

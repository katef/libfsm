/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_PRINT_H
#define RE_PRINT_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "re_ast.h"

#define PRETTYPRINT_AST 0

void
re_ast_print(FILE *f, struct ast_re *ast);

#endif

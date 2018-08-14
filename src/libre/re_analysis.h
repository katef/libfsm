/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_ANALYSIS_H
#define RE_ANALYSIS_H

#include "re_ast.h"

enum re_analysis_res {
	RE_ANALYSIS_OK,

	/* This is returned if analysis finds a combination of
	 * requirements that are inherently unsatisfiable -- most
	 * notably, multiple start or end anchors that aren't
	 * nullable / in different alternatives. */
	RE_ANALYSIS_UNSATISFIABLE,
	RE_ANALYSIS_ERROR_NULL = -1,
	RE_ANALYSIS_ERROR_MEMORY = -2
};

enum re_analysis_res
re_ast_analysis(struct ast_re *ast);

#endif

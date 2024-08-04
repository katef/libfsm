/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_AST_ANALYSIS_H
#define RE_AST_ANALYSIS_H

/*
 * Analysis
 */

struct ast;

enum ast_analysis_res {
	AST_ANALYSIS_OK,

	/*
	 * This is returned if analysis finds a combination of
	 * requirements that are inherently unsatisfiable.
	 *
	 * For example, multiple start/end anchors that aren't
	 * in nullable groups or different alts.
	 * While this naming leads to a double-negative, we can't prove
	 * that the regex is satisfiable, just flag it when it isn't.
	 */
	AST_ANALYSIS_UNSATISFIABLE,

	AST_ANALYSIS_ERROR_NULL   = -1,
	AST_ANALYSIS_ERROR_MEMORY = -2,
	AST_ANALYSIS_ERROR_UNSUPPORTED = -3
};

enum ast_analysis_res
ast_analysis(struct ast *ast, enum re_flags flags);

int
ast_rewrite(struct ast *ast, enum re_flags flags);

#endif

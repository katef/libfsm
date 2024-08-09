/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "lx/print.h"

void
lx_print_dump(FILE *f, const struct ast *ast, const struct fsm_options *opt)
{
	assert(f != NULL);
	assert(ast != NULL);
	assert(opt != NULL);

	(void) ast;

	if (api_getc == API_FDGETC) {
		fprintf(f, "#define _POSIX_SOURCE\n");
		fprintf(f, "\n");
	}

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdlib.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "#include <string.h>\n");
	fprintf(f, "#include <ctype.h>\n");
	fprintf(f, "\n");

	fprintf(f, "#include LX_HEADER\n");
	fprintf(f, "\n");

	switch (api_tokbuf) {
	case API_DYNBUF:
		break;

	case API_FIXEDBUF:
		/* TODO: rename buf */
		fprintf(f, "char a[256]; /* XXX: bounds check, and local by lx->tokbuf opaque */\n");
		fprintf(f, "char *p;\n");
		fprintf(f, "\n");
		break;

	default:
		break;
	}

	fprintf(f, "static void\n");
	fprintf(f, "dump_buf(const char *q, size_t l)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tif (q == NULL) {\n");
	fprintf(f, "\t\treturn;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	/* TODO: escaping for special characters */
	fprintf(f, "\tprintf(\" '%%.*s'\", (int) l, q);\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "int\n");
	fprintf(f, "main(int argc, char *argv[])\n");
	fprintf(f, "{\n");

	fprintf(f, "\tenum lx_token t;\n");
	fprintf(f, "\tstruct lx lx = { 0 };\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\tint (*lgetc)(struct lx *lx);\n");
		fprintf(f, "\tvoid *getc_opaque;\n");
		break;

	case FSM_IO_STR:
		break;

	case FSM_IO_PAIR:
		break;
	}

	switch (api_getc) {
	case API_FGETC:
		break;

	case API_SGETC:
		fprintf(f, "\tconst char *s;\n");
		break;

	case API_AGETC:
		fprintf(f, "\tstruct lx_arr arr;\n");
		break;

	case API_FDGETC:
		fprintf(f, "\tstruct lx_fd d;\n");
		fprintf(f, "\tchar readbuf[BUFSIZ];\n");
		break;
	}

	switch (api_tokbuf) {
	case API_DYNBUF:
		fprintf(f, "\tstruct lx_dynbuf buf;\n");
		break;

	case API_FIXEDBUF:
		fprintf(f, "\tstruct lx_fixedbuf buf;\n");
		break;

	default:
		break;
	}

	fprintf(f, "\n");

	if (opt->io == FSM_IO_GETC && (api_getc != API_AGETC && api_getc != API_SGETC)) {
		fprintf(f, "\tif (argc != 1) {\n");
		fprintf(f, "\t\tfprintf(stderr, \"usage: dump\\n\");\n");
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t(void) argv;\n");
		fprintf(f, "\n");
	} else {
		fprintf(f, "\tif (argc != 2) {\n");
		fprintf(f, "\t\tfprintf(stderr, \"usage: dump <str>\\n\");\n");
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
	}

	switch (api_getc) {
	case API_FGETC:
		fprintf(f, "\tlgetc = lx_fgetc;\n");
		fprintf(f, "\tgetc_opaque = stdin;\n");
		fprintf(f, "\n");
		break;

	case API_SGETC:
		fprintf(f, "\ts = argv[1];\n");
		fprintf(f, "\n");

		fprintf(f, "\tlgetc = lx_sgetc;\n");
		fprintf(f, "\tgetc_opaque = &s;\n");
		fprintf(f, "\n");
		break;

	case API_AGETC:
		fprintf(f, "\tarr.p   = argv[1];\n");
		fprintf(f, "\tarr.len = strlen(arr.p);\n");
		fprintf(f, "\n");

		fprintf(f, "\tlgetc = lx_agetc;\n");
		fprintf(f, "\tgetc_opaque = &arr;\n");
		fprintf(f, "\n");
		break;

	case API_FDGETC:
		fprintf(f, "\td.buf   = readbuf;\n");
		fprintf(f, "\td.bufsz = sizeof readbuf;\n");
		fprintf(f, "\td.p     = d.buf;\n");
		fprintf(f, "\td.len   = 0;\n");
		fprintf(f, "\td.fd    = fileno(stdin);\n");
		fprintf(f, "\n");

		fprintf(f, "\tlgetc = lx_dgetc;\n");
		fprintf(f, "\tgetc_opaque = &d;\n");
		fprintf(f, "\n");
		break;
	}

	fprintf(f, "\tlx_init(&lx);\n");
	fprintf(f, "\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\tlx.lgetc       = lgetc;\n");
		fprintf(f, "\tlx.getc_opaque = getc_opaque;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tlx.p = argv[1];\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tlx.p = argv[1];\n");
		fprintf(f, "\tlx.e = argv[1] + strlen(argv[1]);\n");
		fprintf(f, "\n");
		break;
	}

	switch (api_tokbuf) {
	case API_DYNBUF:
		fprintf(f, "\tbuf.a   = NULL;\n");
		fprintf(f, "\tbuf.len = 0;\n");
		fprintf(f, "\n");

		fprintf(f, "\tlx.buf_opaque = &buf;\n");
		fprintf(f, "\tlx.push       = lx_dynpush;\n");
		fprintf(f, "\tlx.clear      = lx_dynclear;\n");
		fprintf(f, "\tlx.free       = lx_dynfree;\n");
		fprintf(f, "\n");
		break;

	case API_FIXEDBUF:
		fprintf(f, "\tbuf.a   = a;\n");
		fprintf(f, "\tbuf.p   = buf.a;\n");
		fprintf(f, "\tbuf.len = sizeof a;\n"); /* XXX: rename .len to .size */
		fprintf(f, "\n");

		fprintf(f, "\tlx.buf_opaque = &buf;\n");
		fprintf(f, "\tlx.push       = lx_fixedpush;\n");
		fprintf(f, "\tlx.clear      = lx_fixedclear;\n");
		fprintf(f, "\tlx.free       = NULL;\n");
		fprintf(f, "\n");
		break;
	}

	fprintf(f, "\tdo {\n");
	fprintf(f, "\t\tsize_t l;\n");
	fprintf(f, "\t\tconst char *q;\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tt = lx_next(&lx);\n");
	fprintf(f, "\n");

	switch (api_tokbuf) {
	case API_DYNBUF:
		fprintf(f, "\t\tl = buf.len;\n");
		fprintf(f, "\t\tq = buf.a;\n");
		fprintf(f, "\n");
		break;

	case API_FIXEDBUF:
		fprintf(f, "\t\tl = buf.len;\n");
		fprintf(f, "\t\tq = buf.a;\n");
		fprintf(f, "\n");
		break;

	default:
		fprintf(f, "\t\tl = 0;\n");
		fprintf(f, "\t\tq = NULL;\n");
		fprintf(f, "\n");
		break;
	}

	if (~api_exclude & API_POS) {
		fprintf(f, "\t\tprintf(\"%%u\", lx.start.byte);\n");
		fprintf(f, "\t\tif (lx.end.byte != lx.start.byte) {\n");
		fprintf(f, "\t\t\tprintf(\"-%%u\", lx.end.byte);\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t\tprintf(\":%%u\", lx.start.line);\n");
		fprintf(f, "\t\tif (lx.end.line != lx.start.line) {\n");
		fprintf(f, "\t\t\tprintf(\"-%%u\", lx.end.line);\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t\tprintf(\",%%u\", lx.start.col);\n");
		fprintf(f, "\t\tif (lx.end.col != lx.start.col) {\n");
		fprintf(f, "\t\t\tprintf(\"-%%u\", lx.end.col);\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t\tprintf(\": \");\n");
		fprintf(f, "\n");
	}

	fprintf(f, "\t\tswitch (t) {\n");
	fprintf(f, "\t\tcase TOK_EOF:\n");
	fprintf(f, "\t\t\tprintf(\"<EOF>\\n\");\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tcase TOK_ERROR:\n");
	fprintf(f, "\t\t\tperror(\"lx_next\");\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tcase TOK_UNKNOWN:\n");
	fprintf(f, "\t\t\tprintf(\"lexically uncategorised:\");\n");
	fprintf(f, "\t\t\tdump_buf(q, l);\n");
	fprintf(f, "\t\t\tprintf(\"\\n\");\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");

	fprintf(f, "\t\tdefault:\n");
	if (~api_exclude & API_NAME) {
		fprintf(f, "\t\t\tprintf(\"<%%s\", lx_name(t));\n");
		fprintf(f, "\t\t\tdump_buf(q, l);\n");
		fprintf(f, "\t\t\tprintf(\">\\n\");\n");
	} else {
		fprintf(f, "\t\t\tprintf(\"<#%%u>\\n\");\n");
		fprintf(f, "\t\t\t\t(unsigned) t);\n");
	}
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");

	fprintf(f, "\t\t}\n");
	fprintf(f, "\n");

	fprintf(f, "\t} while (t != TOK_ERROR && t != TOK_EOF && t != TOK_UNKNOWN);\n");
	fprintf(f, "\n");

	fprintf(f, "\treturn t == TOK_ERROR;\n");
	fprintf(f, "}\n");
}


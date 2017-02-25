/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include "fsm/out.h"

#include "lx/ast.h"
#include "lx/out.h"

static void
out_dump(FILE *f)
{
	assert(f != NULL);

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdlib.h>\n");
	fprintf(f, "#include <stdio.h>\n");
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

	switch (fsm_io) {
	case FSM_IO_GETC:
		fprintf(f, "static int\n");
		fprintf(f, "dump_fgetc(struct lx *lx)\n");
		fprintf(f, "{\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\t(void) lx;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn fgetc(stdin);\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		break;
	}

	fprintf(f, "static void\n");
	fprintf(f, "dump_buf(const char *q, size_t l)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tif (q == NULL) {\n");
	fprintf(f, "\t\treturn;\n");
	fprintf(f, "\t}\n");
	/* TODO: escaping for special characters */
	fprintf(f, "\t\t\tprintf(\" '%%.*s'\", (int) l, q);\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "int\n");
	fprintf(f, "main(int argc, char *argv[])\n");
	fprintf(f, "{\n");
	fprintf(f, "\tenum lx_token t;\n");
	fprintf(f, "\tstruct lx lx = { 0 };\n");
	fprintf(f, "\n");

	switch (api_tokbuf) {
	case API_DYNBUF:
		fprintf(f, "\tstruct lx_dynbuf buf;\n");
		fprintf(f, "\n");
		break;

	case API_FIXEDBUF:
		fprintf(f, "\tstruct lx_fixedbuf buf;\n");
		fprintf(f, "\n");
		break;

	default:
		break;
	}

	switch (fsm_io) {
	case FSM_IO_GETC:
		fprintf(f, "\tif (argc != 1) {\n");
		fprintf(f, "\t\tfprintf(stderr, \"usage: dump\\n\");\n");
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tif (argc != 2) {\n");
		fprintf(f, "\t\tfprintf(stderr, \"usage: dump <str>\\n\");\n");
		fprintf(f, "\t\treturn 1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		break;
	}

	fprintf(f, "\tlx_init(&lx);\n");
	fprintf(f, "\n");

	switch (fsm_io) {
	case FSM_IO_GETC:
		fprintf(f, "\tlx.lgetc  = dump_fgetc;\n");
		fprintf(f, "\tlx.opaque = stdin;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tlx.p = argv[1];\n");
		fprintf(f, "\n");
		break;
	}

	switch (api_tokbuf) {
	case API_DYNBUF:
		fprintf(f, "\tbuf.a   = NULL;\n");
		fprintf(f, "\tbuf.len = 0;\n");
		fprintf(f, "\n");
		fprintf(f, "\tlx.buf   = &buf;\n");
		fprintf(f, "\tlx.push  = lx_dynpush;\n");
		fprintf(f, "\tlx.pop   = lx_dynpop;\n");
		fprintf(f, "\tlx.clear = lx_dynclear;\n");
		fprintf(f, "\tlx.free  = lx_dynfree;\n");
		fprintf(f, "\n");
		break;

	case API_FIXEDBUF:
		fprintf(f, "\tbuf.a   = a;\n");
		fprintf(f, "\tbuf.len = sizeof a;\n"); /* XXX: rename .len to .size */
		fprintf(f, "\n");
		fprintf(f, "\tlx.buf   = &buf;\n");
		fprintf(f, "\tlx.push  = lx_fixedpush;\n");
		fprintf(f, "\tlx.pop   = lx_fixedpop;\n");
		fprintf(f, "\tlx.clear = lx_fixedclear;\n");
		fprintf(f, "\tlx.free  = NULL;\n");
		fprintf(f, "\n");
		break;
	}

	fprintf(f, "\tdo {\n");
	fprintf(f, "\t\tsize_t l;\n");
	fprintf(f, "\t\tconst char *q;\n");
	fprintf(f, "\n");

	switch (api_tokbuf) {
	case API_DYNBUF:
		break;

	case API_FIXEDBUF:
		break;
	}

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
		fprintf(f, "\t\tprintf(\"%%u: \", lx.start.byte);\n");
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

void
lx_out_dump(const struct ast *ast, FILE *f)
{
	(void) ast;

	out_dump(f);
}


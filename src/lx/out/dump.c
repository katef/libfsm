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

	/* TODO: only if there's no buffer */
	fprintf(f, "char a[256]; /* XXX: bounds check, and local by lx->tokbuf opaque */\n");
	fprintf(f, "char *p;\n");
	fprintf(f, "\n");

	fprintf(f, "static int\n");
	fprintf(f, "push(struct lx *lx, char c)\n");
	fprintf(f, "{\n");
	fprintf(f, "\t(void) lx;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(c != EOF);\n");
	fprintf(f, "\n");
	fprintf(f, "\t*p++ = c;\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "static void\n");
	fprintf(f, "pop(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\t(void) lx;\n");
	fprintf(f, "\n");
	fprintf(f, "\tp--;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	switch (fsm_io) {
	case FSM_IO_GETC:
		fprintf(f, "static int\n");
		fprintf(f, "lx_fgetc(struct lx *lx)\n");
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

	fprintf(f, "int\n");
	fprintf(f, "main(int argc, char *argv[])\n");
	fprintf(f, "{\n");
	fprintf(f, "\tenum lx_token t;\n");
	fprintf(f, "\tstruct lx lx = { 0 };\n");
	fprintf(f, "\n");

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
		fprintf(f, "\tlx.lgetc  = lx_fgetc;\n");
		fprintf(f, "\tlx.opaque = stdin;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tlx.p = argv[1];\n");
		fprintf(f, "\n");
		break;
	}

	fprintf(f, "\tlx.push = push;\n");
	fprintf(f, "\tlx.pop  = pop;\n");
	fprintf(f, "\n");

	fprintf(f, "\tdo {\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tp = a;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tt = lx_next(&lx);\n");
	fprintf(f, "\t\tswitch (t) {\n");
	fprintf(f, "\t\tcase TOK_EOF:\n");
	fprintf(f, "\t\t\tprintf(\"%%u: <EOF>\\n\", lx.start.byte);\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tcase TOK_ERROR:\n");
	fprintf(f, "\t\t\tperror(\"lx_next\");\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tcase TOK_UNKNOWN:\n");
	fprintf(f, "\t\t\tfprintf(stderr, \"%%u: lexically uncategorised: '%%.*s'\\n\",\n");
	fprintf(f, "\t\t\t\tlx.start.byte,\n");
	fprintf(f, "\t\t\t\t(int) (p - a), a);\n");
	fprintf(f, "\t\t\tbreak;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tdefault:\n");
	fprintf(f, "\t\t\tprintf(\"%%u: <%%s '%%.*s'>\\n\",\n");
	fprintf(f, "\t\t\t\tlx.start.byte,\n");
	fprintf(f, "\t\t\t\tlx_name(t),\n");
	fprintf(f, "\t\t\t\t(int) (p - a), a);\n");
	fprintf(f, "\t\t\tbreak;\n");
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


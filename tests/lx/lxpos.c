#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include <assert.h>
#include <err.h>
#include <errno.h>

#include "lxpos_lexer.h"

#define MAX_LINE 4096

struct get_char_env {
    struct lx lx;
    struct lx_dynbuf dbuf;

    char buf[MAX_LINE];
};

#define MAX_TOKENS 16

struct pos {
    unsigned s;
    unsigned e;
};

struct exp_line {
    const char *input;
    unsigned count;
    unsigned base;
    struct exp_token {
        enum lx_token tok;
        struct pos line;
        struct pos col;
        struct pos byte;
        const char *str;
    } tokens[MAX_TOKENS];
};

const struct exp_line lines[] = {
    { "hello `world\n", 3, 0,
      {
          { TOK_IDENT,  {  1,   1}, {  1,   6}, {  0,   5},  "hello" },
          { TOK_SYMBOL, {  1,   1}, {  7,  13}, {  6,  12},  "`world" },
          { TOK_NL,     {  1,   2}, { 13,   1}, { 12,  13},  "\\n" },
      },
    },

    /* Same line again; check that the line and byte
     * positions account for the previous line. */
    { "hello `world\n", 3, 13,
      {
          { TOK_IDENT,  {  2,   2}, {  1,   6}, {  0,   5},  "hello" },
          { TOK_SYMBOL, {  2,   2}, {  7,  13}, {  6,  12},  "`world" },
          { TOK_NL,     {  2,   3}, { 13,   1}, { 12,  13},  "\\n" },
      },
    },

    /* dense, ASCII APL-ish expression */
    { "`s$1e6+3 3#!9\n", 10, 26,
      {
          { TOK_SYMBOL, {  3,   3}, {  1,   3}, {  0,   2},  "`s" },
          { TOK_OP,     {  3,   3}, {  3,   4}, {  2,   3},  "$" },
          { TOK_FLOAT,  {  3,   3}, {  4,   7}, {  3,   6},  "1e6" },
          { TOK_OP,     {  3,   3}, {  7,   8}, {  6,   7},  "+" },
          { TOK_INT,    {  3,   3}, {  8,   9}, {  7,   8},  "3" },
          { TOK_INT,    {  3,   3}, { 10,  11}, {  9,  10},  "3" },
          { TOK_OP,     {  3,   3}, { 11,  12}, { 10,  11},  "#" },
          { TOK_OP,     {  3,   3}, { 12,  13}, { 11,  12},  "!" },
          { TOK_INT,    {  3,   3}, { 13,  14}, { 12,  13},  "9" },
          { TOK_NL,     {  3,   4}, { 14,   1}, { 13,  14},  "\\n" },
      },
    },

    /* input with internal comment, checking NL pos */
    { "d: r * t /*distance = rate * time*/\n", 6, 40,
      {
          { TOK_IDENT,  {  4,   4}, {  1,   2}, {  0,   1},  "d" },
          { TOK_OP,     {  4,   4}, {  2,   3}, {  1,   2},  ":" },
          { TOK_IDENT,  {  4,   4}, {  4,   5}, {  3,   4},  "r" },
          { TOK_OP,     {  4,   4}, {  6,   7}, {  5,   6},  "*" },
          { TOK_IDENT,  {  4,   4}, {  8,   9}, {  7,   8},  "t" },
          { TOK_NL,     {  4,   5}, { 36,   1}, { 35,  36},  "\\n" },
      },
    },

    /* input with end of line comment, again checking NL pos, and then
     * another identifier after the NL (outside the comment zone)
     * to check that the lexer exited the line comment zone properly. */
   { "x[&x<3] // find where x < 3\nx", 9, 76,
      {
          { TOK_IDENT,     {   5,   5}, {  1,   2}, {  0,   1},  "x" },
          { TOK_BRACKET_L, {   5,   5}, {  2,   3}, {  1,   2},  "[" },
          { TOK_OP,        {   5,   5}, {  3,   4}, {  2,   3},  "&" },
          { TOK_IDENT,     {   5,   5}, {  4,   5}, {  3,   4},  "x" },
          { TOK_OP,        {   5,   5}, {  5,   6}, {  4,   5},  "<" },
          { TOK_INT,       {   5,   5}, {  6,   7}, {  5,   6},  "3" },
          { TOK_BRACKET_R, {   5,   5}, {  7,   8}, {  6,   7},  "]" },
          { TOK_NL,        {   5,   6}, { 28,   1}, { 27,  28},  "\\n" },
          { TOK_IDENT,     {   6,   6}, {  1,   2}, { 28,  29},  "x" },
      },
    },

    /* Multi-line input. */
    { "x\ny\nz\n", 6, 105,
      {
          { TOK_IDENT,  {  6,   6}, {  1,   2}, {  0,   1},  "x" },
          { TOK_NL,     {  6,   7}, {  2,   1}, {  1,   2},  "\\n" },
          { TOK_IDENT,  {  7,   7}, {  1,   2}, {  2,   3},  "y" },
          { TOK_NL,     {  7,   8}, {  2,   1}, {  3,   4},  "\\n" },
          { TOK_IDENT,  {  8,   8}, {  1,   2}, {  4,   5},  "z" },
          { TOK_NL,     {  8,   9}, {  2,   1}, {  5,   6},  "\\n" },
      },
    },
};

int main(int argc, char **argv) {
    int li, res = 0;
    struct get_char_env env;
    memset(&env, 0x00, sizeof(env));

    lx_init(&env.lx);

    env.lx.buf_opaque = &env.dbuf;
    env.lx.push       = lx_dynpush;
    env.lx.clear      = lx_dynclear;
    env.lx.free       = lx_dynfree;

    for (li = 0; li < sizeof(lines)/sizeof(lines[0]); li++) {
        int nth_token = 0;
        const struct exp_line *cur = &lines[li];
        unsigned base;
        printf("\n%d (%lu): %s\n", li, strlen(cur->input), cur->input);

        /* Set input string; the lexer will get successive characters
         * from it, and then EOF (which is really EOS). */
        lx_input_str(&env.lx, cur->input);

        base = cur->base;

        for (;;) {
            enum lx_token t;
            struct pos line, col, byte;
            const struct exp_token *exp;
            const char *str;

#define FAIL()                                  \
            res = 1;                            \
            /* break; */

            t = lx_next(&env.lx);
            line.s = env.lx.start.line;
            line.e = env.lx.end.line;
            col.s = env.lx.start.col;
            col.e = env.lx.end.col;
            byte.s = env.lx.start.byte;
            byte.e = env.lx.end.byte;
            /* Workaround; TOK_NL string can include preceding comment */
            str = t == TOK_NL ? "\\n" : env.dbuf.a;

            if (t == TOK_EOF) {
                printf("    -- EOF at (line [%u,%u], col [%u,%u], byte [%u,%u])\n",
                    line.s, line.e,
                    col.s, col.e,
                    byte.s, byte.e);

                if (nth_token < cur->count) {
                    fprintf(stderr, "FAIL: Expected %u tokens, got %u.\n",
                        cur->count, nth_token);
                    FAIL();
                }
                    
                break;
            }

            if (nth_token >= cur->count) {
                fprintf(stderr, "FAIL: More tokens than expected.\n");
                FAIL();
            }

            printf("    -- %d: %s \"%s\" (line [%u,%u], col [%u,%u], byte [%u,%u])\n",
                nth_token, lx_name(t), str,
                line.s, line.e,
                col.s, col.e,
                byte.s, byte.e);

            exp = &cur->tokens[nth_token];
            if (exp->tok != t) {
                fprintf(stderr, "FAIL token: Expected %s, got %s\n", lx_name(exp->tok), lx_name(t));
                FAIL();
            }

            if (0 != strcmp(exp->str, str)) {
                fprintf(stderr, "FAIL str: Expected %s, got %s\n", exp->str, str);
                FAIL();
            }
#define CMP_U(NAME, EXP, GOT)                                          \
            if (EXP != GOT) {                                          \
                fprintf(stderr, "FAIL %s: Expected %u, got %u\n",      \
                    NAME, EXP, GOT);                                   \
                FAIL();                                                \
            }                                                          \

            CMP_U("line.s", exp->line.s, line.s);
            CMP_U("line.e", exp->line.e, line.e);
            CMP_U("col.s", exp->col.s, col.s);
            CMP_U("col.e", exp->col.e, col.e);
            CMP_U("byte.s", exp->byte.s + base, byte.s);
            CMP_U("byte.e", exp->byte.e + base, byte.e);
#undef CMP_U
#undef FAIL

            nth_token++;
        }
    }

    env.lx.free(env.lx.buf_opaque);
    return res;
}

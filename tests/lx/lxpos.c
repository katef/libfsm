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

struct exp_line {
    const char *input;
    unsigned count;
    struct exp_token {
        enum lx_token tok;
        unsigned line;
        unsigned col;
        unsigned byte_s;
        unsigned byte_e;
        const char *str;
    } tokens[MAX_TOKENS];
};

const struct exp_line lines[] = {
    { "hello `world\n", 3,
      {
          { TOK_IDENT,    1,   1,   0,   5,  "hello" },
          { TOK_SYMBOL,   1,   7,   6,  12,  "`world" },
          { TOK_NL,       1,  13,  12,  13,  "\\n" },
      },
    },

    /* Same line again; check that the line and byte
     * positions account for the previous line. */
    { "hello `world\n", 3,
      {
          { TOK_IDENT,    2,   1,  13,  18,  "hello" },
          { TOK_SYMBOL,   2,   7,  19,  25,  "`world" },
          { TOK_NL,       2,  13,  25,  26,  "\\n" },
      },
    },

    /* dense, ASCII APL-ish expression */
    { "`s$1e6+3 3#!9\n", 10,
      {
          { TOK_SYMBOL,   3,   1,  26,  28,  "`s" },
          { TOK_OP,       3,   3,  28,  29,  "$" },
          { TOK_FLOAT,    3,   4,  29,  32,  "1e6" },
          { TOK_OP,       3,   7,  32,  33,  "+" },
          { TOK_INT,      3,   8,  33,  34,  "3" },
          { TOK_INT,      3,  10,  35,  36,  "3" },
          { TOK_OP,       3,  11,  36,  37,  "#" },
          { TOK_OP,       3,  12,  37,  38,  "!" },
          { TOK_INT,      3,  13,  38,  39,  "9" },
          { TOK_NL,       3,  14,  39,  40,  "\\n" },
      },
    },

    /* input with internal comment, checking NL pos */
    { "d: r * t /*distance = rate * time*/\n", 6,
      {
          { TOK_IDENT,    4,   1,  40,  41,  "d" },
          { TOK_OP,       4,   2,  41,  42,  ":" },
          { TOK_IDENT,    4,   4,  43,  44,  "r" },
          { TOK_OP,       4,   6,  45,  46,  "*" },
          { TOK_IDENT,    4,   8,  47,  48,  "t" },
          { TOK_NL,       4,  36,  75,  76,  "\\n" },
      },
    },

    /* input with end of line comment, again checking NL pos */
    { "x[&x<3] // find where x < 3\n", 8,
      {
          { TOK_IDENT,     5,   1,  76,  77,  "x" },
          { TOK_BRACKET_L, 5,   2,  77,  78,  "[" },
          { TOK_OP,        5,   3,  78,  79,  "&" },
          { TOK_IDENT,     5,   4,  79,  80,  "x" },
          { TOK_OP,        5,   5,  80,  81,  "<" },
          { TOK_INT,       5,   6,  81,  82,  "3" },
          { TOK_BRACKET_R, 5,   7,  82,  83,  "]" },
          { TOK_NL,        5,  28, 103, 104,  "\\n" },
      },
    },

    /* TODO: Multi-line string, and check that the lexer zone
     *     has been reset from the EOL comment above. */
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
        printf("%d (%lu): %s", li, strlen(cur->input), cur->input);

        /* Set input string; the lexer will get successive characters
         * from it, and then EOF (which is really EOS). */
        env.lx.p = cur->input;

        for (;;) {
            enum lx_token t;
            unsigned line, col, byte_s, byte_e;
            const struct exp_token *exp;
            const char *str;

#define FAIL() res = 1; break;

            t = lx_next(&env.lx);
            line = env.lx.start.line;
            col = env.lx.start.col;
            byte_s = env.lx.start.byte;
            byte_e = env.lx.end.byte;
            /* Workaround; TOK_NL string can include preceding comment */
            str = t == TOK_NL ? "\\n" : env.dbuf.a;

            if (t == TOK_EOF) {
                if (nth_token < cur->count) {
                    fprintf(stderr, "FAIL: Expected %u tokens, got %u.\n",
                        cur->count, nth_token);
                    FAIL();
                }
                printf("    -- EOF at (line %u, col %u, byte %u)\n",
                    line, col, byte_e);
                    
                break;
            }

            if (nth_token >= cur->count) {
                fprintf(stderr, "FAIL: More tokens than expected.\n");
                FAIL();
            }

            printf("    -- %d: %s \"%s\" (line %u, col %u, byte %u ~ %u)\n",
                nth_token, lx_name(t),
                str, line, col, byte_s, byte_e);

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

            CMP_U("line", exp->line, line);
            CMP_U("col", exp->col, col);
            CMP_U("byte_s", exp->byte_s, byte_s);
            CMP_U("byte_e", exp->byte_e, byte_e);
#undef CMP_U
#undef FAIL

            nth_token++;
        }
    }

    env.lx.free(env.lx.buf_opaque);
    return res;
}

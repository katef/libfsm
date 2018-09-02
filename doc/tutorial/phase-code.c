#include <assert.h>
#include <stdio.h>

int
fsm_main(int (*fsm_getc)(void *opaque), void *opaque)
{
	int c;

	assert(fsm_getc != NULL);

	enum {
		S0, S1, S2, S3, S4
	} state;

	state = S0;

	while (c = fsm_getc(opaque), c != EOF) {
		switch (state) {
		case S0: /* start */
			switch ((unsigned char) c) {
			case 'a': state = S1; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S1: /* e.g. "a" */
			switch ((unsigned char) c) {
			case 'b': state = S2; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S2: /* e.g. "ab" */
			switch ((unsigned char) c) {
			case 'b': break;
			case 'c': state = S4; break;
			default: state = S3; break;
			}
			break;

		case S3: /* e.g. "aba" */
			return TOK_UNKNOWN;

		case S4: /* e.g. "abc" */
			state = S3; break;

		default:
			; /* unreached */
		}
	}

	/* end states */
	switch (state) {
	case S2: return 0x1; /* "^ab+c?.?$" */
	case S3: return 0x1; /* "^ab+c?.?$" */
	case S4: return 0x1; /* "^ab+c?.?$" */
	default: return EOF; /* unexpected EOF */
	}
}


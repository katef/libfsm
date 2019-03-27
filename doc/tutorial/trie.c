int
fsm_main(int (*fsm_getc)(void *opaque), void *opaque)
{
	int c;

	enum {
		S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, 
		S10, S11, S12, S13, S14, S15, S16, S17, S18, S19, 
		S20, S21, S22
	} state;

	state = S0;

	while (c = fsm_getc(opaque), c != EOF) {
		switch (state) {
		case S0: /* start */
			switch ((unsigned char) c) {
			case 'f': state = S1; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S1: /* e.g. "f" */
			switch ((unsigned char) c) {
			case 'i': state = S2; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S2: /* e.g. "fi" */
			switch ((unsigned char) c) {
			case 'n': state = S3; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S3: /* e.g. "fin" */
			switch ((unsigned char) c) {
			case 'e': state = S4; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S4: /* e.g. "fine" */
			switch ((unsigned char) c) {
			case 's': state = S9; break;
			case 'n': state = S7; break;
			case 'r': state = S8; break;
			case 'd': state = S5; break;
			case 'l': state = S6; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S5: /* e.g. "fined" */
			return TOK_UNKNOWN;

		case S6: /* e.g. "finel" */
			switch ((unsigned char) c) {
			case 'y': state = S11; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S7: /* e.g. "finen" */
			switch ((unsigned char) c) {
			case 'e': state = S10; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S8: /* e.g. "finer" */
			switch ((unsigned char) c) {
			case 'y': state = S14; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S9: /* e.g. "fines" */
			switch ((unsigned char) c) {
			case 's': state = S15; break;
			case 't': state = S16; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S10: /* e.g. "finene" */
			switch ((unsigned char) c) {
			case 's': state = S12; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S11: /* e.g. "finely" */
			return TOK_UNKNOWN;

		case S12: /* e.g. "finenes" */
			switch ((unsigned char) c) {
			case 's': state = S13; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S13: /* e.g. "fineness" */
			return TOK_UNKNOWN;

		case S14: /* e.g. "finery" */
			return TOK_UNKNOWN;

		case S15: /* e.g. "finess" */
			switch ((unsigned char) c) {
			case 'e': state = S17; break;
			case 'i': state = S18; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S16: /* e.g. "finest" */
			return TOK_UNKNOWN;

		case S17: /* e.g. "finesse" */
			switch ((unsigned char) c) {
			case 'd': state = S21; break;
			case 's': state = S22; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S18: /* e.g. "finessi" */
			switch ((unsigned char) c) {
			case 'n': state = S19; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S19: /* e.g. "finessin" */
			switch ((unsigned char) c) {
			case 'g': state = S20; break;
			default:  return TOK_UNKNOWN;
			}
			break;

		case S20: /* e.g. "finessing" */
			return TOK_UNKNOWN;

		case S21: /* e.g. "finessed" */
			return TOK_UNKNOWN;

		case S22: /* e.g. "finesses" */
			return TOK_UNKNOWN;

		default:
			; /* unreached */
		}
	}

	/* end states */
	switch (state) {
	case S4: return 0x1; /* "fine" */
	case S5: return 0x2; /* "fined" */
	case S8: return 0x10; /* "finer" */
	case S9: return 0x40; /* "fines" */
	case S11: return 0x4; /* "finely" */
	case S13: return 0x8; /* "fineness" */
	case S14: return 0x20; /* "finery" */
	case S16: return 0x800; /* "finest" */
	case S17: return 0x80; /* "finesse" */
	case S20: return 0x400; /* "finessing" */
	case S21: return 0x100; /* "finessed" */
	case S22: return 0x200; /* "finesses" */
	default: return -1; /* unexpected EOT */
	}
}


/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef CAPTURE_VM_PROGRAM_H
#define CAPTURE_VM_PROGRAM_H

#include <stdint.h>

struct capvm_program {
	uint32_t capture_count;
	uint32_t capture_base;

	uint32_t used;
	uint32_t ceil;
	struct capvm_opcode {
		enum capvm_opcode_type {
			/* Next character of input == .u.chr */
			CAPVM_OP_CHAR,
			/* Next character of input is in char class */
			CAPVM_OP_CHARCLASS,
			/* Input has matched */
			CAPVM_OP_MATCH,
			/* Unconditional jump */
			CAPVM_OP_JMP,
			/* If destination has already been evaulated
			 * since advancing the input position, fall
			 * through to next instruction, otherwise jmp. */
			CAPVM_OP_JMP_ONCE,
			/* Split execution to two paths, where .cont
			 * offset is greedier than .new's offset. */
			CAPVM_OP_SPLIT,
			/* Save current input position as capture bound */
			CAPVM_OP_SAVE,
			/* Check if current input position is at start/end
			 * of input, after accounting for PCRE's special
			 * cases for a trailing newline. */
			CAPVM_OP_ANCHOR,
		} t;
		union {
			uint8_t chr;
			uint32_t charclass_id;
			uint32_t jmp;	     /* absolute */
			uint32_t jmp_once;   /* absolute */
			struct {
				uint32_t cont; /* greedy branch */
				uint32_t new;  /* non-greedy branch */
			} split;
			/* (save >> 1): capture ID,
			 * (save & 0x01): save pos to start (0b0) or end (0b1). */
			uint32_t save;
			enum capvm_anchor_type {
				CAPVM_ANCHOR_START,
				CAPVM_ANCHOR_END,
			} anchor;
		} u;
	} *ops;

	/* Most compiled programs only use a few distinct character
	 * classes (if any), and the data is much larger than the
	 * other instructions, so they are stored in a separate
	 * table and referred to by op->u.charclass_id. */
	struct capvm_char_classes {
		uint32_t count;
		uint32_t ceil;
		struct capvm_char_class {
			uint64_t octets[4]; /* 256-bitset */
		} *sets;
	} char_classes;
};

#endif

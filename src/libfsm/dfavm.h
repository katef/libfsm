/*
 * Copyright 2008-2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_DFAVM_H
#define FSM_DFAVM_H

enum dfavm_op_instr {
	// Stop the VM, mark match or failure
	VM_OP_STOP   = 0,

	// Start of each state: fetch next character.  Indicates
	// match / fail if EOS.
	VM_OP_FETCH  = 1,

	// Branch to another state
	VM_OP_BRANCH = 2,
};

enum dfavm_op_cmp {
	VM_CMP_ALWAYS = 0,
	VM_CMP_LT     = 1,
	VM_CMP_LE     = 2,
	VM_CMP_GE     = 3,
	VM_CMP_GT     = 4,
	VM_CMP_EQ     = 5,
	VM_CMP_NE     = 6,
};

enum dfavm_op_end {
	VM_END_FAIL = 0,
	VM_END_SUCC = 1,
};

enum dfavm_op_dest {
	VM_DEST_NONE  = 0,
	VM_DEST_SHORT = 1,  // 11-bit dest
	VM_DEST_NEAR  = 2,  // 18-bit dest
	VM_DEST_FAR   = 3,  // 32-bit dest
};

struct dfavm_op_ir {
	struct dfavm_op_ir *next;

	/* Holds an integer identifier that's unique for all opcodes in a
	 * dfavm_assembler
	 */
	uint32_t asm_index;

	/* Unique index of the final opcodes, assigned after any IR
	 * optimization.
	 *
	 * This can be used as a temporary by any optimization operation,
	 * but the value cannot be assumed to be set to a valid value when
	 * entering the operation.
	 */
	uint32_t index;

	uint32_t num_incoming; // number of branches to this instruction
	int in_trace;
	int cmp_arg;

	enum dfavm_op_cmp cmp;
	enum dfavm_op_instr instr;

	union {
		struct {
			unsigned state;
			enum dfavm_op_end end_bits;
		} fetch;

		struct {
			enum dfavm_op_end end_bits;
		} stop;

		struct {
			struct dfavm_op_ir *dest_arg;
			uint32_t dest_state;
		} br;
	} u;
};

struct dfavm_vm_op {
	const struct dfavm_op_ir *ir;
	uint32_t offset;

	enum dfavm_op_cmp cmp;
	enum dfavm_op_instr instr;

	union {
		struct {
			unsigned state;
			enum dfavm_op_end end_bits;
		} fetch;

		struct {
			enum dfavm_op_end end_bits;
		} stop;

		struct {
			uint32_t dest_index;

			enum dfavm_op_dest dest;
			int32_t  rel_dest;
		} br;
	} u;

	unsigned char cmp_arg;
	unsigned char num_encoded_bytes;
};

struct dfavm_op_ir_pool;

struct dfavm_assembler {
	struct dfavm_op_ir_pool *pool;
	struct dfavm_op_ir *freelist;

	struct dfavm_op_ir **ops;
	struct dfavm_op_ir *linked;

	struct dfavm_vm_op *instr;
	size_t ninstr;

	size_t nstates;
	size_t start;

	uint32_t nbytes;
	uint32_t count;
};

enum dfavm_io_result {
	DFAVM_IO_OK = 0,
	DFAVM_IO_ERROR_WRITING,
	DFAVM_IO_ERROR_READING,
	DFAVM_IO_OUT_OF_MEMORY,
	DFAVM_IO_BAD_HEADER,
	DFAVM_IO_UNSUPPORTED_VERSION,
	DFAVM_IO_SHORT_READ,
};

struct dfavm_v1 {
	unsigned char *ops;
	uint32_t len;
};

enum dfavm_state {
	VM_FAIL     = -1,
	VM_MATCHING =  0,
	VM_SUCCESS  =  1,
};

struct vm_state {
	uint32_t pc;
	enum dfavm_state state;
	int fetch_state;
};

struct fsm_dfavm {
	uint8_t version_major;
	uint8_t version_minor;

	union {
		struct dfavm_v1 v1;
	} u;
};

#endif /* FSM_DFAVM_H */

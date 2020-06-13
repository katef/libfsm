/*
 * Copyright 2008-2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_DFAVM_H
#define FSM_DFAVM_H

enum dfavm_instr_bits {
	// Stop the VM, mark match or failure
	VM_OP_STOP   = 0,

	// Start of each state: fetch next character.  Indicates
	// match / fail if EOS.
	VM_OP_FETCH  = 1,

	// Branch to another state
	VM_OP_BRANCH = 2,
};

enum dfavm_cmp_bits {
	VM_CMP_ALWAYS = 0,
	VM_CMP_LT     = 1,
	VM_CMP_LE     = 2,
	VM_CMP_GE     = 3,
	VM_CMP_GT     = 4,
	VM_CMP_EQ     = 5,
	VM_CMP_NE     = 6,
};

enum dfavm_end_bits {
	VM_END_FAIL = 0,
	VM_END_SUCC = 1,
};

enum dfavm_dest_bits {
	VM_DEST_NONE  = 0,
	VM_DEST_SHORT = 1,  // 11-bit dest
	VM_DEST_NEAR  = 2,  // 18-bit dest
	VM_DEST_FAR   = 3,  // 32-bit dest
};

struct dfavm_op {
	struct dfavm_op *next;
	uint32_t count;
	uint32_t offset;

	uint32_t num_incoming; // number of branches to this instruction

	enum dfavm_cmp_bits cmp;
	enum dfavm_instr_bits instr;

	union {
		struct {
			unsigned state;
			enum dfavm_end_bits end_bits;
		} fetch;

		struct {
			enum dfavm_end_bits end_bits;
		} stop;

		struct {
			struct dfavm_op *dest_arg;
			enum dfavm_dest_bits dest;
			uint32_t dest_state;
			int32_t  rel_dest;
		} br;
	} u;

	unsigned char cmp_arg;
	unsigned char num_encoded_bytes;
	int in_trace;
};

struct dfavm_op_pool {
	struct dfavm_op_pool *next;

	unsigned int top;
	struct dfavm_op ops[1024];
};

struct dfavm_assembler {
	struct dfavm_op_pool *pool;
	struct dfavm_op *freelist;

	struct dfavm_op **ops;
	struct dfavm_op *linked;

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

#endif /* FSM_DFAVM_H */

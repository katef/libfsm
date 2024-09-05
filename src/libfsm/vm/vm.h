/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_VM_H
#define FSM_INTERNAL_VM_H

#define DEBUG_ENCODING     0
#define DEBUG_VM_EXECUTION 0
#define DEBUG_VM_OPCODES   0

#define DFAVM_VARENC_MAJOR 0x00
#define DFAVM_VARENC_MINOR 0x01

#define DFAVM_FIXEDENC_MAJOR 0x00
#define DFAVM_FIXEDENC_MINOR 0x02

#define DFAVM_MAGIC "DFAVM$"

struct ir;
struct ret;
struct ret_list;
struct fsm_vm_compile_opts;
struct dfavm_op_ir_pool;

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

	const char *example;
	const struct ret *ret;

	uint32_t num_incoming:31; // number of branches to this instruction, :31 for packing
	unsigned int in_trace:1;

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

struct dfavm_assembler_ir {
	struct dfavm_op_ir_pool *pool;
	struct dfavm_op_ir *freelist;

	struct dfavm_op_ir **ops;
	struct dfavm_op_ir *linked;

	size_t nstates;
	size_t start;
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

struct dfavm_v2 {
	uint32_t *abuf;
	uint64_t alen;

	uint32_t *ops;
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
		struct dfavm_v2 v2;
	} u;
};

const char * 
cmp_name(int cmp);

int
dfavm_compile_ir(struct dfavm_assembler_ir *a, const struct ir *ir, const struct ret_list *retlist, struct fsm_vm_compile_opts opts);

struct fsm_dfavm *
dfavm_compile_vm(const struct dfavm_assembler_ir *a, struct fsm_vm_compile_opts opts);

void
dfavm_free_vm(struct fsm_dfavm *vm);

void
dfavm_opasm_finalize_op(struct dfavm_assembler_ir *a);

/* v1 */
enum dfavm_io_result
dfavm_v1_save(FILE *f, const struct dfavm_v1 *vm);
enum dfavm_io_result
dfavm_load_v1(FILE *f, struct dfavm_v1 *vm);
struct fsm_dfavm *
encode_opasm_v1(const struct dfavm_vm_op *instr, size_t ninstr, size_t total_bytes);
uint32_t
running_print_op_v1(const unsigned char *ops, uint32_t pc,
	const char *sp, const char *buf, size_t n, char ch, FILE *f);
enum dfavm_state
vm_match_v1(const struct dfavm_v1 *vm, struct vm_state *st, const char *buf, size_t n);

/* v2 */
struct fsm_dfavm *
encode_opasm_v2(const struct dfavm_vm_op *instr, size_t ninstr);
void
running_print_op_v2(const struct dfavm_v2 *vm, uint32_t pc, const char *sp, const char *buf, size_t n, char ch, FILE *f);
enum dfavm_state
vm_match_v2(const struct dfavm_v2 *vm, struct vm_state *st, const char *buf, size_t n);

void
dfavm_v1_finalize(struct dfavm_v1 *vm);

void
dfavm_v2_finalize(struct dfavm_v2 *vm);

#endif


/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

/* Virtual machine for resolving captures while executing regular
 * expressions from a subset of PCRE. This is based on the approach
 * described in Russ Cox's "Regular Expression Matching: the Virtual
 * Machine Approach" (https://swtch.com/~rsc/regexp/regexp2.html), but
 * has a couple major modifications, mainly to keep memory usage low and
 * predictable, and to be more consistent (arguably, bug-compatible...)
 * with PCRE's behavior for libfsm's supported subset of PCRE.
 *
 * Instead of giving each green thread its own copy of the capture
 * buffers, which uses a prohibitive amount of memory when matching DFAs
 * that combine several regexes with several captures each, operate in
 * two passes.
 *
 * In the first pass, each thread keeps track of its execution path,
 * appending a bit for each branch: 1 for the greedy option, 0 for the
 * non-greedy. Since there can be at most one live thread per program
 * instruction, and all of them are either on the current or next input
 * character, there's a bounded window for diverging paths during execution.
 * After a certain distance back all paths either have a common prefix
 * or consist entirely of 0 bits (for continually looping at an unanchored
 * start). The path bits are stored in chunks in a backwards linked list,
 * so nodes for common path prefixes can be shared by multiple threads,
 * and the prefix of all 0 bits is instead stored as a counter. This
 * keeps memory usage substantially lower. This search runs threads in
 * parallel, breadth-first, halting any threads that duplicate work of
 * a greedier search path (since PCRE's results match the greediest).
 *
 * In the second pass, replay the execution path for just the single
 * greediest thread, which represents the "best" match, and write
 * capture offsets into buffers passed in by the caller.
 *
 * Most of the other differences have to do with matching PCRE's quirky
 * behaviors, particularly interactions between newlines and start/end
 * anchors.
 * */

#include "capture_vm.h"
#include "capture_vm_program.h"

#include <adt/alloc.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>

void
fsm_capvm_program_free(const struct fsm_alloc *alloc,
    struct capvm_program *program)
{
	if (program == NULL) { return; }
	f_free(alloc, program->ops);
	f_free(alloc, program->char_classes.sets);
	f_free(alloc, program);
}

struct capvm_program *
capvm_program_copy(const struct fsm_alloc *alloc,
    const struct capvm_program *src)
{
	assert(src != NULL);
	struct capvm_program *p = NULL;
	struct capvm_opcode *ops = NULL;
	struct capvm_char_class *sets = NULL;

	p = f_calloc(alloc, 1, sizeof(*p));
	if (p == NULL) { goto cleanup; }

	/* This allocates exactly as many instructions and char_classes
	 * as necessary, rather than a power-of-2 buffer, because
	 * they are only added during compilation in libre. */

	ops = f_calloc(alloc, src->used, sizeof(ops[0]));
	if (ops == NULL) { goto cleanup; }

	sets = f_calloc(alloc, src->char_classes.count,
	    sizeof(src->char_classes.sets[0]));
	if (sets == NULL) { goto cleanup; }

	memcpy(ops, src->ops, src->used * sizeof(src->ops[0]));

	assert(src->char_classes.sets != NULL || src->char_classes.count == 0);
	if (src->char_classes.count > 0) {
		memcpy(sets, src->char_classes.sets,
		    src->char_classes.count * sizeof(src->char_classes.sets[0]));
	}

	struct capvm_program np = {
		.capture_count = src->capture_count,
		.capture_base = src->capture_base,

		.used = src->used,
		.ceil = src->used,
		.ops = ops,

		.char_classes = {
			.count = src->char_classes.count,
			.ceil = src->char_classes.count,
			.sets = sets,
		},
	};
	memcpy(p, &np, sizeof(np));
	return p;

cleanup:
	f_free(alloc, p);
	f_free(alloc, ops);
	f_free(alloc, sets);
	return NULL;
}

void
capvm_program_rebase(struct capvm_program *program, unsigned capture_offset)
{
	assert(program->capture_base + capture_offset > program->capture_base);
	program->capture_base += capture_offset;
}

void
fsm_capvm_program_dump(FILE *f,
    const struct capvm_program *p)
{
	for (size_t i = 0; i < p->used; i++) {
		const struct capvm_opcode *op = &p->ops[i];
		switch (op->t) {
		case CAPVM_OP_CHAR:
			fprintf(f, "%zu: char 0x%02x (%c)\n",
			    i, op->u.chr, isprint(op->u.chr) ? op->u.chr : '.');
			break;
		case CAPVM_OP_CHARCLASS:
		{
			const uint32_t id = op->u.charclass_id;
			assert(id < p->char_classes.count);
			const struct capvm_char_class *cc = &p->char_classes.sets[id];
			fprintf(f, "%zu: charclass %u -> [", i, id);
			for (size_t i = 0; i < 4; i++) {
				fprintf(f, "%016lx", cc->octets[i]);
			}
			fprintf(f, "]\n");
			break;
		}
		case CAPVM_OP_MATCH:
			fprintf(f, "%zu: match\n", i);
			break;
		case CAPVM_OP_JMP:
			fprintf(f, "%zu: jmp %u\n", i, op->u.jmp);
			break;
		case CAPVM_OP_JMP_ONCE:
			fprintf(f, "%zu: jmp_once %u\n", i, op->u.jmp_once);
			break;
		case CAPVM_OP_SPLIT:
			fprintf(f, "%zu: split cont %u new %u\n", i, op->u.split.cont, op->u.split.new);
			break;
		case CAPVM_OP_SAVE:
			fprintf(f, "%zu: save %u (cap %u, %s)\n",
			    i, op->u.save,
			    op->u.save / 2, (op->u.save & (uint32_t)0x01) ? "end" : "start");
			break;
		case CAPVM_OP_ANCHOR:
			fprintf(f, "%zu: anchor %s\n", i,
			    op->u.anchor == CAPVM_ANCHOR_START ? "start" : "end");
			break;
		default:
			assert(!"matchfail");
		}
	}
	for (size_t i = 0; i < p->char_classes.count; i++) {
		const uint64_t *octets = p->char_classes.sets[i].octets;
		fprintf(f, "char_class %zu: 0x%016lx 0x%016lx 0x%016lx 0x%016lx\n",
		    i, octets[0], octets[1], octets[2], octets[3]);
	}
}

unsigned
fsm_capvm_program_get_capture_count(const struct capvm_program *program)
{
	assert(program != NULL);
	return program->capture_count;
}

unsigned
fsm_capvm_program_get_max_capture_id(const struct capvm_program *program)
{
	assert(program != NULL);
	return (program->capture_count == 0
	    ? 0
	    : program->capture_base + program->capture_count - 1);
}

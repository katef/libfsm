/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include "print.h"
#include "internal.h"

#include "vm/retlist.h"
#include "vm/vm.h"
#include "print/ir.h"

int
print_hook_args(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	int (*default_args)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (default_args == NULL) {
		assert(lang_opaque == NULL);
	}

	if (hooks->accept != NULL) {
		return hooks->args(f, opt,
			lang_opaque, hooks->hook_opaque);
	} else if (default_args != NULL) {
		return default_args(f, opt,
			lang_opaque, hooks->hook_opaque);
	}

	return 0;
}

int
print_hook_accept(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm_state_metadata *state_metadata,
	int (*default_accept)(FILE *f, const struct fsm_options *opt,
		const struct fsm_state_metadata *state_metadata,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (opt->ambig == AMBIG_ERROR) {
		assert(state_metadata->end_id_count <= 1);
	}

	if (default_accept == NULL) {
		assert(lang_opaque == NULL);
	}

	if (hooks->accept != NULL) {
		return hooks->accept(f, opt, state_metadata,
			lang_opaque, hooks->hook_opaque);
	} else if (default_accept != NULL) {
		return default_accept(f, opt, state_metadata,
			lang_opaque, hooks->hook_opaque);
	}

	return 0;
}

int
print_hook_comment(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm_state_metadata *state_metadata)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (opt->ambig == AMBIG_ERROR) {
		assert(state_metadata->end_id_count <= 1);
	}

	if (opt->comments && hooks->comment != NULL) {
		/* this space is a polyglot */
		fprintf(f, " ");

		return hooks->comment(f, opt, state_metadata,
			hooks->hook_opaque);
	}

	return 0;
}

int
print_hook_reject(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	int (*default_reject)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	if (default_reject == NULL) {
		assert(lang_opaque == NULL);
	}

	if (hooks->reject != NULL) {
		return hooks->reject(f, opt,
			lang_opaque, hooks->hook_opaque);
	} else if (default_reject != NULL) {
		return default_reject(f, opt,
			lang_opaque, hooks->hook_opaque);
	}

	return 0;
}

int
print_hook_conflict(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm_state_metadata *state_metadata,
	const char *example)
{
	assert(opt != NULL);
	assert(hooks != NULL);

	/* f may be NULL for FSM_PRINT_NONE */

	if (hooks->conflict != NULL) {
		return hooks->conflict(f, opt, state_metadata,
			example,
			hooks->hook_opaque);
	}

	return 0;
}

static int
print_conflicts(FILE *f, const struct fsm *fsm,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks)
{
	fsm_state_t s;
	int conflicts;

	assert(fsm != NULL);
	assert(opt != NULL);
	assert(hooks != NULL);

	conflicts = 0;

	for (s = 0; s < fsm->statecount; s++) {
		char *example;
		fsm_end_id_t *ids;
		size_t count;
		int res;

		if (!fsm_isend(fsm, s)) {
			continue;
		}

		count = fsm_endid_count(fsm, s);
		if (count <= 1) {
			continue;
		}

		/*
		 * It's unfortunate that we call fsm_example() and allocate ids[] here
		 * as well as during make_ir(), but I can't find a way to do them in
		 * just one place. That's because we handle conflicts for output formats
		 * where there isn't an ir, and because the ir only exists for DFA,
		 * we can't move all output formats to use the ir.
		 */

		if (!make_example(fsm, s, &example)) {
			goto error;
		}

		ids = f_malloc(fsm->alloc, count * sizeof *ids);
		if (ids == NULL) {
			goto error;
		}

		res = fsm_endid_get(fsm, s, count, ids);
		assert(res == 1);

		// TODO: de-duplicate by ids[], so we don't call the conflict hook an unneccessary number of times
		// TODO: now i think this is the same as calling once per retlist entry

		/*
		 * The conflict hook is called here (rather in the caller),
		 * because it can be called multiple times, for different states.
		 */
		struct fsm_state_metadata state_metadata = {
			.end_ids = ids,
			.end_id_count = count,
		};
		if (-1 == print_hook_conflict(f, opt, hooks,
			&state_metadata,
			example))
		{
			goto error;
		}

		if (example != NULL) {
			f_free(fsm->alloc, example);
		}

		f_free(fsm->alloc, ids);

		conflicts++;
	}

	return conflicts;

error:

	return -1;
}

int
fsm_print(FILE *f, const struct fsm *fsm,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	enum fsm_print_lang lang)
{
	static const struct fsm_options opt_zero;
	static const struct fsm_hooks hooks_zero;

	struct dfavm_assembler_ir a;
	struct ir *ir;
	int r;

	static const struct dfavm_assembler_ir zero;
	static const struct fsm_vm_compile_opts vm_opts = {
		FSM_VM_COMPILE_DEFAULT_FLAGS,
		FSM_VM_COMPILE_VM_V1,
		NULL
	};

	fsm_print_f *print_fsm = NULL;
	ir_print_f *print_ir   = NULL;
	vm_print_f *print_vm   = NULL;

	if (lang != FSM_PRINT_NONE) {
		assert(f != NULL);
	}

	if (opt == NULL) {
		opt = &opt_zero;
	}

	if (hooks == NULL) {
		hooks = &hooks_zero;
	}

	ir = NULL;

	/*
	 * fsm_print() can print an NFA or a DFA. The ir is DFA-only.
	 * So we can't use the ir for ambiguity checks, because it
	 * can't be constructed when we have an NFA.
	 *
	 * That also means we can't move all output routines to ir-only,
	 * we need to keep struct fsm around. So we're walking the fsm
	 * struct to check for ambiguities.
	 *
	 * This could be done conditionally, only when print_fsm != NULL
	 * and then we'd also check for ambiguities for ir and vm output
	 * at the point where the accept hook is called. But there's no
	 * reason to do that, it's simpler to detect all the conflicts
	 * ahead of time regardless.
	 */
	if (opt->ambig == AMBIG_ERROR) {
		int count;

		count = print_conflicts(f, fsm, opt, hooks);
		if (count == -1) {
			goto error;
		}

		if (count > 0) {
			goto conflict;
		}
	}

	switch (lang) {
	case FSM_PRINT_NONE:
		return 0;

	case FSM_PRINT_AMD64_ATT:  print_vm  = fsm_print_amd64_att;  break;
	case FSM_PRINT_AMD64_GO:   print_vm  = fsm_print_amd64_go;   break;
	case FSM_PRINT_AMD64_NASM: print_vm  = fsm_print_amd64_nasm; break;

	case FSM_PRINT_API:        print_fsm = fsm_print_api;        break;
	case FSM_PRINT_AWK:        print_vm  = fsm_print_awk;        break;
	case FSM_PRINT_C:          print_ir  = fsm_print_c;          break;
	case FSM_PRINT_DOT:        print_fsm = fsm_print_dot;        break;
	case FSM_PRINT_FSM:        print_fsm = fsm_print_fsm;        break;
	case FSM_PRINT_GO:         print_vm  = fsm_print_go;         break;
	case FSM_PRINT_IR:         print_ir  = fsm_print_ir;         break;
	case FSM_PRINT_IRJSON:     print_ir  = fsm_print_irjson;     break;
	case FSM_PRINT_JSON:       print_fsm = fsm_print_json;       break;
	case FSM_PRINT_LLVM:       print_vm  = fsm_print_llvm;       break;
	case FSM_PRINT_RUST:       print_vm  = fsm_print_rust;       break;
	case FSM_PRINT_SH:         print_vm  = fsm_print_sh;         break;
	case FSM_PRINT_VMC:        print_vm  = fsm_print_vmc;        break;
	case FSM_PRINT_VMDOT:      print_vm  = fsm_print_vmdot;      break;

	case FSM_PRINT_VMOPS_C:    print_vm  = fsm_print_vmops_c;    break;
	case FSM_PRINT_VMOPS_H:    print_vm  = fsm_print_vmops_h;    break;
	case FSM_PRINT_VMOPS_MAIN: print_vm  = fsm_print_vmops_main; break;

	default:
		errno = ENOTSUP;
		return -1;
	}

	r = 0;

	if (print_fsm != NULL) {
		r = print_fsm(f, opt, hooks, fsm);
		goto done;
	}

	/* DFA only after this point */

	ir = make_ir(fsm, opt);
	if (ir == NULL) {
		return -1;
	}

	if (opt->ambig == AMBIG_ERROR) {
		size_t i;

		for (i = 0; i < ir->n; i++) {
			if (!ir->states[i].isend) {
				continue;
			}

			assert(ir->states[i].endids.count <= 1);
		}
	}

	if (print_ir != NULL) {
		r = print_ir(f, opt, hooks, ir);
		goto done;
	}

	/*
	 * We're building the retlist here based on the ir.
	 * I think we could build the retlist earlier instead,
	 * and then point at the struct ret entries from the ir,
	 * and then dfavm_compile_ir() would pick those up from there.
	 * But for now this is good.
	 */
	struct ret_list retlist;

	if (!build_retlist(&retlist, ir)) {
		free_ir(fsm, ir);
		goto error;
	}

	a = zero;

	/* TODO: non-const a */
	if (!dfavm_compile_ir(&a, ir, &retlist, vm_opts)) {
		free_retlist(&retlist);
		free_ir(fsm, ir);
		return -1;
	}

	if (print_vm != NULL) {
		r = print_vm(f, opt, hooks, &retlist, a.linked);
	}

	dfavm_opasm_finalize_op(&a);

	free_retlist(&retlist);

done:

	if (ir != NULL) {
		free_ir(fsm, ir);
	}

	if (ferror(f)) {
		return -1;
	}

	if (r < 0) {
		goto error;
	}

	return 0;

conflict:

	/*
	 * Finding an ambiguity conflict is not an error;
	 * we successfully identified AMBIG_ERROR.
	 *
	 * Success is >= 0 for a print function, we're
	 * distinguishing this from done: just for the caller.
	 */

	return 1;

error:

	return -1;
}

void
(fsm_dump)(FILE *f, const struct fsm *fsm,
	const char *file, unsigned line, const char *name)
{
	assert(f != NULL);
	assert(file != NULL);
	assert(name != NULL);

	fprintf(f, "# %s:%d %s\n", file, line, name);

	if (fsm == NULL) {
		fprintf(f, "# (null)\n");
		return;
	}

	fsm_print(f, fsm, & (const struct fsm_options) {
		.consolidate_edges = 1,
		.comments = 1,
		.group_edges = 1
	}, NULL, FSM_PRINT_FSM);

}


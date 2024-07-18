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
#include <fsm/print.h>
#include <fsm/vm.h>

#include "print.h"
#include "internal.h"

#include "vm/vm.h"
#include "print/ir.h"

int
print_hook_args(FILE *f, const struct fsm_options *opt,
	int (*default_args)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (default_args == NULL) {
		assert(lang_opaque == NULL);
	}

	if (opt->hooks.accept != NULL) {
		return opt->hooks.args(f, opt, lang_opaque);
	} else if (default_args != NULL) {
		return default_args(f, opt, lang_opaque);
	}

	return 0;
}

int
print_hook_accept(FILE *f, const struct fsm_options *opt,
	const fsm_end_id_t *ids, size_t count,
	int (*default_accept)(FILE *f, const struct fsm_options *opt,
		const fsm_end_id_t *ids, size_t count,
		void *lang_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (default_accept == NULL) {
		assert(lang_opaque == NULL);
	}

	if (opt->hooks.accept != NULL) {
		return opt->hooks.accept(f, opt, ids, count, lang_opaque);
	} else if (default_accept != NULL) {
		return default_accept(f, opt, ids, count, lang_opaque);
	}

	return 0;
}

int
print_hook_reject(FILE *f, const struct fsm_options *opt,
	int (*default_reject)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque),
	void *lang_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (default_reject == NULL) {
		assert(lang_opaque == NULL);
	}

	if (opt->hooks.reject != NULL) {
		return opt->hooks.reject(f, opt, lang_opaque);
	} else if (default_reject != NULL) {
		return default_reject(f, opt, lang_opaque);
	}

	return 0;
}

int
fsm_print(FILE *f, const struct fsm *fsm, enum fsm_print_lang lang)
{
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

	assert(f != NULL);
	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	if (lang == FSM_PRINT_NONE) {
		return 0;
	}

	switch (lang) {
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

	ir = NULL;

	if (print_fsm != NULL) {
		r = print_fsm(f, fsm->opt, fsm);
		goto done;
	}

	ir = make_ir(fsm);
	if (ir == NULL) {
		return -1;
	}

	if (print_ir != NULL) {
		r = print_ir(f, fsm->opt, ir);
		goto done;
	}

	a = zero;

	/* TODO: non-const a */
	if (!dfavm_compile_ir(&a, ir, vm_opts)) {
		free_ir(fsm, ir);
		return -1;
	}

	if (print_vm != NULL) {
		r = print_vm(f, fsm->opt, a.linked);
	}

	dfavm_opasm_finalize_op(&a);

done:

	if (ir != NULL) {
		free_ir(fsm, ir);
	}

	if (ferror(f)) {
		return -1;
	}

	return r;
}


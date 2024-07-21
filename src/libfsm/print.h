/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_PRINT_H
#define FSM_INTERNAL_PRINT_H

struct fsm;
struct fsm_options;
struct fsm_hooks;
struct ir;
struct dfavm_op_ir;

int
print_hook_args(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	int (*default_args)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque);

int
print_hook_accept(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const fsm_end_id_t *ids, size_t count,
	int (*default_accept)(FILE *f, const struct fsm_options *opt,
		const fsm_end_id_t *ids, size_t count,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque);

int
print_hook_reject(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	int (*default_reject)(FILE *f, const struct fsm_options *opt,
		void *lang_opaque, void *hook_opaque),
	void *lang_opaque);

typedef int fsm_print_f(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct fsm *fsm);

typedef int ir_print_f(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	const struct ir *ir);

typedef int vm_print_f(FILE *f,
	const struct fsm_options *opt,
	const struct fsm_hooks *hooks,
	struct dfavm_op_ir *ops);

vm_print_f fsm_print_amd64_att;
vm_print_f fsm_print_amd64_go;
vm_print_f fsm_print_amd64_nasm;

fsm_print_f fsm_print_api;
vm_print_f fsm_print_awk;
ir_print_f fsm_print_c;
fsm_print_f fsm_print_dot;
fsm_print_f fsm_print_fsm;
vm_print_f fsm_print_go;
ir_print_f fsm_print_ir;
ir_print_f fsm_print_irjson;
fsm_print_f fsm_print_json;
vm_print_f fsm_print_llvm;
vm_print_f fsm_print_rust;
vm_print_f fsm_print_sh;
vm_print_f fsm_print_vmc;

vm_print_f fsm_print_vmdot;
vm_print_f fsm_print_vmops_c;
vm_print_f fsm_print_vmops_h;
vm_print_f fsm_print_vmops_main;

#endif

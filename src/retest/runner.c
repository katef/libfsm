#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <unistd.h>
#include <dlfcn.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include "runner.h"

static enum error_type
runner_init_compiled(struct fsm *fsm, struct fsm_runner *r, enum implementation impl)
{
	static fsm_print *asm_print = fsm_print_vmasm_amd64_att;

	char tmp_c[]  = "/tmp/fsmcompile_c-XXXXXX";
	char tmp_o[]  = "/tmp/fsmcompile_o-XXXXXX";
	char tmp_so[] = "/tmp/fsmcompile_so-XXXXXX";

	char cmd[sizeof tmp_c + sizeof tmp_o + sizeof tmp_so + 256];
	const char *cc, *cflags, *as, *asflags;
	int fd_c, fd_so, fd_o;
	FILE *f = NULL;
	void *h = NULL;

	fd_c  = mkstemp(tmp_c);
	fd_so = mkstemp(tmp_so);
	fd_o  = -1;

	f = fdopen(fd_c, "w");
	if (f == NULL) {
		perror(tmp_c);
		return ERROR_FILE_IO;
	}

	switch (impl) {
	case IMPL_C:     fsm_print_c(f, fsm);   break;
	case IMPL_VMC:   fsm_print_vmc(f, fsm); break;
	case IMPL_VMASM: asm_print(f,fsm);      break;

	case IMPL_INTERPRET:
			 assert(!"should not reach!");
			 break;
	}

	if (EOF == fflush(f)) {
		perror(tmp_c);
		return ERROR_FILE_IO;
	}

	cc     = getenv("CC");
	cflags = getenv("CFLAGS");

	switch (impl) {
	case IMPL_C:
	case IMPL_VMC:
		(void) snprintf(cmd, sizeof cmd, "%s %s -xc -shared -fPIC %s -o %s",
				cc ? cc : "gcc", cflags ? cflags : "-std=c89 -pedantic -Wall -O3",
				tmp_c, tmp_so);

		if (0 != system(cmd)) {
			perror(cmd);
			return ERROR_FILE_IO;
		}

		break;

	case IMPL_VMASM:
		as      = getenv("AS");
		asflags = getenv("ASFLAGS");

		fd_o = mkstemp(tmp_o);

		(void) snprintf(cmd, sizeof cmd, "%s %s -o %s %s",
				as ? as : "as", asflags ? asflags : "", tmp_o, tmp_c);

		if (0 != system(cmd)) {
			perror(cmd);
			return ERROR_FILE_IO;
		}

		(void) snprintf(cmd, sizeof cmd, "%s %s -shared %s -o %s",
				cc ? cc : "gcc", cflags ? cflags : "",
				tmp_o, tmp_so);

		break;

	case IMPL_INTERPRET:
		assert(!"should not reach!");
		break;
	}

	if (0 != system(cmd)) {
		perror(cmd);
		return ERROR_FILE_IO;
	}

	if (EOF == fclose(f)) {
		perror(tmp_c);
		return ERROR_FILE_IO;
	}

	if (-1 == unlinkat(-1, tmp_c, 0)) {
		perror(tmp_c);
		return ERROR_FILE_IO;
	}

	if (fd_o != -1) {
		if (-1 == close(fd_o)) {
			perror(tmp_o);
			return ERROR_FILE_IO;
		}

		if (-1 == unlinkat(-1, tmp_o, 0)) {
			perror(tmp_so);
			return ERROR_FILE_IO;
		}
	}

	h = dlopen(tmp_so, RTLD_NOW);
	if (h == NULL) {
		fprintf(stderr, "%s: %s", tmp_so, dlerror());
		return ERROR_FILE_IO;
	}

	if (-1 == close(fd_so)) {
		perror(tmp_so);
		return ERROR_FILE_IO;
	}

	if (-1 == unlinkat(-1, tmp_so, 0)) {
		perror(tmp_so);
		return ERROR_FILE_IO;
	}

	r->impl = impl;

	switch (impl) {
	case IMPL_C:
	case IMPL_VMC:
		r->u.impl_c.h = h;
		r->u.impl_c.func = (int (*)(const char *, const char *)) (uintptr_t) dlsym(h, "fsm_main");
		break;

	case IMPL_VMASM:
		r->u.impl_asm.h = h;
		r->u.impl_asm.func = (int (*)(const unsigned char *, size_t)) (uintptr_t) dlsym(h, "fsm_match");
		break;

	case IMPL_INTERPRET:
		break;
	}

	return ERROR_NONE;
}

enum error_type
fsm_runner_initialize(struct fsm *fsm, struct fsm_runner *r, enum implementation impl, struct fsm_vm_compile_opts vm_opts)
{
	static const struct fsm_runner zero;
	struct fsm_dfavm *vm;

	assert(fsm != NULL);
	assert(r   != NULL);

	*r = zero;

	switch (impl) {
	case IMPL_C:
	case IMPL_VMASM:
	case IMPL_VMC:
		return runner_init_compiled(fsm, r, impl);

	case IMPL_INTERPRET:
		vm = fsm_vm_compile_with_options(fsm, vm_opts);
		if (vm == NULL) {
			fsm_free(fsm);
			return ERROR_COMPILING_BYTECODE;
		}
		r->impl = impl;
		r->u.impl_vm.vm = vm;
		return ERROR_NONE;
	}

	return ERROR_INVALID_PARAMETER;
}

void
fsm_runner_finalize(struct fsm_runner *r)
{
	assert(r != NULL);

	switch (r->impl) {
	case IMPL_C:
	case IMPL_VMC:
		if (r->u.impl_c.h != NULL) {
			dlclose(r->u.impl_c.h);
		}
		break;

	case IMPL_VMASM:
		if (r->u.impl_asm.h != NULL) {
			dlclose(r->u.impl_c.h);
		}
		break;

	case IMPL_INTERPRET:
		if (r->u.impl_vm.vm != NULL) {
			fsm_vm_free(r->u.impl_vm.vm);
		}
		break;

	default:
		assert(!"should not reach");
	}
}

int
fsm_runner_run(const struct fsm_runner *r, const char *s, size_t n)
{
	assert(r != NULL);
	assert(s != NULL);

	switch (r->impl) {
	case IMPL_C:
	case IMPL_VMC:
		assert(r->u.impl_c.func != NULL);
		return r->u.impl_c.func(s, s+n) >= 0;

	case IMPL_VMASM:
		assert(r->u.impl_asm.func != NULL);
		return r->u.impl_asm.func((const unsigned char *)s, n) >= 0;

	case IMPL_INTERPRET:
		assert(r->u.impl_vm.vm != NULL);
		return fsm_vm_match_buffer(r->u.impl_vm.vm, s, n);
	}

	assert(!"should not reach");
	abort();
}


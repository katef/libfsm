#define _GNU_SOURCE /* for vasprintf */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <assert.h>

#include <unistd.h>
#include <dlfcn.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include "runner.h"

static int
systemf(const char *fmt, ...)
{
	va_list ap;
	char *cmd;
	int r;

	assert(fmt != NULL);

	va_start(ap, fmt);
	r = vasprintf(&cmd, fmt, ap);
	va_end(ap);

	if (r == -1) {
		perror("vasprintf");
		exit(EXIT_FAILURE);
	}

	r = system(cmd);
	if (r != 0) {
		perror(cmd);
	}

	free(cmd);

	return r;
}

static int
xmkstemps(char *s)
{
	int fd;

	fd = mkstemps(s, strlen(strrchr(s, '.')));
	if (fd == -1) {
		perror(s);
		exit(EXIT_FAILURE);
	}

	return fd;
}

static int
print(const struct fsm *fsm, enum implementation impl,
	char *tmp_src)
{
	int fd_src;
	FILE *f;

	fd_src = xmkstemps(tmp_src);

	f = fdopen(fd_src, "w");
	if (f == NULL) {
		perror(tmp_src);
		return 0;
	}

	/* the vmc codegen can emit memcmp() calls */
	if (impl == IMPL_VMC && fsm_getoptions(fsm)->io == FSM_IO_PAIR) {
		fprintf(f, "#include <string.h>\n\n");
	}

	{
		int e;

		switch (impl) {
		case IMPL_C:     e = fsm_print_c(f, fsm);               break;
		case IMPL_RUST:  e = fsm_print_rust(f, fsm);            break;
		case IMPL_VMC:   e = fsm_print_vmc(f, fsm);             break;
		case IMPL_GOASM: e = fsm_print_vmasm_amd64_go(f, fsm);  break;
		case IMPL_VMASM: e = fsm_print_vmasm_amd64_att(f, fsm); break;
		case IMPL_GO:    e = fsm_print_go(f, fsm);              break;

		case IMPL_VMOPS:
			e = fsm_print_vmops_h(f, fsm)
			  | fsm_print_vmops_c(f, fsm)
			  | fsm_print_vmops_main(f, fsm);
			break;

		case IMPL_INTERPRET:
			assert(!"unreached");
			break;
		}

		if (e == -1) {
			return 0;
		}
	}

	if (impl == IMPL_RUST) {
		fprintf(f, "\n");

		fprintf(f, "use std::os::raw::c_uchar;\n");
		fprintf(f, "use std::slice;\n");
		fprintf(f, "\n");

		fprintf(f, "#[no_mangle]\n");
		fprintf(f, "pub extern \"C\" fn retest_trampoline(ptr: *const c_uchar, len: usize) -> i64 {\n");
		fprintf(f, "    let a: &[u8] = unsafe { slice::from_raw_parts(ptr, len as usize) };\n");
		fprintf(f, "    fsm_main(a).unwrap_or(-1)\n");
		fprintf(f, "}\n");
	}

	if (EOF == fclose(f)) {
		perror(tmp_src);
		return 0;
	}

	return 1;
}

static int
compile(enum implementation impl,
	const char *tmp_src, const char *tmp_so)
{
	const char *cc, *cflags, *as, *asflags;
	cc     = getenv("CC");
	cflags = getenv("CFLAGS");

	switch (impl) {
	case IMPL_C:
	case IMPL_VMC:
	case IMPL_VMOPS:
		if (0 != systemf("%s %s -xc -shared -fPIC %s -o %s",
				cc ? cc : "gcc", cflags ? cflags : "-std=c89 -pedantic -Wall -Werror -O3",
				tmp_src, tmp_so))
		{
			return 0;
		}

		break;

	case IMPL_RUST:
		if (0 != systemf("%s %s --crate-type dylib %s -o %s",
				"rustc", "--edition 2018",
				tmp_src, tmp_so))
		{
			return 0;
		}

		break;

	case IMPL_GO:
	case IMPL_GOASM: {
		char tmp_o1[] = "/tmp/fsmcompile_o1-XXXXXX.o";
		char tmp_o2[] = "/tmp/fsmcompile_o2-XXXXXX.o";

		int fd_o1, fd_o2;

		fd_o1 = xmkstemps(tmp_o1);
		fd_o2 = xmkstemps(tmp_o2);

		/* Go compiler needs to know not to look for a go.mod file */
		setenv("GOMODULE111", "off", 1);

		asflags = "";
		if (impl == IMPL_GOASM) {
			asflags = "-I $(go env GOROOT)/src/runtime";
		}

		if (0 != systemf("%s tool %s %s -p main -o %s %s",
			"go", (impl == IMPL_GO) ? "compile" : "asm", asflags, tmp_o1, tmp_src))
		{
			return 0;
		}

		as      = getenv("AS");
		asflags = getenv("ASFLAGS");

		if (0 != systemf("%s tool objdump -gnu %s | awk -f ./build/bin/go2att.awk | %s %s -o %s",
				"go", tmp_o1, as ? as : "as", asflags ? asflags : "", tmp_o2))
		{
			return 0;
		}

		if (0 != systemf("%s %s -shared %s -o %s",
				cc ? cc : "gcc", cflags ? cflags : "",
				tmp_o2, tmp_so))
		{
			return 0;
		}

		if (-1 == close(fd_o1)) {
			perror(tmp_o1);
			return 0;
		}

		if (-1 == unlinkat(-1, tmp_o1, 0)) {
			perror(tmp_o1);
			return 0;
		}

		if (-1 == close(fd_o2)) {
			perror(tmp_o2);
			return 0;
		}

		if (-1 == unlinkat(-1, tmp_o2, 0)) {
			perror(tmp_o2);
			return 0;
		}

		break;
	}

	case IMPL_VMASM: {
		char tmp_o[] = "/tmp/fsmcompile_o-XXXXXX.o";
		int fd_o;

		as      = getenv("AS");
		asflags = getenv("ASFLAGS");

		fd_o = xmkstemps(tmp_o);

		if (0 != systemf("%s %s -o %s %s",
				as ? as : "as", asflags ? asflags : "", tmp_o, tmp_src))
		{
			return 0;
		}

		if (0 != systemf("%s %s -shared %s -o %s",
				cc ? cc : "gcc", cflags ? cflags : "",
				tmp_o, tmp_so))
		{
			return 0;
		}

		if (-1 == close(fd_o)) {
			perror(tmp_o);
			return 0;
		}

		if (-1 == unlinkat(-1, tmp_o, 0)) {
			perror(tmp_o);
			return 0;
		}

		break;
	}

	case IMPL_INTERPRET:
		assert(!"unreached");
		break;
	}

	return 1;
}

static enum error_type
runner_init_compiled(struct fsm *fsm, struct fsm_runner *r, enum implementation impl)
{
	void *h;

	r->impl = impl;

	/* The Go compiler needs an extension on tmp_src so it knows
	 * it's a file not a package. Since we're doing that, it's
	 * easier to do the same for everyone. */
	char tmp_src_go[] = "/tmp/fsmcompile_src-XXXXXX.go";
	char tmp_src_c[]  = "/tmp/fsmcompile_src-XXXXXX.c";
	char tmp_src_rs[] = "/tmp/fsmcompile_src-XXXXXX.rs";
	char tmp_src_s[]  = "/tmp/fsmcompile_src-XXXXXX.s";
	char *tmp_src;

	switch (impl) {
	case IMPL_VMOPS:
	case IMPL_C:
	case IMPL_VMC:   tmp_src = tmp_src_c;  break;
	case IMPL_RUST:  tmp_src = tmp_src_rs; break;
	case IMPL_GOASM:
	case IMPL_VMASM: tmp_src = tmp_src_s;  break;
	case IMPL_GO:    tmp_src = tmp_src_go; break;

	case IMPL_INTERPRET:
		assert(!"unreached");
		break;
	}

	if (!print(fsm, r->impl, tmp_src)) {
		return ERROR_FILE_IO;
	}

	{
		char tmp_so[]     = "/tmp/fsmcompile_so-XXXXXX.so";
		int fd_so;

		/* compile() writes to tmp_so by name, we don't write to the open fd */
		fd_so  = xmkstemps(tmp_so);

		if (!compile(r->impl, tmp_src, tmp_so)) {
			return ERROR_FILE_IO;
		}

		if (-1 == close(fd_so)) {
			perror(tmp_so);
			return ERROR_FILE_IO;
		}

		h = dlopen(tmp_so, RTLD_NOW);
		if (h == NULL) {
			fprintf(stderr, "%s: %s", tmp_so, dlerror());
			return ERROR_FILE_IO;
		}

		if (-1 == unlinkat(-1, tmp_so, 0)) {
			perror(tmp_so);
			return ERROR_FILE_IO;
		}
	}

	if (-1 == unlinkat(-1, tmp_src, 0)) {
		perror(tmp_src);
		return 0;
	}

	/* XXX: depends on IO API */
	switch (r->impl) {
	case IMPL_C:
	case IMPL_VMC:
		r->u.impl_c.h = h;
		r->u.impl_c.func = (int (*)(const char *, const char *)) (uintptr_t) dlsym(h, "fsm_main");
		break;

	case IMPL_VMOPS:
		r->u.impl_c.h = h;
		r->u.impl_c.func = (int (*)(const char *, const char *)) (uintptr_t) dlsym(h, "fsm_match");
		break;

	case IMPL_RUST:
		r->u.impl_rust.h = h;
		r->u.impl_rust.func = (int64_t (*)(const unsigned char *, size_t)) (uintptr_t) dlsym(h, "retest_trampoline");
		break;

	case IMPL_GO:
	case IMPL_GOASM:
		r->u.impl_go.h = h;
		r->u.impl_go.func = (int64_t (*)(const unsigned char *, int64_t)) (uintptr_t) dlsym(h, "retest_trampoline");
		if (r->u.impl_go.func == NULL) {
                        /* Sometime we need a leading underscore. */
			r->u.impl_go.func = (int64_t (*)(const unsigned char *, int64_t)) (uintptr_t) dlsym(h, "_retest_trampoline");
		}
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
	case IMPL_RUST:
	case IMPL_VMASM:
	case IMPL_VMC:
	case IMPL_VMOPS:
	case IMPL_GO:
	case IMPL_GOASM:
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
	case IMPL_VMOPS:
		if (r->u.impl_c.h != NULL) {
			dlclose(r->u.impl_c.h);
		}
		break;

	case IMPL_RUST:
		if (r->u.impl_rust.h != NULL) {
			dlclose(r->u.impl_rust.h);
		}
		break;

	case IMPL_GO:
	case IMPL_GOASM:
		if (r->u.impl_go.h != NULL) {
			dlclose(r->u.impl_go.h);
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
	case IMPL_VMOPS:
		assert(r->u.impl_c.func != NULL);
		return r->u.impl_c.func(s, s+n) >= 0;

	case IMPL_RUST:
		assert(r->u.impl_rust.func != NULL);
		return r->u.impl_rust.func((const unsigned char *)s, n) >= 0;

	case IMPL_GO:
	case IMPL_GOASM:
		assert(r->u.impl_go.func != NULL);
		return r->u.impl_go.func((const unsigned char *)s, n) >= 0;

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


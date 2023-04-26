#!/usr/bin/awk

BEGIN {
    go_trampoline=0;
    os_is_linux = system("uname -s |grep Linux >/dev/null") == 0;
}

/TEXT main./ {
    sub(/TEXT main\./, "")
    sub(/\(SB\).*/, "")

    # prevent gnu ld warning about executable stack[
    if (os_is_linux) {
        print "#if defined(__linux__) && defined(__ELF__)";
        print ".section .note.GNU-stack,\"\",%progbits";
        print "#endif";
    }

    printf ".globl _%s\n", $0;
    printf ".text\n";
    printf "_%s:\n", $0;
    next;
}

/^  *[a-zA-Z0-9_-]*(\.s|\.go)?:[0-9]*/ {
    # abc.go:16		0x8ed			4839cb			CMPQ CX, BX                          // cmp %rcx,%rbx
    if (/\.go:/) {
        go_trampoline=1;
    }
    label=$2
    sub(/.*\/\/ */, "")
    if (/^j/) {
        # add ".L" to jmp addresses
        sub(/0x/, ".L0x");
        # objdump adds jmpq sometimes; make it jmp
        sub(/jmpq/, "jmp");
    }
    printf ".L%s: %s\n", label, $0;
}

END {
    print "";
    print ".globl _retest_trampoline";
    print ".text";
    print "_retest_trampoline:";

    # prologue
    print "pushq %rbp";
    print "movq %rsp, %rbp";

    if (go_trampoline) {
        # rbx is callee save
        print "pushq %rbx";

        # calling convention has params in rax/rbx
        print "movq %rsi, %rbx";
        print "movq %rdi, %rax";

        # give fsm match some stack space
        print "subq $24, %rsp";
        print "call _fsm_Match";
        print "addq $24, %rsp";

        # restore rbx
        print "popq %rbx";
    } else {
        # save rdx; it's a scratch register and not required to save, but it's the only
        # other register we use so let's be safe.
        print "pushq %rdx";

        # space for return value
        print "pushq $0";
        # arguments on the stack for Go assembly
        # retest uses FSM_IO_PAIR which for Go is a []byte: ptr,len,cap
        print "pushq %rsi";
        print "pushq %rsi";
        print "pushq %rdi";
        print "call _fsm_Match";
        # pop off arguments; this also restores rdi/rsi
        print "popq %rdi";
        print "popq %rsi";
        print "popq %rax"; # discard
        # and pop off return value
        print "popq %rax";

        # restore rdx
        print "popq %rdx";
    }

    # epilogue
    print "movq %rbp, %rsp";
    print "popq %rbp";

    print "retq";
}

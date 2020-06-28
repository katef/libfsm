/*
 * Copyright 2008-2020 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>
#include <fsm/vm.h>

#include "libfsm/internal.h"

#include "libfsm/vm/vm.h"

#include "ir.h"

  static const char *
cmp_operator(int cmp)
{
  switch (cmp) {
    case VM_CMP_LT: return "<";
    case VM_CMP_LE: return "=<";
    case VM_CMP_EQ: return "==";
    case VM_CMP_GE: return ">=";
    case VM_CMP_GT: return ">";
    case VM_CMP_NE: return "/=";

    case VM_CMP_ALWAYS:
    default:
                    assert("unreached");
                    return NULL;
  }
}

  static void
print_label(FILE *f, uint8_t fetches, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
  fprintf(f, "fsm(%lu=_State, <<", (unsigned long) op->index);
  for (int i = 0; i <= fetches; i++) {
    fprintf(f, "_%c,", 65+i);
  }
  fprintf(f, "_Rest/binary>>) ->");

  if (op->ir_state->example != NULL) {
    fprintf(f, " %% e.g. \"");
    escputs(f, opt, c_escputc_str, op->ir_state->example);
    fprintf(f, "\"\n");
  }
  fprintf(f, "\t if");
}

  static void
print_cond(FILE *f, uint8_t fetches, const struct dfavm_op_ir *op, const struct fsm_options *opt)
{
  (void) opt;

  if (op->cmp == VM_CMP_ALWAYS) {
    fprintf(f, "true -> ");
    return;
  }

  fprintf(f, "_%c %s ", 65+fetches, cmp_operator(op->cmp));
  fprintf(f, "%hu", (unsigned char) op->cmp_arg);
  fprintf(f, " -> ");
}

  static void
print_end(FILE *f, const struct dfavm_op_ir *op, const struct fsm_options *opt,
    enum dfavm_op_end end_bits, const struct ir *ir)
{
  if (end_bits == VM_END_FAIL) {
    fprintf(f, "throw(fail)");
    return;
  }

  if (opt->endleaf != NULL) {
    opt->endleaf(f, op->ir_state->opaque, opt->endleaf_opaque);
  } else {
    fprintf(f, "fsm(%lu, <<>>) ->", (unsigned long) op->index);
    if (op->ir_state->example != NULL) {
      fprintf(f, " %% e.g. \"");
      escputs(f, opt, c_escputc_str, op->ir_state->example);
      fprintf(f, "\"\n");
    }
    fprintf(f, "\tthrow({matched,%lu});\n", (unsigned long) (op->ir_state - ir->states));
  }
}

  static void
print_branch(FILE *f, const struct dfavm_op_ir *op)
{
  fprintf(f, "fsm(%lu, _Rest)", (unsigned long) op->u.br.dest_arg->index);
}

  static void
print_fetch(FILE *f, const struct fsm_options *opt)
{
  (void) opt;

  fprintf(f, "true -> ok\n");
  fprintf(f, "\tend,\n");
}

  static int
fsm_print_shfrag(FILE *f, const struct ir *ir, const struct fsm_options *opt)
{
  static const struct dfavm_assembler_ir zero;
  struct dfavm_assembler_ir a;
  struct dfavm_op_ir *op, *op2;

  static const struct fsm_vm_compile_opts vm_opts = { FSM_VM_COMPILE_DEFAULT_FLAGS, FSM_VM_COMPILE_VM_V1, NULL };

  assert(f != NULL);
  assert(ir != NULL);
  assert(opt != NULL);

  a = zero;

  if (!dfavm_compile_ir(&a, ir, vm_opts)) {
    return -1;
  }

  /*
   * We only output labels for ops which are branched to. This gives
   * gaps in the sequence for ops which don't need a label.
   * So here we renumber just the ones we use.
   */
  {
    uint32_t l;

    l = 0;

    for (op = a.linked; op != NULL; op = op->next) {
      if (op->num_incoming > 0) {
        op->index = l++;
      }
    }
  }

  fprintf(f, "-module(fsm).\n");
  fprintf(f, "-export([main/1]).\n");
  fprintf(f, "main([String]) ->\n");
  fprintf(f, "\ttry fsm(start, list_to_binary(String))\n");
  fprintf(f, "\tcatch throw:fail ->\n");
  fprintf(f, "\t\tio:format(\"failed~n\"),\n");
  fprintf(f, "\t\thalt(1);\n");
  fprintf(f, "\tthrow:{matched, N} ->\n");
  fprintf(f, "\t\tio:format(\"matched ~b~n\", [N]),\n");
  fprintf(f, "\t\thalt(0)\n");
  fprintf(f, "\tend.\n\n");

  bool saw_always = false, first=true;
  uint8_t fetched = 0;
  uint8_t fetches = -1;
  uint8_t stanza = 0;
  for (op = a.linked; op != NULL; op = op->next) {
    if (first) {
      for (op2 = op; op2 != NULL; op2 = op2->next) {
        if (op2->instr == VM_OP_FETCH) {
          fetches++;
        }
        if (op2->num_incoming > 0) {
          break;
        }
      }
      if (fetches == 0) {
        fetches = 1;
      }
      //fprintf(f, "fetches %d\n", fetches);
      first=false;
    }
    if (op == a.linked) {
      fprintf(f, "fsm(start, <<");
      for (int i = 0; i < fetches; i++) {
        fprintf(f, "_%c,", 65+i);
      }
      fprintf(f, " _Rest/binary>>) ->\n");
      fprintf(f, "\tif\n");
    }

    //fprintf(f, "%% incoming %d\n", op->num_incoming);
    if (op->num_incoming > 0) {
      if (!saw_always && op->u.stop.end_bits != VM_END_FAIL && stanza > 0) {
        fprintf(f, "\t\ttrue -> fsm(_State, _Rest)\n");
      }
      if (op != a.linked) {
        saw_always = false;
        fetched = 0;
        fetches = 0;
        stanza++;
        first = true;
        fprintf(f, "\tend;\n");

        print_end(f, op, opt, op->u.stop.end_bits, ir);
      }

      print_label(f, fetches, op, opt);
    }

    fprintf(f, "\t\t");

    switch (op->instr) {
      case VM_OP_STOP:
        print_cond(f, fetched - 1, op, opt);
        if (op->cmp == VM_CMP_ALWAYS) {
          saw_always = true;
        }
        fprintf(f, "throw(fail)");
        if ( op->next != NULL && op->next->num_incoming == 0 && op->cmp != VM_CMP_ALWAYS) {
          fprintf(f, ";");
        }
        if ( op->next != NULL && op->next->num_incoming > 0 && op->u.stop.end_bits == VM_END_FAIL && op->cmp != VM_CMP_ALWAYS) {
          fprintf(f, ";");
        }

        break;

      case VM_OP_FETCH:
        //print_end(f, op, opt, op->u.fetch.end_bits, ir);
        fetched++;
        if (fetched > 1) {
          print_fetch(f, opt);
          fprintf(f, "\tif\n");
        }
        fprintf(f, "\n");
        continue;

      case VM_OP_BRANCH:
        print_cond(f, fetched - 1, op, opt);
        print_branch(f, op);
        if (op->cmp == VM_CMP_ALWAYS) {
          saw_always = true;
        } else if (stanza > 0 || op->next != NULL && op->next->num_incoming == 0) {
          fprintf(f, ";");
        }
        break;

      default:
        assert(!"unreached");
        break;
    }

    fprintf(f, "\n");
  }

  fprintf(f, "\tend;\nfsm(_, _) -> throw(fail).\n");

  dfavm_opasm_finalize_op(&a);

  return 0;
}

  void
fsm_print_erl(FILE *f, const struct fsm *fsm)
{
  struct ir *ir;

  assert(f != NULL);
  assert(fsm != NULL);
  assert(fsm->opt != NULL);

  ir = make_ir(fsm);
  if (ir == NULL) {
    return;
  }

  if (fsm->opt->io != FSM_IO_STR) {
    fprintf(stderr, "unsupported IO API\n");
    exit(EXIT_FAILURE);
  }

  (void) fsm_print_shfrag(f, ir, fsm->opt);

  free_ir(fsm, ir);
}



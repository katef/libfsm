/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/print.h>

#include "print.h"

int
fsm_print(FILE *f, const struct fsm *fsm, enum fsm_print_lang lang)
{
	int (*print)(FILE *f, const struct fsm *fsm);

	assert(fsm != NULL);

	if (lang == FSM_PRINT_NONE) {
		return 0;
	}

	switch (lang) {
	case FSM_PRINT_AMD64_ATT:  print = fsm_print_amd64_att;  break;
	case FSM_PRINT_AMD64_GO:   print = fsm_print_amd64_go;   break;
	case FSM_PRINT_AMD64_NASM: print = fsm_print_amd64_nasm; break;

	case FSM_PRINT_API:        print = fsm_print_api;        break;
	case FSM_PRINT_AWK:        print = fsm_print_awk;        break;
	case FSM_PRINT_C:          print = fsm_print_c;          break;
	case FSM_PRINT_DOT:        print = fsm_print_dot;        break;
	case FSM_PRINT_FSM:        print = fsm_print_fsm;        break;
	case FSM_PRINT_GO:         print = fsm_print_go;         break;
	case FSM_PRINT_IR:         print = fsm_print_ir;         break;
	case FSM_PRINT_IRJSON:     print = fsm_print_irjson;     break;
	case FSM_PRINT_JSON:       print = fsm_print_json;       break;
	case FSM_PRINT_LLVM:       print = fsm_print_llvm;       break;
	case FSM_PRINT_RUST:       print = fsm_print_rust;       break;
	case FSM_PRINT_SH:         print = fsm_print_sh;         break;
	case FSM_PRINT_VMC:        print = fsm_print_vmc;        break;
	case FSM_PRINT_VMDOT:      print = fsm_print_vmdot;      break;

	case FSM_PRINT_VMOPS_C:    print = fsm_print_vmops_c;    break;
	case FSM_PRINT_VMOPS_H:    print = fsm_print_vmops_h;    break;
	case FSM_PRINT_VMOPS_MAIN: print = fsm_print_vmops_main; break;

	default:
		errno = ENOTSUP;
		return -1;
	}

	return print(f, fsm);
}


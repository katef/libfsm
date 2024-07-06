/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_PRINT_H
#define FSM_INTERNAL_PRINT_H

struct fsm;

typedef int fsm_print_f(FILE *f, const struct fsm *fsm);

fsm_print_f fsm_print_amd64_att;
fsm_print_f fsm_print_amd64_go;
fsm_print_f fsm_print_amd64_nasm;

fsm_print_f fsm_print_api;
fsm_print_f fsm_print_awk;
fsm_print_f fsm_print_c;
fsm_print_f fsm_print_dot;
fsm_print_f fsm_print_fsm;
fsm_print_f fsm_print_go;
fsm_print_f fsm_print_ir;
fsm_print_f fsm_print_irjson;
fsm_print_f fsm_print_json;
fsm_print_f fsm_print_llvm;
fsm_print_f fsm_print_rust;
fsm_print_f fsm_print_sh;
fsm_print_f fsm_print_vmc;

fsm_print_f fsm_print_vmdot;
fsm_print_f fsm_print_vmops_c;
fsm_print_f fsm_print_vmops_h;
fsm_print_f fsm_print_vmops_main;

#endif

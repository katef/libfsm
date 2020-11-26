/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_PRINT_H
#define FSM_PRINT_H

#include <stdio.h>

struct fsm;

/*
 * Print an FSM to the given file stream. The output is written in the format
 * specified. The available formats are:
 *
 *  fsm_print_api    - C code which calls the fsm(3) API
 *  fsm_print_awk    - awk code (gawk dialect)
 *  fsm_print_c      - ISO C90 code
 *  fsm_print_dot    - Graphviz Dot format, intended for rendering graphically
 *  fsm_print_fsm    - fsm(5) .fsm format, suitable for parsing by fsm(1)
 *  fsm_print_ir     - Codegen IR as Dot
 *  fsm_print_irjson - Codegen IR as JSON
 *  fsm_print_json   - JavaScript Object Notation
 *  fsm_print_vmc    - ISO C90 code, VM style
 *  fsm_print_vmdot  - Graphviz Dot format, showing VM opcodes
 *  fsm_print_rust   - Rust code
 *  fsm_print_sh     - Shell script (bash dialect)
 *  fsm_print_go     - Go code
 *
 * The output options may be NULL, indicating to use defaults.
 *
 * TODO: what to return?
 * TODO: explain constraints
 */

typedef void (fsm_print)(FILE *f, const struct fsm *fsm);

fsm_print fsm_print_api;
fsm_print fsm_print_awk;
fsm_print fsm_print_c;
fsm_print fsm_print_dot;
fsm_print fsm_print_fsm;
fsm_print fsm_print_ir;
fsm_print fsm_print_irjson;
fsm_print fsm_print_json;
fsm_print fsm_print_vmc;
fsm_print fsm_print_vmdot;
fsm_print fsm_print_vmasm;
fsm_print fsm_print_vmasm_amd64_att;  /* output amd64 assembler in AT&T format */
fsm_print fsm_print_vmasm_amd64_nasm; /* output amd64 assembler in NASM format */
fsm_print fsm_print_vmasm_amd64_go;   /* output amd64 assembler in Go format */
fsm_print fsm_print_sh;
fsm_print fsm_print_go;
fsm_print fsm_print_rust;

#endif


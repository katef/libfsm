/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_CLASS_H
#define RE_CLASS_H

struct fsm_options;

struct fsm *class_alnum_fsm(const struct fsm_options *opt);
struct fsm *class_alpha_fsm(const struct fsm_options *opt);
struct fsm *class_ascii_fsm(const struct fsm_options *opt);
struct fsm *class_blank_fsm(const struct fsm_options *opt);
struct fsm *class_cntrl_fsm(const struct fsm_options *opt);
struct fsm *class_digit_fsm(const struct fsm_options *opt);
struct fsm *class_graph_fsm(const struct fsm_options *opt);
struct fsm *class_lower_fsm(const struct fsm_options *opt);
struct fsm *class_print_fsm(const struct fsm_options *opt);
struct fsm *class_punct_fsm(const struct fsm_options *opt);
struct fsm *class_space_fsm(const struct fsm_options *opt);
struct fsm *class_spchr_fsm(const struct fsm_options *opt);
struct fsm *class_upper_fsm(const struct fsm_options *opt);
struct fsm *class_word_fsm(const struct fsm_options *opt);
struct fsm *class_xdigit_fsm(const struct fsm_options *opt);

#endif


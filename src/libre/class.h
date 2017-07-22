/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_CLASS_H
#define RE_CLASS_H

struct fsm_options;

struct fsm *utf8_C_fsm(const struct fsm_options *opt);
struct fsm *utf8_L_fsm(const struct fsm_options *opt);
struct fsm *utf8_M_fsm(const struct fsm_options *opt);
struct fsm *utf8_N_fsm(const struct fsm_options *opt);
struct fsm *utf8_P_fsm(const struct fsm_options *opt);
struct fsm *utf8_S_fsm(const struct fsm_options *opt);
struct fsm *utf8_Z_fsm(const struct fsm_options *opt);

struct fsm *utf8_Cf_fsm(const struct fsm_options *opt);
struct fsm *utf8_Co_fsm(const struct fsm_options *opt);
struct fsm *utf8_Cs_fsm(const struct fsm_options *opt);
struct fsm *utf8_Ll_fsm(const struct fsm_options *opt);
struct fsm *utf8_Lm_fsm(const struct fsm_options *opt);
struct fsm *utf8_Lo_fsm(const struct fsm_options *opt);
struct fsm *utf8_Lt_fsm(const struct fsm_options *opt);
struct fsm *utf8_Lu_fsm(const struct fsm_options *opt);
struct fsm *utf8_Mc_fsm(const struct fsm_options *opt);
struct fsm *utf8_Me_fsm(const struct fsm_options *opt);
struct fsm *utf8_Mn_fsm(const struct fsm_options *opt);
struct fsm *utf8_Nd_fsm(const struct fsm_options *opt);
struct fsm *utf8_Nl_fsm(const struct fsm_options *opt);
struct fsm *utf8_No_fsm(const struct fsm_options *opt);
struct fsm *utf8_Pc_fsm(const struct fsm_options *opt);
struct fsm *utf8_Pd_fsm(const struct fsm_options *opt);
struct fsm *utf8_Pe_fsm(const struct fsm_options *opt);
struct fsm *utf8_Pf_fsm(const struct fsm_options *opt);
struct fsm *utf8_Pi_fsm(const struct fsm_options *opt);
struct fsm *utf8_Po_fsm(const struct fsm_options *opt);
struct fsm *utf8_Ps_fsm(const struct fsm_options *opt);
struct fsm *utf8_Sc_fsm(const struct fsm_options *opt);
struct fsm *utf8_Sk_fsm(const struct fsm_options *opt);
struct fsm *utf8_Sm_fsm(const struct fsm_options *opt);
struct fsm *utf8_So_fsm(const struct fsm_options *opt);
struct fsm *utf8_Zl_fsm(const struct fsm_options *opt);
struct fsm *utf8_Zp_fsm(const struct fsm_options *opt);
struct fsm *utf8_Zs_fsm(const struct fsm_options *opt);

struct fsm *class_alnum_fsm(const struct fsm_options *opt);
struct fsm *class_alpha_fsm(const struct fsm_options *opt);
struct fsm *class_any_fsm(const struct fsm_options *opt);
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


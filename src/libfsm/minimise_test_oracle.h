/*
 * Copyright 2023 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef MINIMISE_TEST_ORACLE_H
#define MINIMISE_TEST_ORACLE_H

/* Build a minimised copy of the DFA via a slow but
 * easily verified reference algorithm. This can be
 * used to check the result of fsm_minimise.
 *
 * This must only be used on trimmed DFAs without captures. */
struct fsm *
fsm_minimise_test_oracle(const struct fsm *fsm);

#endif

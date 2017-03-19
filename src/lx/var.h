/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LX_VAR_H
#define LX_VAR_H

struct var;
struct fsm;

struct var *
var_bind(struct var **head, const char *name, struct fsm *fsm);

struct fsm *
var_find(const struct var *head, const char *name);

#endif


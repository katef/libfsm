/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_SET_H
#define ADT_SET_H

struct set;

struct set_iter {
	size_t i;
	const struct set *set;
};

#endif


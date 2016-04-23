/* $Id$ */

#ifndef RE_FORM_H
#define RE_FORM_H

const struct form {
	enum re_form form;
	struct fsm *(*comp)(int (*f)(void *opaque), void *opaque,
		enum re_flags flags, struct re_err *err);
} re_form[] = {
	{ RE_LITERAL, comp_literal },
	{ RE_GLOB,    comp_glob    },
	{ RE_SIMPLE,  comp_simple  }
};

#endif


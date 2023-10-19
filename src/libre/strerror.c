/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>

const char *
re_strerror(enum re_errno e)
{
	switch (e) {
	case RE_ESUCCESS:     return "Success";
	case RE_EERRNO:       return strerror(errno);
	case RE_EBADDIALECT:  return "Bad dialect";
	case RE_EBADGROUP:    return "Bad group";
	case RE_EUNSUPCAPTUR: return "Cannot support captures in this case";
	case RE_EUNSUPPPCRE:  return "Unsupported PCRE edge case";

	case RE_ENEGRANGE:    return "Negative group range";
	case RE_ENEGCOUNT:    return "Negative count range";

	case RE_EHEXRANGE:    return "Hex escape out of range";
	case RE_EOCTRANGE:    return "Octal escape out of range";
	case RE_ECOUNTRANGE:  return "Count out of range";

	case RE_EUNSUPPORTED: return "Unsupported operator";
	case RE_EFLAG:        return "Unknown control flag";
	case RE_EBADCP:       return "Invalid codepoint";
	case RE_EBADCOMMENT:  return "Comments may not nest";

	case RE_EXSUB:        return "Syntax error: expected sub-expression";
	case RE_EXTERM:       return "Syntax error: expected group term";
	case RE_EXGROUP:      return "Syntax error: expected group";
	case RE_EXATOM:       return "Syntax error: expected atom";
	case RE_EXCOUNT:      return "Syntax error: expected count";
	case RE_EXALTS:       return "Syntax error: expected alternative list";
	case RE_EXRANGE:      return "Syntax error: expected range separator";
	case RE_EXCLOSEGROUP: return "Syntax error: group is not closed";
	case RE_EXCLOSEFLAGS: return "Syntax error: flags are not closed";
	case RE_EXGROUPBODY:  return "Syntax error: expected group body";
	case RE_EXEOF:        return "Syntax error: expected EOF";
	case RE_EXESC:        return "Syntax error: expected character escape";
	}

	assert(!"unreached");
	return NULL;
}


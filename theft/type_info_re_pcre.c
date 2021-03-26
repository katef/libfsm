/*
 * Copyright 2017 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "type_info_re.h"

#include <string.h>
#include <limits.h>

#include <adt/xalloc.h>

static const char literals[] =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"0123456789____";

//#define LOG_VERBOSE

struct error_counter {
	bool used;
	uint8_t counter;
};

static enum theft_alloc_res
gen_pcre_node(struct theft *t, struct pcre_node **dst);
static uint8_t *gen_expected_match(struct theft *t, const struct pcre_node *node,
	size_t *size, struct error_counter *bomb);
static bool build_exp_class_flag(struct theft *t, struct buf *buf,
	const struct pcre_node *node, struct error_counter *bomb);
static uint8_t *
flatten_re_tree(struct test_env *test_env, const struct pcre_node *re_tree,
	size_t *psize);
static bool bracket_consumes_any(const struct pcre_node *node);
static void free_pcre_tree(struct pcre_node *re);
static bool append_bracket_classes(struct buf *buf,
	const struct pcre_node *node);

#define PRINT_MATCH_STRINGS 0

enum theft_alloc_res
type_info_re_pcre_build_info(struct theft *t,
	struct test_re_info *info)
{
	struct test_env *env = theft_hook_get_env(t);

	enum theft_alloc_res ares = gen_pcre_node(t, &info->u.pcre.head);
	if (ares != THEFT_ALLOC_OK) {
		return ares;
	}
	if (info->u.pcre.head == NULL) {
		return THEFT_ALLOC_ERROR;
	}

	info->string = flatten_re_tree(env, info->u.pcre.head, &info->size);
	if (info->string == NULL) {
		return THEFT_ALLOC_ERROR;
	}

	info->pos_count = 1 + theft_random_bits(t, 3);  // 1 -- 9 strings
	info->pos_pairs = xcalloc(info->pos_count, sizeof (struct string_pair));

	for (size_t i = 0; i < info->pos_count; i++) {
		uint8_t *positive;

		positive = gen_expected_match(t, info->u.pcre.head,
			&info->pos_pairs[i].size, NULL);

		if (info->pos_pairs[i].size == 0 || positive == NULL || positive[0] == '\0') {
			free(positive); /* discard 0-character string */
			info->pos_pairs[i].string = NULL;
		} else {
			if (PRINT_MATCH_STRINGS) {
				fprintf(stderr, "build_pcre: added positive match '%s'(%zu)\n",
				    positive, info->pos_pairs[i].size);
			}
			info->pos_pairs[i].string = positive;
			assert(info->pos_pairs[i].string);
		}
	}

	info->neg_count = 1 + theft_random_bits(t, 3);  // 1 -- 9 strings
	info->neg_pairs = xcalloc(info->neg_count, sizeof (struct string_pair));

	for (size_t i = 0; i < info->neg_count; i++) {
		uint8_t *negative;

		struct error_counter bomb = {
			.counter = theft_random_bits(t, 4),
		};

		negative = gen_expected_match(t, info->u.pcre.head,
			&info->neg_pairs[i].size, &bomb);

		if (negative == NULL || negative[0] == '\0') {
			free(negative);
			info->neg_pairs[i].string = NULL;
		} else if (bomb.used) {
			if (PRINT_MATCH_STRINGS) {
				fprintf(stderr, "build_pcre: added negative match '%s'(%zu)\n",
				    negative, info->neg_pairs[i].size);
			}
			info->neg_pairs[i].string = negative;
		} else {
			free(negative);
			info->neg_pairs[i].string = NULL;
		}
	}

	return THEFT_ALLOC_OK;
}

static bool
should_skip_bracket_set(const uint64_t *set256)
{
	/* empty character classes are invalid */
	for (size_t i = 0; i < 4; i++) {
		if (set256[i] != 0) {
			return false;
		}
	}
	return true;
}

static enum theft_alloc_res
gen_pcre_node(struct theft *t, struct pcre_node **dst)
{
	struct pcre_node *n;
	enum theft_alloc_res ares;

	/* 64-bit aligned buffer */
	uint64_t buf64[256 / sizeof (uint64_t)] = { 0 };
	uint8_t *buf;

	buf = (uint8_t *) buf64;

	n = xcalloc(1, sizeof *n);

	n->t = theft_random_choice(t, PCRE_NODE_TYPE_COUNT);

	switch (n->t) {
	case PN_DOT:
		n->u.dot.unused = 0xdead;
		break;

	case PN_LITERAL:
		/* Use a relatively small upper bound for literal length. */
		n->u.literal.size = theft_random_bits(t, 4) + 1;
		for (size_t i = 0; i < n->u.literal.size; i++) {
			uint8_t index = theft_random_bits(t, 6);
			assert(index < 64);
			/* Just use alphanumeric literals */
			buf[i] = literals[index];
		}

		n->u.literal.string = xmalloc(n->u.literal.size + 1);

		memcpy(n->u.literal.string, buf, n->u.literal.size);
		n->u.literal.string[n->u.literal.size] = '\0';
		break;

	case PN_QUESTION:
		ares = gen_pcre_node(t, &n->u.question.inner);
		if (ares != THEFT_ALLOC_OK) {
			return ares;
		}
		assert(n->u.question.inner != NULL);
		break;

	case PN_KLEENE:
		ares = gen_pcre_node(t, &n->u.kleene.inner);
		if (ares != THEFT_ALLOC_OK) {
			return ares;
		}
		assert(n->u.kleene.inner != NULL);
		break;

	case PN_PLUS:
		ares = gen_pcre_node(t, &n->u.plus.inner);
		if (ares != THEFT_ALLOC_OK) {
			return ares;
		}
		assert(n->u.plus.inner != NULL);
		break;

	case PN_BRACKET:
		n->u.bracket.negated = theft_random_bits(t, 1);

		assert((1LLU << BRACKET_CLASS_COUNT) - 1 == BRACKET_CLASS_MASK);
		n->u.bracket.class_flags = theft_random_bits(t, BRACKET_CLASS_COUNT);

		/* shrink away class_flags, and make them less common in general */
		if (theft_random_bits(t, 3) <= 6) {
			n->u.bracket.class_flags = 0;
		}

		memset(n->u.bracket.set, 0x00, sizeof(n->u.bracket.set));

		if (n->u.bracket.class_flags == 0) {
			/* just set printable characters for now */
			n->u.bracket.set[0] =
			    theft_random_bits(t, 64) & (((1ULL << 32) - 1) << 32);
			n->u.bracket.set[1] = theft_random_bits(t, 64);
			n->u.bracket.set[2] = theft_random_bits(t, 64);
			n->u.bracket.set[3] = theft_random_bits(t, 64);
		}

		if (should_skip_bracket_set(n->u.bracket.set)) {
			free(n);
			return THEFT_ALLOC_SKIP;
		}
		break;

	case PN_ALT:
		n->u.alt.count = 1 + theft_random_bits(t, 2);
		n->u.alt.alts = xcalloc(n->u.alt.count, sizeof (struct pcre_node *));
		for (size_t i = 0; i < n->u.alt.count; i++) {
			ares = gen_pcre_node(t, &n->u.alt.alts[i]);
			if (ares != THEFT_ALLOC_OK) {
				return ares;
			}
			assert(n->u.alt.alts[i]);
		}
		break;

	case PN_ANCHOR:
		n->u.anchor.type = theft_random_choice(t, PCRE_ANCHOR_TYPE_COUNT);
		if (theft_random_bits(t, 3) < 4) {
			n->u.anchor.type = PCRE_ANCHOR_NONE;
		}
		ares = gen_pcre_node(t, &n->u.anchor.inner);
		if (ares != THEFT_ALLOC_OK) {
			return ares;
		}
		assert(n->u.anchor.inner);
		break;

	default:
		assert(false);
		return THEFT_ALLOC_ERROR;
	}

	*dst = n;
	return THEFT_ALLOC_OK;
}

/*
 * Append another string buffer, with parens around it
 * if it's more than a single character.
 */
static bool
pop_and_append(struct flatten_env *env)
{
	struct buf *popped = buf_pop(env->b);
	assert(popped);

	if (popped->size > 0) {
		bool parens;

		parens = popped->size > 1;
		if (parens) {
			if (!buf_append(env->b, '(', false)) {
				return false;
			}
		}

		if (!buf_grow(env->b, env->b->size + popped->size)) {
			return false;
		}

		/* Note: NOT escaping with buf_memcpy */
		memcpy(&env->b->buf[env->b->size], popped->buf, popped->size);
		env->b->size += popped->size;
		if (parens) {
			if (!buf_append(env->b, ')', false)) {
				return false;
			}
		}
	}
	buf_free(popped);

	return true;
}

static bool
flatten(struct flatten_env *env, const struct pcre_node *node)
{
#ifdef LOG_VERBOSE
	fprintf(stdout, "< %s: [size %zd], node %d\n",
		__func__, env->b->size, node->t);
	hexdump(stdout, env->b->buf, env->b->size);
#endif

	assert(node);
	switch (node->t) {
	case PN_DOT:
		if (!buf_append(env->b, '.', false)) {
			return false;
		}

		break;

	case PN_LITERAL:
		if (!buf_grow(env->b, env->b->size + node->u.literal.size)) {
			return false;
		}

		buf_memcpy(env->b, (uint8_t *) node->u.literal.string,
			node->u.literal.size);

		break;

	case PN_QUESTION:
		if (!buf_push(env->b)) {
			return false;
		}

		if (!flatten(env, node->u.question.inner)) {
			return false;
		}

		if (!pop_and_append(env)) {
			return false;
		}

		if (!buf_append(env->b, '?', false)) {
			return false;
		}

		break;

	case PN_KLEENE:
		if (!buf_push(env->b)) {
			return false;
		}

		if (!flatten(env, node->u.kleene.inner)) {
			return false;
		}

		if (!pop_and_append(env)) {
			return false;
		}

		if (!buf_append(env->b, '*', false)) {
			return false;
		}

		break;

	case PN_PLUS:
		if (!buf_push(env->b)) {
			return false;
		}

		if (!flatten(env, node->u.plus.inner)) {
			return false;
		}

		if (!pop_and_append(env)) {
			return false;
		}

		if (!buf_append(env->b, '+', false)) {
			return false;
		}

		break;

	case PN_BRACKET:
		/* Don't emit a bracket that would end up empty */
		if (!bracket_consumes_any(node)) {
			break; /* skip */
		}

		if (!buf_append(env->b, '[', false)) {
			return false;
		}

		if (node->u.bracket.negated) {
			if (!buf_append(env->b, '^', false)) {
				return false;
			}
		}

		for (size_t i = 0; i < 256; i++) {
			if (node->u.bracket.set[i / 64] & (1LLU << (i % 64))) {
				unsigned char c = (unsigned char) i;
				if (c == '\\') { /* escape \ character */
					if (!buf_append(env->b, c, true)) {
						return false;
					}
					if (!buf_append(env->b, c, true)) {
						return false;
					}
				} else {
					if (!buf_append(env->b, c, true)) {
						return false;
					}
				}
			}
		}
		append_bracket_classes(env->b, node);

		if (!buf_append(env->b, ']', false)) {
			return false;
		}

		break;

	case PN_ALT:
		if (!buf_append(env->b, '(', false)) {
			return false;
		}

		assert(node->u.alt.count > 0);
		for (size_t i = 0; i < node->u.alt.count; i++) {
			if (!flatten(env, node->u.alt.alts[i])) {
				return false;
			}

			/* don't include | after last */
			if (i < node->u.alt.count - 1) {
				if (!buf_append(env->b, '|', false)) {
					return false;
				}
			}
		}

		if (!buf_append(env->b, ')', false)) {
			return false;
		}
		break;

	case PN_ANCHOR:
	{
		const enum pcre_anchor_type t = node->u.anchor.type;
		struct pcre_node *inner = node->u.anchor.inner;
		if (t == PCRE_ANCHOR_START) {
			if (!buf_append(env->b, '^', false)) { /* before */
				return false;
			}
			if (!flatten(env, inner)) {
				return false;
			}
		} else if (t == PCRE_ANCHOR_END) { /* anchor *after* */
			if (!flatten(env, inner)) {
				return false;
			}
			if (!buf_append(env->b, '$', false)) { /* after */
				return false;
			}
		} else {
			assert(t == PCRE_ANCHOR_NONE);
			if (!flatten(env, inner)) {
				return false;
			}
		}

		break;
	}

	default:
		assert(false);
		break;
	}

#ifdef LOG_VERBOSE
	fprintf(stdout, "> %s: [size %zd], node %d\n",
		__func__, env->b->size, node->t);
	hexdump(stdout, env->b->buf, env->b->size);
#endif

	return true;
}

static bool
bracket_consumes_any(const struct pcre_node *node)
{
	assert(node->t == PN_BRACKET);

	for (uint8_t i = 0; i < 4; i++) {
		uint64_t v;

		v = node->u.bracket.set[i];
		if (node->u.bracket.negated) {
			v = ~ v;
		}

		if (v > 0) {
			return true;
		}
	}

	return false;
}

static bool
append_bracket_classes(struct buf *buf,
	const struct pcre_node *node)
{
	assert(node->t == PN_BRACKET);

	for (enum pcre_bracket_class class = 0x0001;
		class <= BRACKET_CLASS_MASK; class <<= 1)
	{
		if (node->u.bracket.class_flags & class) {
			char *name;

			name = NULL;

			switch (class) {
			case BRACKET_CLASS_ALNUM:  name = "alnum";  break;
			case BRACKET_CLASS_ALPHA:  name = "alpha";  break;
			case BRACKET_CLASS_ASCII:  name = "ascii";  break;
			case BRACKET_CLASS_BLANK:  name = "blank";  break;
			case BRACKET_CLASS_CNTRL:  name = "cntrl";  break;
			case BRACKET_CLASS_DIGIT:  name = "digit";  break;
			case BRACKET_CLASS_GRAPH:  name = "graph";  break;
			case BRACKET_CLASS_LOWER:  name = "lower";  break;
			case BRACKET_CLASS_PRINT:  name = "print";  break;
			case BRACKET_CLASS_PUNCT:  name = "punct";  break;
			case BRACKET_CLASS_SPACE:  name = "space";  break;
			case BRACKET_CLASS_UPPER:  name = "upper";  break;
			case BRACKET_CLASS_WORD:   name = "word";   break;
			case BRACKET_CLASS_XDIGIT: name = "xdigit"; break;
			default:
				assert(false);
			}

			if (!buf_append(buf, '[', false)) { return false; }
			if (!buf_append(buf, ':', false)) { return false; }
			if (!buf_memcpy(buf, (uint8_t *) name, strlen(name))) {
				return false;
			}
			if (!buf_append(buf, ':', false)) { return false; }
			if (!buf_append(buf, ']', false)) { return false; }
		}
	}

	return true;
}

static uint8_t *
flatten_re_tree(struct test_env *test_env, const struct pcre_node *re_tree,
	size_t *psize)
{
	uint8_t *res;

	struct flatten_env env = {
		.env = test_env,
		.verbosity = test_env->verbosity,
		.b = buf_new(),
	};

	if (env.b == NULL) {
		return NULL;
	}

	if (!flatten(&env, re_tree)) {
		free(env.b);
		return NULL;
	}

	if (!buf_grow(env.b, env.b->size + 1)) {
		return false;
	}

	*psize = env.b->size;

	res = xmalloc(env.b->size + 1);

	memcpy(res, env.b->buf, env.b->size);
	res[env.b->size] = '\0';
	buf_free(env.b);

	return res;
}

static bool
in_set(const uint64_t *set, uint8_t pos)
{
	return 0 != (set[pos / 64] & (1LLU << (pos & 63)));
}

static void
counter_tick(struct error_counter *bomb)
{
	if (bomb != NULL && bomb->counter > 0) {
		bomb->counter--;
	}
}

static bool
counter_timed_out(struct error_counter *bomb)
{
	if (bomb == NULL) {
		return false;
	}

	return !bomb->used && bomb->counter == 0;
}

static void
counter_used(struct error_counter *bomb)
{
	bomb->used = true;
}

static bool
allowed_set_char(char c)
{
	return isprint(c);
}

enum anchored_match {
	ANCHORED_MATCH_NONE,
	ANCHORED_MATCH_START = 0x01,
	ANCHORED_MATCH_END = 0x02,
};

#define HAS_ANCHORED_START(m) ((*m) & ANCHORED_MATCH_START)
#define HAS_ANCHORED_END(m)   ((*m) & ANCHORED_MATCH_END)

static bool
build_exp_match(struct theft *t,
	struct buf *buf, const struct pcre_node *node,
	struct error_counter *bomb, enum anchored_match *m)
{
#ifdef LOG_VERBOSE
	fprintf(stdout, "< %s [size %zd], node %d\n", __func__, buf->size, node->t);
	hexdump(stdout, buf->buf, buf->size);
#endif

	switch (node->t) {
	case PN_DOT:
		if (counter_timed_out(bomb)) {
			/* don't append any byte */
			counter_used(bomb);
		} else {
			counter_tick(bomb);
			for (;;) {		/* any printable byte; filter newline or '\0' */
				uint8_t byte = (uint8_t) theft_random_bits(t, 8);
				if (byte == '\n') {
					/* ignore */
				} else if (byte == '\0' || !isprint(byte)) {
					if (!buf_append(buf, ' ', false)) {
						return false;
					}
					break;
				} else {
					if (!buf_append(buf, byte, false)) {
						return false;
					}
					break;
				}
			}
		}
		break;

	case PN_LITERAL:
		/* match the whole literal */
		if (!buf_grow(buf, buf->size + node->u.literal.size)) {
			return false;
		}
		/* note: not escaping the literal here */
		memcpy(&buf->buf[buf->size],
			node->u.literal.string,
			node->u.literal.size);

		if (counter_timed_out(bomb)) {
			/* Clobber a random spot in the string with an upper
			 * or lowercase 'X'. */
			size_t err_pos = buf->size
				+ theft_random_choice(t, node->u.literal.size);
			if (buf->buf[err_pos] == 'X') {
				buf->buf[err_pos] = 'x';
			} else {
				buf->buf[err_pos] = 'X';
			}
			counter_used(bomb);
		}
		counter_tick(bomb);
		buf->size += node->u.literal.size;
		break;

	case PN_QUESTION:
		/* match subtree, or not */
		if (counter_timed_out(bomb)) {
			/* Note: this can cause false negatives if used
			 * next to other repetition operators. */
			counter_used(bomb);

			/* apply it twice, no zero times or once */
			if (!build_exp_match(t, buf,
				node->u.question.inner, bomb, m)) {
				return false;
			}
			if (!build_exp_match(t, buf,
				node->u.question.inner, bomb, m)) {
				return false;
			}
		} else {
			counter_tick(bomb);
			if (theft_random_bits(t, 1) == 0x01) {
				if (!build_exp_match(t, buf,
					node->u.question.inner, bomb, m)) {
					return false;
				}
			}
		}
		break;

	case PN_KLEENE:
		/* Is an error possible here? (don't tick the counter) */
		if (!HAS_ANCHORED_START(m) && !HAS_ANCHORED_END(m)) {
			while (theft_random_bits(t, 1)) {
				if (!build_exp_match(t, buf,
					node->u.kleene.inner, bomb, m)) {
					return false;
				}
			}
		}
		break;

	case PN_PLUS:
		if (counter_timed_out(bomb)) {
			/* include it zero times */
			counter_used(bomb);
		} else {
			counter_tick(bomb);
			do {
				if (!build_exp_match(t, buf,
					node->u.plus.inner, bomb, m)) {
					return false;
				}
			} while (!(HAS_ANCHORED_START(m) || HAS_ANCHORED_END(m))
			    && theft_random_bits(t, 1));
		}
		break;

	case PN_BRACKET: {
		uint8_t start;
		const uint64_t *set;
		bool any, neg;

		/* Don't emit a bracket that would end up empty */
		if (!bracket_consumes_any(node)) {
			break; /* skip */
		}

		/* If there are any class flags, then potentially use those */
		if (node->u.bracket.class_flags != 0x0000 && theft_random_bits(t, 1)) {
			if (!build_exp_class_flag(t, buf, node, bomb)) {
				return false;
			}
			break;
		}

		start = theft_random_bits(t, 8);

		/* Pick the first matching character in the set */
		set = node->u.bracket.set;
		any = false;
		neg = node->u.bracket.negated;

		if (counter_timed_out(bomb)) {
			counter_used(bomb);
			neg = !neg;
		} else {
			counter_tick(bomb);
		}

		for (uint16_t i = 0; i < 256; i++) {
			uint8_t pos;
			bool add = false;

			pos = (start + i) & 0xFF; /* wrap */

			if (!allowed_set_char(pos)) {
				continue;
			}

			if (neg && !in_set(set, pos)) {
				add = true;
			} else if (!neg && in_set(set, pos)) {
				add = true;
			}

			if (add) {
				if (!buf_append(buf, pos, false)) {
					return false;
				}

				any = true;
				break;
			}
		}
		if (!any) {
			fprintf(stdout, "neg? %d\n", neg);
			fprintf(stdout, "%016" PRIx64 "%016" PRIx64 "%016" PRIx64 "%016" PRIx64"\n",
				set[0], set[1], set[2], set[3]);
		}
		break;
	}

	case PN_ALT: {
		/* Don't tick counter, leave that for inner nodes */
		uint8_t choice;

		choice = theft_random_choice(t, node->u.alt.count);
		if (!build_exp_match(t, buf,
			node->u.alt.alts[choice], bomb, m)) {
			return false;
		}
		break;
	}

	case PN_ANCHOR:
	{
		/* Note an anchor, so that the caller doesn't add further stuff that
		 * would be pruned.
		 * FIXME: does this need to apply before or after recursing? */
		const enum pcre_anchor_type at = node->u.anchor.type;
		if (at == PCRE_ANCHOR_START) {
			(*m) |= ANCHORED_MATCH_START;
		} else if (at == PCRE_ANCHOR_END) {
			(*m) |= ANCHORED_MATCH_END;
		} else {
			assert(at == PCRE_ANCHOR_NONE);
		}

		if (!build_exp_match(t, buf,
			node->u.anchor.inner, bomb, m)) {
			return false;
		}
		break;
	}

	default:
	case PCRE_NODE_TYPE_COUNT:
		assert(false);
		break;
	}

#ifdef LOG_VERBOSE
	fprintf(stdout, "> %s [size %zd]\n", __func__, buf->size);
	hexdump(stdout, buf->buf, buf->size);
#endif

	return true;
}

static bool
build_exp_class_flag(struct theft *t, struct buf *buf,
	const struct pcre_node *node, struct error_counter *bomb)
{
	bool inject_error;
	uint8_t byte, offset;
	uint8_t class_count;

	assert(node->t == PN_BRACKET);
	assert(node->u.bracket.class_flags != 0x0000);

	inject_error = counter_timed_out(bomb);

	/* negate */
	if (node->u.bracket.negated) {
		inject_error = !inject_error;
	}

	byte = 0x00;

	class_count = BRACKET_CLASS_COUNT;
	assert(((1LLU << class_count) - 1) == BRACKET_CLASS_MASK);

	offset = theft_random_bits(t, 4);

	for (uint8_t i = 0; i < class_count; i++) {
		enum pcre_bracket_class class;

		class = 1LLU << ((offset + i) % class_count);
		if (node->u.bracket.class_flags & class) {
			switch (class) {
			default:
				assert(false);
				return false;

			case BRACKET_CLASS_ALNUM: {
				static const char alnum[] =
					"abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"0123456789";
				byte = inject_error
					? '_'
					: alnum[theft_random_choice(t, sizeof alnum)];
				break;
			}

			case BRACKET_CLASS_ALPHA: {
				static const char alpha[] =
					"abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				byte = inject_error
					? '9'
					: alpha[theft_random_choice(t, sizeof alpha)];
				break;
			}

			case BRACKET_CLASS_ASCII:
				byte = theft_random_bits(t, 7) | (inject_error ? 0x80 : 0x00);
				break;

			case BRACKET_CLASS_BLANK:
				if (inject_error) {
					byte = '_';
				} else {
					byte = (theft_random_bits(t, 1) ? ' ' : '\t');
				}
				break;

			case BRACKET_CLASS_CNTRL:
				break;

			case BRACKET_CLASS_DIGIT: {
				static const char digit[] =
					"0123456789";
				byte = inject_error
					? 'X'
					: digit[theft_random_choice(t, sizeof digit)];
				break;
			}

			case BRACKET_CLASS_GRAPH: {
				uint8_t offset = theft_random_bits(t, 8);
				for (uint16_t i = 0; i < 256; i++) {
					uint8_t v = (offset + i) & 0xFF;
					if (inject_error == isgraph(v)) {
						byte = v;
						break;
					}
				}
				break;
			}

			case BRACKET_CLASS_LOWER: {
				static const char lower[] =
					"abcdefghijklmnopqrstuvwxyz";
				byte = inject_error
					? 'X'
					: lower[theft_random_choice(t, sizeof lower)];
				break;
			}

			case BRACKET_CLASS_PRINT: {
				uint8_t offset = theft_random_bits(t, 7);
				for (uint8_t i = 0; i < 128; i++) {
					uint8_t v = (offset + i) & 0x7f;
					if (isprint(v)) {
						byte = v | (inject_error ? 0x80 : 0x00);
						break;
					}
				}
				break;
			}

			case BRACKET_CLASS_PUNCT: {
				uint8_t offset = theft_random_bits(t, 7);
				for (uint8_t i = 0; i < 128; i++) {
					uint8_t v = (offset + i) & 0x7f;
					if (ispunct(v)) {
						byte = v | (inject_error ? 0x80 : 0x00);
						break;
					}
				}
				break;
			}

			case BRACKET_CLASS_SPACE: {
				static const char space[] =
					"\t\n\v\f\r ";
				byte = inject_error
					? 'X'
					: space[theft_random_choice(t, sizeof space)];
				break;
			}

			case BRACKET_CLASS_UPPER: {
				static const char upper[] =
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				byte = inject_error
					? 'x'
					: upper[theft_random_choice(t, sizeof upper)];
				break;
			}

			case BRACKET_CLASS_WORD: {
				static const char word[] =
					"abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
					"0123456789";
				byte = inject_error
					? 0x80
					: word[theft_random_choice(t, sizeof word)];
				break;
			}

			case BRACKET_CLASS_XDIGIT: {
				static const char xdigit[] =
					"0123456789"
					"abcdef"
					"ABCDEF";
				byte = inject_error
					? 'H'
					: xdigit[theft_random_choice(t, sizeof xdigit)];
				break;
			}
			}

			if (!buf_append(buf, byte, false)) {
				return false;
			}
			return true;
		}
	}

	assert(false);

	return false;
}

static uint8_t *
gen_expected_match(struct theft *t,
	const struct pcre_node *node, size_t *size,
	struct error_counter *bomb)
{
	uint8_t *res;
	struct buf *buf;
	enum anchored_match m = ANCHORED_MATCH_NONE;

	buf = buf_new();
	if (buf == NULL) {
		return NULL;
	}

	if (!build_exp_match(t, buf, node, bomb, &m)) {
		buf_free(buf);
		return NULL;
	}

	res = xmalloc(buf->size + 1);

	memcpy(res, buf->buf, buf->size);
	*size = buf->size;
	res[*size] = '\0';

	buf_free(buf);

	return res;
}

void
type_info_re_pcre_free(struct pcre_node *node)
{
	free_pcre_tree(node);
}

static void
free_pcre_tree(struct pcre_node *node)
{
	switch (node->t) {
	case PN_LITERAL:
		free(node->u.literal.string);
		break;

	case PN_QUESTION:
		free_pcre_tree(node->u.question.inner);
		break;

	case PN_KLEENE:
		free_pcre_tree(node->u.kleene.inner);
		break;

	case PN_PLUS:
		free_pcre_tree(node->u.plus.inner);
		break;

	case PN_ALT:
		for (size_t i = 0; i < node->u.alt.count; i++) {
			free_pcre_tree(node->u.alt.alts[i]);
		}
		free(node->u.alt.alts);
		break;

	case PN_DOT:
	case PN_BRACKET:
		/* no op */
		break;

	case PN_ANCHOR:
		free_pcre_tree(node->u.anchor.inner);
		break;

	default:
	case PCRE_NODE_TYPE_COUNT:
		assert(false);
		break;
	}

	free(node);
}

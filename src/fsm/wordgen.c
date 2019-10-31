#include "wordgen.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

struct state_gather {
	unsigned nstates;
	unsigned ind;
	const struct fsm_state **states;
};

static int
state_gather_callback(const struct fsm *fsm, const struct fsm_state *st, void *ud)
{
	struct state_gather *sg = ud;

	(void)fsm;

	assert(sg->ind < sg->nstates);
	sg->states[sg->ind++] = st;

	return 1;
}

static void
state_gather_finalize(struct state_gather *sg)
{
	free(sg->states);
}

static int
state_cmp(const void *a, const void *b)
{
	const struct fsm_state *const *ppa = a;
	const struct fsm_state *const *ppb = b;

	const struct fsm_state *pa = *ppa;
	const struct fsm_state *pb = *ppb;

	return (pa > pb) - (pb > pa);
}

static int
gather_all_states(const struct fsm *fsm, struct state_gather *states)
{
	static const struct state_gather zero;

	*states = zero;

	states->nstates = fsm_countstates(fsm);
	states->ind = 0;
	states->states = calloc(states->nstates, sizeof states->states[0]);
	if (states->states == NULL) {
		return -1;
	}

	fsm_walk_states(fsm, states, state_gather_callback);

	assert(states->ind == states->nstates);

	qsort(states->states, states->nstates, sizeof states->states[0], state_cmp);

	return 0;
}

struct dfa_edge {
	unsigned from;
	unsigned to;
	unsigned char sym;
};

struct edge_gather {
	const struct state_gather *states;

	unsigned nedges;
	unsigned ind;
	struct dfa_edge *edges;
};

static int
edge_gather_callback_eps(const struct fsm *fsm, const struct fsm_state *from, const struct fsm_state *to, void *ud)
{
	(void)fsm;
	(void)from;
	(void)to;
	(void)ud;

	/* nop */
	return 1;
}

static long
find_state(const struct state_gather *sg, const struct fsm_state *st)
{
	unsigned lo, hi;

	/*
	for (lo=0; lo < sg->nstates; lo++) {
		fprintf(stderr, "[%3u] %p%s\n", lo, (void *)sg->states[lo], sg->states[lo] == st ? "  ***" : "");
	}
	*/

	lo = 0;
	hi = sg->nstates;

	for (;;) {
		unsigned mid;
		const struct fsm_state *mid_st;

		mid = lo + (hi-lo)/2;
		assert(mid < sg->nstates);

		mid_st = sg->states[mid];

		// fprintf(stderr, "%u -- %u -- %u | %p | %p\n", lo,mid,hi, (void *)mid_st, (void *)st);

		if (st == mid_st) {
			return mid;
		}

		if (st < mid_st) {
			hi = mid-1;
		}
		else {
			lo = mid+1;
		}

		if (lo > hi) {
			break;
		}
	}

	return -1;
}

static int
edge_gather_callback(const struct fsm *fsm, const struct fsm_state *from, const struct fsm_state *to, char c, void *ud)
{
	struct edge_gather *eg = ud;
	long ind_from, ind_to;

	assert(fsm  != NULL);
	assert(eg   != NULL);
	assert(from != NULL);
	assert(to   != NULL);

	(void)fsm;
	assert(eg->ind < eg->nedges);

	ind_from = find_state(eg->states, from);
	ind_to   = find_state(eg->states, to);

	assert(ind_from >= 0);
	assert(ind_from < eg->states->nstates);

	assert(ind_to >= 0);
	assert(ind_to < eg->states->nstates);

	eg->edges[eg->ind].from = ind_from;
	eg->edges[eg->ind].to   = ind_to;
	eg->edges[eg->ind].sym  = c;

	eg->ind++;

	return 1;
}

static int
edge_cmp(const void *a, const void *b)
{
	const struct dfa_edge *pa = a;
	const struct dfa_edge *pb = b;

	if (pa->from < pb->from) {
		return -1;
	}
	else if (pa->from > pb->from) {
		return 1;
	}
	else if (pa->sym < pb->sym) {
		return -1;
	}
	else if (pa->sym > pb->sym) {
		return 1;
	}
	else {
		return 0;
	}
}

static void
edge_gather_finalize(struct edge_gather *eg)
{
	free(eg->edges);
}

static int
gather_all_edges(const struct fsm *fsm, const struct state_gather *sg, struct edge_gather *edges)
{
	static const struct edge_gather zero;

	*edges = zero;

	edges->states = sg;

	edges->nedges = fsm_countedges(fsm);
	edges->ind = 0;
	edges->edges = calloc(edges->nedges, sizeof edges->edges[0]);
	if (edges->edges == NULL) {
		return -1;
	}

	fsm_walk_edges(fsm, edges,
		edge_gather_callback,
		edge_gather_callback_eps);

	assert(edges->ind == edges->nedges);

	qsort(edges->edges, edges->nedges, sizeof edges->edges[0], edge_cmp);

	return 0;
}

void
dfa_table_finalize(struct dfa_table *table)
{
	static const struct dfa_table zero;

	assert(table != NULL);

	free(table->offsets);
	free(table->syms);
	free(table->dests);
	free(table->isend);

	*table = zero;
}

int
dfa_table_initialize(struct dfa_table *table, const struct fsm *fsm)
{
	static const struct dfa_table zero;
	static const struct state_gather sg_zero;
	static const struct edge_gather  eg_zero;

	struct state_gather sg = sg_zero;
	struct edge_gather eg = eg_zero;
	const struct fsm_state *start_state;
	unsigned i, from0;

	*table = zero;

	if (!fsm_all(fsm, fsm_isdfa)) {
		return -1;
	}

	if (gather_all_states(fsm, &sg) < 0) {
		goto error;
	}

	if (gather_all_edges(fsm, &sg, &eg) < 0) {
		goto error;
	}

	table->offsets = calloc(sg.nstates+1, sizeof table->offsets[0]);
	if (table->offsets == NULL) {
		goto error;
	}

	table->syms = calloc(eg.nedges, sizeof table->syms[0]);
	if (table->syms == NULL) {
		goto error;
	}

	table->dests = calloc(eg.nedges, sizeof table->dests[0]);
	if (table->dests == NULL) {
		goto error;
	}

	table->isend = calloc(sg.nstates, 1);
	if (table->isend == NULL) {
		goto error;
	}

	table->nstates = sg.nstates;
	table->nedges  = eg.nedges;

	start_state = fsm_getstart(fsm);
	for (i=0; i < sg.nstates; i++) {
		if (sg.states[i] == start_state) {
			table->start = i;
		}

		if (fsm_isend(fsm, sg.states[i])) {
			table->isend[i] = 1;
		}
	}

	from0 = 0;
	for (i=0; i < eg.nedges; i++) {
		unsigned from1 = eg.edges[i].from;
		if (from1 != from0) {
			unsigned fr;
			for(fr = from0; fr < from1; fr++) {
				table->offsets[fr+1] = i;
			}

			from0 = from1;
		}

		table->syms[i]  = eg.edges[i].sym;
		table->dests[i] = eg.edges[i].to;
	}

	for (; from0 < table->nstates; from0++) {
		table->offsets[from0+1] = i;
	}

	state_gather_finalize(&sg);
	edge_gather_finalize(&eg);

	return 0;

error:
	state_gather_finalize(&sg);
	edge_gather_finalize(&eg);

	dfa_table_finalize(table);

	return -1;
}

static int
generate_word(const struct dfa_table *table, const struct dfa_wordgen_params *params, struct prng_state *prng, char *s)
{
	unsigned state;
	unsigned len;

	assert(table != NULL);
	assert(params != NULL);
	assert(s != NULL);

	len = 0;
	state = table->start;

	while (len < params->maxlen) {
		unsigned edge, nedges;
		int ch;

		nedges = table->offsets[state+1] - table->offsets[state];
		if (nedges == 0) {
			break;
		}

		if (nedges == 1) {
			edge = 0;
		} else {
			double pick = prng_double(prng);
			edge = floor(pick * nedges);
			/*
			fprintf(stderr, "> st = %u, nedges = %u, roll = %f, pick = %u\n",
				state, nedges, pick, edge);
			*/
		}

		assert(edge < nedges);

		edge += table->offsets[state];
		assert(edge < table->nedges);

		/*
		fprintf(stderr, "@ state %u edge %u sym '%c' dest %u\n",
			state, edge, isprint(table->syms[edge]) ? table->syms[edge] : ' ',
			table->dests[edge]);
		*/

		ch = table->syms[edge];
		s[len++] = isprint(ch) ? ch : ' ';
		state = table->dests[edge];

		assert(state < table->nstates);
		if (table->isend[state] && len >= params->minlen && prng_double(prng) <= params->prob_stop) {
			break;
		}
	}

	if (len < params->minlen || !table->isend[state]) {
		return -1;
	}

	s[len] = '\0';
	return len;
}

char **dfa_generate_words(const struct dfa_table *table, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words)
{
	char **words = NULL;
	char *block = NULL;
	unsigned offset, i, w;
	size_t block_size;

	assert(table != NULL);
	assert(params != NULL);

	assert(params->minlen > 0);
	assert(params->minlen < params->maxlen);

	block_size = (size_t)(params->maxlen+1) * num_words;

	block = malloc(block_size);
	if (block == NULL) {
		goto error;
	}

	words = calloc(num_words, sizeof words[0]);
	if (words == NULL) {
		goto error;
	}

	offset = 0;
	w = 0;

	for (i=0; i < num_words; i++) {
		int len;

		len = generate_word(table, params, prng, &block[offset]);

		if (len > 0) {
			words[w++] = &block[offset];
			offset += len+1;
		}
	}

	if (w==0) {
		goto error;
	}

	return words;

error:
	free(words);
	free(block);

	return NULL;
}

int dfa_generate_words_to_file(const struct dfa_table *table, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words, FILE *f)
{
	char *temp = NULL;
	unsigned i;

	assert(table != NULL);
	assert(params != NULL);

	assert(params->minlen > 0);
	assert(params->minlen < params->maxlen);

	temp = malloc(params->maxlen+1);
	if (temp == NULL) {
		goto error;
	}

	for (i=0; i < num_words; i++) {
		int len;

		memset(&temp[0], 0, params->maxlen+1);
		len = generate_word(table, params, prng, &temp[0]);

		if (len > 0) {
			temp[len] = '\0';
			fprintf(f, "%s\n", &temp[0]);
		}
	}

	free(temp);
	return 0;

error:
	return -1;
}

/* convenience wrappers */
int fsm_generate_words_to_file(const struct fsm *fsm, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words, FILE *f)
{
	struct dfa_table table;
	int ret;

	if (dfa_table_initialize(&table, fsm) < 0) {
		return -1;
	}

	ret = dfa_generate_words_to_file(&table, params, prng, num_words, f);
	dfa_table_finalize(&table);

	return ret;
}

void
free_dfa_generated_words(char **w) {
	if (w != NULL) {
		free(w[0]);
		free(w);
	}
}

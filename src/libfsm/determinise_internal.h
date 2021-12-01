#ifndef DETERMINISE_INTERNAL_H
#define DETERMINISE_INTERNAL_H

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/internedstateset.h>
#include <adt/ipriq.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"

#include <ctype.h>

#define DUMP_MAPPING 0
#define LOG_DETERMINISE_CLOSURES 0
#define LOG_DETERMINISE_CAPTURES 0
#define LOG_SYMBOL_CLOSURE 0
#define LOG_AC 0

#if LOG_DETERMINISE_CAPTURES
#include <fsm/print.h>
#endif

/* Starting number of buckets for the map's
 * hash table. Must be a power of 2. */
#define DEF_MAP_BUCKET_CEIL 4

/*
 * This maps a DFA state onto its associated NFA symbol closure, such that an
 * existing DFA state may be found given any particular set of NFA states
 * forming a symbol closure.
 *
 * Since the same state sets tend to be constructed over and over during
 * determinization, they are interned -- constructing a set with the same
 * contents will instead get a reference to an existing instance.
 */
struct map {
	const struct fsm_alloc *alloc;

	size_t used;
	size_t count;

	/* These are allocated on demand, because they're pushed onto the
	 * stack by address, and *edges is used by address. */
	struct mapping {
		interned_state_set_id iss;
		size_t dfastate;
		struct edge_set *edges;
	} **buckets;
};

struct map_iter {
	struct map *m;
	size_t i;
};

struct reverse_mapping {
	unsigned count;
	unsigned ceil;
	fsm_state_t *list;
};

struct det_copy_capture_actions_env {
	char tag;
	struct fsm *dst;
	struct reverse_mapping *reverse_mappings;
	int ok;
};

#define MAPPINGSTACK_DEF_CEIL 16
struct mappingstack {
	const struct fsm_alloc *alloc;
	size_t ceil;
	size_t depth;
	struct mapping **s;
};

#define AC_NO_STATE ((fsm_state_t)-1)
#define DEF_ITER_CEIL 4
#define DEF_GROUP_CEIL 4
#define DEF_OUTPUT_CEIL 4
#define DEF_DST_CEIL 4

struct analyze_closures_env {
	const struct fsm_alloc *alloc;
	const struct fsm *fsm;
	struct interned_state_set_pool *issp;

	/* TODO: iters and outputs are never in use at the same time, so
	 * they could potentially share the same underlying allocation.
	 * It may not be worth the extra complexity though. */

	/* Temporary state for iterators */
	size_t iter_ceil;
	size_t iter_count;
	struct ac_iter {
		/* if info.to is AC_NO_STATE then it's done */
		struct edge_group_iter iter;
		struct edge_group_iter_info info;
	} *iters;

	struct ipriq *pq;

	/* All sets of labels leading to states,
	 * stored in ascending order. */
	size_t group_ceil;
	size_t group_count;
	struct ac_group {
		fsm_state_t to;
		/* words_used & (1U << x) -> labels[x] has bits set */
		uint8_t words_used;
		uint64_t labels[256/64];
	} *groups;

	/* A collection of (label set -> interned state set) pairs. */
	size_t output_ceil;
	size_t output_count;
	struct ac_output {
		uint64_t labels[256/64];
		interned_state_set_id iss;
	} *outputs;

	size_t dst_ceil;
	fsm_state_t *dst;
};

static int
analyze_closures_for_iss(struct analyze_closures_env *env,
	interned_state_set_id curr_iss);

static int
analyze_closures__init_iterators(struct analyze_closures_env *env,
	const struct state_set *ss, size_t set_count);

static int
analyze_closures__init_groups(struct analyze_closures_env *env);

enum ac_collect_res {
	AC_COLLECT_DONE,
	AC_COLLECT_EMPTY,
	AC_COLLECT_ERROR = -1,
};

static enum ac_collect_res
analyze_closures__collect(struct analyze_closures_env *env);

static int
analyze_closures__analyze(struct analyze_closures_env *env);

static int
analyze_closures__save_output(struct analyze_closures_env *env,
    const uint64_t labels[256/4], interned_state_set_id iss);

static int
analyze_closures__grow_iters(struct analyze_closures_env *env,
    size_t set_count);

static int
analyze_closures__grow_groups(struct analyze_closures_env *env);

static int
analyze_closures__grow_dst(struct analyze_closures_env *env);

static int
analyze_closures__grow_outputs(struct analyze_closures_env *env);

static int
map_add(struct map *map,
	fsm_state_t dfastate, interned_state_set_id iss, struct mapping **new_mapping);

static int
map_find(const struct map *map, interned_state_set_id iss,
	struct mapping **mapping);

static void
map_free(struct map *map);

static struct mapping *
map_first(struct map *map, struct map_iter *iter);

static struct mapping *
map_next(struct map_iter *iter);

static int
add_reverse_mapping(const struct fsm_alloc *alloc,
	struct reverse_mapping *reverse_mappings,
	fsm_state_t dfastate, fsm_state_t nfa_state);

static int
det_copy_capture_actions(struct reverse_mapping *reverse_mappings,
	struct fsm *dst, struct fsm *src);

static int
grow_map(struct map *map);

static int
remap_capture_actions(struct map *map, struct interned_state_set_pool *issp,
	struct fsm *dst_dfa, struct fsm *src_nfa);

static struct mappingstack *
stack_init(const struct fsm_alloc *alloc);

static void
stack_free(struct mappingstack *stack);

static int
stack_push(struct mappingstack *stack, struct mapping *item);

static struct mapping *
stack_pop(struct mappingstack *stack);

#endif

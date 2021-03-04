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

#include "internal.h"
#include "capture.h"
#include "endids.h"

#define DUMP_MAPPING 0
#define LOG_DETERMINISE_CLOSURES 0
#define LOG_DETERMINISE_CAPTURES 0
#define LOG_SYMBOL_CLOSURE 0

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
		struct interned_state_set *iss;
		size_t dfastate;
		fsm_state_t oldstate;
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

#define MAX_EGM 100		/* FIXME: make dynamic and not global */

/* This should be stored in a dynamic set later,
 * keyed on <on, to, first>. Locality on 'on' may be beneficial. */
struct edge_group_mapping {
	fsm_state_t on;
	fsm_state_t to;

	/* first label, which stands for all the labels */
	unsigned char first;
	uint64_t labels[256/64];
};

static int
save_egm(struct edge_group_mapping *egm, size_t *egm_count,
	fsm_state_t on, fsm_state_t to, unsigned char first,
	uint64_t *labels);

static int
load_egm(const struct edge_group_mapping *egm, size_t egm_count,
	fsm_state_t on, unsigned char first,
	uint64_t *labels);

static int
map_add(struct map *map,
	fsm_state_t dfastate, struct interned_state_set *iss,
	fsm_state_t oldstate, struct mapping **new_mapping);

static int
map_find(const struct map *map, struct interned_state_set *iss,
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
interned_symbol_closure_without_epsilons(const struct fsm *fsm, fsm_state_t s,
	struct interned_state_set_pool *issp,
	struct interned_state_set *sclosures[static FSM_SIGMA_COUNT],
	struct edge_group_mapping *egm, size_t *egm_count);

static int
grow_map(struct map *map);

static int
remap_capture_actions(struct map *map,
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

#ifndef MINIMISE_INTERNAL_H
#define MINIMISE_INTERNAL_H

#define DEF_ENDS_CEIL 2
#define DEF_STATE_INFO_FROM_CEIL 2

#define DEF_SPLITTER_STACK_CEIL 1

#define NO_DEPTH ((size_t)-1)
#define NO_PARTITION ((fsm_state_t)-1)
#define NO_ID ((fsm_state_t)-1)

#define DUMP_BACKWARDS_DEPTH_INFO 1
#define DUMP_COARSE_PARTITIONS 1
#define DUMP_FINE_PARTITIONS 1

static int
min_brz(struct fsm *fsm, size_t *minimized_state_count);

static void
collect_labels(const struct fsm *fsm,
    unsigned char *labels, size_t *label_count);

static int
min_naive(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count);

static fsm_state_t
naive_delta(const struct fsm *fsm, fsm_state_t id, unsigned char label);


struct moore_env {
	const struct fsm *fsm;
	fsm_state_t dead_state;
	fsm_state_t *state_ecs;
	fsm_state_t *jump;
	fsm_state_t *ecs;
};

static int
min_moore(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count);

static void
moore_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump);

static int
moore_try_partition(struct moore_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst);

#if DO_HOPCROFT
struct hopcroft_env {
	const struct fsm *fsm;
	fsm_state_t ec_freelist;
	fsm_state_t max_ec_in_use;

	fsm_state_t dead_state;
	fsm_state_t *state_ecs;
	fsm_state_t *jump;

	const unsigned char *labels;
	size_t label_count;

	size_t ecs_ceil;
	fsm_state_t *ecs;

	size_t updated_ecs_ceil;
	struct updated_ecs {
		fsm_state_t src;
		fsm_state_t dst[2];
	} *updated_ecs;

	fsm_state_t *label_offsets;
	fsm_state_t *label_index;
	uint64_t *ec_filter;
	size_t ec_filter_bytes;

	struct {
		size_t index[256];	/* label -> top splitter on stack */
		size_t count;
		size_t ceil;
		struct splitter {
			fsm_state_t ec;
			unsigned char label;
			size_t prev;
		} *splitters;
	} stack;
};

static int
min_hopcroft(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count);

static void
hopcroft_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump);

enum hopcroft_try_partition_res {
	HTP_NO_CHANGE,
	HTP_PARTITIONED,
	HTP_ERROR_ALLOC = -1
};
static enum hopcroft_try_partition_res
hopcroft_try_partition(struct hopcroft_env *env,
    fsm_state_t ec_splitter, unsigned char label,
    fsm_state_t ec_src,
    fsm_state_t *ec_dst_a, fsm_state_t *ec_dst_b,
    size_t partition_counts[2]);

static int
hopcroft_schedule_splitter(struct hopcroft_env *env,
    fsm_state_t ec, unsigned char label);

static int
hopcroft_update_splitter_stack(struct hopcroft_env *env,
    unsigned char label, fsm_state_t ec_src,
    fsm_state_t ec_dst_a, fsm_state_t ec_dst_b,
    fsm_state_t ec_min);

static int
hopcroft_grow_updated_ecs(struct hopcroft_env *env);

static int
hopcroft_grow_ecs_freelist(struct hopcroft_env *env);

static int
hopcroft_build_label_index(struct hopcroft_env *env,
    const unsigned char *labels, size_t label_count);

static void
hopcroft_mark_ec_filter(struct hopcroft_env *env,
    unsigned char label);

static int
hopcroft_check_ec_filter(const uint64_t *filter,
    fsm_state_t ec_id);
#endif

#endif

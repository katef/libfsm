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


struct hopcroft_env {
	const struct fsm *fsm;
	fsm_state_t dead_state;
	fsm_state_t *state_ecs;
	fsm_state_t *jump;
	fsm_state_t *ecs;
	uint64_t *ec_labels;

	struct {
		size_t count;
		size_t ceil;
		struct splitter {
			fsm_state_t ec;
			unsigned char label;
		} *splitters;
	} stack;
};

static int
min_hopcroft(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count);

static void
hopcroft_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump);

static int
hopcroft_try_partition(struct hopcroft_env *env,
    fsm_state_t ec_splitter, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
    size_t partition_counts[2]);

static int
hopcroft_schedule_splitter(struct hopcroft_env *env,
    fsm_state_t ec, unsigned char label);

static int
hopcroft_update_splitter_stack(struct hopcroft_env *env,
    unsigned char label, fsm_state_t ec_src, fsm_state_t ec_dst,
    fsm_state_t ec_min);


#endif

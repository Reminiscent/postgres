/*-------------------------------------------------------------------------
 *
 * goo.c
 *     Greedy operator ordering (GOO) join search for large join problems
 *
 * GOO is a deterministic greedy operator ordering algorithm that constructs
 * join relations iteratively, always committing to the cheapest legal join at
 * each step. The algorithm maintains a list of "clumps" (join components),
 * initially one per base relation. At each iteration, it evaluates all legal
 * pairs of clumps, selects the pair that produces the cheapest join according
 * to the selected greedy rule, and replaces those two clumps with the
 * resulting joinrel. This continues until only one clump remains.
 *
 * ALGORITHM COMPLEXITY:
 *
 * Time Complexity: O(n^3) where n is the number of base relations.
 * - The algorithm performs (n - 1) iterations, merging two clumps each time.
 * - At iteration i, there are (n - i + 1) remaining clumps, requiring
 *   O((n-i)^2) pair evaluations to find the cheapest join.
 * - Total: Sum of (n-i)^2 for i=1 to n-1 ≈ O(n^3)
 *
 * Hybrid join search first uses PostgreSQL's exact dynamic-programming join
 * enumeration for an initial exact prefix, then completes promising frontier
 * seeds with GOO.
 *
 * REFERENCES:
 *
 * This implementation is based on the algorithm described in:
 *
 * Leonidas Fegaras, "A New Heuristic for Optimizing Large Queries",
 * Proceedings of the 9th International Conference on Database and Expert
 * Systems Applications (DEXA '98), August 1998, Pages 726-735.
 * https://dl.acm.org/doi/10.5555/648311.754892
 *
 * The hybrid exact-prefix idea is inspired by IDP:
 *
 * Donald Kossmann and Konrad Stocker, "Iterative Dynamic Programming:
 * A New Class of Query Optimization Algorithms".
 * https://db.in.tum.de/research/publications/journals/idp.pdf
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *     src/backend/optimizer/path/goo.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "miscadmin.h"
#include "nodes/bitmapset.h"
#include "nodes/pathnodes.h"
#include "optimizer/geqo.h"
#include "optimizer/goo.h"
#include "optimizer/joininfo.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"

/*
 * Configuration defaults.  These are exposed as GUCs in guc_tables.c.
 */
bool		enable_goo_join_search = false;
int			goo_greedy_strategy = GOO_GREEDY_STRATEGY_COMBINED;
int			join_search_effort = DEFAULT_GEQO_EFFORT;

/*
 * Keep the hybrid completion tail small and stable.  This MVP considers at
 * most two frontier seeds: the minimum exact-prefix result size and the
 * minimum exact-prefix path cost.  Each seed is completed with COMBINED GOO,
 * which runs one RESULT_SIZE completion and one COST completion.
 */
#define HYBRID_PROMISING_SEED_LIMIT 2

/*
 * Working state for a single GOO search invocation.
 *
 * This structure holds all the state needed during a greedy join order search.
 * It manages three memory contexts with different lifetimes to avoid memory
 * bloat during large join searches.
 *
 * TODO: Consider using the extension_state mechanism in PlannerInfo (similar
 * to GEQO's approach) instead of passing GooState separately.
 */
typedef struct GooState
{
	PlannerInfo *root;			/* global planner state */
	MemoryContext goo_cxt;		/* long-lived (per-search) allocations */
	MemoryContext cand_cxt;		/* per-iteration candidate storage */
	MemoryContext scratch_cxt;	/* per-candidate speculative evaluation */
	List	   *clumps;			/* remaining join components (RelOptInfo *) */
	GooGreedyStrategy strategy; /* candidate comparison heuristic */

	/*
	 * "clumps" are similar to GEQO's concept (see geqo_eval.c): join
	 * components that haven't been merged yet. Initially one per base
	 * relation, gradually merged until one remains.
	 */
	bool		prune_cartesian;	/* skip clauseless joins in this pass? */
}			GooState;

/*
 * Candidate join between two clumps.
 *
 * This structure holds the greedy metrics from a speculative joinrel
 * evaluation. We create this lightweight structure in cand_cxt after discarding
 * the actual joinrel from scratch_cxt, allowing us to compare many candidates
 * without exhausting memory.
 */
typedef struct GooCandidate
{
	RelOptInfo *left;			/* left input clump */
	RelOptInfo *right;			/* right input clump */
	double		result_size;	/* estimated result size in bytes */
	Cost		startup_cost;	/* startup cost of cheapest path */
	Cost		total_cost;		/* total cost of cheapest path */
	int			disabled_nodes;	/* disabled nodes of cheapest path */
	Relids		joinrelids;		/* relids covered by this join */
}			GooCandidate;

typedef struct GooStrategyResult
{
	RelOptInfo *result;
	Cost		startup_cost;
	Cost		total_cost;
	int			disabled_nodes;
	List	   *join_rel_list;
	struct HTAB *join_rel_hash;
}			GooStrategyResult;

static GooState * goo_init_state(PlannerInfo *root, List *initial_rels,
								 GooGreedyStrategy strategy);
static void goo_destroy_state(GooState * state);
static RelOptInfo *goo_search_internal(GooState * state);
static void goo_restore_joinrel_state(PlannerInfo *root, int saved_rel_len,
									  struct HTAB *saved_hash);
static void goo_reset_probe_state(GooState * state, int saved_rel_len,
								  struct HTAB *saved_hash);
static GooCandidate * goo_build_candidate(GooState * state, RelOptInfo *left,
										  RelOptInfo *right);
static RelOptInfo *goo_commit_join(GooState * state, GooCandidate * cand);
static bool goo_candidate_better(GooGreedyStrategy strategy,
								 GooCandidate * a, GooCandidate * b);
static bool goo_candidate_prunable(GooState * state, RelOptInfo *left,
								   RelOptInfo *right);
static const char *goo_strategy_name(GooGreedyStrategy strategy);
static int	goo_compare_path_cost_fields(int disabled_nodes_a,
										 Cost startup_cost_a,
										 Cost total_cost_a,
										 int disabled_nodes_b,
										 Cost startup_cost_b,
										 Cost total_cost_b);
static bool goo_strategy_result_better(GooStrategyResult * a,
									   GooStrategyResult * b);
static GooStrategyResult goo_run_combined_strategy(PlannerInfo *root,
												   List *initial_rels,
												   List *base_join_rel_list,
												   struct HTAB *base_hash,
												   GooGreedyStrategy * best_strategy);
static GooStrategyResult goo_run_strategy(PlannerInfo *root, List *initial_rels,
										  List *base_join_rel_list,
										  struct HTAB *base_hash,
										  GooGreedyStrategy strategy);
static uint64 hybrid_approximate_work_budget(int levels_needed);
static uint64 hybrid_estimate_join_level_work(PlannerInfo *root, int level);
static int	hybrid_seed_result_size_cmp(RelOptInfo *seed_a,
										RelOptInfo *seed_b);
static int	hybrid_seed_cost_cmp(RelOptInfo *seed_a, RelOptInfo *seed_b);
static List *hybrid_select_promising_frontier(List *frontier);
static void hybrid_process_join_level(PlannerInfo *root, int level);
static List *hybrid_build_reduced_problem(List *initial_rels,
										  RelOptInfo *seed);
static GooStrategyResult hybrid_evaluate_seed_attempt(PlannerInfo *root,
													  List *initial_rels,
													  RelOptInfo *seed,
													  GooGreedyStrategy * best_strategy);

/*
 * goo_join_search
 *		Entry point for Greedy Operator Ordering join search algorithm.
 *
 * This function runs GOO over the supplied clumps.  For standalone GOO, the
 * clumps are the base relations from make_rel_from_joinlist().  For hybrid
 * completion, the clumps may include an exact-prefix seed plus the remaining
 * base relations.
 *
 * Returns the final RelOptInfo representing the join of all supplied clumps,
 * or errors out if no valid join order can be found.
 */
RelOptInfo *
goo_join_search(PlannerInfo *root, int levels_needed,
				List *initial_rels)
{
	GooState   *state;
	RelOptInfo *result;
	int			base_rel_count;
	struct HTAB *base_hash;

	(void) levels_needed;

	/* If COMBINED mode, try all strategies and return the better one */
	if (goo_greedy_strategy == GOO_GREEDY_STRATEGY_COMBINED)
	{
		GooStrategyResult best_result;
		GooGreedyStrategy best_strategy;
		List	   *base_join_rel_list;

		base_join_rel_list = root->join_rel_list;
		base_hash = root->join_rel_hash;

		best_result = goo_run_combined_strategy(root, initial_rels,
												base_join_rel_list,
												base_hash,
												&best_strategy);

		/*
		 * Pick the lowest-cost result across strategies.
		 */
		root->join_rel_list = best_result.join_rel_list;
		root->join_rel_hash = best_result.join_rel_hash;

		elog(DEBUG1, "GOO COMBINED mode: %s strategy chosen (cost: %.2f)",
			 goo_strategy_name(best_strategy), best_result.total_cost);

		return best_result.result;
	}

	/* Normal single-strategy mode */
	/* Initialize search state and memory contexts */
	state = goo_init_state(root, initial_rels, goo_greedy_strategy);

	/*
	 * Save initial state of join_rel_list and join_rel_hash so we can restore
	 * them if the search fails.
	 */
	base_rel_count = list_length(root->join_rel_list);
	base_hash = root->join_rel_hash;

	/* Run the main greedy search loop */
	result = goo_search_internal(state);

	if (result == NULL)
	{
		goo_destroy_state(state);

		/* Restore planner state before reporting error */
		goo_restore_joinrel_state(root, base_rel_count, base_hash);
		elog(ERROR, "GOO join search failed to find a valid join order");
	}

	goo_destroy_state(state);
	return result;
}

/*
 * hybrid_join_search
 *		Run an approximate-budget exact-prefix + seeded completion search.
 *
 * The search proceeds in three phases:
 *
 * 1. Map join_search_effort to an approximate exact-search work budget.
 * 2. Build exact DP levels while the next level's estimated work fits that
 *    budget.  Level 2 is always admitted as the first valid hybrid frontier.
 * 3. If exact search stops early, pick at most two frontier seeds: one with
 *    the minimum exact-prefix result size and one with the minimum exact-prefix
 *    path cost.  Each selected seed is completed with COMBINED GOO.
 *
 * TODO: This prototype runs one exact-prefix pass and then completes the
 * selected seeds with GOO.  A closer IDP1 implementation would select a seed,
 * reduce the problem, restart exact DP on the reduced problem, and repeat the
 * break/select/reduce cycle as needed.
 *
 * This is a testing implementation: unexpected states are treated as errors so
 * that problems surface immediately during development.
 */
RelOptInfo *
hybrid_join_search(PlannerInfo *root, int levels_needed, List *initial_rels)
{
	int			frontier_level = 0;
	int			last_nonempty_level = 1;
	List	   *frontier;
	int			remaining_rels;
	int			next_level = 0;
	List	   *promising_seeds;
	uint64		work_budget;
	uint64		work_used = 0;
	uint64		next_level_work = 0;
	GooStrategyResult winner = {0};
	GooGreedyStrategy winner_strategy = GOO_GREEDY_STRATEGY_COST;
	bool		have_winner = false;

	root->assumeReplanning = true;
	work_budget = hybrid_approximate_work_budget(levels_needed);

	Assert(root->join_rel_level == NULL);

	root->join_rel_level = (List **) palloc0((levels_needed + 1) * sizeof(List *));
	root->join_rel_level[1] = initial_rels;

	for (int lev = 2; lev <= levels_needed; lev++)
	{
		uint64		level_work;

		level_work = hybrid_estimate_join_level_work(root, lev);
		if (lev > 2 && work_used + level_work > work_budget)
		{
			next_level = lev;
			next_level_work = level_work;
			break;
		}

		join_search_one_level(root, lev);
		hybrid_process_join_level(root, lev);
		work_used += level_work;

		if (root->join_rel_level[lev] == NIL)
		{
			if (lev == levels_needed)
				elog(ERROR, "hybrid join search failed to build any %d-way joins", levels_needed);
			continue;
		}

		last_nonempty_level = lev;

		if (lev == levels_needed)
		{
			RelOptInfo *exact_rel;

			Assert(list_length(root->join_rel_level[lev]) == 1);
			exact_rel = linitial_node(RelOptInfo, root->join_rel_level[lev]);
			elog(DEBUG2,
				 "HS exact-complete: levels=%d effort=%d approx_work_budget=" UINT64_FORMAT " approx_exact_work=" UINT64_FORMAT " frontier_level=%d",
				 levels_needed, join_search_effort, work_budget, work_used, lev);
			root->join_rel_level = NULL;
			return exact_rel;
		}
	}

	frontier_level = last_nonempty_level;
	if (frontier_level <= 0)
		elog(ERROR, "hybrid join search stopped without a valid frontier level");
	if (frontier_level < 2)
		elog(ERROR, "hybrid join search stopped before building a 2-way frontier");

	frontier = root->join_rel_level[frontier_level];
	if (frontier == NIL)
		elog(ERROR, "hybrid join search frontier at level %d is empty", frontier_level);

	elog(DEBUG2,
		 "HS exact-prefix-stop: levels=%d effort=%d approx_work_budget=" UINT64_FORMAT " approx_exact_work=" UINT64_FORMAT " exact_dp_level=%d exact_dp_rels=%d next_level=%d next_level_work=" UINT64_FORMAT,
		 levels_needed, join_search_effort, work_budget, work_used,
		 frontier_level, list_length(frontier), next_level, next_level_work);

	/*
	 * Prevent heuristic make_join_rel() calls from appending to the exact DP
	 * level arrays.  The frontier list itself remains valid as a read-only
	 * list of seed states.
	 *
	 * TODO: Rework seed evaluation to use per-seed temporary contexts and
	 * final-winner rebuild, similar in spirit to GEQO.
	 */
	root->join_rel_level = NULL;

	remaining_rels = levels_needed - frontier_level;

	promising_seeds = hybrid_select_promising_frontier(frontier);
	if (promising_seeds == NIL)
		elog(ERROR, "hybrid join search did not find any promising frontier seed");

	foreach_ptr(RelOptInfo, seed, promising_seeds)
	{
		GooStrategyResult seed_result;
		GooGreedyStrategy seed_strategy;

		seed_result = hybrid_evaluate_seed_attempt(root, initial_rels, seed,
												   &seed_strategy);

		if (!have_winner ||
			goo_strategy_result_better(&seed_result, &winner))
		{
			winner = seed_result;
			winner_strategy = seed_strategy;
			have_winner = true;
		}
	}

	if (!have_winner)
		elog(ERROR, "hybrid join search did not run any completion attempt");

	root->join_rel_list = winner.join_rel_list;
	root->join_rel_hash = winner.join_rel_hash;
	elog(DEBUG2,
		 "HS frontier-complete: levels=%d effort=%d approx_work_budget=" UINT64_FORMAT " approx_exact_work=" UINT64_FORMAT " frontier_level=%d frontier_seeds=%d promising_seeds=%d remaining_rels=%d winner_strategy=%s winner_cost=%.2f winner_startup=%.2f",
		 levels_needed, join_search_effort, work_budget, work_used,
		 frontier_level, list_length(frontier), list_length(promising_seeds),
		 remaining_rels,
		 goo_strategy_name(winner_strategy), winner.total_cost,
		 winner.startup_cost);

	return winner.result;
}

/*
 * goo_init_state
 *		Initialize per-search state and memory contexts.
 *
 * Creates the GooState structure and three memory contexts with different
 * lifetimes:
 *
 * - goo_cxt: Lives for the entire search, holds the clumps list and state.
 * - cand_cxt: Reset after each iteration, holds candidate structures during
 *   the comparison phase.
 * - scratch_cxt: Reset after each candidate evaluation, holds speculative
 *   joinrels that are discarded before committing to a choice.
 *
 * The three-context design prevents memory bloat during large join searches
 * where we may evaluate hundreds or thousands of candidate joins.
 */
static GooState *
goo_init_state(PlannerInfo *root, List *initial_rels,
			   GooGreedyStrategy strategy)
{
	MemoryContext oldcxt;
	GooState   *state;

	oldcxt = MemoryContextSwitchTo(root->planner_cxt);

	state = palloc(sizeof(GooState));
	state->root = root;
	state->clumps = NIL;
	state->prune_cartesian = false;
	state->strategy = strategy;

	/* Create the three-level memory context hierarchy */
	state->goo_cxt = AllocSetContextCreate(root->planner_cxt, "GOOStateContext",
										   ALLOCSET_DEFAULT_SIZES);
	state->cand_cxt = AllocSetContextCreate(state->goo_cxt, "GOOCandidateContext",
											ALLOCSET_SMALL_SIZES);
	state->scratch_cxt = AllocSetContextCreate(
											   state->goo_cxt, "GOOScratchContext", ALLOCSET_SMALL_SIZES);

	/*
	 * Copy the initial_rels list into goo_cxt. This becomes our working
	 * clumps list that we'll modify throughout the search.
	 */
	MemoryContextSwitchTo(state->goo_cxt);
	state->clumps = list_copy(initial_rels);

	MemoryContextSwitchTo(oldcxt);

	return state;
}

/*
 * goo_destroy_state
 *		Free all memory allocated for the GOO search.
 *
 * Deletes the goo_cxt memory context (which recursively deletes cand_cxt
 * and scratch_cxt as children) and then frees the state structure itself.
 * This is called after the search completes successfully or fails.
 */
static void
goo_destroy_state(GooState * state)
{
	MemoryContextDelete(state->goo_cxt);
	pfree(state);
}

/*
 * goo_compare_path_cost_fields
 *		Compare path cost fields using compare_path_costs() ordering.
 */
static int
goo_compare_path_cost_fields(int disabled_nodes_a,
							 Cost startup_cost_a,
							 Cost total_cost_a,
							 int disabled_nodes_b,
							 Cost startup_cost_b,
							 Cost total_cost_b)
{
	if (disabled_nodes_a < disabled_nodes_b)
		return -1;
	if (disabled_nodes_a > disabled_nodes_b)
		return 1;

	if (total_cost_a < total_cost_b)
		return -1;
	if (total_cost_a > total_cost_b)
		return 1;

	if (startup_cost_a < startup_cost_b)
		return -1;
	if (startup_cost_a > startup_cost_b)
		return 1;

	return 0;
}

/*
 * goo_strategy_result_better
 *		Compare completed strategy results by final path cost.
 */
static bool
goo_strategy_result_better(GooStrategyResult * a, GooStrategyResult * b)
{
	return goo_compare_path_cost_fields(a->disabled_nodes,
										a->startup_cost,
										a->total_cost,
										b->disabled_nodes,
										b->startup_cost,
										b->total_cost) < 0;
}

/*
 * goo_run_strategy
 *		Run one concrete greedy strategy and capture its private join state.
 */
static GooStrategyResult
goo_run_strategy(PlannerInfo *root, List *initial_rels,
				 List *base_join_rel_list, struct HTAB *base_hash,
				 GooGreedyStrategy strategy)
{
	GooStrategyResult result;
	GooState   *state;
	MemoryContext oldcxt;

	result.result = NULL;
	result.startup_cost = 0;
	result.total_cost = 0;
	result.disabled_nodes = 0;
	result.join_rel_list = NIL;
	result.join_rel_hash = NULL;

	oldcxt = MemoryContextSwitchTo(root->planner_cxt);
	root->join_rel_list = list_copy(base_join_rel_list);
	root->join_rel_hash = NULL;
	MemoryContextSwitchTo(oldcxt);

	state = goo_init_state(root, initial_rels, strategy);
	result.result = goo_search_internal(state);

	if (result.result != NULL)
	{
		if (result.result->cheapest_total_path == NULL)
			elog(ERROR, "GOO %s strategy produced a result without a cheapest path",
				 goo_strategy_name(strategy));

		result.startup_cost =
			result.result->cheapest_total_path->startup_cost;
		result.total_cost = result.result->cheapest_total_path->total_cost;
		result.disabled_nodes =
			result.result->cheapest_total_path->disabled_nodes;
	}

	result.join_rel_list = root->join_rel_list;
	result.join_rel_hash = root->join_rel_hash;

	goo_destroy_state(state);

	root->join_rel_list = base_join_rel_list;
	root->join_rel_hash = base_hash;

	return result;
}

/*
 * goo_run_combined_strategy
 *		Run the result-size and cost strategies and choose the better result.
 */
static GooStrategyResult
goo_run_combined_strategy(PlannerInfo *root, List *initial_rels,
						  List *base_join_rel_list, struct HTAB *base_hash,
						  GooGreedyStrategy * best_strategy)
{
	static const GooGreedyStrategy combined_strategies[] = {
		GOO_GREEDY_STRATEGY_RESULT_SIZE,
		GOO_GREEDY_STRATEGY_COST
	};
	GooStrategyResult best_result = {0};
	bool		have_best = false;

	if (best_strategy != NULL)
		*best_strategy = GOO_GREEDY_STRATEGY_COST;

	for (int i = 0; i < lengthof(combined_strategies); i++)
	{
		GooGreedyStrategy strategy = combined_strategies[i];
		GooStrategyResult strategy_result;

		strategy_result = goo_run_strategy(root, initial_rels,
										   base_join_rel_list,
										   base_hash,
										   strategy);

		if (strategy_result.result == NULL)
			elog(ERROR, "GOO %s strategy failed to find a valid join order",
				 goo_strategy_name(strategy));

		if (!have_best ||
			goo_strategy_result_better(&strategy_result, &best_result))
		{
			best_result = strategy_result;
			if (best_strategy != NULL)
				*best_strategy = strategy;
			have_best = true;
		}
	}

	if (!have_best)
		elog(ERROR, "GOO join search failed: all strategies exhausted without a valid join order");

	return best_result;
}

/*
 * goo_search_internal
 *		Main greedy search loop.
 *
 * Implements a two-pass algorithm at each iteration:
 *
 * Pass 1: Evaluate only clause-connected pairs (joins with a clause or join
 *         order restriction).
 *
 * Pass 2: If no legal clause-connected pair exists, allow Cartesian products
 *         to guarantee progress.
 *
 * After selecting the best candidate, we permanently create its joinrel in
 * planner_cxt and replace the two input clumps with this new joinrel. This
 * continues until only one clump remains.
 *
 * The function runs primarily in goo_cxt, temporarily switching to planner_cxt
 * when creating permanent joinrels and to scratch_cxt when evaluating
 * speculative candidates.
 *
 * Returns the final joinrel spanning all base relations, or NULL on failure.
 */
static RelOptInfo *
goo_search_internal(GooState * state)
{
	RelOptInfo *final_rel = NULL;
	MemoryContext oldcxt;

	/*
	 * Switch to goo_cxt for the entire search process. This ensures that all
	 * operations on state->clumps and related structures happen in the
	 * correct memory context.
	 */
	oldcxt = MemoryContextSwitchTo(state->goo_cxt);

	while (list_length(state->clumps) > 1)
	{
		ListCell   *lc1;
		GooCandidate *best_candidate = NULL;

		/* Allow query cancellation during long join searches */
		CHECK_FOR_INTERRUPTS();

		for (int pass = 0; pass < 2; pass++)
		{
			bool		prune_cartesian = (pass == 0);

			/* Reset candidate context for this pass */
			MemoryContextReset(state->cand_cxt);
			state->prune_cartesian = prune_cartesian;
			best_candidate = NULL;

			/*
			 * Evaluate all viable candidate pairs and select the best.
			 *
			 * For each pair that passes the pruning check, we do a full
			 * speculative evaluation using make_join_rel() to get accurate
			 * costs. The candidate with the best cost (according to
			 * goo_candidate_better) is remembered and will be committed after
			 * this pass.
			 *
			 * TODO: Consider caching cheap legality/connectivity or cost
			 * estimates across iterations; keep it simple for now.
			 */
			for (lc1 = list_head(state->clumps); lc1 != NULL;
				 lc1 = lnext(state->clumps, lc1))
			{
				RelOptInfo *left = lfirst_node(RelOptInfo, lc1);
				ListCell   *lc2 = lnext(state->clumps, lc1);

				for (; lc2 != NULL; lc2 = lnext(state->clumps, lc2))
				{
					RelOptInfo *right = lfirst_node(RelOptInfo, lc2);
					GooCandidate *cand;

					cand = goo_build_candidate(state, left, right);
					if (cand == NULL)
						continue;

					/* Track the best candidate seen so far */
					if (best_candidate == NULL ||
						goo_candidate_better(state->strategy,
											 cand, best_candidate))
						best_candidate = cand;
				}
			}

			if (best_candidate != NULL || !prune_cartesian)
				break;
		}

		/* No legal join candidate found for this iteration. */
		if (best_candidate == NULL)
		{
			MemoryContextSwitchTo(oldcxt);
			return NULL;
		}

		/*
		 * Commit the best candidate: create the joinrel permanently and
		 * update the clumps list.
		 */
		final_rel = goo_commit_join(state, best_candidate);
		if (final_rel == NULL)
		{
			MemoryContextSwitchTo(oldcxt);
			elog(ERROR, "GOO join search failed to commit join");
		}
	}

	/* Switch back to the original context before returning */
	MemoryContextSwitchTo(oldcxt);

	return final_rel;
}

/*
 * goo_candidate_prunable
 *		Determine whether a candidate pair should be skipped.
 *
 * We use a two-level pruning strategy:
 *
 * 1. Pairs with join clauses or join-order restrictions are never prunable.
 *    These represent natural joins or required join orders (e.g., from outer
 *    joins or LATERAL references).
 *
 * 2. If prune_cartesian is true, we prune Cartesian products to avoid
 *    evaluating expensive cross joins when better options are available.
 *
 * If no legal clause-connected pairs exist in the current iteration,
 * goo_search_internal() will retry with prune_cartesian disabled.
 *
 * Returns true if the pair should be pruned (skipped), false otherwise.
 */
static bool
goo_candidate_prunable(GooState * state, RelOptInfo *left,
					   RelOptInfo *right)
{
	PlannerInfo *root = state->root;
	bool		has_clause = have_relevant_joinclause(root, left, right);
	bool		has_restriction = have_join_order_restriction(root, left, right);

	if (has_clause || has_restriction)
		return false;			/* never prune clause-connected joins */

	return state->prune_cartesian;
}

/*
 * goo_build_candidate
 *		Evaluate a potential join between two clumps and return a candidate.
 *
 * This function performs a speculative join evaluation to extract greedy metrics
 * without permanently creating the joinrel. The process is:
 *
 * 1. Check basic viability (pruning, overlapping relids).
 * 2. Switch to scratch_cxt and create the joinrel using make_join_rel().
 * 3. Generate paths (including partitionwise and parallel variants).
 * 4. Extract the greedy metrics from the join relation.
 * 5. Discard the joinrel by calling goo_reset_probe_state().
 * 6. Create a lightweight GooCandidate in cand_cxt with the extracted metrics.
 *
 * This evaluate-and-discard pattern prevents memory bloat when evaluating
 * many candidates. The winning candidate will be rebuilt permanently later
 * by goo_commit_join().
 *
 * Returns a GooCandidate structure, or NULL if the join is illegal or
 * overlapping. Assumes the caller is in goo_cxt.
 */
static GooCandidate * goo_build_candidate(GooState * state, RelOptInfo *left,
										  RelOptInfo *right)
{
	PlannerInfo *root = state->root;
	MemoryContext oldcxt;
	int			saved_rel_len;
	struct HTAB *saved_hash;
	RelOptInfo *joinrel;
	Path	   *cheapest_path;
	double		result_size;
	Cost		startup_cost;
	Cost		total_cost;
	int			disabled_nodes;
	GooCandidate *cand;
	bool		is_top_rel;

	/* Skip if this pair should be pruned */
	if (goo_candidate_prunable(state, left, right))
		return NULL;

	/* Sanity check: ensure the clumps don't overlap */
	if (bms_overlap(left->relids, right->relids))
		return NULL;

	/*
	 * Save state before speculative join evaluation. We'll create the joinrel
	 * in scratch_cxt and then discard it.
	 */
	saved_rel_len = list_length(root->join_rel_list);
	saved_hash = root->join_rel_hash;

	/* Switch to scratch_cxt for speculative joinrel creation */
	oldcxt = MemoryContextSwitchTo(state->scratch_cxt);

	/*
	 * Create the joinrel and generate all its paths.
	 *
	 * TODO: This is the most expensive part of GOO. Each candidate evaluation
	 * performs full path generation via make_join_rel().
	 */
	joinrel = make_join_rel(root, left, right);

	if (joinrel == NULL)
	{
		/* Invalid or illegal join, clean up and return NULL */
		MemoryContextSwitchTo(oldcxt);
		goo_reset_probe_state(state, saved_rel_len, saved_hash);
		return NULL;
	}

	is_top_rel = bms_equal(joinrel->relids, root->all_query_rels);

	generate_partitionwise_join_paths(root, joinrel);
	if (!is_top_rel)
		generate_useful_gather_paths(root, joinrel, false);
	set_cheapest(joinrel);

	if (joinrel->grouped_rel != NULL && !is_top_rel)
	{
		RelOptInfo *grouped_rel = joinrel->grouped_rel;

		Assert(IS_GROUPED_REL(grouped_rel));

		generate_grouped_paths(root, grouped_rel, joinrel);
		set_cheapest(grouped_rel);
	}

	if (joinrel->cheapest_total_path == NULL)
		elog(ERROR, "GOO failed to find a cheapest path for a candidate join");

	cheapest_path = joinrel->cheapest_total_path;
	result_size = joinrel->rows * joinrel->reltarget->width;
	startup_cost = cheapest_path->startup_cost;
	total_cost = cheapest_path->total_cost;
	disabled_nodes = cheapest_path->disabled_nodes;

	/*
	 * Switch back to goo_cxt and discard the speculative joinrel.
	 * goo_reset_probe_state() will clean up join_rel_list, join_rel_hash, and
	 * reset scratch_cxt to free all the joinrel's memory.
	 */
	MemoryContextSwitchTo(oldcxt);
	goo_reset_probe_state(state, saved_rel_len, saved_hash);

	/*
	 * Now create the candidate structure in cand_cxt. This will survive until
	 * the end of this iteration (when cand_cxt is reset).
	 */
	oldcxt = MemoryContextSwitchTo(state->cand_cxt);
	cand = palloc(sizeof(GooCandidate));
	cand->left = left;
	cand->right = right;
	cand->result_size = result_size;
	cand->startup_cost = startup_cost;
	cand->total_cost = total_cost;
	cand->disabled_nodes = disabled_nodes;
	cand->joinrelids = bms_union(left->relids, right->relids);
	MemoryContextSwitchTo(oldcxt);

	return cand;
}

/*
 * goo_restore_joinrel_state
 *		Restore the planner's join_rel_list and join_rel_hash.
 *
 * This is shared by failure cleanup and speculative candidate probing.
 */
static void
goo_restore_joinrel_state(PlannerInfo *root, int saved_rel_len,
						  struct HTAB *saved_hash)
{
	int			cur_rel_len;

	cur_rel_len = list_length(root->join_rel_list);

	/*
	 * If the strategy reused the caller's hash table, remove the tail entries
	 * before truncating the list.  If it built a private replacement hash
	 * table, just drop that table by restoring the caller's hash pointer.
	 */
	if (saved_hash != NULL && root->join_rel_hash == saved_hash &&
		cur_rel_len > saved_rel_len)
	{
		for (int i = saved_rel_len; i < cur_rel_len; i++)
		{
			RelOptInfo *joinrel = list_nth_node(RelOptInfo,
												root->join_rel_list, i);
			bool		found;

			(void) hash_search(saved_hash, &(joinrel->relids),
							   HASH_REMOVE, &found);
			Assert(found);
		}
	}

	root->join_rel_list = list_truncate(root->join_rel_list, saved_rel_len);
	root->join_rel_hash = saved_hash;
}

/*
 * goo_reset_probe_state
 *		Clean up after a speculative joinrel evaluation.
 *
 * Reverts the planner's join_rel_list and join_rel_hash to their saved state,
 * removing any joinrels that were created during speculative evaluation.
 * Also resets scratch_cxt to free all memory used by the discarded joinrel
 * and its paths.
 *
 * This function is called after extracting cost metrics from a speculative
 * joinrel that we don't want to keep.
 */
static void
goo_reset_probe_state(GooState * state, int saved_rel_len,
					  struct HTAB *saved_hash)
{
	goo_restore_joinrel_state(state->root, saved_rel_len, saved_hash);
	MemoryContextReset(state->scratch_cxt);
}

/*
 * goo_commit_join
 *		Permanently create the chosen join and update the clumps list.
 *
 * After selecting the best candidate in an iteration, we need to permanently
 * create its joinrel (with all paths) and integrate it into the planner state.
 * This function:
 *
 * 1. Switches to planner_cxt and creates the joinrel using make_join_rel().
 *    Unlike the speculative evaluation, this joinrel is kept permanently.
 * 2. Generates partitionwise and parallel path variants.
 * 3. Determines the cheapest paths.
 * 4. Updates state->clumps by removing the two input clumps and adding the
 *    new joinrel as a single clump.
 *
 * The next iteration will treat this joinrel as an atomic unit that can be
 * joined with other remaining clumps.
 *
 * Returns the newly created joinrel. Assumes the caller is in goo_cxt.
 */
static RelOptInfo *
goo_commit_join(GooState * state, GooCandidate * cand)
{
	MemoryContext oldcxt;
	PlannerInfo *root = state->root;
	RelOptInfo *joinrel;
	bool		is_top_rel;

	/*
	 * Create the joinrel permanently in planner_cxt. Unlike the speculative
	 * evaluation in goo_build_candidate(), this joinrel will be kept and
	 * added to root->join_rel_list for use by the rest of the planner.
	 */
	oldcxt = MemoryContextSwitchTo(root->planner_cxt);

	joinrel = make_join_rel(root, cand->left, cand->right);
	if (joinrel == NULL)
	{
		MemoryContextSwitchTo(oldcxt);
		elog(ERROR, "GOO join search failed to create join relation");
	}

	/* Generate additional path variants, just like standard_join_search() */
	is_top_rel = bms_equal(joinrel->relids, root->all_query_rels);

	generate_partitionwise_join_paths(root, joinrel);
	if (!is_top_rel)
		generate_useful_gather_paths(root, joinrel, false);
	set_cheapest(joinrel);

	if (joinrel->grouped_rel != NULL && !is_top_rel)
	{
		RelOptInfo *grouped_rel = joinrel->grouped_rel;

		Assert(IS_GROUPED_REL(grouped_rel));

		generate_grouped_paths(root, grouped_rel, joinrel);
		set_cheapest(grouped_rel);
	}

	/*
	 * Switch back to goo_cxt and update the clumps list. Remove the two input
	 * clumps and add the new joinrel as a single clump.
	 */
	MemoryContextSwitchTo(oldcxt);

	state->clumps = list_delete_ptr(state->clumps, cand->left);
	state->clumps = list_delete_ptr(state->clumps, cand->right);
	state->clumps = lappend(state->clumps, joinrel);

	return joinrel;
}

/*
 * goo_candidate_better
 *		Compare two join candidates and determine which is better.
 *
 * Returns true if candidate 'a' should be preferred over candidate 'b'.
 */
static bool
goo_candidate_better(GooGreedyStrategy strategy,
					 GooCandidate * a, GooCandidate * b)
{
	switch (strategy)
	{
		case GOO_GREEDY_STRATEGY_COMBINED:
			/* Should not be called in COMBINED mode */
			elog(ERROR, "goo_candidate_better should not be called in COMBINED mode");
			return false;

		case GOO_GREEDY_STRATEGY_RESULT_SIZE:
			if (a->result_size < b->result_size)
				return true;
			if (a->result_size > b->result_size)
				return false;
			break;

		case GOO_GREEDY_STRATEGY_COST:
			{
				int			cmp;

				cmp = goo_compare_path_cost_fields(a->disabled_nodes,
												   a->startup_cost,
												   a->total_cost,
												   b->disabled_nodes,
												   b->startup_cost,
												   b->total_cost);
				if (cmp < 0)
					return true;
				if (cmp > 0)
					return false;
			}
			break;
	}

	return bms_compare(a->joinrelids, b->joinrelids) < 0;
}

static const char *
goo_strategy_name(GooGreedyStrategy strategy)
{
	switch (strategy)
	{
		case GOO_GREEDY_STRATEGY_RESULT_SIZE:
			return "RESULT_SIZE";
		case GOO_GREEDY_STRATEGY_COST:
			return "COST";
		case GOO_GREEDY_STRATEGY_COMBINED:
			return "COMBINED";
	}

	return "UNKNOWN";
}

static uint64
hybrid_approximate_work_budget(int levels_needed)
{
	uint64		n;
	uint64		min_pool_size;
	uint64		max_pool_size;
	uint64		pool_size;
	uint64		search_space_size;

	n = (uint64) levels_needed;
	if (n <= 1)
		return 0;

	/*
	 * Approximate GEQO's default planning work scale.  With default
	 * geqo_pool_size and geqo_generations, GEQO's estimate is:
	 */
	/* pool_size = clamp(2^(n+1), 10 * geqo_effort, 50 * geqo_effort) */
	/* generations = pool_size */
	/* GEQO tree evaluations ~= pool_size + generations = 2 * pool_size */
	/* make_join_rel work ~= 2 * pool_size * (n - 1) */

	/*
	 * Use the same shape here so join_search_effort=5 has roughly the same
	 * default work scale as geqo_effort=5.  This remains an estimate: GEQO
	 * evaluates transient join trees, while hybrid search keeps exact DP
	 * frontier state and then completes selected seeds with GOO.
	 */
	min_pool_size = (uint64) 10 * (uint64) join_search_effort;
	max_pool_size = (uint64) 50 * (uint64) join_search_effort;

	if (n + 1 >= 63)
		pool_size = max_pool_size;
	else
	{
		search_space_size = UINT64CONST(1) << (int) (n + 1);

		if (search_space_size > max_pool_size)
			pool_size = max_pool_size;
		else if (search_space_size < min_pool_size)
			pool_size = min_pool_size;
		else
			pool_size = search_space_size;
	}

	return 2 * pool_size * (n - 1);
}

static uint64
hybrid_estimate_join_level_work(PlannerInfo *root, int level)
{
	List	  **joinrels = root->join_rel_level;
	uint64		work = 0;

	if (level == 2)
	{
		uint64		base_rels = list_length(joinrels[1]);

		return base_rels > 1 ? (base_rels * (base_rels - 1)) / 2 : 0;
	}

	work += (uint64) list_length(joinrels[level - 1]) *
		(uint64) list_length(joinrels[1]);

	for (int k = 2;; k++)
	{
		int			other_level = level - k;
		uint64		left_rels;
		uint64		right_rels;

		if (k > other_level)
			break;

		left_rels = list_length(joinrels[k]);
		right_rels = list_length(joinrels[other_level]);

		if (k == other_level)
			work += left_rels > 1 ? (left_rels * (left_rels - 1)) / 2 : 0;
		else
			work += left_rels * right_rels;
	}

	return work;
}

static int
hybrid_seed_result_size_cmp(RelOptInfo *seed_a, RelOptInfo *seed_b)
{
	double		score_a;
	double		score_b;
	int			cmp;

	score_a = seed_a->rows * (double) seed_a->reltarget->width;
	score_b = seed_b->rows * (double) seed_b->reltarget->width;

	if (score_a < score_b)
		return -1;
	if (score_a > score_b)
		return 1;

	cmp = bms_compare(seed_a->relids, seed_b->relids);
	if (cmp != 0)
		return cmp;

	return 0;
}

static int
hybrid_seed_cost_cmp(RelOptInfo *seed_a, RelOptInfo *seed_b)
{
	int			cmp;

	if (seed_a->cheapest_total_path == NULL || seed_b->cheapest_total_path == NULL)
		elog(ERROR, "hybrid join search cannot rank frontier seeds by cost without cheapest paths");

	cmp = compare_path_costs(seed_a->cheapest_total_path,
							 seed_b->cheapest_total_path,
							 TOTAL_COST);
	if (cmp != 0)
		return cmp;

	return bms_compare(seed_a->relids, seed_b->relids);
}

static List *
hybrid_select_promising_frontier(List *frontier)
{
	RelOptInfo *result_size_seed = NULL;
	RelOptInfo *cost_seed = NULL;
	List	   *promising_seeds = NIL;

	foreach_ptr(RelOptInfo, seed, frontier)
	{
		if (result_size_seed == NULL ||
			hybrid_seed_result_size_cmp(seed, result_size_seed) < 0)
			result_size_seed = seed;

		if (cost_seed == NULL ||
			hybrid_seed_cost_cmp(seed, cost_seed) < 0)
			cost_seed = seed;
	}

	if (result_size_seed != NULL)
		promising_seeds = lappend(promising_seeds, result_size_seed);

	if (cost_seed != NULL && cost_seed != result_size_seed)
		promising_seeds = lappend(promising_seeds, cost_seed);

	Assert(list_length(promising_seeds) <= HYBRID_PROMISING_SEED_LIMIT);
	return promising_seeds;
}

static void
hybrid_process_join_level(PlannerInfo *root, int level)
{
	foreach_ptr(RelOptInfo, rel, root->join_rel_level[level])
	{
		bool		is_top_rel;

		is_top_rel = bms_equal(rel->relids, root->all_query_rels);

		generate_partitionwise_join_paths(root, rel);
		if (!is_top_rel)
			generate_useful_gather_paths(root, rel, false);
		set_cheapest(rel);

		if (rel->grouped_rel != NULL && !is_top_rel)
		{
			RelOptInfo *grouped_rel = rel->grouped_rel;

			Assert(IS_GROUPED_REL(grouped_rel));

			generate_grouped_paths(root, grouped_rel, rel);
			set_cheapest(grouped_rel);
		}
	}
}

static List *
hybrid_build_reduced_problem(List *initial_rels, RelOptInfo *seed)
{
	List	   *reduced_rels = list_make1(seed);

	foreach_ptr(RelOptInfo, rel, initial_rels)
	{
		if (!bms_overlap(rel->relids, seed->relids))
			reduced_rels = lappend(reduced_rels, rel);
	}

	return reduced_rels;
}

static GooStrategyResult
hybrid_evaluate_seed_attempt(PlannerInfo *root, List *initial_rels,
							 RelOptInfo *seed,
							 GooGreedyStrategy * best_strategy)
{
	List	   *reduced_rels;
	List	   *saved_join_rel_list;
	struct HTAB *saved_hash;
	GooStrategyResult strategy_result;

	reduced_rels = hybrid_build_reduced_problem(initial_rels, seed);
	saved_join_rel_list = root->join_rel_list;
	saved_hash = root->join_rel_hash;

	strategy_result = goo_run_combined_strategy(root, reduced_rels,
												reduced_rels, NULL,
												best_strategy);

	root->join_rel_list = saved_join_rel_list;
	root->join_rel_hash = saved_hash;

	if (strategy_result.result == NULL)
		elog(ERROR, "hybrid join search failed to find a valid join order for a seed");

	return strategy_result;
}

# AGENTS.md

## Mission

Act like a PostgreSQL engineering agent that optimizes for:

- correctness,
- explicit evidence,
- reviewer efficiency,
- architectural fit,
- smallest coherent change,
- low context overhead.

## Core Model

For every non-trivial `change`, `investigate`, or `benchmark` task, declare these base fields before substantive work:

- **Task Type**: `change`, `investigate`, `review`, or `benchmark`
- **Target Readiness**: `RFC`, `WIP`, `Answered`, `Ready`, or `Perf`
- **Evidence Plan**: list the validation lanes and non-command evidence sources required to prove the task. Validation lanes come from `compile`, `nearest tests`, `regression`, `TAP`, `isolation`, `docs`, `release benchmark`, `committer review`. Evidence sources may include nearby code, nearby tests, official docs, committed patches with `Discussion:` links, or active unresolved threads.

For pure `review` tasks, declare **Task Type** and **Evidence Plan**. Omit **Target Readiness** unless the review is explicitly assessing whether proposed work is at a particular readiness target.

For `investigate` tasks, also declare a required **Investigation Kind**: `explain-current`, `bug-triage`, `design-explore`, or `rationale-trace`. Default to `explain-current` when the user is asking for understanding and there is no concrete failure evidence.

Use `pg-task-intake` for PostgreSQL work. Use `pg-debug-bugfix` after intake for diagnosis-focused `investigate` tasks of kind `bug-triage`. Use `agent-workflow-router` only when the target is this workflow itself.

## Non-Trivial Threshold

Treat a task as non-trivial when any of these are true:

- user-visible behavior, error behavior, or compatibility may change,
- more than one invariant or module boundary is likely involved,
- concurrency, visibility, locking, transactions, WAL, recovery, catalog state, client behavior, or performance are in scope,
- the rationale is unclear or likely contested,
- more than `compile` or `nearest tests` will be needed,
- the result will need durable task notes or reviewer-facing handoff.

## Durable Rules

1. Read in this authority order: nearby code, nearby tests, nearest README; official PostgreSQL docs/wiki/process pages; committed patches with `Discussion:` links; active unresolved threads.
2. For any non-trivial `change` or `investigate` task, write a short design note before substantive work: RFC for `change`; an investigation note for `investigate`. For `investigate`, do this before edits or temporary probes. For `bug-triage`, include symptom, expected behavior, proof plan, and root-cause hypothesis. For other investigation kinds, state the question, current evidence, likely invariants, and stopping condition.
3. Use canonical commands from `LOCAL_WORKFLOWS.md`. Correctness work defaults to `--enable-cassert --enable-debug`; performance conclusions require a release build.
4. Treat concurrency, visibility, locking, and transaction semantics as isolation-test candidates by default.
5. If docs, client behavior, output files, catalog state, compatibility, or upgrade behavior may change, call that out explicitly.
6. A task is not done without exact commands or exact evidence references, plus concise result summaries.
7. Before handoff of pure `review` work or any reviewable patch work, run a committer-style review using `pg-review-committer` and `code_review.md`. Pure `investigate` handoffs that do not propose a reviewable patch should still run `pg-ready-check` with `Answered` readiness and end with an investigation closeout: question answered, evidence consulted, stopping condition reached, remaining uncertainty, and recommended next step. `pg-review-committer` is not required for that pure investigation closeout.
8. For non-trivial design-sensitive `change` or `investigate` work, use `pg-design-research` when external system precedent or relevant literature would materially affect the design choice. PostgreSQL local sources remain authoritative.
9. Treat PostgreSQL mailing lists as first-class project evidence: `pgsql-hackers` for design and patch discussion, `pgsql-bugs` for bug reports and reproductions, `pgsql-performance` for workload and regression evidence, `pgsql-committers` for landed outcomes and backpatch signals.

## Anti-Patterns

Do not:

- invent rationale or mailing-list history,
- skip the design-note stage for non-trivial `change` or `investigate` work,
- treat one happy-path test as sufficient evidence,
- claim performance wins from a debug/cassert build,
- silently widen scope beyond the smallest coherent patch,
- mistake “builds locally” for “ready for review.”

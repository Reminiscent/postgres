---
name: pg-patch-review-deep-dive
description: "Use this skill for PostgreSQL patch reviews where discussion.txt is the discussion source and the review target is the current branch's applied changes relative to the agent base branch. Use when the user wants first-principles design review, regression validation, implementation walkthrough, Chinese teaching notes, committer-style findings, and adjacent-risk audit."
---

# pg-patch-review-deep-dive

## Purpose

Review an already-applied PostgreSQL patch or patch series as both a correctness review and a learning packet.

Use this when the user has usually created a topic branch from `agent`, applied patches with `git am`, and wants a durable Chinese explanation plus a real review.

## Defaults

Default base and target:

- base branch: `agent`
- target branch: current branch
- commits: `git log --oneline agent..HEAD`
- diff: `git diff --stat agent...HEAD` and `git diff agent...HEAD`
- discussion: `discussion.txt`

Do not apply patches by default. The normal review target is the current branch's changes relative to `agent`.

If the user gives an explicit base, range, or commit list, use that instead. If `agent` is not available or the current branch is `agent`, stop and ask for the intended base/range unless the user has already supplied it.

Use Chinese for the main prose. Keep PostgreSQL identifiers, file names, function names, SQL terms, and community terms in English.

## Durable Output

Write these files by default:

- `notes/tasks/<slug>/task.md`: review scope, base/range, evidence plan, stopping condition
- `notes/tasks/<slug>/log.md`: commands, sources, and concise results
- `notes/tasks/<slug>/learning.md`: Chinese teaching document
- `notes/tasks/<slug>/review.md`: findings and reviewer packet

## Workflow

Follow this order.

### 1. Frame the Review

Run `pg-task-intake` first.

Default classification:

- **Task Type**: `review`
- **Learning Objective**: `teaching-pack`
- **Evidence Plan**: `discussion.txt`, `agent..HEAD` commits, `agent...HEAD` diff, nearby code, nearby tests, canonical regression commands, and `pg-review-committer`

Record:

- current branch, `HEAD`, and merge base with `agent`
- exact commits from `agent..HEAD`
- whether `discussion.txt` exists
- worktree status, including unrelated user changes
- planned regression lane

Create `notes/tasks/<slug>/task.md` before deep review.

### 2. Start With a Simple Example

Begin `learning.md` with a minimal example that makes the problem visible.

For planner or estimator patches, use a tiny SQL/data example and explain:

- current behavior
- why it is insufficient
- what a correct estimate or planner decision should preserve
- what the patch changes

Do this before formulas, internals, or reviewer comments.

### 3. Explain the Discussion

Read `discussion.txt` as project evidence.

Extract the problem, proposed solution, reviewer objections, unresolved questions, expected evidence, and `Discussion:` links if present.

For each important comment, explain whether it is about semantics, architecture, tests, performance, compatibility, or style. State whether the applied branch answers it and what evidence would settle it.

Do not invent mailing-list rationale. If history materially affects the review and is not present locally, use `pg-rationale-trace` or `pg-design-research`.

### 4. Review the Design First

Before line-by-line code review, decide whether the high-level idea is sound.

For planner and estimator patches, check:

- what quantity is being estimated
- which inputs are authoritative
- whether the logic is selectivity, cardinality, upper bound, lower bound, or heuristic correction
- assumptions about independence, containment, uniqueness, duplication, `NULL`, ordering, or distinctness
- whether the change belongs in the touched planner stage
- whether the patch affects only the cases where the motivating invariant applies

State the design verdict plainly: correct, under-specified, over-scoped, misplaced, or needs more evidence.

### 5. Inspect the Relative Diff

Use `agent` as the default base:

- `git status --short`
- `git branch --show-current`
- `git merge-base agent HEAD`
- `git log --oneline --decorate agent..HEAD`
- `git diff --stat agent...HEAD`
- `git diff --name-only agent...HEAD`

Capture changed files, code path summary, tests, expected output changes, and docs or generated-file impact.

### 6. Validate

Use `LOCAL_WORKFLOWS.md` for exact commands. Prefer the smallest relevant regression lane first, then widen only when the blast radius requires it.

For correctness work, use a debug/cassert build unless the existing build is already suitable and no rebuild is needed. Record any `make clean` or `make distclean` reason.

Do not make performance claims from a debug/cassert build. Use the release build profile for performance conclusions.

### 7. Explain the Implementation

In `learning.md`, explain the implementation by file and function:

- surrounding context
- function ownership
- inputs and outputs
- new branches or formulas
- memory ownership or lifetime, if relevant
- why the code belongs in this layer

Include a call chain:

```text
SQL example
  -> parse/analyze stage, if relevant
  -> planner entry
  -> path or selectivity function
  -> patched function
  -> final estimate or plan consequence
```

For each changed test, explain which invariant it proves and which edge case remains uncovered.

### 8. Produce Review Findings

Run `pg-review-committer` with `code_review.md` before handoff.

Lead with findings, ordered by severity. Focus on semantic correctness, estimator or planner contracts, missed edge cases, wrong layer, coverage gaps, portability risk, and unnecessary scope.

If there are no findings, say so directly and name the remaining uncertainty.

### 9. Audit Adjacent Risk

Inspect three to five nearby paths where the same reasoning could fail.

Classify each path as `same risk`, `explicitly guarded`, `different invariant`, or `needs separate evidence`.

Recommend follow-up only when the pattern is real and evidence-backed.

## Final Shape

The final response should be short and point to the durable documents.

The teaching document should include:

1. simple example
2. problem essence
3. discussion summary
4. reviewer comments explained
5. high-level design review
6. `agent...HEAD` diff summary
7. validation commands and results
8. implementation walkthrough
9. call chain
10. review findings
11. adjacent-risk audit
12. final judgment and next step

Keep exact file paths, function names, commands, commit ranges, and test names visible. Separate evidence from inference.

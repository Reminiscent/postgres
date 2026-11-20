---
name: pg-learning-pack
description: Use this skill when a PostgreSQL task has a learning objective above fix-only, or the user asks why something happened, what its essence is, how the code path fits together, where similar risks live, or for reusable study material. It defines the learning levels and the learning.md closeout.
---

# pg-learning-pack

## Purpose

Make deeper understanding repeatable without slowing down fix-only work.

Learning output is an optional layer on top of correctness work, not a
replacement for it. Its durable output is `notes/tasks/<slug>/learning.md`,
using the template in `notes/tasks/_template/learning.md` unless a skill
prescribes its own structure, as `pg-patch-review-deep-dive` does for
patch reviews. It never replaces the evidence, readiness, or posting
workflow.

## Learning Objective

Choose one; the levels are cumulative, so higher levels include the lower ones:

- `fix-only`: stop at the smallest correct answer, patch, or review conclusion
- `root-cause`: explain the broken assumption, why the bug or behavior occurs, and why the chosen answer works
- `deep-dive`: add execution context, call chain, lifecycle placement, or architectural ownership
- `pattern-audit`: add a written sibling-path and adjacent-risk analysis
- `teaching-pack`: assemble the fullest reusable study packet from the task

Default to `fix-only` unless the user clearly asks for deeper
understanding or the output is explicitly meant to teach.

## Choosing the level

The intent can be declared in the intake or task note, or inferred from
the request:

- at least `root-cause` when the user asks why, what the essence is, why
  it happened, or why the patch works,
- at least `deep-dive` when the user asks for the call path, execution
  context, planner / executor / JIT placement, or how the layers fit,
- at least `pattern-audit` when the user asks about similar places,
  adjacent risks, what else to inspect, or how to avoid this class of
  bug,
- `teaching-pack` when the user wants reusable study material, a
  distilled write-up, or a durable handoff that teaches the pattern.

## Pack mapping

- `Root-Cause Pack`: `root-cause` and above
- `Best Practices Extracted`: `root-cause` and above
- `Context Pack`: `deep-dive` and above
- `Pattern Audit Pack`: `pattern-audit` and above
- `Refactor Pack`: optional; use it for `teaching-pack`, or when a cleanup
  or patch-series discussion is a real next step

The sibling sweep itself is not optional learning work: it is part of
every `change` and `bug-triage` task (see `pg-task-intake`). The
`Pattern Audit Pack` writes that sweep up as a reusable pattern and
search heuristic.

## Rules

- Write learning material after the correctness work is sound; it must
  not change the conclusion it explains.
- Separate evidence from inference, and keep file, function, and
  commit references exact.
- Do not promote learning rules into `AGENTS.md` until repeated tasks
  show the same learning friction recurring.

---
name: pg-rationale-trace
description: Use this skill when the current code shape or proposed change requires design history. Return compact rationale cards instead of dumping raw thread history into the prompt.
---

# pg-rationale-trace

## Purpose

Retrieve only the design history that changes the decision.

In PostgreSQL, important constraints often live in commit history, `Discussion:` links, `pgsql-hackers`, and official docs. The goal of this skill is to extract those constraints into compact cards.

## Inputs

- target files
- target function or symbol
- subsystem
- current behavior or desired change
- nearby commits or blame results

## Search order

1. nearby code comments and READMEs
2. nearby commit history
3. commits with `Discussion:` links
4. official docs or wiki pages
5. relevant mailing-list threads, using:
   - `pgsql-hackers` for design history,
   - `pgsql-bugs` for bug reports and repros,
   - `pgsql-performance` for workload and regression context,
   - `pgsql-committers` for landed outcomes and branch movement

Treat PostgreSQL mailing lists as the source of truth for project discussion history, not CommitFest metadata.
If external systems or academic literature are also needed, use `pg-design-research` separately rather than mixing those sources into the local rationale layer.

## Output unit

Return **one to three rationale cards** unless the task explicitly requires a larger pack.

Each card must include:

- `source`: commit, doc, or thread
- `provenance`: direct provenance, close precedent, or analogy
- `proposal`: what was being proposed or changed
- `main objections`: the strongest pushback or risk raised
- `decision or status`: what happened
- `implication`: what the current task should preserve, avoid, or justify
- `uncertainty`: what remains unclear

## Rules

- Prefer direct provenance over analogy.
- Never invent historical rationale.
- If no strong rationale is found, say so plainly.
- Do not paste large raw thread excerpts.
- Expand to a larger rationale pack only for `Ready`, `Perf`, or explicitly requested deep review.

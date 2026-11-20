---
name: pg-rationale-trace
description: Use this skill when the current code shape needs design history (rationale mode), or when a design or fix decision has more than one viable approach and past community discussion of the same or analogous choices should inform it (precedent mode). Return compact cards instead of dumping raw thread history into the prompt.
---

# pg-rationale-trace

## Purpose

Retrieve only the community history that changes the decision.

In PostgreSQL, important constraints often live in commit history,
`Discussion:` links, mailing-list threads, and official docs. This skill
extracts them into compact cards. It has two modes:

- `rationale`: why is the current code or behavior the way it is?
- `precedent`: at a decision point with two or more viable approaches,
  how did the community decide the same or an analogous choice before,
  and why?

Precedent comes before choosing, not after implementing. The community's
past positions on analogous choices are the best available predictor of
what reviewers will ask for. A choice that follows established precedent
needs little defense; one that contradicts it must say why, or it will
be sent back.

## When to use precedent mode

Use it when two or more approaches are viable and local code does not
force the choice, for example:

- which layer owns the logic: parser, planner, executor, or catalog,
- a conservative rule that enumerates safe cases versus a permissive
  rule that excludes known-unsafe ones,
- reusing existing machinery versus adding a new mechanism,
- raising an error versus falling back silently,
- a GUC or option versus none,
- a narrow back-branch fix versus a general HEAD fix,
- which test vehicle, or how much test,
- a bug fix that could go in more than one place, or be narrow or broad.

Skip it when only one approach is viable or the choice is mechanical.

## Inputs

- target files, functions, or symbols
- subsystem
- current behavior or desired change
- for precedent mode: the decision point and the candidate approaches

## Search order

### Rationale mode

1. nearby code comments and READMEs
2. nearby commit history
3. commits with `Discussion:` links
4. official docs or wiki pages
5. relevant mailing-list threads, using:
   - `pgsql-hackers` for design history,
   - `pgsql-bugs` for bug reports and repros,
   - `pgsql-performance` for workload and regression context,
   - `pgsql-committers` for landed outcomes and branch movement

### Precedent mode

Stop as soon as the precedent is clear; this is a targeted search, not
a history sweep.

1. Same code: the commits that shaped the code being changed, and the
   threads behind them.

   ```bash
   git log -L ':<function>:<file>' -s --format='%h %ad %an %s' --date=short
   git log --format='%h %ad %an %s%n  %(trailers:key=Discussion,valueonly,separator=%x20)' \
     --date=short -- <file-or-dir>
   ```

2. Analogous places: the same kind of decision elsewhere, such as
   sibling support functions, other node types, data types, or access
   methods, or the parallel feature. Find the commits that made that
   choice (`git log -S'<symbol>'`, `git log --grep='<term>'`) and read
   their `Discussion:` threads.
3. Similar bugs: how earlier bugs of the same class in this area were
   fixed, through `git log --grep='<symptom term>' -- <dir>` and `Bug:`
   trailers.
4. Archives: search for the design question itself at
   `https://www.postgresql.org/search/?m=1&q=<terms>`, and read whole
   threads at `https://www.postgresql.org/message-id/flat/<message-id>`.
   A local archive copy under `notes/`, when one exists, is faster.
5. Commitfest: earlier patches for the same idea, including those
   returned with feedback, rejected, or withdrawn. Their objections are
   the most direct predictor of the objections this work will meet.

Read threads cheaply: start with the final messages and the committers'
replies, then the message where the direction changed; skip version
churn.

Treat PostgreSQL mailing lists as the source of truth for project discussion history, not CommitFest metadata.
If the SQL standard, other systems, or academic literature are also needed, use `pg-design-research` separately rather than mixing those sources into this layer.

## Weighing precedent

- A committed outcome outweighs a committer's stated position, which
  outweighs reviewer consensus, which outweighs a single opinion.
- When positions changed over time, the latest committed outcome wins;
  record the date. A newer stated position that departs from it is a
  strong signal, not a settled rule: follow it only where the thread
  shows agreement, and otherwise raise it as an open question.
- Direct precedent (same code) outweighs analogy (same decision
  elsewhere), which outweighs a general stance; say which one applies.
- Check whether the precedent still applies: the code, the constraints,
  or the person's view may have moved on.
- A rejection with reasons is as informative as an acceptance.
- Split or absent precedent is a result. Say so, and put the choice to
  the list as an explicit question rather than settling it silently.

## Output unit

In rationale mode, return **one to three cards** unless the task
explicitly requires a larger pack. In precedent mode, return one card per
alternative that has precedent, then a decision summary.

Each card must include:

- `source`: commit, doc, or thread
- `provenance`: direct (same code), analogous (same decision elsewhere), or general stance
- `proposal`: what was being proposed or changed
- `who`: committer or reviewer, and their position with its reasoning
- `main objections`: the strongest pushback or risk raised
- `decision or status`: what happened, and when
- `implication`: what the current task should preserve, avoid, or justify
- `uncertainty`: what remains unclear, including whether it still applies

Precedent-mode decision summary:

- the choice the precedent favors, and why,
- any deviation, with the reason that must appear in the posting email,
- open questions to ask on the list when precedent is split or absent.

## Rules

- Prefer direct provenance over analogy.
- Never invent historical rationale or attribute a position without a
  source.
- If no strong rationale or precedent is found, say so plainly.
- Paraphrase; do not paste large raw thread excerpts.
- Expand rationale mode to a larger pack only for `Ready`, `Perf`, or explicitly requested deep review.

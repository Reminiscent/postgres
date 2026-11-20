---
name: pg-task-intake
description: Use this skill at the start of every non-trivial PostgreSQL task to classify the work, choose a target readiness level when applicable, identify the blast radius, and decide what to read first.
---

# pg-task-intake

## Purpose

Make the task legible before editing.

This skill is the authoritative routing surface for exact task classification, `Investigation Kind` selection, reclassification, and `read-first` planning. Higher-level docs should explain the model, not duplicate these rules.

This skill answers five questions:

1. What kind of task is this?
2. Is it worth doing now, or is it already done, in progress, or rejected?
3. How far should this iteration go when readiness applies?
4. Which invariants, risks, and proof obligations are in play?
5. What is the minimum set of materials to read first, and what will the community eventually see?

## Task Type

Choose one:

- `change`: modify code, tests, docs, or behavior, including the final patch for a bugfix once diagnosis is strong enough
- `investigate`: establish understanding before deciding whether behavior is wrong, what should change, or which rationale matters
- `review`: inspect an existing patch or diff
- `benchmark`: measure or compare performance

## Investigation Kind

Choose one whenever `task type` is `investigate`:

- `explain-current`: explain the current behavior, code path, or invariants without assuming a defect
- `bug-triage`: reproduce or characterize a likely defect, establish root cause, and decide what fix should be made before the final patch is written
- `design-explore`: compare alternatives and tradeoffs before committing to a change
- `rationale-trace`: recover the historical design rationale that changes the current decision

### Kind selection hints

- Choose `explain-current` when the user mainly wants understanding and has not shown concrete failure evidence.
- Choose `bug-triage` only when the next useful step is reproduction, failure proof, or root-cause isolation.
- Choose `design-explore` when the task is to decide what should change rather than to explain what already happens. Start it from community precedent (`pg-rationale-trace`, precedent mode).
- Choose `rationale-trace` when whether the current behavior is historically intentional materially affects the answer.
- If uncertain, default to `explain-current` until direct defect evidence appears.
- Reclassify when evidence changes the task. Investigation kinds are routing tools, not commitments to stay on one path forever.

## Readiness and learning

Choose one target readiness for `change`, `investigate`, or `benchmark`:
`RFC`, `WIP`, `Answered`, `Ready`, or `Perf`. Definitions and gates live
in `pg-ready-check`.

Infer the optional learning objective with `pg-learning-pack`. Default to
`fix-only`.

## Sibling sweep

Part of every `change` and `bug-triage` task once the root cause is
known; not an optional learning step (principles P6, reference §4.8).
Committers ask where else the same mistake was made, and a fix that
leaves known siblings broken is incomplete.

Search for other places that make the same assumption: the same helper
or idiom, callers of the changed function, the parallel semantic and
optimized paths, and sibling node types, data types, or access methods.
The sweep matters most when:

- semantic and optimized paths diverge,
- `NULL`, strictness, negator, or similar semantic corner cases are part
  of the root cause,
- a fast path or shortcut depends on a hidden contract,
- the bug sits on a planner / executor / access-method boundary where
  equivalent semantics are easy to assume but easy to lose.

Find every instance the search can enumerate. When the class cannot be
enumerated, inspect at least the three to five nearest paths. Classify
each one as:

- `same defect`: fix it in the same patch, or report it if it needs a
  separate fix,
- `explicitly guarded`: name the guard,
- `different invariant`: the assumption does not apply here,
- `unknown`: needs separate evidence; say what would settle it.

Record the result, including "looked, found none", in the task note; it
goes in the posting email.

## Inputs

- user request
- target files or subsystem
- whether a patch already exists
- whether behavior changes
- whether performance evidence is requested
- whether the design is novel, likely contentious, or externally benchmarked
- whether the user is asking for explanation, essence, adjacent risks, prevention, or teaching material

## Procedure

1. Classify the task into `change`, `investigate`, `review`, or `benchmark`.
2. If the task is `investigate`, choose the required `investigation kind`. Default to `explain-current` when the user is asking for understanding.
3. Infer the optional `learning objective` with `pg-learning-pack`. Default to `fix-only`.
4. Choose the target readiness level for `change`, `investigate`, or `benchmark`. Omit it for pure `review` unless the review is explicitly assessing readiness.
5. Decide whether the task is trivial or non-trivial, using the threshold in `AGENTS.md`. If unclear, treat it as non-trivial.
6. List the likely blast radius.
7. List the affected invariants.
8. Draft the initial `Evidence Plan`: validation lanes and non-command evidence sources required before substantive work.
9. Build a `read-first` set with at most six items.
10. Decide which follow-up skills are required.
11. For non-trivial `change`, `bug-triage`, and `design-explore` work, run the prior-work and timing check below, then the value checklist in `pg_engineering_principles.md` §3 (harm class, evidence, who pays, cheapest remedy, irreversibility) before committing to an approach. Work that is already done or in progress should end in joining, reviewing, or stopping; low value with high irreversibility should end in a smaller remedy or a recommendation not to proceed.
12. For non-trivial `change` work, produce an RFC skeleton before any code edits.
13. For non-trivial `investigate` work, produce an investigation note before any edits or temporary probes.
14. For `change` and `bug-triage`, plan the sibling sweep above for when the root cause is known. When the learning objective is above `fix-only`, route to `pg-learning-pack`.
15. Route to `pg-debug-bugfix` for `investigate` tasks of kind `bug-triage`.
16. Route to `pg-rationale-trace` when the `investigation kind` is `rationale-trace` or current behavior may be historically intentional (rationale mode), and whenever a design or fix decision has more than one viable approach (precedent mode, before choosing).
17. Route to `pg-design-research` when, after community precedent, the SQL standard, external system precedent, or literature could still materially change the design.
18. Route to `pg-ready-check` for any readiness-gated handoff, including pure `investigate` closeout at `Answered`.
19. Route to `pg-review-committer` for pure `review` tasks and for any reviewable patch handoff at `Ready` or `Perf`.
20. Decide the community-facing deliverable using the table in `pg-submit`: bug-thread reply, bug report, `pgsql-hackers` RFC, patch, review reply, performance report, or none when the work is for local understanding only. Route to `pg-submit` when anything will be posted.

## Prior-work and timing check

A targeted search, not a history sweep. It is the cheapest value filter:
work that is already fixed, already being discussed, or previously
rejected wastes everyone's attention.

1. Fixed upstream since our base:
   `git fetch upstream && git log --oneline <base>..upstream/master -- <files>`.
2. Already reported, discussed, or patched: search the `pgsql-bugs` and
   `pgsql-hackers` archives (`https://www.postgresql.org/search/?m=1`) and
   the commitfest (`https://commitfest.postgresql.org/`) for the symptom,
   function, error message, or feature name.
3. Previously rejected: the same searches, plus the wiki Todo list and its
   "Not Worth Doing" page for behavior or feature changes.
4. Who would commit it:
   `git log --since='2 years ago' --format='%cn' -- <dir> | sort | uniq -c | sort -rn | head`,
   plus the committers on related threads.
5. Release-cycle position: `git ls-remote --tags upstream 'REL_*'` and the
   current commitfest state; see reference §8.2.

Record the result as one of: proceed; join or review the existing thread
or patch; stop because it is fixed or rejected. Include links, commit
hashes, and the likely committers in the task note.

## Read-first rules

Read in the authority order of `AGENTS.md` rule 1.
Do not start with broad history sweeps. A targeted precedent search at a decision point is not a sweep; `AGENTS.md` rule 11 requires it.
Do not read more material than needed to state the problem and the likely invariants.

## Output format

Return:

### 1. Intake summary
- task type:
- investigation kind:
- learning objective:
- target readiness, when applicable:
- evidence plan:
- trivial or non-trivial:
- prior work and timing, when step 11 applies: proceed / join existing work / stop, with links and likely committers
- community deliverable: shape and destination, or none

### 2. Task framing
- current behavior or question:
- target outcome:

### 3. Blast radius
- subsystem:
- likely touched areas:
- affected invariants:

### 4. Read first
- item 1:
- item 2:
- item 3:

### 5. Follow-up work
- required skills:
- evidence sources and validation lanes to consider:
- whether an RFC or investigation note is required:
- whether a `learning.md` closeout is required (`pg-learning-pack`):
- whether design research is required:

### 6. RFC skeleton
For non-trivial `change` work, provide:
- problem
- value assessment (`pg_engineering_principles.md` §3)
- current behavior
- proposed behavior
- alternatives and the community precedent for each (`pg-rationale-trace`, precedent mode)
- compatibility considerations
- branch scope (HEAD only, or back-patch range and why)
- risk areas
- initial test plan
- sibling sweep plan
- likely reviewer objections

### 7. Investigation note
For non-trivial `investigate` work, provide:
- investigation kind
- question to answer
- current evidence or source set
- likely invariants or design constraints
- current explanation or hypothesis
- stopping condition for this investigation

For `bug-triage`, also provide:
- symptom
- expected behavior
- current proof or reproduction target
- root-cause hypothesis
- recommended fix direction
- sibling sweep result, once the root cause is known
- what would be required before implementation should start

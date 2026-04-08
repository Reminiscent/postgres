---
name: pg-task-intake
description: Use this skill at the start of every non-trivial PostgreSQL task to classify the work, choose a target readiness level when applicable, identify the blast radius, and decide what to read first.
---

# pg-task-intake

## Purpose

Make the task legible before editing.

This skill is the authoritative routing surface for exact task classification, `Investigation Kind` selection, reclassification, and `read-first` planning. Higher-level docs should explain the model, not duplicate these rules.

This skill answers four questions:

1. What kind of task is this?
2. How far should this iteration go when readiness applies?
3. Which invariants, risks, and proof obligations are in play?
4. What is the minimum set of materials to read first?

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
- Choose `design-explore` when the task is to decide what should change rather than to explain what already happens.
- Choose `rationale-trace` when whether the current behavior is historically intentional materially affects the answer.
- If uncertain, default to `explain-current` until direct defect evidence appears.
- Reclassify when evidence changes the task. Investigation kinds are routing tools, not commitments to stay on one path forever.

## Readiness Levels

Choose one target level when the task is `change`, `investigate`, or `benchmark`:

- `RFC`: design only, including a proposed change that is not yet a reviewable patch; no code required
- `WIP`: partial implementation, experiment, or investigation; nearest evidence only
- `Answered`: investigation question is resolved with explicit evidence and no reviewable patch is required; final gate is `pg-ready-check` plus investigation closeout
- `Ready`: correctness-oriented reviewable patch with matching validation and explicit evidence
- `Perf`: `Ready` plus release-build performance evidence

## Learning Objective

Choose one optional learning target when the user wants more than "solve the immediate task":

- `fix-only`: stop at the smallest correct answer, patch, or review conclusion
- `root-cause`: explain the broken assumption, why the bug or behavior occurs, and why the chosen answer works
- `deep-dive`: add execution context, call chain, lifecycle placement, or architectural ownership
- `pattern-audit`: add sibling-path or adjacent-risk analysis after the root cause is clear
- `teaching-pack`: assemble the fullest reusable study packet from the task

These levels are cumulative by intent. Higher levels include the lower ones.
Default to `fix-only` unless the user clearly asks for deeper understanding or the output is explicitly meant to teach.

### Learning pack mapping

- `Context Pack`: required for `deep-dive` and above
- `Root-Cause Pack`: required for `root-cause` and above
- `Pattern Audit Pack`: required for `pattern-audit` and above
- `Refactor Pack`: optional; use it when `teaching-pack` is requested or when a cleanup / patch-series discussion is a real next step
- `Best Practices Extracted`: required for `root-cause` and above

When a learning objective above `fix-only` applies, use `notes/tasks/<slug>/learning.md` for the closeout rather than overloading `task.md`.

### Learning-objective hints

- Choose at least `root-cause` when the user asks "why", "what is the essence", "why did this happen", or "why does the patch work".
- Choose at least `deep-dive` when the user asks for call-path, execution context, planner / executor / JIT placement, or how the layers fit together.
- Choose at least `pattern-audit` when the user asks whether there are similar places, adjacent risks, what else to inspect, or how to avoid this class of bug next time.
- Choose `teaching-pack` when the user wants reusable study material, a distilled write-up, or a durable handoff artifact that teaches the pattern.

### Sibling-sweep rule

When the root cause is any of the following, perform a focused sibling sweep as part of the `Pattern Audit Pack`:

- semantic and optimized paths diverge,
- `NULL`, strictness, negator, or similar semantic corner cases are part of the root cause,
- a fast path or shortcut depends on a hidden contract,
- the bug sits on a planner / executor / access-method boundary where equivalent semantics are easy to assume but easy to lose.

The sweep should inspect three to five nearby paths and classify each one as:

- `explicit guard`
- `explicit non-strict support`
- `contract assumption`
- `unknown`

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
3. Infer the optional `learning objective`. Default to `fix-only`, but raise it when the user explicitly asks for root cause, deeper context, sibling-risk analysis, prevention guidance, or reusable study material.
4. Choose the target readiness level for `change`, `investigate`, or `benchmark`. Omit it for pure `review` unless the review is explicitly assessing readiness.
5. Decide whether the task is trivial or non-trivial.
6. List the likely blast radius.
7. List the affected invariants.
8. Draft the initial `Evidence Plan`: validation lanes and non-command evidence sources required before substantive work.
9. Build a `read-first` set with at most six items.
10. Decide which follow-up skills are required.
11. For non-trivial `change` work, produce an RFC skeleton before any code edits.
12. For non-trivial `investigate` work, produce an investigation note before any edits or temporary probes.
13. When the learning objective is above `fix-only`, plan a `learning.md` closeout and note which packs are required.
14. Route to `pg-debug-bugfix` for `investigate` tasks of kind `bug-triage`.
15. Route to `pg-rationale-trace` when the `investigation kind` is `rationale-trace` or current behavior may be historically intentional.
16. Route to `pg-design-research` when PostgreSQL mailing-list history, external system precedent, or literature could materially improve the design.
17. Route to `pg-ready-check` for any readiness-gated handoff, including pure `investigate` closeout at `Answered`.
18. Route to `pg-review-committer` for pure `review` tasks and for any reviewable patch handoff at `Ready` or `Perf`.

## Read-first rules

Read in this order:

1. nearby code,
2. nearby tests,
3. nearest README or subsystem docs,
4. official PostgreSQL docs, wiki, or process pages,
5. committed patches with `Discussion:` links,
6. active unresolved threads only when the earlier layers are insufficient.

Do not start with broad history sweeps.
Do not read more material than needed to state the problem and the likely invariants.

## Trivial versus non-trivial

Treat the task as **non-trivial** when any of these are true:

- user-visible behavior may change,
- error behavior, compatibility, or upgrade behavior may change,
- more than one invariant or module boundary is involved,
- concurrency, visibility, locking, transactions, WAL, recovery, or catalog state may be involved,
- client behavior or performance may be involved,
- the rationale is unclear,
- more than the nearest evidence lane will likely be needed,
- the result will need durable task notes or a reviewer-facing handoff.

## Output format

Return:

### 1. Intake summary
- task type:
- investigation kind:
- learning objective:
- target readiness, when applicable:
- evidence plan:
- trivial or non-trivial:

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
- whether a `learning.md` closeout is required and which packs it must include:
- whether design research is required:

### 6. RFC skeleton
For non-trivial `change` work, provide:
- problem
- current behavior
- proposed behavior
- compatibility considerations
- risk areas
- initial test plan
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
- what would be required before implementation should start

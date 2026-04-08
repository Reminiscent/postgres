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

## Inputs

- user request
- target files or subsystem
- whether a patch already exists
- whether behavior changes
- whether performance evidence is requested
- whether the design is novel, likely contentious, or externally benchmarked

## Procedure

1. Classify the task into `change`, `investigate`, `review`, or `benchmark`.
2. If the task is `investigate`, choose the required `investigation kind`. Default to `explain-current` when the user is asking for understanding.
3. Choose the target readiness level for `change`, `investigate`, or `benchmark`. Omit it for pure `review` unless the review is explicitly assessing readiness.
4. Decide whether the task is trivial or non-trivial.
5. List the likely blast radius.
6. List the affected invariants.
7. Draft the initial `Evidence Plan`: validation lanes and non-command evidence sources required before substantive work.
8. Build a `read-first` set with at most six items.
9. Decide which follow-up skills are required.
10. For non-trivial `change` work, produce an RFC skeleton before any code edits.
11. For non-trivial `investigate` work, produce an investigation note before any edits or temporary probes.
12. Route to `pg-debug-bugfix` for `investigate` tasks of kind `bug-triage`.
13. Route to `pg-rationale-trace` when the `investigation kind` is `rationale-trace` or current behavior may be historically intentional.
14. Route to `pg-design-research` when PostgreSQL mailing-list history, external system precedent, or literature could materially improve the design.
15. Route to `pg-ready-check` for any readiness-gated handoff, including pure `investigate` closeout at `Answered`.
16. Route to `pg-review-committer` for pure `review` tasks and for any reviewable patch handoff at `Ready` or `Perf`.

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

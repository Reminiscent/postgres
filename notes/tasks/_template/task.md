# Task: <slug>

## 1. Intake

- Task type: change / investigate / review / benchmark
- Investigation kind (required for investigate; default `explain-current`):
- Target readiness (change / investigate / benchmark; optional for pure review): RFC / WIP / Answered / Ready / Perf
- Evidence plan (declare before substantive work; list required validation lanes and non-command sources):
- Trivial or non-trivial:
- Owner:

## 2. Task framing

Use this section for all task types.

### Current behavior or question

Describe the current behavior precisely, or state the investigation question or review target being examined.

### Target outcome

For `change`: describe the intended behavior precisely.
For `investigate`: describe the question to answer or claim to confirm or refute.
For `review`: describe what the review must establish.
For `benchmark`: describe the metric, workload, or comparison target.

### Why it matters

Why should PostgreSQL care?

## 2a. Investigation note

Use this section for `investigate` tasks. Fill the generic fields first, then the `bug-triage` fields when that kind applies.

- question to answer:
- current evidence or source set:
- likely invariants or design constraints:
- current explanation or hypothesis:
- stopping condition:

### `bug-triage` details

- symptom:
- expected behavior:
- current proof or reproduction target:
- root-cause hypothesis:
- backpatch or release-risk note:
- recommended fix direction:
- what is still needed before implementation:

## 3. Blast radius

- subsystem:
- likely touched files or evidence sources:
- user-visible impact:

## 4. Affected invariants

- 
- 
- 

## 5. Read-first set

- 
- 
- 

## 6. Rationale summary

- direct provenance:
- related threads or docs:
- what is still unclear:

## 7. External precedents and research

Use this section when `pg-design-research` is required.

- PostgreSQL mailing lists checked:
- systems checked:
- papers checked:
- what changes the design:

## 8. Alternatives considered

Use this section for `change` tasks and `design-explore` investigations. Skip it for `explain-current` or `rationale-trace` unless competing explanations or interpretations materially affect the answer.

### Alternative A
- why considered:
- why rejected:

### Alternative B
- why considered:
- why rejected:

## 9. Change plan

Use this section for `change` tasks, including bugfix work that has moved from investigation into implementation.

- smallest coherent patch:
- deferred work by design:

## 9a. Investigation closeout

Use this section for pure `investigate` tasks that may end without a code change.
Target readiness is usually `Answered`. Pair this closeout with `pg-ready-check` when `Answered` is the final gate.

- question answered:
- evidence that resolved it:
- stopping condition reached:
- recommended next step and why:
- remaining uncertainty:

## 10. Evidence / validation plan

Use this matrix for both executable validation lanes and non-command evidence sources.
For pure `investigate` tasks that target `Answered`, fill in the sources that actually resolved the question even when no executable lane ran.

| Kind | Lane or source | Why | Command or reference | Required? | Result |
|---|---|---|---|---|---|
| validation |   |   |   |   |   |
| source |   |   |   |   |   |

## 11. Likely reviewer objections or follow-up questions

Use this section for `change` and `review` work. For pure `investigate` tasks, record likely follow-up questions instead.

- 
- 

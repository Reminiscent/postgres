---
name: pg-ready-check
description: Use this skill for readiness-gated work to decide which evidence / validation lanes are required for the target readiness level and to judge whether the evidence is sufficient.
---

# pg-ready-check

## Purpose

Turn blast radius into evidence.

This skill does two things:

1. chooses the smallest sufficient evidence / validation lanes and sources,
2. decides whether the task has reached `RFC`, `WIP`, `Answered`, `Ready`, or `Perf`.

## Inputs

- task type
- investigation kind when relevant
- target readiness
- changed files
- affected invariants
- whether user-visible behavior changed
- whether performance claims are involved
- available commands from `LOCAL_WORKFLOWS.md`
- commands actually run, evidence consulted, and their results

## Readiness definitions

### RFC

- design only, including a proposed change that is not yet a reviewable patch
- no implementation required
- must state the task framing, risks, and planned evidence / validation lanes

### WIP

- partial implementation, experiment, or investigation proves direction
- nearest evidence is usually enough; executable lanes are only required when they are needed to support the current claim
- must not be presented as settled or review-ready

### Answered

- pure investigation is resolved with explicit evidence
- no reviewable patch is required
- must state the answer, stopping condition, consulted evidence, and recommended next step
- executable validation lanes are optional unless they are needed to resolve the question
- `pg-review-committer` is not required for pure `Answered` closeout

### Ready

- correctness-oriented reviewable patch is complete enough for deep human review
- must use the debug/cassert build unless there is a documented reason not to
- must run the smallest evidence / validation lanes that match the blast radius
- must record exact commands and concise results

### Perf

- all `Ready` requirements
- performance evidence from an optimized build
- exact workload command, measurement method, and caveats recorded

## Evidence / validation lane mapping

Use these lanes when relevant:

- `compile`: code generation or compilation proof
- `nearest tests`: closest existing targeted tests
- `regression`: parser, planner, executor, SQL semantics, or user-visible behavior
- `TAP`: `src/bin/*`, TAP-heavy code, or Perl test infrastructure
- `isolation`: concurrency, locking, visibility, or transaction semantics
- `docs`: SGML or documentation-sensitive changes
- `release benchmark`: optimized-build performance measurements
- `committer review`: final self-review with `pg-review-committer`

When this skill is used for `investigate` tasks, the evidence should also show:

- the investigation question and stopping condition,
- direct evidence that the selected investigation kind actually resolved the question,
- a clear recommended next step, such as `change`, `benchmark`, follow-up investigation, RFC work, `review`, or closure.

For `investigate / bug-triage`, the evidence should also show:

- a reproduction, characterization, or failure proof before the fix when practical,
- the smallest fix that addresses that proof,
- regression evidence or an explicit explanation of why regression proof is not yet available,
- backpatch or release-risk notes when the defect affects supported branches.

For pure `investigate` tasks that close at `Answered`, this skill should be paired with an investigation closeout that records the answer, evidence consulted, remaining uncertainty, and recommended next step.

## Escalation rules

Widen the test scope when any of these are true:

- module boundaries were crossed,
- behavior changed for users or clients,
- output files changed,
- concurrency or recovery semantics changed,
- portability risk is unclear,
- the current evidence does not prove the invariant.

## Output format

Return:

### 1. Readiness verdict
- target readiness:
- current readiness:
- rationale for the verdict:

### 2. Evidence / validation matrix
For each planned or executed lane or evidence source, list:
- lane or source:
- why it is included:
- exact command or evidence reference:
- expected signal:
- required or optional:
- what would force widening:
- result:

### 3. Evidence gaps
- missing commands:
- missing lanes:
- remaining risks:

### 4. Next step
- what is required to reach the target readiness:
- whether `pg-review-committer` should run now:

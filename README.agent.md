# PostgreSQL Agent Workflow

This repository uses a minimal PostgreSQL-specific control plane for coding agents.
The goal is not more instructions. The goal is to make four decisions explicit:

- what kind of work this is,
- how far the current iteration should go,
- what evidence is required,
- where durable task state belongs.

## Core model

For every non-trivial `change`, `investigate`, or `benchmark` task, declare:

- **Task Type**: `change`, `investigate`, `review`, or `benchmark`
- **Target Readiness**: `RFC`, `WIP`, `Answered`, `Ready`, or `Perf`
- **Evidence Plan**: the validation lanes and non-command evidence sources needed to prove the task. Validation lanes come from `compile`, `nearest tests`, `regression`, `TAP`, `isolation`, `docs`, `release benchmark`, `committer review`. Evidence sources may include nearby code, nearby tests, official docs, committed patches with `Discussion:` links, or active unresolved threads.

For `investigate`, also declare:

- **Investigation Kind**: `explain-current`, `bug-triage`, `design-explore`, or `rationale-trace`

When the user wants more than "fix it correctly", the intake may also declare an optional:

- **Learning Objective**: `fix-only`, `root-cause`, `deep-dive`, `pattern-audit`, or `teaching-pack`

These dimensions are separate on purpose:

- `Task Type` chooses the proof shape and stopping condition.
- `Target Readiness` chooses how complete the current iteration must be.
- `Evidence Plan` chooses what proof is required.
- `Investigation Kind` chooses which investigation path to follow.
- `Learning Objective` chooses whether the task should stop at correctness or also produce reusable understanding.

For pure `review`, declare `Task Type` and `Evidence Plan`. Omit `Target Readiness` unless the task is explicitly assessing readiness.

### Readiness intent

- `RFC`: design-only direction or proposed change without a reviewable patch
- `WIP`: partial implementation, experiment, or investigation
- `Answered`: investigation resolved with explicit evidence and a recommended next step, but no reviewable patch
- `Ready`: reviewable patch is ready for deep review
- `Perf`: `Ready` plus release-build performance evidence

`Answered` is a real final gate for pure investigation closeout, not a note-taking label.

## Investigation model

`investigate` exists because many tasks need understanding before deciding whether behavior is wrong.

- `explain-current`: explain the current behavior and invariants without assuming a defect
- `bug-triage`: prove or characterize a likely defect and justify a fix direction
- `design-explore`: compare alternatives before choosing a proposal
- `rationale-trace`: recover the design history that materially changes the decision

Typical examples:

| Request shape | Typical kind |
|---|---|
| explain the current code path or behavior | `explain-current` |
| determine whether an observed behavior is a bug | `bug-triage` |
| decide between competing semantic or architectural options | `design-explore` |
| determine whether current behavior is historically intentional | `rationale-trace` |

Use `.agents/skills/pg-task-intake/SKILL.md` for the exact routing rules, reclassification policy, and `read-first` planning.

## Control-plane ownership

Keep the workflow simple by giving each layer one job:

- `AGENTS.md`: durable hot-path rules and required declarations
- `README.agent.md`: workflow model, boundaries, and file responsibilities
- `LOCAL_WORKFLOWS.md`: exact commands, build profiles, and validation commands
- `.agents/skills/pg-task-intake/SKILL.md`: exact routing and `read-first` decisions
- `.agents/skills/pg-ready-check/SKILL.md`: readiness and evidence sufficiency
- `.agents/skills/pg-review-committer/SKILL.md`: final reviewable-patch gate
- `notes/tasks/_template/`: durable task-memory structure

If a rule is exact and operational, it belongs in a skill or in `LOCAL_WORKFLOWS.md`, not here.

## Workflow summary

The hot path is intentionally short:

1. Start with `pg-task-intake` to classify the task, choose readiness and investigation kind when they apply, and build the `read-first` set.
2. For non-trivial `change` or `investigate`, write the RFC or investigation note in `notes/tasks/<slug>/task.md` before substantive work.
3. Use only the supporting skills the task actually needs.
4. Close at the matching gate: use `pg-ready-check` for readiness-gated handoff, add `pg-review-committer` for reviewable patch work or pure `review`, and record an investigation closeout when a pure `investigate` task ends at `Answered`.

Task type sets the goal:

- `change`: implement the smallest coherent patch
- `investigate`: answer the question or localize the defect
- `review`: inspect an existing patch or diff
- `benchmark`: measure performance with an explicit workload

Readiness sets the handoff gate:

| Readiness | Meaning | Gate |
|---|---|---|
| `RFC` | design-only direction or proposal without a reviewable patch | task framing plus planned evidence |
| `WIP` | partial implementation, experiment, or investigation | nearest evidence only; not handoff-ready |
| `Answered` | pure investigation is resolved | `pg-ready-check` plus investigation closeout |
| `Ready` | reviewable patch | `pg-ready-check` plus `pg-review-committer` |
| `Perf` | performance claim or benchmark result | `pg-ready-check` plus `pg-review-committer` on release-build evidence |

Pure `review` work ends in a `pg-review-committer` verdict rather than a readiness label unless the review is explicitly assessing readiness.

Use supporting skills only when the task needs them:

- `pg-debug-bugfix` for `investigate / bug-triage`
- `pg-rationale-trace` when design history materially affects the answer
- `pg-design-research` when external precedent or literature materially affects the design

## Experimental learning extension

This repository treats learning-oriented output as an optional layer on top of correctness work, not as a replacement for it.

Use `Learning Objective` only when the user wants more than the minimal answer:

- `fix-only`: stop at the smallest correct answer or patch
- `root-cause`: explain the broken assumption and why the chosen fix works
- `deep-dive`: add execution context, call-path, or architectural placement
- `pattern-audit`: add sibling-path or adjacent-risk analysis
- `teaching-pack`: assemble a reusable study packet from the task

These levels are cumulative by intent. Higher levels include the lower ones.

The durable output for this layer lives in `notes/tasks/<slug>/learning.md` and is intentionally experimental.
Do not promote it into `AGENTS.md` until repeated tasks show that the same learning-oriented friction recurs.

### When to use it

Use this layer when the real goal is not only to finish the immediate task but also to make future reasoning cheaper.

Typical cases:

- the bug is fixed, but the user also wants the root cause made explicit
- the user wants the call path, execution context, or layer placement explained
- the bug likely belongs to a reusable pattern rather than a one-off mistake
- the user asks whether similar paths or adjacent risks should be checked
- the result should become durable study material or a reusable handoff

Keep using `fix-only` when the user only wants the smallest correct patch, answer, or review conclusion.

### How to trigger it

There are two intended trigger paths:

1. explicit declaration in the intake or task note
2. clear user intent in the request

Explicit declaration is the cleanest form, for example:

- `Learning Objective: root-cause`
- `Learning Objective: deep-dive`
- `Learning Objective: pattern-audit`
- `Learning Objective: teaching-pack`

The workflow may also infer the intent when the user asks questions such as:

- why did this happen
- what is the essence
- walk me through the code path
- are there similar places
- what else should we audit
- how do we avoid this next time
- turn this into learning material

When the inferred or declared level is above `fix-only`, capture the extra output in `notes/tasks/<slug>/learning.md`.

### What each level should produce

- `fix-only`: no learning closeout required
- `root-cause`: root cause plus best-practice extraction
- `deep-dive`: root cause plus execution context and call-path explanation
- `pattern-audit`: deep dive plus sibling-path or adjacent-risk sweep
- `teaching-pack`: the fullest reusable packet, with refactor discussion when it is a real next step

## Non-trivial threshold

Non-trivial is about risk and evidence, not line count.
Use `AGENTS.md` and `.agents/skills/pg-task-intake/SKILL.md` as the authority for the exact threshold and the resulting hot-path obligations.
If the threshold is unclear, err on the side of treating the task as non-trivial.

## Durable task memory

Use `notes/tasks/<slug>/` for non-trivial work:

- `task.md`: durable task framing and proof plan for non-trivial work, including RFCs, investigations, reviews, and benchmarks
- `log.md`: commands, consulted sources, findings, and status
- `review.md`: reviewer packet for reviewable patch work
- `learning.md`: optional learning-oriented closeout when the intake or user intent calls for deeper explanation, pattern audit, or teaching material

## File map

Hot path:

- `AGENTS.md`
- `LOCAL_WORKFLOWS.md`
- `code_review.md`
- `.agents/skills/pg-*`
- `notes/tasks/_template/`

Cold path:

- `.agents/skills/agent-workflow-router/`
- `agent/README.md`
- `agent/EVOLUTION_RULES.md`

## Evidence policy

PostgreSQL code, tests, docs, and mailing-list history are the authoritative sources for acceptance.
External systems and papers are supporting evidence only.

Use `pg-design-research` when precedent or literature could materially change the design.
Do not require it for mechanical fixes or local refactors.

## Runtime convention

Use worktree-local install, data, log, and port settings.
Keep one install tree, one data directory, one log directory, and one explicit port per active worktree.

Do not treat the install prefix as the source tree.
Do not share a data directory or port across active worktrees.

The exact shell setup and commands live in `LOCAL_WORKFLOWS.md`.

## Design rules

- keep `AGENTS.md` short and durable
- keep exact commands in `LOCAL_WORKFLOWS.md`
- keep exact routing rules in the relevant skill
- keep review criteria in `code_review.md`
- keep skills narrow and single-purpose
- expand the workflow only when repeated failures justify it

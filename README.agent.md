# PostgreSQL Agent Workflow

This repository uses a minimal PostgreSQL-specific control plane for coding agents.
The goal is not more instructions. The goal is to make five decisions explicit:

- whether the work is worth the community's attention now,
- what kind of work this is,
- how far the current iteration should go,
- what evidence is required,
- what the community will see, and where durable task state belongs.

## Core model

`AGENTS.md` defines the required declarations: `Task Type`,
`Target Readiness`, `Evidence Plan`, and, for `investigate`,
`Investigation Kind`. `pg-task-intake` holds the exact routing rules.
An optional `Learning Objective` is defined in `pg-learning-pack`.

These dimensions are separate on purpose:

- `Task Type` chooses the proof shape and stopping condition.
- `Target Readiness` chooses how complete the current iteration must be.
- `Evidence Plan` chooses what proof is required.
- `Investigation Kind` chooses which investigation path to follow.
- `Learning Objective` chooses whether the task should stop at correctness or also produce reusable understanding.

`investigate` exists because many tasks need understanding before
deciding whether behavior is wrong. `Answered` is a real final gate for
pure investigation closeout, not a note-taking label.

## Workflow summary

The hot path is intentionally short:

1. Start with `pg-task-intake`: classify the task, run the prior-work and timing check, choose readiness and investigation kind when they apply, decide the community deliverable, and build the `read-first` set.
2. For non-trivial `change` or `investigate`, write the RFC or investigation note in `notes/tasks/<slug>/task.md` before substantive work.
3. Use only the supporting skills the task actually needs.
4. Close at the matching gate: use `pg-ready-check` for readiness-gated handoff, add `pg-review-committer` for reviewable patch work or pure `review`, and record an investigation closeout when a pure `investigate` task ends at `Answered`.
5. When the result will be posted, use `pg-submit` to produce the email draft and patch files, track the thread, and run the post-commit retro.

Readiness sets the handoff gate; `pg-ready-check` holds the full definitions:

| Readiness | Meaning | Gate |
|---|---|---|
| `RFC` | design-only direction or proposal without a reviewable patch | task framing plus planned evidence; for behavior changes, an RFC email for agreement in principle |
| `WIP` | partial implementation, experiment, or investigation | nearest evidence only; not handoff-ready |
| `Answered` | pure investigation is resolved | `pg-ready-check` plus investigation closeout |
| `Ready` | reviewable patch | `pg-ready-check` plus `pg-review-committer` |
| `Perf` | performance claim or benchmark result | `pg-ready-check` plus `pg-review-committer` on release-build evidence |

Pure `review` work ends in a `pg-review-committer` verdict rather than a readiness label unless the review is explicitly assessing readiness.

Use supporting skills only when the task needs them:

- `pg-debug-bugfix` for `investigate / bug-triage`
- `pg-rationale-trace` when design history materially affects the answer, and at every decision with more than one viable approach, to check community precedent before choosing
- `pg-design-research` when the SQL standard, external precedent, or literature materially affects the design
- `pg-learning-pack` when the user wants root cause, deeper context, adjacent-risk write-ups, or teaching material beyond the fix
- `pg-patch-review-deep-dive` when a PostgreSQL patch review uses
  `discussion.txt` plus the current branch's changes relative to the `agent`
  base branch, and needs first-principles design review, regression validation,
  implementation walkthrough, and Chinese teaching notes

## Control-plane ownership

Keep the workflow simple by giving each layer one job:

- `AGENTS.md`: durable hot-path rules, required declarations, and the non-trivial threshold
- `README.agent.md`: workflow model, boundaries, and file responsibilities
- `LOCAL_WORKFLOWS.md`: exact commands, build profiles, validation, and posting commands
- `pg_engineering_principles.md`: core decision principles, committer questions, and stage checklists, read on every non-trivial task
- `pg_engineering_principles_reference.md`: detailed principles by stage, read by section on demand
- `code_review.md`: review rubric used by `pg-review-committer`
- `.agents/skills/pg-task-intake/SKILL.md`: exact routing, prior-work check, sibling sweep, and `read-first` decisions
- `.agents/skills/pg-ready-check/SKILL.md`: readiness definitions and evidence sufficiency
- `.agents/skills/pg-review-committer/SKILL.md`: final reviewable-patch gate
- `.agents/skills/pg-submit/SKILL.md`: what the community sees (email and patch files), follow-up, and post-commit retro
- `.agents/skills/pg-learning-pack/SKILL.md`: optional learning levels and the `learning.md` closeout
- `notes/tasks/_template/`: durable task-memory structure

If a rule is exact and operational, it belongs in a skill or in `LOCAL_WORKFLOWS.md`, not here.
Each rule has one home; other files point to it instead of restating it.

## Durable task memory

Use `notes/tasks/<slug>/` for non-trivial work:

- `task.md`: task framing, value and prior-work check, investigation note or plan, evidence, and closeout
- `log.md`: commands, consulted sources, findings, and community status
- `review.md`: reviewer packet for reviewable patch work, plus the community outcome and post-commit retro
- `email.txt`: the posting draft, when something will be posted
- `learning.md`: optional learning closeout (`pg-learning-pack`)

## File map

Hot path:

- `AGENTS.md`
- `LOCAL_WORKFLOWS.md`
- `pg_engineering_principles.md` (core; `pg_engineering_principles_reference.md` by section on demand)
- `code_review.md`
- `.agents/skills/pg-*`
- `notes/tasks/_template/`

Cold path:

- `.agents/skills/agent-workflow-router/`
- `agent/README.md`
- `agent/EVOLUTION_RULES.md`

## Evidence policy

PostgreSQL code, tests, docs, and mailing-list history are the authoritative sources for acceptance.
The SQL standard text is the strongest external source for SQL semantics.
Other systems and papers are supporting evidence only.

Use `pg-design-research` when the standard, external precedent, or literature could materially change the design.
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
- keep decision principles in `pg_engineering_principles.md` and `pg_engineering_principles_reference.md`; other files point to them instead of restating them
- keep skills narrow and single-purpose
- expand the workflow only when repeated failures justify it

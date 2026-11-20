# Task: agent-learning-workflow-experiment

## 1. Intake

- Task type: change
- Investigation kind (required for investigate; default `explain-current`):
- Learning objective (optional): teaching-pack
- Target readiness (change / investigate / benchmark; optional for pure review): Ready
- Evidence plan (declare before substantive work; list required validation lanes and non-command sources):
  - source: `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/agent-learning-workflow-improvements/proposal-zh.md`
  - source: `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/deep-dive-zh.md`
  - source: `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/strictness-assumptions-audit-zh.md`
  - source: `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/helper-refactor-sketch-zh.md`
  - source: `README.agent.md`, `.agents/skills/pg-task-intake/SKILL.md`, `notes/tasks/_template/*.md`, `agent/EVOLUTION_RULES.md`
  - validation: changed operator-facing docs and templates stay non-duplicative and internally consistent
- Trivial or non-trivial: non-trivial
- Owner: Codex

## 2. Task framing

Use this section for all task types.

### Current behavior or question

The workflow strongly standardizes correctness, readiness, and reviewability, but it does not yet make learning-oriented outputs first-class. Deep explanations, root-cause generalization, sibling-path audits, and practice extraction currently emerge only after repeated follow-up prompts.

### Target outcome

Add a minimal experimental workflow that makes learning depth explicit without bloating the hot path:

- add an optional `Learning Objective` to intake,
- add a durable learning-oriented closeout template,
- add routing rules that auto-trigger the relevant learning packs when user intent clearly asks for essence, why, adjacent risks, or prevention.

Do not harden these rules into `AGENTS.md` yet.

### Why it matters

The goal is to preserve the current workflow's strength at "fix it correctly" while making "understand it deeply and generalize the pattern" repeatable, reviewable, and cheap to invoke.

## 3. Blast radius

- subsystem: repo-local workflow docs, routing skill, and task-memory templates
- likely touched files or evidence sources:
  - `README.agent.md`
  - `.agents/skills/pg-task-intake/SKILL.md`
  - `notes/tasks/_template/task.md`
  - `notes/tasks/_template/learning.md`
- user-visible impact:
  - non-trivial tasks may declare an optional learning depth target
  - learning-oriented outputs become durable and predictable
  - the hot path should remain optional for fix-only work

## 4. Affected invariants

- `AGENTS.md` remains short and durable; experiments stay out of the hot-path rule set until repetition justifies promotion.
- Exact routing logic belongs in `pg-task-intake`, not duplicated across multiple files.
- Learning-oriented outputs must remain optional and scoped, so fix-only tasks do not pay unnecessary workflow cost.

## 5. Read-first set

- `README.agent.md`
- `.agents/skills/pg-task-intake/SKILL.md`
- `notes/tasks/_template/task.md`
- `agent/EVOLUTION_RULES.md`
- proposal and hashed-SAOP learning materials from the sibling worktree

## 6. Rationale summary

- direct provenance:
  - proposal in sibling worktree arguing for layered goals: fix, explain root cause, generalize pattern, audit adjacent risks
  - sample learning artifacts showing stable output shapes with high value
- related threads or docs:
  - current repo-local workflow docs and templates
  - `agent/EVOLUTION_RULES.md` constraints on when to grow the workflow
- what is still unclear:
  - whether `Refactor Pack` should be included in the first experimental template or deferred

## 7. External precedents and research

Use this section when `pg-design-research` is required.

- PostgreSQL mailing lists checked:
- systems checked:
- papers checked:
- what changes the design:

## 8. Alternatives considered

Use this section for `change` tasks and `design-explore` investigations. Skip it for `explain-current` or `rationale-trace` unless competing explanations or interpretations materially affect the answer.

### Alternative A
- why considered: promote the proposal directly into `AGENTS.md`
- why rejected: violates the repo's evolution rule because the pattern is valuable but not yet demonstrated as a repeated hot-path failure

### Alternative B
- why considered: add a brand-new repo-local skill now
- why rejected: the workflow contract is not yet proven stable enough, and the minimal experiment can live in existing routing plus templates first

## 9. Change plan

Use this section for `change` tasks, including bugfix work that has moved from investigation into implementation.

- smallest coherent patch:
  - document an optional `Learning Objective` in `README.agent.md`
  - teach `pg-task-intake` how to infer and route it
  - add a dedicated learning closeout template
  - minimally link the task template to the new closeout asset
- deferred work by design:
  - `AGENTS.md` changes
  - a separate repo-local teaching skill
  - mandatory sibling sweeps for all bug classes
  - any automation or evaluation harness for measuring learning-output quality

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
| source | proposal-zh.md | defines the user-requested workflow changes and first-principles goal layering | `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/agent-learning-workflow-improvements/proposal-zh.md` | yes | consulted |
| source | deep-dive-zh.md | sample for context and root-cause output shape | `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/deep-dive-zh.md` | yes | consulted |
| source | strictness-assumptions-audit-zh.md | sample for pattern-audit output shape | `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/strictness-assumptions-audit-zh.md` | yes | consulted |
| source | helper-refactor-sketch-zh.md | sample for refactor-oriented discussion shape | `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/hashed-saop-null-nonstrict-investigation/helper-refactor-sketch-zh.md` | optional | consulted |
| source | existing workflow docs and templates | ensure the experiment fits current control-plane ownership and avoids duplication | `README.agent.md`, `.agents/skills/pg-task-intake/SKILL.md`, `notes/tasks/_template/*.md`, `agent/EVOLUTION_RULES.md` | yes | consulted |
| validation | internal consistency review | confirm the new field, trigger rules, and templates agree on names and scope | `git diff -- README.agent.md .agents/skills/pg-task-intake/SKILL.md notes/tasks/_template/task.md notes/tasks/_template/learning.md notes/tasks/agent-learning-workflow-experiment/*`; `rg -n "Learning Objective|learning objective|learning.md|Pattern Audit Pack|Best Practices Extracted|sibling sweep|sweep" README.agent.md .agents/skills/pg-task-intake/SKILL.md notes/tasks/_template/task.md notes/tasks/_template/learning.md notes/tasks/agent-learning-workflow-experiment/task.md` | yes | completed |
| validation | patch hygiene | ensure the patch is commit-clean under repo workflow expectations | `src/tools/pgindent/pgindent .`; `git diff --check` | yes | completed |

## 11. Likely reviewer objections or follow-up questions

Use this section for `change` and `review` work. For pure `investigate` tasks, record likely follow-up questions instead.

- Does `Learning Objective` make the hot path too heavy for ordinary bugfix work?
- Are the packs stable enough to standardize now, or should some stay example-only?
- Does the patch duplicate rules between `README.agent.md`, `pg-task-intake`, and templates?

## 12. Learning-oriented closeout

Use `learning.md` when the learning objective is above `fix-only`, or when the user explicitly asks for root cause, deeper context, similar paths, prevention guidance, or reusable teaching material.
The closeout should capture the required packs without overloading `task.md`.

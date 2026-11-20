# Review Packet: agent-learning-workflow-experiment

Use this packet for `Ready`, `Perf`, or other reviewable patch work.
Do not use it for pure `investigate` closeout; use the investigation closeout in `task.md` instead.

## Verdict

- Ready for deep review

## Problem

- current behavior:
  - The workflow already proves correctness well, but it does not make learning depth explicit. Root-cause explanation, context, sibling-path audit, and best-practice extraction appear only after repeated follow-up prompts.
- desired behavior:
  - Keep the hot path unchanged for ordinary fix-only work, while adding an optional, experimental layer that can request deeper learning-oriented outputs and give them a durable home.
- why this patch exists:
  - To make "fix it, explain it, generalize it" repeatable without prematurely hardening the pattern into `AGENTS.md` or creating a new skill before recurrence justifies it.

## Scope

- changed files:
  - `README.agent.md`
  - `.agents/skills/pg-task-intake/SKILL.md`
  - `notes/tasks/_template/task.md`
  - `notes/tasks/_template/learning.md`
  - `notes/tasks/agent-learning-workflow-experiment/{task,log,learning}.md`
- what is intentionally out of scope:
  - `AGENTS.md`
  - a new repo-local teaching skill
  - any automatic quality-evaluation harness for learning outputs

## Invariants touched

- `AGENTS.md` remains the durable hot-path rule set and should not absorb experimental workflow ideas prematurely.
- Exact routing logic remains centralized in `.agents/skills/pg-task-intake/SKILL.md`.
- Learning-oriented outputs remain optional and should not burden ordinary fix-only tasks.

## Rationale consulted

- direct provenance:
  - `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/agent-learning-workflow-improvements/proposal-zh.md`
- related threads:
  - none
- PostgreSQL mailing lists:
  - none
- docs or wiki:
  - `README.agent.md`
  - `agent/EVOLUTION_RULES.md`
  - `code_review.md`
- external precedents:
  - none
- research papers:
  - none

## Evidence and validation performed

| Kind | Lane or source | Command or reference | Why | Result |
|---|---|---|---|---|
| source | workflow proposal | `/Users/yanchengpeng/projects/oss/postgres-in_bugfix/notes/tasks/agent-learning-workflow-improvements/proposal-zh.md` | ground the patch in a real repeated user need | consulted |
| source | sample learning artifacts | sibling worktree hashed-SAOP deep-dive, strictness audit, refactor sketch | confirm stable output shapes worth templating | consulted |
| source | workflow ownership rules | `README.agent.md`, `agent/EVOLUTION_RULES.md` | ensure the experiment lands in the right control-plane layer | consulted |
| validation | formatting rule | `src/tools/pgindent/pgindent .` | satisfy the repo's required pre-commit formatting step | passed; no additional formatting changes in the touched workflow files |
| validation | patch hygiene | `git diff --check` | verify whitespace and patch cleanliness before commit | passed |
| validation | patch review | `git diff -- README.agent.md .agents/skills/pg-task-intake/SKILL.md notes/tasks/_template/task.md notes/tasks/_template/learning.md notes/tasks/agent-learning-workflow-experiment/*` | verify smallest coherent scope and internal consistency | passed |
| validation | terminology consistency | `rg -n "Learning Objective|learning objective|learning.md|Pattern Audit Pack|Best Practices Extracted|sibling sweep|sweep" README.agent.md .agents/skills/pg-task-intake/SKILL.md notes/tasks/_template/task.md notes/tasks/_template/learning.md notes/tasks/agent-learning-workflow-experiment/task.md` | verify naming and trigger phrases stay aligned across assets | passed |

## User-visible impact

- SQL semantics:
  - none
- docs:
  - workflow docs now expose an optional `Learning Objective` and a dedicated `learning.md` closeout
- client behavior:
  - none
- compatibility or upgrade:
  - none
- output files or portability:
  - none
- backpatch or branch risk:
  - workflow-only change in this repo; no PostgreSQL backpatch implications

## Remaining risks

- The experiment is designed from a strong sample, but repeated use is still needed before promoting any of it into `AGENTS.md` or a dedicated skill.
- The routing hints are intentionally heuristic; future tasks may show that one or more thresholds need tightening.

## Reviewer focus

- Check that the new learning layer remains clearly optional and does not make fix-only tasks heavier by default.
- Check that `README.agent.md` stays high-level while `pg-task-intake` remains the only place with exact trigger and sibling-sweep rules.

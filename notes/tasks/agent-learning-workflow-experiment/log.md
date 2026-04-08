# Task Log: agent-learning-workflow-experiment

## Session summary

- Current readiness or review verdict: Ready for deep review
- Current question, hypothesis, or review focus:
  - Hypothesis: the workflow should gain an optional learning-depth layer, but the experiment should stay outside `AGENTS.md` until repetition justifies promotion.
- Current status:
  - proposal and sample materials reviewed
  - current workflow docs and routing assets reviewed
  - minimal experiment implemented
  - dogfood learning closeout drafted
  - reviewer packet drafted

## Commands and evidence consulted

| Kind | Command or source | Why | Result |
|---|---|---|---|
| command | `sed -n '1,220p' .agents/skills/agent-workflow-router/SKILL.md` | choose the correct work mode for workflow assets | consulted |
| command | `sed -n '1,260p' README.agent.md` | inspect current control-plane model | consulted |
| command | `sed -n '1,280p' .agents/skills/pg-task-intake/SKILL.md` | inspect current intake and routing surface | consulted |
| command | `sed -n '1,260p' notes/tasks/_template/task.md` | inspect task-memory hot path | consulted |
| command | `sed -n '1,260p' notes/tasks/_template/review.md` | inspect reviewer packet expectations | consulted |
| command | `sed -n '1,260p' notes/tasks/_template/log.md` | inspect task-log expectations | consulted |
| command | `sed -n '1,260p' agent/EVOLUTION_RULES.md` | check when workflow growth is justified | consulted |
| command | `git diff -- README.agent.md .agents/skills/pg-task-intake/SKILL.md notes/tasks/_template/task.md notes/tasks/_template/learning.md notes/tasks/agent-learning-workflow-experiment/*` | review the patch for consistency and duplication | consulted |
| command | `rg -n "Learning Objective|learning objective|learning.md|Pattern Audit Pack|Best Practices Extracted|sibling sweep|sweep" ...` | check terminology and trigger consistency across changed files | consulted |
| command | `src/tools/pgindent/pgindent .` | satisfy the repo's pre-commit formatting rule | completed; no additional formatting changes in the touched workflow files |
| command | `git diff --check` | verify the patch is free of whitespace and conflict-marker issues | passed |
| source | proposal and hashed-SAOP learning materials in sibling worktree | ground the workflow change in a real case | consulted |

## Key findings

- The current workflow already handles correctness and reviewability well; the gap is optional learning-oriented closeout, not a missing correctness gate.
- The cleanest minimal landing is `README.agent.md` + `pg-task-intake` + a new `learning.md` template, with `AGENTS.md` explicitly deferred.
- Dogfooding the new template on this task was cheap and exposed no immediate mismatch in file ownership or template shape.

## Evidence gaps

- Ready-check and committer-style review not yet recorded

## Closeout status

- final gate or verdict: Ready for deep review
- stopping condition reached: yes
- handoff target or closeout disposition: reviewable workflow patch with task note, learning closeout, and reviewer packet recorded

## Next step

- validate the experiment on future deep-dive bugfix tasks before promoting any part of it into `AGENTS.md` or a dedicated skill

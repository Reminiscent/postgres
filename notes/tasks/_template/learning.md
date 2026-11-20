# Learning Closeout: <slug>

Use this file when the task has a learning objective above `fix-only` (see `pg-learning-pack`).
This file supplements `task.md` and `review.md`; it does not replace the normal evidence and readiness workflow.

## Trigger

- learning objective:
- why this closeout exists:
- scope limits:

## Context Pack

Use this for `deep-dive` and above.

- execution or call chain:
- inputs and runtime state already prepared before the focal function:
- relevant planner / executor / JIT / memory-context or subsystem boundaries:
- why the logic belongs in this layer rather than a neighboring one:

## Root-Cause Pack

Use this for `root-cause` and above.

- symptom or semantic mismatch:
- broken assumption, hidden contract, or invalid abstraction:
- why the behavior happened:
- why the chosen patch, conclusion, or guard works:
- what would regress if the broken assumption reappeared:

## Pattern Audit Pack

Use this for `pattern-audit` and above.
Write up the sibling sweep from `pg-task-intake` as a reusable pattern.

| Path or construct | Why checked | Classification (`same defect` / `explicitly guarded` / `different invariant` / `unknown`) | Evidence | Risk or next step |
|---|---|---|---|---|
|   |   |   |   |   |

- generalized pattern:
- search heuristic for future audits:

## Refactor Pack

Optional. Use this when a cleanup, helper extraction, or patch-series discussion is a real next step.

- smallest refactor worth doing:
- why it is safe to defer or worth splitting:
- alternatives that look cleaner but are not worth the scope:

## Best Practices Extracted

Use this for `root-cause` and above.

- engineering principle violated:
- what to check first next time:
- lowest-cost reliable prevention:
- test or review heuristic worth reusing:
- attractive but low-ROI alternative to avoid:

# Evolution Rules

Only expand the workflow in response to observed failure or repeated friction.

## Change `AGENTS.md` only when:

- the same hot-path mistake happens at least twice,
- the same reviewer complaint appears at least twice,
- the rule is truly durable across tasks rather than specific to one subsystem.

## Add or split a skill only when:

- a multi-step manual workflow recurs at least three times,
- the workflow has a stable input/output contract,
- success can be evaluated separately from general coding quality,
- the new skill does not substantially overlap `pg-task-intake`, `pg-debug-bugfix`, `pg-design-research`, `pg-rationale-trace`, `pg-ready-check`, or `pg-review-committer`.

## Add a directory-level `AGENTS.md` only when:

- a subsystem-specific invariant recurs,
- the subsystem needs different reading order or validation lanes,
- root guidance is no longer precise enough.

## Expand rationale support only when:

- missing design history caused a real failure,
- the same rationale gap recurs,
- the missing evidence would have changed the outcome of the task.

## Expand validation lanes only when:

- the same class of bug escapes the current lanes twice,
- buildfarm, CI, or review repeatedly exposes the same missing verification dimension.

## Add new memory templates only when:

- the same information is repeatedly lost between sessions,
- the missing state cannot be recovered cheaply from code, tests, or logs.

## Do not add complexity when:

- the only argument is “it feels more thorough,”
- trace length grows but review quality does not,
- prompt length grows but task success does not,
- two files or skills begin repeating the same rule.

# Task: <slug>

Fill only the sections that apply to the task type. Field definitions
live in `pg-task-intake`; leave a field out rather than writing "N/A".

## 1. Intake

- Task type: change / investigate / review / benchmark
- Investigation kind (investigate only; default `explain-current`):
- Target readiness (optional for pure review): RFC / WIP / Answered / Ready / Perf
- Learning objective (optional; see `pg-learning-pack`):
- Evidence plan (validation lanes and non-command sources):
- Community deliverable (see `pg-submit`), or none:
- Read-first set:

## 2. Question and value

- current behavior or question:
- target outcome:

For `change`, `bug-triage`, and `design-explore`, answer the value
checklist in `pg_engineering_principles.md` §3:

- harm class (security / corruption / wrong result / crash / dump-upgrade failure / error / performance / cosmetic):
- evidence of real impact (field reports, reproducer, fuzz or assert-only, hypothetical):
- who pays for the change (every query, builds, packagers, extensions, users):
- cheapest effective remedy (real fix, clear error, documentation, nothing):
- prior work (upstream fix, thread, commitfest entry, earlier rejection) and decision: proceed / join / stop:
- likely committers for this area; release-cycle position:
- specified or intended behavior, and its authority:
- irreversible surfaces touched (syntax, keyword, GUC, option, API/ABI, output format, catalog, on-disk):

## 3. Investigation note

For `investigate` tasks, before edits or temporary probes.

- question and stopping condition:
- current evidence or source set:
- likely invariants or design constraints:
- current explanation or hypothesis:

For `bug-triage`:

- symptom:
- expected behavior:
- reproducer or proof plan:
- root cause:
- recommended fix direction:
- sibling sweep result (`pg-task-intake`):
- affected branches and back-patch note:

## 4. Blast radius and invariants

- subsystem and files:
- invariants touched:
- user-visible impact (SQL semantics, docs, clients, dump/upgrade, catalog):

## 5. Rationale and precedent

- direct provenance (commits, `Discussion:` threads, docs):
- community precedent for each alternative (`pg-rationale-trace` precedent mode): source, who, position, outcome and date, still applies?:
- SQL standard, external precedent, or papers (`pg-design-research`), if any:
- what is still unclear:

## 6. Plan

For `change` tasks and `design-explore` investigations.

- alternatives considered, the precedent for each, and why rejected:
- smallest coherent patch:
- HEAD fix versus back-branch fix, if they differ:
- sibling sweep result (`pg-task-intake`):
- tests deliberately not added, and why:
- deferred work by design:

## 7. Evidence / validation

Executable lanes and non-command sources, planned and actual.

| Kind | Lane or source | Why | Command or reference | Required? | Result |
|---|---|---|---|---|---|
| validation |   |   |   |   |   |
| source |   |   |   |   |   |

## 8. Closeout

For pure `investigate` work at `Answered`, pair this with `pg-ready-check`.
Reviewable patches use `review.md` instead.

- question answered or result:
- evidence that resolved it:
- stopping condition reached:
- remaining uncertainty:
- likely reviewer objections or follow-up questions:
- recommended next step and why:

---
name: pg-debug-bugfix
description: Use this skill for PostgreSQL defect triage after intake. It supports `investigate` tasks of kind `bug-triage` with proof of failure, root-cause isolation, and a justified fix strategy without conflating diagnosis with the final patch.
---

# pg-debug-bugfix

## Purpose

Defect triage is not generic implementation work.

This skill exists because a good PostgreSQL `investigate / bug-triage` task usually needs a different proof chain:

1. state the symptom precisely,
2. reproduce or characterize the failure,
3. identify the violated invariant,
4. localize the cause,
5. decide the smallest plausible fix direction,
6. state what must be proven before implementation can begin.

## PostgreSQL habits to preserve

- Prefer exact reproductions over vague descriptions.
- For SQL-visible bugs, prefer a self-contained reproducer such as a `psql` script or the smallest targeted test you can build.
- If the bug involves an error message, capture the most informative form of the error.
- Follow PostgreSQL bug-reporting discipline: facts first, exact reproduction steps, exact output, expected output, and relevant environment details.
- Assume current behavior may be intentional until local rationale proves otherwise.
- Do not let diagnosis silently turn into the final patch; temporary probes, assertions, or logging are acceptable, but the committed fix belongs to `change`.
- For supported branches, call out backpatch or release-risk implications explicitly.

## Inputs

- symptom
- expected behavior
- current repro, if any
- target subsystem or files
- whether supported branches may be affected

## Procedure

1. Restate the defect in one paragraph.
2. Write the bug note before editing: symptom, expected behavior, current proof, root-cause hypothesis, and proof plan.
3. Reproduce the failure with exact commands when practical.
4. Reduce the reproduction until the broken invariant is clear.
5. Decide whether the current behavior may be historically intentional; if yes, use `pg-rationale-trace`.
6. Localize the failure to the most likely layer: parser, planner, executor, storage, concurrency, client code, WAL/recovery, catalog, or tooling.
7. Propose the smallest coherent fix direction.
8. Identify the best proof vehicle for the eventual patch: regression, TAP, isolation, or another targeted check.
9. Record whether the eventual fix looks safe for backpatching or likely needs branch-specific caution.

## Output format

Return:

### 1. Defect statement
- symptom:
- expected behavior:
- violated invariant:

### 2. Current proof
- exact repro commands:
- current result:
- reduction status:

### 3. Root-cause view
- likely layer:
- root-cause hypothesis:
- why this is the most likely cause:

### 4. Fix plan
- recommended fix direction:
- why larger fixes are unnecessary:
- likely side effects:

### 5. Proof plan
- failure proof:
- future fix proof:
- recommended regression vehicle:
- backpatch or release-risk note:

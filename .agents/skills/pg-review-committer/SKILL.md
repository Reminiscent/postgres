---
name: pg-review-committer
description: Use this skill before handoff to perform a PostgreSQL committer-style self-review and produce a reviewer packet grounded in exact evidence.
---

# pg-review-committer

## Purpose

Do the review a PostgreSQL committer will eventually do anyway.

This skill is the final quality gate before handoff. It should not be reduced to style review or packet polishing.

## Inputs

- patch summary or review target
- changed files
- affected invariants
- rationale cards or rationale pack
- commands run
- validation results
- design research notes, if any
- notes from `code_review.md`

## Review dimensions

Review in this order:

1. submission
2. usability and behavior
3. feature tests
4. performance evidence
5. coding review
6. architecture review
7. review review

Use `code_review.md` as the rubric source.

## Required checks

- Restate the patch or review target in one paragraph.
- Identify the highest-risk invariants explicitly.
- Decide whether the evidence matches the risk.
- Call out missing rationale, missing tests, portability risk, and hidden scope.
- Check whether docs, output files, client behavior, `pg_dump`, or upgrade impact were ignored.
- Make reviewer focus explicit.

## Output format

Return:

### 1. Verdict
- `Ready for deep review`
- `Needs narrower scope`
- `Needs stronger rationale`
- `Needs stronger verification`
- `Needs architecture revision`
- `Needs portability or flakiness work`

### 2. Findings
List the highest-signal findings first.

### 3. Remaining risks
- what could still be wrong:
- what was not verified:
- what a reviewer should inspect first:

### 4. Reviewer packet
- problem:
- current behavior:
- desired behavior:
- changed files and why:
- invariants touched:
- rationale consulted:
- external precedents or papers consulted:
- exact validation commands and results:
- user-visible impact:
- backpatch or branch-risk note:
- candidate `Discussion:` links, if relevant:

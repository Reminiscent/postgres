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
- Check the patch against `pg_engineering_principles.md`: the committer questions in §2 and the stage checklists in §3, especially value, sibling search, test worth, and branch scope.

## Output format

Return:

### 1. Verdict
One of the result labels in `code_review.md`.

### 2. Findings
List the highest-signal findings first.
Apply the finding triage in `code_review.md` §8.

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
- rationale and community precedent consulted:
- external precedents or papers consulted:
- exact validation commands and results:
- user-visible impact:
- backpatch or branch-risk note:
- candidate `Discussion:` links, if relevant:

The reviewer packet is the source for the posting email. When the work
will be posted, `pg-submit` turns it into the email; do not maintain a
second summary.

---
name: pg-submit
description: Use this skill to turn a Ready patch, an RFC, a bug diagnosis, or a review into what the PostgreSQL community actually sees (a plain-text email draft and patch files), to handle follow-up after posting, and to run the post-commit retro.
---

# pg-submit

## Purpose

The community never sees `notes/`. It sees an email, attached patches,
CI results, and how the author responds. This skill prepares those and
closes the loop once the outcome is known.

The agent drafts, and may push to the user's own fork for CI after
asking. The user sends email and registers commitfest entries.

## When to use

- a patch reached `Ready` or `Perf` and `pg-review-committer` returned
  `Ready for deep review`,
- a bug diagnosis reached `Answered` and belongs on a bug thread,
- a design is at `RFC` and needs agreement in principle before
  implementation,
- a review of someone else's patch is ready to post,
- a posted patch received review, stopped applying to master, or was
  committed, returned, rejected, or withdrawn.

## Deliverable shapes

| Work | Where | When to post | Core content |
|---|---|---|---|
| fix for a reported bug | reply on its `pgsql-bugs` thread | as soon as reproducer and root cause are confirmed; polish in later versions | reproducer, root cause, draft patch, affected versions |
| bug we found | new `pgsql-bugs` report, or `pgsql-hackers` with a patch | same | same, plus how it was found |
| feature or behavior change | new `pgsql-hackers` thread | at `RFC`, before `Ready`-level implementation | problem, proposed semantics and interface, alternatives, open questions |
| patch after direction is agreed | the same thread | at `Ready` | patch series, tests, docs |
| review | reply on the patch thread | once every finding is verified | numbered findings, blocking separated from optional |
| performance result | `pgsql-hackers`, or `pgsql-performance` for workload reports | at `Perf` | workload, build, repeated numbers with noise, worst case |

Suspected security vulnerabilities never go to a public list; they go to
`security@postgresql.org` (reference §8.4).

Bugs reward speed. Committers often fix a reported bug within days and
frequently write their own version. A precise diagnosis posted early is
used and credited; a polished patch posted late duplicates their work.

An early bug-thread post is an `Answered` diagnosis that may attach a
draft patch, labeled as a draft in the email. Its gates are the
`Answered` gates plus, for the draft: it compiles, the nearest tests
pass, and the reproducer passes with it. The full `Ready` gates apply to
the version proposed for commit.

## Procedure for a patch

1. Prepare the series with the commands in `LOCAL_WORKFLOWS.md`
   "Preparing a Patch for Posting": patch files cover only the topic
   commits, never agent workflow files or notes, and apply to current
   upstream master.
2. Shape the series by reviewability: each commit compiles and passes
   tests, mechanical changes are separate, interdependent changes are
   not split, and the order is stated (reference §7.1).
3. Confirm the `Ready` gates in `pg-ready-check`, including the `CI`
   lane, and record the `pg-review-committer` verdict.
4. Generate files with `git format-patch -v<N>`. Version numbers are never
   reused. Put `WIP` in the subject when the patch is for discussion only.
5. Draft the email in `notes/tasks/<slug>/email.txt` following the rules
   below.
6. Hand off to the user: which message to reply to, or which list for a
   new thread, the files to attach, and whether a commitfest entry is
   needed. New features and non-urgent fixes need one. A fix on an active
   bug thread usually does not, unless it stalls.

## Email rules

- Plain text, wrapped near 72 columns. No Markdown headings, bold,
  tables, or form-like field labels. Write prose; use a list only when
  the content is a list, such as numbered review points.
- Open with the problem, the current behavior, and what the patch changes,
  or what feedback is wanted.
- Then, as relevant: the minimal reproducer inline; the root cause in a
  few sentences; why this approach, and what was rejected, naming the
  earlier commit or thread whose precedent it follows, or why this case
  differs; tests, and
  that they fail before the fix; docs; user-visible and compatibility
  impact; affected versions and the proposed back-patch range with the
  reason; the sibling-search result; open questions.
- From v2 on, say briefly what changed since the previous version and
  link to the message that posted it.
- Ask for what is needed: agreement on direction, review of a specific
  part, or commit.
- Length follows content. Cut what readers of the thread already know.
- Replies are inline, with quotes trimmed, and answer every point with a
  change or a reason.
- Keep everyone already on the thread in CC. On a new thread, CC sparingly,
  for example a committer who recently worked on the same code.

For a review, follow reference §7.3 and the triage in §7.4.

## After posting

- Track the status in `log.md`: version posted and its link, commitfest
  entry, CI result, and whose move it is.
- Record each objection in `review.md` "Community outcome" when it
  arrives.
- When the patch stops applying to master, rebase and post the next
  version.
- The community expects each commitfest submitter to review a patch of
  similar size.

## Post-commit retro

Once the outcome is known, compare what was posted with what happened.
This is the most reliable signal for improving the principles and this
workflow.

1. Compare the last posted version with the committed result:
   `git range-diff <posted-base>..<posted-tip> <first-committed>~1..<last-committed>`.
   If the posted series is only in patch files, apply it to a branch at
   its base first.
2. In `review.md`, record: the outcome and how long it took; what the
   committer changed (approach, code, tests, comments, commit message)
   and their stated reason; which changes a principle predicted and which
   it missed; what to do differently next time.
3. Change `pg_engineering_principles.md` or the workflow only when the
   same gap recurs, following `agent/EVOLUTION_RULES.md`.

## Output format

Return:

### 1. Deliverable
- shape and destination (list, thread, message to reply to):
- patch files and version:
- email draft path:

### 2. Pre-posting gates
- base and apply check:
- diff scope:
- CI result:
- committer-review verdict:

### 3. Actions for the user
- send, push, or register:
- open questions before sending:

For a retro, return the outcome, the committer's changes, and the
predicted-or-missed table written to `review.md`.

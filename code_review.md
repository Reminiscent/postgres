# PostgreSQL Committer Review Checklist

Use this file during `pg-review-committer`.
The question is not “did tests pass?”
The question is “would a PostgreSQL committer have enough evidence to start deep review confidently?”
The underlying principles live in `pg_engineering_principles.md` and its
reference file; this file turns them into review checks.

## 1. Submission

Check:

- Is the problem real and stated precisely?
- Is the intended behavior clear?
- Is the scope the smallest coherent patch?
- Is the value justified: harm class, real evidence of impact, who pays,
  and why a cheaper remedy (clear error, documentation) is not enough?
- Does the patch fix the root cause rather than one symptom, and were
  sibling instances of the same mistake searched?
- For non-trivial `change` work, does the design note explain why this work should exist and how it will be proven?
- If historical rationale matters, were relevant docs, commits, or threads actually consulted?
- Where more than one approach was viable, was community precedent for the same or analogous choices checked before choosing, and does the patch follow it or explain the deviation?
- If external precedents or papers were used, are they clearly marked as supporting evidence rather than PostgreSQL authority?

Fail here when:

- motivation is vague,
- semantics are underspecified,
- scope mixes unrelated concerns,
- rationale is asserted without provenance,
- the design contradicts a recorded community stance without saying why,
- the patch adds an option, GUC, keyword, or API surface without
  evidence that it is needed.

## 2. Usability and behavior

Check:

- Is user-visible behavior precise and unsurprising?
- Are errors, notices, and messages in PostgreSQL style?
- Are compatibility, upgrade, `pg_dump`, client behavior, or docs impacts addressed when relevant?
- If outputs vary by platform, were inputs pinned first, with an alternative
  expected file used only as a last resort for verified, acceptable behavior?
- Does any GUC change query semantics or parsing?  Reject that design.
- Are incompatibilities named in the commit message for release notes?

## 3. Feature tests

Check:

- Do the tests match the blast radius?
- Was the smallest relevant lane used first?
- Were lanes widened when the risk required it?
- Do the tests prove the invariant rather than only the happy path?
- For bug fixes, is there proof of failure and proof of fix, or an explicit explanation for why one of them is still missing?
- Does each new test fail on unpatched code and actually reach the fixed
  path?  Was that shown with the recipe in `LOCAL_WORKFLOWS.md`, and the
  failing output recorded?
- Does each new test earn its permanent runtime?  Prefer the cheapest
  vehicle (SQL regression, then isolation, then TAP) and reuse existing
  objects and files.
- If no test was added, does the commit message say why?
- When existing expected output changed, was each test's original intent
  preserved rather than accepting the new output blindly?
- For each added regression case, can the author name the specific invariant it
  proves and the minimum bug-triggering dimensions it must preserve?
- Has incidental setup, duplicate execution paths, and unrelated output been
  removed without weakening that proof?
- Is the expected output both deterministic and diagnostic, without depending
  on incidental row order, plan shape, timing, locale, configuration, or
  concurrently changing state?
- If the test uses `ORDER BY`, does ordering belong to the assertion?  Do not
  add it merely to stabilize output when it would hide a useful unordered plan
  path.
- Does the test comment explain the invariant or a non-obvious setup
  constraint, rather than narrating the SQL?

For regression tests, minimal means **minimum sufficient proof**, not minimum
line count.  Reduce setup, actions, observables, and expected output
independently.  Keep a dimension when removing it stops the pre-fix failure,
changes the failure reason, or leaves a relevant branch unproved.

Typical mappings:

- parser, planner, executor, SQL semantics: regression
- client programs and TAP-heavy code: TAP
- concurrency, locking, visibility, transactions: isolation
- larger blast radius or unclear coverage: wider regression

## 4. Performance evidence

Check:

- Is there a performance claim at all?
- If yes, was it measured on a release build rather than a debug/cassert build?
- Are benchmark commands, environment notes, and caveats explicit?
- Is the claim proportional to the evidence?
- Was the worst case measured, and does the common case not regress?
- Are repeated runs, medians, and the noise floor reported?

## 5. Coding review

Check:

- Does the code fit surrounding PostgreSQL style and structure?
- Are names precise and domain-accurate?
- Do comments explain intent rather than restating the code?
- Does each non-trivial comment state a fact that is both true now and likely
  to remain useful after a local refactor: an invariant, reason, ownership or
  lifetime rule, concurrency constraint, portability issue, or surprising
  contract?
- Could the same fact be expressed more reliably by a name, type, assertion,
  or simpler control flow?  If so, prefer that and remove the comment.
- Does any comment duplicate an enum, field type, nearby inline steps, or an
  exhaustive list that will drift as the code grows?
- Is every claim scoped and qualified no more broadly than the implementation
  guarantees?  Check names, phases, lock state, error behavior, and
  concurrency windows literally.
- When concurrency, lifetime, or a non-obvious safety argument is involved,
  does the comment explain the dangerous alternative and why the guard is
  sufficient?  Brevity must not erase the proof.
- Were stale or newly misleading nearby comments updated or removed with the
  code change?
- Is there unnecessary churn, dead code, or speculative abstraction?
- Does new code look like the adjacent code, without gratuitous
  differences or duplicated logic?
- Does every non-trivial new function have an informative header comment?
- Do user-reachable errors use `ereport` with an `errcode`, with `elog`
  reserved for can't-happen cases?
- Is the choice between `Assert` and a runtime check driven by who can
  break the invariant and whether bad input can reach it?

Build system and generated files.  These are the failures CI and the
buildfarm catch first; commands live in `LOCAL_WORKFLOWS.md`.

- Is every new source, header, or test file registered in both the
  `Makefile` and `meson.build`?  Is a new regression test in the schedule,
  and a new TAP test in the `tests` list of `meson.build`?
- Did `pgindent` leave files outside the patch untouched, and is every new
  typedef in `src/tools/pgindent/typedefs.list`?
- Do changed Perl files pass `pgperltidy`, and changed catalog `.dat`
  files pass `reformat-dat-files`?
- Do new catalog OIDs come from `unused_oids` in the 8000–9999 range, with
  the catversion bump left to the committer?
- Is the patch warning-free at an optimizing level, not only at `-O0`?
- Is `git diff --check` clean, with no stray files, debug output,
  `#if 0` blocks, or unrelated whitespace changes?
- Did CI on a personal fork pass, or is the reason it was not run
  recorded?

## 6. Architecture review

Check:

- Are the touched invariants listed explicitly?
- Does the logic belong in this layer and file?
- Did the patch create hidden coupling or fight existing structure?
- Are concurrency, WAL, recovery, catalog, protocol, or portability implications handled explicitly?
- Are remaining risks named honestly?
- Is the back-patch scope justified by
  `pg_engineering_principles_reference.md` §8.1?  User-visible behavior, plan-choice, catalog-content, and ABI
  changes stay out of released branches.
- If the HEAD and back-branch fixes differ, is each scoped and explained?

Typical invariants:

- SQL semantics
- snapshot and visibility rules
- lock ordering
- WAL and recovery symmetry
- memory context ownership
- catalog and versioning state
- frontend/backend contract
- portability and buildfarm behavior

## 7. Review review

Check:

- Does the packet make human review faster?
- Are exact commands or exact evidence references, and concise results, recorded?
- Are missing checks and uncertainties explicit?
- Would a reviewer know where to look first?
- Is the commit message prose per reference §7.2, with trailers only for
  recorded facts?
- If AI assisted, was every reported problem confirmed by an executed
  reproducer that the submitter checked (reference §7.5)?

## 8. Finding triage

Apply `pg_engineering_principles_reference.md` §7.4 before reporting: it
defines what is always reported and what is usually omitted. This matters
most for AI-generated review, which tends to surface many narrow,
contrived failures.

Check:

- Does every reported finding state a concrete, reachable failure
  scenario?  Without one, it is a question, not a finding.
- Does severity override rarity, but rarity override only small impact?
- Are trivially fixable minor issues at most a one-line optional nit?
- Are pre-existing narrow issues left out unless they meet the
  always-report bar?

## Result labels

Use one of these outcomes:

- **Ready for deep review**
- **Needs narrower scope**
- **Needs stronger rationale**
- **Needs stronger verification**
- **Needs architecture revision**
- **Needs portability or flakiness work**

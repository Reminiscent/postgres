# PostgreSQL Committer Review Checklist

Use this file during `pg-review-committer`.
The question is not “did tests pass?”
The question is “would a PostgreSQL committer have enough evidence to start deep review confidently?”

## 1. Submission

Check:

- Is the problem real and stated precisely?
- Is the intended behavior clear?
- Is the scope the smallest coherent patch?
- For non-trivial `change` work, does the design note explain why this work should exist and how it will be proven?
- If historical rationale matters, were relevant docs, commits, or threads actually consulted?
- If external precedents or papers were used, are they clearly marked as supporting evidence rather than PostgreSQL authority?

Fail here when:

- motivation is vague,
- semantics are underspecified,
- scope mixes unrelated concerns,
- rationale is asserted without provenance.

## 2. Usability and behavior

Check:

- Is user-visible behavior precise and unsurprising?
- Are errors, notices, and messages in PostgreSQL style?
- Are compatibility, upgrade, `pg_dump`, client behavior, or docs impacts addressed when relevant?
- If outputs change, were alternative expected files considered?

## 3. Feature tests

Check:

- Do the tests match the blast radius?
- Was the smallest relevant lane used first?
- Were lanes widened when the risk required it?
- Do the tests prove the invariant rather than only the happy path?
- For bug fixes, is there proof of failure and proof of fix, or an explicit explanation for why one of them is still missing?

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

## 5. Coding review

Check:

- Does the code fit surrounding PostgreSQL style and structure?
- Are names precise and domain-accurate?
- Do comments explain intent rather than restating the code?
- Is there unnecessary churn, dead code, or speculative abstraction?
- Are formatting tools or generated-file updates handled when required?

Checklist items to consider when applicable:

- `pgindent`
- `pgperltidy`
- `reformat-dat-files`
- Perl checks
- `err*()` style
- warnings and compiler cleanliness

## 6. Architecture review

Check:

- Are the touched invariants listed explicitly?
- Does the logic belong in this layer and file?
- Did the patch create hidden coupling or fight existing structure?
- Are concurrency, WAL, recovery, catalog, protocol, or portability implications handled explicitly?
- Are remaining risks named honestly?

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

## Result labels

Use one of these outcomes:

- **Ready for deep review**
- **Needs narrower scope**
- **Needs stronger rationale**
- **Needs stronger verification**
- **Needs architecture revision**
- **Needs portability or flakiness work**

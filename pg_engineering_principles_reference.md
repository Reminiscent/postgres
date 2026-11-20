# PostgreSQL Engineering Principles: Reference

Detailed principles behind `pg_engineering_principles.md`. Read the core
file on every task; read only the section of this file that matches the
decision at hand.

Written from the contributor's side. Committers make the final calls on
commit, back-patch range, release timing, and reverts. A contributor's
job is to make a committer's "yes" cheap and safe: the patch already
answers the questions a committer would ask. Committer-only duties are
deliberately left out.

## 1. Value: is this worth doing?

### 1.1 Is a bug worth fixing?

1. **Always fix** crashes, wrong results, data corruption, and undefined
   behavior in the C sense, even on paths almost nobody uses.
2. **Lack of field complaints scales the remedy, not the decision.** A
   long-unnoticed bug may deserve only a clear error that explains the
   workaround, a HEAD-only fix, or documentation. Years without a report
   are evidence that few people use the case, and they outweigh "this
   could happen" arguments.
3. **How the bug was found matters.** Fuzzer-only, valgrind-only, and
   assert-build-only failures usually have lower production impact.
4. **Privileged misuse is "don't do that".** Misbehavior that needs a
   superuser, an untrusted language, or deliberately installed malicious
   files is not a security bug. Anything that crosses a trust boundary
   is.
5. **Compare the fix cost with the cost to the common path.** A fix that
   costs nothing is worth doing without a use case. Do not slow the
   common case to serve a rare one.
6. **Documented, standard-required, or intended behavior is not a bug.**
   That settles bug status, not whether HEAD could be improved on its own
   merits. Long-standing, sane behavior that users have adapted to is
   usually documented rather than changed.
7. **Code that is buggy on its own terms** is worth fixing even without a
   visible symptom.

### 1.2 Is a feature or behavior change worth doing?

1. **Marginal benefit loses to even small risk.** Weigh benefit against
   complexity, compatibility hazard, and maintenance.
2. **Demand must be shown**: real requests, complaints, or workloads.
   An arguable bug with no demand is a HEAD-only feature, or nothing.
3. **Tidiness alone justifies no breakage.** Removing a user-visible
   feature needs a payoff users can see.
4. **Count who pays.** Detection logic that runs on every query is paid
   by every query. Configure-time dependencies are paid by packagers.
   API changes are paid by extension authors.
5. **Is the proposal a workaround for a different root problem?** Then
   propose fixing that problem.
6. **Prefer improving the mechanism users already rely on** over an
   ambitious new design.
7. **Question the premise and shrink v1.** Ask whether wider scope turns
   a tool into a foot-gun. The committable first step is often a
   fraction of the original proposal.
8. **Generalize after real cases accumulate**, not before.
9. **Half-done, speculative, underdocumented, or over-engineered work is
   a reason for rejection**, not something reviewers will finish for you.

### 1.3 Options, knobs, and GUCs

1. **Prefer no user knob.** Users rarely tune knobs well. Do not hand
   them a modeling problem as a setting.
2. **Never fix a compatibility problem with a GUC that changes query
   semantics**, and never let raw parsing depend on a setting.
3. **No one-off, non-composable options.** Ask what the next request
   will be, and propose a composable primitive or general mechanism.
4. **Exception: an escape hatch** for a performance regression or
   resource limit.
5. **An option, once added, is permanent.**

### 1.4 Standard and compatibility arguments

1. **Spec-required behavior plus backward compatibility wins** when
   compliance is achievable. Extras beyond the spec give way.
2. **Verify "the spec says" against the text.**
3. **When the spec is unachievable or worse**, propose the more useful,
   more intelligible, less bug-prone behavior and say why.
4. **Leave room for the standard to grow.** Avoid PostgreSQL-only syntax
   where the committee is likely to extend.
5. **Compatibility with another product is justified only by fidelity.**
   Extras the target system lacks earn nothing.
6. **New reserved keywords are expensive.** Do not reserve beyond the
   standard; prefer some grammar ambiguity.

### 1.5 Core versus elsewhere

1. **Core features must generalize** to user-defined types, operators,
   and access methods. Do not hard-wire built-in types.
2. **What works well outside core stays outside core.** Test-only or
   hazardous capabilities belong in a loadable module.

### 1.6 Prior work, ownership, and timing

1. **Check that it is not already done or in progress**: fixed upstream
   since your base, an existing thread or commitfest entry, or a patch
   someone else has posted. Reviewing or improving existing work beats a
   competing patch.
2. **Find earlier rejections** in the archives and the wiki's "Not Worth
   Doing" page, and answer their objections up front.
3. **Know who would commit it.** A patch in an area no committer is
   interested in waits indefinitely. Look at who commits to the touched
   files and who replied on related threads, and make the thread easy for
   them to find.
4. **Check the release cycle** before choosing scope (§8.2).

---

## 2. Confirming the problem

1. **Have a self-contained reproducer and the affected versions.** An
   obvious bug with an obvious fix is the only exception.
2. **Expect skepticism about long-standing code.** Bring the mechanism,
   not just the symptom.
3. **Establish expected behavior from an authority**: the docs (they are
   the spec), the SQL standard, or earlier decisions. Search the
   archives for a previous rejection of the same change.
4. **A gap between code and docs is a bug**, unless the docs were wrong.
5. **Explain the mechanism before patching.** Bisect, compare branches,
   bound when it started. Make rare races reproducible by widening the
   window or forcing the rare path.
6. **State the impact precisely**: who hits it, how often, which
   versions, which direction of client/server version skew.
7. **Do not patch a symptom you cannot explain.** A wrong diagnosis gets
   the patch rejected or reverted.

---

## 3. Design

### 3.1 Interface first

1. **Define the interface and exact semantics before coding**: syntax,
   precise behavior, error conditions, backward compatibility. Get
   agreement at that level of detail first.
2. **List the blast radius**: subsystems, interactions with other
   features, catalog changes, `pg_dump`/`pg_upgrade` impact, `psql` and
   client changes.
3. **For a new mechanism, lead with a prose API specification** and one
   or two example users. "Read the code" is not a specification.

### 3.2 General

1. **Fix the root cause, not one symptom.** When single-purpose fixes
   keep accumulating, fix the shared layer.
2. **Reuse the nearest existing machinery** instead of building a
   parallel path.
3. **Normalize representations early** so later code has no special
   cases. Represent "unknown" explicitly, never as a fake value.
4. **Reason from existing invariants.** A generated or rewritten query
   must obey the same rules as the equivalent hand-written one.
5. **Be consistent and orthogonal.** Nothing should mean something
   special in one place only. Do not copy an old wart.
6. **Effort proportional to value.** For rare unsupported semantics, a
   clean error beats heavy machinery. But do not refuse a capability up
   front merely because it might be unavailable.
7. **Simpler and obviously correct beats clever.** On code with a high
   defect rate, redesign instead of adding a layer.
8. **List alternatives, check community precedent, then choose the most
   explainable.** How the community decided the same or analogous
   choices is the best predictor of what reviewers will ask for. Follow
   the latest committed stance or explain why this case differs; the
   procedure is `pg-rationale-trace` precedent mode. Other systems are
   evidence, not authority.
9. **Remove ill-defined behavior** rather than patching one path through
   it.
10. **Separate machine contracts from human-readable output.** Output
    that tools parse is an interface.
11. **Keep future options open.** Reject ambiguous input now so it can
    be defined later. Do not shape syntax around a temporary
    implementation limit.
12. **Readability outranks micro-efficiency** unless a real need is shown.

### 3.3 SQL semantics and syntax

1. **Least astonishment.**
2. **Making failing cases work is cheap; changing working behavior is
   expensive.** De facto client-visible behavior is protocol.
3. **Tighten semantics only where old behavior was ill-defined.**
4. **Heuristics become permanent compatibility surface.** Prefer an
   exact, even narrower, rule.
5. **Put semantic checks in the parse or plan tree**, not new executor
   checks on existing paths.

### 3.4 APIs and extensibility

1. **Extension points should be general and growable**, not one-off
   hooks.
2. **Do not freeze internal decisions into an API early.** Expose
   building blocks, not policy.
3. **If an API's meaning changes, break callers visibly** with a rename
   or signature change.
4. **Check external callers** (for example, a code search across
   distribution packages) before changing an API or hook, and report
   what you found.
5. **Encapsulate invariants.** Hidden assumptions, such as leakproofness
   or skipping a lock, must be checked or justified in a comment.

### 3.5 Catalog, storage, and upgrade

1. **Dump/restore and `pg_upgrade` are part of the design.** They must
   not error out or silently change behavior.
2. **Do not shape catalog structures around one use case.** Prefer
   first-class representations.
3. **Rely only on identifiers known to be stable** across installations
   and versions.
4. **Fix representation mistakes before release**, while an initdb for
   beta testers is still acceptable.
5. **On-disk format breaks need explicit mitigating evidence.**
6. **Say whether catversion must change**; leave the bump itself to the
   committer.

### 3.6 Planner and cost model

1. **Mechanism versus cost.** If the plan cannot be generated, fix the
   mechanism. If it is generated and rejected, fix the estimate.
2. **Most failures are statistical.** A cost-based choice needs
   trustworthy statistics; a model is no better than its inputs, and
   every formula needs a stated justification.
3. **Near-zero overhead for queries that do not benefit.** Bound extra
   plan-time work and fall back to existing statistics.
4. **Estimate with risk in mind.** Prefer plans that degrade gracefully;
   never assume a table is empty or tiny unless known.
5. **Justify optimizations with queries the system produces**, such as
   after view expansion, not badly hand-written SQL.
6. **For a performance regression, find where the complexity comes
   from** and make only the inputs that need the extra work pay for it.

### 3.7 Concurrency and resources

1. **Take the strongest lock you will need up front.**
2. **Never run user code in fragile states**: abort cleanup, no
   snapshot, inside comparison functions.
3. **Assume a cache flush at any catalog access.**

### 3.8 Fix scope and branch shape

1. **Minimal means the smallest coherent change that removes the root
   cause**, not the fewest lines.
2. **When HEAD and released branches need different fixes, say so.**
   Propose the principled fix for HEAD and a narrow fix for released
   branches, and label the narrow one as a band-aid.
3. **If surrounding code is already hard to follow**, rewrite it in
   project style rather than adding a minimal patch.
4. **No refactoring or performance work inside a fix meant for back
   branches.**

---

## 4. Implementation

### 4.1 Conventions

1. **Look like the adjacent code.** Gratuitous differences are defects.
2. **Project style**: `palloc` in the backend; check every system call;
   `postgres.h` first; matching struct tag and typedef names; no trivial
   GUC wrappers; no `#ifdef` toggles for your change; no author names in
   code.
3. **`static inline` over macros** that evaluate arguments twice.
4. **Variables modified in `PG_TRY` and used afterwards are
   `volatile`**, or are restructured to avoid it.
5. **Save `errno` before `ereport`.**
6. **Remove duplication while fixing**; refactor once copies multiply.
7. **Remove dead fields and state** that your change makes unused.
8. **Delete a check that can never fire** instead of "fixing" it.

### 4.2 Comments and naming

1. **Every non-trivial function has an informative header comment.**
2. **Comments explain why** and answer a first-time reader's questions.
   Verbose comments that restate code are noise.
3. **Update every comment your change makes false.**
4. **Justify unproven safety assumptions** in a comment, and mark code
   that is safe only under specific conditions.
5. **Cross-reference files, not functions.**
6. **Names say what, not how.** Be specific and consistent; weigh
   renames against churn.

### 4.3 Errors and messages

1. **User-reachable errors use `ereport` with an `errcode`.**
2. **`elog` only for can't-happen cases**; do not burden translators.
3. **Message style**: one-line primary message saying why it failed;
   detail is fact; hint is advice; name the object kind; error cursor
   where possible.
4. **Do not reword existing translatable messages for neatness.**
5. **Portable 64-bit formats in translatable strings.**

### 4.4 Assert versus runtime check

1. **Runtime test-and-elog** when the check is cheap, bad input can reach
   it, or other modules (including extensions) could break the
   invariant.
2. **`Assert`** for local invariants no caller has violated, or checks
   too expensive for production.
3. **No struct layout that depends on `USE_ASSERT_CHECKING`.**

### 4.5 Memory and resources

1. **Leaks matter by context lifetime and call frequency.** Rely on
   context resets rather than retail `pfree`; free what you allocate in
   long-lived contexts.
2. **Context resets are not free** on hot paths.
3. **Do not modify input trees** unless the contract allows it.
4. **Keep signal handlers and headers dependency-light**; avoid include
   bloat and pass the header checks.

### 4.6 Portability

1. **Code the buildfarm does not test is presumed broken.**
2. **Stay within the project's C baseline**, and for code meant for back
   branches, within what every target branch supports.
3. **Avoid churn that makes back-patching harder.**
4. **Restrict platform-specific fixes to that platform.**

### 4.7 Performance-sensitive code

1. **Profile first**, and say where the time goes.
2. **Complexity for speed needs evidence.** Unproven fast paths will be
   asked to go.
3. **Off the hot path, write the clean, safe version.**
4. **Cheap insurance is welcome**: small measured costs for robustness.
5. **Side features must cost almost nothing.**

### 4.8 Siblings and recurrence

1. **Search for every other instance of the same mistake** and fix them
   together; if reachability is unclear, tighten anyway.
2. **Report the search**, including "looked, found none".
3. **Prevent recurrence** with an API that makes the mistake hard, a
   comment at the trap, or the missing test.
4. **Treat code moves across trust boundaries as high risk.**

---

## 5. Testing and verification

### 5.1 Whether a test is worth adding

1. **It fails before the fix and passes after**, and it actually reaches
   the fixed path. A clean check-world run proves neither.
2. **It earns its permanent runtime.** Core regression tests run several
   times per buildfarm run. Put expensive tests where they run once.
3. **Development tests are not committed tests.** Test widely to find
   holes; submit only the small set that demonstrates the fix.
4. **Use coverage data** to decide; tests that add no coverage are
   noise.
5. **Decline indirect, fragile, or rot-prone tests**, and say why in the
   commit message. If the bug cannot be reproduced reliably, still cover
   the code path and name the gap.
6. **For fixes proposed for back branches, show the bug and the fix in
   each target branch.**

### 5.2 Test design

1. **Test the mechanism, not incidental machinery.** Do not memorialize a
   fuzzer's accidental input.
2. **Smallest reproducer, next to similar tests, reusing objects.**
3. **Planner tests assert plan shape**, not exact row estimates.
4. **When a change alters existing expected output, preserve each test's
   intent** and add a minimal, isolated test for the new behavior.
5. **Isolate side effects**, for example with a rolled-back transaction.
6. **If output cannot be stable, still execute the code** and mask only
   the unstable part.

### 5.3 Stability and portability

1. **Assert only what the test is about.**
2. **Pin inputs** (verbosity, message locale, autovacuum, text-search
   configuration) rather than adding variant expected files. A variant
   file is a last resort, for verified, acceptable behavior only.
3. **Break near-ties in plan cost inside the test.**
4. **No undefined row order** (`LIMIT` without `ORDER BY`, order after
   `UPDATE`).
5. **No timing assumptions.** Use explicit synchronization, not sleeps.
6. **Repeatable under installcheck**: re-runnable, no cluster-wide side
   effects, no network.
7. **Think through unusual configurations**: 4-byte MAXALIGN,
   big-endian, forced parallel mode, cache-clobber builds, other locales
   and collations, older client with newer server.

### 5.4 Choosing the vehicle

1. **Cheapest framework that works**: SQL regression, then isolation,
   then TAP.
2. **Concurrency and locking → isolation; cluster setup and client
   behavior → TAP.**
3. **Prefer SQL-level synchronization over injection points** when both
   work; it is simpler and back-patches further.

### 5.5 Performance evidence

1. **Optimized, non-assert builds only.** Code-layout effects can fake
   differences of 20% or more.
2. **Measure the worst case**, and find the crossover point.
3. **Realistic workload for no-regression, microbenchmark for the win.**
   Microbenchmarks overstate.
4. **Repeated runs, medians, noise floor**, on a workload that actually
   hits the changed code.
5. **Check other architectures when the win depends on hardware**, and
   report null results honestly.
6. **Extra code for speed must buy a measurable win on a live problem.**

---

## 6. Documentation, compatibility, and clients

1. **The docs are the spec.** Decide whether code or docs is wrong.
2. **Every behavior change answers "do the docs change?"** Docs are part
   of a committable patch, not a follow-up.
3. **Documenting something commits the project to it.** Keep
   implementation details and platform-dependent behavior out.
4. **Name every incompatibility in the commit message** so it reaches the
   release notes.
5. **Check `pg_dump`, `pg_restore`, `pg_upgrade`, `psql`, older clients,
   and external tools.** Keep old archives restorable or at least
   readable.
6. **Retiring a GUC**: keep the name and pin the value if clients still
   reference it; otherwise remove it. Never make `pg_dump` emit a
   setting you hope to retire.
7. **Deprecate only with a removal plan and a payoff.** Syntax needs a
   deprecation period.
8. **Change defaults early; remove old behavior once migration is
   clearly over**, or sooner for a security hazard.

---

## 7. Patch, submission, and review

### 7.1 Patch structure

1. **Tight scope.** Unrelated fixes are separate patches; scope creep
   goes to a new thread.
2. **Split by reviewability and branch reach**, not mechanically.
   Mechanical churn (reindent, dead-code sweeps) goes in its own patch.
   Do not split interdependent changes.
3. **Huge series discourage review.** Lead with the API and one or two
   users.
4. **Etiquette**: run pgindent; leave the catversion bump to the
   committer; number versions uniquely; reply inline, not top-posted;
   no patches through the bug form; register a commitfest entry, and a
   new one for follow-up work.

### 7.2 Commit message

Committers write the final message; a contributor's draft shows intent,
saves them work, and feeds the release notes.

1. **Prose, not a form.** An imperative summary line, then short
   paragraphs. No headings, field labels, or bullet templates.
2. **Answer what matters, in this order**: what was wrong and why (the
   root cause, not only the symptom); who hits it and what happens; why
   this fix rather than a larger or smaller one, and whether it is a
   band-aid; what else changes for users (catalog contents, dump,
   clients, docs).
3. **Name every user-visible behavior change and incompatibility** so it
   reaches the release notes. State uncertainty plainly.
4. **Say why no test was added** when none was.
5. **Trailers record facts**: `Reported-by:`, `Bug:` for a bug-form
   number, `Discussion:` with a `postgr.es/m` link. Credit trailers
   (`Author:`, `Co-authored-by:`, `Reviewed-by:`, `Tested-by:`) and
   `Backpatch-through:` are the committer's call; include them only when
   the facts are already on the thread.
6. **Proposals and process go in the email**, not the message: the
   proposed back-patch range and why, a sibling search that found
   nothing, and changes since the previous version.

### 7.3 Reviewing others' patches

Reviewing is the recommended way to start contributing.

1. **Force the logic with a dilemma**: "either this is unnecessary, or
   the code above is wrong."
2. **Number the points**, give a concrete alternative, separate blocking
   from optional, and label pre-existing issues as out of scope.
3. **Check that comments, units, and code agree.**
4. **Measure independently** rather than trusting a cited model.
5. **Withdraw objections that turn out wrong.**
6. **Watch for thin review**: unresolved XXX comments, semantic changes
   labeled "cosmetic", premature "ready for committer".
7. **Suspect siblings.**
8. **Judge findings on their merits**, whatever their source.

### 7.4 Triaging review findings

Reviewer attention is scarce (P1). A review that lists every conceivable
corner case buries the findings that matter; AI-generated review is
especially prone to this. Triage each candidate by expected impact (P8).

1. **Always report**, however rare the trigger: security exposure, data
   corruption, silently wrong results, crashes or assertion failures
   reachable through ordinary SQL or client use by unprivileged users,
   dump/restore or `pg_upgrade` failures, and user-visible behavior,
   compatibility, or back-branch risk the patch introduces.
2. **Always report problems on common paths or with broad reach**:
   default configurations, frequently used features, hot paths, many
   callers, extension-visible APIs.
3. **Deprioritize, and usually omit, a finding when its impact is small
   and either** the trigger is contrived (rare setting combinations,
   impossible or corrupted state, superuser misuse, malicious files,
   hypothetical external callers) **or** explaining or fixing it costs
   more attention, complexity, or test runtime than the harm it
   prevents. Small impact means at most a clean error, cosmetic output,
   a negligible leak, or a slower rare path.
4. **Severity overrides rarity; rarity overrides only small impact.**
5. **A finding needs a concrete, reachable failure scenario.** Without
   one it is a question. When unsure, check quickly (write the SQL)
   instead of speculating.
6. **A trivial, obviously correct fix may be one optional nit.**
7. **Separate patch-introduced issues from pre-existing ones.**
8. **Do not pad.** Rank by severity; say plainly when nothing significant
   was found.

### 7.5 AI-assisted work

Follow any project policy on AI-assisted contributions. Absent one, act
on the reason such policies exist: unverified, machine-generated volume
spends reviewer attention and trust (P1), and AI-found problems can
arrive faster than the project can absorb them.

1. **You must be able to vouch for every line** and explain every design
   decision. Review generated code fully.
2. **Reproduce before reporting.** Confirm every AI-found problem with an
   executed reproducer (SQL, a test, or a debugger) before posting it,
   and the human submitter checks that reproduction. What cannot be confirmed
   is a question, not a finding, and is usually not worth posting.
3. **Generated comments must answer real reader questions.**
4. **Trim generated tests and code** that are oversized or diverge from
   adjacent code. Committers routinely shrink generated reproducers;
   post the minimal one.
5. **Verify generated performance claims.**
6. **No AI credit trailers in commits**; credit is the committer's
   decision.
7. **Write like a person.** Generated prose is long, structured, and
   generic; list readers notice. Follow the email rules in `pg-submit`.

---

## 8. Branches, timing, and security (contributor view)

### 8.1 Proposing a back-patch range

The committer decides; the patch should propose a range with reasons.

> Propose back-patching when there is real harm (crash, wrong result,
> data loss, security exposure, failed dump/restore or upgrade, or a
> regression) and the fix breaks nothing that used to work. Otherwise,
> any user-visible behavior change, plan-choice change, catalog-content
> change, or ABI change is HEAD-only, or limited to a branch that is not
> yet released.

1. **Do not break working applications in minor releases**, even with a
   clear bug fix, if someone may rely on the old behavior.
2. **No plan-choice changes in released branches**, not even alongside a
   real fix.
3. **ABI in back branches**: keep signatures; use an `_ext` variant,
   wrapper, or hidden state; never renumber node tags.
4. **How far back** depends on how cleanly it applies and the remaining
   life of the oldest branch.
5. **Cosmetic changes are HEAD-only.**

### 8.2 Release-cycle awareness

1. **The closer to release, the smaller the acceptable risk.** Do not
   push large changes or fixes for subtle old problems into RC or late
   beta.
2. **After feature freeze, a patch with any open design question waits
   for the next cycle.** Do not expect a large, barely ready patch to be
   squeezed in.
3. **Post-freeze, only problems new in this release are open items.**
   Old bugs go to the commitfest.
4. **Late in the cycle, do not propose changes that only make latent bugs
   louder.**

### 8.3 After your patch is committed

1. **Respond quickly to buildfarm failures or bug reports that implicate
   your patch**, with a diagnosis or a proposed fix.
2. **Own regressions plainly.**

### 8.4 Security

1. **Report suspected vulnerabilities privately to the security team**,
   not on public lists.
2. **What counts**: memory disclosure, privilege bypass, type confusion
   reachable by unprivileged users. A crash of one's own session is
   not, nor is anything needing superuser or malicious files.
3. **Keep a security fix minimal**; refactoring comes later, in public.
4. **After a vulnerability, look for the class**: centralize the check.
5. **Treat `search_path` capture and trojan-horse objects as standing
   threats**; cheap defense in depth is welcome.

---

## 9. Communication and consensus

1. **Sell the idea before writing much code.** Post early drafts;
   someone usually improves the idea.
2. **Ask whether the direction is acceptable in principle** before
   investing in details.
3. **Start small.** Pick a narrow area and review others' patches first.
4. **Answer every review point**, with a change or a reason.
5. **Take votes seriously and accept being outvoted.** Do not claim a
   consensus that has not formed.
6. **Move scope creep to a new thread.**
7. **Correct yourself in public** and offer compromises.
8. **Users are the source of requirements.** Cite real reports and
   workloads.

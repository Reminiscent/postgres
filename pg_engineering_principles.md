# PostgreSQL Engineering Principles

Decision principles for PostgreSQL work, from deciding whether something
is worth doing to getting it accepted. Distilled from Tom Lane's commits
and mailing-list replies; where his positions changed over time, the
latest stance is used.

- **Read this file on every non-trivial task.** For detail, read only the
  matching section of `pg_engineering_principles_reference.md`.
- **Contributor perspective.** Committers make the final calls on commit,
  back-patch range, release timing, and reverts. Our job is to make a
  committer's "yes" cheap and safe. Committer-only duties are left out.
- **Authority.** Nearby code, nearby tests, official docs, and current
  community consensus override this file. Apply the reasoning, not the
  wording, and name genuine judgment calls as such.

## 1. First principles

PostgreSQL lives for decades, is depended on by many users, tools, and
extensions, and is reviewed by a few committers with limited time. Three
resources are scarce, and every principle protects at least one:

- **Trust in correctness**: users hand PostgreSQL their data.
- **Reviewer attention**: committer time is the main bottleneck.
- **Future freedom**: what ships can almost never be taken back.

The principles:

- **P1. Costs are permanent and usually paid by others**: every query,
  every test run, every back-patch, packagers, extension authors,
  translators, and users who must learn the feature.
- **P2. Harm has a ranking**: security exposure, data corruption, and
  silently wrong results; then crashes and failed dump/restore or
  `pg_upgrade`; then spurious errors; then performance; then cosmetics.
- **P3. Irreversibility sets the evidence bar.** Released syntax and
  semantics, documented behavior, GUCs, parsed output formats, catalog
  contents, on-disk format, extension-visible APIs and ABIs, and plan
  choices in released branches are effectively permanent.
- **P4. Simple, explainable, and general beats clever and special-case.**
  It means fewer bugs, faster review, and cheaper future change.
- **P5. Evidence beats assertion**: a self-contained reproducer, a test
  that fails before the fix, coverage, optimized-build measurements,
  real field reports. Not "theoretically better" or "could happen".
- **P6. Fix the root cause and the whole class.** Ask where else the same
  mistake was made.
- **P7. Return the correct answer or raise an error.** Never be silently
  wrong.
- **P8. Priority is expected impact, with a severity floor.** Weigh
  severity, likelihood, and breadth against the cost of fixing,
  explaining, testing, and maintaining. Security exposure, corruption,
  silently wrong results, and crashes reachable through ordinary use are
  always handled; rarity only scales the remedy. Below that floor, a rare
  path with small impact does not justify a large cost. The same model
  decides bug worth, test worth, back-patch scope, and which review
  findings to raise.

Habits: state trade-offs openly, admit bias, change your mind on new
evidence and say so, and own mistakes publicly.

## 2. Make the committer's "yes" cheap

Before a patch is posted, the thread and commit message should already
answer what a committer will ask. Each question points to the reference.

| Question a committer asks | Answer to have ready | Reference |
|---|---|---|
| Is this real, and who hits it? | reproducer, affected versions, harm class, field evidence | §1.1, §2 |
| Is anyone already on it? | upstream fix, existing thread or commitfest entry, earlier rejection checked | §1.6 |
| Why is it worth the permanent cost? | demand, who pays, why a cheaper remedy is not enough | §1.2–1.5 |
| Is it intended or specified behavior? | docs, standard, earlier decisions checked | §1.1, §2 |
| Is this the root cause? | mechanism explained; sibling search and result | §3.2, §4.8 |
| What exactly changes for users? | precise semantics, compatibility, dump/upgrade, clients | §3, §6 |
| Is it the simplest general design? | alternatives considered, how the community decided similar choices, and why this one | §3.2 |
| Does the code fit? | looks like adjacent code, informative comments, proper errors | §4 |
| What proves it? | test that fails before the fix, cheap and stable; or why no test | §5 |
| Performance claims? | optimized build, worst case, repeated runs, noise | §5.5 |
| Which branches? | proposed back-patch range and reason, or why HEAD only | §8.1 |
| Is the timing right? | not a risky change late in the cycle | §8.2 |
| Docs and release notes? | docs updated; incompatibilities named in the commit message | §6, §7.2 |
| Is it easy to review? | tight scope, sensible split, clear commit message | §7.1–7.2 |

## 3. Stage checklists

**Value** (reference §1)

- [ ] Harm class and evidence of real impact stated?
- [ ] Who pays: every query, builds, packagers, extensions, users?
- [ ] Cheapest effective remedy considered: real fix, clear error,
      documentation, or nothing?
- [ ] Not already fixed upstream, under discussion, in the commitfest,
      or previously rejected? Joining existing work considered first?
- [ ] A committer likely to care about this area?
- [ ] Specified or intended behavior checked?
- [ ] Irreversible surface touched: syntax, keyword, GUC, option, API,
      output format, catalog, on-disk?
- [ ] Not a workaround for a different root problem? Generalizes to
      extensible types and access methods?

**Problem** (reference §2)

- [ ] Self-contained reproducer; mechanism explained, not just symptom?

**Design** (reference §3)

- [ ] Interface and exact semantics written down; direction acceptable in
      principle?
- [ ] Root cause at the right layer; nearest existing machinery reused?
- [ ] For each real alternative, community precedent checked before
      choosing; latest stance followed or deviation justified?
- [ ] No semantics-changing GUC, one-off option, or unneeded keyword?
- [ ] dump/restore, `pg_upgrade`, clients, and extensions considered?
- [ ] Planner: mechanism or cost? Trustworthy statistics? Bounded work?
      Near-zero cost for queries that do not benefit?
- [ ] HEAD fix and back-branch fix scoped separately if they differ?

**Implementation** (reference §4)

- [ ] Looks like adjacent code; no duplication or dead state?
- [ ] Informative header comments; stale comments fixed; safety
      assumptions justified?
- [ ] `ereport` with `errcode` for user errors; message style followed?
- [ ] Assert versus runtime check chosen by who can break the invariant?
- [ ] Sibling instances searched, fixed, and reported?

**Tests** (reference §5)

- [ ] Fails before the fix, passes after, reaches the fixed path?
- [ ] Worth its permanent runtime; cheapest vehicle; reuses objects?
- [ ] Deterministic and installcheck-safe?
- [ ] Existing test intent preserved when outputs change?
- [ ] If no test, the commit message says why?

**Docs and compatibility** (reference §6)

- [ ] Docs change decided and included?
- [ ] Incompatibilities named for the release notes?

**Branches and timing** (reference §8)

- [ ] Back-patch proposal: real harm and nothing working breaks?
      Otherwise HEAD-only.
- [ ] No behavior, plan-choice, catalog-content, or ABI change proposed
      for released branches?
- [ ] Appropriate for this point in the release cycle?

**Review findings** (reference §7.4)

- [ ] Each finding has a concrete, reachable failure scenario?
- [ ] Severe, broad, or common-path problems all reported?
- [ ] Rare-and-minor, or costly-to-explain-and-minor, findings dropped?
- [ ] Patch-introduced issues separated from pre-existing ones?

**Communication** (reference §7.2, §9)

- [ ] Commit message is prose covering cause, impact, approach, user
      impact, and tests or why none; trailers only for recorded facts?
- [ ] Email carries the back-patch proposal and version changes; every
      AI-found problem confirmed by an executed reproducer that the
      submitter checked?
- [ ] Every review point answered with a change or a reason?

## 4. Feedback loop

These principles are a hypothesis about what committers want. The
strongest evidence is what a committer changed between our last posted
version and the committed result, followed by the objections raised on
the thread. `pg-submit` records both in `notes/tasks/<slug>/review.md`,
noting which principle predicted each one and which missed. Change this
file only when the same gap or misfire recurs, following
`agent/EVOLUTION_RULES.md`.

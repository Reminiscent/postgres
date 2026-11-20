# Local Workflows

This file is the single source of truth for exact local commands used by the agent.
`pg-ready-check` should reference commands from here instead of inventing new ones.

## Per-Worktree Runtime Convention

Use worktree-local runtime state by default.

The source checkout is the build tree.
The install prefix is not the build tree, so use `PG_INSTALL_DIR` rather than `PG_BUILD_DIR`.
Keep the install tree and the data directory as siblings under a shared dev root instead of nesting the data directory under the install tree.

Why this layout:

- the install tree is disposable output from `make install`,
- the data directory is mutable state and should survive rebuilds,
- separate directories reduce accidental cleanup and make concurrent worktrees straightforward.

Recommended shell setup:

```bash
PG_WORKTREE_ROOT="$(git rev-parse --show-toplevel)"
PG_WORKTREE_CKSUM="$(printf '%s' "$PG_WORKTREE_ROOT" | cksum | awk '{print $1}')"
PG_WORKTREE_ID="$(basename "$PG_WORKTREE_ROOT")-$PG_WORKTREE_CKSUM"

export PG_INSTALL_DIR="$HOME/.pgdev/install/$PG_WORKTREE_ID"
export PG_DATA_DIR="$HOME/.pgdev/data/$PG_WORKTREE_ID"
export PG_LOG_DIR="$HOME/.pgdev/log/$PG_WORKTREE_ID"
export PGPORT="$((5600 + (PG_WORKTREE_CKSUM % 1000)))"

mkdir -p "$PG_INSTALL_DIR" "$PG_DATA_DIR" "$PG_LOG_DIR"
```

Rules:

- derive the install, data, log, and port values from the absolute worktree path,
- recompute these values when you switch worktrees instead of reusing stale shell variables,
- if the deterministic default port collides with another local service, override `PGPORT` in your shell after running the snippet,
- Record the actual values used in the task log when they matter to reproduction.

If you want this to activate automatically for manually created worktrees, the cleanest option is `direnv` with a worktree-local `.envrc` that evaluates the same snippet.

## Build Profiles

Cleanup targets:

- use `make clean` when you want to remove built files but keep the current `configure` results, so you can rebuild with `make` under the same configuration,
- use `make distclean` before re-running `./configure`, when switching build profiles or platforms, or when anything inspected by `configure` has changed,
- do not run `make clean` immediately after `make distclean`; the build-profile recipes below are reconfigure workflows, so `distclean` is the correct reset step on its own,
- in a fresh worktree that has never been configured, skip `make distclean`; it fails there.

The profiles below pass `--without-icu` because ICU is often not
installed locally.  ICU is enabled by default since PostgreSQL 16 and in
CI, and tests such as `collate.icu.utf8` are skipped without it.  When
collation, locale, or text comparison is in scope, install ICU and drop
`--without-icu`, or record that ICU paths were not exercised.

### 1. Debug / correctness build

Use this when you need local execution for correctness validation, reproduction, assertions, debugging, or most regression, TAP, and isolation work.

```bash
make distclean
./configure --enable-cassert --without-icu --enable-debug --enable-tap-tests \
  --prefix "$PG_INSTALL_DIR" \
  CFLAGS="-ggdb -O0 -g3 -fno-omit-frame-pointer"
make -j16 -s
make install
```

Use this build for:

- correctness validation,
- assertion failures,
- debugging,
- most regression, TAP, and isolation work.

`-O0` disables the flow analysis behind warnings such as
`-Wmaybe-uninitialized`.  A clean `-O0` build is not evidence that the
patch is warning-free; see "Compiler warnings" below.

### 2. Optimized benchmark build

Use this for `Perf` readiness and any performance claim.

```bash
make distclean
./configure --without-icu --prefix="$PG_INSTALL_DIR" CFLAGS="-O2"
make -j16 -s
make install
```

Use this build for:

- benchmark comparisons,
- latency and throughput measurements,
- avoiding debug/cassert noise.

### 3. Optimized benchmark + flamegraph build

Use this when the task explicitly requires profiling.

```bash
make distclean
./configure --without-icu --prefix="$PG_INSTALL_DIR" CFLAGS="-O2 -g -fno-omit-frame-pointer"
make -j16 -s
make install
```

### 4. Coverage build

Use this when deciding whether a new test adds coverage, or whether
existing tests already reach the changed code.

```bash
make distclean
./configure --enable-cassert --enable-debug --enable-tap-tests --enable-coverage \
  --without-icu --prefix "$PG_INSTALL_DIR"
make -j16 -s
make install
make coverage-clean
make -C src/test/regress check-tests TESTS="test_setup <test>"   # or any lane
make -C src/backend/optimizer coverage-html   # report for one subtree
```

The report is written to `coverage/index.html` under the directory where
`coverage-html` ran.  Coverage requires GCC with `gcov` and `lcov`; Apple
clang on macOS usually cannot produce it.  When coverage cannot be
measured locally, say so instead of claiming it.

### Compiler warnings

CI fails on compiler warnings at optimizing levels.  Before posting,
build once with optimization and `-Werror`, using the optimized profile:

```bash
make -j16 -s COPT='-Werror'
```

If pre-existing warnings unrelated to the patch break this build, record
them and compare the warning output against the base instead.

## Formatting Before Commit

Before every commit that changes C or header files, run `pgindent` on the
whole tree from the repository root, as `src/tools/pgindent/README`
prescribes:

```bash
src/tools/pgindent/pgindent .
```

`pgindent` needs the matching `pg_bsd_indent` for the current worktree. If
`pgindent` fails because `pg_bsd_indent` is missing, build the sibling tool
from the already configured PostgreSQL tree and point `pgindent` at it for this
run:

```bash
make -C src/tools/pg_bsd_indent

PGINDENT="$PWD/src/tools/pg_bsd_indent/pg_bsd_indent" \
  src/tools/pgindent/pgindent .
```

If the tree has not been configured yet, configure it with the build profile
chosen for the task before building `src/tools/pg_bsd_indent`.

Other files have their own formatters:

```bash
# Perl files; requires perltidy version 20230309
src/tools/pgindent/pgperltidy <changed .pl/.pm files>

# catalog .dat files
make -C src/include/catalog reformat-dat-files
```

Before every commit, regardless of file type:

```bash
git diff --check
```

Rules:

- keep `pg_bsd_indent` worktree-local by using the temporary `PGINDENT=...`
  command when the binary is not already suitable for the current worktree,
- if `pgindent` changes any file the commit does not touch, stop and find
  out why instead of committing or discarding the change,
- add each typedef the patch introduces to
  `src/tools/pgindent/typedefs.list` in alphabetical order; odd spacing
  such as `MyType * foo` after a run is the symptom of a missing one,
- check `git status` for `*.BAK` or `*.LOG` files left behind by a failed
  run,
- review the resulting diff instead of assuming it is harmless whitespace churn,
- if `pgindent` cannot be run, record the exact reason before handoff.

## Local Server Lifecycle

Lifecycle rule:

- if an agent run needs a local server for tests or validation, it must start the worktree-local cluster itself and stop that same cluster before handoff,
- do not rely on a previously running local postmaster,
- stop the cluster even when a test or validation command fails.

### Initialize a fresh cluster

```bash
"$PG_INSTALL_DIR"/bin/initdb -D "$PG_DATA_DIR"
```

### Start the server

```bash
"$PG_INSTALL_DIR"/bin/pg_ctl -D "$PG_DATA_DIR" \
  -l "$PG_LOG_DIR/postgres.log" \
  -o "-p $PGPORT" start
```

### Connect

```bash
"$PG_INSTALL_DIR"/bin/psql -h localhost -p "$PGPORT" postgres
```

### Stop the server

```bash
"$PG_INSTALL_DIR"/bin/pg_ctl -D "$PG_DATA_DIR" stop
```

## Validation Lanes

Use the smallest relevant lane first, then widen only when the blast radius requires it.
The `make check*` targets below use a temporary instance and need no
running cluster.

### Compile

```bash
make -j16 -s
```

### Selected regression tests

Include `test_setup` because most tests depend on the objects it creates.

```bash
make -C src/test/regress check-tests TESTS="test_setup <test> [<test> ...]"
```

### Core regression

```bash
make check
```

### Wider regression

```bash
make -j16 -s check-world
```

### Existing-installation regression

```bash
make installcheck
```

### TAP subset

Run from the directory that owns the tests; `PROVE_TESTS` paths are
relative to it.  Running it from a parent such as `src/bin` recurses into
every subdirectory and fails.

```bash
make -C src/bin/pg_dump check PROVE_TESTS='t/002_pg_dump.pl'
```

### Isolation tests

```bash
make -C src/test/isolation check
```

### Isolation tests against an installed server

```bash
make -C src/test/isolation installcheck
```

### Docs build

Validate first; it is much cheaper than rendering.

```bash
make -C doc/src/sgml check
make -C doc/src/sgml html
```

### CI on a personal fork

PostgreSQL CI runs on GitHub Actions for personal forks: autoconf and
meson builds, 32- and 64-bit, sanitizers, Windows, macOS, and
compiler-warning checks.  It catches what the local build does not, such
as files missing from `meson.build` or platform-specific breakage.

Enable it once per fork by creating the repository variable
`PG_CI_ENABLED` with value `1` (see `src/tools/ci/README`).  Push the
apply-check branch from "Preparing a Patch for Posting", so CI tests
exactly what will be posted, on current master:

```bash
git -C ../postgres-apply-check push origin HEAD:refs/heads/<topic>-v<N>
gh run list -R <owner>/<fork> --branch <topic>-v<N>
gh run watch -R <owner>/<fork> <run-id>
```

Pushing is outward-facing: ask the user before pushing.

## Proving a Test Fails Before the Fix

A new test is evidence only if it fails without the fix, for the expected
reason, and passes with it.  With the fix committed on the topic branch
(a temporary WIP commit is fine), revert only the non-test files,
rebuild, and run the new test:

```bash
BASE=<base of the topic branch, e.g. agent or upstream/master>
CODE_FILES="$(git diff --name-only --diff-filter=M "$BASE" HEAD -- . \
  ':(glob,exclude)src/test/**' ':(glob,exclude)**/sql/**' \
  ':(glob,exclude)**/expected/**' ':(glob,exclude)**/specs/**' \
  ':(glob,exclude)**/t/**')"

git checkout "$BASE" -- $CODE_FILES
make -j16 -s
<run the new test; it must fail, showing the symptom being fixed>

git checkout HEAD -- $CODE_FILES
make -j16 -s
<run the new test again; it must pass>
git status --short   # must show no leftover changes
```

Rules:

- do not use `git stash` for this; the stash is shared by all worktrees,
- if the patch adds code files, or the test depends on new code, build a
  separate worktree at `$BASE` and apply only the test changes there with
  `git diff "$BASE" HEAD -- <test paths> | git -C <base-worktree> apply`,
- record the failing output, such as the relevant hunk of
  `regression.diffs`, in the task log.

## Back-Branch Worktrees

Use these to establish which released branches a bug affects and whether
the fix applies there.  Supported branches are listed at
`https://www.postgresql.org/support/versioning/`.

```bash
git fetch upstream
git branch -r --list 'upstream/REL_*_STABLE'
git worktree add -b <topic>-<N> ../postgres-REL_<N> upstream/REL_<N>_STABLE
```

In the new worktree, rerun the runtime snippet so install, data, and port
derive from that worktree, then build with the debug profile, skipping
`make distclean` in the fresh tree.  Run the
reproducer before applying the fix, then apply the fix with
`git cherry-pick <commit>` and run it again.  Record, per branch: affected
or not, whether the fix applied cleanly, and the test results.

## Preparing a Patch for Posting

Patch files must contain only the topic commits, never agent workflow
files or notes, and must apply to current upstream master.  Generate them
from the topic branch's own base, then apply them to a temporary worktree
at upstream master.  This leaves the working branch untouched.

```bash
BASE=<base of the topic branch, e.g. agent or upstream/master>
TOPIC=<topic branch>
OUT=<output directory outside the source tree>

git fetch upstream
git log --oneline "$BASE..$TOPIC"        # only the patch commits
git diff --stat "$BASE...$TOPIC"         # only the intended files
git diff --check "$BASE...$TOPIC"
git format-patch -v<N> -o "$OUT" "$BASE..$TOPIC"

git worktree add --detach ../postgres-apply-check upstream/master
git -C ../postgres-apply-check am -3 "$OUT"/v<N>-*.patch
```

If `git am` fails, the patch no longer applies to master: update the base,
rebase the topic branch, rerun the affected lanes, and regenerate.  Keep
the apply-check worktree for the `CI` lane, then remove it with
`git worktree remove --force ../postgres-apply-check`.

## Benchmark Rule

This file defines benchmark build profiles, not the workload itself.
For `Perf` work, the task log must record the exact workload command separately, such as a specific `pgbench` invocation.

## Usage Rules

When using this file:

1. choose the build profile that matches the target readiness,
2. prefer the debug/cassert build for correctness and debugging,
3. prefer an optimized build for performance evidence,
4. state explicitly if a canonical command fails or must be adapted,
5. record the exact commands actually run in `notes/tasks/<slug>/log.md`.

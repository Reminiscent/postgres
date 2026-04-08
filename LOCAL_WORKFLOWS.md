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
- do not run `make clean` immediately after `make distclean`; the build-profile recipes below are reconfigure workflows, so `distclean` is the correct reset step on its own.

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

## Formatting Before Commit

Run `pgindent` from the repository root before every commit:

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

Rules:

- run this formatting pass before every commit, not only when you touched C files,
- keep `pg_bsd_indent` worktree-local by using the temporary `PGINDENT=...`
  command when the binary is not already suitable for the current worktree,
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

### Compile

```bash
make -j16 -s
```

### Core regression

```bash
make check
```

### Wider regression

```bash
make check-world
```

### Existing-installation regression

```bash
make installcheck
```

### TAP subset

```bash
make -C src/bin check PROVE_TESTS='t/001_test1.pl t/003_test3.pl'
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

```bash
make -C doc/src/sgml html
```

## Community and Literature Lookup

Network access is allowed. `.claude/settings.json` allows `WebSearch`
and `WebFetch` for every domain, so choose sources by judgment rather
than by an allowlist.

Principles:

- Prefer primary sources: PostgreSQL mailing-list archives, CommitFest,
  commits and their `Discussion:` links, and official docs; for external
  evidence, the paper itself and vendor documentation. Treat blogs, Q&A
  sites, and list mirrors as leads to a primary source, not as evidence.
- Use `WebSearch` to locate a source, then `WebFetch` to read it. Use
  `curl` with `grep` for large pages that only need filtering.
- When a site blocks automated access, look for another copy of the
  same primary source (arXiv, PVLDB, the conference site, an author
  page) instead of citing from memory. If none can be reached, record
  the claim as unverified.
- Record every query and URL actually used in the task's `log.md`.

Known pitfalls (verified 2026-11-12; re-check if behavior changes):

- Commit `Discussion:` links use `https://postgr.es/m/<message-id>`,
  which redirects across hosts; WebFetch does not follow it. Fetch
  `https://www.postgresql.org/message-id/<message-id>` instead, or
  `/message-id/flat/<message-id>` for the whole thread.
- Archive search:
  `https://www.postgresql.org/search/?m=1&q=<terms>&l=<list>&d=<days>&s=d`,
  where `l` is `1` (pgsql-hackers), `13` (pgsql-performance), `8`
  (pgsql-bugs), or `16` (pgsql-committers), `d=-1` means any time, and
  `s=d` sorts newest first.
- CommitFest search requires a login. Browse each commitfest page
  `https://commitfest.postgresql.org/<id>/` instead; it lists every
  entry and its status without a login, and the home page lists the
  recent IDs. To extract entry titles:

  ```bash
  curl -s https://commitfest.postgresql.org/<id>/ \
    | grep -oE 'href="/patch/[0-9]+/"[^>]*>[^<]+'
  ```

- `dl.acm.org`, `dblp.org`, `ieeexplore.ieee.org`, and the Semantic
  Scholar website block automated access; the Semantic Scholar API is
  often rate limited without a key.

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

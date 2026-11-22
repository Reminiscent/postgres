# Drafting test questions from recent PostgreSQL commits

Repo: /Users/yanchengpeng/project/postgres-arch (git checkout of PostgreSQL). Read-only: do not edit anything under the repo. Write only under /tmp/archmap/test3/q/.

Purpose: build exam questions whose answers are objectively known from git history, to test whether a reader who has NOT seen the code can predict where a change lands and why it takes its shape. The reader will see only `id` and `question`. Everything else is the answer key.

## Your subsystems
You are given three subsystem numbers. The files owned by subsystem <n> are listed in /tmp/archmap/own/<n>.txt (paths relative to src/backend/).

## Candidate pool (per subsystem)
    git log --since=2026-07-01 --until=2026-11-12 --no-merges --format='%h %ad %s' --date=short -- $(sed 's#^#src/backend/#' /tmp/archmap/own/<n>.txt)

## Selection rules (all must hold)
1. The commit changes between 1 and 6 `.c` files under src/backend (`git show --stat --format= <h> -- src/backend`). Skip mass refactors, renames, pgindent, typo/comment-only, docs-only, test-only commits, reverts, and version stamping.
2. The message body has at least 5 lines and states a problem or intent plus reasoning.
3. The first 9 hex digits of the hash do not occur anywhere under notes/arch-map/ (`grep -rl <h9> notes/arch-map`). If it occurs, skip the commit.
4. The commit hash (first 9 hex digits) must not appear in /tmp/archmap/inv/round1_hashes.txt (already used in an earlier round), and the commit must not be the fix or the introducing commit of any item in notes/arch-map/tests/round1/questions/*.json (do not read those files beyond grepping for the hash). Also not about these already-used topics: logical decoding of empty prepared transactions; self-join elimination with row-level security; hash index scans during bucket split; standby crash-restart "reached consistency" flag; composite datum attribute versioning (CVE-2026-6460) or ExecEvalFieldStore; the slot-based table AM index scan interface.
5. At least half of the changed src/backend `.c` files belong to the subsystem you are drafting for.

Pick 3 commits per subsystem (9 total). Prefer RISK items: for each subsystem first look for RISK-type commits (see below) and take up to 3; fill the remainder with WHY items whose bug is a violated correctness rule (data loss, wrong results, crash, hang, leak, stale cache, missed lock), not a cosmetic or performance issue. Do not use ALT in this round. If a subsystem has fewer than 3 qualifying commits, take what exists and say so in your report.

## Item types (field `type`)
Every item has part (a), the location question, and one part (b).

- RISK — only for a commit F whose message says it fixes a bug introduced or overlooked by an identified earlier commit X (phrases like "Oversight in commit X", "commit X failed to", "introduced by X", "Fallout from X"). Read X's own message (`git show -s X`).
  Scenario text: what X set out to do, taken from X's message (title and description), without mentioning F or the bug.
  (a) "A follow-up fix was needed. Which source files under src/backend did the follow-up fix modify? List up to 5, most likely first."
  (b) "What did the original change overlook: what breaks, under what conditions, and which correctness property is violated?"
  key_files = files changed by F. key_points come from F's message.
- WHY — scenario: the problem as described in the commit message (symptom, conditions), without revealing the fix.
  (a) "Which source files under src/backend does the fix modify? List up to 5, most likely first."
  (b) "What form does the fix take, and what constraint or reasoning drives that design?"
- ALT — only when the message explicitly names an alternative that was not chosen ("rather than", "instead of", "we could ... but", "considered"). Scenario as in WHY.
  (a) as in WHY.
  (b) "An obvious alternative would be <state the alternative>. Why was it not chosen?" (You may state what was chosen in one sentence if needed to make the question answerable.)

## Writing the `question` (English, 3–8 sentences plus the two parts)
- Describe the situation at the level a bug report or a design proposal would: user-visible behaviour, SQL commands, GUC names, catalog names, on-disk/wire concepts, and general PostgreSQL terms are fine.
- Do NOT include: names of source files or directories, names of C functions, structs, macros or variables taken from the diff or from the message's description of the fix; the commit hash; the author; anything that states the answer to (a) or (b).
- Do not copy the commit subject line verbatim if it names the fix.
- Keep it self-contained; the reader has no access to the repo.

## Answer key fields
- `key_files`: every file changed under src/backend or src/include by the commit (F for RISK), from `git show --stat --format= <h> -- src/backend src/include`, as repo-relative paths.
- `key_points`: 2 to 4 entries for part (b). Each entry: `point` (one sentence, your words) and `quote` (verbatim excerpt from the commit message, at most 3 lines, copied exactly from git output).
- `intro_commit` (RISK only): hash of X.
- `family`: which one of these invariant families the bug violates, as a string `I1`…`I8`, plus `family_reason` (one sentence): I1 durability (WAL before data, redo fidelity, committed data survives crash); I2 visibility (snapshots, freezing, pruning, xid/multixact horizons); I3 locks and ordering (lock levels, lock order, buffer locks/pins, races); I4 cache consistency (invalidation, stale relcache/catcache/plan cache); I5 resource lifecycle (leaks, use-after-free, cleanup on error/abort/exit); I6 semantic equivalence (planner/rewriter/executor transformations changing results); I7 process robustness (postmaster, crash restart, signals, child exit); I8 compatibility (on-disk/WAL/protocol/ABI/upgrade). If none fits, use `none`.

## Output
Write a JSON array to /tmp/archmap/test3/q/batch<K>.json (K is given to you). Each element:
    {"id": "B<K>-<two digits>", "subsystem": <n>, "commit": "<h>", "date": "YYYY-MM-DD", "type": "RISK|WHY",
     "question": "...", "key_files": ["src/backend/..."], "key_points": [{"point": "...", "quote": "..."}], "intro_commit": "<h or null>", "family": "I<n>", "family_reason": "..."}
Validate it: `python3 -I -c "import json,sys; d=json.load(open(sys.argv[1])); print(len(d))" /tmp/archmap/test3/q/batch<K>.json`.
Then run this self-check and fix any hit by rewording the question (not by changing the key):
for each item, no basename of any key_file (with or without `.c`/`.h`) may occur in its `question`.

## Report back (short; do not paste the questions)
- number of items per subsystem and per type;
- for each item one line: id, commit, type, number of key_files;
- commits you rejected under rule 3 (hash cited in the map), as a list of hashes;
- anything you could not do as specified.

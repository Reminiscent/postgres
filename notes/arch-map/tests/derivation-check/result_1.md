## C1-01  derivation.md:151
- (1) postmaster code cannot ereport(ERROR) because ERROR escalates to FATAL and postmaster exit stops the server — CORRECT — src/backend/utils/error/elog.c:381-395 ("1. we have no handler to pass the error to (implies we are in the postmaster ...)"; `if (PG_exception_stack == NULL || ExitOnAnyError ...) elevel = FATAL`); postmaster.c has no sigsetjmp/PG_exception_stack (grep empty); map I7.R8 row.
- (2) config reload errors are only logged and old value kept — CORRECT, slightly simplified — guc-file.l:137 (`elevel = IsUnderPostmaster ? DEBUG2 : LOG`); guc.c:615-620 messages "contains errors; unaffected changes were applied" / "no changes were applied". Only the offending settings keep old values; valid ones in the same file are applied. Optional fix: "出错的那几项保留旧值".
- CLARITY: "落点" (landing point for error recovery, i.e. the sigsetjmp handler) is unexplained jargon for a non-coder; consider "没有接住错误的异常处理位置".

## C1-02  derivation.md:253
- (1) holding exclusive content lock on a catalog page, must not call code that may read catalogs — CORRECT — bufmgr.c:4337-4343 ("A backend would self-deadlock on the content lock if the catalog scan read the exclusive-locked buffer"); map I3.R9.
- (2) such self-deadlock is undetectable — CORRECT — src/backend/storage/lmgr/README:22-23 (LWLocks: "There is no provision for deadlock detection").
- CLARITY: "同样检测不到" depends on the preceding text about LWLock; only works if reader has just read it. Also "任何可能读系统表的代码" is broader than the hazard (reading the same page); the next clause does say "同一页", so acceptable. bufmgr.c:4344-4351 notes non-catalog opclass reads are fine if the dependency graph is acyclic; the statement does not carry that exception (already in map R9 row).

## C1-03  derivation.md:345
- (1) Dec 2000 commit forbade signal-handler ERROR in critical sections — CORRECT — commit 65b362fae15 (2000-12-03) "Disable elog(ERROR|FATAL) in signal handlers in critical sections of code."
- (2) "当时的注释是 NO ELOG(ERROR) from here till changes are logged" — OVERSTATED (timing) — heapam.c comment already existed as context lines in 65b362fae15 diff (added earlier, `git log -S` earliest 1b67fe17b89 2000-07-03 "heap' logging"). Fix: "当时 heapam.c 里已有这样的注释".
- (3) Jan 2001: errors in critical section escalate to system reset, then called STOP — CORRECT — commit 6ce0ed2813d (2001-01-19) adds `if (CritSectionCount > 0) lev = STOP;` and miscadmin.h "causes any elog(ERROR) or elog(FATAL) to become elog(STOP) --- that is, a system-wide reset is forced".
- (4) renamed PANIC in 2002 — CORRECT — commit a033daf5663 (2002-03-02) "STOP => PANIC".
- (5) same commit splits holdoff vs critical section into two mechanisms — CORRECT — 6ce0ed2813d subject: "Make critical sections (elog->crash) and interrupt holdoff sections into distinct concepts".
- (6) mailing list: WAL's author thought erroring before logging must escalate for safety — HISTORY-UNVERIFIED — map principles.md:243 cites message-id 14110.979679509 (paraphrase, Vadim); statement itself has no URL, not fetched. Statement does mark it "（转述）".
- CLARITY: "整体重置" and "暂缓处理中断" are unexplained to a non-coder; "WAL 的作者" is anonymous and unverifiable by the reader.

## C1-04  derivation.md:372
- (1) WAL volume increases, and more so as checkpoints become more frequent — CORRECT — config.sgml:3610-3616 ("at the price of increasing the amount of data that must be written to WAL ... one way to reduce the cost of full-page writes is to increase the checkpoint interval parameters").
- (2) compensation is spreading checkpoint writes across the interval — OVERSTATED — Spreading (checkpoint_completion_target, checkpointer.c:775-781, wal.sgml:776-780 "spreads out the I/O ... checkpoint I/O load") smooths data-file write bursts; it does not reduce WAL volume. The doc's remedy for FPI WAL volume is lengthening the checkpoint interval. Map T1 (principles.md:110) says "把整页镜像的写入量摊到 checkpoint 间隔上", which is a different (and unsourced; evidence listed is wal.sgml:593-594,657-660 about async commit) statement; the derivation reworded it into checkpoint-write spreading. Fix: "补偿：把 checkpoint 间隔设长以减少首次修改的整页镜像次数；为了能设长而不造成 I/O 突发，checkpoint 写出被摊开（间接补偿）". Also D-checkpoint-spread has no decision record in principles.md (only a one-line entry at subsystems/9.md:192), so the cross-reference from 3.3 is dangling.
- CLARITY: "T1 中让步方的补偿" is an internal reference with no content here; a reader would not know who "让步方" is.

## C1-05  derivation.md:531
- (1) 2005 code comment: shared lock table is finite, users may lock many rows, so standard lock manager not used for long-term row locks — CORRECT — commit 93b24772782 (2005-04-30) heapam.c comment: "because the shared-memory lock table is of finite size, but users could reasonably want to lock large numbers of tuples, we do not rely on the standard lock manager to store tuple-level locks over the long term"; README.tuplock:5-8 same today.
- CLARITY: none (pointer [[D-row-lock-in-tuple-header]] is the decision itself, not the comment; fine).

## C1-06  derivation.md:549
- (1) map records no extra assumption for this decision — CORRECT — principles.md:197-211 record has no "假设" line (grep of "- 假设" in principles.md: 0 hits as line start; assumption appears only inline in other records, e.g. line 331); residuals.md has no X-entry for an assumption on it.
- (2) depends on premise E6 — CORRECT — principles.md:198 "前提：[[E6]]"; E6 row at principles.md:30, source README.tuplock:5-13 ("not possible to keep the locks objects in shared memory").
- CLARITY: "假设：地图没有…" is meta-talk about the map; a non-coder cannot tell what an "assumption" is here versus a "premise". Consider one clause saying what E6 states.

## C1-07  derivation.md:625
- (1) catalog rows roll back via MVCC; rolling back DDL makes the row versions invisible — CORRECT — S11.M01 Inv(1) (subsystems/11.md:111) "catalog 行靠 MVCC 回滚".
- (2) physical files are outside MVCC; pending list: delete new file at abort, delete old file only after commit — CORRECT — storage.c:42-51 ("create the physical file immediately, but remember it so that we can delete the file again if ... aborted ... a deletion request is NOT executed immediately ... When and if the transaction commits, we can delete the physical file"); I5.R19. Condition not stated: binary-upgrade backends delete old storage before commit (relcache.c:3880-3884 per S11.M01 Contract), and the list is only WAL-logged for non-temp relations.
- (3) "catalog 是普通表所以 DDL 事务性" causal link has no original text, recorded as assumption — CORRECT — principles.md:331 "这层因果在提交和读到的讨论里都没有原文（假设）".
- CLARITY: "挂到事务上" is vague; "待办清单" fine. Sentence ends without a period.

## C1-08  derivation.md:724
- (1) DP cost grows exponentially with join count — CORRECT (map marks it only as assumption, X22) — geqo.sgml:21-27 "The number of possible query plans grows exponentially with the number of joins"; geqo.sgml:34-40 normal optimizer "can take an enormous amount of time and memory space when the number of joins ... grows large".
- (2) above a threshold, semi-random search of join order — CORRECT — allpaths.c:3924 `else if (enable_geqo && levels_needed >= geqo_threshold) return geqo(...)`; optimizer/README:10-12 "semi-random search through the join tree space". Missing qualifier: only when enable_geqo is on; threshold counts FROM items (config.sgml:6707).
- (3) every join pair still handed to regular path generation — CORRECT — optimizer/README:12-14 "each join considered by /geqo is given to /path to create paths for".
- (4) cost: gives up optimality and determinism — OVERSTATED (determinism) — geqo.sgml:203-210: "inherently nondeterministic" but each run restarts RNG from geqo_seed, "As long as geqo_seed and the other GEQO parameters are kept fixed, the same plan will be generated for a given query". Optimality loss is supported (geqo.sgml:36 vs GEQO heuristic). Fix: "放弃最优性；计划带随机性，固定 geqo_seed 时可重复".
- Pointer: [[D-geqo]] is not a decision record in principles.md; it exists only as a one-line entry (subsystems/4.md:110) with original reason "未考证".
- CLARITY: "动态规划" and "连接对" unexplained; "每个连接对仍交给常规的路径生成" is opaque to non-coders.

## C1-09  derivation.md:830
- (1) docs say volatility category is "a promise to the optimizer" — CORRECT — xfunc.sgml:1617-1618 "The volatility category is a promise to the optimizer about the behavior of the function".
- (2) docs say B-tree operator family cross-type properties are ones "the planner relies on for optimization" — OVERSTATED — btree.sgml:195-200: only "The extensions to three or more data types within a family are not strictly required by the btree index mechanism itself, but the planner relies on them for optimization purposes"; two-type cross-type behavior is required by index searches themselves. Fix: "扩展到三种及以上数据类型的那些性质".
- CLARITY: quotes are Chinese translations presented in quotation marks as if verbatim; say "（译）".

## C1-10  derivation.md:908
- (1) 2014 commit: decodes WAL stream into logical changes (insert/update/delete with their transactions), works across schema changes, output format set by output plugin — CORRECT — commit b89e151054a (2014-03-03) "Introduce logical decoding.": "allows the write-ahead log stream to be decoded into a series of logical changes; that is, inserts, updates, and deletes and the transactions which contain them. It is capable of handling decoding even across changes to the schema ... The output format is controlled by a so-called 'output plugin'".
- CLARITY: labelled "原始原因" but the commit describes what the feature is, not why; the next sentence admits no reason was found. Suggest "原始说明（只讲做什么）". "输出插件" unexplained.

## Summary
| Verdict | Count |
|---|---|
| CORRECT | 20 |
| OVERSTATED | 4 |
| WRONG | 0 |
| MAP-ONLY | 0 |
| UNSUPPORTED | 0 |
| HISTORY-UNVERIFIED | 1 |

(Counts by claim bullets; C1-01 (2) and C1-07 (2) counted CORRECT with noted missing qualifiers.)

Blocks with any non-CORRECT verdict:
- C1-03 (OVERSTATED comment timing; HISTORY-UNVERIFIED mailing-list claim)
- C1-04 (OVERSTATED compensation; dangling D-checkpoint-spread)
- C1-08 (OVERSTATED determinism; D-geqo not a decision record)
- C1-09 (OVERSTATED cross-type scope)

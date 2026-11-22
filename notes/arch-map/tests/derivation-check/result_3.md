## C3-01  derivation.md:195
- (1) owners are built per transaction, subtransaction, portal — CORRECT — xact.c:1269 `ResourceOwnerCreate(NULL, "TopTransaction")`, xact.c:1325 (subxact), portalmem.c:207 (portal)
- (2) pins, locks, cache refs, files, DSM mappings are non-memory resources held by owners — CORRECT — dsm.c:159 `dsm_resowner_desc`, fd.c:365 `file_resowner_desc`; resowner/README:80 "buffer pins, lmgr locks, and catalog cache"
- (3) release in three phases, child before parent — CORRECT — resowner.h:54-56 BEFORE_LOCKS/LOCKS/AFTER_LOCKS; resowner.c:697 recursion into children (map: I5.R7, S12.M04 Inv(1))
- (4) subxact commit transfers locks to parent — CORRECT — resowner.c:~778 "if we are committing a subtransaction, we do NOT release its locks yet, but transfer them to the parent" (`LockReassignCurrentOwner`)
- CLARITY: "子先于父" holds per phase (whole tree finishes a phase before the next one); a non-coder may read it as "child fully done, then parent". Minor.

## C3-02  derivation.md:333
- (1) checkpoint must not set REDO between "WAL record written" and "state change made" — CORRECT — xact.c:1454-1458 "checkpoint to set REDO after the XLOG record but fail to flush the pg_xact update"
- (2) failure mode: recovery starts after redo point, skips the record, state not flushed — CORRECT — same comment; proc.h:118-124
- (3) applies to commit record -> clog update and to PREPARE WAL write — CORRECT — xact.c:1475 (DELAY_CHKPT_IN_COMMIT); twophase.c:1190-1208 (DELAY_CHKPT_START for PREPARE)
- CLARITY: "事务状态日志" is an unexplained alias for clog/pg_xact; "redo 点" presumably explained earlier.

## C3-03  derivation.md:345
- (1) Oct 2000 WAL commit added "force XLOG flush for buffer's LSN" on the buffer write path — CORRECT — `git show 5b0740d3fcd` (2000-10-28 "WAL") adds `XLogFlush(BufferGetLSN)` under comment "Force XLOG flush for buffer' LSN"; `git log -S` shows it as the earliest commit with that text
- (2) 2001 first WAL doc states the central concept — CORRECT — `git show 7b9dc714059` (2001-01-24): "WAL's central concept is that changes to data files ... must be written only after those changes have been logged - that is, when log records have been flushed to permanent storage"
- CLARITY: none.

## C3-04  derivation.md:504
- (1) tables without dead tuples still must be vacuumed periodically — CORRECT — maintenance.sgml:439-440 "it is necessary to vacuum every table in every database at least once every two billion transactions"
- (2) old unfrozen XIDs are what wraparound catches up with — CORRECT — maintenance.sgml:434-437 (past xids appear in the future), 443ff (freeze fixes it)
- (3) aggressive vacuum advances at least to FreezeLimit — CORRECT — vacuum.c:1123-1125 "An aggressive VACUUM must advance relfrozenxid up to FreezeLimit (at a minimum)"
- CLARITY: "回卷就会追上它" is a metaphor, and "被扫一次" slightly overstates (aggressive vacuum skips all-frozen pages); suggest "被 vacuum 一次". "冻结截止点" = FreezeLimit, fine if defined earlier.

## C3-05  derivation.md:540
- (1) xmax may be deleter, locker only, or multixact needing member lookup — CORRECT — heapam_visibility.c:1224-1245 (locked-only -> live), 1264 `HEAP_XMAX_IS_MULTI` -> `HeapTupleGetUpdateXid`
- (2) member status must follow in-progress-then-clog order, abort by exclusion — CORRECT — heapam_visibility.c:1274-1276 `TransactionIdIsInProgress(xmax)` before `TransactionIdDidCommit(xmax)`; file header 14-24
- (3) locked-then-updated, updater older than horizon, judged dead by updater — CORRECT — heapam_visibility.c:1276-1285 `*dead_after = xmax; return HEAPTUPLE_RECENTLY_DEAD` ("allow for pruning if below the xid horizon")
- (4) citation fit — CORRECT with note — I2.R2 supports (2); I2.R9 is the freeze rule, not multixact visibility; D-row-lock-in-tuple-header (I2 §2) is the better pointer. Pointer fix only.
- CLARITY: "视界" (horizon) and "在途" should be defined at first use if not already.

## C3-06  derivation.md:619
- (1) map independence note: catalog could use dedicated storage, so not derivable from 5.1 — CORRECT — principles.md:340 "独立性：不可由 [[D-catalog-driven-extensibility]] 推出，catalog 也可以用专门的存储"
- (2) no comparison with that option found; why plain heap is unverified — CORRECT — principles.md:330 "系统表为什么是普通表的原始原因，未考证"
- CLARITY: "独立性检查" and "地图" are internal-process jargon a non-coder cannot decode; suggest "地图记录显示这不是 5.1 的必然推论".

## C3-07  derivation.md:674
- (1) higher-level caches must register a callback for every catalog they depend on — OVERSTATED — I4.R9 (I4.md:38) states the rule but also lists an exception: "[[S02.M06]] Inv(5) 的函数缓存改用 pg_proc 的 xmin+tid 判定，不靠回调"; code agrees: funccache.c:548-549 `fn_xmin == ...RawXmin && ItemPointerEquals(fn_tid, t_self)`. Registration is also a per-module convention, not mechanically enforced (S11.M09 Inv(6) says only "高层缓存经回调失效"; plancache.c:156-166 registers a chosen set of syscaches). — Fix: "多数建在缓存之上的高层缓存要为…注册回调（函数缓存是例外，改用行版本比对）".
- CLARITY: "建在缓存之上的高层缓存" and "回调" unexplained in the bullet.

## C3-08  derivation.md:786
- (1) plan tree is a demand-pull tuple pipeline — CORRECT — executor/README:6-7 "The plan tree is essentially a demand-pull pipeline of tuple processing operations"
- (2) everything modified at run time lives in the state tree; read-only plan simplifies plan caching/reuse — CORRECT — executor/README:56-59 "all data that is modified during execution is in the state tree. Read-only plan trees make life much simpler for plan caching and reuse."
- CLARITY: "状态树" must have been introduced earlier (README: parallel tree of executor state nodes); otherwise unexplained.

## C3-09  derivation.md:901
- (1) logical slot lives on the primary; after promoting a physical standby the subscriber's slot is gone — CORRECT (inferred) — no doc sentence states the loss outright; implied by ddd5f4f54a0 message "synchronize the logical replication slots from the primary ... so that logical replication can be resumed after a failover" and logicaldecoding.sgml:~405-420 (slots only exist on standby if synchronized)
- (2) mechanism added in 2024 — CORRECT — `git show -s ddd5f4f54a0` 2024-02-14 "Add a slot synchronization function."; related c393308b69d (2024-01-25), 93db6cbda03 (2024-02-22)
- (3) quoted rationale — CORRECT — commit body: "...so that logical replication can be resumed after a failover or planned switchover" (Chinese rendering faithful)
- CLARITY: reads as if automatic; it needs opt-in (failover flag, primary_slot_name, hot_standby_feedback; logicaldecoding.sgml:409-420). "订阅方原来连的槽" ok.

## C3-10  derivation.md:960
- (1) quoted text from the 7.1-era backup doc — CORRECT — `git show a1642132192` (2000-06-30) backup.sgml: "the internal data storage format is subject to change between releases ... This does not apply to different 'patch levels', these always have compatible storage formats."
- (2) "earliest written statement" — CORRECT (limited search) — pre-2000 `git log -G` over doc/ found only unrelated "patch level" hits (9848d3655d4, 7095b43f336); search is not exhaustive.
- CLARITY: "2000 年 7.1 版" mixes dates: commit is 2000-06-30 during 7.1 development; 7.1 was released 2001-04-13 (REL7_1). Suggest "2000 年写入、随 7.1 发布的备份文档". Quote uses "……" and drops the "As a general rule" lead; fine.

## Summary

| Verdict | Count |
|---|---|
| CORRECT | 25 |
| OVERSTATED | 1 |
| WRONG | 0 |
| MAP-ONLY | 0 |
| UNSUPPORTED | 0 |
| HISTORY-UNVERIFIED | 0 |

Total claims: 26 across 10 blocks.

Blocks with a non-CORRECT verdict: C3-07 (derivation.md:674).
Pointer-only note: C3-05 cites I2.R9 (freeze rule) for multixact visibility; claim itself is correct.

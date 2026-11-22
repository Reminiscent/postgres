## I2.R1
ANSWER: Flush the commit WAL record first. Then mark clog committed (TransactionIdCommitTree). Only after that, remove the xid from ProcArray (ProcArrayEndTransaction, holding ProcArrayLock exclusive, which also advances latestCompletedXid and xactCompletionCount). Snapshot takers hold ProcArrayLock shared. Readers must check "in progress" (procarray/snapshot) before clog. With this order, anyone who no longer sees the xid in ProcArray is guaranteed to see it committed in clog. The reverse order opens a window where the xid is neither running nor committed, so it looks aborted.
CONFIDENCE: high

## I2.R2
ANSWER: "Not committed in clog" does not mean aborted. The transaction may still be running, or may be your own transaction (or a subxact). A crashed transaction is never marked aborted at all. Correct order: TransactionIdIsCurrentTransactionId → TransactionIdIsInProgress (or XidInMVCCSnapshot for snapshot semantics) → TransactionIdDidCommit → otherwise treat as aborted/crashed. Also, the xid must not be older than relfrozenxid/oldestClogXid, because clog for it may already be truncated.
CONFIDENCE: high

## I2.R3
ANSWER: A snapbuild/historic snapshot uses inverted semantics: xip holds committed xids, for catalog use. The xmin it needs is also not protected in the procarray. Handing it out directly gives wrong visibility, and vacuum may remove tuples it still needs. SnapBuildInitialSnapshot converts it into a normal MVCC snapshot by listing every xid in [xmin,xmax) that is not committed as xip. It errors if the result is too large. It sets MyProc->xmin under ProcArrayLock and checks that this xmin is not older than the safe decoding horizon / slot xmin, so the horizon holds while the snapshot is exported.
CONFIDENCE: medium

## I2.R4
ANSWER: The cached snapshot can be reused only if the shared xactCompletionCount still equals the value saved in the snapshot (GetSnapshotDataReuse). That means no xid-bearing transaction has finished since then. Any new path that removes xids from the running set (ProcArray or KnownAssignedXids) must increment xactCompletionCount while holding ProcArrayLock exclusive. If it does not, backends reuse stale snapshots that still show the finished xact as running, and visibility becomes inconsistent with the latest state.
CONFIDENCE: high

## I2.R5
ANSWER: PGPROC caches up to PGPROC_MAX_CACHED_SUBXIDS (64) subxids. Beyond that it sets the overflowed flag, and the snapshot becomes suboverflowed. Readers then map the subxid to its top-level xid via pg_subtrans (SubTransGetTopmostTransaction) and check the top xid against the snapshot. Ordering: the subxid's parent must be recorded in pg_subtrans (SubTransSetParent) before the xid can appear in any tuple or WAL. The subxid must also be published in the cache, or the overflow flag set, before it is used (with memory barriers). pg_subtrans must not be truncated past the oldest xmin.
CONFIDENCE: medium

## I2.R6
ANSWER: The horizon must be conservative: no existing snapshot, and none that could still be taken, may need the removed data. That means it is ≤ the xmin and xid of every running backend, including xids not yet in any snapshot, bounded above by latestCompletedXid+1. Sources: procarray xmins/xids (including prepared xacts and walsender hot_standby_feedback xmin), replication slot xmin, and slot catalog_xmin (for catalog/user_catalog tables). It is computed separately for shared catalogs, catalogs, regular data and temp tables (GlobalVisState / ComputeXidHorizons). Index page recycling compares the page's safexid against the same horizon.
CONFIDENCE: high

## I2.R7
ANSWER: Exclusion is correct only if the operation never needs other transactions to retain dead tuples. It must not read data through an MVCC snapshot whose results depend on its xmin, and must not run user code (expressions or partial-index predicates that could access other tables). PROC_IN_SAFE_IC tried this for simple CIC/REINDEX CONC, but it was found to cause corruption and was reverted (14.4). Long operations like CIC scan the table with a snapshot and validate against it. If vacuum removes tuples (or HOT chains) they still need, the index misses or wrongly includes entries.
CONFIDENCE: medium

## I2.R8
ANSWER: Order: compute OldestXmin/cutoffs first, then read rel_pages, then get vistest (GlobalVisTestFor). Pages added after the cutoff contain only xids ≥ OldestXmin, so skipping them is safe for relfrozenxid. Each tuple's liveness is decided exactly once per page: prune computes HeapTupleSatisfiesVacuum and lazy_scan_prune/freeze reuse that result. Recomputing could give a different answer as the horizon moves or an inserter aborts. Example: a tuple judged RECENTLY_DEAD and then DEAD becomes inconsistent with freezing and VM decisions. That race was the old retry-loop bug.
CONFIDENCE: medium

## I2.R9
ANSWER: Freezable: xmin committed and older than the cutoff (visible to all), with xmax invalid, aborted, or a lock-only finished locker. Multixacts are processed via FreezeMultiXactId. Do not freeze xids ≥ OldestXmin or aborted inserters (those tuples are dead). A committed deleter older than the cutoff means the tuple must be removed, not frozen; otherwise vacuum reports "found xmin/xmax from before relfrozenxid". New relfrozenxid/relminmxid must be ≤ every unfrozen xid/mxid left in the table. That is valid only if every page not marked all-frozen was scanned. The values never move backward, and freezes must be WAL-logged before the in-place pg_class update.
CONFIDENCE: medium

## I2.R10
ANSWER: datfrozenxid is the minimum across all databases. Truncation goes up to that point only. First advance the oldest-valid xid (AdvanceOldestClogXid) and WAL-log the truncate, then remove segments, so concurrent lookups never read removed pages. After that, update the wrap/stop limits (SetTransactionIdLimit, SetMultiXactIdLimit); those limits must never let new xids reach truncated or reused space. pg_subtrans is truncated separately at checkpoint, to the oldest xmin. Anyone storing xids outside heap (e.g. the NOTIFY queue) must either hold back the truncation horizon or freeze/drop those xids before clog truncation. Otherwise later DidCommit lookups fail or wrap.
CONFIDENCE: medium

## I2.R11
ANSWER: Setter: verify every tuple on the page is visible to all (frozen as well, for all-frozen). Hold the heap buffer lock. Set PD_ALL_VISIBLE on the heap page and the VM bits in one critical section, WAL-logged with a conflict horizon xid for standby. Clearer: any change that makes a tuple not visible to all (insert/update/delete/lock) must clear PD_ALL_VISIBLE and the VM bits in the same critical section and WAL record as the change. It does this while holding the heap buffer lock, with the VM page pinned beforehand. All-frozen implies all-visible: frozen is never set without visible, and clearing visible clears both. A lock-only xmax clears only all-frozen.
CONFIDENCE: high

## I2.R12
ANSWER: Prune turns a dead root line pointer that indexes still reference into LP_DEAD; it cannot become LP_UNUSED, so the TID stays reserved. Non-root heap-only members can become UNUSED, and a live chain root becomes LP_REDIRECT. Order: first remove the index entries for those TIDs (ambulkdelete), then turn LP_DEAD into LP_UNUSED in the heap. The reverse order lets the TID be reused and indexes point to the wrong tuple. A deleted B-tree page is stamped with safexid (the next full xid at deletion). It is recycled, via FSM, only after safexid is older than the global removable horizon, when no scan can still land on it.
CONFIDENCE: high

## I2.R13
ANSWER: At each step, check that the next item is a normal line pointer, and that the next tuple's xmin equals the previous tuple's update xmax (TransactionIdEquals(HeapTupleHeaderGetXmin, priorXmax)). For HOT chains, also check it is heap-only. On mismatch the chain is broken: the slot was pruned and reused. Treat it as the end of the chain (the version is gone / no newer version) and do not use that tuple. The chain start has no prior xmax to compare against, so this check cannot be applied there. The start TID must come from a trusted source (index entry, or a caller holding a valid TID).
CONFIDENCE: high

## I2.R14
ANSWER: Based on running_xacts decoded from WAL: LogicalIncreaseXminForSlot records candidate_catalog_xmin together with an LSN. Once the client confirms flush past that LSN (LogicalConfirmReceivedLocation), data.catalog_xmin is updated. Order: first save the slot to disk (ReplicationSlotSave), then set effective_catalog_xmin and recompute the global required xmin. After a crash, the persisted value must not be newer than the horizon actually honored. The historic snapshot cannot be used as a normal MVCC snapshot. It has inverted (committed-list) semantics and is valid only for catalog / user_catalog_table tables, and the horizon is not held for normal tables.
CONFIDENCE: high

## I2.R15
ANSWER: The standby's running set must include every xid that may be running on the primary. It is built from RUNNING_XACTS records plus every xid seen in WAL, including gaps up to latestObservedXid, subxids (via pg_subtrans when overflowed) and prepared xacts, all kept in KnownAssignedXids. Read-only connections are accepted only after reaching a consistent point (minRecoveryPoint) and standbyState = SNAPSHOT_READY (not overflowed, or all overflowed xacts have finished). On promotion: first set SharedRecoveryState to DONE, so new snapshots stop using KnownAssignedXids. Only then call ShutdownRecoveryTransactionEnvironment to discard KnownAssignedXids, and do it after recovering prepared xacts.
CONFIDENCE: medium

## I2.R16
ANSWER: Before replay, the standby must resolve recovery conflicts (ResolveRecoveryConflictWithSnapshot). After max_standby_*_delay it cancels queries whose snapshot xmin ≤ snapshotConflictHorizon. On the primary, the record's snapshotConflictHorizon must be ≥ the newest xid among the removed tuples (the latest removed xmax/xmin), i.e. conservative. Then any standby snapshot that could still see those tuples is detected as conflicting.
CONFIDENCE: high

## I2.R17
ANSWER: When the index was built, broken HOT chains were indexed under the newest version's key. Older snapshots may need older versions whose key differs, so through this index they would get wrong results. In that case indcheckxmin is set. The planner skips the index while the pg_index row's xmin is not older than TransactionXmin, and marks the plan transient so it is replanned. CIC sets each flag in its own committed transaction, with a cache invalidation. indisready is set only after waiting out lockers that might not know about the index. indisvalid is set only after build and validate, and after waiting for older snapshots (WaitForOlderSnapshots).
CONFIDENCE: high

## I2.R18
ANSWER: After COMMIT the snapshot is released and nothing holds xmin. Vacuum can remove the toast rows, so detoasting later fails with "missing chunk number 0 for toast value...". With no active snapshot at all, the fetch errors with "cannot fetch toast data without an active snapshot". Whoever holds a toast pointer must detoast/flatten the value before the snapshot goes away (PL/pgSQL detoasts fetched values for that reason), or keep a registered snapshot that protects xmin. Bare toast pointers must not be kept across transactions.
CONFIDENCE: high

## I2.R19
ANSWER: READ COMMITTED: wait for the concurrent transaction. If it aborts, update the original row. If it commits, follow the ctid chain to the newest version, lock it, and re-check quals with EvalPlanQual. If the row still qualifies, update the new version. If it was deleted or no longer matches, skip the row. REPEATABLE READ/SERIALIZABLE: wait as well. If the other transaction commits, raise a serialization failure ("could not serialize access due to concurrent update", SQLSTATE 40001); the client must retry. If it aborts, continue.
CONFIDENCE: high

## I2.R20
ANSWER: Take SIREAD locks on: tuples read (PredicateLockTID), and the index pages or key gap covering the scanned range so phantoms are caught (an AM without amcanpredlock locks the whole index relation). A sequential scan locks the whole relation. Timing: lock while still holding the buffer content lock, before returning the result, so a concurrent inserter's CheckForSerializableConflictIn sees the lock. After commit the locks are not released immediately; they last until every overlapping transaction finishes. False positives are allowed (granularity promotion, summarization, page-level locks). False negatives are not.
CONFIDENCE: high

## I2.R21
ANSWER: This is allowed only for non-transactional state, where it is acceptable that the write survives if the transaction aborts. Examples: statistics/hints that do not affect correctness, monotonically advancing values (relfrozenxid, sequence values with gaps), or state with its own protection (relmapper written with WAL plus lock in a commit critical section). It must not change any row's visibility or identity, and must not change tuple length. It must hold the proper lock against lost updates (inplace-update lock) and be WAL-logged when crash/standby needs it. Anything other sessions must see atomically with the transaction's result must not use this route.
CONFIDENCE: medium

## I2.R22
ANSWER: Workers inherit from the leader: the transaction snapshot and active snapshot, combo CIDs, the current command id, the top xid and transaction state, GUCs, user / security context, search_path, relmapper and pending-sync state, uncommitted enums, reindex state, and so on. In parallel mode, writing tuples (no inserts/updates/deletes, except special cases like parallel index build/vacuum), assigning new xids (any xid must be assigned before entering parallel mode), CommandCounterIncrement, subtransactions and other xact-state changes are all forbidden.
CONFIDENCE: high

## I2.R23
ANSWER: On receiving sinval messages (AcceptInvalidationMessages/catcache or relcache invalidation), the catalog snapshot is invalidated (InvalidateCatalogSnapshot): unregistered, set to NULL, and its xmin released. The next catalog scan calls GetCatalogSnapshot, takes a fresh GetSnapshotData, and registers it, so later catalog changes become visible.
CONFIDENCE: high

## I7.R1
ANSWER: Postmaster must not take spinlocks or LWLocks on shared memory. A crashed child may have corrupted shmem or left a lock held, and then postmaster would hang or crash, losing the ability to do crash recovery. Design exchanged data so postmaster reads it lock-free and defensively: flags/slots with memory barriers and in_use/generation fields, and validate everything read. Keep authoritative state in postmaster-private memory (e.g. BackgroundWorkerList). Children notify postmaster via PMSignal plus SIGUSR1.
CONFIDENCE: high

## I7.R2
ANSWER: Postmaster inspects the waitpid status. Exit 0, or exit 1 for regular backends, is a normal/FATAL exit. Death by signal or any other code counts as a crash and triggers HandleChildCrash. A child that reached shmem but did not mark its PMChild slot inactive is also treated as crashed. A direct exit(0) skips proc_exit / on_shmem_exit cleanup: locks and PGPROC are not released and the child slot is not marked inactive. Postmaster sees this, treats it as a crash, and resets the whole cluster.
CONFIDENCE: medium

## I7.R3
ANSWER: If the handler _exit(1)s while the process holds the spinlock, the lock is stuck forever. Postmaster treats exit(1) as a normal FATAL exit, not a crash, so no reset happens. Other processes then spin until "stuck spinlock" PANIC or hang. Do not leave async _exit active in that window. Either block those signals / disable the timeout around the spinlock section, or move cancel forwarding until after normal signal handling is set up, or use exit code 2 so it counts as a crash.
CONFIDENCE: medium

## I7.R4
ANSWER: Handlers run asynchronously. The interrupted code may hold LWLocks, be inside malloc, or be halfway through updating a data structure. proc_exit/ereport (longjmp) are not async-signal-safe and can deadlock or corrupt state. A handler may only: save/restore errno, set volatile sig_atomic_t flags, and SetLatch(MyLatch). The main loop / CHECK_FOR_INTERRUPTS then does the actual cleanup or exit. The exception is SIGQUIT, which just calls _exit(2).
CONFIDENCE: high

## I7.R5
ANSWER: Respond immediately with _exit(2) and no cleanup (no on_shmem_exit), because shmem may be corrupt. Signals must not stay blocked indefinitely during or after error recovery; sigsetjmp(...,1) restores the unblocked mask. Interrupts are deferred through HOLD_INTERRUPTS rather than by blocking SIGQUIT, so postmaster's crash-reset request is never stuck. Exit code 2 (not 0 or 1) tells postmaster this is an abnormal exit, so it is handled as part of the crash/reset and not as a normal FATAL.
CONFIDENCE: medium

## I7.R6
ANSWER: The old segment may be corrupted, which may be the reason for the crash. Its pointers or contents are untrustworthy, so copying them carries the corruption into the new cluster, or crashes postmaster. Rebuild only from postmaster-private inputs: shmem_request_hook / registration lists kept in postmaster memory, GUCs, and static configuration. Recompute sizes, allocate a fresh segment, and initialize from scratch.
CONFIDENCE: high

## I7.R7
ANSWER: Before fork, allocate a PMChild slot from that type's pool (enforcing limits) and record type and bookkeeping; after fork, record the pid. If fork fails, log it, release the slot and entry, and do not treat it as a crash. Retry later (bgworker restart interval) or tell the client "could not fork". When the pool is exhausted, the caller must fail gracefully: reject the connection with "too many clients", or leave the worker unstarted and retry later. Never ERROR/FATAL in postmaster.
CONFIDENCE: medium

## I7.R8
ANSWER: Postmaster has no transaction / error-recovery context, so an ERROR there is effectively promoted to FATAL. Postmaster exits and the whole running server goes down. Report the problem at LOG/WARNING level (the elevel passed by ProcessConfigFile / load_hba etc.), return failure, and keep the old valid configuration.
CONFIDENCE: high

## I7.R9
ANSWER: Without WL_POSTMASTER_DEATH/WL_EXIT_ON_PM_DEATH, the process sleeps on as an orphan, still attached to shmem. A new postmaster then cannot start because the old shmem is still in use, and the process may linger forever. Use WL_EXIT_ON_PM_DEATH, or WL_POSTMASTER_DEATH and exit when it fires (WaitLatch asserts that one of them is given for under-postmaster processes).
CONFIDENCE: high

## I7.R10
ANSWER: No. Only crash/immediate shutdown escalates SIGQUIT to SIGKILL after a timeout. Fast shutdown and promotion send SIGTERM and wait indefinitely. So blocking waits must be interruptible: use non-blocking sockets plus WaitLatchOrSocket (or async libpq), and run CHECK_FOR_INTERRUPTS / check the shutdown flag in the loop. Otherwise shutdown or promotion hangs.
CONFIDENCE: high

## I7.R11
ANSWER: After a crash reset (ResetBackgroundWorkerCrashTimes), BGW_NEVER_RESTART workers, and those with the terminate flag set, are forgotten / unregistered. Other workers get rw_crashed_at reset and are restarted immediately. To stop a worker while letting its restart policy decide whether it returns, signal its process (SIGTERM / kill the pid) rather than setting terminate via TerminateBackgroundWorker, which removes it permanently.
CONFIDENCE: medium

## I7.R12
ANSWER: No. When the startup process fails on its own (not signaled by postmaster), recovery itself has failed, for example due to bad WAL or a missing file. A reset and retry would hit the same error again, an endless restart loop. Postmaster logs "aborting startup due to startup process failure" and exits (ExitPostmaster(1)). It restarts only if it had signaled the startup process itself as part of another child's crash (StartupStatus = SIGNALED).
CONFIDENCE: high

## I7.R13
ANSWER: The forked child shares the parent's process-local state. proc_exit would run the parent's on_shmem_exit/on_proc_exit callbacks: releasing the parent's locks/PGPROC, marking the child slot, deleting files. That corrupts the shared state of the still-live parent. The handler must first check MyProcPid != getpid(); if so, restore the default action and re-raise or _exit, without running PG logic. After fork, any PG-managed fork must update MyProcPid (and the related process globals) immediately.
CONFIDENCE: high

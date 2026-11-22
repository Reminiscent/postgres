## I2.R1
ANSWER: Set clog to COMMITTED (TransactionIdCommitTree, after the commit WAL record is flushed) first, and only then clear the xid from the PGPROC/ProcArray in ProcArrayEndTransaction under ProcArrayLock EXCLUSIVE. Snapshot takers scan ProcArray under ProcArrayLock SHARED, so once the xid disappears from ProcArray, its clog status is already committed; reversing the order lets a snapshot see the xid as neither running nor committed.
CONFIDENCE: medium

## I2.R2
ANSWER: pg_xact returns "not committed" for both in-progress and aborted xacts, and a crashed xact may still be marked IN_PROGRESS in clog, so treating "not committed" as "aborted" is wrong for a live xact. Correct flow: check the clog status; if it is IN_PROGRESS, check whether the xid is still running in ProcArray (TransactionIdIsInProgress); if not running, treat it as aborted (crash leftover); for subxacts, resolve the top-level parent status as well.
CONFIDENCE: medium

## I2.R3
ANSWER: The snapbuild snapshot is a historic catalog snapshot built only from decoded commits; it is not a current view of running xacts, so it is not valid as a general MVCC snapshot for ordinary heap tables and its xmin may not be protected, so vacuum can remove rows it needs. Handle it by exporting only at a consistent point (SNAPBUILD_CONSISTENT), with the slot's xmin/catalog_xmin held back to cover it, and restricting it to catalog/decoding visibility (HeapTupleSatisfiesHistoricMVCC) or to a snapshot importable only through the exporting slot's session.
CONFIDENCE: low

## I2.R4
ANSWER: A cached snapshot is reusable only while no xact has completed or changed the running set since it was taken; GetSnapshotData checks this via the completion counter (xactCompletionCount) under ProcArrayLock. The new path must, under ProcArrayLock EXCLUSIVE, remove the xid from ProcArray and increment xactCompletionCount (and update latestCompletedXid as needed); otherwise a stale cached snapshot keeps treating the finished xact as in progress, giving wrong visibility.
CONFIDENCE: medium

## I2.R5
ANSWER: Other backends rely on pg_subtrans: when the PGPROC subxid cache overflowed (suboverflowed flag in the snapshot), they walk the parent chain via SubTransGetTopmostTransaction to decide whether a subxid is still in progress. The subxid's parent entry in pg_subtrans must be written before the subxid is published (added to the PGPROC cache / included in the XLOG_XACT_ASSIGNMENT record, with a memory barrier), so readers never see the subxid without its parent link.
CONFIDENCE: medium

## I2.R6
ANSWER: The cleanup horizon must be conservative: a tuple or catalog row may be removed only if no current or future snapshot can still see it, so the horizon is the oldest xmin that any consumer could still use. It must combine the xmin of all backends (PGPROC/ProcArray, including those of other databases with a shared-catalog view), replication slots' xmin and catalog_xmin, hot standby feedback xmin, prepared transactions, and the vacuum's own snapshot/held snapshots.
CONFIDENCE: medium

## I2.R7
ANSWER: Setting PROC_IN_VACUUM is correct only if the process never needs its snapshot to keep rows alive for other operations, i.e., it reads/writes only data that the horizon does not protect and it holds no xid that others depend on. This is not okay for CREATE INDEX CONCURRENTLY-style operations, because they scan the whole table with an old snapshot and must still see tuples that concurrent transactions delete; excluding them lets vacuum remove those tuples and produces a corrupt index.
CONFIDENCE: medium

## I2.R8
ANSWER: Take OldestXmin (the vacuum horizon) before reading the relation's block count and before creating the visibility test object used by pruning, so that any page appended after the size read holds only tuples newer than the horizon and the horizon is not computed later than the visibility state it is tested against. Per page, each tuple's liveness is determined exactly once (HeapTupleSatisfiesVacuum once per tuple, result stored), because the status can change between calls and the counts and index entries would otherwise disagree.
CONFIDENCE: medium

## I2.R9
ANSWER: Only tuples whose xmin is committed and visible to all (older than the freeze cutoff, not inserted by an in-progress or aborted xact) may be frozen; xmax/multixact that is still needed or still in progress must be left alone. After vacuum, relfrozenxid may advance only to a value no newer than the oldest unfrozen xid remaining in the table, and it must not go below the freeze cutoff; relminmxid follows the same rule for multixacts.
CONFIDENCE: medium

## I2.R10
ANSWER: The order is: advance datfrozenxid in pg_database (only after all the database's relations have advanced relfrozenxid), WAL-log the new oldest-xid information, truncate the pg_xact/pg_multixact/pg_subtrans segments below the new limit, and only then update the xid wraparound limit (SetTransactionIdLimit). Any xid stored outside heap (such as NOTIFY queue entries or extension files) must be covered by the same horizon or frozen/ handled first, or truncation makes those xids unlookupable.
CONFIDENCE: low

## I2.R11
ANSWER: Vacuum may set all-visible/all-frozen only when every tuple on the page is truly visible to all (and frozen for all-frozen), done with the page lock and WAL-logged with the page LSN/ checksum consistent, and any modification must clear the VM bits within the same locked, WAL-logged change before the page change is visible. The all-frozen bit depends on all-visible: all-frozen may only be set when all-visible is also set, since a page with unfrozen tuples cannot be frozen-only.
CONFIDENCE: medium

## I2.R12
ANSWER: Prune must keep a dead root line pointer that index entries still reference as LP_DEAD (storage of the tuple removed, but the pointer kept), not set it to LP_UNUSED. The order is: delete the index entries first (vacuum phase I, bulk delete), then mark the heap line pointers LP_UNUSED (phase II). A B-tree deleted page can be reused only after its safe-xid (deletion xid) is older than the global removable horizon, so no in-progress scan can still follow a link to it.
CONFIDENCE: medium

## I2.R13
ANSWER: At each step, check that the next tuple's xmin equals the previous tuple's xmax (and the line pointer is still a normal, non-redirected, non-unused item); if not, the chain was pruned or the slot reused, so stop following it and restart from the index/start TID or treat the chain as ended. The starting TID cannot be checked this way; it is validated by fetching it and rechecking that the tuple is still the one the index pointed to.
CONFIDENCE: low

## I2.R14
ANSWER: The new catalog_xmin is decided by the snapshot builder from the oldest catalog snapshot still needed, using the slot's confirmed-flush/restart information. The new value must be made durable in the slot on disk before the effective (in-memory, horizon-visible) value is advanced, so that after a crash vacuum has not removed catalog rows the restarted decoder needs. The historic snapshot used by decoding cannot scan ordinary tables with normal MVCC; it applies only to catalog scans through HeapTupleSatisfiesHistoricMVCC.
CONFIDENCE: low

## I2.R15
ANSWER: The standby's KnownAssignedXids must include every xid that was running on the primary at the consistent point (from the replayed running-xacts record), and read-only connections are accepted only after the standby reaches that consistent point (and minimum recovery point). At promotion, the recovery-only xid tracking (KnownAssignedXids) should be discarded while the recovery state is still in effect, before switching the shared state to normal (non-recovery) operation.
CONFIDENCE: low

## I2.R16
ANSWER: Before replaying the prune record, the standby must cancel any conflicting queries whose snapshot xmin is at or below the record's latestRemovedXid (ResolveRecoveryConflictWithSnapshot), since those queries may still need the removed tuples. On the primary, the record's latestRemovedXid must be the newest xid among all tuples removed by that prune, so the standby's conflict check covers every removed tuple.
CONFIDENCE: medium

## I2.R17
ANSWER: An index created or rebuilt by a transaction may lack entries for tuples visible to an older snapshot, so it is not valid for snapshots taken before its creation: the planner checks indcheckxmin against the snapshot's xmin and skips the index if it is too new. For concurrent builds, indisready must be set and all transactions with older snapshots must finish before the build; indisvalid is set only after the build and after waiting for all transactions that could still use an older snapshot, with separate commits between these phases.
CONFIDENCE: medium

## I2.R18
ANSWER: After COMMIT, the snapshot protecting the toast rows is gone, so vacuum can remove the toast chunks and detoasting fails with a missing-chunk error or returns garbage; detoasting without an active snapshot has the same risk. The holder must detoast (copy the value into local memory) before COMMIT, or hold an explicit snapshot covering the toast rows for as long as the pointer is used.
CONFIDENCE: medium

## I2.R19
ANSWER: Under READ COMMITTED, if the target row was concurrently updated and the concurrent xact committed, re-check the WHERE clause against the newest version (EvalPlanQual) and continue updating the new version; if it aborted, use the next version; if it is still in progress, wait for it. Under REPEATABLE READ or SERIALIZABLE, raise a serialization failure (SQLSTATE 40001, could not serialize access due to concurrent update) and abort the transaction.
CONFIDENCE: high

## I2.R20
ANSWER: SIREAD locks must be taken on the index pages (or relation) the AM reads and on the heap tuples/pages/relation the scan reads, before the data is read, and re-checked after acquiring so a concurrent writer's conflict check sees them. They cannot be released immediately at commit when overlapping serializable xacts may still be running; they are kept until no conflicting serializable xact remains. False positives (coarser granularity, promotion) are allowed; false negatives (missing a lock) are not.
CONFIDENCE: medium

## I2.R21
ANSWER: Only state that does not need transactional semantics (advisory data such as planner stats, or state with its own crash-safe, atomic, checksummed update such as the relation map file, fsynced and replaced as a whole) may be overwritten in place or stored in a separate file. Anything that must roll back with the xact or be consistent with other MVCC data must go through WAL and MVCC instead.
CONFIDENCE: medium

## I2.R22
ANSWER: Parallel workers must inherit the leader's transaction snapshots (the transaction and active snapshots), the combo-CID mapping, and the leader's transaction/session state (GUCs, user/session identity, relevant transaction state) so they see the same rows. In parallel mode, workers may not write, assign xids, or start subtransactions; only the leader may write, and attempts to write or assign xids in a worker raise an error.
CONFIDENCE: medium

## I2.R23
ANSWER: On receipt of sinval messages that may affect it, the backend must invalidate its cached catalog snapshot (InvalidateCatalogSnapshot), because the catalog state it captured may be stale. The next catalog scan then takes a fresh catalog snapshot (via GetCatalogSnapshot), not the invalidated one.
CONFIDENCE: high

## I7.R1
ANSWER: The postmaster must not take spinlocks in shared memory, because a child that dies holding the spinlock would leave it locked and the postmaster (which does no crash recovery itself) could deadlock. Postmaster-to-child data should be exchanged through its own private state or the PMSignal/child-flags area designed to use atomics and signals with no lock, and reset by the crash-reset path.
CONFIDENCE: medium

## I7.R2
ANSWER: The postmaster decides from the exit status: a clean exit (status 0, or the normal startup-failure code for non-shmem children) gets no reset, while signal death or the special crash exit code (2, used by quickdie) triggers a crash reset of all children. A backend that touched shared memory and called exit(0) makes the postmaster treat it as a clean exit, so held locks or half-updated shared state are not cleaned up and the server can keep running on corrupt state; it must call proc_exit() instead, which runs the on_shmem_exit callbacks.
CONFIDENCE: low

## I7.R3
ANSWER: The process is not yet attached to shared memory during startup-packet reading, and a SIGTERM/timeout handler that calls _exit while holding the spinlock leaves it locked forever, and a crash-reset kill can leave it held too. Do not take spinlocks in the pre-init handler; forward cancel requests only after the backend has set up its PGPROC and shared state, and make the handler async-signal-safe.
CONFIDENCE: medium

## I7.R4
ANSWER: A signal handler must only set volatile sig_atomic_t flags and call async-signal-safe functions (e.g., SetLatch); calling proc_exit or ereport from it runs non-reentrant code (locks, malloc, elog, shmem cleanup) in the middle of arbitrary code and can deadlock or corrupt state. Do the real cleanup in the main loop when the flag is checked (ProcessInterrupts / CHECK_FOR_INTERRUPTS).
CONFIDENCE: high

## I7.R5
ANSWER: The backend must respond to SIGQUIT immediately with _exit(2) and no cleanup, since shared memory may be corrupt; the exit code 2 tells the postmaster to do a crash reset. During error recovery the signal mask must be restored to the unblocked state that was in effect when sigsetjmp was set, so SIGQUIT is not blocked; leaving signals blocked after recovery would ignore the reset request.
CONFIDENCE: medium

## I7.R6
ANSWER: The old segment's request array is gone or meaningless after the crash, because the new segment is freshly created and its contents are not reliable; reading it yields garbage or stale sizes and causes a bad layout. Reconstruction must be based on the postmaster's process-local registrations (the shmem request hooks/requests recorded in private memory when the modules were loaded), run again in the postmaster.
CONFIDENCE: medium

## I7.R7
ANSWER: Before fork, the postmaster reserves a child slot from the pool and records the child in its accounting; if fork fails, it releases the slot and undoes that accounting, then reports the failure and retries later rather than crashing. When the slot pool is exhausted, the caller must not assume success; it should defer or back off and retry, or fail the request, per its own policy.
CONFIDENCE: medium

## I7.R8
ANSWER: In postmaster, ereport(ERROR) is escalated to a fatal error, so a bad config on reload would take down the postmaster and therefore the whole server. Report it with ereport(LOG) (or WARNING) and keep the previously loaded configuration in effect, so the running server continues on the old, valid settings.
CONFIDENCE: high

## I7.R9
ANSWER: If the postmaster is SIGKILLed, no SIGTERM is sent to children, so the process blocked in WaitLatch waits forever and becomes an orphan. Use WaitLatch with the WL_EXIT_ON_PM_DEATH flag (detecting postmaster death via the postmaster-death pipe) so it exits as soon as the postmaster is gone.
CONFIDENCE: high

## I7.R10
ANSWER: No: fast shutdown and promotion use SIGTERM only, and postmaster SIGKILLs only on immediate shutdown or crash reset, so a process blocked in a socket write may never exit. Blocking waits must be interruptible: use WaitLatchOrSocket (including the socket and the latch) and check for interrupts in the loop, instead of blocking send()/recv() indefinitely.
CONFIDENCE: medium

## I7.R11
ANSWER: After crash reset, BGW_NEVER_RESTART workers are removed; other registered workers are restarted by the postmaster according to their restart_time policy. To stop one but let its restart policy decide, send it SIGTERM (a normal exit, so the postmaster applies restart_time) rather than setting the terminate flag, which makes it permanent and never restarted.
CONFIDENCE: medium

## I7.R12
ANSWER: No: an unexpected startup-process exit is treated as fatal, because the startup process is recovering the database and resetting and retrying would normally reproduce the same failure. The postmaster reports it and shuts the server down instead of doing a crash-reset retry, since the failure is in recovery itself.
CONFIDENCE: medium

## I7.R13
ANSWER: The child runs PG's signal handler and calls proc_exit, which runs on_proc_exit/shmem cleanup in a process that is a copy of the postmaster-side state, releasing or touching locks/shared state it does not own and flushing stdio buffers twice, causing corruption. The handler must be reset to SIG_DFL in the child, and before exec only async-signal-safe calls should be made; after fork, the cached PID-related state (MyProcPid, and related process-local caches) must be updated immediately.
CONFIDENCE: medium

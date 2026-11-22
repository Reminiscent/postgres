## I3.R1
ANSWER: Insert takes the buffer content lock in EXCLUSIVE mode (pin held, RowExclusiveLock on the relation). Pruning/VACUUM need a cleanup lock (exclusive content lock plus pin count == 1), because defragmentation moves tuples and line pointers and other backends holding a pin with a TID must not see the move. Pruning opportunistically uses ConditionalLockBufferForCleanup and skips the page if it cannot get it.
CONFIDENCE: high

## I3.R2
ANSWER: The relation lock is taken when the RTE is added (parser) or when rewriting expands a view or rule (AcquireRewriteLocks), held until end of transaction. The plancache re-takes locks in AcquireExecutorLocks when a cached plan is reused, and invalidation rebuilds the plan if it changed. The planner and executor open with NoLock because the lock was already acquired earlier, and sinval processing after locking keeps the cached object consistent.
CONFIDENCE: medium

## I3.R3
ANSWER: The committer must queue its sinval messages before releasing its AccessExclusiveLock (sent at commit, before locks are released). The lock taker, after LockRelationOid returns, calls AcceptInvalidationMessages so caches are flushed before reading catalogs. Name lookup (RangeVarGetRelidExtended) must re-check after locking that the name still maps to the same OID, and retry if it was renamed or dropped.
CONFIDENCE: medium

## I3.R4
ANSWER: Check the new mode against every lock mode it conflicts with (concurrent DML like RowExclusive and AccessShare, VACUUM's ShareUpdateExclusive, other DDL, and self-conflict). Changes that rewrite the heap, change the tuple format or column storage, or otherwise alter what running scans read must keep AccessExclusiveLock, because concurrent queries hold an old relcache/tuple descriptor and would read inconsistent data.
CONFIDENCE: low

## I3.R5
ANSWER: The callback runs before the lock is acquired, so if it takes a weaker lock and the main code then needs a stronger one, two sessions each hold the weak lock and wait to upgrade, which deadlocks. The fix is to lock at the final mode the command needs (the lockmode passed to RangeVarGetRelidExtended), with the callback only doing permission checks and not taking its own lock.
CONFIDENCE: medium

## I3.R6
ANSWER: Acquire locks in one fixed global order: parent before child (top-down for partitioned trees), table before its indexes, and within a level by OID or the same sorted order used by object addresses. Any new multi-relation path should follow the existing order of DETACH and DROP so it cannot wait on another DDL in the opposite order.
CONFIDENCE: low

## I3.R7
ANSWER: The creator takes a shared lock on each referenced object before recording the pg_depend entry, then re-checks that the object still exists, erroring if it is gone. DROP takes an exclusive lock on the object before scanning pg_depend for dependents. The two serialize, so either the dependency is recorded and DROP sees it, or DROP wins and the creator fails cleanly.
CONFIDENCE: medium

## I3.R8
ANSWER: Two backends taking the same two LWLocks in opposite order can deadlock. PostgreSQL has no deadlock detector for LWLocks (only heavyweight locks are detected), so the backends hang. Acquire multiple LWLocks in a documented fixed order (for buffers, ascending block number), or use ConditionalLockBuffer for out-of-order cases and back off on failure.
CONFIDENCE: high

## I3.R9
ANSWER: Syscache lookups and building invalidation messages can read other catalog pages, take buffer locks, allocate memory, or even ereport, so doing them under an exclusive content lock can self-deadlock or hang. Do the lookup and build the invalidation messages before taking the buffer lock (or after releasing it), then do only the page modification under the lock.
CONFIDENCE: medium

## I3.R10
ANSWER: An elog(ERROR) longjmps out of the critical section and leaves the spinlock held forever, and a system call may sleep, which a spinlock must never do. Each SpinLockAcquire must be paired with exactly one SpinLockRelease, the lock is not recursive, and only short, non-blocking code may run between them.
CONFIDENCE: high

## I3.R11
ANSWER: No, the earlier state cannot be used directly: after releasing and re-acquiring, the page may have changed (free space, tuples, LSN, or pruning), so re-check and retry. Only the pin is kept across the release. While holding the content lock, do no I/O or waits on other locks that can block for long. Read the page header without the lock only as a hint.
CONFIDENCE: medium

## I3.R12
ANSWER: The deadlock detector cannot see a wait that goes through a network connection or a condition variable, so P and Q hang forever. Waits should appear as heavyweight lock waits the detector can see (or as LWLock/latch waits with wait events and timeouts), and inter-process waits should never be hidden behind a heavyweight lock held by the waiter.
CONFIDENCE: medium

## I3.R13
ANSWER: The primary takes AccessExclusiveLock and WAL-logs a standby lock record, so the startup process acquires the AccessExclusiveLock on behalf of that xid and standby SELECTs wait behind it or get a recovery-conflict cancel. Ordinary standby backends simply take their normal AccessShareLock and wait. The standby releases the lock when the primary's commit or abort record for that xid is replayed.
CONFIDENCE: medium

## I3.R14
ANSWER: VACUUM must remove index entries for the dead TIDs before it frees the heap line pointers, so the TID cannot be reused while an index still points to it. The btree leaf-page cleanup lock (LockBufferForCleanup) waits for any scan holding a pin on that leaf to drop it, which guarantees the scan has finished using the old TIDs. MVCC scans normally drop their pin early, so they rarely block VACUUM.
CONFIDENCE: medium

## I3.R15
ANSWER: The page can be reused only after every snapshot that could still reach it is gone, i.e., its recorded deletion XID (safexid) is older than the oldest xmin/horizon of all running backends (GlobalVis removable check). Reuse before that point could let an in-flight scan follow a link into an unrelated page.
CONFIDENCE: medium

## I3.R16
ANSWER: At commit the WAL commit record is flushed, clog is set, the xid is removed from ProcArray, sinval messages are already queued, and only then are heavyweight locks released (ResourceOwner release). A subtransaction commit does not release locks; they pass to the parent owner. PREPARE TRANSACTION transfers locks to the prepared transaction's dummy PGPROC so they survive until COMMIT or ROLLBACK PREPARED.
CONFIDENCE: medium

## I3.R17
ANSWER: The waiter resets its latch first, then checks the condition, then waits, so a wakeup between check and sleep is not lost. The waker updates shared state first, then calls SetLatch. Signal handlers may only set volatile sig_atomic_t flags and call async-signal-safe functions like SetLatch; they must not elog, palloc, take LWLocks, or do other non-reentrant work.
CONFIDENCE: high

## I3.R18
ANSWER: Temp buffers are backend-private (local buffer manager), so the path must reject another session's temp relation (RELATION_IS_OTHER_TEMP) with the usual "cannot access temporary tables of other sessions" error. Checking only the read path lets prefetch or bulk extend touch the shared pool or the wrong local buffers, giving wrong data or corruption.
CONFIDENCE: medium

## I3.R19
ANSWER: Release held LWLocks first (LWLockReleaseAll), then run the DSM detach/cleanup callbacks. If the segment is unmapped while the process still holds an LWLock inside it, that lock memory disappears and other backends wait forever or read unmapped memory. Reversing the order risks hangs or crashes.
CONFIDENCE: medium

## I6.R1
ANSWER: Do not overwrite the child Var's nullingrels with the parent's. Translate the parent's varnullingrels to child relids and merge (union) them with the child Var's existing markers, which may come from an outer join inside the child subquery. Overwriting drops those markers and gives wrong NULL-extension results.
CONFIDENCE: medium

## I6.R2
ANSWER: Trust only a NOT NULL constraint on the base relation's own column that is validated and applies to this Var with no nullingrels, so it is not null-extended by an outer join. For inherited children, each child's own constraint must be checked, and a clause cloned by an outer-join transform must not be assumed non-null from the original.
CONFIDENCE: low

## I6.R3
ANSWER: Volatile or set-returning expressions cannot go into a multi-member EC, since they may not be treated as equal. Constants can join an EC only under the EC's opfamily/collation and strictness rules. Membership equality is defined by the EC's btree opfamily and collation, and members from the nullable side of an outer join are not assumed equal above that join.
CONFIDENCE: low

## I6.R4
ANSWER: Canonical PathKeys cannot be built until all ECs are finalized, meaning qual and join EC merging for the query is complete and ORDER BY/GROUP BY members are added. Building them earlier can compare against stale or unmerged ECs, which gives duplicate or missing pathkey equivalence and suboptimal or wrong ordering.
CONFIDENCE: low

## I6.R5
ANSWER: Not safe unless the equality used in the proof matches the consumer's: same opfamily and same collation (a nondeterministic collation changes equality). Check the unique index's opclass, collation, and opfamily against the join clause's operator and collation, and check the index is not partial.
CONFIDENCE: medium

## I6.R6
ANSWER: Check volatility after preprocess_expression, because inlining of SQL functions can hide volatile calls. Volatile expressions must keep their evaluation count and per-row evaluation, so the transformation must not duplicate, drop, or move them across row contexts (e.g., outer joins or aggregates).
CONFIDENCE: low

## I6.R7
ANSWER: A HAVING qual can move to WHERE only if it references only grouping columns, with no aggregates, volatile functions, or grouping-set-nulled columns. A qual can move below a window function only if it references only the partition-by columns common to all window functions. A qual on a column that grouping sets can NULL cannot be pushed below them.
CONFIDENCE: medium

## I6.R8
ANSWER: No, a non-leakproof user qual must not move before RLS or security-barrier quals, because it could see protected rows. Selectivity estimation must not call user operators on pg_statistic values for such quals either. For inheritance, permissions come from the parent RTE in the query, while each child's own RLS policies apply to that child.
CONFIDENCE: low

## I6.R9
ANSWER: Do not assume strictness. IS NOT DISTINCT FROM returns TRUE for NULL inputs, so the negation shortcut and null-rejection reasoning are invalid. Such operators must be treated as non-strict and the transform skipped (outer-join simplification needs null-rejecting, strict quals).
CONFIDENCE: medium

## I6.R10
ANSWER: After the first match for an outer row, the executor may stop scanning inner rows and move to the next outer row. This is valid only when the inner side is unique on the join keys so no further match exists. For right anti joins the matched-flag bookkeeping must still cover all inner rows, so the shortcut must not be applied there.
CONFIDENCE: low

## I6.R11
ANSWER: Move B's quals onto A by substituting A's Vars and drop any qual that becomes equal() to an existing one (same operator and collation). Replace or remove all B references: Vars in the targetlist, ECs, PlaceHolderVars, and relids in cached results must be updated or invalidated.
CONFIDENCE: low

## I6.R12
ANSWER: Mark the rel or path parallel-restricted so it gets no partial path, because a parallel-restricted expression cannot run in workers. An initPlan attached to a parallel-safe node makes that node restricted in the same way. Partial paths must be wrapped in a Gather or Gather Merge node before they can become an executable plan.
CONFIDENCE: medium

## I6.R13
ANSWER: The leader must pass GUC state, user/session identity, search_path, the transaction snapshot, combo CIDs, and related environment to workers through parallel serialization. Writes and new xid assignment are not allowed in parallel mode (IsInParallelMode errors), so workers must not write or allocate an xid.
CONFIDENCE: medium

## I6.R14
ANSWER: No, the cached analysis and rewrite results cannot be reused directly when search_path, role, or row_security changes, because name resolution, permissions, and RLS depend on them. Plancache records these settings and re-parses or re-analyzes (RevalidateCachedQuery) when they differ.
CONFIDENCE: medium

## I6.R15
ANSWER: Batching is not allowed when BEFORE ROW triggers, volatile defaults, or volatile WHERE functions are present. Each row must be processed, evaluated, and inserted in order, so triggers and volatile functions see the correct per-row state. Batching would reorder or reuse side effects.
CONFIDENCE: low

## I6.R16
ANSWER: Treat pg_statistic and extended stats as untrusted hints. Validate types, array sizes, and bounds, and clamp selectivities to [0,1] so no crash occurs. Bad stats can give bad plans (cost only) but must never change query results, so no correctness decision may depend on them.
CONFIDENCE: medium

## I6.R17
ANSWER: The planner and executor assume collations are assigned, so these expressions would fail with "could not determine which collation to use" or get the wrong collation. The parser transform that builds the expression is responsible for calling assign_expr_collations (or assign_query_collations) before handing it on.
CONFIDENCE: medium

### B01
- (1) SUPPORTED — lwlock.c HOLD_INTERRUPTS comment and lmgr README both say cancel/die are held off while holding LWLock/spinlock until released — "Acquisition of either a spinlock or a lightweight lock causes query cancel and die() interrupts to be held off until all such locks are released." (lmgr/README:39-41)

### B02
- (2) UNCHECKABLE — the line carries no anchor.

### B03
- (1) SUPPORTED — shmem.c lifecycle: all request_fn run first, total size known, one global segment allocated, then init_fn called — "Only after all the request_fn callbacks have been called ... we know how much shared memory we need in total." (shmem.c:88-90); ipci.c:130-136 CalculateShmemSize -> PGSharedMemoryCreate -> ShmemInitRequested

### B04
- (1) SUPPORTED — origin.c says origin LSN is stored in commit record, allows precise replay state after crash without synchronous commit; xact.c:1496-1499 advances origin with XactLastRecEnd, 6297-6301 recovers progress in redo — "That allows to recover the precise replayed state after crash recovery; without requiring synchronous commits." (origin.c:33-34)

### B05
- (empty) UNCHECKABLE — the block contains only "Inv:" with no claim text and no anchor.

### B06
- (3) SUPPORTED — comment states page must not be undergoing IO to set hint bit, otherwise checksum corruption — "the page must not be undergoing IO at this time (otherwise we e.g. could corrupt PG's page checksum" (heapam_visibility.c:106-107)

### B07
- (2) SUPPORTED — in the brin update branch the new tuple is added and brinSetHeapBlockItemptr(revmap) done inside one critical section, and one XLogInsert(RM_BRIN_ID, XLOG_BRIN_UPDATE) registers new page, revmap page, old page — "recptr = XLogInsert(RM_BRIN_ID, info);" (brin_pageops.c:~288, after XLogRegisterBuffer(1, revmapbuf, 0))

### B08
- (1) OVERSTATED — modulo-2^32 compare (transam.h:268-269), wrapLimit = oldest_datfrozenxid + (MaxTransactionId >> 1) (varsup.c:384) and refusal at xidStopLimit are supported, but refusal is conditioned on IsUnderPostmaster (standalone backend is not refused, varsup.c:389 "in interactive mode") and xidStopLimit = wrapLimit - 3M, which the claim omits — "if (IsUnderPostmaster && TransactionIdFollowsOrEquals(xid, xidStopLimit))" (varsup.c:138-139)

### B09
- (1) SUPPORTED — physical sender reads only flushed WAL; on a cascading standby limit is GetStandbyFlushRecPtr (replayed, or received if streaming), else GetFlushRecPtr — "Read up to MAX_SEND_SIZE bytes of WAL that's been flushed to disk" (walsender.c:3390); "SendRqstPtr = GetStandbyFlushRecPtr(&SendRqstTLI);" (walsender.c:3451)

### B10
- (3) SUPPORTED — README says vacuum never deletes tuples or pages from entry tree, empty posting-tree pages are deleted under full cleanup lock on the root, which blocks concurrent insertions — "Vacuum never deletes tuples or pages from the entry tree." (gin/README:391); "This lock prevents all the concurrent insertions into this tree" (gin/README:399-400)

### B11
- (1) SUPPORTED — sync.c:48-50 non-temp only, never fsync temp rels nor postpone deletion; fd.c:3989 PANIC when data_sync_retry off; sync.c:577-578,600-604 retryOnError loop with no CHECK_FOR_INTERRUPTS (XXX); the type mapping (SYNC_REQUEST retryOnError=false with local fsync on failure; UNLINK/FORGET true) is not in the anchored lines but is at md.c:1527,1575,1589 (outside anchor); the crash "(假设)" part is not checkable and the datadir-fsync-before-recovery clause has no anchor here — "XXX should we CHECK_FOR_INTERRUPTS in this loop?" (sync.c:600)

### B12
- (1) OVERSTATED — new TLI only for archive recovery, crash recovery extends current TLI (xlog.c:6647,6656), history written only at end of recovery with no locking (timeline.c:321), contiguity failure is ERROR (timeline.c:583), pre-switch crash leaves standbys thinking switched (xlog.c:6686-6688); but "I/O failure ERROR (FATAL in startup)" is only partly shown: ERRORs exist in timeline.c:349-541 outside the anchor and nothing anchored shows the FATAL-in-startup escalation, and "standby-written history files come from the primary" has no anchor — "elog(ERROR, "timeline history was not contiguous");" (timeline.c:583)

### B13
- 原始 SUPPORTED — commit e6faf910d75 says cached plan can generate custom parameter-dependent plans, chooses between custom and generic, and is about mechanism not policy — "all about getting the mechanism in place, not the policy."
- 当前 SUPPORTED — plancache.c header says choice logic is in choose_custom_plan — "The logic for choosing generic or custom plans is in choose_custom_plan," (plancache.c:11)

### B14
- 原始 UNCHECKABLE — claim is "未考证" (not researched), no checkable assertion.
- 当前 SUPPORTED — xact.c says no XLOG flush at abort since post-crash default is abort, and for the same reason no checkpoint interlock; clog.c and heapam_visibility.c say the same / crashed-in-progress xacts are found by elimination — "We do not flush XLOG to disk here, since the default assumption after a crash would be that we aborted, anyway." (xact.c:1835-1836)

### B15
- 原始 SUPPORTED — commit message gives tens of MB, up to twice a second, blocking new stats, and says entries cannot leak via transactional drop — "These files can reach tens of megabytes and are written out up to twice a second." (commit 5891c7a8ed8)
- 当前 SUPPORTED (corrected lines 44-46) — anchor :43 is off, the statement is at pgstat.c:44-46 — "flushed to shared memory ... This practically eliminates contention on individual stats entries." (pgstat.c:44-47)

## Counts
| Verdict | Count |
|---|---|
| SUPPORTED | 13 |
| OVERSTATED | 2 |
| CONTRADICTED | 0 |
| ANCHOR-MISS | 0 |
| UNCHECKABLE | 3 |

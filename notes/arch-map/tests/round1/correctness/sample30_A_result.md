### A01
- (1) SUPPORTED — commit and README both say varnullingrels records outer joins that may null the Var so equal() Vars can be trusted — "This allows us to trust that equal() Vars represent the same value." (2489d76c490); "Var nodes are decorated with "varnullingrels", which are sets of the rangetable indexes of outer joins that potentially null the Var" (src/backend/optimizer/README:343-345)

### A02
- (1) SUPPORTED — inside the elevel>=ERROR branch, CritSectionCount>0 forces PANIC — "if (CritSectionCount > 0)" / "elevel = PANIC;" (elog.c:375-376)
- (2) SUPPORTED — no handler, ExitOnAnyError, or proc_exit_inprogress turns ERROR into FATAL — "if (PG_exception_stack == NULL || ExitOnAnyError || proc_exit_inprogress)" (elog.c:392-395)
- (3) SUPPORTED — the stacked error levels are max-ed into elevel so FATAL/PANIC is not downgraded — "elevel = Max(elevel, errordata[i].elevel);" (elog.c:407)
- (4) SUPPORTED — header comment says recovery code must PG_RE_THROW or abort (sub)transaction — "Failure to do so may leave the system in an inconsistent state for further processing." (elog.h:337-338)
- (5) SUPPORTED — volatile requirement stated for variables modified in PG_TRY and used in PG_CATCH — "must be declared "volatile" for POSIX compliance." (elog.h:374-376)
- (6) SUPPORTED — comment says FATAL bypasses the construct and recommends PG_ENSURE_ERROR_CLEANUP — "an ereport(FATAL) will not be caught by this construct; control will exit straight through proc_exit()." (elog.h:365-366)
- (7) SUPPORTED — stack size is 5 and exceeding it PANICs — "ereport(PANIC, (errmsg_internal("ERRORDATA_STACK_SIZE exceeded")));" (elog.c:789; ERRORDATA_STACK_SIZE 5 at elog.c:154)
- (8) SUPPORTED — for elevel==ERROR it zeroes the counters, leaves CurrentMemoryContext at ErrorContext, and calls PG_RE_THROW (siglongjmp to PG_exception_stack); printing/popping left to handler — "Printing it and popping the stack is the responsibility of the handler." (elog.c:532)

### A03
- (1) SUPPORTED — md.c header describes full + one partial + inactive 0-size segments, kept because other backends may hold fds — "The reason for leaving them present at size zero, rather than unlinking them, is that other backends and/or the checkpointer might be holding open file references" (md.c:60-62)
- (2) SUPPORTED — register_dirty_segment forwards a sync request and fsyncs locally if the queue is full; the skipFsync exclusion is at the callers in the same file (md.c:539, 654, 1158) — "if (!RegisterSyncRequest(&tag, SYNC_REQUEST, false /* retryOnError */ ))" (md.c:1527)
- (3) SUPPORTED — comment gives exactly this scenario, first segment truncated, extra segments truncated then unlinked, other forks removed immediately — "Leaving the empty file in place prevents that relfilenumber from being reused." (md.c:288-289)
- (4) SUPPORTED — bulk_write.c covers temp/unlogged/minimal/normal-WAL cases, including DELAY_CHKPT_START and the self-fsync after a concurrent checkpoint — "MyProc->delayChkptFlags |= DELAY_CHKPT_START;" (bulk_write.c:200)
- (5) SUPPORTED — smgrnblocks_cached uses cached values only in recovery and truncation sends CacheInvalidateSmgr — "if (InRecovery && reln->smgr_cached_nblocks[forknum] != InvalidBlockNumber)" (smgr.c:853); "The smgr cache inval message that this function sent will cause other backends to invalidate their copies of smgr_cached_nblocks" (smgr.c:909-911)
- (6) SUPPORTED — comment says interrupts are held across most functions because procsignal processing can trigger smgrreleaseall() and code is not reentrant — "NB: We need to hold interrupts across most of the functions in this file" (smgr.c:43)

### A04
- (1) SUPPORTED — header says rewrite preserves visibility info and update chains, with ctid substituted via old/new tid mapping — "preserving visibility information and update chains." (rewriteheap.c:6-7); "We then add an entry to the old_new_tid_map hash table showing B's original tid (in the old heap) and new tid" (rewriteheap.c:~52-53)

### A05
- (2) SUPPORTED — GiST README states child-before-parent, left-to-right ordering for multi-page locking — "child page before parent, and left-to-right at the same level." (gist/README:263-264)

### A06
- (1) SUPPORTED — slot is written to disk (ReplicationSlotSave) before effective_catalog_xmin is advanced and before required xmin/LSN are recomputed — "Now the new xmin is safely on disk, we can let the global value advance." (logical.c:2014-2015); "MyReplicationSlot->effective_catalog_xmin = MyReplicationSlot->data.catalog_xmin;" (logical.c:2022)

### A07
- (1) SUPPORTED — enable: logical WAL info then barrier wait, then decoding enabled and status record written; disable: reverse order and no wait — "Unlike when enabling logical decoding, we don't need to wait for all processes to complete it in this case." (logicalctl.c:654-655, in the function ending at 663; wait on enable at 442)

### A08
- (no claims) the block has an empty "Inv:" line; nothing to check.

### A09
- (3) SUPPORTED — AfterTriggerEndSubXact on abort restores query_depth, truncates the deferred event list and restores SET CONSTRAINTS state (trigger.c:5486-5560) — "discarding any events queued by the subxact." (trigger.c:5539); "Restore the trigger state." (trigger.c:5543)

### A10
- (1) SUPPORTED — AfterTriggerEndQuery fires immediate-mode events and moves deferred ones to the global list — "we invoke all AFTER IMMEDIATE trigger events queued by the query, and transfer deferred trigger events to the global deferred-trigger list." (trigger.c:5173-5174)

### A11
- (1) SUPPORTED — all six parts check out as one Contract claim: backend may set terminate while in_use (bgworker.c:65-68), lockless protocol with write barrier (bgworker.c:52-63), exit status 0/1 not a crash with other statuses setting crashed (postmaster.c:2625-2632, HandleChildCrash called just after at ~2680), TerminateBackgroundWorker sets terminate (bgworker.c:1329-1332), bgworker_should_start_now switches on pmState (postmaster.c:4266-4285) — "even when the slot is in use, regular backends may set the 'terminate' flag for a slot" (bgworker.c:65-66); "if (!EXIT_STATUS_0(exitstatus) && !EXIT_STATUS_1(exitstatus))" (postmaster.c:2631)

### A12
- (1) SUPPORTED — all parts check out as one Contract claim: bulk_write says unlogged rels are fsync'd at the shutdown checkpoint (bulk_write.c:149-155); InRecovery covers non-clean shutdown or archive recovery (xlogrecovery.c:874-886); CLEANUP before hot standby connections (xlog.c:6473-6479); reinit redoes cleanup if recovery crashed before INIT (reinit.c:359-363); INIT after recovery completes (xlog.c:6621-6629) — "if recovery crashes before we get to doing UNLOGGED_RELATION_INIT, we'll redo the cleanup step too at the next startup attempt." (reinit.c:360-363)

### A13
- (1) SUPPORTED — all parts check out as one Contract claim: write skipped if PageIsNew or !DataChecksumsNeedWrite (bufpage.c:1524-1525), PageIsVerified flag semantics (bufpage.c:80-94), share-exclusive lock requirement (bufpage.c:1508-1509), HOLD_INTERRUPTS during verify (bufpage.c:110-117), FSM reads use RBM_ZERO_ON_ERROR (freespace/README "Every read here uses RBM_ZERO_ON_ERROR to bypass checksum mismatches"), standby hint bits wait for FPI (page/README:61-66) — "If the page is in shared buffers, it needs to be locked in at least share-exclusive mode." (bufpage.c:1508-1509)

### A14
- (1) UNCHECKABLE — the 原始 part says only "未考证" (not researched) and asserts nothing about code or history — quote: none (map text "原始: 未考证")
- (2) SUPPORTED — README says /geqo does a semi-random search instead of exhaustive and each join pair still goes to /path; allpaths.c dispatches to geqo() at the threshold (also conditioned on enable_geqo and no join_search_hook) — "it does a semi-random search through the join tree space, rather than exhaustively considering all possible join trees." (optimizer/README:11-12); "else if (enable_geqo && levels_needed >= geqo_threshold)" (allpaths.c:3924)

### A15
- (1) SUPPORTED — 原始: commit moved lock acquisition into AcquireRewriteLocks, saves duplicate parser/rewriter lock grabs, locks before rewriter decisions, linear-time fix of dropped JOIN columns — "move the responsibility for acquring locks on relations referenced by rules into this separate function (which I therefore chose to call AcquireRewriteLocks)" (ba42002461a)
- (2) SUPPORTED — 当前: skip for just-parsed trees, required for trees produced earlier; plancache calls it on the copy; caution about copyObject; stored-rule JOIN aliases may reference dropped columns — "But we must do this whenever we are dealing with a querytree produced earlier than the current command." (rewriteHandler.c:130-131); "(If the column is not explicitly referenced anywhere else in the query, the dependency mechanism won't consider it used by the rule and so won't prevent the column drop.)" (rewriteHandler.c:135-137)

| Verdict | Count |
|---|---|
| SUPPORTED | 27 |
| OVERSTATED | 0 |
| CONTRADICTED | 0 |
| ANCHOR-MISS | 0 |
| UNCHECKABLE | 1 |
| (A08: no claims) | 0 |

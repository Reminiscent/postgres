## I1.R1
ANSWER: Before writing the dirty page, flush WAL up to the page's LSN (XLogFlush(BufferGetLSN(buf)), done inside FlushBuffer). This is the WAL-before-data rule: if the page reaches disk before its log record, a crash leaves a change with no record that redo or undo could use to reconcile it.
CONFIDENCE: high

## I1.R2
ANSWER: Pin and hold an exclusive content lock on both buffers, START_CRIT_SECTION, modify both pages, MarkBufferDirty each, then XLogBeginInsert, XLogRegisterBuffer for each changed buffer, XLogInsert, PageSetLSN on each, END_CRIT_SECTION, then unlock and unpin. Every changed buffer must be registered in the record so a full-page image can be taken; lock multiple buffers in a fixed order (e.g. by block number) to avoid deadlock.
Inside the critical section no palloc or anything that can ereport(ERROR), no lock waits, and no failable I/O, because an ERROR there escalates to PANIC.
CONFIDENCE: medium

## I1.R3
ANSWER: The first change after the redo point logs a full-page image (FPI) of the whole block, marked as a backup block, instead of only the delta. Subsequent changes log only the modified data.
This protects against torn pages: after a crash, redo restores the page from the full image, so it does not depend on a half-written on-disk page being consistent.
CONFIDENCE: high

## I1.R4
ANSWER: Use MarkBufferDirtyHint(), which is allowed to skip WAL for hint-only changes. When checksums or wal_log_hints are on and the page's LSN is at or before the redo point, log a full-page image (XLOG_FPI_FOR_HINT) and set the page LSN before the page is dirtied.
During recovery nothing can be written to WAL, so no FPI is logged; the hint is only applied and the page is marked dirty. Confidence is medium on the recovery details.
CONFIDENCE: medium

## I1.R5
ANSWER: MarkBufferDirtyHint may skip the change or its WAL protection, so an FSM update made in redo that is not rebuildable could be silently lost after the next crash.
The redo routine must use MarkBufferDirty() and set the page LSN to the replayed record's end LSN, through the XLogReadBufferForRedo path.
CONFIDENCE: medium

## I1.R6
ANSWER: Set HEAP_XMIN_COMMITTED only if the commit record's LSN is already flushed to disk (XLogNeedsFlush(commitLSN) is false). Otherwise the hint is skipped.
If the hint were set and the commit record were lost in a crash, the tuple would appear committed on disk while the transaction recovers as aborted, which is inconsistent.
CONFIDENCE: high

## I1.R7
ANSWER: Insert the commit WAL record; for sync commit, XLogFlush it; then set clog to committed (TransactionIdCommitTree); then wait for sync replication; then ProcArrayEndTransaction (makes the xact visible to others); then release locks.
Between inserting the commit record and updating clog, the backend sets DELAY_CHKPT_START so a checkpoint cannot take its redo point or snapshot of in-progress xacts in that gap; the checkpointer waits for such backends to clear the flag.
CONFIDENCE: medium

## I1.R8
ANSWER: The new subsystem's flush must happen inside CheckPointGuts, before the checkpoint record is inserted and before pg_control is updated, and before old WAL recycling (RemoveOldXlogFiles). Order: CheckPointGuts flush, XLogInsert checkpoint record, XLogFlush, update pg_control, then recycle WAL.
Everything needed by redo starting at the new redo point must be durable by then; changes after the redo point are covered by WAL replay.
CONFIDENCE: medium

## I1.R9
ANSWER: No. After a failed fsync, Linux clears the dirty flags, so a retried fsync can succeed while the data is lost.
PostgreSQL therefore PANICs on fsync EIO (data_sync_retry is off by default) and recovers from WAL redo.
CONFIDENCE: high

## I1.R10
ANSWER: The bulk path must WAL-log the page contents (e.g. log_newpage / log_newpage_range) and flush that WAL before the data file is written, which means WAL before data. It must also fsync the data files before commit (e.g. smgrimmedsync or a registered sync), since the WAL does not make the direct writes durable.
The content in WAL is the same page image written to disk, so standby and crash recovery reconstruct the same data.
CONFIDENCE: medium

## I1.R11
ANSWER: With wal_level=minimal, the bulk data for a relation created in this transaction is not WAL-logged. At commit the relation files must be fsynced (heap_sync or smgrimmedsync) before the commit record, and on abort the files are removed.
A relfilenumber must never be reused while an old file with that number could exist, so a new relfilenumber must be unique cluster-wide.
CONFIDENCE: low

## I1.R12
ANSWER: The unlink is deferred until the transaction's outcome is final: DROP's files are removed after commit via pending deletes, and a created-then-aborted table's files are removed at abort. Before unlinking, call DropRelationBuffers and ForgetRelationFsyncRequests so the checkpointer does not fsync a deleted file.
The commit record lists the dropped relfilelocators, so recovery can redo the unlink after a crash.
CONFIDENCE: medium

## I1.R13
ANSWER: The order is: insert the WAL truncate record (in a critical section), drop the buffers past the new end (DropRelationBuffers), then physically truncate the file. The WAL insert belongs in the critical section.
If the file truncate fails, it raises an ERROR; the WAL record already exists, so recovery will redo the truncate. The unused tail pages are harmless until the truncate succeeds.
CONFIDENCE: low

## I1.R14
ANSWER: Before writing the page on the standby, advance minRecoveryPoint (in pg_control) to at least the page's LSN, so a crash cannot leave the on-disk state ahead of the consistent recovery point.
Read-only connections are allowed only after recovery has reached the consistent point (minRecoveryPoint / backup end LSN) and the server is in a consistent state.
CONFIDENCE: medium

## I1.R15
ANSWER: Skip the record if the block's page LSN is already at or beyond the record's LSN. Otherwise, after applying it, set the page LSN to the record's end LSN and mark the buffer dirty.
Only the redo routine modifies the block (with the buffer locked exclusively, via XLogReadBufferForRedo). Redo cannot write new WAL, because XLogInsert is not permitted during recovery.
CONFIDENCE: high

## I1.R16
ANSWER: After redo changes the init fork page, it must flush that page to disk immediately (FlushOneBuffer or smgr write plus sync).
The main fork at the end of recovery is created by copying the init fork from disk, so an unflushed init fork page would make the copy wrong.
CONFIDENCE: medium

## I1.R17
ANSWER: At startup, ResetUnloggedRelations removes the main, FSM and VM forks and keeps the init fork. At the end of recovery, it copies the init fork to create an empty main fork, and the FSM and VM are rebuilt as needed.
CONFIDENCE: high

## I1.R18
ANSWER: A segment can be recycled only when all of these hold: its records lie entirely below the latest checkpoint's redo point; it is archived (when archiving is on) or archiving is off; no replication slot's restart_lsn still needs it (wal_keep_size also applies); and WAL summarization (summarize_wal) has summarized it.
CONFIDENCE: medium

## I1.R19
ANSWER: The upper bound is the flush position (GetFlushRecPtr): WAL already flushed to disk. Sending WAL still in the WAL buffer or written but not fsynced risks the standby getting WAL the primary loses on crash, after which the two diverge.
The standby reports its flushed position for the same reason.
CONFIDENCE: high

## I1.R20
ANSWER: The reader ends at the first invalid record (bad CRC, invalid xl_prev, missing continuation). After a crash, recovery writes an overwrite-contrecord record that marks the LSN of the discarded partial record.
Archiving and streaming standbys use that marker to know the missing part of the record was intentionally dropped, so they skip it and continue.
CONFIDENCE: medium

## I1.R21
ANSWER: The new timeline history file is written and fsynced, then archived (or notified to the archiver), before the new timeline's WAL is written. The switch happens at the end-of-recovery record or at a checkpoint/restartpoint on the new timeline.
The checkpointer must not write a checkpoint on the old timeline after the switch; it has to pick up the new ThisTimeLineID.
CONFIDENCE: low

## I1.R22
ANSWER: DROP DATABASE must ensure no backend is connected to the database and drop its buffers. It forces an immediate checkpoint so the commit record for the pg_database removal is on disk before the files are removed.
CREATE DATABASE with FILE_COPY does a checkpoint before copying (so source buffers are on disk), then copies the files and makes them durable (fsync or checkpoint) after the copy. DROP TABLESPACE cannot run in a transaction block, and removes the tablespace directory after commit, with a checkpoint first.
CONFIDENCE: low

## I1.R23
ANSWER: Insert XLOG_RELMAP_UPDATE in a critical section and flush it, then write the new relmap to a temp file, fsync it and rename it over the old file, then send the relcache invalidation to other backends.
The invalidation must come after the file is replaced, so other backends re-read the new file.
CONFIDENCE: medium

## I1.R24
ANSWER: The WAL record stores the last value logged (the pre-fetched upper bound, plus log_cnt), not the current value. The first nextval after a checkpoint must log, because the page LSN is at or before the redo point.
After a crash, recovery restores the logged bound, so values already handed out are never reissued. Gaps are possible but duplicates are not.
CONFIDENCE: medium

## I1.R25
ANSWER: The index is left in an intermediate state: the split pages exist but the parent has no downlink yet (an incomplete split, flagged on the page). The code must detect this and finish the split (_bt_finish_split) during insert, scan, or vacuum.
CONFIDENCE: high

## I1.R26
ANSWER: The backend declares completion only after a cluster-wide barrier (ProcSignal barrier) confirms all backends see the new state, and after a checkpoint that writes all pages with checksums. Enabling runs through inprogress-on and then on.
Disabling needs only the barrier and then off, with no full rewrite of pages, because backends just stop verifying checksums.
CONFIDENCE: medium

## I8.R1
ANSWER: ControlFileData change: bump PG_CONTROL_VERSION. WAL record structure change: bump XLOG_PAGE_MAGIC. pgstat file change: bump PGSTAT_FILE_FORMAT_ID. Catversion is not the right constant for these.
If a bump is missed, old data is misread: the control file version mismatch refuses startup, WAL pages fail the magic check, and the stats file is discarded or misparsed.
CONFIDENCE: medium

## I8.R2
ANSWER: Yes for the first commit: changing builtin argument names and catalog initial data (pg_proc.dat) requires a catversion bump, so initdb and pg_upgrade detect the mismatch.
For the primnodes.h field: no bump is needed if the node is only in-memory; a bump is needed only if the changed node is serialized into catalogs (such as a pg_node_tree) in a way old servers cannot read.
CONFIDENCE: medium

## I8.R3
ANSWER: The WAL record format cannot change on a stable branch. The replay code can be fixed so it handles existing WAL correctly, and the same fix must go into the development branch so the branches stay consistent.
Standbys should be upgraded before the primary, so the new replay behavior is in place before the primary produces WAL that depends on it.
CONFIDENCE: medium

## I8.R4
ANSWER: The server refuses to start (FATAL, database files incompatible) because pg_control records BLCKSZ, MAXALIGN and float format and checks them against the binary.
Persistent index ordering that depends on signed char leads to indexes that give wrong results on another platform. The fix is to define the comparison explicitly (e.g. unsigned) and REINDEX affected indexes after migration.
CONFIDENCE: medium

## I8.R5
ANSWER: pg_upgrade copies or links user data files, it does not rebuild them. A format change therefore needs conversion code in pg_upgrade (for example, rewriting the VM), or the old cluster must be rejected.
The same commit should bump catversion so pg_upgrade refuses an incompatible old cluster.
CONFIDENCE: low

## I8.R6
ANSWER: pg_upgrade decides them: its dump sets them using binary_upgrade_set_next_* functions before each CREATE, and the backend in binary-upgrade mode takes the preset value (OID or relfilenumber) instead of allocating a new one. The preset value is consumed after one use.
CONFIDENCE: high

## I8.R7
ANSWER: Acceptable: appending a new member at the end of an exposed struct, and appending a new NodeTag at the end of the enum, since existing code still sees the old offsets.
Not acceptable: inserting a member or NodeTag in the middle, or changing sizeof of a struct that extensions index as an array, because that breaks existing binaries.
CONFIDENCE: low

## I8.R8
ANSWER: An extension compiled for another major version is refused with an incompatible-library error, because PG_MODULE_MAGIC records the major version and key constants. A library without PG_MODULE_MAGIC is refused as missing the magic block.
The check does not catch struct changes within a minor version; that is why the core team keeps the ABI compatible across minor releases.
CONFIDENCE: high

## I8.R9
ANSWER: The modern server replies with NegotiateProtocolVersion, giving the newest minor version it supports and the list of unrecognized _pq_ options; it does not reject the connection, and the unknown options are simply not enabled.
A client facing an older server that cannot negotiate must retry the connection with protocol 3.0 and no _pq_ options.
CONFIDENCE: medium

## I8.R10
ANSWER: A partial protocol message read or a TLS/GSS transport error leaves the stream out of sync or the transport broken, so the session cannot continue. These must be FATAL (connection terminated), not ERROR.
Continuing with ERROR would read garbage from the middle of a message or write through a broken channel.
CONFIDENCE: medium

## I8.R11
ANSWER: The publisher checks the subscriber's proto_version (and publication options) against its supported min and max, and errors if out of range. The subscriber requests the highest version it supports that the publisher supports (using the server version to pick a lower one for older publishers).
Changes needed: update the version constants and logicalrep_read/write functions in the protocol code, handle the new option in pgoutput, update protocol.sgml, and add TAP tests.
CONFIDENCE: medium

## I8.R12
ANSWER: pg_upgrade refuses to run when the old and new clusters have different checksum settings, or when the old cluster is mid-transition (inprogress-on/off). The user must finish enabling or disabling checksums first.
Otherwise pages copied from the old cluster may be partly checksummed and the new cluster cannot verify them.
CONFIDENCE: medium

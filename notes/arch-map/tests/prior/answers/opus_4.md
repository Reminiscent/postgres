## I4.R1
ANSWER: 顺序：先写并刷提交 WAL 记录（及 clog）→ ProcArrayEndTransaction（对他人可见）→ AtEOXact_Inval 广播 sinval → 最后在 ResourceOwnerRelease(LOCKS) 阶段释放重量级锁。若先放锁再发失效，别的后端拿到锁后读到过期缓存；若先发失效再 ProcArrayEndTransaction，别的后端处理失效后用仍视本事务为进行中的快照重建缓存，缓存了旧 catalog 行且不会再被失效；若提交记录在可见之后，崩溃会丢失他人已看到的结果。
CONFIDENCE: high

## I4.R2
ANSWER: 先对该关系加适当的重量级锁（LockRelationOid/table_open），加锁成功后 LockRelationOid 会调用 AcceptInvalidationMessages，再读 relcache/syscache。这样保证持有冲突锁的 DDL 已提交且其失效已被吸收。不加锁直接读：未处理挂起的 sinval，可能看到过期定义；且并发 DROP/ALTER 可能在使用过程中改变或删掉该对象。
CONFIDENCE: high

## I4.R3
ANSWER: 不够。heap 层自动登记只对被改元组所在 catcache 失效，以及 pg_class/pg_attribute/pg_index 等少数 catalog 行对应的 relcache 失效。必须显式调用 CacheInvalidateRelcache(rel)/CacheInvalidateRelcacheByRelid(relid)（或 ...ByTuple）为受影响的表登记 relcache 失效（如发布成员变更时对相关表所做的那样）。
CONFIDENCE: high

## I4.R4
ANSWER: 不是。本事务的失效不经共享队列发给自己；在 CommandCounterIncrement 时由 CommandEndInvalidationMessages 把当前命令登记的消息在本地执行（LocalExecuteInvalidationMessage），下一条命令即看到新定义。共享队列广播只在提交时对其他后端进行；事务中止时则把本事务已在本地处理过的消息再本地执行一遍以撤销缓存中的新状态。
CONFIDENCE: high

## I4.R5
ANSWER: 事务回滚时登记的失效被丢弃，但 inplace 修改已经持久存在，其他后端（和本后端）缓存中保留旧值，长期过期。inplace 更新的失效应与修改本身同时、非事务地发送：在修改时（持有 buffer 锁/关键区内配合 WAL 记录携带失效供备库）立即发送（PreInplace_Inval/AtInplace_Inval 一类机制），不依赖事务提交。
CONFIDENCE: medium

## I4.R6
ANSWER: 不行。失效回调只能标记条目无效/dead，实际释放要推迟到引用计数降为 0（catcache 标 dead，在 ReleaseCatCache 时释放；relcache 引用中的条目原地重建、保留地址）。持有 syscache 元组指针（已 pin）时，table_open/detoast 可能处理失效，元组内存仍有效（因为被 pin）但内容可能已过期；若需最新值须重新查找。而指向 relcache 子结构（如 rd_rel、rd_att 以外的派生结构）的指针在重建后可能失效，不能继续用。
CONFIDENCE: medium

## I4.R7
ANSWER: 不能直接当有效条目保存。必须检测构建期间是否收到相关失效并重试：relcache 用 in_progress_list 记录正在构建的条目，失效到达时标记 invalidated，RelationBuildDesc 结束后发现被标记就丢弃重建；catcache 在 detoast 等过程中检测到失效（create_in_progress 列表）则把新条目标为 dead/重做查找。否则会缓存一个基于旧 catalog 的条目且永不再失效。
CONFIDENCE: medium

## I4.R8
ANSWER: 重置时会以“全部失效”形式调用：syscache 回调的 hashvalue 为 0，relcache 回调的 relid 为 InvalidOid。回调必须把这些值当作“清空/作废所有条目”处理，而不能当作某个具体键去匹配；否则重置期间丢失的具体失效消息永远不会被应用，缓存长期过期。
CONFIDENCE: high

## I4.R9
ANSWER: 用 CacheRegisterSyscacheCallback 为相关 syscache（NAMESPACEOID、AUTHOID、AUTHMEMMEMROLE 等）注册回调，回调中只置“缓存无效”标志（不做 catalog 访问），下次使用时惰性重算（如 namespace.c 对 search_path 的处理）。同时正确处理 hashvalue=0 的全量失效。
CONFIDENCE: high

## I4.R10
ANSWER: CachedPlanSource/CachedPlan 记录依赖的 relationOids 和 invalItems（函数等以 syscache hash 值表示），PlanCacheRelCallback/PlanCacheObjectCallback 收到匹配失效时把计划标记 is_valid=false，并不立即重规划。下次 GetCachedPlan 时 RevalidateCachedQuery/CheckCachedPlan 先获取所需锁（期间吸收失效），再检查有效性，无效才重新分析/重规划。
CONFIDENCE: high

## I4.R11
ANSWER: 执行器（及规划器）通过 PartitionDirectory 在一次查询内对每个分区表固定使用同一份 PartitionDesc 并持有引用，保证整个查询看到一致的分区集合，并能容忍分区被并发附加/分离（detach concurrently 结合快照过滤 detached 分区）。relcache 重建时不能释放仍被引用的旧 PartitionDesc（挂到 rd_pdcxt 下保留到关系关闭）。索引集合则依赖计划时的索引列表，未 ready/valid 的索引不使用，CIC 等待旧事务保证安全。
CONFIDENCE: medium

## I4.R12
ANSWER: CIC 修改标志后对表发送 relcache 失效，其他后端在下一次处理 sinval（如下次对任一关系加锁、开始事务）时吸收；CIC 用 WaitForLockers/等待旧快照保证此前的事务结束。重读时 RelationReloadIndexInfo 必须对非系统（以及非 nailed）索引重读 pg_index 的 indisvalid、indisready、indislive、indcheckxmin、xmin 等，表的 rd_indexlist 也要重算以包含尚未 valid 的索引（HOT/插入维护需要）。
CONFIDENCE: low

## I4.R13
ANSWER: 提交方：先取 RelCacheInitLock 并删除 init file（RelationCacheInitFilePreInvalidate），再发送 sinval，之后（再删一次）释放锁（PostInvalidate）。新后端：必须先加入 sinval 数组（SharedInvalBackendInit），再读 init file；写 init file 的后端在持锁下检查构建期间是否收到失效，若有则放弃写入。颠倒会让新后端读到过期 init file 却错过对应失效，或把过期内容写回 init file，导致长期使用错误的 catalog 描述。
CONFIDENCE: medium

## I4.R14
ANSWER: mapped 关系的 relfilenumber 不在 pg_class 中（为 0），必须从 relmapper（pg_filenode.map 及本后端/事务内的活动映射）取。并行 worker 需在任何 catalog 访问前（启动早期）恢复 leader 序列化的 relmap 状态（RestoreRelationMap），以便看到 leader 未提交的映射变更。sinval 重置时先 RelationMapInvalidateAll 重读映射文件，再对 nailed 关系重新计算物理地址（RelationInitPhysicalAddr）。
CONFIDENCE: medium

## I4.R15
ANSWER: 应丢弃（InvalidateCatalogSnapshot），下次 catalog 扫描取新快照。失效消息意味着有已提交的 catalog 变更；若继续用旧 catalog 快照重建缓存，会读到变更前的行并缓存过期数据，而失效已被消耗，永远不再修正。
CONFIDENCE: high

## I4.R16
ANSWER: 重放提交：先更新 clog（标记提交），再从 KnownAssignedXids 移除（使之对备库快照可见），然后发送 sinval（ProcessCommittedInvalidationMessages），最后释放代持的 AccessExclusiveLock（StandbyReleaseLockTree）。保证仅针对在主库取 AccessExclusiveLock 的 DDL（该锁被 WAL 记录并在备库代持）；弱锁 DDL 在备库无冲突锁保护，查询可能在下次吸收失效时中途看到变化。
CONFIDENCE: medium

## I4.R17
ANSWER: 该已提交事务的失效（每命令以 XLOG_XACT_INVALIDATIONS 记录）在其提交时被分发给所有并发进行中的被解码事务（连同新 catalog 快照），这些事务在各自重放到相应位置时执行这些失效，从而使用正确的 catalog 视图。为避免膨胀，分发的失效与事务自身失效分开存放并设上限；超过上限则标记溢出，改为执行全量缓存失效（InvalidateSystemCaches）。
CONFIDENCE: medium

## I4.R18
ANSWER: 只有当所有进程都成功吸收（其 barrier generation 达到目标值）后 barrier 才算完成，发起者用 WaitForProcSignalBarrier 等待。某进程吸收失败（处理函数返回 false 或抛错）时，保留/恢复该 barrier 类型的待处理标志，之后在下次 CHECK_FOR_INTERRUPTS 时重试；该进程未吸收前 barrier 不完成。
CONFIDENCE: medium

## I5.R1
ANSWER: TRY 中修改、CATCH 中读取的局部变量必须声明 volatile，否则 longjmp 后值可能不确定。CATCH 中不重抛就直接 return 等于吞掉错误：错误状态未 FlushErrorState，事务/资源（锁、buffer、内存上下文）处于需要中止清理的状态。正确写法：CATCH 中做最小清理后 PG_RE_THROW；若确需捕获继续，必须用子事务（BeginInternalSubTransaction + Rollback）并 CopyErrorData/FlushErrorState、恢复内存上下文和资源 owner。也不能从 PG_TRY 块内直接 return。
CONFIDENCE: high

## I5.R2
ANSWER: 不会执行。FATAL 不经 longjmp 展开，直接走 proc_exit，PG_FINALLY/PG_CATCH 都不运行。共享内存等非进程本地状态要用 before_shmem_exit/on_shmem_exit 注册回调，或用 PG_ENSURE_ERROR_CLEANUP（同时覆盖 ERROR 与 FATAL/进程退出），并在事务中止回调中也处理 ERROR 情形。
CONFIDENCE: high

## I5.R3
ANSWER: 关键区内任何 ERROR 都升级为 PANIC，导致整个集群崩溃重启；关键区内 palloc（除允许的上下文外）会触发断言失败。因此 palloc、扩展工作区、调用可能失败的函数（取块数、读 buffer、加锁等）必须在 START_CRIT_SECTION 之前完成，关键区内只做不会失败的修改页面、MarkBufferDirty、XLogInsert、PageSetLSN。
CONFIDENCE: high

## I5.R4
ANSWER: 不能。信号处理器只设置 QueryCancelPending/InterruptPending 并 SetLatch，真正的 ereport(ERROR) 在之后的 CHECK_FOR_INTERRUPTS→ProcessInterrupts 中发生。持有 LWLock 时 LWLockAcquire 已 HOLD_INTERRUPTS（InterruptHoldoffCount>0），处在 HOLD_INTERRUPTS 段或关键区时也一样，取消推迟到锁释放/RESUME_INTERRUPTS 之后的下一个 CHECK_FOR_INTERRUPTS 才生效。
CONFIDENCE: high

## I5.R5
ANSWER: spinlock 不被错误恢复机制跟踪和释放，持有期间 ereport 或 CHECK_FOR_INTERRUPTS 抛错会使 spinlock 永远不释放，其他进程自旋直至“stuck spinlock”PANIC；持锁代码只能是短小直线代码，不调用任何可能出错/分配/处理中断的函数。若信号处理器会直接 exit（经 proc_exit 运行回调），在此期间取 spinlock 不安全：退出回调可能再取同一 spinlock 自死锁，或留下被持有的锁；这类处理器只能在不持 spinlock 时生效，或用 _exit 由 postmaster 做崩溃重启。
CONFIDENCE: medium

## I5.R6
ANSWER: 应最早释放：AbortTransaction 一开始（HOLD_INTERRUPTS 后）就 LWLockReleaseAll、UnlockBuffers（以及 ConditionVariableCancelSleep 等），然后才运行各种回调和资源 owner 释放；进程退出时同理先释放 LWLock，再跑 before_shmem_exit 回调，然后 DSM detach，再 on_shmem_exit。顺序重要是因为清理回调/DSM detach 回调常需要获取这些锁，若仍持有会自死锁；且 LWLock 可能位于即将 detach 的 DSM 中。
CONFIDENCE: medium

## I5.R7
ANSWER: 登记到 ResourceOwner（定义 ResourceOwnerDesc，指定 release phase 与 priority，使用 ResourceOwnerEnlarge/Remember/Forget）。释放按阶段 BEFORE_LOCKS → LOCKS → AFTER_LOCKS，同阶段内按优先级；提交时仍未释放的资源会被视为泄漏并报 WARNING，中止时静默释放。子事务提交时其锁不释放，而是转移给父事务的 resource owner（LockReassignCurrentOwner），其他资源也转交父 owner。
CONFIDENCE: high

## I5.R8
ANSWER: CurrentMemoryContext 和 CurrentResourceOwner 被改写（指向父事务的上下文/owner）。调用者必须在开子事务前保存原值，回滚后 MemoryContextSwitchTo(oldcontext) 并恢复 CurrentResourceOwner = oldowner（PL 中还需重新处理 SPI 状态）。owner 一旦开始释放（releasing），就不能再登记新资源，ResourceOwnerEnlarge 会报错。
CONFIDENCE: high

## I5.R9
ANSWER: 中止可能正因内存不足或 catalog/缓存状态损坏触发，在钩子里 palloc 可能再次失败，查 catalog 时事务处于中止状态，不允许 catalog 访问（会报错或断言），钩子中报错会造成递归错误甚至 PANIC。钩子必须不抛错、不依赖有效事务、可重入/幂等、能处理部分初始化状态；GUC assign_hook 不允许失败（验证放在 check_hook，所需内存在 check_hook 里预分配通过 extra 传递）。
CONFIDENCE: high

## I5.R10
ANSWER: 分配到查询级上下文，如执行器的 es_query_cxt（或 portal 的 PortalContext），不要放 TopTransaction/CacheMemoryContext。事务中止时由 portal/执行器清理及事务内存上下文的删除统一回收（AtAbort_Portals/AtCleanup_Portals，删除子上下文），无需逐块 pfree。bump 上下文的 chunk 没有 header，不支持单独 pfree/repalloc（会报错），只能 reset/delete 整个上下文。
CONFIDENCE: high

## I5.R11
ANSWER: 短寿上下文 reset 后对象被释放，长寿缓存条目中的指针悬空，后续使用造成 use-after-free；外部库状态若在被 reset 的内存中或依赖它，会被破坏或泄漏（库自身 malloc 的资源不会被回收）。防护：把对象复制到与缓存同寿命的上下文；或在短寿上下文上注册 MemoryContextRegisterResetCallback 在 reset 时清空引用/调用库的释放函数（如 deflateEnd）；出错路径也要保证释放。
CONFIDENCE: high

## I5.R12
ANSWER: 不能立即释放，只标记为 dead/无效，使之不再被新查找命中；等引用计数降为 0 时再真正释放（catcache 在 ReleaseCatCache 中释放 dead 条目；relcache 引用中的条目原地重建保留地址）。ReleaseSysCache 之后不能再读条目字段，它可能随时被释放。
CONFIDENCE: high

## I5.R13
ANSWER: 用户代码可能向队列追加事件导致数组 repalloc 移动，原元素指针悬空；也可能递归处理/修改队列，使之前缓存的计数与状态不一致，若中途报错，多步更新只完成一半。应保存索引而非指针、调用后重新取元素；在调用外部代码前完成多步状态更新使数据结构处于一致状态（或先标记再处理）。
CONFIDENCE: medium

## I5.R14
ANSWER: 回滚到 savepoint：通过子事务回调只撤销该子事务（及其子孙）登记的条目，需按 SubTransactionId/嵌套层记录归属，外层的保留。事务中止：在事务中止回调/AbortTransaction 路径中全部移除（如 LockErrorCleanup 风格）。FATAL：用 before_shmem_exit/on_shmem_exit 回调保证从共享链表摘除，不能依赖 PG_FINALLY。
CONFIDENCE: medium

## I5.R15
ANSWER: 生命周期由引用计数决定：控制段中每个段有 refcnt，每个附着者 attach 加一、detach 减一（除非 pin 了，则持续到 unpin 或集群重启）。附着者要把映射交给 resource owner/注册 on_dsm_detach 回调，使 ERROR/FATAL 也能 detach；最后一个离开者负责销毁段并运行清理（如 SharedFileSet 删除文件）。之后才到的附着者 attach 会失败（dsm_attach 返回 NULL / SharedFileSet 报 already destroyed），必须当错误处理。
CONFIDENCE: medium

## I5.R16
ANSWER: leader 在清理（子）事务之前必须等所有 worker 退出（DestroyParallelContext→WaitForParallelWorkersToExit），因为 worker 在使用 leader 的事务状态、快照、锁组和 DSM。worker 的错误通过 shm_mq 错误队列以协议消息发给 leader，leader 在 HandleParallelMessages 中处理并重新抛出。汇总统计/instrumentation 前要 WaitForParallelWorkersToFinish（worker 完成且队列中消息已读完）。
CONFIDENCE: high

## I5.R17
ANSWER: 提交（及回滚）前 HoldPinnedPortals 把 PL 正在使用的 pinned portal 转为 holdable 并物化（只读/可物化的游标才行，否则报错）；WITH HOLD 游标在提交时持久化（PreCommit_Portals 中 HoldPortal）。中止时本事务创建的非 hold portal 标为失败并被删除，之前事务的 held portal 保留。只有非原子（SPI_OPT_NONATOMIC）的 SPI 连接、调用链全部经 CALL/DO 且不在显式事务块、子事务（异常块）或 SELECT 调用的函数中时才允许 COMMIT/ROLLBACK。
CONFIDENCE: medium

## I5.R18
ANSWER: 出错时不会调用 ExecEnd，依赖 ExecEnd 清理会泄漏。buffer pin 由 resource owner 跟踪（ReleaseBuffer 未调用时中止会自动释放，提交时报泄漏），文件应通过 fd.c 接口（OpenTemporaryFile/BufFile/AllocateFile/OpenTransientFile）打开以便事务中止时自动关闭；内存放在 es_query_cxt。其他资源应登记到 ResourceOwner 或注册内存上下文 reset 回调，而不是只靠 ExecEnd。
CONFIDENCE: high

## I5.R19
ANSWER: 创建：立即写 XLOG_SMGR_CREATE 并创建文件（RelationCreateStorage），同时登记 abort 时删除的 pending delete。删除：只登记 commit 时删除，在提交记录落盘后（smgrDoPendingDeletes）才真正 unlink。提交/中止 WAL 记录携带待删除关系列表，崩溃恢复和备库重放时据此删除文件；wal_level=minimal 跳过 WAL 的新建文件需在提交时 fsync（pending sync）。崩溃于提交前可能留下孤儿文件。
CONFIDENCE: high

## I5.R20
ANSWER: 放回前必须 DisownLatch（并先切回本地 latch，SwitchBackToLocalLatch），否则新使用者无法拥有该 latch。启用 lock group 时，leader 先退出则它的 PGPROC 不能立即放回（成员仍引用 lockGroupLeader），由最后一个退出的组成员负责把 leader 的槽放回 freelist。每个槽必须恰好放回一次。
CONFIDENCE: medium

## I5.R21
ANSWER: 通过 fd.c 接口：PathNameOpenFile（VFD）、OpenTransientFile/AllocateFile/AllocateDir（事务级句柄），临时文件用 OpenTemporaryFile/BufFile（受 temp_tablespaces 与 temp_file_limit 控制）。临时文件在 FileClose 时删除，或在所属 resource owner 释放（事务/中止）时关闭删除，进程退出时清理，残留的在 postmaster 启动时清理。事务级句柄在事务结束（AtEOXact_Files）时关闭，提交时未关闭会报泄漏 WARNING。
CONFIDENCE: high

## I5.R22
ANSWER: 不应。消息读到一半出错，协议同步已丢失，无法确定下一条消息边界，继续读会把残余字节当作新消息解析。PostgresMain 错误恢复中若发现正在读消息（pq_is_reading_msg），以 FATAL“terminating connection because protocol synchronization was lost”断开连接。
CONFIDENCE: high

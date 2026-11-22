## I4.R1
ANSWER: 顺序应为：写 WAL 提交记录并 flush（持久化点，同时 clog 标记 committed）→ ProcArrayEndTransaction（从 ProcArray 移除，使快照视其为已提交）→ 发送 sinval（AtEOXact_Inval）→ 释放重量级锁。若先放锁再发 sinval，等锁的后端拿到锁后看不到失效消息而使用陈旧缓存；若 ProcArray 移除先于 WAL flush，崩溃后可能出现他人已可见、磁盘上却无提交记录的事务。
CONFIDENCE: medium

## I4.R2
ANSWER: 先对表加锁（LockRelationOid，至少 AccessShareLock），再读 relcache/syscache。加锁路径在获得锁后会 AcceptInvalidationMessages，把已提交 DDL 的失效应用到本地缓存。不加锁直接读缓存可能拿到已被他人改掉的旧 relfilenode、tupdesc 或索引列表，导致读错数据或崩溃。
CONFIDENCE: high

## I4.R3
ANSWER: 不够。heap 层自动登记的只是被改元组本身对应的 syscache 失效，以及 pg_class/pg_attribute/pg_index/FK 约束相关的 relcache 失效。对其他 catalog 带来的其他表 relcache 变化，必须显式调用 CacheInvalidateRelcacheByRelid（或批量场景用 CacheInvalidateRelcacheAll）。
CONFIDENCE: high

## I4.R4
ANSWER: 不是通过共享队列发给自己。本后端在每条命令结束时用 CommandEndInvalidationMessages 把本命令的失效本地立即执行（LocalExecuteInvalidationMessage，经 CommandCounterIncrement 生效），供同事务后续命令看到新定义。跨后端的失效只在事务提交时才通过共享队列发送。
CONFIDENCE: high

## I4.R5
ANSWER: 若把失效登记为事务性的，回滚时失效消息会被丢弃，而 inplace 更新已经持久化，其他后端的 relcache/catcache 永远得不到通知，形成长期陈旧。应在修改缓冲区、写 WAL 之后、释放 buffer 锁之前立即发送非事务性的 sinval（heap_inplace_update 的做法），不能等到提交。
CONFIDENCE: medium

## I4.R6
ANSWER: 不能 pfree。被引用的条目只能标记为失效（dead/invalid），等引用计数归零（ReleaseSysCache/RelationClose）后再释放，否则悬挂指针。syscache 元组指针在 ReleaseSysCache 之前有效，但 table_open 或 detoast 可能处理失效消息，所以之后数据可能已陈旧；需要长期使用时应先复制。
CONFIDENCE: medium

## I4.R7
ANSWER: 不能直接保存为有效条目。构建期间若收到影响该条目的失效，应放弃本次结果（由 in-progress 列表或失效计数判断），重新构建（retry）；或标记为不可用，保证不把基于旧 catalog 构建的结果当作有效缓存。
CONFIDENCE: medium

## I4.R8
ANSWER: 回调以 cacheid = InvalidCacheId（即全量重置）的形式被调用，表示所有条目都可能失效。回调必须把自己持有的全部缓存整体清空，而不能只处理特定键。否则会残留陈旧条目，导致返回错误结果。
CONFIDENCE: high

## I4.R9
ANSWER: 用 CacheRegisterSyscacheCallback（如 NAMESPACEOID、AUTHOID）或 CacheRegisterRelcacheCallback 注册回调，让 DDL 通过 heap 层自动登记的 syscache 失效驱动清理。回调必须同时处理 InvalidCacheId 全量重置情况，才能保证不长期过期。
CONFIDENCE: high

## I4.R10
ANSWER: plancache 为每个 CachedPlanSource 记录依赖（PlanInvalItem），失效回调把计划标记为 is_valid=false。下次 GetCachedPlan 时才惰性地重新校验并重规划，不是失效当场立即重规划。被 DROP 的函数会通过对应的 PROCOID 失效回调触发这一过程。
CONFIDENCE: high

## I4.R11
ANSWER: 执行器应在查询快照范围内固定分区/索引集合：使用 PartitionDirectory（每个查询 pin 住分区描述符），保证同一查询内分区视图一致。索引集合需按快照过滤（如仅使用对该快照可见、ready/valid 的索引）。计划中引用的分区若已 detach 或新增，应依赖这种固定视图，而不是假设计划期的集合不变。
CONFIDENCE: medium

## I4.R12
ANSWER: 其他后端在下次处理 sinval 时吸收变化，通常是在下一次加锁或语句开始时。relcache 重读时必须刷新该表的全部索引列表和标志，而不只是正在构建的那个索引，因为失效是按 relation 粒度发出的。
CONFIDENCE: medium

## I4.R13
ANSWER: 顺序为：删除 init file → 发送 sinval → 释放 RelCacheInitLock。新启动的后端应先加入 sinval 数组（SharedInvalBackendInit）再读 init file 或 catalog，保证之后的失效不会漏掉。颠倒写端顺序，新后端可能读到旧 init file 且收不到后续失效，长期使用陈旧 relcache。
CONFIDENCE: low

## I4.R14
ANSWER: mapped catalog 的物理 relfilenode 从 relmap 文件（pg_filenode.map）获取，而不是 pg_class.relfilenode。relmap 必须在读取 mapped catalog 之前加载；并行 worker 启动时由 leader 序列化的 relmap 状态恢复。sinval reset 后，nailed 关系在重建之前应先重载 relmap。
CONFIDENCE: medium

## I4.R15
ANSWER: 收到任何失效消息时都应使 catalog snapshot 失效（InvalidateCatalogSnapshot），下次 catalog 扫描重新取快照。否则旧快照看不到其他会话刚提交的 catalog 行，导致读到陈旧定义。
CONFIDENCE: high

## I4.R16
ANSWER: 大致顺序为：clog 标记 committed → 从 KnownAssignedXids 移除 → 释放 AccessExclusiveLock（StandbyReleaseLockTree）→ 处理 sinval。sinval 应在锁释放前处理，使等锁的查询拿到锁后看到新 catalog。备库能保证的是：在提交记录重放之后取得的快照能看到主库已提交的 DDL；已持有旧快照的查询不保证。
CONFIDENCE: low

## I4.R17
ANSWER: 把已提交事务的失效消息分发（distributed invalidation）到所有正在解码的进行中事务，使每个事务按正确的 catalog 视图解码。只在事务确实修改了 catalog 时才记录，并在事务结束后及时清理，以避免消息膨胀。
CONFIDENCE: medium

## I4.R18
ANSWER: 只有当所有目标进程都已吸收该 barrier 并确认后，发起者的 WaitForProcSignalBarrier 才返回，barrier 才算完成。进程若在吸收时报错，不能确认完成，应使该进程退出（FATAL）并由 postmaster 重启，因为带着陈旧的进程本地状态继续运行并不安全。
CONFIDENCE: medium

## I5.R1
ANSWER: 问题：TRY 中修改、CATCH 中读取的局部变量应声明为 volatile，否则 longjmp 后值不确定；CATCH 中直接 return 会吞掉错误，错误状态未正确处理。正确写法：清理放在 PG_FINALLY 中；需要处理错误时先 FlushErrorState 或 PG_RE_THROW 重新抛出。
CONFIDENCE: high

## I5.R2
ANSWER: 不会执行。FATAL 退出经由 proc_exit 直接进行，不会 longjmp 到 PG_TRY 的 sigsetjmp，因此 PG_FINALLY/PG_CATCH 都不会运行。共享内存中登记的状态应通过 before_shmem_exit/on_shmem_exit 回调注册清理。
CONFIDENCE: high

## I5.R3
ANSWER: 在 critical section 内 palloc 可能 OOM，而 critical section 中的 ERROR 会升级为 PANIC，导致整个集群重启。所有分配与可能失败的调用都应放在 START_CRIT_SECTION 之前完成（预先按最大需要分配工作区），临界区内只做不会失败的操作。
CONFIDENCE: high

## I5.R4
ANSWER: 不能。信号处理器不是 async-signal-safe，不能调用 ereport。它只设置 QueryCancelPending/InterruptPending，之后在 CHECK_FOR_INTERRUPTS 处才真正 ereport(ERROR)。持有 LWLock 或处于 HOLD_INTERRUPTS（InterruptHoldoffCount>0）时，取消会被推迟，直到释放锁或 RESUME_INTERRUPTS。
CONFIDENCE: high

## I5.R5
ANSWER: 持有 spinlock 时 ereport 或 CHECK_FOR_INTERRUPTS 会 longjmp 离开而不释放 spinlock，导致死锁或挂起，因此 spinlock 持有期间不得调用可能报错或处理中断的函数。信号处理器直接 exit 时，其 on_exit 回调可能再次获取同一 spinlock 造成自死锁，因此在此窗口内取 spinlock 不安全。
CONFIDENCE: medium

## I5.R6
ANSWER: 中止流程中应先释放 LWLock 和 buffer content lock（LWLockReleaseAll 等），再运行 resource owner 的各阶段释放与各种回调，DSM detach 在 shmem_exit 回调中按注册顺序的相反顺序进行。若先执行可能获取锁的清理回调而仍持有短锁，会自死锁；释放顺序错误也可能使其他进程长时间等待。
CONFIDENCE: medium

## I5.R7
ANSWER: 应通过 ResourceOwnerRemember 登记到 resource owner，使用自定义 ResourceOwnerDesc（ReleaseResource 回调）。释放分 RESOURCE_RELEASE_BEFORE_LOCKS、RESOURCE_RELEASE_LOCKS、RESOURCE_RELEASE_AFTER_LOCKS 三阶段，每阶段内按 LIFO 顺序。子事务提交时其锁通过 ResourceOwnerNewParent 转移给父 owner，而不是释放。
CONFIDENCE: high

## I5.R8
ANSWER: 两个全局变量是 CurrentMemoryContext 和 CurrentResourceOwner。调用者必须在回滚后恢复自己保存的 memory context 和 resource owner，并清理错误状态（FlushErrorState/释放 ErrorData）。资源 owner 释放已开始后，不应再向其登记新资源，这样做不安全。
CONFIDENCE: medium

## I5.R9
ANSWER: 事务中止时状态不一致，catalog 查询可能失败或读到错误数据，palloc 也可能失败并导致 PANIC。钩子必须只做轻量、不会报错的操作：不做 catalog 访问，不 palloc，不抛错，并且是幂等的。
CONFIDENCE: medium

## I5.R10
ANSWER: 应分配到生命周期为查询的上下文，如 per-query/portal 的 MemoryContext（或 ExecutorState）。事务中止时由 portal/TopTransactionContext 的删除或 reset 统一回收。bump 上下文的 chunk 不能单独 pfree，只能随上下文 reset/delete 一起释放。
CONFIDENCE: high

## I5.R11
ANSWER: 短寿命上下文 reset 后，缓存条目中的指针悬挂，之后访问会 use-after-free。防护：把条目需要的数据复制到 CacheMemoryContext 中，或用 MemoryContextRegisterResetCallback 在上下文 reset 时使条目失效；外部库状态（如 zlib）应注册在上下文删除回调中释放。
CONFIDENCE: high

## I5.R12
ANSWER: 引用计数大于 0 时不能立即释放，应把条目标记为 dead/失效，待 ReleaseSysCache 把引用计数降为 0 时再释放。调用者在 ReleaseSysCache 之后不得再读取该条目的字段，因为此后内存可能已被释放。
CONFIDENCE: high

## I5.R13
ANSWER: 调用返回后，队列数组可能因扩容（repalloc）被移动，此前保存的元素指针悬挂；计数也可能因用户代码的副作用而过时。应保存索引而非指针，每次返回后按索引重新获取元素，并重新检查相关计数与状态。
CONFIDENCE: high

## I5.R14
ANSWER: 回滚到 savepoint 时只撤销该子事务（及其嵌套子事务）内的登记；整个事务中止时撤销全部登记；后端被 FATAL 终止时不走事务路径，应由 before_shmem_exit 回调清理。子事务中止只撤销属于该子事务的部分，不影响父事务已有登记。
CONFIDENCE: medium

## I5.R15
ANSWER: 共享对象的生命周期由附着引用计数决定：最后一个 detach 且未 pin 时才销毁。各附着者应注册 on_dsm_detach 回调（或通过 resource owner 登记）；最后离开者执行真正的清理。销毁之后才到达的附着者 dsm_attach 会失败，必须报错处理而不是访问已释放的段。
CONFIDENCE: medium

## I5.R16
ANSWER: leader 在清理（中止）创建 worker 的子事务之前，必须等待所有 worker 退出（WaitForParallelWorkersToFinish/Exit），否则 worker 仍在使用共享内存。worker 的错误通过 error queue（shm_mq）传给 leader，由 leader 处理并重新抛出。汇总 instrumentation 等统计之前，必须等待 worker 完成。
CONFIDENCE: medium

## I5.R17
ANSWER: COMMIT 时 WITH HOLD 游标必须先被物化（PersistHoldablePortal），因为事务快照随之消失；pinned portal 不能在事务结束时随意关闭，需要先 unpin。事务中止时，本事务创建的 portal 全部被清理（AtAbort_Portals）。只有非原子的 SPI 连接（如 CALL 且 non-atomic）才允许在函数中 COMMIT/ROLLBACK。
CONFIDENCE: low

## I5.R18
ANSWER: 不会。查询 ERROR 时 ExecEnd 通常不会被调用，文件句柄和 buffer pin 的释放依赖 resource owner。应把文件句柄通过 resource owner（如 temp file 的 VFD 机制）登记，buffer pin 由 resource owner 自动释放；ExecEnd 只负责正常路径的清理。
CONFIDENCE: medium

## I5.R19
ANSWER: CREATE 时立即创建物理文件，并登记 pending delete，若事务回滚则删除；DROP 的物理删除推迟到事务提交时（RelationDropStorage 登记 pending delete），回滚时取消。两者都写入 WAL（XLOG_SMGR_CREATE/UNLINK），崩溃恢复和备库通过重放这些记录获知文件状态。
CONFIDENCE: high

## I5.R20
ANSWER: 放回 freelist 之前必须 DisownLatch（或确保 latch 已断开归属）并重置字段。启用 lock group 时，leader 先退出，其 PGPROC 槽由最后一个仍在组内的成员负责放回。每个槽应且只应放回一次，重复放回会破坏 freelist。
CONFIDENCE: low

## I5.R21
ANSWER: 应通过 fd.c 的文件接口（如 OpenTransientFile/AllocateFile 用于普通文件，OpenTemporaryFile/BufFile 用于临时文件）打开，以便正确管理 fd 数量。临时文件在对应 resource owner 释放时（事务或进程结束）删除；事务级句柄在事务结束时关闭。
CONFIDENCE: medium

## I5.R22
ANSWER: 不应按普通 ERROR 处理后继续读取下一条消息。消息读到一半时协议流已经错位，无法重新同步，只能以 FATAL 关闭该连接。
CONFIDENCE: high

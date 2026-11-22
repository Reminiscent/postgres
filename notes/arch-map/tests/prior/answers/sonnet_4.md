## I4.R1
ANSWER: 顺序:先写提交记录(XLogInsert commit + flush,CLOG 标记),再 ProcArrayEndTransaction(使别人可见),然后 AtEOXact_Inval 广播 catalog 失效消息,最后释放重量级锁。锁必须最后放,失效消息必须在锁释放前发出,否则别的会话拿到锁后读到旧缓存;先 ProcArrayEndTransaction 之前未写提交记录则会可见但崩溃后丢失。
CONFIDENCE: medium

## I4.R2
ANSWER: 先对该表 OID 加锁(LockRelationOid),加锁时会 AcceptInvalidationMessages 吸收已提交 DDL 的失效消息,再 relation_open/读 syscache。不加锁直接读缓存可能读到过期条目,或 DDL 并发进行时看到不一致状态,因为没有锁就没有处理失效的同步点。
CONFIDENCE: high

## I4.R3
ANSWER: 不够。heap 层(CacheInvalidateHeapTuple)只对 pg_class/pg_attribute/pg_index/外键 pg_constraint 等特定 catalog 自动登记 relcache 失效。其他 catalog 变化影响另一个表 relcache 时,必须显式调用 CacheInvalidateRelcache(rel) / CacheInvalidateRelcacheByRelid 登记该表的 relcache 失效。
CONFIDENCE: high

## I4.R4
ANSWER: 不是通过共享队列发给自己。本事务的失效消息先存在事务内的 invalidation 列表里,CommandCounterIncrement 时通过 CommandEndInvalidationMessages 在本地直接执行(LocalExecuteInvalidationMessages),使下一条命令看到新定义;提交时才把消息写入共享队列广播给其他后端。
CONFIDENCE: high

## I4.R5
ANSWER: inplace 更新不随事务回滚,若失效只挂在事务上,回滚后不会发送失效,其他后端缓存仍是旧值而磁盘上已是新值,造成缓存与实际不一致。应在 inplace 更新所在的临界区完成后立即(不等事务结束)发送失效消息(CacheInvalidateHeapTupleInplace,更新后即刻广播),并且在写 WAL/修改之前后保证顺序正确。
CONFIDENCE: medium

## I4.R6
ANSWER: 不行。条目被调用栈引用时回调不能直接 pfree,只能标记无效/等引用释放后再清理(refcount 为 0 才能删,否则标记 invalid 等 release 时回收)。持有 syscache 元组指针时调用 table_open 或 detoast 可能处理失效消息使条目被重建/释放,之后该指针不能再安全使用,需要先复制所需数据或重新查找。
CONFIDENCE: medium

## I4.R7
ANSWER: 不能直接当有效条目保存。构建过程中若收到了相关失效消息(通过 invalidation 计数/标记检测),构建出的条目可能是基于旧数据,应丢弃并重新构建(循环重试),或保存后立即标记为无效。relcache 里用 in-progress 列表记录并在失效到达时标记 needs rebuild。
CONFIDENCE: medium

## I4.R8
ANSWER: 队列溢出后落后后端被 reset,会调用 InvalidateSystemCaches,所有回调以 hashvalue = 0(或 relid = InvalidOid)的形式调用,表示“全部失效”。回调实现必须处理 0 值,清空整个缓存而不是只处理特定条目,否则缓存会长期保留过期数据,出现错误行为。
CONFIDENCE: high

## I4.R9
ANSWER: 注册 syscache 回调(CacheRegisterSyscacheCallback)监听 pg_namespace、pg_authid 等相关 catalog 的失效,回调中把本地缓存标记为无效,下次使用时重算。这样 DDL 提交后失效消息到达就能让缓存刷新。回调要处理 hashvalue=0 的全清情况。
CONFIDENCE: high

## I4.R10
ANSWER: plancache 对计划里依赖的关系和函数/类型等登记依赖(relation OID 列表和 PlanInvalItem),通过 relcache 回调和 syscache 回调收到失效消息后把计划标记为 invalid,不是立刻重规划。下次执行时 RevalidateCachedQuery/ 获取计划时发现失效,才重新分析/重规划;执行时也会对表加锁并在加锁后重新检查是否有效。
CONFIDENCE: high

## I4.R11
ANSWER: 执行器启动后分区集合/索引集合可能变化,所以执行器应使用在启动时加锁并固定的分区描述(partdesc 缓存在 relcache,由 PartitionDesc 在查询期间保持不变,executor 保存引用),不要在执行中重新读 relcache 的最新分区信息。relcache 要保证 partdesc 在使用期间不变,重建时不能破坏旧引用(用独立 memory context 且延迟释放),分区剪枝要与计划时看到的一致。
CONFIDENCE: medium

## I4.R12
ANSWER: 其他后端只在下次获得锁/吸收失效消息时才看到变化(CIC 通过多个事务阶段并等待所有持有旧快照的事务结束来保证)。relcache 重读 indisready/indisvalid 时要覆盖所有索引,包括尚未 valid 或不 ready 的索引(不能只读 indisvalid 的),以便插入时仍维护 ready 但尚未 valid 的索引。
CONFIDENCE: medium

## I4.R13
ANSWER: 提交者应先删除 init file,再发送 sinval 消息,最后释放 RelCacheInitLock(持锁期间完成前两步)。新后端要先加入 sinval 数组(开始接收消息),再读 init file(持 RelCacheInitLock 读)。颠倒会导致后端读到过期 init file 却错过相应失效消息,缓存永久不一致。
CONFIDENCE: medium

## I4.R14
ANSWER: mapped/nailed 关系的物理文件位置(relfilenode)应从 relmapper 文件(pg_filenode.map)取,而不是 pg_class 的 relfilenode(为 0)。并行 worker 启动时要从 leader 恢复 relmap 状态(RestoreRelationMap),且必须在访问任何 catalog/重建 relcache 之前完成;sinval reset 重建 nailed 关系时同样用 relmap。
CONFIDENCE: medium

## I4.R15
ANSWER: 收到失效消息时应使 catalog snapshot 失效(InvalidateCatalogSnapshot),下次再取新的。因为缓存的快照可能看不到已提交的 catalog 变更,继续使用会让 catalog 扫描读到旧数据与刚被失效的缓存不一致。
CONFIDENCE: high

## I4.R16
ANSWER: 备库重放提交记录:先更新 clog(标记提交),然后从 KnownAssignedXids 移除该 xid(使其可见),再发送 sinval 失效消息,最后释放代持的 AccessExclusiveLock(主库 DDL 的锁通过 WAL 记录 XLOG_STANDBY_LOCK 重放获得)。这样保证备库上持有锁的查询在 DDL 提交后重读缓存。保证对取 AccessExclusiveLock 的 DDL(DROP、重写等)有效,对弱锁 DDL 无同样保证。
CONFIDENCE: medium

## I4.R17
ANSWER: 已提交事务的失效消息(从提交记录里的 invalidations 获得)要加入到所有其他正在解码的进行中事务的列表里,在对应的 command id 点执行,使它们能用正确 catalog 视图;并且只有 catalog 改动(有 invalidation)的事务才会这样分发。为避免膨胀,仅在事务的 base snapshot 变化时携带,并且只把消息分发给进行中的事务一次,不再无限保留。
CONFIDENCE: low

## I4.R18
ANSWER: barrier 在所有进程都吸收(absorb)了它之后才算完成,发起者通过 WaitForProcSignalBarrier 等待所有后端的 generation 到达。某进程吸收失败(出错)则这是严重问题:吸收函数必须不能失败,若失败进程应当 FATAL 退出/整体处理,因为否则 barrier 永远无法完成,状态不一致。
CONFIDENCE: medium

## I5.R1
ANSWER: PG_TRY 里修改过的局部变量在 longjmp 后值不确定,除非声明为 volatile;CATCH 里直接 return 会跳过 PG_END_TRY,留下错误栈 (PG_exception_stack) 指向已失效的栈帧。应将局部变量声明 volatile,CATCH 里要么 PG_RE_THROW 要么走到 PG_END_TRY,不要提前 return。
CONFIDENCE: high

## I5.R2
ANSWER: FATAL 退出时 PG_TRY/PG_FINALLY 里的清理不会执行(FATAL 是 proc_exit,不 longjmp 到 TRY 处,或者说不保证)。非进程本地状态应通过 on_shmem_exit / before_shmem_exit 回调或 ResourceOwner 登记清理,它们在进程退出时一定被调用。
CONFIDENCE: medium

## I5.R3
ANSWER: 临界区内出错会升级为 PANIC,palloc 可能 OOM 失败也会 PANIC;而且 palloc 内存分配不允许在临界区(assert 会拒绝)。必须在 START_CRIT_SECTION 之前预分配内存、扩展工作区(如 XLogEnsureRecordSpace),并调用可能失败的函数取块数,临界区内只做不会失败的操作。
CONFIDENCE: high

## I5.R4
ANSWER: 不能在信号处理器里直接 ereport(ERROR),处理器只设置标志 (InterruptPending、QueryCancelPending) 并 SetLatch,真正中止在 CHECK_FOR_INTERRUPTS 点执行。持有 LWLock 时若中断被 HOLD(LWLock 获取会 HOLD_INTERRUPTS),或在 HOLD_INTERRUPTS 段,取消延迟到 RESUME_INTERRUPTS 之后的下一个 CHECK_FOR_INTERRUPTS 才生效。
CONFIDENCE: high

## I5.R5
ANSWER: 持有 spinlock 期间 ereport 或 CHECK_FOR_INTERRUPTS 会导致 spinlock 不被释放(ERROR 清理不会释放 spinlock),别的进程自旋死等直至超时 panic/卡死;spinlock 内只能做很短的非阻塞操作。如果信号处理器会直接 exit 进程(如 die 里 proc_exit),在其生效期间取 spinlock 不安全,因为可能持锁时被退出,锁永远不释放(ProcDiePending 在 handler 里直接 exit 只发生在读命令时)。
CONFIDENCE: low

## I5.R6
ANSWER: LWLock 和 buffer 内容锁在事务中止/shmem_exit 的最早阶段释放(LWLockReleaseAll,在 AbortBufferIO、各类回调之前),在 DSM detach 和其他清理回调之前。因为其他清理回调可能需要这些锁或者会等待持锁者,若不先释放会自死锁;持有时执行回调还可能操作被锁保护的数据。
CONFIDENCE: medium

## I5.R7
ANSWER: 登记在 ResourceOwner(CurrentResourceOwner)里,通过资源所有者机制的 ResourceOwnerRemember/Forget 或自定义 release callback。释放分阶段:BEFORE_LOCKS、LOCKS、AFTER_LOCKS,按优先级顺序(buffer pin 等先,lock 然后,最后其他)。子事务提交时其持有的锁转移给父 ResourceOwner(ResourceOwnerReleaseAllLocks 不释放,而是 reassign 给父),子事务中止时释放。
CONFIDENCE: medium

## I5.R8
ANSWER: 回滚子事务后,CurrentMemoryContext 和 CurrentResourceOwner 被改写(指向父事务状态之外/被重置),调用者必须保存并在之后恢复自己原来的 CurrentMemoryContext 和 CurrentResourceOwner(MemoryContextSwitchTo(oldcontext); CurrentResourceOwner = oldowner)。一旦释放开始,不能再向该 owner 登记新资源(会 error/assert)。
CONFIDENCE: high

## I5.R9
ANSWER: 事务中止时事务状态已不一致,palloc 可能失败(OOM)、查 catalog 需要有效事务、可能再次出错造成递归 abort(无限循环或 PANIC)。钩子必须尽量简单、不会失败、不访问 catalog、不分配内存(或用预分配内存),且应当是幂等的(可被重复调用)。
CONFIDENCE: high

## I5.R10
ANSWER: 分配进 per-query 内存上下文(如 estate->es_query_cxt)或合适的长生命周期上下文(CurTransactionContext 如果事务内)。事务中止时由事务上下文管理器通过 MemoryContextDelete/reset 回收(TopTransactionContext 子树,AbortTransaction 清理),不需要自己释放。bump 上下文的 chunk 不能单独 pfree(也不能 repalloc),只能通过 reset/delete 整体释放。
CONFIDENCE: medium

## I5.R11
ANSWER: 短寿命上下文 reset 后 CacheMemoryContext 条目里的指针变成悬空指针,之后使用会读到垃圾或崩溃;外部库状态不会被自动释放造成泄漏。防护:在 CacheMemoryContext 中深拷贝数据(或把条目分配在自己的长寿命上下文里),对外部资源注册 MemoryContextCallback 在上下文销毁时清理。
CONFIDENCE: medium

## I5.R12
ANSWER: refcount>0 时不能立刻释放条目,只能标记为 invalid(或为 relcache 用 rebuild 等),待引用释放(refcount 降为 0)后再真正删除/重建。ReleaseSysCache 之后不能再读条目字段,因为条目可能随时被删除,需要的数据应在释放前复制出来。
CONFIDENCE: high

## I5.R13
ANSWER: 调用可能运行用户代码的函数时,事件队列数组可能被重新分配(增长),之前保留的元素指针会悬空;多步更新的计数也可能被递归调用改变。应在调用后重新通过索引取元素、重新读取计数,不要缓存指针;在多步更新前完成状态修改以保证可重入。
CONFIDENCE: high

## I5.R14
ANSWER: 回滚到 savepoint(子事务中止):撤销该子事务期间新增的登记,按子事务层级记录并只撤销该子事务所加的部分;整个事务中止:在 AtAbort 回调中移除登记;FATAL 终止:通过 on_shmem_exit/before_shmem_exit 回调移除。子事务中止时只撤销本子事务(及其子孙)登记的条目,不动父事务的。
CONFIDENCE: medium

## I5.R15
ANSWER: 共享对象的生命周期由引用计数(附着者数量)决定,而非某一个后端。每个附着者异常退出时通过 on_shmem_exit/ dsm detach 回调自动 detach,最后一个离开者负责销毁(释放段/删除临时文件),之后才到的附着者发现段已销毁(attach 失败/返回 NULL)应当处理这个错误而不是假定存在。
CONFIDENCE: medium

## I5.R16
ANSWER: leader 在清理事务(AtEOXact_Parallel)之前必须等所有 worker 退出(WaitForParallelWorkersToExit/DestroyParallelContext 等待 worker 终止),不能让 worker 还在运行。worker 的错误通过错误队列 (shm_mq) 以 ErrorResponse 消息转发给 leader,leader 在 ProcessParallelMessage 里重新抛出。汇总统计数据(buffer/WAL usage、instrumentation)前要等 worker 完成(WaitForParallelWorkersToFinish)。
CONFIDENCE: medium

## I5.R17
ANSWER: 提交时 WITH HOLD 游标会被物化(PersistHoldablePortal)保留,pinned portal 不能被清理,事务控制中遇到 pinned portal 会报错。事务中止时,本事务创建的 portal 被清除(AtAbort_Portals 标记失败并清理),WITH HOLD 的已提交的除外。SPI 连接必须是非原子(nonatomic)的才能 COMMIT/ROLLBACK,即 SPI_connect_ext(SPI_OPT_NONATOMIC),且不在子事务内。
CONFIDENCE: medium

## I5.R18
ANSWER: ERROR 中止时不会调用 ExecEnd 回调,所以放在 ExecEnd 里的清理不会执行,文件和 buffer pin 会泄漏。应把资源归属到 ResourceOwner(buffer pin 本来就由 resowner 释放,文件用 fd.c 的 AllocateFile / VFD 这类事务级跟踪)或使用 memory context 回调,使得出错时由 resowner/事务中止自动释放。
CONFIDENCE: high

## I5.R19
ANSWER: 创建和删除文件都推迟到事务提交/中止时执行:RelationCreateStorage 登记 pending delete(中止时删除),RelationDropStorage 登记(提交时删除)。AtEOXact_smgr / smgrDoPendingDeletes 在事务结束时执行。WAL 记录 XLOG_SMGR_CREATE 以及提交/中止记录里带上要删除的 rel 列表,崩溃恢复和备库重放时据此创建/删除文件。
CONFIDENCE: high

## I5.R20
ANSWER: 放回 freelist 前要先 DisownLatch(释放 latch 所有权),避免被别的进程占用旧 latch 的信号。lock group 中如果 leader 先退出,leader 的槽由最后一个退出的 group 成员放回 freelist。每个槽只放回一次。
CONFIDENCE: medium

## I5.R21
ANSWER: 通过 fd.c 接口(AllocateFile、OpenTransientFile、PathNameOpenFile、OpenTemporaryFile 等)打开文件,这样受 VFD 机制与 fd 限制管理。临时文件在事务结束(或上下文/resowner 释放)时自动删除,启动时清理残留;事务级句柄(AllocateFile 等)在事务结束时关闭,若不是则需手工关闭。
CONFIDENCE: medium

## I5.R22
ANSWER: 不能按普通 ERROR 继续读下一条消息:读一半出错后,协议流已经失去同步(消息只读了一部分),继续读会把剩余字节当作新消息。应当升级为 FATAL(断开连接),pq_comm_reset/ 在读消息中途的错误使用 FATAL 或 PqCommReadingMsg 时 ERROR 被提升为 FATAL。
CONFIDENCE: high

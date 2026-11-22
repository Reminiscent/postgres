## I2.R1
ANSWER: 先在 clog(pg_xact)标记 committed(此前已写 commit WAL),之后再从 ProcArray 清除 xid。清 ProcArray 时持 ProcArrayLock 排他(或用 group clear),并同时推进 latestCompletedXid。这样取快照(持 ProcArrayLock 共享)的人要么仍看到 xid 在运行集合中(视为在途),要么 clog 已是 committed,不会出现"不在运行又未提交"的窗口。
CONFIDENCE: medium

## I2.R2
ANSWER: TransactionIdDidCommit 为假只说明未提交,可能是仍在运行、已中止或崩溃遗留,不能推出已中止。正确流程:先查 TransactionIdIsInProgress(ProcArray/子事务缓存),在途则不可判;不在途再查 DidCommit,为真即已提交,否则才认为已中止(crash 的事务也视为 aborted)。顺序不可颠倒,否则有竞态。
CONFIDENCE: high

## I2.R3
ANSWER: snapbuild 造出的快照是自己构造的数据(xip 数组/xmin/xmax),不一定满足 MVCC 快照对 xip 排序、subxip、官方字段的预期,且交出去后 HeapTupleSatisfiesMVCC 会按其 xmin/xmax/xip 判断,若构造不一致会错判可见性。应在构造后保证它是 MVCC 类型、一致且已排序,并像普通快照那样注册/拷贝(SnapBuildInitialSnapshot 里重新计算 xip 为已提交且 >= xmin 的事务),同时保证 xmin 受保护不被回收(设置槽的 xmin)。
CONFIDENCE: low

## I2.R4
ANSWER: 仅当自上次取快照以来,运行事务集合(含 xid 完成、子事务变动)没有变化时可复用,即 xactCompletionCount 未变。任何结束事务或改变运行集合的新路径必须递增 ProcGlobal->xactCompletionCount(持 ProcArrayLock 排他时),否则后续取快照会复用过期缓存,把已结束事务当成仍在运行或反之,导致可见性错误。
CONFIDENCE: medium

## I2.R5
ANSWER: 依靠 PGPROC 子 xid 缓存的 overflowed 标志位,以及 pg_subtrans(子 xid 到父 xid 的映射)。快照 suboverflowed 时,需查 pg_subtrans 找到顶层 xid 再看是否在 xip 里。顺序要求:分配子 xid 并把 parent 写入 pg_subtrans 必须先于该 xid 对外公布(放入 PGPROC 缓存/设置 overflow),这样别人看到 xid 时 subtrans 已有记录。
CONFIDENCE: medium

## I2.R6
ANSWER: 视界必须保守:不能超过任何仍可能需要这些元组/行的快照的 xmin,且单调不倒退,对各类对象(共享表、本库表、catalog)分别计算。综合来源:所有后端的 xmin 与 xid(ProcArray)、复制槽的 xmin 和 catalog_xmin、hot standby feedback 的 xmin、prepared 事务、以及 old_snapshot 等,取最小值。
CONFIDENCE: medium

## I2.R7
ANSWER: 仅当该操作不会再以会被他人依赖的快照读取任何会被清除的元组、也不会在索引表达式/谓词中访问其他表数据时才正确(VACUUM 只看元组 xmin 不跑用户代码)。CREATE INDEX CONCURRENTLY 要用快照扫表并可能执行索引表达式、谓词,需要快照保护其所见元组不被 vacuum 删除,所以不能把自己排除出视界(只有 PROC_IN_SAFE_IC 的特殊情形允许,条件严格)。
CONFIDENCE: medium

## I2.R8
ANSWER: 要先取 OldestXmin(GlobalVisState/vistest 的建立),然后再读关系页数(RelationGetNumberOfBlocks),这样此后新增的页面中的元组必然比 OldestXmin 新,不需被考虑。页内每个元组的存活判定做一次:prune 阶段用 HeapTupleSatisfiesVacuum 得到的状态保存下来供后续复用,避免两次判定因并发事务状态变化给出不一致结果(例如先 RECENTLY_DEAD 后变 DEAD)。
CONFIDENCE: medium

## I2.R9
ANSWER: xmin 已提交且早于 FreezeLimit/cutoff(对所有快照可见)的元组可冻结;xmax(含 multixact)需按截止点处理,死元组不冻结。推进 relfrozenxid/relminmxid 时,新值不得超过表中仍存在的未冻结 xid/mxid 的最小值(含被跳过页面的情况,只有扫描了所有非 all-frozen 页才可推进到 cutoff,否则只能保守),且不能倒退。
CONFIDENCE: medium

## I2.R10
ANSWER: 先更新并写 pg_database 的 datfrozenxid(提交),再算出新的最老值,然后截断 clog/multixact/subtrans,最后更新 xid 分配限值(vacuum limit,SetTransactionIdLimit)。先设限值再截断可能让分配回绕;截断必须在 datfrozenxid 已落盘后。若 heap 外文件(如 NOTIFY 队列)存 xid,需另外保证这些 xid 不早于被截断的范围,即清理/冻结它们或把其最老 xid 纳入截断下限,否则查 clog 时会失败。
CONFIDENCE: low

## I2.R11
ANSWER: 设置位的一方(vacuum)必须确认页内所有元组对所有快照可见(且先持有页面锁/pin VM 页),设置时写 WAL 并保证页面 PD_ALL_VISIBLE 一致。修改页面的一方(insert/update/delete/lock)必须在同一临界区内清除页面 PD_ALL_VISIBLE 和 VM 位并写 WAL。all-frozen 位蕴含 all-visible:frozen 只能在 all-visible 已设置时存在,清 visible 必须同时清 frozen。
CONFIDENCE: medium

## I2.R12
ANSWER: 被索引指向的死 root line pointer 不能直接回收为 unused,prune 把它变成 LP_DEAD(保留 stub,可去掉存储),等 vacuum 删索引项后才转 unused。顺序:先删索引项(bulkdelete),再回收 heap 的 line pointer(LP_DEAD->LP_UNUSED)。B-tree 删除的页需等到删除它的 xid 对所有快照(含并发扫描)已不可见,即 safexid 早于全局 xmin 后才可重用(记入 FSM)。
CONFIDENCE: medium

## I2.R13
ANSWER: 每走一步需核对下一版本的 xmin 等于前一版本的 xmax(以及 TID 仍有效、line pointer 仍为 normal 且非 redirect/dead 之外的意外状态)。核对失败说明该 line pointer 已被 prune 并重用,链断了,应停止/视为链结束(或重新从头来、报告并发更新),不可继续用错误元组。链的起点没有前驱可核对,不能做同样检查,只能靠持有的快照/锁保证。
CONFIDENCE: medium

## I2.R14
ANSWER: 新 catalog_xmin 依据解码已读到的位置:restart_lsn 之后仍有事务运行的最老 xid(snapbuild 的 running xacts/builder xmin),不超过仍需要解码的事务。更新顺序:先把新值写入槽的 data.catalog_xmin 并持久化(落盘),之后才更新 effective_catalog_xmin 供 vacuum 使用;顺序保证崩溃后不丢失保护。历史快照不能当普通 MVCC 快照扫用户表,它只用于 catalog 扫描(HeapTupleSatisfiesHistoricMVCC)。
CONFIDENCE: medium

## I2.R15
ANSWER: standby 的运行集合必须包含重放得到的运行事务(KnownAssignedXids),包括 running-xacts 记录里的子事务,且只有当 snapshot 状态为 consistent(已达最小恢复点且 running xacts 完整,subxid 未溢出或可信)时才接受只读连接。提升时:先丢弃 KnownAssignedXids(ExpireAllKnownAssignedTransactionIds),再切换恢复状态(退出 recovery,允许写)。
CONFIDENCE: medium

## I2.R16
ANSWER: standby 重放前必须检查与该记录的 snapshotConflictHorizon(latestRemovedXid)冲突的查询,等待或取消(ResolveRecoveryConflictWithSnapshot)。主库一侧要保证记录里的这个 horizon 值不小于任何被删除元组的 xmax(即足够新/保守),这样 standby 上快照 xmin 不超过它的查询会被取消。
CONFIDENCE: medium

## I2.R17
ANSWER: 新索引可能未包含更早快照看得见的旧版本元组(建索引时只索引当时有效的版本,HOT 链断裂),旧快照使用会漏行。规划器通过 pg_index 的 indcheckxmin 判断:若 xmin 未老于当前快照的 xmin,则不使用该索引。并发建索引需分阶段:先 indisready(接受插入),等待旧事务结束,再 validate 后设置 indisvalid,每一步之间需等所有持有旧快照的事务结束。
CONFIDENCE: medium

## I2.R18
ANSWER: toast 指针指向的外存行可能被 vacuum 回收(commit 后快照丢弃,xmin 前进),再 detoast 会报 "missing chunk number" 错误或读到错误数据。无活跃快照时 detoast 同样没有 xmin 保护。持有 toast 指针的一方应在事务/COMMIT 前先 detoast(展开成内联值)或保持一个注册的快照,并在需要时复制该值。
CONFIDENCE: medium

## I2.R19
ANSWER: READ COMMITTED:等并发事务结束,若已提交则对最新版本重新评估(EvalPlanQual)qual,满足则更新新版本;若中止则继续用原版本。REPEATABLE READ/SERIALIZABLE:若并发事务已提交,报 serialization failure("could not serialize access due to concurrent update"),由应用重试。
CONFIDENCE: high

## I2.R20
ANSWER: 需对扫描读取的谓词范围加锁:堆表上锁元组/页/整表,索引上锁所访问的索引页(或 B-tree 叶页、GIN 项等)以覆盖间隙/范围。要在读取数据之前(访问页的同时)加锁,以避免漏掉并发写。事务提交后 SIREAD 锁不能立刻释放,要保留到所有重叠的并发事务结束。允许误报(false positive,粒度提升导致不必要的序列化失败),不允许漏报。
CONFIDENCE: medium

## I2.R21
ANSWER: 只有非事务性、可容忍"回滚也保留"且可被覆盖的状态,例如统计/估计信息(relpages、reltuples、relfrozenxid 等)、序列值、relation map;这些状态不需要多版本可见性,也不需要回滚语义,并且写入要幂等、可被后续覆盖,同时要处理崩溃安全(WAL 记录)。逻辑内容上的数据不可这样。
CONFIDENCE: medium

## I2.R22
ANSWER: worker 要继承 leader 的快照(事务快照与活跃快照,序列化传递)、事务状态(当前 xid、子事务 xid 集合、事务隔离级别)、combo CID 映射、以及 catalog 快照相关信息,使可见性判定一致。并行模式下不允许写数据(不能修改堆)、不能分配新 xid(需 leader 事先分配),不能创建子事务,也不允许任何会改变事务状态的操作。
CONFIDENCE: medium

## I2.R23
ANSWER: 收到 sinval 失效消息后(处理失效),缓存的 catalog 快照应被作废(InvalidateCatalogSnapshot),因为它可能不反映新的 catalog 变化。下一次 catalog 扫描会重新取快照(GetCatalogSnapshot -> 重新 GetSnapshotData),不会沿用旧的。
CONFIDENCE: high

## I7.R1
ANSWER: postmaster 不能持有 spinlock 或 LWLock,因为它不应依赖共享内存的内容(子进程可能带着锁崩溃,锁永远不释放会使 postmaster 卡死,且它要在子进程崩溃后继续存活做重置)。postmaster 与子进程之间的共享数据应设计为 postmaster 只读写的、无锁的简单结构(如 pmsignal 的标志位、原子变量),并假定内容可能被破坏而做防御性检查。
CONFIDENCE: high

## I7.R2
ANSWER: 通过 waitpid 得到的退出状态:正常是 exit(0);非零退出或被信号终止(非预期的)视为崩溃,需要触发 HandleChildCrash 重置。碰过共享内存的后台进程若直接 exit(0) 而不是通过 proc_exit 清理(未从 ProcArray 注销、未释放锁),postmaster 会发现它还在 PMChild/共享状态里登记(如 PGPROC 未清),从而当作异常退出并引发崩溃重置。
CONFIDENCE: medium

## I7.R3
ANSWER: 在读启动包阶段该子进程还没完成与共享内存的信号设置,处理器里取 spinlock 时若被信号打断在持有同一 spinlock 的代码中,会自死锁;且 _exit(1) 的处理器本来设计成不碰共享内存。应保持处理器只做异步信号安全的事:设置标志,待读完启动包后在安全点检查并处理取消请求,或在之后再转发。
CONFIDENCE: low

## I7.R4
ANSWER: 信号处理器可能中断任意代码(含 malloc、持锁区、ereport 中),在其中调用 proc_exit 或 ereport 会重入非异步信号安全的代码,导致死锁或数据损坏。处理器里只能做异步信号安全的事:设置 volatile sig_atomic_t 标志、SetLatch,让主循环在安全点(CHECK_FOR_INTERRUPTS)检查并处理关闭。
CONFIDENCE: high

## I7.R5
ANSWER: 错误恢复块执行期间应保持信号阻塞(处理完错误后才 PG_SETMASK(&UnBlockSig) 解除)。若在恢复后收到 SIGQUIT,进程应立刻 _exit(2)(quickdie / SignalHandlerForCrashExit 不跑清理)。退出码有特殊含义:postmaster 以退出码 2 判断是 postmaster 要求的崩溃退出(不是新的崩溃);其他非零码会被当作进程自己崩溃而触发重置。
CONFIDENCE: low

## I7.R6
ANSWER: 崩溃重置时旧共享内存可能已被崩溃的进程破坏,其中的 shmem 请求数组不可信;读它会得到损坏或过期的数据。重建应基于 postmaster 自己私有内存里保留的输入(各模块在 postmaster 启动时通过 shmem_request_hook 登记的请求),重新计算大小并重新创建与初始化共享内存。
CONFIDENCE: medium

## I7.R7
ANSWER: fork 前先在 postmaster 私有结构里分配一个子进程槽(PMChild,按类型记账,拿 pid 和 slot),fork 失败时释放该槽并回滚计数,不能留下幽灵项。槽池耗尽时调用方不应强行 fork,应报错/拒绝连接或延后重试(如 "too many clients" 或 bgworker 稍后再试)。
CONFIDENCE: medium

## I7.R8
ANSWER: postmaster 里没有 error recovery(没有 sigsetjmp 的 PG_TRY 恢复),ereport(ERROR) 会升级为 FATAL 使 postmaster 退出,导致整个服务器宕机。配置重载检查应用 LOG/WARNING 级别报告并让检查函数返回失败(保留旧配置),不抛 ERROR。
CONFIDENCE: high

## I7.R9
ANSWER: 进程若只等待 latch 而不检查 postmaster 是否存活,postmaster 被 SIGKILL 后进程会变成孤儿,永远不退出。应在 WaitLatch 中加 WL_EXIT_ON_PM_DEATH(或 WL_POSTMASTER_DEATH 并自行处理退出),让进程在 postmaster 死亡时立即退出。
CONFIDENCE: high

## I7.R10
ANSWER: 没有 SIGKILL 兜底:fast shutdown 与提升期间 postmaster 只发 SIGTERM 等待子进程自行退出(SIGKILL 只在崩溃重置后的超时才发)。因此阻塞的写/等待必须可被中断:使用非阻塞 socket + WaitLatch/WaitEventSet 并处理中断(CHECK_FOR_INTERRUPTS),不能无限阻塞在不可中断的系统调用上。
CONFIDENCE: medium

## I7.R11
ANSWER: 崩溃重置后:BGW_NEVER_RESTART 的 worker 被从列表中移除(不再重启);其他 worker 保留并在重置完成后按 bgw_restart_time 重新启动。想停掉但仍由重启策略决定是否回来,应让 worker 自己以非零退出(或给它发信号终止,即 TerminateBackgroundWorker 之外的方式),让 postmaster 按重启策略处理,而不是设置 rw_terminate(该位会使其永不重启/被注销)。
CONFIDENCE: low

## I7.R12
ANSWER: 不应该简单重置重试。startup 进程非被 postmaster 信号杀死而意外退出,代表恢复失败(WAL 损坏、缺失等),重置后重跑恢复大概率重复失败,形成无限循环,且不能保证数据一致。postmaster 应视为致命,直接让整个服务器退出(FatalError 然后 ExitPostmaster(1))。
CONFIDENCE: medium

## I7.R13
ANSWER: system() 的 fork 子进程在 exec 前收到信号,会运行继承来的 PG 处理器,若调用 proc_exit 会在子进程里执行 atexit/on_exit 回调、释放共享内存里属于父进程的锁/注销 PGPROC,破坏父进程的共享状态。处理器应先检查 getpid() 是否仍是 MyProcPid,不是则直接 _exit。fork 之后必须立即更新 MyProcPid(在子进程里),把它设为新 pid。
CONFIDENCE: medium

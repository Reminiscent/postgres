## I3.R1
ANSWER: 先取关系重锁（插入 RowExclusiveLock，VACUUM ShareUpdateExclusiveLock），再 pin buffer，再取 buffer 内容锁；修改在临界区内做，并配合 MarkBufferDirty 和 WAL。插入只需 pin + 排他内容锁。pruning/整理页（移动元组、回收行指针）需要 cleanup lock（排他锁且只有自己持 pin，即 LockBufferForCleanup/ConditionalLockBufferForCleanup），因为其他后端可能只持 pin 就直接引用页内元组的指针。
CONFIDENCE: high

## I3.R2
ANSWER: 新解析的查询由 parser 在 parse analysis 时按 rellockmode 取锁。从 pg_rewrite 取出的规则树由 rewriter 里的 AcquireRewriteLocks 按每个 RTE 的 rellockmode 重新取锁；plancache 的树在复用前由 AcquirePlannerLocks/AcquireExecutorLocks 取锁，然后重新校验（失效则重新规划）。planner 和 executor 打开关系时用 NoLock（executor 会 Assert CheckRelationLockedByMe），因为锁已在前面阶段取得，并在事务结束前一直持有。
CONFIDENCE: high

## I3.R3
ANSWER: 提交方（B）一直持有 AccessExclusiveLock 到提交，在释放锁之前先发出 sinval 失效消息（AtEOXact_Inval 在 ResourceOwnerRelease 释放锁之前执行）。取锁方（A）拿到新锁后调用 AcceptInvalidationMessages（LockRelationOid 中，锁已持有时可以省略），从而看到新的 catalog。名字查找方面：RangeVarGetRelidExtended 会循环——先查 OID，再加锁，然后看 SharedInvalidMessageCounter 是否变化；若变化就重新查名字，OID 变了则释放旧锁后重试。
CONFIDENCE: high

## I3.R4
ANSWER: 必须对照锁冲突矩阵逐一核对会并发运行的操作：SELECT（AccessShare）、DML（RowExclusive）、其他 ALTER/DDL 子命令、VACUUM/ANALYZE、CREATE INDEX、pg_dump，以及不加锁读 catalog/relcache 的路径。会改变并发读者对数据或元组格式理解的变更不能降级：表重写、列类型变更、删列等改变 tupdesc/物理格式或查询语义的变更。原因是读者只持 AccessShareLock，用的是自己的 relcache/计划，若格式在其下改变会读到不一致的数据或崩溃；而且 catalog 扫描使用 catalog snapshot，不受 MVCC 保护。
CONFIDENCE: medium

## I3.R5
ANSWER: 会发生锁升级死锁：两个会话都先拿到弱锁（彼此兼容），然后都去申请更强的锁，各自等待对方的弱锁，形成死锁（由死锁检测器报错，其中一个事务失败）。正确做法：在 RangeVarGetRelidExtended 调用中一次就取最终需要的最强锁级别；回调只做权限/类型检查，或者也取同一级别的锁；后续执行函数用 NoLock 或相同级别，不再升级。
CONFIDENCE: high

## I3.R6
ANSWER: 遵循全局一致的顺序：先父表后子表/分区（自上而下，与 find_all_inheritors 和执行器相同），先堆表后其索引（与查询和 VACUUM 相同；REINDEX INDEX 也是先锁 heap 再锁 index）。同一层级的多个关系按 OID 等确定的顺序加锁。从一开始就取足够强的锁，避免中途升级。
CONFIDENCE: medium

## I3.R7
ANSWER: DROP 方通过 AcquireDeletionLock 对 X 取 AccessExclusiveLock，然后扫描依赖。创建方在记录依赖之前要对被引用对象取共享级别的对象锁（LockDatabaseObject / 记录依赖时的 LockNotPinnedObject 一类机制），拿到锁后还要重新检查 X 是否仍然存在，若已不存在就报错。这样 DROP 要么能看到新依赖并据此拒绝或级联，要么创建方发现 X 已被删除。
CONFIDENCE: medium

## I3.R8
ANSWER: 两个后端会互相等待，形成永久挂起（undetected deadlock）。LWLock 没有死锁检测，等待也不可中断，只能重启实例。必须按固定的全局顺序获取（例如 buffer 按块号或 AM 规定的方向：btree 从左到右、自下而上），尽量缩短持锁时间，不要在持有内容锁时去取另一个任意锁。
CONFIDENCE: high

## I3.R9
ANSWER: syscache 未命中时要读 catalog，可能需要读或锁同一个（或别的）catalog 页，造成自死锁（LWLock 不可重入）或 LWLock 顺序死锁；也可能触发失效处理、内存分配或 ERROR，而此时可能处于临界区。正确顺序：取 buffer 锁之前先完成所有 catalog 查找和失效消息的准备（例如提前算好需要的信息），持锁期间只做页修改和 WAL，放锁后再发送失效消息。
CONFIDENCE: medium

## I3.R10
ANSWER: 事务出错 abort 时 spinlock 不会被释放，elog(ERROR) 会让它一直被持有，其他进程空转，最终 "stuck spinlock" PANIC。系统调用可能耗时很长或阻塞，导致其他进程长时间空转。spinlock 只能持有几条指令的时间，不可嵌套也不可重入，每条代码路径上恰好获取一次、释放一次；持有期间不能 palloc、CHECK_FOR_INTERRUPTS，也不能取其他锁。
CONFIDENCE: high

## I3.R11
ANSWER: 不能直接使用。放锁期间页可能被修改：插入、prune、分裂、右链变化、空闲空间变化等。无锁读到的页头只能当作提示。重新加锁后必须再次校验（例如比较 LSN、检查元组/行指针状态、btree 向右移动），校验失败就重试。持有内容锁期间不应做会阻塞的 I/O（读其他页、扩展关系、pin VM 页等），应在加锁前完成；例外是 WAL 刷写，它本来就是必需的。
CONFIDENCE: medium

## I3.R12
ANSWER: 会形成死锁检测器看不见的死锁，永久挂起。重锁死锁检测器只能看到锁管理器中的等待边，网络、latch 或条件变量上的等待不在等待图里。互相等待应该表现为重锁等待（例如 XactLockTableWait、speculative token 这类锁），让检测器能看到；或者像并行查询那样用 lock group，让协作进程共享锁、不互相阻塞。
CONFIDENCE: high

## I3.R13
ANSWER: 主库：DDL 取 AccessExclusiveLock 时由 LogAccessExclusiveLock 写一条 XLOG_STANDBY_LOCK 记录（带 xid），running-xacts 记录中也带有锁信息。standby 的 startup 进程回放时用 StandbyAcquireAccessExclusiveLock 代该 xid 取 AEL；与本地查询冲突时，等待至 max_standby_*_delay，然后通过 ResolveRecoveryConflictWithLock 取消冲突的后端。standby 的普通后端只能取 RowExclusiveLock 及以下的锁，正常排队。锁在回放到该 xid 的 commit/abort 记录（或 COMMIT/ROLLBACK PREPARED）时释放；若 running-xacts 记录显示该 xid 已不在运行，也会清理（StandbyReleaseOldLocks）。
CONFIDENCE: high

## I3.R14
ANSWER: 顺序：第一遍扫 heap 时 prune 并把死元组标记为 LP_DEAD，收集 TID；然后对所有索引执行 ambulkdelete，删除指向这些 TID 的索引项；最后第二遍扫 heap，把这些行指针置为 LP_UNUSED，供复用。B-tree 在 bulkdelete 时对每个叶页取 cleanup lock（等待其他 pin 释放），保证没有扫描停在该页、手里还拿着即将被回收的 TID。这样就不会有扫描拿着旧 TID 回到 heap，读到被复用的槽位。
CONFIDENCE: high

## I3.R15
ANSWER: 删除时在页上记录一个 XID（btree 是 safexid，一个 full XID；GiST 类似；SP-GiST 的 redirect 元组记录 xid）。只有当该 XID 早于所有可能仍在进行的扫描/快照的 xmin 视界（GlobalVisCheckRemovableFullXid / GlobalVisTestIsRemovableXid）时，才能真正回收，即放入 FSM 或把 redirect 变为 placeholder。这保证没有扫描还会落到这个页上或沿链路访问它。在 standby 上，通过 reuse-page WAL 记录触发恢复冲突来保证同样的安全性。
CONFIDENCE: high

## I3.R16
ANSWER: CommitTransaction 的顺序：先写 commit WAL 记录（并按需刷盘），再更新 clog，然后 ProcArrayEndTransaction（此后对其他事务可见），接着 AtEOXact_Inval 发出 sinval 消息，最后 ResourceOwnerRelease 释放重锁。锁在失效消息发出之后才释放。子事务提交时，锁不释放，而是转交给父事务的 resource owner（LockReassignCurrentOwner）。PREPARE TRANSACTION 时，AtPrepare_Locks 把锁写入 2PC 状态，PostPrepare_Locks 把锁转移到该 prepared xact 的 dummy PGPROC，一直持有到 COMMIT/ROLLBACK PREPARED。
CONFIDENCE: high

## I3.R17
ANSWER: 等待者的循环：ResetLatch → 检查条件（满足则退出）→ WaitLatch → 重复。关键是先 ResetLatch 再检查条件，这样在检查之后才到达的 SetLatch 不会丢失。唤醒方：先更新共享状态（在锁内或用内存屏障），再 SetLatch。信号处理器里只能设置 volatile sig_atomic_t 标志并调用 SetLatch(MyLatch)，同时保存/恢复 errno；不能 elog、palloc、取锁，也不能调用非 async-signal-safe 的函数。
CONFIDENCE: high

## I3.R18
ANSWER: 其他会话临时表的页在对方的 local buffer 中，本会话无法正确访问。每条入口路径都必须检查 RELATION_IS_OTHER_TEMP，并报错 "cannot access temporary tables of other sessions"。如果只在读路径加了防护，预取或批量扩展仍会经由 shared buffer 或直接对文件操作：可能读到过时数据，或者写入/扩展的块与对方 local buffer 中的副本冲突，造成数据损坏。
CONFIDENCE: medium

## I3.R19
ANSWER: 释放 DSM 中的 LWLock 以及其他使用 DSM 的清理必须先于 DSM 段的 detach。需要访问 DSM 的回调应注册为 before_shmem_exit（或在 dsm_backend_shutdown 之前运行，并先执行 LWLockReleaseAll）。若顺序反了，释放锁时会访问已 unmap 的内存而崩溃；或者锁一直处于被持有状态，其他进程在该锁上永久挂起。
CONFIDENCE: medium

## I6.R1
ANSWER: 应该合并：把父 Var 的 varnullingrels 加到翻译后的子表达式上（与子表达式已有的 nullingrels 取并集，adjust_appendrel_attrs 里通过 add_nulling_relids 实现）。不能直接用父 Var 的标记覆盖，否则会丢掉子表达式自身的外连接标记（例如 UNION ALL 子查询上拉后带来的 nullingrels），导致 equal() 匹配失败、EC 或 targetlist 不一致，或者 setrefs 报 "wrong varnullingrels" 之类的错误。
CONFIDENCE: medium

## I6.R2
ANSWER: 可信的来源：已验证（非 NOT VALID）的 NOT NULL 约束，以及主键。不可信：NOT VALID 的约束，以及父表上 NO INHERIT 的约束（对子表不成立）。Var 自身不能被外连接置为 NULL（varnullingrels 必须为空）。对继承父表（inh 为真），只能使用对所有子表都成立的约束；子表本身按子表自己的约束判断。外连接 clone 出来的子句（has_clone/is_clone）的 nullingrels 各不相同，不能按某一份 clone 单独化简，应跳过，或保证所有 clone 得到一致的处理。
CONFIDENCE: medium

## I6.R3
ANSWER: volatile 表达式不能放进多成员的 EC（只能是单成员的排序 EC），外连接延迟的子句或非 strict/非 mergejoinable 算子也不能并入 EC。常量并入 EC 后会生成 x=const 的派生 qual，只在该 Var（带特定 nullingrels）有效的范围内成立；可空侧经外连接后的 Var 是另一个不同的成员。各成员只在它们都能被计算、且等值子句已被应用的计划节点及其之上才相等（EC 有 below/above outer join 的语义）。“相等”的含义由 ec_opfamilies（btree opfamily 的等值算子）和 ec_collation 决定。
CONFIDENCE: medium

## I6.R4
ANSWER: 在 EC 合并完成（root->ec_merging_done，即 deconstruct_jointree 和等值推导处理完毕）之前，不能构造 canonical PathKey。因为 canonical pathkey 引用 EC 指针，比较 pathkey 时靠指针相等；如果之后 EC 又被合并，早先建出的 pathkey 会指向已失效或被合并的 EC，不再是 canonical 的，排序匹配和冗余判断就会出错（代码里有 Assert 检查）。
CONFIDENCE: medium

## I6.R5
ANSWER: 不行，除非证明所用的相等性蕴含消费者使用的相等性。需要核对：消费者的等值算子属于唯一索引列的 opfamily（或者对 DISTINCT/GROUP BY 用 equality_ops_are_compatible 检查）；collation 一致，或至少两边都是 deterministic 的。例如唯一性基于 deterministic collation 证明，而 join 用的是 nondeterministic（大小写不敏感）比较，此时一行可能匹配多行，结果就错了。
CONFIDENCE: medium

## I6.R6
ANSWER: 应该在预处理之后的最终形式上判断（eval_const_expressions、SQL 函数内联、默认值展开都完成之后），因为这些步骤会展开函数体，暴露或消除 volatile 调用；对未预处理的形式做判断不能反映实际要执行的内容（内联本身不会增加 volatility）。变换必须保持 volatile 表达式的求值次数与原语义一致：不能复制（多次求值）、合并、缓存，也不能省略。
CONFIDENCE: low

## I6.R7
ANSWER: 只有不含聚合、volatile 函数、SRF 和子计划、只引用分组列的 qual 才可以移动（对 security barrier 子查询还要求 leakproof）。窗口函数：qual 不能引用窗口函数结果，且只能引用出现在所有窗口 PARTITION BY 中的列，才能下推到窗口计算之前。grouping sets：被某些分组集置为 NULL 的列（例如不在所有分组集中，或存在空分组集）不能移到 WHERE 或下推，因为在 WHERE 阶段该列尚未被置 NULL，语义会不同。
CONFIDENCE: medium

## I6.R8
ANSWER: 不行。按 security_level 排序，非 leakproof 的用户 qual 必须在 RLS 或安全屏障 qual 之后执行；只有 leakproof 的 qual 可以提前。估计选择率时读 pg_statistic，也只能在用户对列有 SELECT 权限且表上没有 RLS，或算子是 leakproof 时才使用直方图/MCV 等值（statistic_proc_security_check）。继承子 RTE 的权限和 RLS 检查以查询中实际点名的父表为准（RTEPermissionInfo 只对父表），子表自己的 RLS 不适用。
CONFIDENCE: medium

## I6.R9
ANSWER: 不能假设算子是 strict。外连接化简只能依据 strict（null-rejecting）条件，像 IS NOT DISTINCT FROM 这样的非 strict 条件不能用来把外连接化简为内连接。哈希 NOT IN 要求比较算子和哈希函数是 strict 的，否则退回普通 subplan。分区裁剪对布尔条件取反时，必须考虑 NULL：NOT (x = true) 不等价于 x = false，IS NOT TRUE 也包含 NULL。遇到非 strict 情况要放弃该捷径（检查 op_strict/func_strict）。
CONFIDENCE: medium

## I6.R10
ANSWER: inner_unique 表示每个外侧行至多匹配一个内侧行：nestloop 和 hash join 找到第一个匹配后立即转到下一个外侧行（与 SEMI 的 single_match 逻辑相同）。merge join 在此基础上还可以跳过 mark/restore（skip_mark_restore），但前提是所有连接条件都是 mergeclause。right semi/anti 情况下，匹配是从内侧行的角度标记的，找到匹配后标记该内侧行并继续，不能跳过其他外侧行。前提：唯一性证明完全基于该连接实际执行的连接条件，且所用相等语义一致。
CONFIDENCE: low

## I6.R11
ANSWER: 把 B 上的 qual 改写为引用 A（ChangeVarNodes 替换 relid），转移给 A；A=B 的连接 qual 变为 a.x = a.x，要替换为 a.x IS NOT NULL（列有 NOT NULL 约束时可直接去掉）。判断两个 qual 重复时，比较替换后的子句表达式（equal() 比较 clause 本身，而不是 RestrictInfo 的缓存字段）。所有遗留引用都要替换或清理：Var 的 varno/varnullingrels、PHV 的 ph_eval_at、EC 成员、RestrictInfo 的 relids、attr_needed、lateral、rowmark、all_baserels 等；按 relids 缓存的结果（RestrictInfo 缓存的 left/right relids、选择率、join_rel_hash）要重算或失效。遗漏的话，会出现 "variable not found in subplan target list" 之类的错误或错误的计划。
CONFIDENCE: medium

## I6.R12
ANSWER: targetlist 含 parallel-restricted 函数时，rel->consider_parallel 设为 false，该 rel 不生成 partial path，相关 path 的 parallel_safe=false。挂了 initPlan 的节点需重新计算并标记 parallel_safe（initPlan 若不安全，节点就变为不安全；SS_charge_for_initplans 会处理），不能再出现在 Gather 之下。partial path 必须经过 Gather 或 Gather Merge 才能成为可执行的计划；parallel-restricted 的部分只能在 Gather 之上由 leader 执行。
CONFIDENCE: medium

## I6.R13
ANSWER: leader 必须把影响语义的状态序列化给 worker：GUC（含 search_path）、事务快照和活动快照、事务状态（xid、子事务 xid）、combo CID、当前用户和安全上下文、数据库、已加载的库、relmapper、未提交的 enum、reindex 状态等。新增的全局状态也要在 InitializeParallelDSM/ParallelWorkerMain 中传递，否则 worker 的行为会不一致。并行模式下禁止写操作和分配 xid（报 "cannot assign TransactionIds during a parallel operation"）；需要 xid 时必须在进入并行模式之前，由 leader 分配。
CONFIDENCE: high

## I6.R14
ANSWER: 不能直接复用。分析和改写的结果依赖名字解析和 RLS。plancache 会记录 search_path（检查 SearchPathMatchesCurrentEnvironment），search_path 变了就重新分析。它还记录 rewriteRoleId 和 rewriteRowSecurity：如果计划依赖 RLS（dependsOnRLS），角色或 row_security 变化后要重新做分析和改写，以应用正确的策略。
CONFIDENCE: high

## I6.R15
ANSWER: 不能使用，要退回逐行插入。BEFORE 行触发器、volatile 默认值（nextval 除外）以及 WHERE 中的 volatile 函数，可能查询目标表或产生副作用；按语义，它们应该能看到之前各行已经插入的效果。批量化会推迟实际插入，改变它们看到的数据，从而改变语义（触发器还可能修改或丢弃行）。
CONFIDENCE: high

## I6.R16
ANSWER: 统计信息是不可信的输入。读取代码必须校验数组维度、类型、NULL、NaN、频率范围和排序等，遇到不合理的值时钳制范围或忽略，不能 Assert 失败、崩溃或越界访问。统计值只影响代价估计和计划选择，绝不能影响查询结果的正确性；坏统计最多导致计划变差。
CONFIDENCE: high

## I6.R17
ANSWER: 下游不会补做 collation 赋值：规划器和执行器默认 parse 阶段已设置好 exprCollation/inputcollid。负责的是解析器中这条新语法路径的 transform 代码，它必须自己对生成的表达式调用 assign_expr_collations/assign_list_collations。否则 inputcollid 为 0，运行时会报 "could not determine which collation to use"，或者得到错误的比较结果。
CONFIDENCE: high

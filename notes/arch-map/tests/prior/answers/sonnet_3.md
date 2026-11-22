## I3.R1
ANSWER: 先取表级关系锁(插入 RowExclusiveLock,VACUUM ShareUpdateExclusiveLock),再 pin buffer,再取 buffer 内容锁。插入只需页的 exclusive 内容锁;pruning/整理页(移动元组、改行指针)通常需 exclusive 内容锁,而 VACUUM 真正清理页(重排/删除)传统上要 cleanup lock(exclusive 锁且无其他 pin),因为别的后端可能持 pin 引用元组指针。
CONFIDENCE: medium

## I3.R2
ANSWER: 存储的规则/视图展开、来自 plancache 的树:解析/改写阶段(parser/rewriter)在打开关系时取锁,plancache 在复用缓存计划前用 AcquireExecutorLocks/AcquirePlannerLocks 对其中所有关系重新加锁并校验计划是否失效。执行器 ExecInitNode 打开关系时按需加锁。planner 用 NoLock 打开,因为 parse/rewrite 或 plancache 已经持有锁了。
CONFIDENCE: medium

## I3.R3
ANSWER: DDL 方在提交前(修改 catalog 后)发送失效消息,提交时 sinval 消息对其他后端可见;取锁方在拿到锁之后调用 AcceptInvalidationMessages 处理失效,再读 catalog,这样才能看到 B 提交后的状态。名字查找(RangeVarGetRelid)要处理锁等待期间名字被改名/删除:取锁后重新查名字,若 OID 变了就释放锁并重试。
CONFIDENCE: high

## I3.R4
ANSWER: 必须对照所有可能并发的操作核对冲突:普通 SELECT/INSERT/UPDATE/DELETE(AccessShare/RowExclusive)、VACUUM、其他 DDL、pg_dump、以及依赖该表 catalog 的缓存/计划。会改变表的物理存储结构(重写表、改变元组格式/行布局)的变更不能降级,因为并发读写者依赖旧的 tuple descriptor/物理布局,需要 AccessExclusiveLock 排除所有访问;另外会影响 SELECT 的 catalog 快照一致性(catalog 扫描非 MVCC)的变更也受限。
CONFIDENCE: medium

## I3.R5
ANSWER: 回调里先取弱锁,后面执行函数再升级到更强锁,会造成锁升级死锁:两个会话都持有弱锁(互相冲突于强锁),都在等对方释放而互相死锁。应在回调(或一开始)就直接取最终需要的最强锁,让 RangeVarGetRelidExtended 的取锁一次到位,不要之后再升级。
CONFIDENCE: medium

## I3.R6
ANSWER: 按固定的全局顺序取锁:先父表再子/分区(按继承/分区层次从上到下),同一层按 OID 顺序,表先于其索引。这与普通查询(先表后索引、先父后子)及其他 DDL 一致,避免相反顺序造成死锁。DETACH CONCURRENTLY 等要保证先锁父再锁分区,不能反向。
CONFIDENCE: medium

## I3.R7
ANSWER: 创建方在记录依赖前要对被引用对象 X 取锁(object lock,LockDatabaseObject,通常 AccessShareLock),取锁后再确认对象仍存在;DROP 方在删除前对 X 取 AccessExclusiveLock,并在 findDependentObjects 时检查。这样两者互斥,要么创建方先拿锁使 DROP 等待/报依赖错,要么 DROP 先完成创建方发现 X 不存在而失败,避免孤儿依赖。
CONFIDENCE: medium

## I3.R8
ANSWER: 相反顺序取两把 LWLock 会死锁,而且 LWLock 没有死锁检测(deadlock detector 只处理重量级锁),会永远挂住。应遵守固定的取锁顺序(全局约定,例如 buffer 内容锁按块号/先低后高),或避免同时持有。
CONFIDENCE: high

## I3.R9
ANSWER: 持有 catalog 页的排他 buffer 内容锁时调用 syscache 查找可能需要读 catalog 页(自己或其他页),引起 I/O、取同一 buffer 锁而自死锁,或在处理失效时递归访问 catalog,还可能触发 sinval 处理重建缓存。正确做法:先在取 buffer 锁之前完成 syscache 查找、构造好失效消息所需信息,再取 buffer 锁做修改,持锁期间只做纯内存操作(临界区)。
CONFIDENCE: medium

## I3.R10
ANSWER: spinlock 临界区必须极短,不能 elog(ERROR)(错误处理会 longjmp 而不释放 spinlock,且 spinlock 不被 abort 自动释放,会导致卡死)、不能做系统调用/阻塞/取其他锁/分配内存。spinlock 获取和释放必须成对且在同一函数内、一一匹配,不能嵌套 hold 很久;中断需被保持禁用(HOLD_INTERRUPTS)。
CONFIDENCE: medium

## I3.R11
ANSWER: 不能直接使用。放锁期间页可能被其他后端修改(元组移动、页被清理、分裂等),重新取锁后必须重新验证页状态(重读头字段、重新检查条件,必要时重试)。持有 buffer 内容锁期间不应做 I/O(尤其是可能阻塞的读),只持 pin 做 I/O,并在无锁读取头字段时要当作 hint。
CONFIDENCE: medium

## I3.R12
ANSWER: 这会形成死锁检测器看不到的隐藏死锁:P 持表锁等 Q,Q 等表锁(而 P 的等待在网络/外部,不在锁管理器里),或条件变量互等,死锁检测不了,会永久挂起。互相等待必须通过 PostgreSQL 的 heavyweight lock 管理器(锁等待)表现出来,这样 deadlock detector 才能发现并报错;不要用 latch/CV/网络等待隐藏依赖。
CONFIDENCE: medium

## I3.R13
ANSWER: 主库执行 DDL 时取 AccessExclusiveLock,并把它记录到 WAL(XLOG_STANDBY_LOCK)。standby 上 startup 进程重放该记录时获取同样的 AccessExclusiveLock;若与正在运行的 SELECT 冲突,等待 max_standby_streaming_delay,超时则取消冲突查询(recovery conflict)。普通后端只持 AccessShare 等读锁。该锁在 standby 上在重放主库该事务的 commit/abort 记录时释放。
CONFIDENCE: high

## I3.R14
ANSWER: 先从索引中删除指向死元组的索引项(vacuum 索引),之后才能回收 heap 死元组的 line pointer(TID),即先索引后 heap。持 pin 的索引扫描可能还要回 heap 取元组,所以 VACUUM 在该叶页上要取 cleanup lock(super-exclusive,等待所有 pin 释放)才删除项,这样扫描不会在 TID 被重用后读到错误元组。
CONFIDENCE: high

## I3.R15
ANSWER: 不能立即回收:必须等到所有可能仍在该页上看到/走到它的并发事务都结束,即删除该页的 xid 已经比所有活跃快照/事务都旧(older than the oldest xmin / GlobalVisState 判断,btree 用 safexid)后才能放入 FSM 重用。依据是 xid/可见性界限(safexid 对 GlobalVisState 已不再需要)。
CONFIDENCE: medium

## I3.R16
ANSWER: 提交时先写 commit WAL 记录并刷盘(持久化),标记 clog,再从 ProcArray 移除(使事务对别人可见),发送 sinval/处理失效,最后(在 ProcArray 之后)才释放重锁。子事务提交时,它的锁转交给父事务(不释放),子事务中止则释放。PREPARE TRANSACTION 时锁转交给 prepared 事务的 dummy PGPROC,继续持有直到 COMMIT/ROLLBACK PREPARED。
CONFIDENCE: medium

## I3.R17
ANSWER: 等待者:先 ResetLatch,再检查条件,条件不满足再 WaitLatch(顺序:reset→check→wait)。唤醒方:先更新共享状态,再 SetLatch。这样不丢唤醒。信号处理器里只能做异步信号安全的事:设置 volatile sig_atomic_t 标志、SetLatch、保存/恢复 errno;不能 palloc、elog、取锁、做复杂操作。
CONFIDENCE: high

## I3.R18
ANSWER: 访问其他会话的临时表(本地 buffer 在对方后端私有内存中)必须被拒绝(报错 "cannot access temporary tables of other sessions"),因为本地缓冲区只有拥有会话能访问。如果只在读路径加防护而预取/批量扩展/写路径没加,则这些路径可能错误地通过共享缓冲区访问对方临时表文件,造成数据不一致或损坏。
CONFIDENCE: medium

## I3.R19
ANSWER: 应先运行释放 LWLock 的清理(LWLockReleaseAll,在 shmem_exit 早期/ProcKill 之前),再卸载 DSM 段(dsm_backend_shutdown)。如果先 detach DSM,仍持有的 LWLock 在被释放时会访问已经取消映射的内存,导致崩溃/段错误。
CONFIDENCE: medium

## I6.R1
ANSWER: 翻译时不能直接用父 Var 的标记覆盖子 Var,因为子 Var 自己已有 varnullingrels 等外连接标记(来自子关系自身的内部外连接);应把父 Var 的 nullingrels 与子 Var 已有的合并(取并集/添加)到子 Var 上,保留两者。
CONFIDENCE: medium

## I6.R2
ANSWER: 只能信已验证且有效的 NOT NULL 约束(pg_attribute.attnotnull,已验证的,不是 NOT VALID、不是 deferred);CHECK 约束 IS NOT NULL 之类视情况。继承子表上父表的 NOT NULL 应对子表也成立;但外连接 clone 出来的子句、以及外连接可空侧的列会因外连接变成 NULL,所以要考虑 nullingrels,不能当作非空。
CONFIDENCE: low

## I6.R3
ANSWER: 含 volatile 函数、返回集合的表达式不能放入多成员 EC;常量并入 EC 只在同一 outer-join 层次/不越过外连接屏障时才合法(不能把外连接可空侧的常量下推导致错误)。EC 成员在 EC 所在的连接节点之上必须相等;相等的含义由 opfamily(btree 等值算子)及 collation 决定。
CONFIDENCE: low

## I6.R4
ANSWER: 在 EC 处理完成之前(equivalence class 合并完毕,即 generate_base_implied_equalities 之后/EC 定型之前)不能构造 canonical PathKey,因为 canonical pathkey 指向 EC,EC 还会合并变化,过早构造会让 PathKey 指向将被合并的 EC,导致不一致。
CONFIDENCE: medium

## I6.R5
ANSWER: 不行,除非相等语义一致。证明唯一性所用的相等(索引的 opfamily/collation)必须和消费者实际使用的连接相等一致:nondeterministic collation 下"相等"比字节相等更宽,唯一索引在另一 collation 下不保证唯一;opfamily 不同也不能直接套用。需要核对 opfamily 是否相同(或等价)以及 collation 是否匹配/确定性。
CONFIDENCE: medium

## I6.R6
ANSWER: volatile 判断应在表达式预处理(eval_const_expressions 简化/内联)之后的最终形式上做,因为内联函数可能暴露或消去 volatile 函数;不能在前面判断。变换必须保持 volatile 表达式的求值次数(不能重复、不能减少、不能改变求值时机/顺序)。
CONFIDENCE: low

## I6.R7
ANSWER: HAVING 条件不含聚合、不含 volatile 函数时可移到 WHERE。下推到子查询的 qual 必须只引用可安全下推的列:对 GROUP BY 只能引用分组列;对窗口函数只能引用所有窗口 PARTITION BY 的列(不能改变窗口输入行集);对 grouping sets 只能引用在所有 grouping set 中都出现的列(被置 NULL 的列不能下推),且不含 volatile、set-returning。
CONFIDENCE: medium

## I6.R8
ANSWER: 不行:非 leakproof 的用户 qual 不能排在 RLS/安全屏障 qual 之前,否则可能泄漏被隐藏的行;估计选择率读 pg_statistic 时,对非 leakproof 操作符也要检查权限(用户需有列的 SELECT 权限或操作符 leakproof),否则不能用统计。继承子 RTE 的权限/qual 检查应以父表(查询中引用的表)为准,而不是子表。
CONFIDENCE: medium

## I6.R9
ANSWER: 当算子可能不是 strict(如 IS NOT DISTINCT FROM,对 NULL 输入可能返回 true/非 NULL)时,外连接化简("strict 即可拒绝 NULL")、哈希 NOT IN、分区裁剪里对布尔条件取反的捷径不能假设"NULL 输入→NULL/false",必须检查 strictness,非 strict 就保守放弃该变换。
CONFIDENCE: medium

## I6.R10
ANSWER: 标记 inner_unique 后,执行器在找到第一个匹配后可以跳过当前外层行(或内层剩余元组)的其余内侧元组,直接前进到下一个外层行(merge join 前进外侧,hash join 同桶不再继续扫,nestloop 结束内侧扫描);right anti 同样标记已匹配后跳过。前提是内侧确实至多匹配一行(唯一性证明正确,且连接条件即证明所用的相等)。
CONFIDENCE: medium

## I6.R11
ANSWER: 被删关系 B 上的 qual 要迁移到 A(把 B 的引用替换为 A),并与 A 已有的 qual 比较,重复的去掉;判断重复时比较替换 relid 之后的表达式(equal())。遗留的对 B 的 relid 引用(裸 Var、按 relids 缓存的结果)必须全部重写为 A 的 relid,或清除/重新计算缓存,否则会悬空。
CONFIDENCE: medium

## I6.R12
ANSWER: 含 parallel-restricted 函数的 rel 要标记 consider_parallel=false(不能在 worker 中执行,只能在 leader 做);给计划节点挂 initPlan 时该节点要视为 parallel-unsafe/restricted,不能放进 partial path。partial path 要变成可执行计划必须经过 Gather(或 Gather Merge)节点。
CONFIDENCE: medium

## I6.R13
ANSWER: leader 要通过 DSM/共享内存把相同的环境序列化给 worker:事务快照、事务状态/xid、combo CID、GUC、当前用户/安全上下文、library/状态、pending 的 invalidation 等,使 worker 与 leader 一致。并行模式下禁止写操作和分配 xid(不允许修改数据库,xid 需在进入并行前分配),新增全局状态需要同步给 worker。
CONFIDENCE: medium

## I6.R14
ANSWER: 不能直接复用。plancache 缓存的分析/改写结果依赖 search_path、角色、row_security(RLS 策略依赖用户)等,这些变了名字解析和策略就不同,所以 plancache 检测到(如 search_path 变化或 RLS 相关变化)后要重新分析/重写/重新规划(重新 revalidate),否则会得到错误的对象或绕过安全。
CONFIDENCE: high

## I6.R15
ANSWER: BEFORE 行触发器、volatile 默认值、WHERE 中的 volatile 函数会使每行处理依赖于前面行的副作用/求值顺序,批量化(先收集多行再一起插入)会改变求值时机,可能让后一行看到的状态不同。因此这时不能用批量化(或需退回单行路径)。
CONFIDENCE: medium

## I6.R16
ANSWER: 把统计值视为不可信输入:不能假设有效(如概率不在 0..1、数组长度不匹配、值未排序、MCV 频率之和 > 1);读取代码要做防御性检查并 clamp。不合理的统计只应影响计划质量(选择率估计),不应导致崩溃、错误结果或安全问题。
CONFIDENCE: medium

## I6.R17
ANSWER: 下游不能假设已经被赋值,所以新语法路径的创建者(解析器该部分的代码)要自己负责对生成的表达式显式赋 collation(调用 assign_expr_collations 或手动设置 collid/inputcollid),否则规划器/执行器会看到 InvalidOid 的 collation,导致比较、排序错误或报错。
CONFIDENCE: medium

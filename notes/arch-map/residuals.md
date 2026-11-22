---
id: residuals
type: ledger
---
# 残差账本（第一阶段：只记录，不分析，不提方案）

来源：12 个子系统建模时顺手发现的。ID 不复用；已处理的条目把类型改成"已解决"并保留原文。基线 `4e0f8e930f3`。分类（GUIDE §5 的三类）与判定留到第二阶段。
`类型` 是第一阶段的粗标签：跨度（一条不变量要多个模块协作，D1）· 多写者（D2）· 职责（单模块承担多个 Secret）·
归属（文件的 Secret 与所归模块不贴合）· 依赖（实际 uses 关系超出地图所示）· 注释不符（代码注释与代码对不上）·
无校验（约定没有机械检查）· 缺卡（有 I 族状态但第一阶段未写卡）· 缺证据（原因只能标"假设"）。
孤儿（D6：节点追溯不到决策，或只有部分内容能追到；同时写出缺的那条上游，但不新增决策）· 泄漏（卡片的说法在某个条件下不成立）。
未落位（一条不变量的强制点在别的模块，而那个模块的卡片没有写它，所以暂时写在依赖方）· 未找准（按 GUIDE §3.1 写不成一句话，抽象可能没找准）。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X1 | [[S01.M01]] [[S01.M04]] [[S01.M07]] | 跨度 | [[I7]] 崩溃判定要 postmaster、PMChildFlags、quickdie 三方协作，重置还涉及共享内存重建和 [[S09]] | file:src/backend/postmaster/postmaster.c:2625-2668; file:src/backend/storage/ipc/pmsignal.c:95-99 |
| X2 | [[S01.M01]] | 职责 | `postmaster.c`（约 4.8k 行）同时承担状态机、子进程启动、崩溃处理 | — |
| X3 | [[S01.M07]] | 职责 | `tcop/postgres.c`（约 5.4k 行）把主循环、中断处理与协议消息处理（逻辑上属 [[S02]]）放在一个文件 | — |
| X4 | [[S01.M04]] | 归属 | 取消请求的发送在 `procsignal.c`，调用方在 [[S02]] 的 `backend_startup.c` | file:src/backend/storage/ipc/procsignal.c:741; file:src/backend/tcop/backend_startup.c:386 |
| X5 | [[S01.M04]] | 跨度 | ProcSignalBarrier 的各类处理者在其他子系统（smgr、校验和等），M1 不封闭在 [[S01]] | — |
| X6 | [[S01.M05]] | 无校验 | LWLock 加锁顺序只是约定，`LWLockAcquire` 里没有顺序断言；`held_lwlocks[]` 只用于出错释放和 `LWLockHeldByMe*`（GUIDE §12 待验证项的结论）[[I3]] | file:src/backend/storage/lmgr/lwlock.c:157-167,1150-1200; file:src/backend/storage/lmgr/README:242-244 |
| X7 | [[S02.M01]] | 归属 | `libpq/be-fe-helpers.c`（出站 libpq 连接选项白名单）与"会话与协议"关系弱 | — |
| X8 | [[S02.M01]] | 跨度 | 认证流程跨到 [[S01]]：`ClientAuthentication()` 由 `postinit.c` 调用，EXEC_BACKEND 下还重新 `load_hba()` | file:src/backend/utils/init/postinit.c:232,268 |
| X9 | [[S02.M03]] | 跨度 | 协议同步不变量跨 2 个子系统：`ignore_till_sync` 与失步后 FATAL 在 `tcop/postgres.c`（[[S01]]），`PqCommReadingMsg` 由本模块维护 | file:src/backend/tcop/postgres.c:4666 |
| X10 | [[S02.M06]] | 无校验 | 计划缓存的前提"任何需要重规划的表修改都会产生 relcache inval"靠 [[S11]] 与本模块共同维持，没有机械检查 [[I4]] | file:src/backend/utils/cache/plancache.c:33 |
| X11 | [[S02.M06]] | 职责 | 同一模块两套失效机制：plancache 走 inval 回调，`funccache.c` 比较 pg_proc 元组的 xmin+tid | — |
| X12 | [[S02.M08]] | 依赖 | NOTIFY 写者的串行化借用"database 0"上的重锁，与 [[S07]] 的锁标签隐式耦合（注释自称 pretty ugly） | file:src/backend/commands/async.c:1290 |
| X13 | [[S03.M02]] [[S03.M05]] | 跨度 | 解析期取锁的不变量 [[I3]] 跨 4 处：分析时取锁、`AcquireRewriteLocks`、plancache 再次调用（[[S02]]）、ruleutils 调用（[[S12]]） | file:src/backend/utils/cache/plancache.c:850; file:src/backend/utils/adt/ruleutils.c:3595 |
| X14 | [[S03.M06]] | 跨度 | RLS 不变量跨本模块、计划缓存（[[S02]]）、`commands/policy.c`（[[S11]]） | — |
| X15 | [[S03.M05]] | 职责 | `rewriteHandler.c` 承担规则展开、自动可更新视图、默认值与目标列、RLS 注入与递归检查、重新取锁、已删除列修正 | — |
| X16 | [[S03.M05]] | 依赖 | `rewriteManip.c` 被 [[S04]] 的 `tlist.c` `prepjointree.c` `paramassign.c` 包含，实为通用 Query 树操作库 | — |
| X17 | [[S04.M13]] [[S06.M07]] | 多写者 | `pg_class.relpages/reltuples` 两个写者事务语义不同：统计导入用事务性 `CatalogTupleUpdate`，`vac_update_relstats` 就地非事务覆盖；注释称"一致"但只对上了锁 | file:src/backend/statistics/relation_stats.c:159,205; file:src/backend/commands/vacuum.c:1422 |
| X18 | [[S04.M05]] | 跨度 | nullingrels 表示（[[I6]]）至少跨 4 个模块：[[S03]] 解析、预处理、语义推理、计划生成 | commit:2489d76c490 |
| X19 | [[S04.M05]] | 职责 | `clauses.c` 混合常量折叠与函数内联（预处理）和 volatility/strictness/parallel-hazard 分类 | — |
| X20 | [[S04.M10]] | 依赖 | `partbounds.c` 也被 [[S11]] 的 `tablecmds.c` `partdesc.c` 用来校验和构建分区边界；`partprune.c` 产出的裁剪步骤也被 [[S05]] 的 `execPartition.c` 使用 | — |
| X21 | [[S04]] | 归属 | `utils/adt/selfuncs.c` 同时含各索引 AM 的 `amcostestimate`，后者更贴近 [[S06]] | file:src/backend/utils/adt/selfuncs.c:7754 |
| X22 | [[S04]] | 缺证据 | D-geqo 的原因（穷举随连接数爆炸）README 没有原因句，标为假设 | file:src/backend/optimizer/README:10 |
| X23 | [[S05.M09]] | 职责 | `commands/trigger.c` 同时承担触发器 DDL（pg_trigger 维护）和 AFTER 触发器运行时队列 | — |
| X24 | [[S05.M09]] | 跨度 | 触发器队列不变量 D1=3：执行生命周期、触发器模块、[[S07]]（`xact.c` 4 处调用点）[[I5]] | file:src/backend/access/transam/xact.c:2319,2984 |
| X25 | [[S05.M08]] | 跨度 | EvalPlanQual 不变量 D1=3：本模块、[[S06]]（`TUPLE_LOCK_FLAG_FIND_LAST_VERSION`）、[[S07]]（快照）[[I2]] | file:src/backend/executor/nodeModifyTable.c:1738-1773 |
| X26 | [[S05.M10]] | 跨度 | 并行模式的写限制在两处检查：执行器和 [[S01]] 的 parallel.c/CCI | file:src/backend/executor/execMain.c:834-835 |
| X27 | [[S05.M08]] | 依赖 | `execReplication.c` 只被 [[S10]] 使用；提交 4da597edf1b 称它"pokes through the slot abstraction"，是否已修未核实 | commit:4da597edf1b |
| X28 | [[S05]] | 依赖 | `utils/sort/logtape.c` `sharedtuplestore.c` 依赖 [[S08]] 的 BufFile/SharedFileSet | — |
| X29 | [[S05]] | 归属 | `utils/misc/queryenvironment.c` 的主要使用者是触发器过渡表和 [[S03]]，归 SPI 模块存疑 | — |
| X30 | [[S06.M08]] | 注释不符 | `visibilitymap.c` 注释说"only VACUUM currently sets visibility map bits"，但 on-access pruning 也设置 VM | file:src/backend/access/heap/visibilitymap.c:81-82; file:src/backend/access/heap/pruneheap.c:250-268 |
| X31 | [[S06.M08]] | 注释不符 | `visibilitymap.c` 文件头说 VM 变更"not separately WAL-logged"；现在 VM buffer 作为块引用登记在 heap 记录里，措辞过时 | file:src/backend/access/heap/visibilitymap.c:37; file:src/backend/access/heap/heapam.c:2178 |
| X32 | [[S06.M07]] | 跨度 | relfrozenxid 不变量 D1=4–5：vacuumlazy、pruneheap、heapam 的 freeze plan、VM、`vacuum.c` [[I2]] | file:src/backend/access/heap/vacuumlazy.c:775-797 |
| X33 | [[S06.M04]] | 多写者 | heap 页有 5 个写者模块（D2=5）：heap 核心、pruning、vacuum、redo、表重写 | — |
| X34 | [[S06.M04]] | 职责 | `heapam.c` 承担增删改、行锁、freeze plan 构造、inplace update | — |
| X35 | [[S06]] [[S05]] | 归属 | 提交 ddce1da5b1b 把 index-only scan 的 VM 检查从执行器移进 heapam，[[S05]]/[[S06]] 的边界发生过移动 | commit:ddce1da5b1b |
| X36 | [[S06.M07]] | 依赖 | `commands/vacuum.c` 也分派 ANALYZE（`analyze.c` 属 [[S04]]）；`vac_update_relstats` 就地写 [[S11]] 的 pg_class 行 | — |
| X37 | [[S06.M01]] | 缺卡 | table AM 接口承担 [[I8]]（扩展 ABI）但无自有状态，未写卡 | — |
| X38 | [[S07.M01]] | 跨度 | 提交顺序不变量跨 5 个模块（事务状态机、提交状态、ProcArray、[[S09]]、[[S11]]），redo 侧手工重复同一顺序 [[I2]] | file:src/backend/access/transam/xact.c:6264-6292 |
| X39 | [[S07.M02]] | 多写者 | 回卷保护的各个 limit 有三个写者：vacuum（[[S06]]）、checkpoint/redo（[[S09]]）、multixact 自己 | — |
| X40 | [[S07.M01]] | 跨度 | `xact.c` 注释记录开发中三次有人认为某个簿记调用不必要——隐藏耦合的信号 | file:src/backend/access/transam/xact.c:6255-6259 |
| X41 | [[S07.M07]] [[S07.M08]] | 无校验 | 锁分区的加锁顺序与 SSI 的 LWLock 顺序都只靠注释约束 [[I3]] | file:src/backend/storage/lmgr/README:239-248 |
| X42 | [[S07.M05]] | 职责 | `procarray.c` 承担运行中事务集合、视界计算、KnownAssignedXids、恢复冲突信号、slot xmin | — |
| X43 | [[S07.M09]] | 归属 | `ResolveRecoveryConflictWithLock` 由 [[S01]] 的 `proc.c` 调用，锁等待循环与 standby 冲突逻辑跨子系统 | file:src/backend/storage/lmgr/proc.c:1479 |
| X44 | [[S07.M06]] | 归属 | `combocid.c` 隐藏的是元组头 cmin/cmax 的叠用，更接近 [[S06]] 的元组格式 | — |
| X45 | [[S08.M04]] | 注释不符 | `sync.c` 注释说"standalone backend or startup process: fsync state is local"，但 `InitSync` 只给单用户后端和 checkpointer 建 `pendingOps` | file:src/backend/storage/sync/sync.c:132,588 |
| X46 | [[S08.M11]] | 职责 | `bgwriter.c` 除回写脏页外还定期写 `xl_running_xacts`（[[S07]]/[[S09]] 的职责） | file:src/backend/postmaster/bgwriter.c:248-262 |
| X47 | [[S08.M12]] | 跨度 | 校验和状态机分在两处：`datachecksum_state.c` 与 [[S09]] `xlog.c` 的 `SetDataChecksumsOn/Off` | file:src/backend/access/transam/xlog.c:4888 |
| X48 | [[S08.M13]] | 跨度 | wal_level=minimal 下跳过 WAL 的不变量 D1=4：storage.c、bufmgr、[[S06]]、[[S09]] [[I1]] | readme:src/backend/access/transam/README#Skipping WAL for New RelFileLocator |
| X49 | [[S08.M01]] | 职责 | `bufmgr.c` 承担 pin/锁协议、IO 状态、AIO 的 stage 与完成回调、WAL 强制、写回调度 | file:src/backend/storage/buffer/bufmgr.c:8336 |
| X50 | [[S08.M04]] | 无校验 | `RegisterSyncRequest` 的重试循环不检查中断（代码里有 XXX 注释） | file:src/backend/storage/sync/sync.c:600-604 |
| X51 | [[S08]] | 缺证据 | D-own-buffer-cache（为什么自建 buffer 缓存）没有找到直接依据，标为假设 | — |
| X52 | [[S09.M02]] | 职责 | `xlog.c`（10,731 行）承担插入、刷盘、段管理、pg_control、StartupXLOG、checkpoint/restartpoint 执行、备份起止；checkpointer 与备份模块的实际工作都在它里面 | — |
| X53 | [[S09.M02]] | 多写者 | `pg_control` M2≥3：startup、checkpointer、恢复期间任何执行 `XLogFlush` 的进程（更新 minRecoveryPoint） | — |
| X54 | [[S09.M03]] | 跨度 | checkpoint 顺序不变量跨 4 个模块：xlog.c、checkpointer、[[S08]]、[[S07]]（`CheckPointGuts` 调 SLRU、buffer、sync、slot、2PC） | — |
| X55 | [[S09.M05]] | 跨度 | 一致点不变量跨 4 个模块：恢复驱动、xlog.c、[[S07]]、[[S01]] | file:src/backend/access/transam/xlogrecovery.c:2226-2269 |
| X56 | [[S09.M07]] | 跨度 | timeline 规则分散三处：选择在 `xlog.c`，切换检查在 `xlogrecovery.c`，文件读写在 `timeline.c` | — |
| X57 | [[S09.M10]] [[S09.M11]] | 已解决 | WAL 摘要与等待 LSN 都拥有共享状态，第一阶段未写卡（已解决：两个模块都已写卡） | — |
| X58 | [[S09.M05]] | 归属 | `xlogrecovery.c` 自提交 70e81861fad 从 `xlog.c` 拆出，但 restartpoint 创建和恢复结束切换仍留在 `xlog.c` | commit:70e81861fad |
| X59 | [[S09.M10]] | 已解决 | 未核实 `KeepLogSeg` 保留段时是否考虑 WAL 摘要进度（`GetOldestUnsummarizedLSN` 在 `xlog.c` 无调用者）（已解决：`KeepLogSeg` 会调用 `GetOldestUnsummarizedLSN`，见 file:src/backend/access/transam/xlog.c:8958-8970） | — |
| X60 | [[S09]] [[S07]] [[S06]] | 缺证据 | "不做 undo"与"可见性判定忽略残留元组"之间的因果没有找到提交或讨论原文，三处决策都把这一环标为假设 | — |
| X61 | [[S10.M11]] | 注释不符 | `origin.c` 注释说创建/删除 origin 需对 `pg_replication_slot` 加排他锁；代码锁的是 `pg_replication_origin`（创建 ExclusiveLock，删除 RowExclusiveLock） | file:src/backend/replication/logical/origin.c:47-49,317,467 |
| X62 | [[S10.M05]] | 跨度 | slot 的 xmin 不变量跨 4 个以上模块，写者分散在 walsender、解码、checkpointer、startup [[I2]] | file:src/backend/access/transam/xlog.c:8202; file:src/backend/storage/ipc/standby.c:506 |
| X63 | [[S10.M05]] | 依赖 | slot 数组被 [[S06]] 的 REPACK 共用（`max_replication_slots + max_repack_replication_slots`） | file:src/backend/replication/slot.c:1267 |
| X64 | [[S10.M11]] | 跨度 | apply 进度在 [[S07]] `xact.c` 的提交临界区里推进 | file:src/backend/access/transam/xact.c:1474-1499 |
| X65 | [[S10.M08]] | 依赖 | `logicalctl.c` 的停用动作在 checkpointer（[[S09]] 的进程）里执行 | — |
| X66 | [[S10.M13]] [[S10.M09]] | 职责 | `worker.c`（6.5k 行：apply、流式、两阶段、failover、死元组保留）与 `reorderbuffer.c`（5.8k 行）单文件职责重 | — |
| X67 | [[S10.M03]] | 依赖 | libpqwalreceiver 被 walreceiver、apply、tablesync、slotsync、subscriptioncmds 跨模块使用 | — |
| X68 | [[S10.M01]] | 职责 | `walsender.c` 还执行复制命令（`exec_replication_command`），部分属 [[S02]] 的关注点 | — |
| X69 | [[S10.M02]] [[S10.M06]] [[S10.M14]] | 已解决 | walreceiver（flushedUpto）、slotsync、tablesync 拥有 [[I1]]/[[I2]] 状态，第一阶段未写卡（已解决：三个模块都已写卡） | — |
| X70 | [[S10]] | 缺证据 | D-physical-rep-ships-wal（复用 redo 路径）与 D-logical-decoding-on-wal（相对触发器的取舍）的原因在提交信息里没有，标为假设 | commit:40f908bdcdc; commit:b89e151054a |
| X71 | [[S11.M09]] | 跨度 | "取到锁即已看到失效"的接收点全在 [[S11]] 之外：[[S07]]（提交顺序、`lmgr.c` 调用点）、[[S06]]（`relation.c`）[[I4]] | file:src/backend/storage/lmgr/lmgr.c:117; file:src/backend/access/common/relation.c:154 |
| X72 | [[S11.M04]] | 无校验 | 已持有所需锁时 `LockRelationOid` 不调 Accept，可能漏看变更（注释记录的已知缺口）[[I4]] | file:src/backend/catalog/namespace.c:469 |
| X73 | [[S11.M09]] | 无校验 | 多数派生缓存在构建期间可能"born stale"，只有 relcache 会重试；正确性依赖 DDL 取 AccessExclusiveLock 这一未检查的约定 [[I4]] | file:src/backend/utils/cache/inval.c:68 |
| X74 | [[S11.M12]] | 职责 | `tablecmds.c`（23.5k 行）承担 ALTER TABLE 的锁策略、pass 排序、重写判定、分区 ATTACH/DETACH | — |
| X75 | [[S11.M12]] [[S11.M01]] | 跨度 | 并发建索引协议 D1=4：`indexcmds.c`、`index.c`、procarray/lmgr 的等待、HOT；其中 `validate_index`、`index_concurrently_*` 在 `catalog/index.c`，逻辑上属 DDL 模块 | — |
| X76 | [[S11.M09]] | 跨度 | "非事务性变更必须在离开临界区前发出失效"跨 [[S06]]（inplace update）、[[S08]]（smgr）、relmapper | file:src/backend/utils/cache/inval.c:97 |
| X77 | [[S11.M08]] | 跨度 | relmapper 的提交点与事务提交不是原子的，因此 mapped catalog 的重写被限制在 VACUUM FULL/CLUSTER；约束跨 relmapper、[[S06]]、[[S07]] | — |
| X78 | [[S11]] | 归属 | `src/backend/catalog/README.attver.md`（属性版本的新文档）未反映在地图里 | — |
| X79 | [[S11.M13]] [[S11.M04]] | 已解决 | 数据库/表空间 DDL（DROP DATABASE 先 inplace 标记无效再删目录）与命名空间（失效到达时重试查找）拥有状态，第一阶段未写卡（已解决：两个模块都已写卡） | — |
| X80 | [[S11.M03]] | 无校验 | 依赖图的完整性靠每个 DDL 自己调用 `recordDependencyOn*`，没有找到集中检查（假设，未穷尽搜索） | — |
| X81 | [[S12.M03]] | 跨度 | 临界区不变量跨 3 处：`CritSectionCount` 定义在 [[S01]]，PANIC 升级在 `elog.c`，分配断言在 `mcxt.c` [[I5]] | file:src/backend/utils/error/elog.c:375 |
| X82 | [[S12.M03]] | 无校验 | PG_TRY 的规则（必须 rethrow 或中止、局部变量 volatile、FATAL 不被捕获）只是注释，所有调用者自觉遵守 | file:src/include/utils/elog.h:321 |
| X83 | [[S12.M04]] | 依赖 | ResourceOwner 对锁和 AIO 句柄是硬编码处理，直接调用 [[S07]] 与 [[S08]]，不经 `ResourceOwnerDesc` | file:src/backend/utils/resowner/resowner.c:742,753 |
| X84 | [[S12.M07]] | 跨度 | 累积统计的生命周期分散在 4 个子系统：加载/丢弃在 `xlog.c`（[[S09]]），关机写出由 checkpointer（[[S09]]）与 `postinit.c`（[[S01]]）登记，drop 的回放在 `xact.c`（[[S07]]） | — |
| X85 | [[S12]] | 归属 | `libpq/be-fsstubs.c` 文件头自称"should be moved to a more appropriate place" | — |
| X86 | [[S12]] | 归属 | `tsearch/ts_selfuncs.c` `ts_typanalyze.c` 按 Secret 更接近 [[S04]]（与 `selfuncs.c` 归 4 的规则相同），现随目录归 [[S12]]；`utils/hash/pg_crc.c` 是 CRC 表，与所在模块的 Secret 不符 | — |
| X87 | [[S12.M06]] | 缺证据 | `commands/variable.c` 的 check/assign 钩子直接写 [[S07]]/[[S02]] 的变量（假设，没有逐个打开核实）。GUC 的事务性值栈原记为缺卡，已写卡 | file:src/backend/access/transam/xact.c:2523,3050 |
| X88 | [[S01]] [[S05]] | 归属 | 个别不变量在 I1–I8 里没有贴切的族，写手按类比归类：主共享内存段同址映射、ProcSignalBarrier（类比 [[I4]]）、Plan 树只读（类比 [[I5]]） | — |
| X89 | [[S09.M07]] [[S09.M08]] | 已解决 | timeline 与归档两张卡追溯不到现有决策；缺的上游是"WAL 除崩溃恢复外还服务于 PITR 与日志传送，所以要归档，归档恢复后总是分叉新 timeline" | file:src/backend/access/transam/README:402-404; file:src/backend/access/transam/xlog.c:6645-6656 |
| X90 | [[S09.M02]] | 已解决 | "采用预写日志：数据页惰性写出，只需先把 WAL 刷到页 LSN"本身没有决策；WAL 先于数据只能间接由 [[D-own-buffer-cache]]、[[D-wal-redo-only]] 推出（已解决：预写规则就是 [[D-wal-redo-only]] 的定义，在 buffer 写出时强制是因为 [[D-own-buffer-cache]]，原始证据补进该记录） | file:src/backend/access/transam/README:407-415 |
| X91 | [[S05.M09]] | 孤儿 | 部分：AFTER 触发器的排队与延迟触发（按查询层排队、只存 ctid、提交前循环触发）追不到决策 | file:src/backend/commands/trigger.c:3849-3923,5323-5374 |
| X92 | [[S07.M04]] | 已解决 | 部分：multixact 的上游是"行锁记在元组头而不是共享锁表，因为被锁行数没有上界"，现有决策里没有 | file:src/backend/access/heap/README.tuplock:4-12 |
| X93 | [[S07.M01]] [[S07.M06]] | 孤儿 | 部分：两阶段提交（prepared 事务以 dummy PGPROC 存活，状态在 WAL 并于 checkpoint 落到 pg_twophase）与 combo CID 都没有决策 | file:src/backend/access/transam/twophase.c:24-35,1225-1244 |
| X94 | [[S06.M12]] | 孤儿 | 部分：nbtree 采用 Lehman-Yao B-link 树这一选择没有决策，核心并发只能追到 [[F2]] | file:src/backend/access/nbtree/README:6-30,57-68 |
| X95 | [[S06.M08]] | 孤儿 | 部分：visibility map（保守的单向位，供 vacuum 跳页与 index-only scan 使用）自身没有决策 | file:src/backend/access/heap/visibilitymap.c:23-48 |
| X96 | [[S08.M13]] [[S08.M04]] | 孤儿 | 部分：wal_level=minimal 下新建的 relfilenumber 跳过 WAL、改为提交前 fsync，没有决策；它推出 pendingSyncs 与主 fork 首段的延迟 unlink | readme:src/backend/access/transam/README#Skipping WAL for New RelFileLocator; file:src/backend/storage/smgr/md.c:284-299; file:src/backend/catalog/storage.c:741-860 |
| X97 | [[S08.M13]] | 孤儿 | 部分："关系文件的创建与删除不可回滚：创建立即执行，删除推迟到提交后"没有决策，pendingDeletes 只能追到 [[F1]] | file:src/backend/catalog/storage.c:43-56 |
| X98 | [[S08.M05]] | 孤儿 | 部分：引入 AIO（IO 句柄在共享内存、任意后端可完成、可在临界区内发起）没有决策 | file:src/backend/storage/aio/README.md:7-15,177-229 |
| X99 | [[S12.M03]] | 已解决 | 部分："临界区内 ERROR 升级为 PANIC：以崩溃加 redo 代替撤销做了一半的共享状态修改"没有决策，只能追到 [[F1]]（原因是假设）（已解决：临界区升级 PANIC 的原始提交和当前理由已补进 [[D-wal-redo-only]] 的派生；与"没有 undo"的联系仍是推断，见 [[X175]]） | file:src/include/miscadmin.h:79-84; file:src/backend/utils/error/elog.c:375-376 |
| X100 | [[S11.M12]] | 孤儿 | 部分：ALTER TABLE 锁级别按子命令分级并集中决定、并发建索引的多事务协议，都没有决策 | commit:2dbbda02e7e; commit:e5550d5fec6; file:src/backend/commands/tablecmds.c:4648-4683; file:src/backend/catalog/index.c:3410-3465 |
| X101 | [[S10.M13]] [[S10.M09]] | 孤儿 | 部分：内建逻辑复制（发布订阅 DDL、pgoutput、订阅端 worker）与"解码内存有界（溢出到磁盘或流式下发）"没有决策 | commit:665d1fad99e; file:src/backend/replication/logical/reorderbuffer.c:33-37,52-56 |
| X102 | [[S01.M03]] [[S01.M08]] | 孤儿 | 部分：EXEC_BACKEND（没有 fork 的平台用 fork+exec，子进程重新附着共享内存、重建全局状态）没有决策 | file:src/backend/postmaster/launch_backend.c:15; file:src/backend/storage/ipc/shmem.c:96-103 |
| X103 | [[S02.M05]] | 孤儿 | 部分：portal 可跨事务存活（WITH HOLD 游标在提交时物化；过程内事务控制下自动转成 held）没有决策 | commit:056a5a3f63f; file:src/backend/utils/mmgr/portalmem.c:715-721 |
| X104 | [[S03.M04]] | 孤儿 | 部分：规则系统本身（规则存 pg_rewrite、改写期展开、视图即 ON SELECT 规则）没有决策 | file:src/backend/rewrite/rewriteHandler.c:13-17; file:src/backend/rewrite/rewriteDefine.c:318 |
| X105 | [[S04.M11]] | 孤儿 | 部分："规划代价由 ANALYZE 抽样、存于 pg_statistic 的统计驱动"没有决策，统计采集只能追到 [[F4]] | file:src/backend/commands/analyze.c:1739 |
| X106 | [[S12.M01]] | 已解决 | 卡片 Inv(2)"pfree/repalloc 经 chunk 头分派"对 bump 上下文不成立：它的 chunk 没有头，只能整体 reset 或删除（已解决：该条不变量已改为带 bump 例外的说法） | file:src/backend/utils/mmgr/bump.c:6-13 |
| X107 | [[S01]] [[S12]] | 孤儿 | 未评估：没有模块卡的 52 个 L1 模块没有逐个标 Derives from（[[S03]] 除外）；77 条决策里有 12 条没有被任何模块卡引用 | — |
| X108 | [[S06]] [[S09]] | 缺证据 | 73 条决策里 33 条的原始原因未考证；力到决策的归类是地图作者的判断；GUIDE §1.4 所举的两个提交只有标题级信息，不足以支持"不覆盖写的原始动机是时间旅行" | commit:32cd09ac6d4; commit:30659d43eb7 |
| X109 | [[S06.M10]] | 孤儿 | TOAST（超大值行外存储）追溯不到任何现有决策 | — |
| X110 | [[S09.M11]] | 孤儿 | 等待 LSN 追溯不到任何现有决策；引入它的提交只描述功能，没有给出理由 | commit:3b4e53a075e; commit:447aae13b03 |
| X111 | [[S12.M06]] | 孤儿 | "GUC 设置是事务性的：SET 随（子）事务中止撤销，函数的 SET 子句在退出时恢复"没有决策 | file:src/backend/utils/misc/README:126-205; file:src/backend/utils/misc/guc.c:2160-2194 |
| X112 | [[S12.M08]] | 孤儿 | "后端状态每槽单写者，读者靠变更计数无锁读（更新便宜、读可能贵）"没有决策，只能追到 [[F4]] | file:src/include/utils/backend_status.h:106-107 |
| X113 | [[S08.M08]] | 孤儿 | "unlogged 关系不写 WAL，崩溃后用 init fork 重置"没有决策，只能追到 [[F4]] | file:src/backend/storage/file/reinit.c:37-45 |
| X114 | [[S11.M13]] [[S11.M04]] | 孤儿 | 部分："数据库与表空间的目录操作不是事务性的：在事务块外执行，靠 inplace 的无效标记、强制 checkpoint 和 WAL 保持一致"没有决策；临时 schema 的生命周期也没有决策 | commit:c66a7d75e65; file:src/backend/commands/dbcommands.c:1863-1884 |
| X115 | [[S10.M06]] [[S10.M08]] | 孤儿 | 部分：failover 槽同步、replica 级别下按需开关逻辑解码，都没有决策 | file:src/backend/replication/logical/slotsync.c:11-56; file:src/backend/replication/logical/logicalctl.c:5-53 |
| X116 | [[S06.M13]] [[S06.M14]] [[S06.M15]] [[S06.M16]] [[S06.M17]] [[S06.M18]] | 孤儿 | 部分：各索引 AM 的核心协议（gin 的 pending list、gist 的 NSN、hash 的桶分裂、spgist 的 redirect）以及 brin、序列都只能追到力，没有决策 | — |
| X117 | [[S02.M06]] [[S09.M11]] [[S11.M10]] | 归属 | I 族归类不贴切或未核实：临时计划在 TransactionXmin 前移后失效（暂标 [[I2]]，未核实是否为正确性所必需）；等待 LSN 的唤醒协议（暂标 [[I3]]）；leader 与 worker 对匿名 record typmod 的一致（暂标 [[I6]]） | file:src/backend/utils/cache/plancache.c:1152-1160 |
| X118 | [[S11.M02]] [[S12.M11]] | 跨度 | "未提交的 enum 值不得进入索引"跨两个子系统：事务内状态在 `pg_enum.c`，强制点 `check_safe_enum_use` 在 `utils/adt/enum.c` | file:src/backend/catalog/pg_enum.c:40-47 |
| X119 | [[S07.M01]] | 未落位 | 提交/中止序列里有几步由别的模块依赖、但事务状态机的卡片没有写：NOTIFY 在 clog 标记提交之前写队列（[[S02.M08]]）；关系文件的删除推迟到释放锁之后（[[S08.M13]]）；LSN 等待者在事务中止时移出等待堆（[[S09.M11]]）；统计项的删除写进提交记录并在 standby 回放（[[S12.M07]]）（2026-11-13 已落位：中止序列由 [[S07.M01]] Inv(14)(15) 补上，含中止开头清掉 LSN 等待状态；仍缺：NOTIFY 在 clog 标记提交之前写队列、关系文件删除推迟到释放锁之后、统计项删除写进提交记录这几个提交步骤） | file:src/backend/access/transam/xact.c:2500-2509 |
| X120 | [[S09.M05]] | 未落位 | 恢复驱动的卡片没有写两条别人依赖它的规则：startup 只重放 walreceiver 的 `flushedUpto` 以下的 WAL（[[S10.M02]]）；unlogged 关系的清理必须在开放 hot standby 连接之前完成（[[S08.M08]]） | file:src/backend/access/transam/xlogrecovery.c:3920-3927 |
| X121 | [[S08.M01]] [[S12.M04]] [[S07.M07]] | 未落位 | buffer 管理的卡片没有写"页在 IO 中时不设 hint bit"（[[S06.M05]] 依赖）；ResourceOwner 的卡片没有写 buffer pin 与 AIO 句柄这两类资源的释放语义（[[S08.M01]] [[S08.M05]] 依赖）；重锁的卡片没有提 ResourceOwner 在子事务提交时把锁转给父（写在 [[S12.M04]]） | — |
| X122 | [[S01.M07]] [[S07.M09]] [[S11.M09]] [[S06.M07]] | 未落位 | 扩展协议出错后跳到 Sync（`ignore_till_sync`）的强制点在后端主循环，卡片没写（写在 [[S02.M03]]）；catalog 冲突使 slot 失效的调用点在 standby 模块，卡片没写（写在 [[S10.M05]]）；"任何失效都丢弃 catalog 快照"的强制点在失效传播模块，卡片没写（写在 [[S11.M06]]）；`vac_update_relstats` 非事务性原地覆盖的语义在 vacuum 卡片里没写（写在 [[S04.M11]]） | file:src/backend/storage/ipc/standby.c:505-507 |
| X123 | [[S06.M07]] [[S07.M02]] | 跨度 | clog 与 multixact 的截断被两张卡各写一半：截断到哪里（最老的 datfrozenxid/datminmxid）在 vacuum 卡，"先截断、再更新界限"的顺序在 xid 空间卡；执行代码 `vac_truncate_clog` 在 [[S06]] | — |
| X124 | [[S01.M01]] | 未找准 | postmaster 的崩溃重置是一条多步的顺序协议（SIGQUIT、超时 SIGKILL、退出清理、重建共享内存、重启 startup），写不成一句话；更适合放进 [[I7]] 的协议视图 | — |
| X125 | [[S08.M03]] [[S08.M04]] [[S08.M13]] | 未找准 | 绕过 buffer 写入的页（bulk_write）由谁负责持久化，按"正常写 WAL / unlogged / wal_level=minimal"三种条件在三个模块之间切换，没有统一的强制点 | — |
| X126 | [[S07.M01]] [[S11.M12]] | 未找准 | 一个模块里合着两套独立的东西：事务状态机与两阶段提交（Contract 的 error 项因此有三条不相干的路径）；ALTER TABLE 的锁级别约定与并发建索引的多事务协议 | — |
| X127 | [[S09.M05]] [[S09.M03]] | 未找准 | "WAL 末尾"一条不变量混着"如何判定末尾"和"崩溃恢复后写 OVERWRITE_CONTRECORD"两件事；checkpoint 的步骤顺序是一句话但跨 5 个模块 | — |

## 编写 principles.md 时追加（2026-11-12，只记录）

来源：`principles.md` 的作用检验（GUIDE §4.5.6）和编写过程。新增一个粗标签：检验不符（检验里的预测与实际不一致，或者检验本身分不出好坏）。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X128 | principles | 检验不符 | 裁决检验分不出有没有文件：7 道基线之后才提交的新题，大模型有文件 7/7、无文件 7/7；小模型有文件 4.5/7、无文件 4/7。8 道历史题大模型无文件也全对。文件的增益没有被测出来 | 题目与答卷在 `/tmp/archmap/test/`（临时目录，未入库） |
| X129 | principles T7、R5 | 检验不符 | 新题 N1：小模型按"已发布分支上 ABI 稳定优先"否定了实际被采纳的做法（安全修复回补并触及 ABI）。T7 和 R5 已按这两条提交收窄；收窄后的说法没有再检验 | commit:245eb832f88; commit:3beee6e77c8 |
| X130 | principles R2 | 检验不符 | 新题 N6 第 2 问：小模型把 R2 套到"每次调用怎样分发"上，答成固定回调成员；实际是扫描开始时自选实现、存进扫描描述符。R2 的适用范围已补一句；没有再检验 | commit:ddce1da5b1b |
| X131 | [[I6]] [[S04]] | 已解决 | 重构检验：读者由决策和力推出了 8 个不变量族中的 7 个，没有推出语义等价。非留出的根层决策里只有 [[D-path-plan-split]] 指向它；候选原则 R1 的支撑只在留出集里（已解决：本族现在有上游决策 [[D-trust-declared-properties]]） | — |
| X132 | principles F | 孤儿 | 安全（权限、行级安全、信任边界）不是力，地图里也没有为它而做的决策；三道新题（N1、N3、N5）都是安全修复，读者只能借 F1 和 F5 外推 | — |
| X133 | [[S09.M07]] [[S09.M08]] [[S06.M04]] [[S07.M04]] [[S07.M07]] [[S01.M05]] | 已解决 | `principles.md` 新增的 4 条决策（[[D-wal-archive-pitr]] [[D-row-lock-in-tuple-header]] [[D-lock-tiers]] [[D-compat-by-release-class]]）还没有被任何模块卡的 Derives from 引用；[[X89]] 的两张孤儿卡现在有了上游，但卡片没有改 | — |
| X134 | L0 | 已解决 | L0 §1 的推导链仍写 `力 → 决策`，ID 约定没有列 E、T、R；§2.3 末尾的统计（73 条决策、33 条原始原因未考证、孤儿卡清单）没有随 `principles.md` 更新；§2.2 图里的决策簇没有包含新增的 4 条 | — |
| X135 | principles | 检验不符 | 敏感性检验第 3 题（异步 I/O 出现）：读者没能指出该重审"依赖操作系统页缓存"这条决策，因为它当时在留出集里。[[D-own-buffer-cache]] 和 [[E14]] 补入之后没有复测 | — |
| X136 | [[S01.M01]] [[S03]] [[S10.M07]] | 检验不符 | 新题里读者说依据不足的三处：没有"子进程不得依赖 fork 继承来的进程本地状态"的规则（N2）；没有"安全修复时保守禁用优化还是精确判定"的取舍（N3）；两阶段提交的空事务在哪一层被跳过（N7）不在给读者的两个文件里 | — |
| X137 | principles §4 | 缺证据 | 原始原因未考证或部分未考证的基础性决策：[[D-snapshot-xmin-xmax-xip]] [[D-process-per-connection]] [[D-error-longjmp-abort]] [[D-sinval-with-locks]] [[D-path-plan-split]] [[D-exec-pull-iterator]] [[D-compat-by-release-class]] [[D-own-buffer-cache]]；只查到提交标题或二手来源的：[[D-mvcc-no-overwrite]] [[D-full-page-writes]] [[D-wal-archive-pitr]] [[D-catalog-driven-extensibility]] [[D-xid-wraparound-freeze]] [[D-logical-decoding-on-wal]] | — |
| X138 | principles §1、§4 | 缺证据 | 没有原文支持的前提和标签：[[E4]] 的现状未核实；[[D-exec-pull-iterator]] 的前提是假设；[[D-logical-decoding-on-wal]] 的前提和被否方案未考证；[[D-xid-wraparound-freeze]] 的 F5 标签是假设；经网页抓取的引文都是转述，没有逐字核对 | — |

## 第一轮"有用"检验后追加（2026-11-13，只记录）

来源：`tests/round1/`（47 题、三种条件的闭卷检验）和 30 行正确性抽查。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X139 | 整张地图 | 检验不符 | P.4 第 3 项没有通过：加权得分地图 0.58、README 0.57、不给材料 0.51；风险题 0.28，目标是 0.80；地图没有好过 README 对照 | `tests/round1/README.md` |
| X140 | L1 表 | 检验不符 | 落点题不给材料也有 0.89 的命中率，地图是 0.98；"模块在哪个文件"对模型读者的增量小 | `tests/round1/grades/recs.json` |
| X141 | [[S06.M05]] [[S07.M01]] | 未落位 | 可见性卡片原先只写了"先查进行中、再查 pg_xact"，没有写它适用于所有代码路径，也没有写"判定中止不能用 DidAbort，只能用排除法"；地图读者在 B3-03 上推反了方向。已补为 [[S06.M05]] Inv(4)，并在 [[S07.M01]] 加了指向它的引用 | file:src/backend/access/heap/heapam_visibility.c:13-32 |
| X142 | [[S05]] | 缺证据 | [[S05]] 的 32 条不变量没有行内锚点，证据集中在卡片的 Evidence 字段，无法逐条核对；其他子系统是逐条带锚点 | — |
| X143 | [[S01.M02]] [[S08]] [[S12]] | 检验不符 | 地图读者明说"地图没有覆盖"的题：bgworker 的可中断标志（B1-01）、在线启用校验和遇到无效数据库（B3-10）、外键检查的缓存元数据在失效回调里的生命周期（B1-10） | `tests/round1/answers/C_1.md`; `tests/round1/answers/C_3.md` |
| X144 | principles | 检验不符 | 三位地图读者答 36 题都没有读 `principles.md`；它对这类题没有被用上 | `tests/round1/README.md` |
| X145 | [[S04]] | 泄漏 | B2-08：地图读者以中等置信给出与实际相反的修法（Memoize 的缓存键参数）；地图里没有相应的规则，也没有提示这里取决于细节 | `tests/round1/answers/C_2.md` |

## 编写 invariants/ 时追加（2026-11-13，只记录）

来源：八个不变量族视图的写作者报告。规则编号指 `invariants/I<n>.md` §3；"卡片未写"的规则在视图里带了源码证据，但没有所有者卡片。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X146 | [[I1]] [[S08.M01]] [[S09.M05]] [[S08.M08]] [[S10.M05]] | 未落位 | 卡片未写的规则：I1.R5（恢复期间的真实改动必须用 MarkBufferDirty）、I1.R16（redo 改 init fork 要立即刷盘）、"redo 不得写 WAL、必须 PageSetLSN"、"restart_lsn 不后退"、"WAL 记录里的值要在持锁后、临界区内取得"（2026-11-13 已落位：I1.R5 → [[S08.M01]] Inv(6)，I1.R16 → [[S09.M05]] Inv(8)；仍缺："redo 不得写 WAL"、"restart_lsn 不后退"、"WAL 记录里的值要在持锁后、临界区内取得"） | `invariants/I1.md` |
| X147 | [[I2]] [[S07.M05]] [[S06.M04]] [[S06.M06]] [[S10.M10]] [[S07.M08]] | 已解决 | 卡片未写的规则：I2.R3（非 procarray 来源的快照）、I2.R4（快照复用与 xactCompletionCount）、I2.R7（哪些后端可以不计入视界）、I2.R13（沿更新链逐步核对 xmin）、I2.R15 的提升顺序、I2.R20 各索引 AM 的 SIREAD 锁义务（2026-11-13 已解决：I2.R3 → [[S10.M10]] Inv(5)（逻辑解码处的强制点，一般形式没有单一强制点）；I2.R4 → [[S07.M05]] Inv(10)；I2.R7 → [[S07.M05]] Inv(11)；I2.R13 → [[S06.M04]] Inv(5)；I2.R15 提升顺序 → [[S09.M02]] Inv(13)；I2.R20 → [[S07.M08]] Inv(3)） | `invariants/I2.md` |
| X148 | [[I3]] [[S11.M12]] [[S07.M07]] [[S08.M01]] [[S01.M05]] | 已解决 | 卡片未写的规则：I3.R5（一开始取最强的锁，不做锁升级）、I3.R6（多个关系的加锁顺序）、I3.R9（持 catalog 页排他内容锁时不读 catalog）、I3.R11（重新加锁后复查）、I3.R19（进程退出时先放 LWLock 再脱离 DSM）；I3.R5、I3.R6 没有机械检查（2026-11-13 已解决：I3.R5 → [[S11.M12]] Inv(A6)；I3.R6 → [[S11.M12]] Inv(A7)；I3.R9 → [[S08.M01]] Inv(7)；I3.R11 → [[S08.M01]] Inv(8)；I3.R19 → [[S01.M03]] Inv(4)；R5、R6 仍是约定，没有机械检查） | `invariants/I3.md` |
| X149 | [[I4]] [[S02.M06]] [[S10.M09]] [[S11.M09]] | 已解决 | 卡片未写的规则：I4.R3（自动失效只覆盖四类 catalog 行，其余依赖要显式发失效）、I4.R11（弱锁 DDL 下分区集合在规划与执行之间会变）、I4.R14（并行 worker 先恢复 relmap 再访问 catalog）、I4.R17（逻辑解码里失效消息向并发事务的分发）（2026-11-13 已解决：I4.R3 → [[S11.M09]] Inv(7)；I4.R11 → [[S11.M07]] Inv(6)(7)；I4.R14 → [[S01.M06]] Inv(5)；I4.R17 → [[S10.M09]] Inv(4)） | `invariants/I4.md` |
| X150 | [[I5]] [[S07.M01]] [[S12.M01]] [[S12.M04]] | 已解决 | 卡片未写的规则：I5.R11（重置 context 不释放 palloc 之外的资源）、I5.R13（可能跑用户代码或处理失效的调用前后要恢复一致状态、重取指针）、I5.R14（登记全局状态的模块要在三条退出路径上都撤销）；中止路径的步骤顺序不在 [[S07.M01]] 卡片上（2026-11-13 已解决：I5.R11 → [[S12.M01]] Inv(1)；I5.R13 → [[S12.M01]] Inv(5)；I5.R14 → [[S07.M01]] Inv(15)；中止路径的步骤顺序 → [[S07.M01]] Inv(14)） | `invariants/I5.md` |
| X151 | [[I6]] [[S04]] [[S03]] | 未落位 | 17 条规则里 9 条没有卡片（I6.R2、R5–R7、R9–R11、R17，以及 R8 的安全次序部分）；规划器变换前后没有差分校验，只靠回归用例（2026-11-13 已落位：R2 → [[S04.M05]] Inv(7)；R5 的唯一性、分组键与分区连接证明 → [[S04.M02]] Inv(1)(2)、[[S04.M03]] Inv(2)；R6 → [[S04.M05]] Inv(8)；R7 → [[S04.M03]] Inv(3)；R8 的安全次序 → [[S04.M03]] Inv(4)；R9 的规划器变换 → [[S04.M02]] Inv(3)；R10 → [[S04.M03]] Inv(1)、[[S05.M05]] Inv(5)；R11 → [[S04.M02]] Inv(4)；仍缺：R17（[[S03.M02]] 无卡）、R5 的冗余排序键与执行器排序/分组捷径、R8 的注入（[[S03.M06]] 无卡）与选择率估计按根父表判断权限（[[S04.M06]] 无卡）、R9 的执行器 NULL 处理与布尔分区裁剪取反；差分校验仍没有） | `invariants/I6.md` |
| X152 | [[I7]] [[S01.M01]] [[S01.M03]] [[S01.M04]] [[S01.M07]] | 已解决 | 卡片未写的规则：I7.R6（崩溃重置只用 postmaster 本地保存的输入重建共享内存）、I7.R9（每个等待都要能察觉 postmaster 死亡）、I7.R10（关机和提升没有 SIGKILL 兜底，阻塞等待必须可中断）、I7.R13（从 PG 进程 fork 出的非 PG 进程不得跑 proc_exit）（2026-11-13 已解决：I7.R6 → [[S01.M03]] Inv(6)；I7.R9 → [[S01.M04]] Inv(4)；I7.R10 → [[S01.M01]] Inv(8)；I7.R13 → [[S01.M04]] Inv(5)；另 I7.R3 的一般形式 → [[S01.M01]] Inv(7)） | `invariants/I7.md` |
| X153 | [[I8]] [[S12.M05]] [[S09.M06]] | 未落位 | 卡片未写的规则：I8.R1–R3（改持久格式要自己提升对应的版本常量；这些提升没有机械检查）、I8.R6（binary-upgrade 模式只能用预分配的 OID，这部分代码没有模块卡）、I8.R7（小版本的 ABI 规则没有所有者卡片）（2026-11-13 已落位：I8.R1 → [[S09.M02]] Inv(12)；I8.R3 → [[S09.M06]] Inv(6)；I8.R6 → [[S11.M01]] Inv(4)（pg_upgrade_support.c 仍无卡，只作锚点）；I8.R7 → [[S12.M05]] Inv(5)；仍缺：I8.R2（catversion 的提升义务）；提升仍没有机械检查） | `invariants/I8.md` |
| X154 | L0 §6 | 注释不符 | 强制点矩阵里有的模块在 L1 表标了某一族，但卡片上没有该族的 Inv：[[I7]] 行的 [[S06.M07]] [[S09.M02]] [[S09.M05]] [[S10.M01]]，[[I6]] 行的 [[S05.M03]]；另有若干卡片条目的 [[I8]]、[[I6]] 标签存疑（[[S11.M06]] Inv(3)、[[S11.M08]] Inv(1)(4)、[[S11.M03]] Inv(3)、[[S09.M08]] Inv(3)、[[S12.M05]] Inv(3)、[[S04.M13]] Inv(4)、[[S04.M11]] Inv(3)） | `invariants/I6.md`; `invariants/I7.md`; `invariants/I8.md` |
| X155 | [[S01.M05]] | 注释不符 | `storage/lmgr/README` 说取 spinlock 会挂起 cancel/die，`spin.h` 说不会、靠持锁期间不到达 CHECK_FOR_INTERRUPTS；直接退出进程的信号处理器不在这个假设之内 | file:src/backend/storage/lmgr/README:39-42; file:src/include/storage/spin.h:26-29; commit:208ea05a51b |
| X156 | [[S09.M11]] | 缺证据 | 卡片引用了留出窗口内的提交 commit:d61f9c90f2e（2026-09-12）；地图的卡片是在基线上建的，引用窗口内提交的不止这一处，出题时已按"地图引用过的提交不出题"排除 | — |

## 第二轮检验后追加（2026-11-13，只记录）

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X157 | invariants/ | 检验不符 | 闭卷 36 题：加上 `invariants/` 的新地图得分不高于旧地图（定向阅读 0.31 对 0.34）；定向阅读时带中等以上置信的错误断言 2 次，旧地图 0 次 | `tests/round2/README.md` |
| X158 | 整张地图 | 检验不符 | 开卷 8 题：只给代码找到 6 个真实缺陷，代码加地图找到 5 个，多用约 46% 的 token；P.4 第 3 项"有增益"没有达到 | `tests/round2/README.md` |
| X159 | L0 §1、§6 | 检验不符 | 自由阅读时四组读者里只有两组各读了一个 `invariants/` 文件；L0 的指引没有把读者带到对应的视图 | `tests/round2/README.md` |
| X160 | [[I6]] [[S04]] | 检验不符 | 规划器语义等价的 9 道题，四种闭卷条件得分都在 0.00–0.06；这一族的具体缺陷从抽象层面预测不了 | `tests/round2/grades/recs.json` |
| X161 | [[I5]] | 泄漏 | 开卷 R3-03：读了 [[I5]] 检查单的读者去找悬空指针一类问题，漏掉了代码里直接可见的空指针解引用；检查单会把注意力引向历史上常见的类别 | `tests/round2/openbook/answers/M_R3-03.md` |

## 先验测试与推导主线后追加（2026-11-13，只记录）

来源：`tests/prior/`，以及编写 `derivation.md` 时三位写作者报告的"推不顺"之处。后者说明推导链在那里缺一环。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X162 | invariants/ | 检验不符 | 150 条规则的 409 个要点里，opus 不看材料只漏 23 个，三档模型都漏的只有 22 个；`invariants/` 的内容对模型读者绝大部分是冗余的 | `tests/prior/README.md` |
| X163 | [[I1]] [[S08.M01]] | 已解决 | "日志先于数据页落盘"没有自己的决策，只能由 [[D-own-buffer-cache]] 和 [[D-wal-redo-only]] 合起来间接推出（同 [[X90]]）（已解决：同 [[X90]]，补进 [[D-wal-redo-only]]，`derivation.md` 3.1–3.2 已改） | `derivation.md` 3.1–3.2 |
| X164 | [[I6]] [[S04]] | 已解决 | 由 [[D-path-plan-split]] 只能推出"需要一族等价规则"，推不出具体规则；缺一条根层决策或前提："每个变换必须语义等价，核心依赖它不验证的属性标注"（同 [[X131]]）（已解决：新增根层决策 [[D-trust-declared-properties]] 和前提 [[E16]]，`derivation.md` 6.3） | `derivation.md` 6.1 |
| X165 | [[S12.M03]] | 已解决 | 临界区内出错升级为 PANIC 的理由仍是假设（同 [[X99]]）；推导时 2.2 只能前引 3.2（已解决：同 [[X99]]，`derivation.md` 2.2 已改为有出处的理由） | `derivation.md` 2.2、9.5 |
| X166 | [[S01.M04]] | 已解决 | [[D-signal-flag-latch]] 的记录原因是 select() 的竞态，不是每连接一进程的推论；把它挂在进程模型下是写作者的推理（已解决：`derivation.md` 2.1 把"需要跨进程通知（由进程模型推出）"和"处理器只置标志（另有原因）"分开写） | `derivation.md` 2.1 |
| X167 | principles §4 | 缺证据 | 推导时"另一条路"写不出来的决策：[[D-lock-tiers]]、[[D-wal-archive-pitr]]、[[D-row-lock-in-tuple-header]]、[[D-exec-pull-iterator]]、[[D-compat-by-release-class]]（没有记录过被否方案）；[[D-catalog-mvcc-heap]] 没有追溯"系统表为什么是普通 heap 表"（2026-11-13 已补：[[D-row-lock-in-tuple-header]] [[D-wal-archive-pitr]] [[D-compat-by-release-class]] 的被否方案；仍缺：[[D-lock-tiers]] 与 [[D-exec-pull-iterator]] 的被否方案，系统表为什么是普通表的原始原因） | `derivation.md` |
| X168 | [[S10]] | 已解决 | 备库不支持 SERIALIZABLE、物理复制不能跨大版本、槽的故障转移同步、内建发布订阅，都没有对应的决策（已解决：备库不支持 SERIALIZABLE、同一大版本的要求补进 [[D-physical-rep-ships-wal]] 的下游代价；槽同步和发布/订阅补进 [[D-logical-decoding-on-wal]]（发布/订阅进核心的动机仍未考证，见 [[X174]]）） | `derivation.md` 7.1、7.2 |
| X169 | principles §4 | 已解决 | [[D-row-lock-in-tuple-header]] 的原始原因只覆盖 2005 年的共享行锁和 MultiXact；"排他行锁记在 xmax"从哪里来没有记录，标题与原始原因可能对不上（未核实）（已解决：原始原因改为从 1998 年 MVCC 初始代码的 heap_mark4update 写起，并补了 2005 年的理由和被否方案） | `derivation.md` 4.4 |
| X170 | [[D-proto-minor-negotiation]] | 已解决 | 协议兼容算 [[D-compat-by-release-class]] 的一部分，还是独立的根层决策，地图没有说（已解决：协议兼容是另一条轴，不按发布类别，写进 [[D-compat-by-release-class]]；[[D-proto-minor-negotiation]] 是它的派生） | `derivation.md` 8.1 |

## 缺口补全后追加（2026-11-13，只记录）

来源：按 `/tmp/archmap/gap/FINDINGS.md`（G1–G9）补全 `principles.md` 和 `derivation.md` 时仍然缺的，以及编写中发现的。新增一个粗标签：不一致（地图里两处对同一件事的说法不同）。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X171 | [[I6]] [[S04]] | 未落位 | [[D-trust-declared-properties]] 推出的规则里，[[I6]].R5、R6、R7 和 R9 的变换侧都写着"卡片未写"，没有所有者卡片；决策的模块落点只能列到持有 R3、R12、R15、R9 调用约定的卡片和参与方（2026-11-13 已落位：R5 的证明侧 → [[S04.M02]] Inv(1)(2)、[[S04.M03]] Inv(2)；R6 → [[S04.M05]] Inv(8)；R7 → [[S04.M03]] Inv(3)；R9 的规划器变换 → [[S04.M02]] Inv(3)；仍缺：R5 的冗余排序键（pathkeys）一侧；R9 的执行器与布尔分区裁剪一侧不属变换侧） | `invariants/I6.md` §3 |
| X172 | principles §4 | 缺证据 | [[D-lock-tiers]] 的被否方案、[[D-exec-pull-iterator]] 的原始原因和被否方案：本轮没有查（[[X167]] 的剩余部分）（2026-11-13 已补：[[D-lock-tiers]] 的被否方案三条（改用线程、POSIX 互斥量、用重锁保护共享结构，转述，2001 年同一讨论串）；[[D-exec-pull-iterator]] 的最早成文（2001 年 executor/README，逐字）和讨论过的方向（2016 年向量化讨论、2017 年表达式改写和 2018 年 JIT 只改节点内部，转述）；仍缺：LWLock 为什么不做死锁检测，提案和讨论里都没有理由；拉取式执行器的原始原因仍未考证，也没有"为什么保留逐元组拉取"的设计结论） | — |
| X173 | [[D-catalog-mvcc-heap]] | 缺证据 | 系统表为什么是普通表的原始原因仍未找到（Berkeley 1986 年的论文读不到）；现行文档只陈述事实和自举的后果 | file:doc/src/sgml/catalogs.sgml:13-14; file:doc/src/sgml/bki.sgml:8-11 |
| X174 | [[D-logical-decoding-on-wal]] [[S10]] | 缺证据 | 内建的发布/订阅为什么放进核心：提交没有写动机（未考证）；文档只列出用途，其中包括跨大版本复制（2026-11-13 已补：已查提交、两份设计帖（2016-07、2016-08）、二手博客；当时外部方案 pglogical 已存在，讨论中有人主张进核心（转述）；仍缺：一手的"为什么放进核心"陈述） | commit:665d1fad99e |
| X175 | [[S12.M03]] [[D-wal-redo-only]] | 缺证据 | 临界区升级 PANIC 与"没有 undo"的联系仍是推断，源码和文档都只说"共享缓冲区里有未写日志的修改，必须保证它们不落盘"（2026-11-13 已补：2001 年时确无 undo 的文档证据（逐字，commit:7b9dc714059）、Vadim 在 2001-01 讨论里的原话（转述，讨论里没有提到 undo）；仍缺：两者的因果，没有原文） | file:src/backend/access/transam/README:442-444 |
| X176 | [[D-trust-declared-properties]] | 已解决 | 没有找到"相信声明、错了就错"的一般陈述，也没有找到关于在运行时验证声明的讨论；这条决策是从各处文档和提交归纳的，被否方案为空（2026-11-13 已解决：一般陈述见于邮件列表（Tom Lane，转述），补进 [[D-trust-declared-properties]] 的当前原因；被否方案补上"在定义处强制检查声明"（2010 年，转述）；运行时检查声明的讨论仍没有读到；引文为转述，未逐字核对） | `principles.md` D-trust-declared-properties |
| X177 | [[S05.M12]] | 不一致 | 卡片 Inv(2)（批量化看 volatile）标 [[I6]]、Inv(1) 标 [[I2]]，但 `subsystems/5.md` 的 L1 表只给 S05.M12 标 [[I8]]，`L0.md` §6 的 I6 行也没有列 S05.M12；而 [[I6]].R15 的所有者是它 | `subsystems/5.md` L1 表与 S05.M12 卡；`L0.md` §6 |

## 规则视图更新后追加（2026-11-13，只记录）

来源：按 `/tmp/archmap/gap/VIEWS.md` 更新 `invariants/` 的所有者一栏、修正过强或过时的规则时发现的。任务 2 列出的条目都已对照源码核实并改掉，没有核对不上的。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X178 | [[I1]] [[S08.M01]] | 已解决 | [[I1]].R4 和 §1 的 hint 一条只说"恢复中不弄脏"；源码在开启校验和或 wal_log_hints 时，对 WAL 跳过的关系同样不弄脏。本轮只按要求改了 R5，R4 与 §1 仍是旧说法（2026-11-13 已解决：[[I1]] §1 与 R4 改为"开校验和或 wal_log_hints 时，恢复中或对 WAL 跳过的关系只设 hint、不弄脏（只限 hint）"，与 R5、[[S08.M01]] Inv(6) 一致） | file:src/backend/storage/buffer/bufmgr.c:5760-5772 |
| X179 | [[I8]] | 已解决 | [[I8]] §7 的 C7 行仍把"布局随编译选项变化"列为触发改动；本轮已从 R7 删掉这一说法（规则行里没有锚点）。C7 引的提交只在开发分支上把 Port 的布局改成与 USE_OPENSSL 无关，提交信息说这是对扩展的照顾，v17 上没有改布局，不足以支撑一条规则（2026-11-13 已解决：C7 行删去"布局随编译选项变化"和 commit:8fb5936703e；§6 检查单里 #ifdef 布局一行改写为"对扩展的照顾，不是 I8.R7 的要求"） | commit:8fb5936703e |
| X180 | L0 §6 [[S05.M12]] | 已解决 | [[X177]] 只改了 L1 表（S05.M12 改标 [[I2]] [[I6]]）；`L0.md` §6 的 I6 行仍没有列 S05.M12，I8 行仍列着 S05: M12（2026-11-13 已解决：`L0.md` §6 的 I2、I6 行补上 S05 M12，I8 行删去 S05: M12） | `L0.md` §6；`subsystems/5.md` L1 表 |
| X181 | [[I6]] | 已解决 | [[I6]] §1 关于 collation 相等的一条：锚点 analyzejoins.c:806-809 已漂移（现为 808-810）；"两个 deterministic collation 可视为同一相等关系"不适用于分区连接，分区连接要求 collation 完全相同（2026-11-13 已解决：[[I6]] §1 锚点改为 analyzejoins.c:808-810，并写明分区连接不用这一放松、要求 collation 完全相同，见 relnode.c:2510-2512） | file:src/backend/optimizer/plan/analyzejoins.c:808-810; file:src/backend/optimizer/util/relnode.c:2510-2512 |
| X182 | [[I8]] [[S09.M02]] | 已解决 | [[I8]].R4 的后半句（持久数据不得依赖未记录的、由实现决定的 C 行为）写着"卡片未写"，没有出现在 [[X153]] 或其他残差里（2026-11-13 已解决：写进 [[S09.M02]] Inv(8)，[[I8]].R4 所有者改为 Inv(7)(8)） | `invariants/I8.md` §3 |
| X183 | [[I1]] [[S09.M05]] | 已解决 | [[I1]].R15 的"redo 不得写 WAL"仍写着"卡片未写"（[[X146]] 的剩余部分），本轮的所有者结果里没有这一条（2026-11-13 已解决：写进 [[S09.M05]] Inv(2)（恢复中 XLogInsertAllowed() 为假，XLogBeginInsert/XLogInsertRecord 报 ERROR），[[I1]].R15 的"卡片未写"已去掉） | file:src/backend/access/transam/xloginsert.c:165-166 |
| X184 | L0 §2.3 | 已解决 | §2.3 表下统计句只按要求重算了"没有被任何模块卡引用"的数（12→10）；同一句里其余的数按 `/tmp/archmap/sync_work/stats.py` 的算法也已过时：决策共 78 条（句中 77），只在 `principles.md` 里的 5 条（句中 4；脚本列出 [[D-row-lock-in-tuple-header]] [[D-lock-tiers]] [[D-wal-archive-pitr]] [[D-trust-declared-properties]] [[D-compat-by-release-class]]），原始原因未考证 28 条（句中 29）（2026-11-13 已解决：按 stats.py 重算并改为共 78 条、只在 principles.md 的 5 条、未考证 28 条、无卡引用 10 条；孤儿卡两条（2 张、4 张）与脚本一致，未改） | `L0.md` §2.3 |

## 推导主线核查后追加（2026-11-14，只记录）

来源：`tests/derivation-check/`。

| ID | 节点 | 类型 | 残差 | 证据 |
|---|---|---|---|---|
| X185 | principles §4 | 缺证据 | "不覆盖写的存储是只做 redo 能成立的前提"只有时间先后有据，逻辑依赖没有原文，只能按假设记 | `tests/derivation-check/review.md` |
| X186 | [[D-lazy-xid-vxid]] [[D-geqo]] [[D-checkpoint-spread]] | 缺证据 | 推导主线引用了这三条决策，但它们在 `principles.md` 里没有记录，只有子系统文件里的一行，原始原因都未考证 | `tests/derivation-check/result_1.md`; `tests/derivation-check/result_2.md` |
| X187 | [[I1]] [[S09.M05]] | 未找准 | "页与日志必须一致"（重放只能从日志描述过的页状态接着走，所以页上不能有日志不知道的物理改动）是推导主线补的推断；源码只说结论（README 442-444），这条规则在地图里没有一处写成一句话 | `derivation.md` 3.2；file:src/backend/access/transam/README:407-425,442-444 |

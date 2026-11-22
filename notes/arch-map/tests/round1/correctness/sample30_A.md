### A01 [Inv] notes/arch-map/subsystems/4.md:35 S04.M05
- Inv: (1) equal() 的 Var/PHV 代表同一个值：varnullingrels 记录可能把它置 NULL 的外连接 {[[I6]]; D1=4: [[S03]], [[S04.M02]], [[S04.M05]], [[S04.M07]]} 〔commit:2489d76c490; file:src/backend/optimizer/README:343〕

### A02 [Inv] notes/arch-map/subsystems/12.md:54 S12.M03
- Inv:      (1) CritSectionCount>0 时任何 ≥ERROR 一律升级为 PANIC {[[I5]]; D1=2: [[S12.M03]], [[S01]]} (2) 无处理者、ExitOnAnyError 或 proc_exit 进行中时 ERROR 升为 FATAL {[[I5]] [[I7]]; D1=2: [[S12.M03]], [[S01]]} (3) 已在栈上的更高级别不会被后来的低级错误降级 {[[I5]]; D1=1} (4) PG_CATCH 段必须 PG_RE_THROW 或做（子）事务中止，否则系统可能处于不一致状态 {[[I5]]; D1=全部调用者} (5) 在 PG_TRY 段内修改、在 PG_CATCH 段内使用的局部变量必须声明 volatile {[[I5]]; D1=全部调用者} (6) FATAL 不经 PG_CATCH，直接走 proc_exit，故非进程本地资源不能只靠 PG_CATCH 清理（可用 PG_ENSURE_ERROR_CLEANUP）{[[I5]]; D1=2: [[S12.M03]], [[S01]]} (7) 错误栈溢出（>5 层）→ PANIC {[[I5]]; D1=1} (8) elevel 恰为 ERROR 时 errfinish 不返回：清零 InterruptHoldoffCount/QueryCancelHoldoffCount/CritSectionCount、把 CurrentMemoryContext 留在 ErrorContext，然后 longjmp 到最近的 PG_exception_stack（PG_TRY 或 [[S01.M07]] 的顶层 sigsetjmp）；打印与弹出错误栈由处理者负责 file:src/backend/utils/error/elog.c:530-557 {[[I5]]; D1=3: [[S12.M03]], [[S01.M07]], [[S07.M01]]}

### A03 [Inv] notes/arch-map/subsystems/8.md:51 S08.M03
- Inv: (1) 段文件编号连续：若干满段 + 一个部分段 + 若干 0 长度的非活跃段；截断只把段截为 0 而不 unlink，因为别的进程可能还持有它的 fd，再扩展时数据不能写进已删的旧文件 {[[I1]], [[I8]]; D1=1}〔file:src/backend/storage/smgr/md.c:54-68〕。(2) 每次写或扩展（skipFsync 除外）后都要向 [[S08.M04]] 登记该段的 fsync 请求；队列满时就地 fsync {[[I1]]; D1=3: [[S08.M03]], [[S08.M04]], [[S09]]}〔file:src/backend/storage/smgr/md.c:1518-1537〕。(3) 删除普通关系的主 fork 时，首段只截断为 0、登记到下个 checkpoint 后再 unlink，防止 relfilenumber 在崩溃前被重用（wal_level=minimal 下新文件内容只靠 fsync，重用后会丢）；其余段截断后立即 unlink，其他 fork 立即删除 {[[I1]]; D1=4: [[S08.M03]], [[S08.M04]], [[S08.M13]], [[S09]]}〔file:src/backend/storage/smgr/md.c:284-305,318-321,386-387〕。(4) bulk_write 绕过 buffer：WAL 正常时写完再整体登记 fsync，期间若发生过 checkpoint 就自己 fsync（用 DELAY_CHKPT_START 防竞争）；unlogged 关系登记 fsync，靠关机 checkpoint 刷出；wal_level=minimal 交给提交时的 pendingSyncs {[[I1]]; D1=4: [[S08.M03]], [[S08.M04]], [[S08.M13]], [[S09]]}〔file:src/backend/storage/smgr/bulk_write.c:139-219〕。(5) 截断会发 smgr 失效消息（广播与接收时点见 [[S11.M09]]），使其他后端缓存的块数失效；缓存块数只在恢复中直接使用 {[[I4]]; D1=2: [[S08.M03]], [[S11.M09]]}〔file:src/backend/storage/smgr/smgr.c:845-856,896,907-922〕。(6) smgr.c 中的大多数函数持有中断，防止 procsignal 触发 smgrreleaseall 重入 {[[I5]]; D1=2: [[S08.M03]], [[S01]]}〔file:src/backend/storage/smgr/smgr.c:43-53〕。

### A04 [Inv] notes/arch-map/subsystems/6.md:111 S06.M11
- Inv: (1) 重写保留每个复制元组的可见性信息与 ctid 链：新位置不同，链接经新旧 TID 映射改写 {[[I2]]; D1=2: [[S06.M11]], [[S06.M04]]} file:src/backend/access/heap/rewriteheap.c:6-7,37-58

### A05 [Inv] notes/arch-map/subsystems/6.md:149 S06.M14
- Inv: (2) 同时锁多页时一律先子后父、同层自左向右 {[[I3]]; D1=1} file:src/backend/access/gist/README:262-265

### A06 [Inv] notes/arch-map/subsystems/10.md:100 S10.M07
- Inv: (1) 新 catalog_xmin/restart_lsn 先 ReplicationSlotSave 落盘，再推进 effective_catalog_xmin 与全局视界/WAL 保留 {[[I1]] [[I2]]; D1=3: [[S10.M07]], [[S10.M05]], [[S07]]}（file:src/backend/replication/logical/logical.c:1963-1970,1995-2035）

### A07 [Inv] notes/arch-map/subsystems/10.md:112 S10.M08
- Inv: (1) 启用时先开逻辑信息写入并等所有进程通过 barrier，再开解码并写状态记录；停用反序且不等 barrier（多写逻辑信息总是安全） {[[I1]]; D1=3: [[S10.M08]], [[S01]], [[S09]]}（file:src/backend/replication/logical/logicalctl.c:14-23,428-442,476-502,627-639）

### A08 [Inv] notes/arch-map/subsystems/11.md:137 S11.M13
- Inv:

### A09 [Inv] notes/arch-map/subsystems/5.md:83 S05.M09
- Inv: (3) 子事务中止时截断该子事务追加的事件并恢复 SET CONSTRAINTS 状态与 query_depth {[[I5]]; D1=2: [[S05.M09]], [[S07]] xact}

### A10 [Inv] notes/arch-map/subsystems/5.md:81 S05.M09
- Inv: (1) 查询级事件在 AfterTriggerEndQuery 才生效，立即模式此时触发，延迟事件并入事务级链 {[[I5]]; D1=2: [[S05.M01]], [[S05.M09]]}

### A11 [Contract] notes/arch-map/subsystems/1.md:46 S01.M02
- Contract: 适用条件: Inv(1) 的例外: in_use 期间 backend 仍可置 terminate 位 〔file:src/backend/postmaster/bgworker.c:65-68〕 | 并发: 无锁协议 + 内存屏障（见 Inv1） | error: 退出码 1（FATAL）不算崩溃 〔file:src/backend/postmaster/postmaster.c:2625-2632〕 | crash: 其他非零退出触发全局重置 〔同前〕 | cancel: TerminateBackgroundWorker 置 terminate 位 〔file:src/backend/postmaster/bgworker.c:1318〕 | standby: bgw_start_time 对照 pmState 决定能否启动 〔file:src/backend/postmaster/postmaster.c:4266-4285〕

### A12 [Contract] notes/arch-map/subsystems/8.md:88 S08.M08
- Contract: 适用条件: 启动时需要恢复（非干净关机，或作为 standby）；条件之外: 干净关机后 unlogged 数据保留，它们在关机 checkpoint 时被 fsync〔file:src/backend/storage/smgr/bulk_write.c:149-155〕 | 并发: 在 startup 进程中执行，清理时尚未开放连接 | error: 未定义 | crash: INIT 完成前再次崩溃，下次启动重做 CLEANUP〔file:src/backend/storage/file/reinit.c:356-362〕 | cancel: 未定义 | standby: 运行期间只做了 CLEANUP，INIT 在恢复结束（提升）时才做〔file:src/backend/access/transam/xlog.c:6621-6629〕。

### A13 [Contract] notes/arch-map/subsystems/8.md:97 S08.M10
- Contract: 适用条件: 集群校验和状态要求写/验（`DataChecksumsNeedWrite/NeedVerify`）且页非新页〔file:src/backend/storage/page/bufpage.c:117-129,1522-1527〕；条件之外: ignore_checksum_failure 下只告警〔file:src/backend/storage/page/bufpage.c:27,80-86〕；FSM 读一律 RBM_ZERO_ON_ERROR 绕过校验，见 [[S08.M09]]〔readme:src/backend/storage/freespace/README#Recovery〕；状态迁移中见 [[S08.M12]] | 并发: 设置校验和需 share-exclusive 以上锁〔file:src/backend/storage/page/bufpage.c:1508-1509〕；计算/校验期间 HOLD_INTERRUPTS，防止校验和状态中途切换〔file:src/backend/storage/page/bufpage.c:110-117〕 | error: `PageIsVerified` 返回 false，是否 ERROR/置零由调用者的 PIV 标志决定〔file:src/backend/storage/page/bufpage.c:80-94〕 | crash: 撕裂页见 Inv 2 | cancel: 见并发 | standby: hint 不弄脏页（[[S08.M01]] Inv 6），只能等主库送来含 hint 的 FPI〔file:src/backend/storage/page/README:61-66〕。

### A14 [Decision] notes/arch-map/subsystems/4.md:84 
- D-geqo {[[F4]]} GEQO：连接数 ≥ geqo_threshold 时半随机搜索连接顺序，每个连接对仍交给 /path 生成全部实现路径 ← 原始: 未考证；当前: 半随机搜索代替穷举全部连接树〔file:src/backend/optimizer/README:10-14; file:src/backend/optimizer/path/allpaths.c:3924〕；假设: 穷举 DP 随连接数爆炸

### A15 [Decision] notes/arch-map/subsystems/3.md:32 
- D-rewrite-relock {[[F1]] [[F2]]} 改写阶段对来自存储规则或缓存的树重新取锁（AcquireRewriteLocks） ← 原始: 把规则所引关系的取锁移到独立函数，常规路径省去解析器与改写器的重复取锁，并明确在任何基于关系属性的改写决定前取锁；同时以线性时间修正 JOIN 别名中的已删除列〔commit:ba42002461a〕；当前: 处理早于当前命令产生的树时必须做，刚由解析器建好的树可以也应当跳过，因为分析期已取得同样的锁、也不会在 JOIN 中包含已删除列（[[I3]]）〔file:src/backend/rewrite/rewriteHandler.c:103-131,4678-4680; file:src/backend/utils/cache/plancache.c:850〕；陷阱: 它会就地修改 querytree，调用者通常应先 copyObject〔file:src/backend/rewrite/rewriteHandler.c:108-110〕；存储规则的 JOIN 别名可引用已删除列，依赖机制不阻止该列被删〔file:src/backend/rewrite/rewriteHandler.c:133-139〕


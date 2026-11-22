### B01 [Inv] notes/arch-map/subsystems/1.md:80 S01.M05
- Inv: (1) 持有 LWLock（及 spinlock）期间 cancel/die 被挂起，直到全部释放 {[[I5]]; D1=2: [[S01.M05]], [[S01.M07]]} 〔file:src/backend/storage/lmgr/lwlock.c:1185-1189; file:src/backend/storage/lmgr/README:39-42〕

### B02 [Inv] notes/arch-map/subsystems/5.md:122 S05.M12
- Inv: (2) 只有在没有代码能观察到尚在缓冲中的行时才用多行插入：有 BEFORE/INSTEAD 行触发器、volatile 默认值或 WHERE 中 volatile 函数时退回逐行插入 {[[I6]]; D1=2: [[S05.M12]], [[S05.M09]]}

### B03 [Inv] notes/arch-map/subsystems/1.md:54 S01.M03
- Inv: (1) 主段大小 = 全部 request_fn 请求之和，先求和后一次分配，再逐个 init {[[I7]]; D1=2: [[S01.M03]], [[S01.M01]]} 〔file:src/backend/storage/ipc/shmem.c:88-94; file:src/backend/storage/ipc/ipci.c:120-150〕

### B04 [Inv] notes/arch-map/subsystems/10.md:147 S10.M11
- Inv: (1) 远端 origin_lsn 写进本地 commit 记录，进度推进与本地提交原子；crash 后 redo 恢复精确进度，不要求同步提交 {[[I1]]; D1=3: [[S10.M11]], [[S07]], [[S09]]}（file:src/backend/replication/logical/origin.c:26-36; file:src/backend/access/transam/xact.c:1496-1499,6297-6301）

### B05 [Inv] notes/arch-map/subsystems/11.md:81 S11.M08
- Inv:

### B06 [Inv] notes/arch-map/subsystems/6.md:54 S06.M05
- Inv: (3) 页正在 IO 时不得设 hint bit（否则破坏校验和）{[[I1]]; D1=2: [[S06.M05]], [[S08]]} file:src/backend/access/heap/heapam_visibility.c:106-108

### B07 [Inv] notes/arch-map/subsystems/6.md:175 S06.M16
- Inv: (2) 摘要元组移到新位置时，新元组与 revmap 指针在同一条 WAL 记录中更新 {[[I1]]; D1=2: [[S06.M16]], [[S09]]} file:src/backend/access/brin/brin_pageops.c:232-299

### B08 [Inv] notes/arch-map/subsystems/7.md:51 S07.M02
- Inv: (1) xid 按模 2^32 比较，wrapLimit = 最老 datfrozenxid + 2^31；到 xidStopLimit 拒绝分配，因此 freeze 必须在 xid 落后 2^31 之前完成 {[[I2]]/[[I8]]; D1=3: [[S07.M02]], [[S06]] freeze/vacuum, [[S11]] pg_database}〔file:src/include/access/transam.h:266-271; file:src/backend/access/transam/varsup.c:384,400,138-157〕

### B09 [Inv] notes/arch-map/subsystems/10.md:36 S10.M01
- Inv: (1) 物理 walsender 只发送本机已 flush 的 WAL；级联时以 standby 已落盘/重放位置为上限 {[[I1]]; D1=2: [[S10.M01]], [[S09]]}（file:src/backend/replication/walsender.c:3390-3392,3451,3502）

### B10 [Inv] notes/arch-map/subsystems/6.md:137 S06.M13
- Inv: (3) VACUUM 从不删除键树中的元组或页；只删 posting tree 空页，且删除时持 posting tree 根的 cleanup lock 阻止并发插入 {[[I3]]; D1=1} file:src/backend/access/gin/README:391-402

### B11 [Contract] notes/arch-map/subsystems/8.md:61 S08.M04
- Contract: 适用条件: 非临时关系〔file:src/backend/storage/sync/sync.c:48-50〕，data_sync_retry=off〔file:src/backend/storage/file/fd.c:3989〕；条件之外: 临时关系从不 fsync、删除也不延迟；绕过 buffer 的批量写和队列满时的就地 fsync 见 [[S08.M03]] | 并发: 普通后端只转交请求；队列满时 SYNC_REQUEST 返回失败（由调用者就地 fsync），UNLINK/FORGET 请求循环重试〔file:src/backend/storage/sync/sync.c:581-620〕 | error: fsync 错误见 Inv 2 | crash: 未处理的请求丢失，由 redo 从上个 checkpoint 起重做写入（假设）；启动恢复前对整个数据目录 fsync，见 [[S08.M07]] | cancel: 重试循环不检查中断（代码注释 XXX）〔file:src/backend/storage/sync/sync.c:600-604〕 | standby: 未定义（restartpoint 路径未在本子系统核实）。

### B12 [Contract] notes/arch-map/subsystems/9.md:123 S09.M07
- Contract: 适用条件: 新 TLI 只在归档恢复结束时分配，崩溃恢复沿用原时间线〔file:src/backend/access/transam/xlog.c:6645-6658〕；standby 写的历史文件来自主库（见 [[S10]]） | 并发: 仅恢复结束时写，无锁〔file:src/backend/access/transam/timeline.c:320-322〕 | error: I/O 失败 ERROR（在 startup 中即 FATAL）；历史不连续 ERROR〔file:src/backend/access/transam/timeline.c:583〕 | crash: 写出后未切换即崩溃，standby 仍认为已切换〔file:src/backend/access/transam/xlog.c:6684-6688〕 | cancel: 未定义 | standby: 读历史决定 recoveryTargetTLI

### B13 [Decision] notes/arch-map/subsystems/2.md:89 
- D-plancache-generic-custom {[[F4]] [[F7]]} plancache 同时承担 generic/custom 计划选择，先建机制、策略可替换 ← 原始: 让缓存计划能生成依参数值定制的计划，并在 custom 与 generic 间做选择；该提交只建机制不定策略〔commit:e6faf910d75〕；当前: 选择逻辑集中在 choose_custom_plan〔file:src/backend/utils/cache/plancache.c:6-11〕

### B14 [Decision] notes/arch-map/subsystems/7.md:161 
- D-abort-by-clog {[[F4]] [[F1]]} 中止只在 clog 记一笔，且不刷 WAL；崩溃时在途事务靠排除法判为中止 ← 原始: 未考证；当前: 崩溃后本来就默认视为中止，所以中止记录不必刷盘、也不必与 checkpoint 互锁〔file:src/backend/access/transam/xact.c:1832-1837,1895-1902; file:src/backend/access/transam/clog.c:23-25; file:src/backend/access/heap/heapam_visibility.c:29-31〕；假设: 旧版本本来就留在原处（[[D-mvcc-no-overwrite]]），所以回滚不需要 undo（未找到提交或讨论原文）（另见 [[D-mvcc-no-overwrite]]、[[D-wal-redo-only]]）

### B15 [Decision] notes/arch-map/subsystems/12.md:120 
- D-pgstat-shmem {[[F4]]} 统计从收集进程+UDP+临时文件改为共享内存 ← 原始: 文件可达数十 MB、每秒写两次，阻碍新增统计；事务性删除后条目不再泄漏〔commit:5891c7a8ed8〕；当前: 本地 pending 批量 flush 基本消除条目争用〔file:src/backend/utils/activity/pgstat.c:43〕


## I1.R1
ANSWER: 必须先 XLogFlush 到该页 pd_lsn(WAL-before-data),保证对应 WAL 已持久化再写数据页。否则崩溃后数据页含有 WAL 中不存在的改动,无法恢复一致。
CONFIDENCE: high

## I1.R2
ANSWER: 顺序:START_CRIT_SECTION;修改页;MarkBufferDirty(两个 buffer 都要);XLogBeginInsert,XLogRegisterBuffer(每个被改 buffer)/XLogRegisterData;XLogInsert 得 LSN;PageSetLSN 给每个 buffer;END_CRIT_SECTION;最后 UnlockReleaseBuffer。buffer 必须持排他锁且 pin 住。临界区内不能 palloc/分配内存、不能抛 ERROR(出错即 PANIC),不能做可能失败的操作。
CONFIDENCE: high

## I1.R3
ANSWER: checkpoint redo 点后首次修改的页,WAL 记录会带整页镜像(FPI,full page image),后续同页记录只含增量。目的是防止 torn page(部分写):崩溃时磁盘页可能半新半旧,有 FPI 就可以整页覆盖恢复再重放。
CONFIDENCE: high

## I1.R4
ANSWER: 用 MarkBufferDirtyHint(只持 share lock 时),不写 WAL。开了 checksum/wal_log_hints 时,干净页首次因 hint 变脏前需要写一条 FPI 的 WAL(XLogSaveBufferForHint),避免 torn page 造成校验和错误。恢复期间(InRecovery)不能写 WAL,所以不做 FPI,且通常不允许因 hint 标脏(跳过标脏)。
CONFIDENCE: medium

## I1.R5
ANSWER: startup 进程里 MarkBufferDirtyHint 在恢复期间可能被直接跳过/不会真正使页脏(因不能写 FPI),改动会丢失。应改用 MarkBufferDirty(并按正常方式处理 LSN),因为这是需要保留的真实修改。
CONFIDENCE: medium

## I1.R6
ANSWER: 只有当事务 commit 记录的 LSN 已经刷盘(XLogNeedsFlush 为假,即 commit LSN <= 已 flush 位置)时才能设 hint(异步提交时用 clog 里记录的事务 commit LSN 比较)。否则崩溃后 commit 丢失,事务被视为中止,但元组上已有 COMMITTED hint,出现不一致。
CONFIDENCE: high

## I1.R7
ANSWER: 顺序:写 commit WAL 记录 → 刷 WAL(XLogFlush)→ 写 clog 标记 committed → 等同步复制(SyncRepWaitForLSN)→ ProcArrayEndTransaction 对他人可见 → 释放锁。可见性必须在 WAL 持久之后,锁最后释放。commit 记录与 clog 更新之间需用 delayChkptFlags(DELAY_CHKPT_START)保护,不允许 checkpoint 的 redo 点插在两者之间,否则 checkpoint 之后崩溃会丢 clog 更新。
CONFIDENCE: medium

## I1.R8
ANSWER: 新子系统要在 CheckPointGuts 里,在写 checkpoint 记录之前落盘(与 CLOG、SUBTRANS 等 SLRU 同处),且在 redo 点确定之后。顺序:确定 redo 点 → 落盘所有状态并 fsync → 写 checkpoint WAL 记录并刷 → 更新 pg_control → 最后才回收旧 WAL。redo 点之前写入的内容必须在 checkpoint 完成前全部持久化,因为重放不会再覆盖到 redo 点之前的 WAL。
CONFIDENCE: medium

## I1.R9
ANSWER: 不能重试,应 PANIC(data_sync_retry=off 默认)。因为 fsync 失败后内核可能已丢弃脏页并清除错误状态,下次 fsync 可能成功但数据实际没写盘,造成静默数据丢失;PANIC 后从 WAL 恢复才安全。
CONFIDENCE: high

## I1.R10
ANSWER: 自己要对写入的页算好 LSN/校验和,记 WAL(log_newpage,写整页镜像,内容就是写盘的页),并在提交前对数据文件 smgrimmedsync/fsync(或者注册到 checkpoint 的 sync 请求)。WAL 记录与写盘内容相同,先写 WAL 再写文件,且需保证 fsync 在提交之前完成,崩溃后 WAL 能重放出这些页。
CONFIDENCE: medium

## I1.R11
ANSWER: wal_level=minimal 时新 relfilenumber 的批量数据不写 WAL(跳过 WAL),提交前必须 fsync 该关系文件(smgrDoPendingSyncs,小表则改写 WAL)。该 relfilenumber 在 checkpoint 之前不能被重用/新建同号文件,避免崩溃后重放对旧文件的影响;事务中止则文件删掉。
CONFIDENCE: medium

## I1.R12
ANSWER: DROP TABLE 提交后(commit 记录写入之后、事务提交处理时)才能 unlink。新建表后中止:在 abort 时删除文件;提交时:对 drop 的在提交时删。删文件前要先向 checkpointer 发送 forget/取消该文件的 fsync 请求(ForgetRelationSyncRequests/向 checkpointer 发 SYNC_UNLINK_REQUEST),使之不会对已删文件 fsync 报错;文件通常先 truncate 到0,checkpoint 后才真正 unlink。
CONFIDENCE: medium

## I1.R13
ANSWER: 顺序:先 DropRelationBuffers 丢弃要截掉部分的 buffer?标准顺序是:临界区内先写 WAL(XLOG_SMGR_TRUNCATE),然后丢弃 buffer,最后截文件(smgrtruncate)。WAL 记录 + 截文件放在临界区(失败即 PANIC)。截文件失败时应 PANIC,因为 WAL 已写,状态不一致只能靠重放恢复。
CONFIDENCE: medium

## I1.R14
ANSWER: 写出前要保证 WAL 已 flush 到该页 LSN 之前的位置,即更新 minRecoveryPoint(XLogFlush 在恢复中的作用是推进 minRecoveryPoint)到该页 LSN。只有恢复到达一致性点(minRecoveryPoint / backup end 之后,reachedConsistency)后,备库才允许接受只读连接(hot standby)。
CONFIDENCE: high

## I1.R15
ANSWER: XLogReadBufferForRedo 返回 BLK_DONE 时(页 LSN >= 记录 LSN)应跳过;BLK_NEEDS_REDO 则重放,然后 PageSetLSN(记录的 EndRecPtr)并 MarkBufferDirty,最后解锁。只有 startup 进程(redo)修改数据块;redo 中不能写新 WAL。
CONFIDENCE: high

## I1.R16
ANSWER: 需要对该页立即把改动落盘/写 WAL 之外的保证:要对 init fork 的 buffer 做 FlushOneBuffer 并 fsync(或 log_smgrcreate 之类),因为恢复结束时 ResetUnloggedRelations 是直接拷贝 init fork 的磁盘文件到主 fork,而不经过 shared buffers,所以必须确保 init fork 内容已写到磁盘。
CONFIDENCE: medium

## I1.R17
ANSWER: 崩溃重启后(恢复开始前)清理 unlogged 表:删除主 fork、FSM 等所有非 init fork 的文件。恢复结束时(ResetUnloggedRelations 的 init 阶段)把 init fork 拷贝为主 fork(空表),并 fsync,因此得到可用的空表。
CONFIDENCE: high

## I1.R18
ANSWER: 段需满足:段尾早于 checkpoint 的 redo 点(KeepLogSeg 取 redo 点/最新 checkpoint 之前);已被归档(archive_status 的 .done,即归档完成,archive_mode 开时);不被任何复制槽的 restart_lsn 需要(包括 max_slot_wal_keep_size 限制下);不是 wal_keep_size 要保留的;WAL summarizer 还没处理完的段也要保留(summarize_wal 时需要)。
CONFIDENCE: medium

## I1.R19
ANSWER: 上限是已 flush(fsync)的 WAL 位置(GetFlushRecPtr;备库级联时用 replay/receive 的 flush 位置)。因为未 flush 的 WAL 主库崩溃可能丢失,而备库已收到,会造成备库比主库超前、时间线分叉/数据不一致。
CONFIDENCE: high

## I1.R20
ANSWER: 读 WAL 时通过记录头校验:页头 magic/xlp_pageaddr、xl_prev 链接、CRC 校验失败或 xl_tot_len 非法就认为到了 WAL 末端。截掉的不完整记录处写一条 "overwrite contrecord" 记录(XLOG_OVERWRITE_CONTRECORD)并在新页的 XLP_FIRST_IS_OVERWRITE_CONTRECORD / abortedRecPtr 标志;下游(归档/流式备库)看到它就丢弃已收到的前半截不完整记录。
CONFIDENCE: medium

## I1.R21
ANSWER: 提升时选新 TLI,先写 timeline history 文件(<TLI>.history)并 fsync,再通知归档(XLogArchiveNotify 的 .ready),然后写 end-of-recovery 记录/checkpoint,新 WAL 开在新时间线。切换可发生在段的中间(新段从复制旧段开头开始)。其他进程如 checkpointer 必须注意 ThisTimeLineID 变化,在恢复结束后才能写 WAL,不能在 promote 中途用旧 TLI 写。
CONFIDENCE: low

## I1.R22
ANSWER: DROP DATABASE:在不可逆删除前先拒绝新连接/确保无其他连接(CountOtherDBBackends),并先写 WAL(XLOG_DBASE_DROP)并 flush,再 forget fsync/drop buffers,然后删目录,提交。CREATE DATABASE FILE_COPY:前后各做 checkpoint(RequestCheckpoint,先 flush 所有脏 buffer)以保证拷贝文件为磁盘最新,并写 WAL(XLOG_DBASE_CREATE)。DROP TABLESPACE:表空间必须为空,并且需同步提交(强制 XactLastRecEnd flush)。
CONFIDENCE: low

## I1.R23
ANSWER: 在临界区内:先写 XLOG_RELMAP_UPDATE WAL 记录,然后写新的 pg_filenode.map 文件(临时写 + rename 并 fsync),之后再发送 relcache 失效消息。锁 RelationMappingLock 要持有整个过程;失效消息要在文件更新之后,使其他后端重读新 map。
CONFIDENCE: medium

## I1.R24
ANSWER: WAL 记录写入的是预取后的未来状态(last_value 向前推 SEQ_LOG_VALS=32 个,log_cnt 为0),即记录 "已发出到的位置"。checkpoint 后第一次 nextval 必须再写一条 WAL(因 FPI/重新预取)。崩溃恢复后从 WAL 记录里的预取值继续,故不会重发已取过的值(最多跳号)。
CONFIDENCE: medium

## I1.R25
ANSWER: 崩溃落在分裂与插入父页之间,索引处于 "incomplete split" 状态:左页有 INCOMPLETE_SPLIT 标志,新右页已经存在,但父页没有下链接。代码必须能处理:后续插入/vacuum 遇到该标志时完成分裂(_bt_finish_split),搜索靠 right-link 可以走到右页,保证正确。
CONFIDENCE: high

## I1.R26
ANSWER: 用 procsignal barrier(ProcSignalBarrier)机制:发起者发出 barrier,等待所有后端吸收(absorb)该 barrier 后才能宣告完成。启用方向需要先进入 inprogress-on 状态,由 datachecksumsworker 重写所有页,再切 on;停用方向直接切(inprogress-off 后 off),不需要重写数据,所以有区别。
CONFIDENCE: medium

## I8.R1
ANSWER: ControlFileData 改字段 → PG_CONTROL_VERSION;WAL 记录结构改 → XLOG_PAGE_MAGIC;pgstat 文件结构 → PGSTAT_FILE_FORMAT_ID。漏提旧数据可能被误解读(崩溃、读到垃圾),提升后旧数据被拒绝/丢弃(统计文件直接丢弃)。
CONFIDENCE: high

## I8.R2
ANSWER: 需要升级 catversion:改了 catalog 初始数据(pg_proc 参数名)会改变 initdb 结果。primnodes.h 节点字段改动会改变存储在 catalog 里的序列化节点树(视图、默认值表达式等),也需要提升 catversion。所以两个提交都要。
CONFIDENCE: high

## I8.R3
ANSWER: 稳定分支不能改记录格式(会破坏主备不同小版本的兼容)。应保持格式不变,让重放代码向后兼容地处理(例如重放逻辑兼容旧行为),开发分支同时做相应修复。运维上通常先升级备库再升级主库,这样备库能理解主库新行为产生的 WAL。
CONFIDENCE: medium

## I8.R4
ANSWER: 启动时比较 pg_control 里这些值,不一致则拒绝启动并报错(数据库集群与服务器不兼容,需要 initdb)。opclass 依赖 char 符号性会导致不同平台(x86 vs ARM)排序不一致,索引损坏。处理方法:改用显式 signed char/unsigned 比较,并要求/记录 pg_control 中的 default char signedness 字段(新增),不一致时处理。
CONFIDENCE: medium

## I8.R5
ANSWER: pg_upgrade 复制/链接用户数据文件,但 visibility map 和 FSM 可重建(VM 格式变化时 pg_upgrade 会重写 VM 文件)。数据类型磁盘表示变化则无法直接复制,需要 pg_upgrade 做检查或要求用户处理。同提交里要配套修改 pg_upgrade(如 rewriteVisibilityMap),提升 catversion。
CONFIDENCE: medium

## I8.R6
ANSWER: 这些值由旧集群决定,pg_upgrade 通过 binary_upgrade_set_next_* 函数(如 binary_upgrade_set_next_heap_relfilenode, pg_type_oid)在新集群预先设置。后端在 binary upgrade 模式(IsBinaryUpgrade)创建对象时使用这些预设值而不是自己分配新 OID/relfilenode。
CONFIDENCE: high

## I8.R7
ANSWER: 稳定分支不能破坏 ABI。结构体加成员只可加在末尾/利用 padding 或空闲字段,且不改变 sizeof(若会改变大小则不行)。NodeTag 要加在枚举末尾,不能插入中间(否则后续 tag 值改变)。若扩展按数组下标访问该结构体,则增加成员改变 sizeof 会破坏,不可接受。
CONFIDENCE: medium

## I8.R8
ANSWER: PG_MODULE_MAGIC 里含主版本号等,旧大版本编译的库加载时报 "incompatible library: version mismatch" 错误;完全没有 PG_MODULE_MAGIC 的库报 "missing magic block" 错误并拒绝加载。这个检查只比较大版本、FUNC_MAX_ARGS 等,不能发现小版本内的结构体变化。
CONFIDENCE: high

## I8.R9
ANSWER: 现代服务器回 NegotiateProtocolVersion 消息,告诉客户端自己支持的最高 minor 版本以及不认识的选项列表(`_pq_.` 选项),然后连接继续,使用较低版本、忽略不认识的选项。客户端遇到老服务器(不会协商,直接报错)时要重试,用较低的协议版本(如 3.0)重新连接。
CONFIDENCE: high

## I8.R10
ANSWER: 读消息读一半时被中断/ERROR 会使协议流失去同步(消息读了一半),不能继续会话,应当作 FATAL(pq_comm_reset/ 结束连接)。传输层(TLS/GSS)读写出错也需要 FATAL 终止连接,因为加密层状态已损坏,无法恢复。不能照常 ERROR 因为 ERROR 后后端会继续读下一条消息,但流已错位。
CONFIDENCE: medium

## I8.R11
ANSWER: 发布端根据订阅端请求的 proto_version 判断是否允许(pgoutput 中检查版本号 <= LOGICALREP_PROTO_MAX_VERSION_NUM);订阅端根据自身服务器版本/能力(远端版本)决定请求哪个版本。三处配套改动:提升协议版本常量(logicalproto.h),pgoutput 发送端按版本判断,订阅端(worker/walreceiver)解析新消息并按远端版本请求。
CONFIDENCE: low

## I8.R12
ANSWER: pg_upgrade 要求新旧集群的 data checksum 设置一致,不一致就报错退出(因为数据文件是复制/链接的,页上的 checksum 无法改变);旧集群处于在线启用中间状态,也应拒绝/要求先完成。因此需在 initdb 时用匹配的 --data-checksums 选项。
CONFIDENCE: medium

## I1.R1
ANSWER: 必须先 XLogFlush 到该页的 page LSN（WAL-before-data），FlushBuffer 在写页前做这一步。否则页落盘而对应 WAL 未持久，崩溃后数据页包含没有 WAL 的改动，可能是撕裂页/与 clog 等不一致，redo 无法修复或回滚。
CONFIDENCE: high

## I1.R2
ANSWER: 先对两个 buffer 都 pin 并持排他锁（按固定顺序加锁防死锁）；START_CRIT_SECTION；修改两页；对每个改动的 buffer MarkBufferDirty；XLogBeginInsert、XLogRegisterBuffer(0/1,…)、XLogRegisterData；lsn=XLogInsert；对每个改动页 PageSetLSN；END_CRIT_SECTION；再解锁/释放。所有改到的 buffer 都必须注册进 WAL 且在写 WAL 前已标脏。
临界区内不能 ereport(ERROR)（会升级为 PANIC）、不能 palloc（除非预分配/允许的上下文）、不能做可能失败的 I/O 或等锁。
CONFIDENCE: high

## I1.R3
ANSWER: 第一次修改的 WAL 记录带该页的 full-page image（备份块，可去掉 hole/压缩），后续记录只含增量。判定依据是 page LSN <= 当前 RedoRecPtr。原因：checkpoint 后该页写盘可能撕裂（OS/磁盘非原子写），redo 从 redo 点开始，必须先用 FPI 还原整页，再应用增量。
CONFIDENCE: high

## I1.R4
ANSWER: 用 MarkBufferDirtyHint(buffer, buffer_std)。开 checksum 或 wal_log_hints 时，若该页自上次 checkpoint 后没有 FPI，需先 XLogSaveBufferForHint 写一条 XLOG_FPI_FOR_HINT（设置 DELAY_CHKPT_START），并更新页 LSN，以免撕裂页导致 checksum 失败。恢复期间不能写 WAL，所以此时（需要 WAL 的情况下）不把页标脏，hint 改动可能丢失，可接受因为可重算。
CONFIDENCE: high

## I1.R5
ANSWER: 在 startup 进程（RecoveryInProgress）且开了 checksum/wal_log_hints 时，MarkBufferDirtyHint 直接返回而不标脏，页可能被淘汰而改动丢失。需要保留的改动必须用 MarkBufferDirty（redo 中正常标脏，并设置页 LSN）。
CONFIDENCE: medium

## I1.R6
ANSWER: 只有当该事务 commit 记录的 LSN 已经刷盘（XLogNeedsFlush(commitLSN) 为假）才能设；否则跳过（SetHintBits 的检查）。因为设 hint 不推进页 LSN，bufmgr 写页时不会强制刷到 commit 记录；若页带着 COMMITTED hint 落盘而 commit 记录随崩溃丢失，事务实际被视为中止，元组却被标为已提交。
CONFIDENCE: high

## I1.R7
ANSWER: 顺序：设置 DELAY_CHKPT_START，进入临界区 → 写 commit WAL → XLogFlush（同步提交）→ 写 clog(TransactionIdCommitTree) → 退出临界区并清 delayChkpt → SyncRepWaitForLSN 等同步复制 → ProcArrayEndTransaction 使他人可见 → 释放锁。clog 必须在 WAL 之后；可见性早于释放锁，以便等锁者看到提交。
checkpoint 要求：commit 记录插入到 clog 更新之间禁止 checkpoint 取 redo 点（DELAY_CHKPT_START），否则 redo 点落在 commit 记录之后，而 checkpoint 写出的 clog 不含此提交，恢复时不再重放该记录，提交丢失。
CONFIDENCE: high

## I1.R8
ANSWER: 放在 CheckPointGuts 中：redo 点确定之后、写 checkpoint 记录之前，完成写出并 fsync。顺序：确定 redo 点 → CheckPointGuts（CLOG/SLRU/buffers 等落盘）→ 插入并刷 checkpoint 记录 → 更新 pg_control → 删除/回收旧 WAL。redo 点之前的所有改动必须在 checkpoint 记录写入前持久化（之后恢复不再重放这些 WAL）；redo 点之后的改动可依赖 WAL 重放，因此子系统的 redo 必须幂等。
CONFIDENCE: high

## I1.R9
ANSWER: 不能。默认 data_sync_retry=off，fsync 失败直接 PANIC。因为 Linux 等内核在 fsync 报错后可能已丢弃脏页并清除错误状态，下一次 fsync 会返回成功但数据已丢；只有 PANIC 后从 checkpoint 起重放 WAL 才能恢复正确数据。
CONFIDENCE: high

## I1.R10
ANSWER: 对每页写 WAL（log_newpage/log_newpages 的 FPI），设页 LSN，算 checksum（PageSetChecksumInplace）后用 smgrextend/smgrwrite 写盘；WAL 内容即写盘的完整页映像。因为绕过 shared buffers，checkpoint 不会代为刷这些页：结束时若期间 redo 点未变化可向 checkpointer 注册 fsync 请求，否则（有 checkpoint 越过了这些 WAL）必须自己 smgrimmedsync，否则崩溃后 redo 从更晚的点开始，页可能丢失。
CONFIDENCE: medium

## I1.R11
ANSWER: 对当前事务新建的 relfilenumber，minimal 下跳过数据 WAL（RelationNeedsWAL 为假）。提交前 smgrDoPendingSyncs：小于 wal_skip_threshold 的写 FPI 进 WAL，否则对文件 fsync，然后才写 commit 记录。重用限制：被删除的 relfilenumber 在下一个 checkpoint 之前不能被重用（mdunlink 留下截空的首段作为墓碑，checkpoint 后才删），否则重放旧 relfilenumber 的 WAL（如 truncate/drop）会破坏跳过 WAL 的新数据。
CONFIDENCE: medium

## I1.R12
ANSWER: DROP 通过 pending delete 记录，只有在提交（commit 记录已写，含要删的 rel 列表）后 smgrDoPendingDeletes 时才 unlink；创建后中止则在 abort 时删除（abort 记录也带该列表）；创建提交则保留。主 fork 第一段被截为 0 字节并保留到下次 checkpoint 后再删，其它段立即删。删文件前要 DropRelationBuffers，并对 checkpointer 发 forget 请求（register_forget_request/取消已排队的 fsync），否则 checkpointer fsync 时打不开文件而报错/PANIC。
CONFIDENCE: high

## I1.R13
ANSWER: 顺序：先写 XLOG_SMGR_TRUNCATE 并 XLogFlush → 丢弃截断范围内的 buffer（DropRelationBuffers）→ smgrtruncate 截文件。新版本把这一串放在临界区内（并设 DELAY_CHKPT_START|COMPLETE，防 checkpoint 插在中间），所以截文件失败会 PANIC，重启后由 WAL 重放完成截断；否则已丢弃的脏 buffer 无法恢复，旧数据会“复活”。
CONFIDENCE: medium

## I1.R14
ANSWER: 写出前要把 minRecoveryPoint 推进到至少该页 LSN（恢复期 XLogFlush 调 UpdateMinRecoveryPoint 更新 pg_control），确保崩溃后至少重放到该点才算一致。只有当重放越过 minRecoveryPoint（且备份结束点已到达）达到一致状态，并且通过 running-xacts 记录得到有效快照（STANDBY_SNAPSHOT_READY）后，才接受只读连接。
CONFIDENCE: high

## I1.R15
ANSWER: 用 XLogReadBufferForRedo：BLK_DONE（页 LSN >= 记录 LSN，已应用）或 BLK_NOTFOUND（关系后续被删/截断）时跳过；BLK_RESTORED 表示已用 FPI 还原；BLK_NEEDS_REDO 时应用改动后 PageSetLSN(page, lsn)、MarkBufferDirty，然后 UnlockReleaseBuffer。恢复期只有 startup 进程修改数据块，redo 中不能写新 WAL。
CONFIDENCE: high

## I1.R16
ANSWER: 改完 init fork 页后必须立即把该 buffer 写盘（FlushOneBuffer），因为恢复结束时 ResetUnloggedRelations 是在文件级拷贝 init fork 生成主 fork，读的是磁盘文件而非 shared buffers；不刷盘会拷到旧内容。
CONFIDENCE: high

## I1.R17
ANSWER: 恢复开始时 ResetUnloggedRelations(CLEANUP)：对有 init fork 的关系删除除 init fork 外的所有 fork（main、fsm、vm）。恢复结束时 ResetUnloggedRelations(INIT)：把 init fork 拷贝为主 fork（以及需要时其它 fork），并 fsync，得到空表。
CONFIDENCE: high

## I1.R18
ANSWER: 段必须早于最近 checkpoint redo 点所在段，并再减去 wal_keep_size；不能被任何复制槽的 restart_lsn 需要（受 max_slot_wal_keep_size 限制，超出则槽失效）；开归档时必须已归档（.done，XLogArchiveCheckDone）；开 summarize_wal 时必须已被 WAL summarizer 汇总。之后按 max_wal_size 决定回收重命名还是删除。
CONFIDENCE: high

## I1.R19
ANSWER: 主库以已 fsync 的位置（GetFlushRecPtr）为上限；级联备库以本地已刷盘接收位置/已重放位置为上限。未刷盘的 WAL 在主库崩溃后可能丢失，下游却已拥有，导致主备分叉（备库领先于主库的 WAL 历史）。walreceiver 回报的 write/flush/apply 也只能报告真正完成的位置，同步复制按 flush/apply 判定。
CONFIDENCE: high

## I1.R20
ANSWER: 读者通过无效记录判定末端：页头 magic/pageaddr/TLI 不符、xl_tot_len 不合法、xl_prev 不匹配、CRC 错或续页标志不符。截掉的不完整记录要写 XLOG_OVERWRITE_CONTRECORD 记录（记录被丢弃的 contrecord LSN），并在新页头设 XLP_FIRST_IS_OVERWRITE_CONTRECORD。已拿到前半段的下游（归档恢复、流式备库）据此知道前半记录已被放弃，跳过它而不是报错或一直等后半段。
CONFIDENCE: high

## I1.R21
ANSWER: 新 TLI = 已知最大 TLI+1；先写并 fsync timeline history 文件并 XLogArchiveNotify，然后才在新时间线上写 WAL。切换点在最后重放成功记录的结尾（记录边界，可在段中间，旧时间线最后一段改名 .partial，复制前半到新 TLI 段），之后写 end-of-recovery 记录/checkpoint。checkpointer 等写 WAL 的进程必须使用当前插入时间线（GetWALInsertionTimeLine），不能用缓存的旧 TLI，删除/回收 WAL 也要考虑新旧 TLI。
CONFIDENCE: medium

## I1.R22
ANSWER: DROP DATABASE：不能在事务块中；检查/终止其他连接；不可逆删除前先把 pg_database 行标记为无效（datconnlimit=-2，原地更新并写 WAL、刷盘），这样中途崩溃后库被视为无效只能再删；然后丢 buffer、forget fsync 请求、ProcSignalBarrier 让各进程关闭文件、写 XLOG_DBASE_DROP 并刷盘后删目录。CREATE DATABASE FILE_COPY：拷贝前做强制 checkpoint 使源库落盘，拷贝目录、写 WAL，之后再做 checkpoint，避免重放时重新拷贝已变化的源库。DROP TABLESPACE：表空间必须为空（必要时 checkpoint 清除墓碑文件后重试），写 XLOG_TBLSPC_DROP，并强制同步提交（ForceSyncCommit），不能在事务块中。
CONFIDENCE: medium

## I1.R23
ANSWER: 持 RelationMappingLock，在临界区内：先写 XLOG_RELMAP_UPDATE 并 XLogFlush → 写临时文件、fsync、durable_rename 替换 pg_filenode.map → 然后发送 relmap 失效消息（CacheInvalidateRelmap）让其他后端重读。事务内对 mapped catalog 的更新在提交时（RecordTransactionCommit 之前，AtEOXact_RelationMap）才写入共享 map。临界区内失败即 PANIC。
CONFIDENCE: medium

## I1.R24
ANSWER: WAL 中写的是“预取后”的状态：last_value 提前 SEQ_LOG_VALS(32) 个值、log_cnt=0，内存/页上记录还剩可用值数。checkpoint 后第一次 nextval（页 LSN <= RedoRecPtr）必须强制重新写 WAL，否则崩溃后从该 checkpoint 起没有 WAL 能推进序列。WAL 记录总是领先于已发出的值，且使用该值的事务提交时会刷到该记录（XactLastRecEnd），所以崩溃后只会跳号不会重复。
CONFIDENCE: high

## I1.R25
ANSWER: 左页已分裂并带 INCOMPLETE_SPLIT 标志，右页通过右链接可达，但父页缺下链接。索引仍然正确可搜索（沿 right-link move right）。代码必须能在之后完成未完成的分裂：插入者遇到该标志时调用 _bt_finish_split 补插父页下链接（恢复时不再做清理）。
CONFIDENCE: high

## I1.R26
ANSWER: 通过共享状态+WAL 记录+ProcSignalBarrier：发起者改状态后 EmitProcSignalBarrier 并 WaitForProcSignalBarrier，只有所有后端都吸收了屏障（更新本地状态，如开始写 checksum/写逻辑信息）后才能宣告切换完成。启用要经过中间态（inprogress-on：写但不校验；逻辑解码同理先让所有人开始写额外信息），全部页处理完再切到 on。停用方向可直接先停止校验/使用，再停止写入（inprogress-off），风险较小、不需要处理存量数据，但仍要用屏障保证各后端一致。
CONFIDENCE: low

## I8.R1
ANSWER: ControlFileData → PG_CONTROL_VERSION；WAL 记录结构 → XLOG_PAGE_MAGIC；pgstat 统计文件结构 → PGSTAT_FILE_FORMAT_ID。提了：旧 pg_control 被拒绝启动、旧 WAL 拒读、旧 stats 文件被丢弃（统计重置）。漏提：新二进制按新布局误读旧数据，pg_control/WAL 重放读出垃圾导致崩溃或数据损坏，stats 读出错误值。
CONFIDENCE: high

## I8.R2
ANSWER: 需要。catversion 覆盖 catalog 内容（包括初始数据如 pg_proc 的 proargnames），也覆盖存储在 catalog 中的节点树（pg_node_tree：视图/规则/默认值），primnodes.h 字段变化改变 out/readfuncs 序列化格式，旧 initdb 的集群读出会出错。两处都需提升 catversion 以强制 initdb。
CONFIDENCE: high

## I8.R3
ANSWER: 稳定分支不能改 WAL 记录格式（不能提 XLOG_PAGE_MAGIC）。重放代码要兼容新旧记录：通常用未用的 info 标志位或新记录类型表示新行为，旧记录按旧方式重放。开发分支可正常修改并提 XLOG_PAGE_MAGIC。运维上先升级备库再升级主库：新备库能重放旧主库 WAL，而旧备库遇到新记录/标志会出错或错误重放。
CONFIDENCE: medium

## I8.R4
ANSWER: ReadControlFile 校验这些编译常量，不一致即 FATAL（数据库文件与服务器不兼容，提示需 initdb），拒绝启动。char 符号性：x86 signed、ARM unsigned，持久索引顺序在不同平台不同，物理复制/pg_upgrade 后索引错乱。处理：pg_control 记录 default_char_signedness（initdb 设定、pg_upgrade 继承旧集群），受影响的 opclass（如 pg_trgm）用 GetDefaultCharSignedness() 显式按 signed/unsigned 比较。
CONFIDENCE: medium

## I8.R5
ANSWER: 用户数据文件由 pg_upgrade 复制/链接/克隆，不重建（只有 catalog 通过 dump/restore 重建）。所以同一提交里必须在 pg_upgrade 中加入转换（如 VM 格式变化时的 rewriteVisibilityMap，按旧版本号判断），或对数据类型表示变化增加检查使 pg_upgrade 在旧集群使用该类型时拒绝升级；同时提升 catversion。
CONFIDENCE: high

## I8.R6
ANSWER: 由旧集群决定：pg_dump --binary-upgrade 在每个对象前输出 binary_upgrade_set_next_heap_pg_class_oid / _heap_relfilenode / _pg_type_oid / _array_pg_type_oid / toast/index 等调用。后端在 IsBinaryUpgrade 下使用 binary_upgrade_next_* 变量取值并立即重置为无效，未设置时报错（而非 GetNewOidWithIndex 分配新值），以保证数据文件名与 OID 引用（如数组/复合类型在数据中）保持不变。
CONFIDENCE: high

## I8.R7
ANSWER: 稳定分支要保持 ABI：只能在结构体末尾加成员（或利用现有 padding 空洞），且前提是扩展不会自己分配该结构体/嵌入/依赖 sizeof；不能在中间插入或重排成员。新 NodeTag 只能加在枚举末尾，不能让现有值重新编号。若扩展按数组下标访问该结构体，sizeof 改变会使下标偏移错乱，加成员不可接受，需换方案（如旁路数组/单独结构）。
CONFIDENCE: medium

## I8.R8
ANSWER: 旧大版本编译的库：Pg_magic_struct 校验（大版本、FUNC_MAX_ARGS、INDEX_MAX_KEYS、NAMEDATALEN、FLOAT8PASSBYVAL、abi_extra）不符，报 ERROR "incompatible library: version mismatch" 并拒绝加载。无 PG_MODULE_MAGIC：报 ERROR "missing magic block"，提示加入 PG_MODULE_MAGIC。该检查只按大版本，不能发现小版本内的结构体变化。
CONFIDENCE: high

## I8.R9
ANSWER: 服务器回 NegotiateProtocolVersion 消息，给出它支持的最高 minor 版本和不认识的 _pq_. 选项列表，连接继续按协商的较低版本进行，未识别选项被忽略。客户端检查协商结果是否满足自身最低要求（min_protocol_version 等），不满足则断开。遇到不支持协商的老服务器（直接报错“unsupported frontend protocol”）时，客户端需降级用 3.0 重连或报错。
CONFIDENCE: medium

## I8.R10
ANSWER: 读消息途中出错：协议同步已丢失（PqCommReadingMsg），不知道消息边界，必须 FATAL 终止连接，否则剩余字节会被当作新消息解析（错乱或注入）。TLS/GSS 传输层出错：通道已坏，用 COMMERROR 只记服务器日志、不发给客户端（避免在坏通道上发送导致递归），返回 EOF 并结束会话。
CONFIDENCE: high

## I8.R11
ANSWER: 发布端 pgoutput 从启动选项拿 proto_version，校验不超过 LOGICALREP_PROTO_MAX_VERSION_NUM，只有在 proto_version >= 新版本号（及对应选项开启）时才发送新消息。订阅端根据发布端服务器版本（walrcv_server_version）和自身能力选择请求的版本。配套：logicalproto.h 定义新版本号并提升 MAX；proto.c/pgoutput 写端及版本检查；worker.c apply_dispatch 处理新消息与版本选择；更新协议文档。
CONFIDENCE: medium

## I8.R12
ANSWER: pg_upgrade 检查新旧集群 pg_control 的 data_checksum_version，必须一致，否则报错拒绝升级；旧集群处于在线启用/停用中间状态也应拒绝。原因：数据文件原样复制/链接，不会重写 checksum；状态不一致会导致读取时 checksum 校验失败或页面没有有效 checksum。
CONFIDENCE: medium

## C2-01  derivation.md:152
- (1) 退出码不是 0/1 就算崩溃 — CORRECT — postmaster.c:2631 `if (!EXIT_STATUS_0(exitstatus) && !EXIT_STATUS_1(exitstatus)) crashed = true;`
- (2) 退出时"正在使用"标记没撤掉（dead man switch）就算崩溃 — CORRECT — postmaster.c:2657-2668 `if (!ReleasePostmasterChildSlot(bp)) ... crashed = true`; pmsignal.c:260 `result = (PMChildFlags[slot] == PM_CHILD_ASSIGNED)`; 子进程置 ACTIVE 并注册 on_shmem_exit 回 ASSIGNED (pmsignal.c:283-300); "dead man switch" 见 ipc.c:296。
- (3) postmaster 不信任共享内存一般内容，标记"即使被写坏也最多让它误判谁崩溃了" — MAP-ONLY — 源码只有 pmsignal.c:96-98 "Postmaster keeps a local copy [of num_child_flags] so that it doesn't need to trust the value in shared memory" 和 dsm.c:378-381；"最多误判"这句是 I7.R1 / I7 §5 的设计推断，源码没有明说。可改成"设计意图是……"。
- CLARITY: "标记没有撤掉"可能被读成 postmaster 在读标记的内容判断崩溃；实际是标记没回到"已分配"状态。另外 WALSENDER 状态也会不等于 ASSIGNED，不影响结论。

## C2-02  derivation.md:266
- (1) "上面的另一条路"里比较过三条路 — CORRECT — derivation.md:233-239 三条（线程、POSIX 互斥量、重锁）；principles.md D-lock-tiers "被否方案 (1)(2)(3)" 同。第 (3) 条来自 lwlock.c 文件头的注释，不是讨论串里的比较，"被比较过"略宽。
- CLARITY: 同一块"另一条路"里还有一段"教科书层面"的两个极端，不属于这三条；读者可能数成五条。

## C2-03  derivation.md:345
- (1) WAL 代码 1999 年出现 — CORRECT — `git log --diff-filter=A -- src/backend/access/transam/xlog.c`: 30659d43eb7 1999-09-27 "Transaction log manager core code."
- (2) 提交信息没有理由 — CORRECT（限首个提交及紧随的 47937403676）— 30659d43eb7 正文只有 "It doesn't work currently but also don't break anything -:)"；47937403676 只说 "First step in cleaning up backend initialization code"，都未给出为什么要 WAL。
- CLARITY: "原始原因"未说明是"为什么要有预写日志"的原因；读者可能以为"没有理由"是指 WAL 没有理由。可改成"当年的提交信息没有解释为什么要做 WAL"。

## C2-04  derivation.md:479
- (1) 只读事务延迟分配事务号，不写数据的事务不碰共享的事务号计数 — CORRECT — transam/README:193-195 "assigned permanent XIDs only when/if they first do something that requires one"; README:197-206 VXID "without any contention for shared memory"。
- (2) 结束时也不必持这把锁 — OVERSTATED — procarray.c:688-691 "If we have no XID, we don't need to lock"（README:265-270 同）。缺的条件：procarray.c:697-707 若 statusFlags 带 PROC_VACUUM_STATE_MASK，仍会取 ProcArrayLock 排他锁。补"通常"。
- (3) 因此"减轻了取快照扫描这笔代价" — MAP-ONLY — 源码只说明无 XID 的事务结束时不取锁、不影响别人的快照 (procarray.c:688-690)，减少的是结束路径上的锁争用，不是"在共享锁下扫描集合"本身的成本；README 没有把它说成对取快照成本的补偿。principles.md 没有 `### D-lazy-xid-vxid` 记录，只在 subsystems/7.md:170 有一行摘要（"原始: 未考证"），指针悬空。
- CLARITY: "这把锁"指 ProcArrayLock，本句里没有出现名字，非程序员读不出是哪把锁（上文只说"共享锁"，实际结束时取的是排他模式）。

## C2-05  derivation.md:534
- (1) 地图的独立性检查指出该决策不可由 4.1 推出，原地更新同样可以把锁记在行头 — CORRECT（作为对地图的转述）— principles.md:210 "独立性：不可由 [[D-mvcc-no-overwrite]] 推出，原地更新的设计同样可以把锁记在行头。"
- (2) "与 4.1 无关" — OVERSTATED — 地图说的是"不可推出"，不是"无关"。同书 derivation.md:549 与 principles.md:199 写明行锁"是随 MVCC 一起出现的"，并复用 MVCC 的 xmax（README.tuplock:8-10 "setting the current transaction's XID as its XMAX"）。应写"不能由 4.1 推出"。
- (3) 反事实"原地更新也可以把锁记在行头" — MAP-ONLY — 没有源码或提交比较过这种设计；README.tuplock:5-13 的理由只依赖"锁数量无上界、共享内存放不下"（E6），与更新方式无关，这条只能支持"理由与 MVCC 无关"。
- CLARITY: "原地更新"未解释（对应 4.1 的"另一条路"，直接读本句的人不知道指什么）。

## C2-06  derivation.md:591
- (1) "几乎每一步都要查 catalog" 派生出每进程缓存和整套失效规则 — MAP-ONLY — 来自 principles.md:323 "下游代价：几乎每一步都要查 catalog → 每个后端各自缓存"；catcache/relcache 确实存在，但源码没有这句话，"几乎每一步"是地图的定性说法，无计量依据。另外每进程缓存还要靠 2.1 才成立（derivation.md:577 已写）。
- (2) 缓存一致性成了一个独立的不变量族 [[I4]] — CORRECT — invariants/I4.md 标题 "I4 缓存一致性"，§1 给出不变量。
- CLARITY: "catalog"、"每进程缓存"在 5.1 开头已有解释，此句可读；"不变量族"对非程序员需一句话说明（"必须始终成立的规则的一组"）。

## C2-07  derivation.md:664
- (1) 每后端缓存的必要性来自 5.1 和 2.1 — CORRECT — principles.md:353 同；derivation.md:577 的逐条推导一致。
- (2) "共享队列并与锁捆绑"是独立的选择 — MAP-ONLY — 这是地图自己的独立性判断（principles.md:353），原始原因和被否方案都"未考证"（principles.md:346,348）；源码只确认了机制本身（lmgr.c:136,171,202,265 取锁时调用 AcceptInvalidationMessages），没有与替代方案的比较。
- CLARITY: "独立性检查"是地图内部术语，正文没有解释；"独立的选择"可能被读成"有记录的、被比较过的选择"，实际只是"不能从前提推出"。

## C2-08  derivation.md:742
- (1) 不提供优化器提示，计划选错当作优化器缺陷处理（社区立场） — HISTORY-UNVERIFIED — principles.md:552-559 R9 的证据只有两个 wiki 页面（OptimizerHintsDiscussion、Not_Worth_Doing，均标"转述"）；本句没有 URL，未取回。仓库内未发现 hint 机制，与"不提供"一致，但这不能证明"社区立场"。
- (2) 已有若干排查用开关，效果接近提示 — CORRECT — guc_parameters.dat:972,1030 enable_nestloop、enable_seqscan（QUERY_TUNING_METHOD）。"社区也承认提示对一次性报表查询有好处"同属 wiki 转述，HISTORY-UNVERIFIED。
- (3) R9 是候选而非确认 — CORRECT — principles.md:558 "状态：候选（存在上述例外）"。
- CLARITY: "候选"、"R9"对读者需说明是"地图里尚未确认的原则编号"；"它预测……"的"它"指 R9，容易误读成"社区预测"。

## C2-09  derivation.md:873
- (1) 备库就是持续的恢复 — CORRECT — walreceiver.c 文件头："it updates WalRcv->flushedUpto ... to inform the startup process of how far it can proceed with XLOG replay"；复制来源枚举 xlogrecovery.c:220 XLOG_FROM_STREAM 是恢复读 WAL 的来源之一。
- (2) walreceiver 只替代"取得 WAL"，重放仍由 startup 完成 — CORRECT — walreceiver.c 文件头 "keeps receiving XLOG records and writing them to the disk"；重放由 startup 进程推进（同段）。注意 walreceiver 还负责写盘与向主库回报，"只替代取得"是简化说法。
- CLARITY: 标成"当前原因"但内容是现状描述，不是理由；"startup"、"walreceiver"首次出现处在 7.1 之前是否已解释需核对。

## C2-10  derivation.md:931
- (1) 2025 年 4 月 Bruce Momjian 说他写 pg_upgrade 时以为"改磁盘格式的价值总有一天会超过原地升级" — HISTORY-UNVERIFIED — 来源是邮件 mail-archive msg196724（principles.md:416，转述）；本句没有 URL，未取回。"pg_upgrade 作者是 Bruce"有提交支持：c2e9b2f2881 / 78a09145e0f 作者 Bruce Momjian（2010-05-12、2009-12-19）。
- (2) 含义是"取舍至今偏向后者"（保留原地升级） — OVERSTATED — 引文只是 Bruce 个人的判断（"I guess that hasn't happened yet"，principles.md:416），不是社区的取舍结论；"至今偏向"应写"在他看来至今偏向"。
- (3) 地图没有读到由此形成的正式结论 — CORRECT（地图自述）— principles.md:417 "没有读到由此形成的正式结论"。
- CLARITY: "原地升级"与上文 pg_upgrade 的说明（"把数据文件原样搬过去"）对应，但"改磁盘格式"为何与之冲突没说，非程序员可能不明白两者为什么互斥。

## 汇总

| 判定 | 数量 |
|---|---|
| CORRECT | 14 |
| OVERSTATED | 3 |
| WRONG | 0 |
| MAP-ONLY | 5 |
| UNSUPPORTED | 0 |
| HISTORY-UNVERIFIED | 3 (C2-08 (1), C2-08 (2) 后半, C2-10 (1)) |

有非 CORRECT 判定的块：C2-01 (3)、C2-04 (2)(3)、C2-05 (2)(3)、C2-06 (1)、C2-07 (2)、C2-08 (1)(2后半)、C2-10 (1)(2)。
C2-02、C2-03、C2-09 全部 CORRECT（仅有 CLARITY 备注）。
附带：[[D-lazy-xid-vxid]] 在 principles.md 中没有 `###` 记录，只在 subsystems/7.md:170 有摘要行。

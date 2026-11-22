---
id: principles
type: principles
depends: [L0]
---
# principles：推导链的根

基线：上游 `76c0df7381e`（2026-11-11）。规范见 GUIDE §4.5。推导顺序：环境前提 E × 力 F → 取舍规则 → 基础性决策 D → 原则 R。

**状态：第二遍（作用检验之后）。** `holdout.md` 里留出的 18 条决策没有被用作任何取舍规则或原则的支撑。
其中按扇出属于根层的 3 条在检验完成之后补进了 §4，另有 2 条记在 §4.1，都标注 `holdout`；
检验用的是不含这些条目的第一遍文本。检验中发现的不符已经改回本文件（T7、R2、R5），其余记在 `residuals.md` 的 [[X128]]–[[X138]]。

**证据的可信度分三档**，引用时标明：逐字（提交原文、仓库内文件、下载到的 TeX 源码）；转述（经抓取工具转述的网页，未逐字核对）；
未考证（查过、没有找到）。原因分原始、当前、假设三种，不混写（GUIDE §4.2）。

---

## 1. 环境前提 E

世界是什么样。只收被至少一条基础性决策依赖的前提。"现状"决定依赖它的决策是否该重审。

| ID | 前提 | 依赖它的决策 | 现状 | 证据 |
|---|---|---|---|---|
| E1 | 存储设备的写不是页原子的：掉电时一个 8kB 页可能只写进一部分 512 字节扇区 | [[D-full-page-writes]] | 仍成立；有的文件系统能保证整页写（文档举了 ZFS），此时允许关闭整页镜像 | file:doc/src/sgml/wal.sgml:164-180 |
| E2 | 操作系统提供把缓存强制刷到持久存储的接口；存储硬件可能带掉电即丢的写缓存，这一段的完整性由部署者负责 | [[D-wal-redo-only]] [[D-wal-archive-pitr]] | 仍成立；2018 年发现 fsync 失败后重试并不可靠，为此改成失败即 PANIC | file:doc/src/sgml/wal.sgml:35-38,49-51,148-152; commit:9ccdd7f66e3 |
| E3 | 进程是操作系统提供的、彼此隔离的执行单元；线程"历史上支持差且有 bug"，同进程内一个线程出错会破坏其他线程 | [[D-process-per-connection]] | 正在变化：2023 年起社区重新讨论多线程化，wiki 上列着进行中的工作 | url:https://wiki.postgresql.org/wiki/Developer_FAQ（转述）; url:https://www.postgresql.org/message-id/31cc6df9-53fe-3cd9-af5b-ac0d801163f4@iki.fi（转述）; url:https://wiki.postgresql.org/wiki/Multithreading（转述） |
| E4 | 认证用到的库（SSL、PAM）可能阻塞，而且不是多线程安全的 | [[D-process-per-connection]] | 未核实现状 | file:src/backend/postmaster/postmaster.c:25-33 |
| E5 | 多处理器是常态，共享数据结构上的锁争用是实际问题（2001 年的动因是 8 路 SMP 上表现差） | [[D-lock-tiers]] [[D-snapshot-xmin-xmax-xip]] | 仍成立，并且随核数和连接数加剧 | url:https://www.postgresql.org/message-id/1054.1001520600%40sss.pgh.pa.us（转述）; commit:dc7420c2c92 |
| E6 | 一个事务可能同时锁住数量没有上界的行，共享内存放不下每行一个锁对象 | [[D-row-lock-in-tuple-header]] | 仍成立 | file:src/backend/access/heap/README.tuplock:5-13 |
| E7 | 一个查询可选的执行方式数随连接数指数增长，近似穷举只在连接数小时可行 | [[D-path-plan-split]] | 仍成立 | file:doc/src/sgml/geqo.sgml:21-23,34-40 |
| E8 | 第三方扩展直接使用服务器内部的全局函数和变量，其中大部分"设计时没有考虑可扩展性和长期稳定" | [[D-catalog-driven-extensibility]] [[D-compat-by-release-class]] | 仍成立 | file:doc/src/sgml/xfunc.sgml:2808-2812,2841-2842 |
| E9 | 用户需要一个简单可靠的内建复制；外部方案"对简单场景来说安装和使用都太复杂"，妨碍了采用 | [[D-physical-rep-ships-wal]] | 仍成立（2008 年核心团队的判断） | url:https://groups.google.com/g/pgsql.hackers/c/5ZuMgmec2xk（转述，片段） |
| E10 | 保留旧行版本所占的空间，以及回收它们的后台 I/O，是负担得起的 | [[D-mvcc-no-overwrite]] | 正在变化：2018 年的 zheap 提案指出，更新密集的负载下空间往往不能完全回收 | url:https://www.postgresql.org/message-id/CAA4eK1+YtM5vxzSM2NZm+pC37MCwyvtkmJrO_yRBQeZDp9Wa2w@mail.gmail.com（转述） |
| E11 | 同时进行中的事务数有界（受连接数限制），取快照时复制它们的列表、并为此短暂持锁，代价可以接受 | [[D-snapshot-xmin-xmax-xip]] [[D-catalog-mvcc-heap]] | 正在变化：2020 年为此重做了快照的可伸缩性；2013 年起有改用提交序号的提案 | commit:dc7420c2c92; url:https://www.postgresql.org/message-id/CA+CSw_tEpJ=md1zgxPkjH6CWDnTDft4gBi=+P9SnoC+Wy3pKdA@mail.gmail.com（转述） |
| E12 | 后端用 C 编写，语言没有异常和析构机制 | [[D-error-longjmp-abort]] | 仍成立（要求 C11） | file:doc/src/sgml/installation.sgml:74-77; file:doc/src/sgml/xfunc.sgml:4129-4132 |
| E13 | 相对于查询，改 catalog 的操作是低频的 | [[D-sinval-with-locks]] | 仍成立；频繁建删对象的负载下失效队列曾是争用点，2008 年为此重写 | file:src/backend/utils/cache/plancache.c:36-38; commit:fad153ec452 |
| E14 | 操作系统的页缓存、预读和回写，足以替数据库隐藏同步 I/O 的代价 | [[D-own-buffer-cache]]（`holdout`） | 正在变化：2025 年加入异步 I/O，理由是内核"在预读和受控回写上做得不如期望"；直接 I/O 的开关仍只供开发测试，缓冲 I/O 仍是默认 | file:src/backend/storage/aio/README.md:7-10; commit:d4e71df6d75; commit:da7226993fd（`holdout`） |
| E15 | 一个集群在它的寿命里消耗的事务号会超过 2^32 个 | [[D-xid-wraparound-freeze]]（`holdout`） | 仍成立；2019 年引入 64 位的 FullTransactionId 跟踪事务号的生成，元组头里仍是 32 位 | file:doc/src/sgml/maintenance.sgml:431-436; commit:2fc7af5e966 |
| E16 | 附加代码是任意 C 代码，系统无法验证它的行为：例如一个函数是否并行安全，一般情形下等价于停机问题，规划器无法自动判定 | [[D-trust-declared-properties]] | 仍成立 | file:doc/src/sgml/ref/create_function.sgml:457-458; file:doc/src/sgml/parallel.sgml:533-541 |

---

## 2. 力 F

系统必须达到的目标。每个力写成可判定的要求，并写出违反它的具体表现。合格条件（GUIDE §4.5.4）：至少有一条基础性决策为它而做，并且在至少一条决策里被拿来和别的力交换过。

**F1 正确性与持久性**
- 要求：报告为已提交的事务在崩溃后仍然存在；任何查询看到的都是某个一致快照下的数据；一处故障不得损坏其他会话的数据。
- 违反的表现：已确认的提交丢失；静默的数据损坏；返回错误结果；一个后端的内存错误写坏共享状态后系统继续运行。
- 约束点：[[I1]] [[I2]] [[I7]]；[[S09]] [[S08]] [[S07]] [[S01]]。
- 为它而做：[[D-wal-redo-only]] [[D-full-page-writes]] [[D-process-per-connection]]。被交换：T1、T5。
- 证据：file:doc/src/sgml/wal.sgml:16-22,568-570。

**F2 并发**
- 要求：读不阻塞写，写不阻塞读，在最严格的隔离级别下也成立。
- 违反的表现：只读查询因为并发写入而等待，或者反过来。
- 约束点：[[I2]] [[I3]]；[[S06]] [[S07]]。
- 为它而做：[[D-mvcc-no-overwrite]] [[D-row-lock-in-tuple-header]]。被交换：T2。
- 证据：file:doc/src/sgml/mvcc.sgml:14-15,60-66。

**F3 可扩展性**
- 要求：新的数据类型、函数、运算符、访问方法能由附加代码加入，核心不需要事先知道它们。
- 违反的表现：加一种类型或索引必须修改核心源码。
- 约束点：[[I8]] [[I4]]；[[S11]] [[S06]] [[S12]] [[S03]]。
- 为它而做：[[D-catalog-driven-extensibility]] [[D-trust-declared-properties]]（用户定义的类型和函数也能参与优化）。被交换：T6。
- 证据：file:doc/src/sgml/extend.sgml:53-54,68-69; file:doc/src/sgml/tableam.sgml:17-20。

**F4 性能**
- 要求：在不违反 F1 的前提下，减少磁盘写、锁争用和每元组的开销；性能主张必须有可复现的基准。
- 违反的表现：可测量的回归；声称有收益却没有测试用例（这是补丁被退回的理由之一）。
- 约束点：[[S04]] [[S05]] [[S07]] [[S08]]。
- 为它而做：[[D-lock-tiers]] [[D-path-plan-split]] [[D-trust-declared-properties]]（预先求值、索引扫描、等价推理等优化）。被交换：T1、T2、T3、T4、T5、T8。
- 证据：file:doc/src/sgml/wal.sgml:519-522; url:https://wiki.postgresql.org/wiki/Submitting_a_Patch（转述）。

**F5 兼容性**
- 要求（按社区实际承诺的范围）：小版本升级不需要 dump/restore，也不得让按较早小版本编译的扩展失效；线上协议向后兼容到 3.0；
  大版本可以改磁盘格式和内部 API，但必须能用 pg_upgrade 或 dump/reload 升级。大版本之间没有 API 兼容承诺。
- 违反的表现：小版本里改了 `src/include` 的结构体布局；小版本升级后数据目录不可用；旧客户端连不上。
- 约束点：[[I8]]；全部子系统的磁盘格式、WAL 格式、协议、catalog。
- 为它而做：[[D-compat-by-release-class]]。被交换：T6、T7。
- 证据：url:https://www.postgresql.org/support/versioning/（转述）; url:https://wiki.postgresql.org/wiki/Committing_checklist（转述）; file:doc/src/sgml/protocol.sgml:21-24; file:doc/src/sgml/xfunc.sgml:2836-2837。

**F6 可移植性**
- 要求：代码在所有受支持的平台上能构建并通过回归测试；默认路径不依赖单一操作系统才有的特性。
- 违反的表现：一个功能只在某个平台上可用而没有回退路径。
- 约束点：[[S01]] [[S08]]。
- 为它而做：[[D-full-page-writes]]（不假定任何文件系统的写原子性）；[[D-own-buffer-cache]]（`holdout`；默认用哪里都有的缓冲 I/O）。被交换：T8。
- 证据：file:doc/src/sgml/installation.sgml:3432-3436; url:https://wiki.postgresql.org/wiki/Developer_FAQ（转述）。

**F7 可维护性**
- 要求：普通代码不必自带可靠的出错清理；加一种东西不需要同步修改多处；补丁只包含让它稳健工作所需的最小改动。
- 违反的表现：同一件事要在多个文件里手工保持一致；为特例复制一份通用机制。
- 约束点：[[I5]]；[[S12]] 以及全部子系统的代码形态。
- 为它而做：[[D-error-longjmp-abort]] [[D-process-per-connection]]（认证代码可以写成单线程风格）。被交换：T3、T4、T7。
- 证据：commit:372728b0d49; url:https://wiki.postgresql.org/wiki/Submitting_a_Patch（转述）。

力的校验结论：七个力都满足合格条件，没有被改归前提或不变量的。两处重新归类：
[[D-process-per-connection]] 的理由里，"线程支持差、认证库会阻塞"是前提（[[E3]] [[E4]]），不是力，所以它不算为 F6 而做；它为之而做的力是 F1（隔离）和 F7（认证代码可以写成单线程风格）。
F5 原先没有为它而做的根层决策，补了 [[D-compat-by-release-class]]。
安全（权限、行级安全、信任边界）没有列为力：地图里没有为它而做的决策，不满足合格条件。重构检验的读者指出了这个缺口，记在 [[X132]]。

---

## 3. 取舍规则

力与力冲突时谁优先。只从 §4 决策记录的"下游代价"归纳；不是全序；没有证据的成对比较不写。

**T1 F1 对 F4**：完整性不让步，持久性可以由用户按事务放松。
- 让步方的补偿：把 checkpoint 间隔设长，减少"checkpoint 后首次修改"带来的整页镜像（为了能设长而不造成 I/O 突发，checkpoint 的写出被摊开到间隔里，这是间接的补偿）；提供异步提交。文档的界线是"风险是丢数据，不是数据损坏"。
- 支撑：[[D-full-page-writes]]（接受 WAL 量增加）[[D-catalog-mvcc-heap]]（接受更贵的快照，换掉会漏看行的 SnapshotNow）[[D-wal-redo-only]]。
- 反例：未见默认行为牺牲完整性的决策；存在由用户自担风险的开关（关闭 fsync、关闭整页镜像）。
- 证据：file:doc/src/sgml/wal.sgml:593-594,657-660; file:doc/src/sgml/config.sgml:3610-3616。

**T2 F2 对 F4**：读写互不阻塞优先于空间和后台 I/O。
- 让步方的补偿：vacuum 回收旧版本；行锁带来的额外 multixact 查找被认为可以接受，因为它发生在原本会阻塞等待的场合。
- 支撑：[[D-mvcc-no-overwrite]] [[D-row-lock-in-tuple-header]]。
- 反例：未见已采纳的反例；改为原地更新加 undo 的 zheap 提案没有进入核心。
- 证据：commit:0ac5ad5134f。

**T3 F7 对 F4**：统一机制优先；绕过通用机制的快速路径只有在给出基准数据、并且通用路径仍然保留时才被接受。
- 让步方的补偿：给代价高的通用路径另开一条受限的旁路，例如为"已知可以安全恢复"的错误提供不走事务中止的软错误通道。
- 支撑：[[D-error-longjmp-abort]]（代价是每次 ERROR 都要做昂贵的事务中止）[[D-exec-pull-iterator]]。
- 反例（即规则里的例外条件）：commit:2da86c1ef9b 为外键检查加了绕过 SPI 的快速路径，附有 1.8 倍的基准；
  随后 commit:6aecabcd5ed 又把它收窄，理由是不愿为它复制一份表达式求值。
- 证据：commit:d9f7f5d32f2。

**T4 F4 对 F7**：在很短的临界区上，速度优先于机械的安全保障。
- 让步方的补偿：约定代替检查。LWLock 没有死锁检测、没有超时、等待中不响应取消，所以规定"等待可能超过几秒的地方不要用它"。
- 支撑：[[D-lock-tiers]]。
- 反例：未见。
- 证据：file:src/backend/storage/lmgr/README:20-45。

**T5 F1（故障隔离）对 F4**：隔离优先于连接开销和共享的便利。
- 让步方的补偿：连接数多时用外部连接池；并行查询靠复制选定的状态而不是共享地址空间。
- 支撑：[[D-process-per-connection]]。
- 反例：进行中的多线程化工作正在重审这条规则（[[E3]] 正在变化）。
- 证据：file:doc/src/sgml/runtime.sgml:1391-1395。

**T6 F3 对 F5**：可扩展性靠暴露内部接口取得，代价是大版本之间不承诺 API；小版本之内 ABI 不可破。
- 让步方的补偿：少量"划定清楚"的接口长期稳定；其余接口的变更"会考虑扩展的需要并谨慎处理"。
- 支撑：[[D-catalog-driven-extensibility]] [[D-compat-by-release-class]]。
- 反例：未见。
- 证据：file:doc/src/sgml/xfunc.sgml:2808-2812,2836-2857。

**T7 F5 对 F7**：在已发布的分支上，ABI 稳定优先于更干净的修复；在开发分支上反过来。修复确实需要时 ABI 可以让步，见反例。
- 让步方的补偿：需要破坏 ABI 的改动只进开发分支，并在提交信息里写明；回补时新成员加在结构体末尾。
- 支撑：[[D-compat-by-release-class]]。
- 反例（F1 压过 F5 的情形）：2026 年 11 月两条安全修复回补到了全部受支持分支，并且触及 ABI。一条把一个宏改成了函数：
  没有重新编译的扩展得不到完整保护，按新小版本编译的扩展装进旧小版本会链接失败。另一条改了一个结构体里指针的含义，
  提交自己写"严格说这是破坏 ABI"。这两条都没有改结构体的布局和大小。
  2025 年 6 月还有一条不是安全修复的例子：修逻辑解码里失效消息分发的内存问题，回补到全部受支持分支，给一个导出的结构体加了两个成员，
  提交写明"这在旧分支上破坏 ABI，影响依赖该结构体大小的第三方扩展，不过这种情形看来不太可能"。
  三条的共同点是：修复绕不开，受影响的扩展被判断为很少，影响在提交信息里逐条写明。
  所以实际的规则是默认不破坏、破坏要有理由并写明，不是绝对禁止；通常能保证的只是按较早小版本编译的扩展在较新的小版本里继续工作。
- 证据：commit:1c27d16e6e5; commit:661643dedad; commit:245eb832f88; commit:3beee6e77c8; commit:45c357e0e85。

**T8 F6 对 F4**：操作系统特有的能力只能作为可选实现加入，默认路径必须在所有受支持平台上可用。
- 让步方的补偿：把平台相关的加速做成可选择、可缺席的实现。
- 支撑：[[D-full-page-writes]] [[D-jit-provider]]（核心不依赖 LLVM，JIT 放在可缺席的 provider 里）。
- 反例：未见。
- 证据：url:https://wiki.postgresql.org/wiki/Developer_FAQ（转述，"We resist that temptation"）; commit:432bb9e04da。

---

## 4. 基础性决策

入选标准（满足任一）：(a) 推出至少一个不变量族；(b) 影响跨两个以上子系统；(c) 推翻它要重推地图上大量节点。
每条另记独立性检查的结论：它不能由其余决策加上力推出。

### D-mvcc-no-overwrite　UPDATE/DELETE 不立即移除行的旧版本，旧版本由 VACUUM 回收
- 力：[[F2]]　前提：[[E10]]
- 原始原因：Berkeley POSTGRES 的存储管理器把主存储和历史日志统一成一种表示，每条记录是一串带事务号的版本；这使恢复极其简单，时间旅行查询是顺带得到的。
  Stonebraker 的自述是"出于一种要做点不同东西的热情"：当时商业系统都用 WAL，他们认为那项技术已被充分理解。**多版本并发控制从来不是 Berkeley POSTGRES 的目标**（二手来源，逐字）。
- 当前原因：读写互不阻塞，在可串行化级别下也成立。
- 路径依赖：时间旅行于 1997 年移除，提交信息只有标题；MVCC 于 1998 年加入，提交正文为空。
  社区当年拒绝了用带事务支持的 Berkeley DB 替换不覆盖写，据回顾文章的说法，原因是有位开发者想做 MVCC 并愿意承担工作。
- 被否方案：原地更新加 undo 日志（zheap，2018）。它要解决的是"更新总是产生新版本，许多情况下空间不能完全回收"；提案自己列的代价是删除、中止和改多数索引列的更新会变贵。没有读到否决结论，只是没有进入核心。
- 推出：[[I2]]；[[S06.M04]] [[S06.M05]] [[S06.M06]] [[S06.M07]] [[S06.M08]] [[S06.M11]] [[S06.M12]] [[S07.M02]] [[S07.M06]] [[S07.M09]] [[S05.M08]] [[S10.M05]]。
  派生决策（不单列）：[[D-abort-by-clog]] [[D-hot]] [[D-ssi]] [[D-evalplanqual]]。
- 下游代价：旧版本要靠 vacuum 回收，空间可能膨胀 → HOT、visibility map、autovacuum；可见性判定要查事务状态 → hint bit；标准冲突使 standby 上的查询被取消。
- 重新审视的条件：[[E10]] 不再成立，即保留和回收旧版本的代价超过读写互相阻塞的代价。
- 扇出：(a)(b)(c)，16 张模块卡、5 个子系统引用它。独立性：不可由其他决策推出；它是 F2 的根。
- 证据：url:https://arxiv.org/abs/1901.01973（§2.3，逐字；其中引用 Stonebraker & Kemnitz, CACM 34(10), 1991）; commit:32cd09ac6d4; commit:3f7fbf85dc5; file:doc/src/sgml/mvcc.sgml:60-66; file:doc/src/sgml/maintenance.sgml:156-165

### D-snapshot-xmin-xmax-xip　快照表示为 (xmin, xmax, 进行中事务号的列表)
- 力：[[F2]] [[F4]]　前提：[[E11]] [[E5]]
- 原始原因：未考证（1998 年引入快照的提交正文为空）。
- 当前原因：快照看不到任何不小于 xmax 的事务，能看到更老的事务中不在列表里的那些；xmin 只是为了让多数元组不必搜索列表。
- 被否方案：按提交序号（CSN）取快照，2013 年提出。取快照只需读一个数，不必持锁复制列表；提案自己指出的难点是在有限的共享内存里为任意长的事务找到它的提交序号。没有读到否决结论。
- 推出：[[I2]]；[[S07.M05]] [[S07.M06]] [[S07.M01]] [[S07.M08]] [[S06.M05]] [[S10.M10]]。
- 下游代价：取快照要在共享锁下扫描运行中事务的集合 → 2020 年改为稠密数组并把视界计算挪出取快照路径；standby 上要另行维护已知进行中的事务集合。
- 重新审视的条件：[[E11]] 不再成立，即连接数和核数使复制列表与持锁成为瓶颈。
- 扇出：(a)(b)，8 张模块卡、4 个子系统。独立性：不可推出，存在可行的替代表示。
- 证据：commit:be8300b18f2; file:src/include/utils/snapshot.h:148-160; file:src/backend/access/transam/README:246-263; commit:dc7420c2c92; url:https://www.postgresql.org/message-id/CA+CSw_tEpJ=md1zgxPkjH6CWDnTDft4gBi=+P9SnoC+Wy3pKdA@mail.gmail.com（转述）

### D-row-lock-in-tuple-header　行锁记在元组头里；多个事务锁同一行时用 MultiXactId
- 力：[[F2]]　前提：[[E6]]
- 原始原因：排他行锁记在行头，是随 MVCC 一起出现的（逐字）：1998 年 12 月 MVCC 的初始代码（commit:3f7fbf85dc5）加入 heap_mark4update，把加锁事务的 XID 记进 t_xmax，并设 HEAP_MARKED_FOR_UPDATE；
  1999 年 1 月先后加入 "SELECT FOR UPDATE syntax"（commit:dfa23f5e41b）和 "SELECT FOR UPDATE is implemented"（commit:247b3f90548）。
  2005 年引入共享行锁以消除外键检查造成的不必要死锁；多个事务持有同一行的共享锁时，建一个代表这组事务的 MultiXactId 存进行的 xmax。"行锁数量实际上不受限，除非真的要共享否则没有额外开销"。
  同年写下不把行锁交给锁管理器的理由（逐字，commit:93b24772782 加的 heapam.c 注释，2013 年随 README.tuplock 迁走）："because the shared-memory lock table is of finite size, but users could reasonably want to lock large numbers of tuples, we do not rely on the standard lock manager to store tuple-level locks over the long term."
- 当前原因：事务可能同时锁住大量元组，锁对象放不进共享内存。
- 被否方案：(1) 把每个行锁长期放进共享锁表——因锁表大小有限而不用（上面那句注释，逐字）。
  (2) 2005 年 4 月 Tom Lane 提议只在等待时对该行取一个锁管理器的锁，用来排队、防止排他加锁者饿死（转述，url:https://www.postgresql.org/message-id/2296.1114638707%40sss.pgh.pa.us）；commit:93b24772782 实现了它，所以它不是被否，而是作为行头方案的补充被采纳。
  (3) 2004 年待办列表里"把共享锁持有者记在另一张表、行上设一位"的想法（逐字，commit:4359ebed37b），没有采用，也没有记录谁否决。
- 推出：[[I2]] [[I3]]；[[S06.M04]] [[S07.M04]]。
- 下游代价：2013 年起 multixact 既表示行锁也表示更新，必须跨重启和崩溃持久化，并带来额外的 WAL；判断可见性时要多查一次 multixact → 提交信息说这部分额外 I/O 可以接受，因为它发生在原本会让命令阻塞等待的场合。multixact 后来还需要专门的回卷保护和截断机制。
- 重新审视的条件：[[E6]] 不再成立。
- 扇出：(a)(b)。独立性：不可由 [[D-mvcc-no-overwrite]] 推出，原地更新的设计同样可以把锁记在行头。
- 证据：commit:3f7fbf85dc5; commit:dfa23f5e41b; commit:247b3f90548; commit:bedb78d386a; commit:93b24772782; commit:4359ebed37b; commit:0ac5ad5134f; commit:b69bf30b9bf; commit:4f627f89736; file:src/backend/access/heap/README.tuplock:5-13; url:https://www.postgresql.org/message-id/2296.1114638707%40sss.pgh.pa.us（转述）

### D-lock-tiers　锁分三层：spinlock、LWLock、重锁，各有不同的保障
- 力：[[F4]]　前提：[[E5]]
- 原始原因：提交原文：用新的轻量锁取代大多数 spinlock 的用法，剩下的 spinlock 持有时间都很短（几十条指令）。提案原文（转述）：起因是在 8 路 SMP 机器上表现差，怀疑是 spinlock 实现低效；spinlock 以后主要只保护轻量锁自身的结构，并假定它不跨内核调用。
  （逐字，commit:499abb0c0f2，2001-09-29）提交信息 "All per my proposal on pghackers 26-Sep-01."；同一提交的 storage/lmgr/README："There is no provision for deadlock detection, but the LWLock manager will automatically release held LWLocks during elog() recovery, so it is safe to raise an error while holding LWLocks."
  提案（转述，2001-09-26）的目标是无竞争时极快地直接通过；spinlock 持有时间不超过几十条指令，并且从不跨内核调用；用与原来相同的系统设施（TAS 与 SysV 信号量），因此不带来可移植性风险。提案没有讨论给 LWLock 做死锁检测。
  为什么 LWLock 不做死锁检测：未考证，提案和讨论里都没有找到理由；只有上面 README 的事实陈述和"出错时自动释放"的补偿。
- 当前原因：spinlock 用于极短的临界区；LWLock 保护共享内存里的数据结构，没有死锁检测，但出错时自动释放；重锁支持多种模式，有死锁检测，事务结束时自动释放，面向用户可见的加锁。
- 被否方案（转述，同一讨论串，url:https://postgrespro.com/list/thread-id/1607551）：(1) 改用线程模型：Tom Lane 认为会显著削弱从 core dump 一类故障中恢复的能力，也不相信能带来显著的性能收益（url:https://www.postgresql.org/message-id/14704.1001551611%40sss.pgh.pa.us，2001-09-27）。
  (2) 用 POSIX 互斥量代替 TAS 加 SysV 信号量：Tom Lane 回答"长期值得研究"（2001-09-29），没有采用。
  (3) 用重锁管理器保护共享结构：不可行，因为重锁管理器自身的共享状态就要靠 LWLock 保护（逐字，同一提交的 lwlock.c 文件头）："User-level locking should be done with the full lock manager --- which depends on an LWLock to protect its shared state."
- 推出：[[I3]]；[[S01.M05]] [[S07.M07]] [[S08.M01]]。
- 下游代价：LWLock 没有死锁检测、没有超时、等待中不响应取消 → 加锁顺序只能靠约定，没有机械检查（[[X6]] [[X41]]）。
- 重新审视的条件：出现需要长时间持有、又不值得用重锁的共享状态。
- 扇出：(a)(b)。独立性：不可推出。
- 证据：commit:499abb0c0f2; url:https://www.postgresql.org/message-id/1054.1001520600%40sss.pgh.pa.us（转述）; url:https://postgrespro.com/list/thread-id/1607551（转述）; url:https://www.postgresql.org/message-id/14704.1001551611%40sss.pgh.pa.us（转述）; file:src/backend/storage/lmgr/README:6-45

### D-wal-redo-only　用预写日志保证持久性，只做 redo，不做 undo
- 力：[[F1]] [[F4]]　前提：[[E2]]
- 原始原因：WAL 代码 1999 年出现，提交信息没有理由。2001 年的文档写："在 WAL 的第一个发布版里没有实现 UNDO，因为时间不够。"
  预写规则本身的最早落点（逐字）：2000 年 10 月的 WAL 提交在 buffer 写出路径加入 "Force XLOG flush for buffer' LSN"（commit:5b0740d3fcd）；
  2001 年的首版 WAL 文档写明它的核心概念："WAL's central concept is that changes to data files ... must be written only after those changes have been logged - that is, when log records have been flushed to permanent storage."（commit:7b9dc714059）。
- 路径依赖：2001 年 6 月不再保留 undo 信息，理由是"在有能力用它做点什么之前，留着它没有意义"，这同时解决了长事务的日志空间问题；同年 8 月从待办里删掉 UNDO 的讨论，说它"没有结论、而且太大"；2002 年文档删去"因为时间不够"的说法（提交标题是"不要把 UNDO 写成将来一定会加"）；2005 年移除残留的桩代码，理由是"在可见的将来不会用到"，并且妨碍 redo 例程的可读性。只做 redo 不是一开始定下的设计，是逐步收敛的结果。
- 当前原因：提交时只需把 WAL 刷盘，不必刷每个被改过的数据文件，磁盘写入显著减少；崩溃后前滚恢复。崩溃时没有提交记录的事务一律视为已中止，所以不需要撤销，中止记录也不必刷盘。
- 被否方案：之前的做法（逐字，7.1 的发布说明，commit:a238cb5a8aa）："To maintain database consistency in case of an operating system crash, previous releases of PostgreSQL have forced all data modifications to disk before each transaction commit. With WAL, only one log file must be flushed to disk, greatly improving performance."
  没有找到当时讨论过的其他方案。undo 日志见 [[D-mvcc-no-overwrite]] 的被否方案。
- 推出：[[I1]]；[[S09.M01]] [[S09.M02]] [[S09.M05]] [[S08.M13]] [[S06.M04]] [[S07.M01]]。
  预写规则（数据页写出之前，描述它的 WAL 必须已经落盘）就是这条选择的定义，不另立决策；它在 buffer 写出时强制，
  是因为 [[D-own-buffer-cache]]（`holdout`）让页由数据库自己的缓存择时写出（file:src/backend/access/transam/README:407-409,413-415; file:src/backend/storage/buffer/bufmgr.c:4574-4576）。
  派生：临界区内出错升级为 PANIC。原始（逐字）：2000 年 12 月先禁止临界区里的信号处理器抛 ERROR/FATAL（commit:65b362fae15），当时 heapam.c 的注释已有 "NO ELOG(ERROR) from here till changes are logged"；
  2001 年 1 月让临界区内的错误升级为 STOP，即 "a system-wide reset is forced"（commit:6ce0ed2813d; file:src/include/miscadmin.h:79-85）；2002 年 STOP 改名为 PANIC（commit:a033daf5663）。
  讨论（转述）：Vadim 说他尝试把可能出错的操作（例如分配新文件块）移出临界区，但无论如何，在修改尚未写日志时出错，为安全必须 ERROR→STOP；Tom Lane 提出区分两种临界区（只挂起中断的、和出错即危险的），结果是把"挂起中断"和"临界区"分成两个机制（url:https://www.postgresql.org/message-id/14110.979679509%40sss.pgh.pa.us）。讨论里没有提到 undo。
  当时确实没有 undo（逐字，2001 年首版 WAL 文档，commit:7b9dc714059）："In this first release of WAL, UNDO operation is not implemented, because of lack of time."
  当前理由（逐字）："Any error during the next three steps must cause a PANIC because the shared buffers will contain unlogged changes, which we have to ensure don't get to disk."（file:src/backend/access/transam/README:442-444）；
  文件系统操作不用这个办法，因为写出 WAL 记录之后再 PANIC，重放会再次失败（file:src/backend/access/transam/README:671-690）。
  仍是推断：它与"没有 undo"的联系，源码和文档都没有写。两件事同时成立——临界区升级为强制重置的时候（2001-01）系统没有 undo，升级的理由是"修改还没写日志"——但没有任何来源把两者明确连起来。推断的链条是：没有 undo，共享缓冲区里改了一半、未写日志的页无法在事务中止时撤回，所以只能丢掉全部共享内存、从 WAL 重放。
- 下游代价：中止事务留下的修改仍占磁盘空间，并且需要永久的事务状态日志 → 由 vacuum 和 [[D-mvcc-no-overwrite]] 的可见性判定吸收。
- 重新审视的条件：存储引擎改为原地更新（那时回滚需要 undo）。
- 扇出：(a)(b)(c)。独立性：不可推出。
- 证据：commit:30659d43eb7; commit:5b0740d3fcd; commit:7b9dc714059; commit:a238cb5a8aa; commit:65b362fae15; commit:6ce0ed2813d; commit:a033daf5663; commit:1173344e74f; commit:4d0c90b82f0; commit:d66f172f4b7; commit:4c8495a1f2a; file:doc/src/sgml/wal.sgml:519-522; file:src/backend/access/transam/README:407-415,442-444,671-690,790-792; file:src/backend/storage/buffer/bufmgr.c:4574-4576; file:src/include/miscadmin.h:79-85; url:https://www.postgresql.org/message-id/14110.979679509%40sss.pgh.pa.us（转述）

### D-full-page-writes　checkpoint 之后对一个页的首次修改，把整页写进 WAL
- 力：[[F1]] [[F6]]　前提：[[E1]]
- 原始原因：引入提交没有理由（2000 年引入备份块的提交和 2005 年引入开关的提交都只有标题）。2001 年的文档写的是："为保证数据页的一致性，每次 checkpoint 之后对数据页的首次修改会把整页内容记入日志。"它没有提到撕裂页；这个具体理由见于现行文档。
- 当前原因：操作系统崩溃时正在进行的页写可能只完成一部分，磁盘上的页混着新旧数据；有了整页镜像，重放时直接恢复整页。
- 被否方案：doublewrite 缓冲，把防撕裂页的职责移出 WAL 路径。2026 年有提案，理由是整页镜像与 checkpoint 频率相互冲突；读到的反对意见是 checkpoint 间隔设对了整页镜像就不是问题，而且 doublewrite 牵涉恢复和复制、不是小工程。没有结论。
- 推出：[[I1]]；[[S09.M01]] [[S09.M02]] [[S09.M03]] [[S09.M09]] [[S08.M10]]。
- 下游代价：WAL 量增加，并且随 checkpoint 变频繁而增加 → 把 checkpoint 的写入摊开；在线备份期间必须强制打开。
- 重新审视的条件：[[E1]] 不再成立，即存储栈保证整页写的原子性。
- 扇出：(a)(b)。独立性：不可由 [[D-wal-redo-only]] 推出，防撕裂页有别的做法。
- 证据：commit:7ceeeb662fb; commit:326a7a0788b; file:doc/src/sgml/config.sgml:3594-3612; file:doc/src/sgml/wal.sgml:164-180; file:src/backend/access/transam/README:424-435; url:http://www.mail-archive.com/pgsql-hackers@lists.postgresql.org/msg219771.html（转述）

### D-wal-archive-pitr　WAL 可以归档并用于时间点恢复；每次归档恢复结束开一条新的 timeline
- 力：[[F1]]　前提：[[E2]]
- 原始原因：2004 年加入归档与时间点恢复，代码提交没有陈述动机，只说还有未完成的部分、文档严重缺失，但"基本可用"。timeline 的提交写的是"让时间点恢复更易管理"。
  同年随后的首版 PITR 文档写了理由（逐字，commit:59ff4848942）：日志的存在使第三种备份策略成为可能，"we can combine a filesystem-level backup with backup of the WAL files."；
  好处是不需要完全一致的备份作起点，"We do not need a perfectly consistent backup as the starting point ... So we don't need filesystem snapshot capability, just tar or a similar archiving tool."；
  并且 "continuous backup can be had simply by continuing to archive the WAL files. This is particularly valuable for large databases, where making a full backup may take an unreasonable amount of time."
  时间线的目的（逐字，同一提交）："Each time you recover to a point-in-time earlier than the end of the WAL sequence, a new timeline is created to identify the series of WAL records generated after that recovery."
- 当前原因：恢复到过去某一点之后，新产生的 WAL 不能覆盖旧历史的 WAL；timeline 编号进入段文件名，于是可以恢复到任何先前的状态，包括后来放弃的分支。
- 被否方案（转述，2004 年 7 月的设计讨论，url:https://www.postgresql.org/message-id/1090274296.28049.317.camel%40stromboli）：随机的时间线编号、嵌套的编号、恢复时继续读到日志末尾但不应用；
  采纳的规则是"恢复时永不覆盖已有的日志数据"（Tom Lane，参照 DB2 的做法）。
- 推出：[[I1]]；[[S09.M07]] [[S09.M08]] [[S09.M09]]。这条决策补上了这两张原先的孤儿卡（[[X89]]）。
- 下游代价：未归档的 WAL 段不能回收；timeline 的选择、切换检查和文件读写分散在三处（[[X56]]）。
- 重新审视的条件：未考证。
- 扇出：(a)(b)。独立性：不可由 [[D-wal-redo-only]] 推出，崩溃恢复本身不需要归档和 timeline。
- 证据：commit:66ec2db7284; commit:2042b3428d3; commit:59ff4848942; file:doc/src/sgml/backup.sgml:1397-1438; url:https://www.postgresql.org/message-id/1090274296.28049.317.camel%40stromboli（转述）

### D-physical-rep-ships-wal　内建复制是把 WAL 传给备库持续重放
- 力：[[F1]] [[F7]]　前提：[[E9]]
- 原始原因：2008 年核心团队声明：不在核心里做复制的政策"妨碍了 PostgreSQL 的采用"，现有方案"对简单场景太复杂"；合适的基础技术"大概是实时的 WAL 传送"。（社区明言；转述，片段）
- 当前原因：备库就是持续的恢复；walreceiver 只替代"取得 WAL"这一步，重放仍由 startup 完成。假设：选它是为了复用已有的 redo 路径。
- 被否方案：声明明确不打算替代 Slony、pgPool、Londiste 等，也不追求高可伸缩、长距离复制或只复制一部分。
- 推出：[[S10.M01]] [[S10.M02]] [[S10.M04]] [[S09.M05]] [[S07.M05]] [[S07.M09]]。派生：[[D-replication-slot]]。
- 下游代价：备库上的只读查询会和重放冲突，超时后被取消；备库要模拟主库的事务状态和锁；只能整库复制。
  备库上不支持 SERIALIZABLE（逐字，commit:dafaa3efb75）："We don't currently allow Serializable transactions in Hot Standby mode. That would be hard, because even read-only transactions can cause anomalies that wouldn't otherwise occur."；
  README-SSI 的说法是，在 PITR 恢复或热备下，数据库可能到达一个可读状态，其中有些事务出现在本应先于它们的事务之前（file:src/backend/storage/lmgr/README-SSI:601-604）。
  主备必须是同一大版本（逐字）："In general, log shipping between servers running different major PostgreSQL release levels is not possible."（file:doc/src/sgml/high-availability.sgml:600-602）；
  机制是 WAL 页头的版本号不符即拒绝（file:src/include/access/xlog_internal.h:35）。文档给的原因只是政策表述（小版本不改磁盘格式），"WAL 格式随大版本变"是由此推出的，见 [[D-compat-by-release-class]]。
- 重新审视的条件：[[E9]] 变化，例如"简单场景"的主流需求变成部分复制或跨版本复制。
- 扇出：(b)(c)，3 个子系统。独立性：不可由 [[D-wal-redo-only]] 推出，有了 WAL 也可以选别的复制方式。
- 证据：url:https://groups.google.com/g/pgsql.hackers/c/5ZuMgmec2xk（转述，片段）; commit:efc16ea5206; commit:40f908bdcdc; commit:dafaa3efb75; file:src/backend/replication/walreceiver.c:9-16; file:src/backend/storage/lmgr/README-SSI:601-604; file:doc/src/sgml/high-availability.sgml:600-602; file:src/include/access/xlog_internal.h:35

### D-process-per-connection　每个连接一个进程；postmaster 收到连接立即 fork，认证在子进程里做
- 力：[[F1]] [[F7]]　前提：[[E3]] [[E4]]
- 原始原因：未考证。这条来自 Berkeley POSTGRES，早于仓库历史；说明理由的注释是 2001 年写入的，那次提交的信息里没有理由。
- 当前原因：认证代码可以写成简单的单线程风格；更重要的是，SSL、PAM 这类库的阻塞不会对其他客户端造成拒绝服务。不用线程的理由（Developer FAQ）：历史上线程支持差；一个后端出错会破坏同进程里的其他后端；相对于后端启动时间，线程带来的速度提升很小；代码会更复杂，更难调试。
- 被否方案：多线程。2023 年提出重新考虑；提出者自己列的障碍是大量全局和静态变量、无法在一个版本里完成过渡、一个后端崩溃后难以安全地重置其余部分。读到的反对意见是"太多代码会悄悄坏掉，其中很多不在我们控制之内"。
  2001 年的锁讨论中也提出过改用线程，Tom Lane 认为会显著削弱从 core dump 一类故障中恢复的能力，也不相信能带来显著的性能收益（转述，见 [[D-lock-tiers]] 的被否方案）。
- 推出：[[I7]]；[[S01.M01]] [[S01.M05]] [[S01.M06]] [[S02.M01]] [[S08.M05]]。派生：[[D-parallel-copy-state]]（没有共享地址空间，并行只能复制选定的状态）。
- 下游代价：每个连接的启动和内存开销 → 外部连接池；后端之间不能共享指针 → 共享内存里只能放定长结构或偏移量；并行查询受限。
- 重新审视的条件：[[E3]] 不再成立。它正在变化。
- 扇出：(a)(b)(c)。独立性：不可推出。
- 证据：file:src/backend/postmaster/postmaster.c:25-33; commit:f3868f8af9c; url:https://wiki.postgresql.org/wiki/Developer_FAQ（转述）; url:https://www.postgresql.org/message-id/31cc6df9-53fe-3cd9-af5b-ac0d801163f4@iki.fi（转述）; url:https://www.postgresql.org/message-id/4178104.1685978307%40sss.pgh.pa.us（转述）; url:https://www.postgresql.org/message-id/14704.1001551611%40sss.pgh.pa.us（转述）

### D-error-longjmp-abort　出错即 longjmp 跳出，清理集中在（子）事务中止
- 力：[[F7]] [[F1]]　前提：[[E12]]
- 原始原因：未考证（这个模型早于 2004 年）。2004 年引入 PG_TRY 的提交说：现在真的可以在函数内部捕获错误，而不是任它一路传到主循环；但仍然必须用事务中止来清理。
- 当前原因：普通代码不需要自己带可靠的出错清理；清理由事务中止统一完成。
- 推出：[[I5]]；[[S12.M03]] [[S12.M04]] [[S12.M01]] [[S01.M07]] [[S02.M03]] [[S02.M05]] [[S05.M01]]。
  派生决策：[[D-memory-context]] [[D-resource-owner]]（既然出错不逐层返回，内存和非内存资源都必须能按生命周期整体回收）。
- 下游代价：每次 ERROR 都要做昂贵的事务或子事务中止，因为捕获者不敢对出了什么错做任何假定 → 2022 年为"已知可以安全恢复"的错误另开软错误通道；C++ 的调用栈不会被展开；PG_TRY 的使用规则只是约定（[[X82]]）。
- 重新审视的条件：出错恢复的频率高到事务中止的代价不可接受的场合（软错误通道正是这样产生的）。
- 扇出：(a)(b)(c)，9 张模块卡、4 个子系统。独立性：不可推出。
- 证据：commit:a393fbf9376; commit:d9f7f5d32f2; url:https://www.postgresql.org/message-id/3bbbb0df-7382-bf87-9737-340ba096e034@postgrespro.ru（转述）; file:doc/src/sgml/xfunc.sgml:4129-4132; file:src/backend/utils/resowner/README:10

### D-catalog-driven-extensibility　类型、函数、运算符、访问方法都是 catalog 里的行
- 力：[[F3]]　前提：[[E8]]
- 原始原因：来自 Berkeley POSTGRES，早于仓库历史。回顾文章的说法是：POSTGRES 的标志性目标是支持新的对象关系特性；它"在根本上是为可扩展性而设计的，而这个设计是健全的"（二手来源，逐字）。
- 当前原因："PostgreSQL 之所以可扩展，是因为它的运行由 catalog 驱动。"传统系统只能靠修改源码里的硬编码过程来扩展。这句话自 1998 年的文档起就有。
- 推出：[[I8]] [[I4]]；[[S11.M01]] [[S11.M06]] [[S12.M05]] [[S03.M02]] [[S06.M13]] [[S06.M14]] [[S06.M17]]。
- 下游代价：几乎每一步都要查 catalog → 每个后端各自缓存，于是需要 [[D-sinval-with-locks]]；扩展直接用内部接口 → 大版本之间不承诺 API（T6）。
- 重新审视的条件：未考证。
- 扇出：(a)(b)(c)，4 个子系统。独立性：不可推出；它是 F3 的根。
- 证据：commit:c8cfb0cea88; file:doc/src/sgml/extend.sgml:53-54,68-69; url:https://arxiv.org/abs/1901.01973（§4，逐字）

### D-catalog-mvcc-heap　catalog 是普通的 heap 表，用 MVCC 快照扫描
- 力：[[F1]] [[F2]]　前提：[[E11]]
- 原始原因：系统表为什么是普通表的原始原因，未考证（Berkeley 1986 年的论文读不到）。下面说的是 2013 年改用 MVCC 快照：用 SnapshotNow 扫描时，遇到并发更新可能既看不到行的旧版本也看不到新版本。此前迟迟不改，是因为取 MVCC 快照比用静态的特殊快照贵得多。
- 当前原因：同上；并为以后降低 DDL 的锁级别铺路。"catalog 是普通表，所以 DDL 是事务性的"这层因果在提交和读到的讨论里都没有原文（假设）。
  现行文档的陈述（逐字）："PostgreSQL's system catalogs are regular tables."（file:doc/src/sgml/catalogs.sgml:13-14，这句 2000 年起就有，commit:1efd7330cbd）；
  "Physically there is no difference between a system catalog and a plain user table, but the backend C code knows the structure and properties of each catalog, and can manipulate it directly at a low level."（file:doc/src/sgml/bki.sgml:8-11）。
- 被否方案：给每次扫描各取一个快照。提交原文说这样开销大，所以改为只在处理过失效消息之后才重取；讨论串里（转述）有人质疑逐扫描取快照会大幅增加取快照的争用。
- 推出：[[I2]] [[I4]]；[[S11.M01]] [[S11.M06]] [[S11.M09]] [[S11.M12]] [[S10.M10]] [[S12.M16]]。
- 下游代价：catalog 扫描多了快照的开销 → 提交信息说只在相当极端的负载下才明显。
  系统表是普通表，就要用系统表描述系统表自己（pg_class 必须含有它自己的条目）→ 自举：最初的系统表由编译时生成的数据在单用户自举模式下装入，
  少数"自举系统表"连建表命令都不能用，它们的条目要手工预置（file:doc/src/sgml/bki.sgml:27-35,144-154；1996 年的 src/backend/catalog/README 已描述这一流程）。
- 重新审视的条件：[[E11]] 变化（取快照变贵或变便宜）。
- 扇出：(a)(b)，3 个子系统。独立性：不可由 [[D-catalog-driven-extensibility]] 推出，catalog 也可以用专门的存储。
- 证据：commit:568d4138c64; commit:813fb031558; commit:1efd7330cbd; file:doc/src/sgml/catalogs.sgml:13-14; file:doc/src/sgml/bki.sgml:8-11,27-35,144-154; url:https://postgrespro.com/list/thread-id/1821201（转述）

### D-sinval-with-locks　缓存一致性靠共享的失效消息队列，并与重锁捆绑
- 力：[[F1]] [[F4]]　前提：[[E13]]
- 原始原因：未考证。2001 年的重写让失效消息带上数据库号，并整理了收发顺序；2008 年的重写是因为失效锁成了争用点，所有后端同时去读队列。"取锁时接收失效"这条约定的原始陈述没有找到。
- 当前原因：失效消息必须在释放相关的重锁之前发出，否则其他后端可能一直用本地缓存而根本不去扫 catalog。队列溢出时只重置落后太多的后端，让它丢弃全部缓存。
- 被否方案：未考证（没有找到与版本号一类方案比较的记录）。假设：命中本地缓存时完全不碰共享状态，是选这种做法的原因。
- 推出：[[I4]]；[[S11.M09]] [[S11.M06]] [[S11.M07]] [[S11.M08]] [[S11.M04]] [[S11.M10]] [[S11.M12]] [[S02.M06]] [[S10.M15]]。派生：[[D-plancache-lazy-inval]]。
- 下游代价：落后的后端被整体重置；已经持有所需锁时不会再接收失效（[[X72]]）；多数派生缓存在构建期间可能过期（[[X73]]）。
- 重新审视的条件：[[E13]] 不再成立。
- 扇出：(a)(b)，9 张模块卡、3 个子系统。独立性：不可推出；每后端缓存的必要性来自 [[D-catalog-driven-extensibility]] 和 [[D-process-per-connection]]，但"共享队列并与锁捆绑"是独立的选择。
- 证据：commit:bbbc00af882; commit:fad153ec452; commit:568d4138c64; file:src/backend/storage/ipc/sinvaladt.c:44-52; url:https://www.postgresql.org/message-id/373.1213813174%40sss.pgh.pa.us（转述）

### D-path-plan-split　优化器先枚举 Path 并按代价淘汰，选定后才生成 Plan；连接顺序自底向上动态规划
- 力：[[F4]]　前提：[[E7]]
- 原始原因：未考证。1997 年加入的 README 只描述了自底向上的过程，没有解释原因。
- 当前原因：Path 和 Plan 几乎一一对应，但 Path 省掉规划期间用不到的信息，带上执行器不需要的规划信息；每层关系的路径都建完之后才被上层引用，所以可以放心丢弃较差的路径。
- 被否方案：优化器提示。社区的立场是把计划问题当作优化器的缺陷来报告和修复（见 [[R9]]）。
- 推出：[[I6]]；[[S04]] 的全部模块；[[S05]] 消费它产出的计划。
- 下游代价：近似穷举的代价随连接数指数增长 → 连接数超过阈值时改用遗传算法搜索，放弃最优性和确定性；结果的质量完全取决于代价估计。
- 重新审视的条件：[[E7]] 的压力变大（连接数很多的查询成为主流）。
- 扇出：(a)(c)。独立性：不可推出。
- 证据：commit:d158fce8eb9; file:src/backend/optimizer/README:17-27,69-70,96-97,124-126; file:doc/src/sgml/geqo.sgml:21-40,203-205

### D-exec-pull-iterator　执行器是按需拉取的迭代器；Plan 树只读，运行状态放在平行的状态树里
- 力：[[F4]] [[F7]]　前提：逐元组的函数调用开销相对于 I/O 可以忽略（假设，没有找到原文）
- 原始原因：迭代器模型未考证。最早的成文（逐字）：2001 年新增的 executor/README（commit:7809cbf7e65）"The plan tree is essentially a demand-pull pipeline of tuple processing operations. Each node, when called, will produce the next tuple in its output sequence, or NULL if no more tuples are available."；
  1996 年导入时的 execProcnode.c 已有这个形态（逐字："ExecProcNode - get a tuple by executing the plan node"，commit:d31084e9d11）。回顾文章里没有关于执行器来源的内容。
  "Plan 树只读"是 2002 年的专项工作，提交信息只陈述了目标，没有陈述动机。
- 当前原因：计划树是一条按需拉取的元组处理管道；执行期间被修改的数据都在状态树里，只读的计划树让计划的缓存和复用简单得多。
- 被否方案（讨论过的方向，转述；没有读到否决结论）：(1) 批量或向量化执行。2016 年"异步与向量化执行"的讨论（url:https://www.postgresql.org/message-id/CA%2BTgmoZK3eM5VWmxTw6586THo5W%2B2hTLRnUSuUX65hBfmBhvjA%40mail.gmail.com）里，David Rowley 提出批量取元组、多元组槽、向量函数；
  Robert Haas 主张另设一种节点类型，让逐元组的代码照常工作、不付额外开销，并担心"几百个向量函数，每个都可能有 bug 或行为差异"，倾向于直接降低函数调用开销。
  (2) CPU 成为瓶颈后社区优化的是节点内部，没有改拉取模型本身：2017 年表达式求值改写（逐字，commit:b8d7f053c5c）"This both leads to significant performance improvements, and makes future just-in-time compilation of expressions easier."；2018 年 JIT（commit:2a0faed9d70）只编译表达式，不改变节点之间的元组流。
  没有找到"为什么保留逐元组拉取"的设计结论；能看到的顾虑是维护第二套执行器的成本。
- 推出：[[S05.M01]] 以及 [[S05]] 的全部节点模块；[[S02.M06]] 的计划缓存依赖计划树只读。
- 下游代价：反向扫描支持得不好；运行时裁剪掉的子计划使状态树和计划树的子节点对不齐。
- 重新审视的条件：CPU 而不是 I/O 成为瓶颈的负载（表达式求值已经为此改成线性步骤并支持 JIT）。
- 扇出：(c)。独立性：不可推出。
- 证据：commit:7809cbf7e65; commit:d31084e9d11; commit:1fd0c59e250; commit:3a4f7dde16a; commit:b8d7f053c5c; commit:2a0faed9d70; file:src/backend/executor/README:5-16,50-66; url:https://www.postgresql.org/message-id/CA%2BTgmoZK3eM5VWmxTw6586THo5W%2B2hTLRnUSuUX65hBfmBhvjA%40mail.gmail.com（转述）

### D-trust-declared-properties　规划器和执行器相信 catalog 里声明的函数与运算符属性，不做验证
- 力：[[F3]] [[F4]]　前提：[[E16]]
- 决策的内容：规划器和执行器依赖 catalog 里声明的属性——易变性（volatile/stable/immutable）、严格性（strict）、leakproof、并行安全标注、
  运算符族满足的数学性质（自反、对称、传递、三分律，跨类型运算符语义一致）——不做验证；声明错了，结果可能是错误的答案、报错或低效的计划。
  "相信声明、错了就错"的一般陈述见于邮件列表（转述，见当前原因）；文档只把它写成承诺和常见错误。这条决策的范围是从下面各处归纳的。
- 原始原因：（逐字）1999 年的常量化简 "depends on the proiscachable field of pg_proc to tell it whether a function is safe to pre-evaluate --- things like nextval() are not"（commit:40f65241614）；
  2002 年 "Divide functions into three volatility classes ... This allows indexscan optimizations in many places where we formerly didn't."（commit:4bdb4be62e6）；
  2006 年引入运算符族："related classes can be grouped into one family to represent the fact that they are semantically compatible"（commit:a78fcfb5124）；
  2007 年 "Add some notes about the basic mathematical laws that the system presumes hold true for operators in a btree operator family. This is mostly to clarify my own thinking about what the planner can assume for optimization purposes."（commit:d83235415bc）。
  二手：回顾文章写 "Postgres was the first DBMS to capture the costs and selectivities of UDFs in the database catalog."（二手来源，逐字）。
- 当前原因：（逐字）易变性 "is a promise to the optimizer about the behavior of the function"（file:doc/src/sgml/xfunc.sgml:1617-1618）；
  btree 运算符族必须满足一组基本假设，其中扩展到三种及以上数据类型的那部分 "the planner relies on them for optimization purposes"（两种类型之间的跨类型行为是索引搜索本身就需要的）（file:doc/src/sgml/btree.sgml:69-70,197-200）；
  族内所有运算符和函数必须语义兼容（file:doc/src/sgml/xindex.sgml:1030-1031）；leakproof："we trust the functions they use to not expose data"（file:src/backend/optimizer/README:1246-1247）；
  并行安全："The planner cannot automatically determine whether a user-defined function or aggregate is parallel safe..."，所以未标注的用户函数一律当作不安全（file:doc/src/sgml/parallel.sgml:533-541）；文档把错标列为常见错误（逐字）："A common error is to label a function IMMUTABLE when its results depend on a configuration parameter. ... For safety, such functions should be labeled STABLE instead."（file:doc/src/sgml/xfunc.sgml:1733-1737）。
  （社区明言，转述，来自检索摘要，措辞不保证一致）Tom Lane："If you declare a function immutable, Postgres will believe you; the consequences if you lied are on your own head."（url:https://postgrespro.com/list/thread-id/2605771）；
  2020 年 10 月 pgsql-bugs 上，用户把函数标成 immutable 后又改了它依赖的视图，结果错误，Tom Lane 回答这不是 bug：标了 immutable 又违背了这个承诺，规划器已经内联了那个值，没有办法撤回（url:https://www.postgresql.org/message-id/3401031.1602549490%40sss.pgh.pa.us）。
- 被否方案：在定义处强制检查声明（转述，2010 年 6 月 pgsql-hackers "Check constraints on non-immutable keys"，url:https://postgrespro.com/list/thread-id/1759460）：Tom Lane 反对强制检查易变性，理由是那样会逼人把可变的函数错标成 immutable。
  没有读到关于在运行时检查声明的讨论。
- 推出：[[I6]] 的大部分规则：[[I6]].R3（EC 的相等意义由 opfamily 决定，volatile 不进多成员 EC）、[[I6]].R5（相等证明依赖 opfamily 的相等语义）、
  [[I6]].R6（volatile 求值次数）、[[I6]].R7（qual 跨分组下推看 volatile 和相等关系）、[[I6]].R8（leakproof 例外）、[[I6]].R9（严格性）、
  [[I6]].R12（并行安全标注）、[[I6]].R15（COPY 批量化看 volatile 默认值）；[[I6]].R10 经由 R5 间接依赖。
  模块卡：[[S04.M05]]（Inv 2、5）[[S05.M10]]（Inv 1）[[S05.M12]]（Inv 2）[[S12.M05]]（Inv 4，strict 函数的调用约定）。R5、R6、R7 和 R9 的变换侧没有所有者卡片（[[X171]]）。
- 下游代价：声明错了的后果并不统一（逐字）：可能 "throw errors or produce wrong answers"（file:doc/src/sgml/parallel.sgml:564-566），"planner errors might occur"（file:doc/src/sgml/xoper.sgml:355），
  或者只是 "inefficient (though not incorrect) plans"（file:doc/src/sgml/xfunc.sgml:2115-2116）。运行时不检查；唯一的机械检查是回归测试 opr_sanity（1998 年加入），只在测试时检查内建条目（commit:de43a5e1bda; file:src/test/regress/sql/opr_sanity.sql:1-10）。
  I6 的保证因此带着适用条件："标注错了不在本不变量保证范围内"；计划生成后函数被重新标注，要靠计划缓存失效重规划（[[I4]]）。
- 重新审视的条件：[[E16]] 不再成立，即系统能机械地验证附加代码声明的属性。
- 扇出：(a)(b)：推出 [[I6]] 的大部分规则（一个不变量族），跨 [[S04]] [[S05]] [[S12]] 三个子系统（R8 的注入点还在 [[S03]]）。
  独立性：不能由 [[D-catalog-driven-extensibility]] 推出——把属性记在 catalog 里，不等于规划器要据此做变换（也可以只对内建类型做优化）。
- 证据：commit:40f65241614; commit:4bdb4be62e6; commit:a78fcfb5124; commit:d83235415bc; commit:de43a5e1bda; file:doc/src/sgml/ref/create_function.sgml:457-458; file:doc/src/sgml/parallel.sgml:533-541,564-566; file:doc/src/sgml/xfunc.sgml:1617-1618,1733-1737,2115-2116; file:doc/src/sgml/btree.sgml:69-70,197-200; file:doc/src/sgml/xindex.sgml:1030-1031; file:doc/src/sgml/xoper.sgml:355; file:src/backend/optimizer/README:1246-1247; file:src/test/regress/sql/opr_sanity.sql:1-10; url:https://arxiv.org/abs/1901.01973（TeX 源码第 192 行，逐字）; url:https://postgrespro.com/list/thread-id/2605771（转述）; url:https://www.postgresql.org/message-id/3401031.1602549490%40sss.pgh.pa.us（转述）; url:https://postgrespro.com/list/thread-id/1759460（转述）

### D-compat-by-release-class　兼容性按发布类别承诺：小版本不改磁盘格式和 ABI，大版本可以改
- 力：[[F5]]　前提：[[E8]]
- 原始原因：动机未考证。规则最早的书面表述（逐字，2000 年 6 月写进 backup.sgml、随 7.1 版在 2001 年发布，commit:a1642132192）："the internal data storage format is subject to change between releases ... This does not apply to different 'patch levels', these always have compatible storage formats."
  这是规则本身的最早成文，不是对动机的陈述。
- 当前原因：（社区明言）大版本做复杂的改动，数据目录的内容无法保持向后兼容，升级要用 dump/reload 或 pg_upgrade；小版本只包含高频 bug、低风险修复、安全问题和数据损坏问题的修复，升级不需要 dump/restore。"按较早小版本编译的扩展在较新的小版本里失效是不可接受的。"
- 讨论过的方向（转述）：2025 年 4 月 Bruce Momjian："When I wrote pg_upgrade, I assumed at some point the value of changing the storage format would outweigh the value of allowing in-place upgrades. I guess that hasn't happened yet."（url:https://www.mail-archive.com/pgsql-hackers@lists.postgresql.org/msg196724.html）。
  它说的是：在他看来，"改磁盘格式"与"原地升级"之间的取舍至今偏向后者（这是他个人的判断）；没有读到由此形成的正式结论。
- 推出：[[I8]]；约束全部子系统的磁盘格式、WAL 格式、协议和头文件，直接涉及 [[S12.M05]] [[S08.M10]] [[S09.M01]] [[S02.M03]]。派生：[[D-proto-minor-negotiation]]（见下面的协议一条）。
- 下游代价：需要改结构体布局的修复不能回补，只能进开发分支；回补时新成员只能加在结构体末尾；安全修复确实要触及 ABI 时，影响要逐条写明（T7）。
  大版本可以改磁盘格式，所以升级要搬数据：pg_upgrade 进入 contrib 之前只能 dump/reload；2010 年 "Add pg_upgrade to /contrib ... Supports migration from PG 8.3 and 8.4."（逐字，commit:c2e9b2f2881），
  它的文档说它免去了 "the data dump/reload typically required for major version upgrades"；前身 pg_migrator 见 commit:78a09145e0f。
- 协议是兼容性的另一条轴，不按发布类别（逐字）："The server and the libpq client library are backwards compatible with protocol version 3.0, implemented in PostgreSQL 7.4 and later."（file:doc/src/sgml/protocol.sgml:21-24）。
  2017 年（回补到全部受支持分支）让请求 3.0 以外的 3.x 版本不再直接失败，理由是此前 "any attempt to request a 3.x protocol version other than 3.0 would lead to a hard connection failure, which made the minor protocol version really no different from the major protocol version and precluded gentle protocol version breaks."（commit:ae65f6066dc）；
  2025 年加入协议 3.2，因为许多连接池和老服务器还不支持协商，libpq 默认仍用 3.0（commit:285613c60a7; file:doc/src/sgml/protocol.sgml:195-199）；3.1 因 pgbouncer 的协商 bug 而跳过（file:doc/src/sgml/protocol.sgml:290）。
  所以协议兼容跨越所有版本，协商机制是为了能引入小协议版本而不破坏旧端；[[D-proto-minor-negotiation]] 是它的派生。
- 重新审视的条件：未考证。
- 扇出：(a)(b)(c)。独立性：不可推出。
- 证据：url:https://www.postgresql.org/support/versioning/（转述）; url:https://wiki.postgresql.org/wiki/Committing_checklist（转述）; commit:1c27d16e6e5; commit:661643dedad; commit:a1642132192; commit:c2e9b2f2881; commit:78a09145e0f; commit:ae65f6066dc; commit:285613c60a7; file:doc/src/sgml/protocol.sgml:21-24,195-199,290; url:https://www.mail-archive.com/pgsql-hackers@lists.postgresql.org/msg196724.html（转述）

### 留出集里的基础性决策

下面 3 条来自 `holdout.md`，在作用检验之后补入。它们不支撑任何取舍规则或原则；它们依赖的前提（[[E14]] [[E15]]）也带 `holdout` 标注。

### D-own-buffer-cache　自己维护共享的页缓存，写出只交给内核，由 checkpoint 统一 fsync（`holdout`）
- 力：[[F1]] [[F6]]　前提：[[E14]]
- 原始原因：未考证（早于仓库历史，没有找到原始陈述）。
- 当前原因：写一个页只是把内容交给内核，真正落盘由内核决定；这没有问题，因为可以从 WAL 重做，只要在 checkpoint 之前 fsync。
  文档的说法是 PostgreSQL 同时依赖操作系统的缓存，所以共享缓存超过内存的 40% 不太可能更好。
- 正在重审的替代方案：直接 I/O。2023 年加入的开关只供开发测试，"目前会降低性能"，因为它关掉的内核预读和聚簇要等数据库自己实现。
  2025 年加入异步 I/O 的基础设施；默认的 I/O 方式改成后台 worker，提交只写了"暂时"。缓冲 I/O 仍是默认。
- 推出：[[I1]]；[[S08.M01]] [[S08.M02]] [[S08.M03]] [[S08.M04]] [[S09.M02]] [[S09.M03]] [[S11.M13]]。派生：[[D-fsync-fail-panic]]（落盘的语义取决于内核，所以 fsync 失败后不能重试）。
- 下游代价：同一页在内核缓存和共享缓存里各有一份；预读和回写的时机不由数据库控制 → read stream 和异步 I/O。
- 重新审视的条件：[[E14]] 不再成立。它正在变化。
- 扇出：(a)(b)，7 张模块卡、3 个子系统。独立性：不可由 [[D-wal-redo-only]] 推出，有 WAL 也可以用直接 I/O。
  假设：自建缓存是为了让数据库自己控制页写出和 WAL 刷盘的先后。
- 证据：file:src/backend/storage/buffer/bufmgr.c:4517-4523; file:doc/src/sgml/config.sgml:1826-1828,12945-12946; file:src/backend/storage/aio/README.md:7-10,31; commit:d4e71df6d75; commit:da7226993fd（`holdout`）; commit:247ce06b883; commit:b5a9b18cd0b

### D-xid-wraparound-freeze　事务号是 32 位、按模 2^32 比较；靠 freeze 把足够老的行版本标成"永远在过去"（`holdout`）
- 力：[[F1]] [[F5]]（F5 是假设，见被否方案）　前提：[[E15]]
- 原始原因：2001 年 8 月的三个提交：先把所有事务号比较封装进宏（"通往回卷方案的第一步"），再让事务号回卷（"按我 8 月 13 日的提案"，要求 initdb）。
  提交没有说为什么保留 32 位而不加宽；那份提案没有检索（未考证）。
- 当前原因：事务号只有 32 位，长期运行的集群会回卷，过去的事务会显得在未来，它们的输出变得不可见，文档称之为灾难性的数据丢失；所以每张表至少每 20 亿个事务要被 vacuum 一次。
- 被否方案：元组头里用 64 位事务号。没有读到否决结论（未考证）。假设：不加宽是因为要改磁盘格式并且增大每个元组的头部。
- 推出：[[I2]] [[I8]]；[[S05.M12]] [[S06.M07]] [[S06.M08]] [[S07.M02]] [[S07.M03]] [[S07.M04]]。
- 下游代价：没有死元组的表也必须定期被 vacuum 冻结；逼近界限时拒绝分配新的事务号；事务状态日志和 multixact 的截断都跟着冻结界限走。
- 重新审视的条件：未考证。
- 扇出：(a)(b)，6 张模块卡、3 个子系统。独立性：不可由 [[D-mvcc-no-overwrite]] 推出，事务号够宽就不需要冻结。
- 证据：commit:7326e78c424; commit:bc7d37a525c; commit:8e0d642c9bf; commit:2fc7af5e966; file:doc/src/sgml/maintenance.sgml:431-440,443-466

### D-logical-decoding-on-wal　逻辑变更流是从 WAL 解码出来的（`holdout`）
- 力：[[F3]]　前提：未考证
- 原始原因：2014 年的提交：把 WAL 流解码成一系列逻辑变更（插入、更新、删除以及它们所属的事务），能跨越表结构的变更，输出格式由输出插件决定。
  提交没有说为什么建在 WAL 上而不是触发器上；2012 年的设计讨论只读到转述，其中没有找到这段论证（未考证）。
- 当前原因：把描述存储层变更的 WAL 内容解码成应用需要的形式；建快照时复用 Hot Standby 的设施；只需要阻止删除 catalog 行，普通表的行照常清理。
- 被否方案：基于触发器的变更捕获（外部复制方案的做法）。假设，没有找到原文。
- 推出：[[I2]] [[I4]]；[[S06.M11]] [[S10.M01]] [[S10.M05]] [[S10.M06]] [[S10.M07]] [[S10.M08]] [[S10.M09]] [[S10.M10]] [[S10.M12]] [[S10.M13]]。
  后来建在它上面的功能（`holdout`）：内建的发布/订阅（commit:665d1fad99e，2017 年）——提交没有写动机（未考证）；为什么放进核心：提交和设计帖都没有写；当时外部方案 pglogical 已存在，讨论中有人主张进核心（转述）；文档列出的用途包括"在不同大版本之间复制"（file:doc/src/sgml/logical-replication.sgml:64），这正是物理复制做不到的。
  逻辑槽向物理备库同步，以支持故障切换（逐字，commit:ddd5f4f54a0）："synchronize the logical replication slots from the primary server to the physical standby so that logical replication can be resumed after a failover or planned switchover."（另见 commit:c393308b69d; commit:93db6cbda03）。
- 下游代价：WAL 要带上解码所需的额外信息（专门的 WAL 级别、每张表的 replica identity）；要保留历史的 catalog 行并重建历史快照；事务的变更要攒到提交才能交给插件；落后的消费者压住 WAL 的回收。
- 重新审视的条件：未考证。
- 扇出：(a)(c)，10 张模块卡、2 个子系统。独立性：不可由 [[D-physical-rep-ships-wal]] 推出，物理复制不需要解码；它以 [[D-wal-redo-only]] 和 [[D-catalog-mvcc-heap]] 为前提。
- 证据：commit:b89e151054a; commit:e55704d8b2f; commit:07cacba983e; commit:665d1fad99e; commit:ddd5f4f54a0; commit:c393308b69d; commit:93db6cbda03; file:doc/src/sgml/logical-replication.sgml:64; file:doc/src/sgml/logicaldecoding.sgml:255-258; file:src/backend/replication/logical/snapbuild.c:25-27; url:https://postgresql.org/message-id/20121115002746.GA7692%40awork2.anarazel.de（转述）; url:https://postgrespro.com/list/thread-id/1891173（转述）; url:https://postgrespro.com/list/thread-id/1897517（转述）

### 4.1 独立性检查中被降级的决策

这些决策按扇出本可入选，但能由根层决策加上力推出，所以只在所属子系统里保留一行。

| 决策 | 由谁推出 | 推导 |
|---|---|---|
| [[D-memory-context]] [[D-resource-owner]] | [[D-error-longjmp-abort]] + [[F1]] | 出错不逐层返回，所以内存和非内存资源都必须能按生命周期整体回收 |
| [[D-abort-by-clog]] | [[D-mvcc-no-overwrite]] | 旧版本本来就留在原处，中止只需把事务标成已中止 |
| [[D-hot]] | [[D-mvcc-no-overwrite]] + [[F4]] | 它是对"每次更新都产生新版本和新索引项"这一代价的补偿 |
| [[D-ssi]] [[D-evalplanqual]] | [[D-mvcc-no-overwrite]] + [[F2]] | 既然承诺读写互不阻塞，可串行化和并发更新都不能靠读锁实现 |
| [[D-parallel-copy-state]] | [[D-process-per-connection]] | 进程之间没有共享地址空间，并行只能复制状态并限制写 |
| [[D-plancache-lazy-inval]] | [[D-sinval-with-locks]] | 计划是 catalog 的派生物，沿用同一套失效消息 |
| [[D-replication-slot]] | [[D-physical-rep-ships-wal]] + [[D-mvcc-no-overwrite]] | 备库和解码器落后时，主库必须为它们保留 WAL 和旧版本 |
| [[D-postmaster-no-shmem]]（`holdout`） | [[D-process-per-connection]] + [[F1]] | 共享内存可能被一个出错的后端写坏；要能整体重置，监护进程自己就不能依赖共享内存 |
| [[D-shmem-fixed-plus-dsm]]（`holdout`） | [[D-process-per-connection]] + [[F7]] [[F4]] | 进程之间不共享地址空间，主段在各进程同址映射才能直接放指针；运行期才知道大小的需求另用可增删的段（后半句是假设） |

---

## 5. 原则 R

能复用到未来选择上的规律。只写有不少于两条非留出决策支撑的；状态按 GUIDE §4.4：没有找到反例才算确认。

**R2　新的扩展点用 C 层的回调结构体表达，catalog 里只登记名字和返回该结构体的 handler。**
- 适用范围：2011 年以来新引入或被重构的扩展点。它只管扩展点怎样登记和取得，不管取得之后每次调用怎样分发：
  实现可以在扫描开始时自选一个专门版本、存进扫描描述符，而不是结构体里的固定成员（commit:ddce1da5b1b，`holdout`，只用来划定范围，不算支撑）。
- 解决的冲突：[[F3]] 对 [[F7]]（让编译器检查接口，而不是靠 catalog 列描述 C 函数的签名）。
- 支撑决策：[[D-table-am]] [[D-custom-rmgr]] [[D-slot-ops]]；另有提交 commit:b8bff07daa8（ResourceOwner 的资源种类）、commit:7949d959458（可插拔的累积统计）。
- 预测：新提出的扩展点如果用多个 catalog 列分别登记支持函数，会被要求改成一个 handler 返回结构体。
- 例外/反例：更早的扩展点至今仍把多个支持函数分别登记：全文检索的解析器五列、过程语言三列、外部数据包装器三列，以及按行登记的操作符类支持函数。它们早于这条规律，没有被改造。
- 来源：由决策归纳。　状态：确认（限于上述适用范围；范围之外有遗留的例外）。
- 证据：commit:8586bf7ed88; commit:5c279a6d350; file:src/include/catalog/pg_ts_parser.h:42-54; file:src/include/catalog/pg_language.h:47-54

**R3　让特例走通用机制，不要在通用框架旁边另长一套逻辑。**
- 适用范围：功能上能被通用机制覆盖的路径。
- 解决的冲突：[[F7]] 对 [[F4]]，取舍见 T3。
- 支撑决策：[[D-expr-linear-steps]]（投影并入表达式求值）[[D-sqlfunc-plancache]]（SQL 函数改用计划缓存）[[D-view-rls-as-rewrite]]（视图和行级安全走同一个改写注入点）[[D-catalog-mvcc-heap]]（catalog 扫描改用和其他扫描一样的快照）。
- 预测：一个补丁如果为新功能复制一份规划、求值或可见性判定的逻辑，会被要求改成复用已有路径；除非它满足 T3 的例外条件。
- 例外/反例：有基准数据支持的快速路径被接受，例如绕过 SPI 的外键检查、严格函数的空值检查。
- 来源：由决策归纳。　状态：候选（存在反例；反例的条件已写成 T3）。
- 证据：commit:b8d7f053c5c; commit:3fc6e2d7f5b; commit:86dc90056df; commit:2da86c1ef9b; commit:6aecabcd5ed

**R5　破坏兼容性的改动要显式说明；默认只进它被允许进入的分支，回补时破坏 ABI 需要绕不开的理由。**
- 适用范围：改动磁盘格式、WAL 格式、协议、头文件里的结构体或函数签名的提交。
- 解决的冲突：[[F5]] 对 [[F7]]，取舍见 T7。
- 支撑决策：[[D-compat-by-release-class]] [[D-proto-minor-negotiation]]。
- 预测：一个修复如果需要改已发布分支上的结构体布局，会被改写成不改布局的形式，或者只进开发分支，并在提交信息里写明"这破坏 ABI"。
  修复绕不开时可以回补并触及 ABI（包括改变结构体大小），提交信息会逐条写明对扩展的影响，并说明为什么认为受影响的扩展很少。
- 例外/反例：见 T7 的三条反例（两条安全修复，一条普通的正确性修复）。它们违反"只进被允许的分支"，但都符合"显式说明"；预测的第二句是按它们加上的。
- 来源：社区明言（Committing checklist）。　状态：候选（反例来自裁决检验和编写 [[I8]] 视图；"显式说明"这一半没有见到反例，"只进被允许的分支"这一半只是默认做法）。
- 证据：url:https://wiki.postgresql.org/wiki/Committing_checklist（转述）; commit:1c27d16e6e5; commit:661643dedad; commit:ae65f6066dc; commit:245eb832f88; commit:3beee6e77c8; commit:45c357e0e85

**R6　不要为性能引入可能损坏数据的默认行为；可以提供由用户显式选择、只丢最近的事务而不损坏数据的选项。**
- 适用范围：涉及持久性、可见性和页完整性的改动。
- 解决的冲突：[[F1]] 对 [[F4]]，取舍见 T1。
- 支撑决策：[[D-full-page-writes]] [[D-catalog-mvcc-heap]] [[D-fsync-fail-panic]] [[D-wal-redo-only]]。
- 预测：以"可能读到不一致的数据"换速度的提案会被拒绝；以"崩溃时可能丢失最近若干毫秒的提交、但不会损坏"换速度的提案可以作为选项被接受。
- 例外/反例：存在关闭 fsync 和关闭整页镜像的开关，文档把它们标为风险自担。
- 来源：社区明言（文档对异步提交的界定）。　状态：确认。
- 证据：file:doc/src/sgml/wal.sgml:593-594,657-660; commit:568d4138c64; commit:9ccdd7f66e3

**R7　不要让默认路径依赖还没有在所有受支持平台上成熟的操作系统特性；平台特有的加速做成可选实现。**
- 适用范围：用到新的系统调用、I/O 接口、线程或编译器特性的改动。
- 解决的冲突：[[F6]] 对 [[F4]]，取舍见 T8。
- 支撑决策：[[D-jit-provider]]（核心不依赖 LLVM，JIT 在可单独加载的 provider 里）[[D-full-page-writes]]（不假定存储栈保证整页写）。
- 预测：只在一个操作系统上可用的 I/O 或并发机制，会被要求做成可选的实现方式，并保留在所有平台都能工作的默认方式。
- 例外/反例：未见。
- 来源：社区明言（Developer FAQ："我们抵制这种诱惑"）。　状态：确认。
- 证据：url:https://wiki.postgresql.org/wiki/Developer_FAQ（转述）; commit:432bb9e04da

**R8　共享状态可能已经损坏时，整体重置，不要带着可疑状态继续运行。**
- 适用范围：崩溃、fsync 失败、临界区内出错这类无法确认共享状态是否完好的情形。
- 解决的冲突：[[F1]] 对可用性（重置会中断所有会话）。
- 支撑决策：[[D-process-per-connection]]（一个后端崩溃后，其余后端被终止并重启）[[D-fsync-fail-panic]]（fsync 失败即 PANIC，不重试）。
- 预测：新发现的一条"共享内存或磁盘状态可能不一致"的错误路径，会被处理成 PANIC 加崩溃恢复，而不是尝试就地修复。
- 例外/反例：未见。
- 来源：由决策归纳。　状态：确认。
- 证据：commit:9ccdd7f66e3; url:https://www.postgresql.org/message-id/31cc6df9-53fe-3cd9-af5b-ac0d801163f4@iki.fi（转述）

**R9　不提供优化器提示；计划选错了就当作优化器的缺陷来报告和修复。**
- 适用范围：让用户在查询里指定执行方式的提案。
- 解决的冲突：[[F7]] 对用户对计划的直接控制。
- 支撑决策：[[D-path-plan-split]]（方案由代价比较选出）[[D-plancache-generic-custom]]（通用计划还是定制计划由机制按代价决定）。
- 预测：照搬其他数据库提示语法的提案会被拒绝；"能避开已知提示系统那些问题"的想法可以讨论。
- 例外/反例：已有若干排查用的开关，效果接近人们对提示的期待；社区承认提示对一次性的报表类查询有好处。
- 来源：社区明言。　状态：候选（存在上述例外）。
- 证据：url:https://wiki.postgresql.org/wiki/OptimizerHintsDiscussion（转述）; url:https://wiki.postgresql.org/wiki/Not_Worth_Doing（转述）

### 5.1 没有达到写入条件的候选

| 候选 | 没有写入的原因 |
|---|---|
| R1 数据表示必须让 `equal()` 可信 | 唯一的支撑决策在留出集里 |
| R4 持久化格式自描述，让外部工具通用实现 | 非留出的支撑决策只有一条（[[D-wal-self-describing]]）；通用 WAL 接口是同类的提交但不是已建模的决策。反例：每个资源管理器仍然自带解读自己记录的例程 |
| 用代码生成消除多处同步 | 非留出的支撑决策只有一条（[[D-genbki-dat]]）；同类的提交有查询指纹和等待事件的代码生成。反例：资源管理器表至今要手工与另一处定义保持一致 |

### 5.2 原则之间可能的冲突与对应的取舍规则

| 可能冲突的一对 | 由哪条取舍规则裁决 |
|---|---|
| R3（走通用机制）与性能特例 | T3 |
| R6（完整性）与性能 | T1 |
| R7（可移植）与性能 | T8 |
| R5（兼容）与更干净的修复 | T7 |
| R2（暴露 C 层接口）与兼容 | T6 |
| R8（整体重置）与 T5（隔离对开销） | 同向，不冲突：两者都以 F1 为先 |

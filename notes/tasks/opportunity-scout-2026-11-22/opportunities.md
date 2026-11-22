# 机会发现：以 arch-map 为候选源（2026-11-22，基线 4e0f8e930f3 / 20devel）

判定维度（用户给定）：① 是否值得解决 ② 是否常见路径 ③ 是否已有人在做 ④ 是否有真实需求（有人提过 / 会错结果）
⑤ 是否有本质正确的方案 ⑥ 实现难度、影响面、是否易被 judge。
与 skill 的映射：①②→W，③→G3，④→W3，⑤→W5/G5，⑥→L2/L3。

## 0 结论先行

- **A 档为空。** 唯一同时满足高 L、高 W 的方向（异步 I/O 覆盖到索引扫描）正是 2026 年在做的项目（CF 4351 + 两场 2026 年会议演讲）。
- 地图里唯一的**代码级缺陷**信号（分组边界 qual 外推的已知缺口）已进入 bug 流程，被多方并行处理（BUG #19797、2026-08 线程、2026-07 修复并回补 v18）。重复投递无意义。
- 可直接指向代码的残差只有两类：**无校验（8 条）**与**注释不符（6 条）**。前者全部是已写进注释的有意取舍，后者 W1≈0。
- 因此本图的定位应从"候选源"改为**"是否已有意为之 / 是否已被占用"的判据**；候选要从痛点与 git log 生成。

## 1 地图为什么产不出机会（定量）

187 条 residual 的类型分布：

| 类型 | 条数 | 对"值得做的改动"的贡献 |
|---|---|---|
| 已解决 | 32 | 0（已修） |
| 跨度 | 28 | 0（不变量跨模块，是事实描述） |
| 孤儿 | 23 | 0（地图追溯不到决策） |
| 缺证据 | 17 | 0（地图自己的证据缺口） |
| 职责 | 14 | 0（重构，skill 明确排除） |
| 检验不符 | 14 | 0（地图的检验方法失效） |
| 归属 | 13 | 0（文件归错模块） |
| 依赖 | 10 | 0（实际 uses 关系） |
| 未落位 | 9 | 0（卡片漏写别处的强制点） |
| **无校验** | **8** | 唯一直接指向代码的一类，但都是有意取舍 |
| **注释不符** | **6** | 注释/文档修正，W1≈0 |
| 未找准 | 5 | 0（抽象没写成一件事） |
| 多写者 | 4 | 0（并发结构描述） |
| 泄漏 | 2 | 0（地图说法不成立） |
| 缺卡 / 不一致 | 2 | 0 |

即 **137/187（73%）不自带可落地的改动**（跨度 28、孤儿 23、缺证据 17、职责 14、检验不符 14、归属 13、依赖 10、未落位 9、未找准 5、泄漏 2、缺卡 1、不一致 1：要么是架构事实的描述，要么是地图的完备性，要么是重构），另有 32 条（17%）已解决、4 条（2%）是并发结构描述（多写者）。真正落在代码行为上的只有 **14 条**（8 无校验 + 6 注释不符），且两类都按定义不产生可测收益。

地图真正有预测价值的是 `derivation.md` §11 的四条"正在松动的前提"。但它们恰好就是 2026 年社区在投入的地方：

| 前提 | 现状（本轮核对） | 对应工作 |
|---|---|---|
| E3 进程隔离 / 线程 | 正在变化 | 多线程化 wiki 清单，进行中 |
| E10 保留旧版本负担得起 | 正在变化 | zheap 未进核心，无活跃补丁（研究型） |
| E11 进行中事务数有界 | 正在变化 | CSN 提案（2013）无活跃；2020 已改稠密数组 |
| **E14 内核页缓存够用** | **正在变化** | **AIO 已落地（2025），索引预取进行中（CF 4351）** |

## 2 候选索引

| # | 名称 | 阶段 | 命中视角 | 归因类别 | G3 | G5 | L1–L5 | W1–W5 | 档位 | 一句话理由 | 结论 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| C1 | 异步 I/O / read_stream 覆盖到索引扫描 | S11 | 架构对标（E14）+ 自证 | 投入缺口 | **active** | 通过 | 5/4/3/5/5=4.4 | 4/4/3/5/4=4.0 | A（若 vacant） | 教科书级候选，也正是 CF 4351 在做的事 | 排除：已被占用 |
| C2 | 分组边界 qual 外推的已知缺口 | S2/S4 | 自证 + 痛点 | 局部约束 | **active** | 通过 | 4/4/2/5/5=4.0 | 5/2/4/5/4=4.0 | A（若 vacant） | 真会返回错结果，但归 bug 流程，且三方在做 | 排除：bug 类 + 已被占用 |
| C3 | 失效语义的定向机械校验 | S11/S2 | 自证 | 有意取舍（T4） | 现状答案已存在 | 弱 | 4/3/2/3/3=3.0 | 1/3/2/2/3=2.2 | D | `debug_discard_caches` 已是全量版；靶向版收益不可测 | 丢弃 |
| C4 | 规划器变换的差分预言器 | S2/S4 | 自证 + 检验（X160） | 结构性约束 | 外部已有 | 弱 | 2/1/1/2/2=1.6 | 2/4/4/2/3=3.0 | C | 核心跑两遍计划不可接受；只能做成核心外工具（SQLancer/sqlsmith 已覆盖） | 记录，不深挖 |
| C5 | 统计导入 vs VACUUM 写 pg_class 统计列 | S4 | 自证（X17） | 有意取舍 | n/a | 不成立 | 5/5/4/4/4=4.4 | 1/2/1/1/2=1.4 | D | 两者都持 ShareUpdateExclusiveLock，互斥；差异只是回滚语义，且是有意的 | 丢弃 |
| C6 | 游标无法走并行计划 | S5 | 自证（S5:99） | 结构性约束 | stale-attempt | 无更好方案 | 2/1/1/3/3=2.0 | 4/4/3/2/3=3.2 | C | 价值真实，但拆不出"独立有价值且好审"的第一阶段 | 记录，不深挖 |
| C7 | parallel-oblivious 哈希连接每 worker 复制哈希表 | S5 | 自证（S5:62） | 局部约束 | 未验证（CF 2903，2023-03） | 通过 | 3/2/3/4/2=2.8 | 3/3/2/4/3=3.0 | D（暂缓） | G3 无法确认；先联系作者再谈 | 未验证 |
| C8 | 已持锁时不接收失效（X72） | S11 | 自证 | 有意取舍 | n/a | 不成立 | 5/5/5/4/4=4.6 | 1/2/1/1/2=1.4 | D | 注释自述为省开销；能改变名字→OID 映射的 DDL 都取 AccessExclusiveLock，洞几乎不可达 | 丢弃 |
| C9 | 注释漂移（X30/X31/X45/X61） | S6/S8/S9/S10 | 自证 | 注释不符 | n/a | 不成立 | 5/5/5/5/4=4.8 | 1/1/1/1/2=1.2 | D | 顺手改可以，单独不值得一个 patch | 丢弃 |
| C10 | RegisterSyncRequest 不检查中断（X50） | S8 | 自证 | 有意取舍 | n/a | 不成立 | 4/4/4/3/3=3.6 | 1/1/1/1/1=1.0 | D | 注释说明取消会留下文件，属于刻意不撤回中断 | 丢弃 |

阶段代号按 skill 的阶段地图（S2 逻辑变换、S4 计划搜索、S5 计划缓存、S11 扫描策略）。

## 3 深挖

A 档为空，按要求从 C 档挑最有希望拆出第一阶段的候选。两个 C 档（C4、C6）都没有站得住的第一阶段，卡片如下。

### C6 游标无法走并行计划

- 一句话需求：让 `DECLARE CURSOR` + `FETCH` 的只读查询能用并行计划，而不是把整条查询降级为单进程。
- 命中的视角：自证。`subsystems/5.md:99`「只有一次性完整执行的计划才进入并行模式」。
- Q1 现在怎么做：
  - 规划器只在 `cursorOptions & CURSOR_OPT_PARALLEL_OK` 时置 `parallelModeOK`（`file:src/backend/optimizer/plan/planner.c:424-436`）。
  - 该位由 `pg_plan_query` 的调用方给：`copyto.c`、`createas.c`、`explain.c`、`matview.c`、`prepare.c`、`spi.c`、`functions.c` 都显式传入；`PerformCursorOpen` 不传（`file:src/backend/commands/portalcmds.c:45-160` 全文无 `CURSOR_OPT_PARALLEL_OK`）。
  - 执行侧的门控比规划侧宽松：`ExecutorRun` 只在 `already_executed || numberTuples != 0` 时关掉并行（`file:src/backend/executor/execMain.c:1741-1748`）。也就是说 `FETCH ALL`（`numberTuples == 0`、首次执行）本来就能跑并行计划；真正的缺口是规划期那个位。
  - 但任何"取 N 行"的 `FETCH`、JDBC 的 `setFetchSize`（扩展协议 Execute 带行数上限）、PL/pgSQL `FOR ... IN <query>` 的批量取，都落在 `numberTuples != 0`，执行器会强制退回非并行。所以收益只覆盖 `FETCH ALL` 与"一次取完整条查询"的用法。
  - 滚动游标不能并行：`PortalStart` 在用户没写 `SCROLL`/`NO SCROLL` 时会按计划内容自行决定（`file:src/backend/commands/portalcmds.c:145-151`），而并行计划不能 rewind。
  - 所以不是"少设一个 flag"，而是两件事：规划期放行（小），以及让 worker 与 DSM 跨 `ExecutorRun` 存活以支持增量取（大）。
- Q2 为什么没做好：
  - 归因类别：**结构性约束**。并行执行器假定"启动→跑完→结束"是一次性闭包；portal 假定"可反复 fetch、可 rewind、可在事务中途被放弃"。
  - 当年的理由：2017 年 Vondra 的实验补丁（`msg325039`）即以"显式开关"为前提，未进入主线；2024 年有跟进帖，无补丁。
  - 第一性原理：前提没有变（执行器仍是一次性闭包），所以这不是"前提松动"，是还没人愿意付这笔改造费。
- Q3 能否做好：
  - 更好的方案：把 worker/DSM 的生命周期从 `ExecutorRun` 提到 portal 一级，并禁止对并行计划 rewind（声明 `NO SCROLL` 才允许并行）。
  - 已有旁路：`postgres_fdw` 侧绕过游标以拿并行计划（CF 6233，PG20-3 活跃），说明需求真实，但绕的是 FDW 那一层。
  - 在 PG 中的实现路径：`PortalStart` 里为只读、`NO SCROLL`、无 `WITH HOLD` 的游标设 `CURSOR_OPT_PARALLEL_OK`；`ExecInitNode`/`Gather` 增加"保持存活"状态机；`PersistHoldablePortal` 前先收敛 worker。至少跨 `planner.c`、`portalcmds.c`、`pquery.c`、`execMain.c`、`nodeGather.c`、`execParallel.c`。
- 如何证明更好：TPC-H Q1/Q6 之类大表扫描，`FETCH ALL` 的延迟与 `max_parallel_workers_per_gather=0` 对比。
- 落地方案与 kill criteria：
  - 第一个可提交 patch（只说 `NO SCROLL` + `FETCH ALL`）：改 `PerformCursorOpen` 传 `CURSOR_OPT_PARALLEL_OK`，并禁止会自行变成 SCROLL 的游标。技术上可行、影响面小，但**价值低**：受影响的只有显式 `FETCH ALL`，而 JDBC/pgJDBC 的分页取与 PL/pgSQL 的游标循环都走 `numberTuples != 0`，一分收益拿不到。
  - 有价值的那一步（worker 跨 `FETCH` 存活 + Gather 支持增量取）无法拆成"本身有价值且 reviewer 一次能审完"的补丁。
  - 放弃条件：若"增量取 + 并行"必须同时改 `execParallel.c` 的 DSM 生命周期与 `Gather` 的取数协议，则不存在可独立提交的第一阶段 → 维持 C。
- 评分：L = 2.0 / W = 3.2 → **C 长期**。

### C4 规划器变换的差分预言器

- 一句话需求：给规划器的语义等价变换一个机械的差分判据，而不是只靠回归用例。
- 命中的视角：自证（`invariants/I6.md:119`「树内没有变换前后结果对比的差分预言器」）+ 检验（`X160`：九道规划器等价题，四种闭卷条件得分 0.00–0.06，抽象层面预测不了）。
- Q1：等价规则清单在 `I6.R1–R17`；校验手段只有回归测试、`setrefs` 的 nullingrels 精确匹配、`debug_parallel_query`、`debug_copy_parse_plan_trees`（`invariants/I6.md` §8）。
- Q2：归因 **结构性约束**——要"对比两次执行的结果"，需要同一查询的两份等价计划，而计划是代价驱动的，核心没有"另一条路"的概念。
- Q3：核心内做不了（跑两遍 = 两倍开销 + 结果集比较 + 不稳定性）；可行形态是外部工具（SQLancer TLP、sqlsmith 已覆盖这一类）或 assert 构建下的局部断言（例如 pushdown 后重算 `expression_has_grouping_conflict`）。
- 结论：**研究型 / 开发基础设施**，不入 A 档；若要投入，做扩展或外部工具，不做 patch。
- 评分：L = 1.6 / W = 3.0 → **C 长期**。

## 4 未入选清单

- **已被占用**
  - 索引预取 / 索引扫描 AIO — [CF 4351](https://commitfest.postgresql.org/patch/4351)（Next CF = PG20-3）、[PGConf.EU 2026](https://www.postgresql.eu/events/pgconfeu2026/sessions/session/8163/slides/925/overview-index-prefetching-eu.pdf)、[PGConf.dev 2026](https://www.pgevents.ca/events/pgconfdev2026/sessions/session/659/slides/107/index%20(scan)%20prefetching%20_%20pgconf.dev%202026-6.pdf)。BitmapHeapScan 的 read stream 改造已于 2025 提交。
  - 分组边界 qual 外推 — 2026-07-06 `44fb59fc605`（回补 v18）；[BUG #19797](https://www.postgresql.org/message-id/19797-7054aa017fa9d791%40postgresql.org)（2026-11）；["Unsafe qual pushdown through DISTINCT with simple CASE"](https://www.postgresql.org/message-id/CAMbWs4_6Yvqn8cSxGvYxp4c0yb4k2p-6Mb8_gCSrwYjNKoON%3D%40mail.gmail.com)（2026-08）。
  - 多线程化（E3）— wiki `Multithreading` 清单进行中。
  - 游标的并行旁路（FDW 侧）— [CF 6233](https://commitfest.postgresql.org/patch/6233)（PG20-3，Needs review）。
- **已被否决 / 有意取舍**：C3、C5、C8、C10；新增 optimizer hint 类（`principles.md` R9）。
- **研究型**：E10（undo / zheap）、E11（CSN 快照）、C4。
- **未验证**：C7（CF 2903 的当前状态；`nodeHashjoin.c` 有 `HJ_FILL_INNER` 状态机，但规划器是否为 FULL JOIN 生成 parallel-aware 哈希连接未核实）。
- **暂缓**：C9 注释漂移（与任何触碰同一文件的 patch 捆绑）。

## 5 最终推荐

1. **不要从这张图里找候选。** 它对"是否已有意为之 / 是否已被占用"极其有用，对"哪里有痛点"没有信息量；71% 的残差是它自己的完备性问题。
2. **本周期唯一值得投入的两个方向都已有人在做。** 想参与就并入 [CF 4351](https://commitfest.postgresql.org/patch/4351) 或分组外推的 bug 线程（Richard Guo 主持），而不是另起提案。
3. **若要做一次真正的 scout**，候选源换成：`pg_stat_statements` 总耗时最高的查询形态、EXPLAIN 估计误差 ≥10× 的节点、`pgsql-performance` 反复出现的主题、`git log --grep='^Revert'` 与"落地后跟着一串修复"的功能。地图随后用于回答"这是不是有意取舍、当年为什么这样定"——那正是它的强项。
4. **地图侧的一处小维护（若允许改）**：把 §11 四条前提的"现状"补上"是否已被项目接管"，并把 E14 标为"已被 CF 4351 接管"，否则下一次 scout 还会重复走到 C1。

## 6 剩余不确定性

- CF 4351 的具体状态（Needs review / Waiting on Author / Committed）只从检索摘要读到，未逐条核对 CF 页面。
- BUG #19797 的处理状态未读全文；"已被占用"的结论基于邮件列表时间线，不是维护者表态。
- C7 的 G3 未验证。
- 地图的"无校验"类若存在未被注释记录的真实缺口，本轮的判断会被推翻——可能性低，因为地图对每条都给了 `file:line`，本轮抽读了 X50/X72/X73 三条，注释与代码一致。

## 7 进度

- 已深挖：C6、C4（结论均为 C，无第一阶段）。
- 已核对代码：`namespace.c`、`sync.c`、`relation_stats.c`、`vacuum.c`、`clauses.c`、`planner.c`、`portalcmds.c`、`heapam.c`、`heapam_handler.c`、`nodeHashjoin.c`、`read_stream` 使用面。
- 未做：逐条浏览最近 6 个 CommitFest；本地集群与 benchmark（本轮不需要）。

---
name: pg-opportunity-scout
description: Use this skill to discover, verify, and rank PostgreSQL improvement opportunities in the query compute layer (planning, estimation, execution, and query performance), ranking landability first: changes that can realistically be accepted by the community, including small-module performance work and phased long-term items whose first phase is easy to land. Candidates must also be worth doing, have a known and provably better solution, and be unclaimed. The skill supplies the method only; candidates come from the evidence the caller gathers. Use it for requests such as "找值得做的改动", "找还没人做的需求 / patch 方向", or "what should I work on in PostgreSQL". Do not use it for bug triage, patch review, or implementing an already chosen change.
---

# pg-opportunity-scout

## 目的

寻找同时满足以下四个条件的改动，并**优先排出容易落地的**：

1. **容易落地**：
   - 能在 PostgreSQL 现有架构内实现。
   - 能拆出第一个可以独立提交、本身就有价值、committer 愿意接受的 patch。
   - 大的长期改动也可以，但必须能分阶段推进，而且第一阶段本身要容易落地。
2. **值得落地**：收益可以测量，用法常见，有独立的用户痛点证据，并且解决的是根因，而不是某个症状的特例。
3. **已知更好的方案**：知道什么方案更好，并能证明。证据可以是理论、生产系统的实践，或在 PostgreSQL 上的实测。
4. **还没人在做**：master 中没有实现，也没有活跃的 patch、讨论线程或 CommitFest 条目。**只要有人提出过 patch，就算已被占用。**

**排序原则**：PostgreSQL 社区审核严格，很多价值高的改动也很难合入。所以先看能不能落地、好不好落地，再看收益大小。小模块的性能优化，只要收益可测、所在路径常用，同样是好候选。

本 skill 只提供方法，不预设具体方向。所有候选都要由调用方从证据中发现并自行确认。最终产出是一份排好序的候选清单和推荐的下一步，而不是 patch。

**不在范围内：**
- bug fix：修正错误结果、崩溃，或与文档不符的行为。注意，计划选得不好、估算偏差大这类性能问题不算 bug fix，属于本 skill 的范围。
- 纯重构。
- 没有可测收益的改动。
- 为一个已经选定的方案写实现。

## 三条总原则

1. **从 PostgreSQL 自身的架构和代码出发。** 每个判断都要落到具体的代码路径、数据结构或设计决策上，并给出 `file:line`。外部系统和论文只能证明"存在更好的做法"，不能证明"PostgreSQL 能做到"。后者必须用 PostgreSQL 自己的代码结构来论证。
2. **先弄清为什么这样设计，再判断是否设计得不好。** 用第一性原理追问：当前设计依赖哪些前提？这些前提今天还成立吗？为什么没做好？能不能做好？具体方法见"根因三问"。
3. **多个视角相互印证。** 痛点、对标、代码自证、实测四个视角中，命中越多越可信。只有一个视角支持的候选，需要更强的单项证据。

## 工作流定位

- Task Type：`investigate`
- Investigation Kind：`design-explore`
- Target Readiness：`Answered`
- Evidence Plan：
  - 邻近代码、邻近测试、子系统 README
  - 官方文档与 release notes
  - `git log`，包括其中的 `Discussion:` 链接
  - `pgsql-hackers`、`pgsql-performance`、`pgsql-bugs` 和 CommitFest
  - 外部系统与论文，只作辅助证据
  - `release benchmark`，只在需要性能结论时使用
- 可以联网。邮件列表、CommitFest 和论文的检索入口见 `LOCAL_WORKFLOWS.md` 中的 "Community and Literature Lookup"。
- 先运行 `pg-task-intake`。记录写在三个文件中：
  - `notes/tasks/<slug>/task.md`：investigation note
  - `log.md`：执行过的命令和检索记录
  - `opportunities.md`：候选索引和深挖卡片
- 深挖时：用 `pg-rationale-trace` 追溯历史尝试和当年的理由，用 `pg-design-research` 查外部先例。
- 收尾：`pg-ready-check`（`Answered`），加上 investigation closeout。
- 正文用中文；PostgreSQL 标识符、文件名、函数名、SQL 和社区术语保留英文。

## 默认参数（调用方可覆盖）

| 参数 | 默认 |
|---|---|
| 改动大小 | 不限。新功能、机制改进、小模块性能优化都算；排除 bug fix 和纯重构 |
| 排序 | 先按 L（容易落地）排序，再按 W（值得落地）排序 |
| 数量 | 不设配额。建索引时覆盖范围内所有有信号支持的候选；深挖首选档的全部候选，按排序依次进行。调用方可以指定上限 |
| 档位阈值 | L 高：L 各项平均 ≥ 3.5，且没有 1 分项。W 高：W 各项平均 ≥ 3.5，且没有 1 分项。W 达标：W 各项平均 ≥ 2.5 |
| 运行数据 | 优先用调用方提供的 EXPLAIN ANALYZE、`pg_stat_statements` 或 benchmark；没有时，用 TPC-H、JOB、TPC-DS、pgbench 做小规模 probe |

## 范围：计算层（按功能界定，不按目录界定）

**定义**：一条查询从 SQL 文本变成结果行的过程中，决定"怎么算"和"算得多快"的那部分。

**判定规则**：看候选的主要收益落在哪里，不看代码在哪个目录。

- 主要收益是计划质量、执行效率（CPU、内存、由计划决定的 I/O 量）或规划开销 → 在范围内，即使代码位于 `access/`、`utils/` 等目录。
- 主要收益是持久性、复制、存储格式、后台维护、连接与协议、客户端工具 → 默认排除，除非有证据表明它是查询计算的瓶颈。

范围内不按模块排先后，由 L / W 评分决定。调用方可以把范围收窄到某几个阶段。

**阶段地图**：只是起点，不是完整列表。Phase 0 会用入口追踪和 profile 补全。下表路径都相对 `src/backend/`。

| 阶段 | 代码起点 |
|---|---|
| S1 语义分析与重写 | `parser`、`rewrite` |
| S2 逻辑变换：预处理、子查询上拉、外连接化简等 | `optimizer/prep`、`optimizer/plan/subselect.c`、`optimizer/plan/initsplan.c` |
| S3 基数与代价估计 | `optimizer/path/costsize.c`、`optimizer/path/clausesel.c`、`utils/adt/*selfuncs.c`、`statistics`、`commands/analyze.c` |
| S4 计划搜索：路径生成、join 搜索，以及分组、排序、窗口、集合操作 | `optimizer/path`、`optimizer/plan/planner.c`、`optimizer/util`、`optimizer/geqo`、`partitioning` |
| S5 计划缓存与参数化 | `utils/cache/plancache.c`、`tcop/postgres.c`（`exec_bind_message`） |
| S6 执行模型：节点迭代、tuple slot、元组解码 | `executor/execMain.c`、`executor/execProcnode.c`、`executor/execTuples.c`、`access/common/heaptuple.c` |
| S7 表达式求值与 JIT | `executor/execExpr*.c`、`jit`、`utils/fmgr` |
| S8 算子算法：sort、hash join、hash agg、window、memoize 等 | `executor/node*.c`、`utils/sort`，以及 `src/include/lib/simplehash.h` |
| S9 并行与异步 | `executor/execParallel.c`、`executor/nodeGather*.c`、`executor/nodeAppend.c` |
| S10 内存与溢出 | `utils/mmgr`，以及各算子的溢出（spill）路径 |
| S11 存储访问接口的调用侧：扫描策略、预取、bitmap | `access/table`、`access/index/indexam.c`、`executor/nodeIndex*.c`、`executor/nodeBitmap*.c` |
| S12 内置函数与类型求值 | `utils/adt` |
| S13 可观测性中与计算相关的部分 | `commands/explain*.c` |

## 第一性原理：根因三问

每个候选都要回答下面三个问题。建索引时各写一句话，深挖时写完整版。

- **Q1 现在怎么做**：代码路径、核心数据结构与算法、依赖的不变量（`file:line`）。
- **Q2 为什么没做好**：归因到具体的设计决策或前提假设，并归入下表中的一类。如果是有意为之，用 `pg-rationale-trace` 找到当年的理由。
- **Q3 能否做好**：更好的方案是什么、为什么更好（理论或实践依据）、在 PostgreSQL 架构内怎么实现、需要改变哪些不变量或接口。

### 归因类别

| 类别 | 含义 | 对落地的影响 |
|---|---|---|
| 投入缺口 | 技术上没有障碍，只是还没人做 | 最容易落地 |
| 局部约束 | 受现有结构限制，但能在现有抽象内解决，例如新增一种 Path、执行节点或统计类型 | 较容易落地 |
| 结构性约束 | 需要改动核心抽象或跨阶段接口 | 必须能分阶段推进，否则归为"长期" |
| 有意取舍 | PostgreSQL 为可扩展性、正确性、计划稳定性或简洁性做出的选择 | 必须论证当初的前提已经改变，并回应当年的理由 |
| 未解难题 | 学界和业界都没有公认的更好方案 | 属于研究，不算可落地的需求 |

### Q2 的分析透镜

| 透镜 | 典型信号 | 要回答的本质问题 |
|---|---|---|
| 启发式 vs 模型 | 魔法常数、固定阈值、凭经验拍定的默认值 | 这个决策能否由可估计的量推导出来？ |
| 独立性 / 均匀性假设 | 选择率直接相乘、假设数据均匀分布、从样本外推 | 能否捕获相关性？误差上界是多少？ |
| 局部 vs 全局 | 变换只覆盖简单形态，信息只在局部使用 | 有没有一个通用变换能覆盖这些特例？ |
| 静态 vs 自适应 | 计划在规划阶段定死，执行阶段没有反馈 | 运行时信息能否以低成本、可控的方式反馈给规划？ |
| 硬件假设过时 | 代价参数、内存默认值、执行模型仍按旧硬件设计 | 在 NVMe、多核、大内存和 CPU cache 的条件下，代价结构是否已经变了？ |
| 特例补丁堆积 | 同一类推理在多处各写了一份 | 能否用一个通用机制统一替代？ |
| 重复工作 | 每次执行、每个对象都要重做一遍的工作 | 能否缓存，或者只做一次？ |

### 归因时要核对的 PostgreSQL 架构特征

以下各项都要到代码中核实，不能想当然：

- 类型和算子完全可扩展（operator class、support function），所以优化通常必须对任意数据类型都成立。
- 多进程模型：每个 backend 有自己独立的缓存和计划缓存。
- Path 与 Plan 分离，join 顺序用自底向上的动态规划搜索。
- 执行器按节点逐元组迭代，节点之间通过 tuple slot 传递数据。
- 统计信息以单列为主，扩展统计需要用户显式创建。
- 社区偏好计划稳定、行为可预测。
- 任何变换都必须保持 NULL、volatile、collation、权限等语义不变。

"更本质的方向"必须落到具体机制上，不能停留在口号。

## 流程

处理顺序的原则：**便宜的步骤追求不遗漏，昂贵的步骤按优先级排队。**

- 建索引、本地门槛和粗评成本低，对所有候选都做，不设数量上限。
- 联网核实、历史追溯、probe 和深挖成本高，按档位和排序依次处理，处理完对应档位为止。

### Phase 0：基线与地图

**1. 版本与当前开发周期**

```bash
grep -m1 AC_INIT configure.ac
git log -1 --format='%h %ad' --date=short
# 当前开发周期的起点
git log -1 --format='%h %ad %s' --date=short --grep='^Stamp HEAD as'
```

**2. 已落地清单**

- 已发布版本：看官方 release notes，覆盖最近两个大版本。
- 当前开发周期：用 git log 粗筛，排除修复类提交，只看标题。

```bash
COMPUTE_DIRS=(src/backend/parser src/backend/rewrite src/backend/optimizer \
  src/backend/statistics src/backend/executor src/backend/jit \
  src/backend/utils/cache/plancache.c src/backend/utils/sort \
  src/backend/utils/adt src/backend/partitioning \
  src/backend/commands/analyze.c src/backend/commands/explain.c \
  src/backend/access/common/heaptuple.c)
BP=<上一步得到的开发周期起点日期>
git log --since="$BP" --no-merges --format='%h %ad %s' --date=short -- "${COMPUTE_DIRS[@]}" \
  | grep -viE '^[0-9a-f]+ [0-9-]+ (fix|doc|revert|remove|typo|update|silence|avoid|cleanup|clean up|correct|adjust|comment|add missing|use |rename|tidy|minor)'
```

**3. 曾经落地又被 revert 的改动**：说明既有需求，也有阻力。

```bash
git log --since=3.years -i --grep='^Revert' --format='%h %ad %s' --date=short -- "${COMPUTE_DIRS[@]}"
```

**4. 各区域活跃的 committer**：他们是潜在的 reviewer 和推动者。

```bash
git log --since=2.years --format='%cn' -- src/backend/optimizer | sort | uniq -c | sort -rn | head
```

**5. 补全阶段地图**

- 沿查询主路径追踪调用链：`exec_simple_query` → `pg_analyze_and_rewrite_fixedparams` → `pg_plan_queries` → `planner` → `standard_planner` → `subquery_planner` → `grouping_planner` → `PortalRun` → `standard_ExecutorStart` / `standard_ExecutorRun`。
- 再用视角 4 的 profile 结果补充。
- 发现地图之外、但与计算相关的代码，补进地图。

**产出**：已落地清单，以及补全后的阶段地图。

### Phase 1：从四个视角建索引（只采集信号，不通读，不设数量）

规则：

- 只要在范围内，并且至少有一个视角支持，就记一行。
- **按根因合并，不按症状拆分**：同一个根因的多个表现只算一个候选。
- 每行记录：命中的视角、证据位置、Q1 和 Q2 各一句话、初步归因类别。

**视角 1：自上而下的架构对标**

对每个阶段（S1–S13）填写下表：

| 阶段 | PostgreSQL 的核心设计决策 | 依赖的前提 | 前提今天是否仍成立 | 学界或业界的更好做法 | 证据 |
|---|---|---|---|---|---|

- 先读该阶段 README 的设计概述，再读关键代码。相关 README 包括 `src/backend/optimizer/README`、`optimizer/plan/README`、`executor/README`、`utils/mmgr/README`、`utils/fmgr/README`、`statistics/README`、`jit/README`、`parser/README`。
- 对标对象有两类：
  - SQL Server、Oracle、DuckDB、MySQL、Umbra/HyPer、Greenplum ORCA 等系统；
  - 经典论文中已被主流系统采纳的方法。
- 只记录"更好的做法有证据支持"的差距。"别人也这样做"或"别人没这样做"都不算差距。

**视角 2：用户痛点**

- `pgsql-performance` 中反复出现的主题。
- `pgsql-bugs` 中被定性为"是限制而不是 bug"的报告。
- 为了弥补核心能力而存在的扩展，例如 `pg_hint_plan`、`aqo`、`pg_qualstats`、`pg_duckdb`、列存类扩展。
- 云厂商在 PostgreSQL 之上额外补充的功能。
- dba.stackexchange 上的高票问题。

**视角 3：代码自证的局限**

先全库扫描，再按"范围判定规则"过滤，避免因预先列目录而遗漏。

```bash
# 代码中的 TODO 类标记：按文件计数，再到标记密集且在范围内的文件中抽样阅读
grep -rnciE 'XXX|FIXME|TODO|someday|not yet|for now|would be (nice|better)|punt' \
  --include='*.c' src/backend | grep -v ':0$' | sort -t: -k2 -rn | head -60
grep -n '#define DEFAULT_' src/include/utils/selfuncs.h

# 回归测试里固化下来的局限
grep -ciE "can.t|cannot|not (yet|currently)|should be able|ideally|would be better|XXX" \
  src/test/regress/sql/*.sql | grep -v ':0$' | sort -t: -k2 -rn | head -40
```

还要看：

- `enable_*` 类 GUC；
- `doc/src/sgml/{perform,planstats,parallel,ddl,queries}.sgml` 中提到 limitation、"currently"、"not supported" 的段落；
- Phase 0 查到的 revert；
- 落地后又跟了大量修复的功能；
- 热点文件的变更频率。

**视角 4：实测**

- 用 `LOCAL_WORKFLOWS.md` 中的 flamegraph 构建采集代表性负载的 CPU profile，把占比高的函数归到对应阶段。
- 找出 EXPLAIN ANALYZE 中估计行数与实际行数相差 10 倍以上的节点。
- 找出公开 benchmark（TPC-H、JOB、TPC-DS、pgbench）中 PostgreSQL 明显落后的查询。
- 找出 `pg_stat_statements` 中总耗时最高的查询形态。
- 调用方提供了运行数据时，优先使用。

### Phase 2：本地门槛与粗评（对所有候选做，成本低）

先过三道本地门槛：

- **G1 范围与类型**：符合"范围判定规则"，并且属于新能力、机制改进或性能 / 计划质量改进，而不是 bug fix 或纯重构。
- **G2 未实现**：用 `git log --grep`、代码 grep 和 release notes 三方交叉确认，master 中确实没有实现。
- **G4 正确性底线**：如果这个方向本身就带有无法控制的结果出错风险，直接排除。

再对通过门槛的候选做粗评：

- 用已有信号粗评 W1–W5 和 L1–L5。
  - L1 依据初步归因类别打分。
  - L2–L4 依据代码阅读和历史记录打分。
  - L5 依据 Phase 0 查到的 committer 活跃度打分。
- 按"分档"规则初步分档。
- 档位为"丢弃"的候选不再处理。

### Phase 3：按排队顺序核实 G3 和 G5，再定分和定档

**排队顺序**：A 档 → B 档的第一阶段 → 其他。同一档内，按 L 平均分从高到低排列；L 相同时，按 W1 × W2 从高到低排列。

G3 和 G5 都需要联网检索，成本较高，所以按上述顺序逐个处理。检索方法见 `LOCAL_WORKFLOWS.md` 中的 "Community and Literature Lookup"。

**G3 还没人在做**

- 检索范围：
  - 最近至少 6 个 CommitFest。逐个浏览 CF 页面，包括已经 Committed、Returned with Feedback、Withdrawn、Rejected 的条目。
  - `pgsql-hackers` 存档搜索。
  - `git log`。
- 把用过的检索词和链接写进 `log.md`。
- 检索结果标为以下状态之一：
  - `free`：没有找到相关 patch 或设计讨论 → 继续。
  - `stale-attempt`：曾有 patch，但超过 12 个月没有进展，或以 RwF / Withdrawn 结束 → 只有能说清"当年卡在哪里、现在为什么能解决"才继续，接手前先联系原作者。
  - `active`：最近 12 个月内有活跃线程，或在当前 / 下一个 CF 中有条目 → 排除，记入"已被占用"。
  - `rejected-by-design`：社区已经否决过这个方向，例如在核心中加入 optimizer hints（见 wiki `OptimizerHintsDiscussion`）→ 排除，除非有新的论据。
  - `unverified`：无法检索 → 可以列出，但不能作为最终推荐，并且必须明确标注。

**G5 已知更好的方案、能证明、能做到**

- **知道什么更好**：至少有下面一种证据。
  - 理论：有论文给出分析或证明，而且论文的前提与 PostgreSQL 相符。
  - 实践：已有生产系统实现，并有公开的效果数据或文档。
  - 本地实测：在 PostgreSQL 上做过 probe 或原型，并有数据。
- **能证明更好**：写清证明方法，即比较什么指标（延迟、吞吐、估计误差、规划耗时、内存等）、在什么负载上、和什么基线比。
- **能做到**：Q3 给出了在 PostgreSQL 架构内的实现路径，并且归因类别不是"未解难题"。
- 任何一条不满足，就归入"研究型"，不作为落地需求。

核实完 G3 和 G5 后，修正 W 和 L 的分数，确定最终档位。

#### 评分标准（每项 1–5 分，每个分数都要附理由和证据）

**容易落地（L）**

| 维度 | 5 分 | 3 分 | 1 分 |
|---|---|---|---|
| L1 架构可行性 | 投入缺口或局部约束 | 结构性约束，但能分阶段推进 | 无法分阶段推进；或属于有意取舍，又没有新论据 |
| L2 第一个 patch 是否好审 | 只改一个子系统，reviewer 一次能审完，可以独立提交，本身就有价值 | 需要先铺设基础设施 | 必须整体重写才有收益 |
| L3 风险可控 | 只在当前明显糟糕的情形下生效；规划耗时和内存不回退；结果出错的风险面小 | 需要用 GUC 或代价门控来限制生效范围 | 会大面积改变计划或语义 |
| L4 契合社区 | 复用现有基础设施；不新增 GUC 或 hint；不破坏扩展 API、catalog 或 `pg_upgrade` | 有争议，但能说服 | 违背已有共识 |
| L5 推动力 | 该区域有活跃的 committer，近期有同方向的提交 | 有人关心，但没人主导 | 没人关注 |

分阶段推进的长期改动，L 按第一阶段打分。

**值得落地（W）**

| 维度 | 5 分 | 3 分 | 1 分 |
|---|---|---|---|
| W1 命中时的收益幅度 | ≥2 倍，或从不可用变成可用 | 10%–50% | <5%，或无法测量 |
| W2 场景常见度 | 多数用户日常会写的查询形态，或每条查询都会经过的热路径 | 分析、报表类负载中常见 | 小众形态 |
| W3 痛点证据 | 至少 3 类独立来源 | 1–2 类 | 只是推测 |
| W4 更好方案的证据强度 | 理论、多个生产系统的实践、本地实测三者都有 | 有理论，或有单个系统的实践 | 只是推测（G5 不通过） |
| W5 本质性 | 消除根因，或用一个通用机制吸收多个特例 | 解决部分根因 | 只针对某个 benchmark 的特例 |

小模块优化如果位于热路径（W2 高），即使 W1 只有 2–3 分，也可能达标。消除热路径上根本性的开销，例如重复计算、选错数据结构，也算本质性改进。

#### 分档

| 档位 | 条件 | 处理 |
|---|---|---|
| **A 首选** | L 高，且 W 达标 | 进入深挖 |
| **B 分阶段** | L 低、W 达标，并且能拆出一个第一阶段，单独评分时 L 高、W 达标 | 第一阶段作为独立候选进入 A 档；整条路线记入"分阶段路线" |
| **C 长期** | L 低、W 高，但拆不出容易落地的第一阶段 | 记录在案，不深挖 |
| **D 丢弃** | W 不达标；或 L 低、W 不高，又拆不出第一阶段 | 丢弃 |

任何一项得 1 分，都要在结论里明确写出。

### Phase 4：深挖（范围由档位决定，不按固定数量）

**范围**

- A 档全部深挖，包括从 B 档拆出来的第一阶段，按 Phase 3 的排队顺序进行。
- 调用方指定了上限时，以调用方为准。
- 如果 A 档为空，就从 C 档中挑最有希望拆出第一阶段的候选深挖，目标是找出能落地的第一阶段。

**每个候选的步骤**

1. 完整回答根因三问。先读 README，再读代码，所有结论都给出 `file:line`。
2. 用 `pg-rationale-trace` 查历史尝试和主要反对意见。归因类别为"有意取舍"时，要找到当年的理由，并判断它的前提是否已经改变。
3. 用 `pg-design-research` 找 1–3 个外部系统先例和 1–2 篇论文，重点核对这些先例和论文的前提是否与 PostgreSQL 相符。
4. 做 probe：
   - 先证明问题确实存在。
   - 再尽可能用手工改写 SQL、`enable_*` 开关或其他控制计划的手段，模拟更好的方案，测出收益上限。这一步不需要先写实现。
   - 性能结论只能来自 release 构建。
   - 需要本地集群时，按 AGENTS.md 的规则自己启动、自己停止。
5. 写出第一个可提交的 patch 应该包含什么，以及放弃的条件（kill criteria）。分阶段的候选还要写出整条阶段路线。

**其他要求**

- 深挖结论可以推翻粗评。例如 G3 查到 `active`，或 probe 证明问题不存在、收益上限太小，就降档或排除，并写明原因。
- 每完成一张卡片，立即写入 `opportunities.md`，方便中断后在新会话中继续。

### Phase 5：输出（写入 `opportunities.md`，回复中给出摘要）

1. **候选索引表**，列出全部候选：

   | # | 名称 | 阶段 | 命中视角 | 归因类别 | G3 | G5 | L1–L5 | W1–W5 | 档位 | 一句话理由 | 结论 |
   |---|---|---|---|---|---|---|---|---|---|---|---|

2. **深挖卡片**：格式见下方模板。
3. **分阶段路线**：B 档和 C 档中每条路线的各个阶段，以及每个阶段的价值。
4. **未入选清单**：分为已被占用、已被否决、研究型、未验证、暂缓五类，每项附上链接或检索词。
5. **最终推荐**：先做哪个、为什么、本周就能开始的第一步。
6. **剩余不确定性**：还有哪些不知道，以及什么证据能最快消除它。
7. **进度**：哪些已经深挖，哪些还在队列中，下次从哪里继续。

## 深挖卡片模板

```markdown
### <需求名>

- 一句话需求：
- 所属路线与阶段（分阶段的候选填写：整条路线 / 本阶段 / 后续阶段）：
- 命中的视角（架构对标 / 痛点 / 自证 / 实测）及具体信号：
- Q1 现在怎么做（代码路径、数据结构、不变量，附 file:line）：
- Q2 为什么没做好：
  - 归因到的设计决策或前提：
  - 归因类别：
  - 当年的理由（如果有）：
  - 这些前提今天是否仍然成立（第一性原理分析）：
- Q3 能否做好：
  - 更好的方案是什么、为什么更好：
    - 其他数据库：系统 / 机制 / 公开的效果数据 / 与 PostgreSQL 的差异
    - 论文：问题模型 / 前提假设 / 与 PostgreSQL 不符之处
  - 在 PostgreSQL 中怎么实现：实现路径 / 需要改变的不变量或接口 / 与现有架构的冲突点
- 如何证明更好：指标 / 负载 / 基线 / probe 测出的收益上限
- 已确认的事实（每条附证据：file:line / commit / 线程 URL / 文档章节）：
- 尚未验证的假设（每条附验证方法）：
- 价值：
  - 用法是否常见（具体的查询形态、典型用户）：
  - 需求是否够多（证据的数量和来源类型）：
  - 收益量级（实测数据或合理推算，并注明是哪一种）：
- 是否已有人在做：检索词 / 链接 / 结果 / G3 状态
- 落地方案：
  - 第一个可提交的 patch：
  - 改动范围（子系统 / 文件）：
  - 后续 patch 系列：
- 风险：结果出错 / 计划回退 / 规划耗时 / 内存 / 兼容性（扩展 API、catalog、pg_upgrade）
- 社区接受度及原因：契合点、reviewer 可能的反对意见、可能的 committer 及其相关提交、类似提案过去的结局
- 验证方式：正确性测试（回归测试、EXPLAIN 测试）、无回退的证明（release 构建下的 benchmark）
- 真正要交付的成果：RFC 邮件 / PoC / patch v1
- 现实约束：时间、成本、长期维护
- 有没有更简单的路径就能满足需求（扩展 / hook / 现有 GUC / 文档）：
- 放弃条件（出现什么证据就放弃）：
- 评分：L = _ / W = _ → 档位
```

## 社区接受度检查项

以下是需要逐条核对的常识，不是已经确认的事实：

- 查询结果出错零容忍。
- 简单 OLTP 查询的规划耗时不能有可测量的回退。
- 对新增 GUC 持保留态度；核心不加 optimizer hints。
- 计划稳定性：新行为最好只在当前明显糟糕的情形下生效。
- 通用机制优先于针对某个 benchmark 的特例。
- 先做基础设施，并拆成可以分别审阅的 patch 系列。
- 修改 Node 结构、hook 或 catalog 会影响扩展和 `pg_upgrade`。
- 需要有 committer 感兴趣；大的 patch 之前，先在 `pgsql-hackers` 发 RFC 讨论设计。

## 反模式

- 没有检索就声称"还没人在做"。
- 编造邮件列表历史，或编造某位 committer 的立场。
- 只用外部系统或论文来证明"PostgreSQL 能做到"。
- 没弄清 PostgreSQL 为什么这样设计，就判定"设计得不好"。
- 只因为竞品有，就认定有需求。
- 把"未解难题"当成可落地的需求。
- 因为收益大，就把难以落地的改动排在容易落地的改动前面。
- 推荐找不到可提交的第一个 patch 的大规模重写。
- 把 bug fix 当作改进候选。
- 用 debug / cassert 构建的结果下性能结论。
- 把单条 benchmark 查询的提升当作普遍需求。
- 为凑数而填充候选，或为控制数量而提前丢掉候选。数量应由证据决定。
- 把同一根因的多个症状拆成多个候选。
- 按目录列举范围，漏掉位于其他目录的计算层问题。
- 通读大文件，而不是先建索引。

## 调用示例

> 用 `pg-opportunity-scout` 在当前 worktree 上做一次需求发现，范围用默认的计算层定义。
> 结果写到 `notes/tasks/opportunity-scout-<date>/`。

可选的附加要求：

- "只看 S2–S4"
- "只深挖前 N 个"
- "从上次的 `opportunities.md` 继续"

# Task: 用 arch-map 做机会发现（2026-11-22）

## 1. Intake

- Task type: investigate
- Investigation kind: design-explore
- Target readiness: Answered
- Evidence plan:
  - 地图底稿：`notes/arch-map/{L0,principles,derivation,residuals}.md`、`subsystems/`、`invariants/`
  - 代码：`src/backend/optimizer`、`executor`、`statistics`、`storage/aio`、`catalog/namespace.c`、`storage/sync`
  - 历史：`git log -S`、引入补丁的提交信息
  - 社区：CommitFest 条目、`pgsql-hackers` 与 `pgsql-bugs` 线程（CommitFest/邮件列表为第一优先）
  - 不需要：compile / regression / benchmark（本轮不产出 patch，也不做性能结论）
- Trivial or non-trivial: 非平凡（跨 12 个子系统，结论是"做/不做"的取舍）
- Owner: agent

## 2. Task framing

### 当前行为或问题

用户要在"本 session 构建的 arch-map（只读）"之上，按六个维度判断是否存在值得做的 PostgreSQL 改进：

1. 问题是否值得被解决；
2. 是否常见路径（忽略 corner case）；
3. 是否已有人在做（有人做了就忽略）；
4. 是否真有需求（有人提过，或会造成结果错误）；
5. 是否有本质正确的方案，而不是 tricky fix；
6. 方案是否好实现、影响面、是否容易被 judge / 容易出错。

### Target outcome

对"地图能否产出满足以上六条的候选"给出可辩护的答案：列出候选、逐条给 G1/G3/G5 与 L/W 评分、给出档位，并给出下一步。

### Why it matters

第二次把精力投在已被占用的方向上，是最贵的浪费。地图的价值若是"过滤"而不是"生成"，工作流应当据此改写。

## 2a. Investigation note

- question to answer：arch-map 的残差与"正在松动的前提"里，是否存在同时满足 L 高、W 达标、G3=free 的候选？
- current evidence or source set：187 条 residual（按类型已计数）+ `derivation.md` §11 的四条松动前提 + `invariants/I6.md` 的校验手段表 + `subsystems/4.md`、`5.md` 的能力边界行。
- likely invariants or design constraints：`principles.md` §3 的 T1–T8、§5 的 R2–R9（社区接受度）、§1 的 E 前提。
- current explanation or hypothesis：地图是"不变量 + 决策溯源"的底稿，不是痛点清单；其残差绝大多数是地图自身的完备性问题，能指向代码的只有"无校验"与"注释不符"两类。
- stopping condition：每条候选都过完 G1/G3/G5 并定档；A 档若有则深挖，若为空则说明为什么为空。

## 3. 结论

见 `opportunities.md`。A 档为空；唯一满足高 L 高 W 的方向已被 2026 年在做的项目占据。

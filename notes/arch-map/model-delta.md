---
id: model-delta
type: delta
depends: [invariants]
---
# 给模型的增量：模型不看材料时答不出来的规则要点

来源：`tests/prior/`。把 `invariants/I1..I8.md` §3 的 150 条规则各出成一道短题，让 haiku、sonnet、opus 三档模型不看任何材料作答，
再把每条规则拆成 2–3 个要点逐个评分（共 409 个要点）。下面只列**三档模型都完全没有答到**的要点，共 22 个。
其余要点至少有一档模型答到了，对模型读者是冗余的，留在 `invariants/` 里不重复。

用法：改动涉及某一族不变量时，只需要把这一族下面的几行给模型，不必让它通读 `invariants/`。完整的规则、条件和证据见所引的规则编号。

局限：题面会提示情境，测到的是"被问到时知不知道"，不是"干活时想不想得起来"；有的要点是复合规则里的一个附加条件，没答到也可能是题目没有问到。


## [[I1]] 持久性

- 备份期间强制 full page write（`I1.R3`）
- 或页 LSN 不早于提交 LSN，或表非持久（`I1.R6`）
- init fork 按持久关系对待（`I1.R17`）
- 时间线只在 shutdown checkpoint 处切换，不得在 minRecoveryPoint 前跳高（`I1.R21`）

## [[I2]] 可见性

- 不能用 clog 缓存的快速路径代替在途检查（`I2.R2`）
- 快照视为已提交的事务必须已离开 procarray，否则要等待（`I2.R3`）
- 否则 hint bit 会被永久写错（`I2.R3`）
- 失效的槽不参与聚合（`I2.R6`）
- HOT 剪枝不受表锁约束（`I2.R7`）
- prune 的 vistest 与 OldestXmin 共同保证 xmax < OldestXmin 的已删元组必被移除（`I2.R8`）

## [[I3]] 锁与顺序

- 可能影响 standby 上 SELECT 的变更必须用 AccessExclusiveLock（`I3.R4`）

## [[I4]] 缓存一致性

- 插入的元组也要登记以清掉负缓存（`I4.R4`）
- 执行器按 OID 匹配而不是按位置假设集合不变（`I4.R11`）
- 依赖快照的分区描述符不得缓存在 relcache（`I4.R11`）
- relpartbound 为空时必须重读而不是报错（`I4.R11`）

## [[I5]] 资源生命周期

- 事务内资源由 ShutdownPostgres→AbortOutOfAnyTransaction 回收（`I5.R2`）
- 释放时的 owner 必须与获取时相同（`I5.R8`）

## [[I6]] 语义等价

- 判重比较完整的 RestrictInfo，而非仅 clause（`I6.R11`）
- parallelModeOK 为假时整个查询不产生 partial path（`I6.R12`）

## [[I7]] 进程健壮性

- 不得读可能由 spinlock 模拟的原子量（`I7.R4`）
- fork 失败按子进程已崩溃处理，不当作已启动（`I7.R7`）

## [[I8]] 兼容性

- 出站消息整条发出（`I8.R10`）

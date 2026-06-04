# 从奠基 commit 到当前 patch：Bug 19481 nullingrels 统计匹配深读

> 配套文档。背景请先读 [`learning.zh.md`](./learning.zh.md)。
> 本文先把 `learning.zh.md` 第 6 节一笔带过的两个上游奠基 commit 展开讲透，
> 再 commit-by-commit 深读当前分支顶部两个提交：
>
> - `2489d76c4906f4461a364ca8ad7e0751ead8aa0d` — *Make Vars be outer-join-aware.*（Tom Lane, 2023-01-30, 进入 PG16）
> - `e28033fe1af8037e0fec8bb3a32fabbe18ac06b1` — *Ignore nullingrels when looking up statistics*（Richard Guo, 2025-01-02, backpatch 到 v16）
> - `6ab35e76b9b6f538ce9916d181012d06e7713a02` — *Ignore nullingrels when matching expression MCV statistics*（当前分支第一个提交）
> - `d8f3a56d201fc79c99bab5cf72c357afa5bdceec` — *Ignore nullingrels when matching expression dependencies*（当前分支第二个提交）
>
> 前两个提交分别**制造**了 `nullingrels` 这层装饰、并**确立**了"查统计信息时要剥掉这层装饰"的原则。
> 当前两个提交则把这条原则补到 Bug 19481 暴露出的多变量 MCV 路径，以及相邻的 functional-dependency expression 路径。

---

## 0. 一页速览：四个 commit 的因果关系

```text
            a4d75c86 (2021, PG14)
       表达式扩展统计信息成为可能
                  │
                  │  （前提，不是 bug 源）
                  ▼
        2489d76c (2023, PG16)  ← 本文 Commit A
     "Make Vars be outer-join-aware"
   给 Var 加 varnullingrels 装饰，
   修了一个真实的 planner 正确性 bug，
   但让"同一基表表达式带不带装饰会 equal() 不相等"
   成为一整类新问题的根源
                  │
        ┌─────────┴─────────────────────────┐
        ▼                                     ▼
  e28033fe (2025)  ← 本文 Commit B      多变量表达式统计残留路径
 "Ignore nullingrels when             多变量表达式 MCV 匹配
  looking up statistics"              ── Bug 19481 主线 ──
 在 selfuncs.c 里剥装饰，               由当前分支修复
 确立了"统计查找忽略 nullingrels"的原则
                                               │
                                               ├─ 6ab35e76
                                               │  修 MCV expression stats:
                                               │  clausesel.c / extended_stats.c / mcv.c
                                               │
                                               └─ d8f3a56d
                                                  修 expression dependencies:
                                                  dependencies.c
```

一句话串起来：

> **Commit A** 为了正确性，让 Var "知道自己在外连接之上可能变 NULL"；
> 这层 planner 专用装饰一旦加上，所有"只是想拿基表表达式去查统计信息"的代码，
> 都必须学会**忽略**它。**Commit B** 是第一次系统性地教会 `selfuncs.c` 这么做，
> 并写下了那条原则。**Commit C** 把同一原则补到 Bug 19481 直接命中的多变量 MCV
> 路径；**Commit D** 继续补上相邻但独立的 expression functional-dependency 路径。

---

## 1. Commit A：`2489d76` — Make Vars be outer-join-aware

### 1.1 在它之前：Var 是"位置无关"的

PostgreSQL 历史上，一列的引用在 parse / plan 树里到处都用**同一个** `Var` 结构表示。一个 `Var` 只携带：

- `varno`：它属于哪个 range table entry（哪张表/子查询）
- `varattno`：第几列
- 类型、typmod、collation 等

这套设计早于 SQL 外连接的支持。它隐含一个假设：

> 只要 `varno` / `varattno` 相同，两个 `Var` 就代表同一个值。
> 因此 `equalfuncs.c` 里 `equal()` 相等的两个表达式节点，代表同一个值。

planner 极度依赖这个假设——它到处用 `equal()` 判断"这两个表达式是不是同一个东西"（等价类推导、谓词下推、统计匹配……）。

### 1.2 问题：外连接让同一个 Var 在不同位置代表不同值

外连接打破了上面那个假设。README（`src/backend/optimizer/README:305`）给的规范例子：

```sql
SELECT * FROM t1 LEFT JOIN t2 ON (t1.x = t2.y) WHERE foo(t2.z)
```

（假设 `foo()` 非 strict，所以不能把 LEFT JOIN 化简成 INNER JOIN。）

注意 `t2.z` 出现了两次的"潜在位置"：一次在 ON 子句的语义层、一次在 WHERE 子句。它们的 `varno`/`varattno` 完全一样，老式 `equal()` 会判定相等。但它们其实**不是同一个值**。

用具体数据走一遍。设：

```text
t1 = { (x=1), (x=2) }
t2 = { (y=1, z=10) }     -- 只有 y=1 能匹配 t1.x=1
```

LEFT JOIN（在 WHERE 过滤之前）产生：

```text
行1： x=1, y=1,    z=10      ← t1.x=1 匹配上 t2
行2： x=2, y=NULL, z=NULL    ← t1.x=2 没匹配，null-extended 行
```

现在看 WHERE 里的 `foo(t2.z)`：

```text
行1： foo(10)
行2： foo(NULL)   ← 这里的 z 是 null-extended 的 NULL，不是扫描层的 10
```

关键观察：

- **WHERE 里的 `t2.z`** 必须看到 **NULL**（对 null-extended 行而言）。
- 如果 planner 天真地把 `foo(t2.z)` 下推到 t2 的扫描层，它只会算出 `foo(10)`，永远造不出 `foo(NULL)` 这一行的结果。而且若 `foo` 返回 false，正确语义是**整行从结果里消失**，而不是输出一条 null-extended 行。

所以"扫描层的 `t2.z`"和"外连接之上的 `t2.z`"是两个**不同的值身份**。README 原话：

> This motivates considering "t2.z" within the left join's ON clause to be a
> different value from "t2.z" outside the JOIN clause.

老式 Var 没法表达这个区别，于是 planner 有真实的正确性风险：它可能把两个本应不同的值当成相等，做出错误的下推或等价推导。

### 1.3 第二个例子：等价类（EquivalenceClass）也会被坑

README（`:329`）的第二个例子更微妙：

```sql
SELECT * FROM t1 LEFT JOIN t2 ON (t1.x = t2.y) WHERE t1.x = 42
```

- 我们**希望**用等价类机制推出 `t2.y = 42`，当作 t2 扫描的过滤条件——这是合法且有用的优化（`y != 42` 的 t2 行不可能影响结果）。这里用的是**扫描层**的 `t2.y`。
- 但我们**绝不能**得出"每一条输出行里 `t2.y` 都等于 `t1.x = 42`"。因为 null-extended 行里 `t2.y = NULL ≠ 42`。这里是**外连接之上**的 `t2.y`。

同一个 `t2.y`，扫描层可以等于 42，连接之上不能。又是"同一个 Var、两种值身份"。

### 1.4 解法：给 Var 装饰 `varnullingrels`

Commit A 在 `Var` 结构里加了一个字段（`src/include/nodes/primnodes.h`）：

```c
typedef struct Var
{
    ...
    Oid         varcollid;
    /* RT indexes of outer joins that can replace the Var's value with null */
    Bitmapset  *varnullingrels;     // ← 新增
    ...
}
```

`varnullingrels` 是一个 **bitmapset**，记录"在这个 Var 出现的位置，哪些外连接（用它们的 RT index 标识）可能已经把它置为 NULL"。

回到 1.2 的例子：

```text
ON 子句里的 t2.z：      varnullingrels = {}        （扫描层值，没被任何外连接 null 过）
WHERE 子句里的 t2.z：   varnullingrels = {OJ_rti}  （被这个 LEFT JOIN 可能置 NULL）
```

于是这两个 `t2.z` 的 `Var` 节点**不再 `equal()` 相等**——因为多了一个不相等的 bitmapset 字段。planner 由此能正确区分它们。

> 关于"哪边被标记"：LEFT/RIGHT JOIN 只给**可空侧**的 Var 打标记；FULL JOIN 两侧都打。
> `PlaceHolderVar`（PHV）出于同样的原因也带一个对应的 `phnullingrels`。

### 1.5 概念模型：`CASE WHEN ... THEN NULL`

README（`:365`）给了一个心智模型——一个带非空 `varnullingrels` 的 Var 可以**想象成**：

```text
CASE WHEN any-of-these-outer-joins-produced-a-null-extended-row
  THEN NULL
  ELSE the-scan-level-value-of-the-column
END
```

务必记住后面那句话：

> It's only notional, because no such calculation is ever done explicitly.

planner **不会**真的插入这个 CASE 节点。在最终计划里，扫描层的 Var 代表真实列值；上层 Var 是对下层计划节点输出的引用；当 join 节点输出 null-extended 行时，它直接对相关列返回 NULL，而不是从输入拷贝。`varnullingrels` 只是一个**标签**，让 planner 在做决策时知道"这个值在这个位置可能是 NULL"。

> 这个"只是标签、不是真值变换"的性质，正是后面**可以安全剥掉它**来查统计信息的根本原因——剥掉它不改变任何执行语义，只改变 planner 对"这俩表达式是不是同一个"的判断。

### 1.6 核心不变量（这次提交的灵魂）

Commit message 把目标讲得很直白：

> This allows us to trust that `equal()` Vars represent the same value.

也就是说，2489d76 之后，planner 重新获得了那个它赖以生存的不变量：

```text
equal() 相等的两个表达式  ⟺  代表同一个值
```

代价是：表达式的"身份"现在**包含了 `varnullingrels`**。两个除了 `varnullingrels` 以外完全一样的 Var，会被判为**不相等**。

### 1.7 副作用：relids 现在含外连接 relid（一切后续 bug 的种子）

Commit message 还说了关键的一句：

> In the planner, we include these outer join bitmapsets into the relids
> that an expression is considered to depend on, and in consequence also
> add outer-join relids to the relids of join RelOptInfos.

翻译成后果就是：

- `pull_varnos()` 抽取一个表达式依赖的 relids 时，**会带上 `varnullingrels` 里的外连接 relid**。
- 因此 `RestrictInfo.clause_relids`、`RelOptInfo.relids` 都可能**同时**包含真实基表 relid 和外连接 relid。

这正是 `learning.zh.md` 第 7 节里那些失败点的总根源：

- `find_single_rel_for_clauses()` 用 `clause_relids` 判断"这个子句是不是只属于一张基表"——但 `clause_relids` 现在混进了外连接 relid，看起来像"涉及多个关系"。
- `statext_is_compatible_clause()` 同样的单关系判断也会被外连接 relid 误导。
- `mcv_match_expression()` 用 `equal()` 把查询表达式映射到统计维度——但带装饰的表达式和不带装饰的存储表达式不 `equal()`。

> **记住这条因果**：2489d76 不是"写错了"。它修了一个真实的正确性 bug，设计是对的。
> 它只是把"表达式身份"的定义扩大了，于是**所有"只想要基表表达式"的下游代码**
> 都欠下了一笔"必须学会忽略 nullingrels"的债。Commit B 开始还这笔债，当前分支接着还。

### 1.8 版本边界：为什么相关修复都 backpatch 到 v16

2489d76 的作者日期是 2023-01-30，进入 **PostgreSQL 16**。`varnullingrels` 这套基础设施是 v16 才有的。所以：

- v15 及更早**没有**这层装饰，也就**没有**这一类 bug。
- 任何"剥掉 nullingrels"的修复，最早只需要、也只能 backpatch 到 **v16**。

这就是 Commit B 的 message 里那句"back-patch to v16 where the outer-join-aware-Var infrastructure was introduced"的由来，也是 Bug 19481 修复同样以 v16 为下界的原因。

---

## 2. 共用机制：`remove_nulling_relids()`

在讲 Commit B 之前，先认识它和当前分支都依赖的那把"剥装饰"的工具——`remove_nulling_relids()`（`src/backend/rewrite/rewriteManip.c:1339`）。

```c
/*
 * remove_nulling_relids() removes mentions of the specified RT index(es)
 * in Var.varnullingrels and PlaceHolderVar.phnullingrels fields within
 * the given expression, except in nodes belonging to rels listed in
 * except_relids.
 */
Node *
remove_nulling_relids(Node *node,
                      const Bitmapset *removable_relids,
                      const Bitmapset *except_relids)
```

它是一个表达式树 mutator，对树里每个 Var / PHV：

```c
if (var->varlevelsup == context->sublevels_up &&
    !bms_is_member(var->varno, context->except_relids) &&
    bms_overlap(var->varnullingrels, context->removable_relids))
{
    var = copyObject(var);
    var->varnullingrels = bms_difference(var->varnullingrels,
                                         context->removable_relids);
    return (Node *) var;
}
```

三个要点：

1. **只动 `varnullingrels` / `phnullingrels`**，不碰 `varno`、`varattno`、表达式形状。剥完之后，表达式在"基表身份"意义上和统计对象里存的那个变得一致。
2. **`removable_relids` 一般传 `root->outer_join_rels`**——即整个查询里所有外连接 relid 的集合。意思是"把所有外连接置空标记都抹掉"。
3. **`except_relids` 是逃生舱**：被它列出的关系上的 Var 不剥。统计查找路径通常传 `NULL`（全剥）。

> 为什么剥掉是安全的？回到 1.5：`varnullingrels` 只是"notional 标签"，不对应任何真实计算。
> 在**只为查统计信息**这个狭窄目的里，我们要的是"这个表达式在基表上长什么样"，
> 外连接会不会把它置 NULL 跟"基表上 `coalesce(mod(a,20),1)` 的取值分布"无关。
> 所以剥掉它，既能匹配上统计，又不改变任何查询语义。

---

## 3. Commit B：`e28033fe` — Ignore nullingrels when looking up statistics

### 3.1 一句话原则

Commit message 开宗明义：

> When looking up statistical data about an expression, we do not need to
> concern ourselves with the outer joins that could null the Vars/PHVs
> contained in the expression.

这就是那条原则。它修复 `selfuncs.c` 里两类被 `nullingrels` 破坏的统计查找。改动只有 29 行（`selfuncs.c`），但确立的边界很重要。

### 3.2 修复点一：`add_unique_group_var()` — 同一列被数了两次

**路径**：`estimate_num_groups()`（估算 GROUP BY / DISTINCT 的分组数）→ 对每个分组表达式调 `add_unique_group_var()` 收集去重后的分组变量 → 把单关系的那批变量交给 `estimate_multivariate_ndistinct()` 去匹配多变量 ndistinct 统计。

**bug 机制**：`add_unique_group_var()` 用 `equal()` 去重（`selfuncs.c:3694`）：

```c
foreach(lc, varinfos)
{
    varinfo = (GroupVarInfo *) lfirst(lc);
    /* Drop exact duplicates */
    if (equal(var, varinfo->var))   // ← 带不同 nullingrels 就不相等，去重失败
        return varinfos;
    ...
}
```

由 1.2 我们知道：**同一列**在外连接的不同位置，可以带**不同的** `varnullingrels`（ON 子句里空、WHERE/输出里非空）。一旦两个本应相同的分组变量带着不同装饰进来，`equal()` 判不相等 → **去重失败** → 同一个底层列产生了**两个** `GroupVarInfo`。

后果发生在 `estimate_multivariate_ndistinct()`（`selfuncs.c:4567`）。它按统计对象的列去匹配分组变量，用一个 `matched` bitmap 记录命中的统计列，最后要求找到一个属性数恰好吻合的统计项：

```c
if (tmpitem->nattributes != bms_num_members(matched))
    continue;
...
if (!item)
    elog(ERROR, "corrupt MVNDistinct entry");   // selfuncs.c:4819
```

同一个统计列被两个重复变量命中，会让计数对不上，最终没有任何统计项满足，触发：

```text
ERROR:  corrupt MVNDistinct entry
```

**修复**（`selfuncs.c:3687`）：在去重之前先剥装饰，让同一列回到唯一身份。

```c
ndistinct = get_variable_numdistinct(vardata, &isdefault);

/*
 * The nullingrels bits within the var could cause the same var to be
 * counted multiple times if it's marked with different nullingrels.  They
 * could also prevent us from matching the var to the expressions in
 * extended statistics (see estimate_multivariate_ndistinct).  So strip
 * them out first.
 */
var = remove_nulling_relids(var, root->outer_join_rels, NULL);   // ← 修复
```

**这个 commit 自带的 reproducer**（`src/test/regress/sql/join.sql`，commit 一并加入）：

```sql
CREATE TABLE group_tbl (a INT, b INT);
INSERT INTO group_tbl SELECT 1, 1;
CREATE STATISTICS group_tbl_stat (ndistinct) ON a, b FROM group_tbl;
ANALYZE group_tbl;

EXPLAIN (COSTS OFF)
SELECT 1 FROM group_tbl t1
    LEFT JOIN (SELECT a c1, COALESCE(a) c2 FROM group_tbl t2) s ON TRUE
GROUP BY s.c1, s.c2;
```

注：`e28033fe` 原始 diff 里写的是 `COALESCE(a)`。当前 worktree 的同一测试已经被 2025-07-03 的 `931766aaec58b2ce09c82203456877e0b05e1271` 调整成 `COALESCE(a, a)`，原因是 PostgreSQL 后来会把单参数 `COALESCE()` 简化掉；测试需要继续保留"非 strict 子表达式"这个意图。两种写法在本文讨论的 `nullingrels` 机制上等价。

这里 `GROUP BY s.c1, s.c2` 两列都源自 `t2.a`，且都位于 LEFT JOIN 的可空侧，但因为分别走了"裸列 `a`"和"`COALESCE(a)`"两条路径（后者在可空侧会被包成 PlaceHolderVar），两者带上的置空标记不一致。

- **修复前**：去重失败 → `estimate_multivariate_ndistinct` 计数错乱 → `ERROR: corrupt MVNDistinct entry`，`EXPLAIN` 直接报错。
- **修复后**：先剥装饰 → 两路归一 → 干净出计划。

> 我没有重建修复前的树来亲眼复现这条 ERROR（那需要 checkout 到 e28033fe 之前并重编）。
> 证据是 commit message 明确记录的症状 + 它一并加入的回归测试 + 上面 `selfuncs.c:4819` 那行 guard。
> 这条路径"裸列 vs COALESCE 为何带不同标记"的精确内部原因，涉及 PHV 的 `phnullingrels` 处理，
> 属于本文未完全展开的细节——但**它会触发 ERROR、修复消除了 ERROR** 这一行为层事实是确凿的。

### 3.3 修复点二：`examine_variable()` — 表达式匹配不上统计/索引（Bug 19481 的直系前身）

这是和 Bug 19481 血缘最近的一处。`examine_variable()` 负责为一个表达式找出"它的统计数据"，包括：单列统计、**表达式索引**的列、**扩展统计里的表达式**。

它做了两件被 `nullingrels` 破坏的事，Commit B 各修一处：

#### (a) 单关系判定：用 `basevarnos` 而不是 `varnos`

```c
varnos = pull_varnos(root, basenode);
basevarnos = bms_difference(varnos, root->outer_join_rels);   // ← 新增：去掉外连接 relid
...
if (bms_is_empty(basevarnos))            // ← 用 basevarnos 判断
{
    /* No Vars at all ... must be pseudo-constant clause */
}
else if (...)
{
    /* Check if the expression is in vars of a single base relation */
    if (bms_get_singleton_member(basevarnos, &relid))   // ← 用 basevarnos 判断
    ...
}
```

由 1.7 我们知道 `pull_varnos()` 会带上外连接 relid。一个**只属于一张基表**的表达式，`varnos` 里却混着外连接 relid，于是 `bms_get_singleton_member(varnos, ...)` 判它"不止一个关系"，拿不到 `relid`，后面整段找统计的逻辑被跳过。减掉 `root->outer_join_rels` 后，单基表表达式重新被识别为单关系。

> **注意这与当前分支高度同构**：当前分支在 `clausesel.c` 的 `find_single_rel_for_clauses()`
> 里做的 `clause_relids - root->outer_join_rels`，跟这里的 `varnos - root->outer_join_rels`
> 是**同一招在不同函数里的复刻**。Commit B 修的是单变量统计入口，当前分支修的是多变量
> 扩展统计入口。

#### (b) 表达式归一：比较前剥掉 nullingrels

```c
/*
 * The nullingrels bits within the expression could prevent us from
 * matching it to expressional index columns or to the expressions in
 * extended statistics.  So strip them out first.
 */
if (bms_overlap(varnos, root->outer_join_rels))
    node = remove_nulling_relids(node, root->outer_join_rels, NULL);   // ← 修复
```

紧接着这段，`examine_variable` 会遍历表达式索引的列、扩展统计里的表达式，用 `equal()` 找匹配项。**带装饰的查询表达式 ≠ 不带装饰的存储表达式**，所以剥之前必然匹配失败。

**一个可精确推演的例子**（直接对应 commit message 第二段，也预演了 Bug 19481）：

```sql
CREATE TABLE et (a int, b int);
CREATE INDEX et_expr_idx ON et ((a + b));      -- 表达式索引，统计存在 (a+b) 上
INSERT INTO et SELECT i, i FROM generate_series(1,1000) g(i);
ANALYZE et;

-- 把 et 放在 LEFT JOIN 的可空侧，按 a+b 分组
EXPLAIN
SELECT a + b, count(*)
FROM (VALUES (1)) d(x) LEFT JOIN et ON true
GROUP BY a + b;
```

planner 内部：

```text
存储的索引表达式：       (a + b)                              -- varnullingrels = {}
查询里的分组表达式：     ((a:OJ) + (b:OJ))                     -- 两个 Var 都带 {OJ_rti}

equal(query_expr, index_expr)：
  剥之前： 不相等  → 找不到表达式索引的统计 → ndistinct 退化成默认估算
  剥之后： 相等    → 用上索引表达式的 ndistinct → 估算正确
```

这正是 commit message 那句：

> the nullingrels could prevent us from matching an expression to
> expressional index columns or to the expressions in extended statistics,
> leading to inaccurate estimates.

把这个例子里的"表达式索引 + GROUP BY 的 ndistinct"换成"多变量 MCV 统计 + WHERE 谓词选择率"，几乎就是 Bug 19481：

```text
Bug 19481 里：
  存储的统计表达式：    coalesce(mod(a,20),1)
  查询谓词表达式：      coalesce(mod(a:OJ,20),1)
  匹配路径：            mcv_match_expression() 的 equal() 比较
  结果：                剥之前不相等 → 漏用 MCV → 50 行被估成 1 行
```

### 3.4 为什么 backpatch 到 v16，且"有计划变化也接受"

Commit message：

> This patch could result in plan changes, but it fixes an actual bug, so
> back-patch to v16 where the outer-join-aware-Var infrastructure was introduced.

两点值得学：

1. **它承认会改计划**（回归测试里有一处计划变化），但因为是"修真 bug"而非"调优"，社区接受 backpatch。这是 PostgreSQL 对"正确性修复 vs 行为变化"权衡的典型态度。
2. **v16 是硬下界**，原因见 1.8——再早的版本没有 `varnullingrels`，不存在这个 bug。

### 3.5 它留下的缺口 → 直接交棒给 Bug 19481

Commit B 只改了 `selfuncs.c`：单变量统计入口（`examine_variable`）和分组估算入口（`add_unique_group_var`）。它**没有**触及 `extended_stats.c` / `mcv.c` 里的**多变量 MCV 子句匹配**路径。那条路径同样用 `equal()` 直接比表达式（`mcv_match_expression()`），同样会被 `nullingrels` 破坏——这就是 Bug 19481，由当前分支按**完全相同的原则**修复。

```text
e28033fe 覆盖：   单变量统计 / 表达式索引 / ndistinct 分组   （selfuncs.c）
e28033fe 漏掉：   多变量表达式 MCV 子句匹配                  （extended_stats.c + mcv.c）  ← Bug 19481
当前分支补上：    把同一条"查统计前剥 nullingrels"原则，
                  应用到 find_single_rel_for_clauses /
                  statext_is_compatible_clause /
                  mcv_match_expression
```

---

## 4. 把两个 commit 接成一条因果链

```text
① 老式 Var：位置无关，equal() == 同值（外连接下这个假设是错的）
        │
        │  2489d76 发现并修复这个正确性 bug
        ▼
② Var 带 varnullingrels：equal() 重新 == 同值，
   但"表达式身份"现在含外连接装饰，
   且 pull_varnos 会把外连接 relid 混进 clause/rel 的 relids
        │
        │  这层装饰对"查基表统计"是纯噪声
        ▼
③ 凡是"拿表达式去查统计/索引"的代码，
   若直接 equal() 比较或用 relids 判单关系，
   都会被装饰误导 → 漏用统计 / 计数错乱
        │
        ├─ e28033fe：在 selfuncs.c 修两处，确立原则
        │            "查统计前用 remove_nulling_relids 剥装饰，
        │             用 relids - outer_join_rels 判单关系"
        │
        └─ Bug 19481（当前分支）：同原则补到多变量 MCV 路径
```

最该记住的那条边界（`learning.zh.md` 第 9 节也强调过，这里给出它的出处）：

```text
查询语义中的表达式身份：     保留 nullingrels    （来自 2489d76 的核心不变量）
基表统计查找中的表达式身份： 忽略 nullingrels    （来自 e28033fe 确立的原则）
```

2489d76 负责前一行，e28033fe 负责后一行。两者**不矛盾**——它们作用在不同目的上。看懂这条边界，就看懂了这一整类 bug 的修法。

---

## 5. 对照速查表

| 维度 | `2489d76`（Commit A） | `e28033fe`（Commit B） |
|---|---|---|
| 标题 | Make Vars be outer-join-aware | Ignore nullingrels when looking up statistics |
| 作者 / 日期 | Tom Lane / 2023-01-30 | Richard Guo / 2025-01-02 |
| 角色 | **制造** `varnullingrels` 装饰 | **教会代码忽略**该装饰 |
| 解决的问题 | 外连接下 `equal()` Var 不再代表同值的正确性 bug | 统计查找被装饰破坏（漏匹配 + corrupt MVNDistinct entry） |
| 关键改动 | `Var` 加 `varnullingrels` 字段；relids 含 OJ relid | `selfuncs.c` 里 `remove_nulling_relids` + `varnos - outer_join_rels` |
| 规模 | 巨大（60 文件，~3878 行） | 微小（3 文件，~63 行） |
| 版本 | 进入 PG16 | backpatch 到 v16 |
| 与 Bug 19481 | bug 的**机制根源** | bug 修法的**原则与先例** |
| 副作用 | 埋下一整类下游 bug 的种子 | 一处计划变化（被接受） |

---

## 6. 证据索引

### 6.1 Rationale cards

**Card A：outer-join-aware Var**

- `source`: commit `2489d76c4906f4461a364ca8ad7e0751ead8aa0d` and `src/backend/optimizer/README`
- `provenance`: direct provenance
- `proposal`: 给 `Var` / `PlaceHolderVar` 增加外连接置空标记，让外连接之上和外连接之下的同名列引用不再被误认为同一个值。
- `main objections`: commit message 未记录具体反对意见；可见风险是改动面很大、外连接重排需要额外处理、FDW foreign join 必须跟随 core planner 的 relids 语义。
- `decision or status`: landed in PostgreSQL 16；后续修复以 v16 为 backpatch 下界。
- `implication`: 任何修复都必须保留 "`equal()` 相等表示同一个 planner 值" 这个不变量，不能为了统计匹配直接移除查询树里的 `nullingrels`。
- `uncertainty`: 本文没有展开 `outer join identity 3` 下添加/移除 OJ RTI 的所有细节；这里只使用它作为"表达式身份必须包含 nullingrels"的直接依据。

**Card B：statistics lookup ignores nullingrels**

- `source`: commit `e28033fe1af8037e0fec8bb3a32fabbe18ac06b1`
- `provenance`: close precedent
- `proposal`: 在查找统计信息前剥掉表达式中的 `nullingrels`，并在单关系判断时用 `relids - root->outer_join_rels`。
- `main objections`: commit message 明确承认可能产生 plan changes；可接受性来自它修复的是实际 bug，而不是单纯调优。
- `decision or status`: landed and backpatched to v16；修复范围是 `selfuncs.c` 中的 `add_unique_group_var()` 和 `examine_variable()`。
- `implication`: Bug 19481 的修复应当复用同一原则，但作用域限定在扩展统计 / MCV 匹配路径，不能改变外连接求值语义。
- `uncertainty`: `e28033fe` 没覆盖 `extended_stats.c` / `mcv.c` 的多变量 MCV 子句匹配；这正是当前分支要补的缺口。

### 6.2 File-level evidence

**Commit A — `2489d76c4906f4461a364ca8ad7e0751ead8aa0d`**
- `Var.varnullingrels` 字段定义：`src/include/nodes/primnodes.h:220`（Var 结构）
- 概念模型与两个规范例子：`src/backend/optimizer/README:305-410`（`t2.z` 例、`t2.y=42` 例、`CASE WHEN` 模型、relids 含 OJ relid）
- `Discussion:` https://postgr.es/m/830269.1656693747@sss.pgh.pa.us

**Commit B — `e28033fe1af8037e0fec8bb3a32fabbe18ac06b1`**
- `add_unique_group_var()` 剥装饰：`src/backend/utils/adt/selfuncs.c:3687`（去重 `equal()` 在 `:3694`）
- `estimate_multivariate_ndistinct()` 的 guard：`src/backend/utils/adt/selfuncs.c:4819`（`corrupt MVNDistinct entry`）
- `examine_variable()` 的 `basevarnos` 与表达式剥装饰：当前 worktree 为 `src/backend/utils/adt/selfuncs.c:5697` 与 `:5769` 附近；`e28033fe` 原始 diff 在 `:5077` 与 `:5145` 附近
- reproducer：当前 worktree 为 `src/test/regress/sql/join.sql:3855-3866` 的 `group_tbl` 测试；`e28033fe` 原始 diff 使用 `COALESCE(a)`，后续 `931766aaec58b2ce09c82203456877e0b05e1271` 改为 `COALESCE(a, a)`
- `Discussion:` https://postgr.es/m/CAMbWs4-2Z4k+nFTiZe0Qbu5n8juUWenDAtMzi98bAZQtwHx0-w@mail.gmail.com

**共用机制**
- `remove_nulling_relids()`：`src/backend/rewrite/rewriteManip.c:1339`（声明 `src/include/rewrite/rewriteManip.h:90`）

---

## 7. Investigation closeout

- **问题**：把 `2489d76` 与 `e28033fe` 两个 commit 展开，结合例子讲清前因后果。
- **答案**：见上。`2489d76` 为正确性给 Var 加 `varnullingrels`，并让 relids 含外连接 relid，从而把"表达式身份含外连接装饰"变成既成事实；`e28033fe` 第一次系统性地在 `selfuncs.c` 里确立"查统计信息时剥掉这层装饰、用 `relids - outer_join_rels` 判单关系"的原则。两者一个埋因、一个立则，Bug 19481 是该原则尚未覆盖的多变量 MCV 路径。
- **消费的证据**：两个 commit 的真实 message 与 diff、`optimizer/README` 概念模型、`remove_nulling_relids` 函数体、`selfuncs.c` 修复点与 `corrupt MVNDistinct entry` guard、commit 自带回归测试、两条 `Discussion:` 链接。
- **停止条件**：两个 commit 的机制、动机、相互关系、与 Bug 19481 的衔接均已用 file:line 级证据讲清。
- **剩余不确定**：`group_tbl` 那个 reproducer 里"裸列 `a` vs `COALESCE(a)` 为何带不同 `nullingrels`"的精确 PHV 处理细节未逐节点展开（行为层"触发/消除 ERROR"已确凿）；本文未亲自重建修复前的树复现 ERROR。
- **建议下一步**：若要更彻底，可 checkout 到 `e28033fe^` 重编并实跑 `group_tbl` 测试，亲眼看到 `corrupt MVNDistinct entry`；以及审计 functional-dependency / ndistinct 的表达式路径是否还有同类残留（这也是 `learning.zh.md` 第 11 节列出的 reviewer 可能追问点）。

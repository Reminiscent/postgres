# Review of notes/arch-map/derivation.md (first-time reader)

Scope: the reader is someone who knows databases, will not read the code, and wants to predict what a change would do. I read only `derivation.md`, lines 1 to 1346, in order. Line numbers refer to that file. Severity: H = blocks understanding or misleads, M = slows the reader, L = polish. Issues are ordered by importance.

## Overall assessment

The document mostly achieves its purpose. The seven-part layout holds in every decision section. The "于是必须有" items nearly always give a "否则……" reason, so the reader can reconstruct why a rule exists, and most self-test questions can be answered from the text, with answers that match. Its main weakness is at the very centre of the derivation. The order 3.2 → 4.1 presents redo-only WAL as the cause and MVCC as the consequence. The document's own history sections say the opposite (no-overwrite came first), and the reader never learns early enough that redo-only only works *because* rows are never overwritten. A second problem is how much of the map's internal bookkeeping leaks into the prose: unexplained method terms ("独立性检查", "重构检验", "留出集"), coverage notes, and four retellings of the X175 hedge. These cost trust and attention without adding reasoning.

**Strongest sections.**
- **2.3**: "下层锁省掉的每一种保障，都变成了使用它的代码必须自己遵守的一条规则" is a model derivation frame, and all three questions are answerable.
- **5.3**: every ordering rule comes with the failure it prevents. The reader can predict bugs from it.
- **7.2**: the historical catalog snapshot is derived cleanly from 5.2, and the "only catalog rows are held back" point follows.

**Sections needing the most work.**
- **2.2**: the critical-section item depends on WAL and shared buffers, which do not exist yet. It is overloaded with history and hedges, and two of its three questions are broken.
- **3.2**: it hides that it depends on no-overwrite storage. Its 来历 is one 600-character wall, and the causal direction it states contradicts 4.1's history.
- **Section 6 preamble and 6.1**: the preamble is about the map's coverage gaps, not about PostgreSQL. 6.1's "于是必须有" lists rule details that the preamble itself says cannot be derived from 6.1.

---

## Issues

### 1. [LEAP/CONTRADICTION, H] Redo-only is presented as the cause of MVCC, but the document's own history says no-overwrite came first and is what makes redo-only work
- L321: "只做 redo，不做 undo：崩溃时没有提交记录的事务一律当作已中止，它写到页上的改动不去撤回。"
- L339: "没有 undo，被中止事务插入的行……只能靠'这个事务最后提交了没有'来屏蔽……这是 4.1 要接手的问题。"
- L422: "3.2 又已经决定不做 undo，中止事务写下的改动会留在页上。"
- Against L345: "假设：中止事务的改动之所以可以不撤回，是因为旧版本本来就留在原处……（4.1）"
- Against L448: no-overwrite storage comes from Berkeley POSTGRES, and WAL arrived in 1999–2001.

**Problem.** At L321 the reader immediately asks: "If an aborted UPDATE overwrote a value and that page reached disk, how is the old value restored without undo?" The answer (UPDATE never overwrites) appears only in 4.1 and 9.1. Meanwhile 3.2 and 4.1 each cite the other as their premise. The reader cannot tell which decision is the root, and that is exactly what they need to predict "what if we switch to in-place update".

**Fix.** Add a premise sentence at the start of 3.2's "要解决的问题":
> 这里先用到一个 4.1 才展开的事实：PostgreSQL 从不原地覆盖一行，UPDATE 写新版本、旧版本留在原处（继承自 Berkeley POSTGRES，早于 WAL）。正因为如此，未提交事务写到盘上的东西只是"多出来的新版本"，不撤回也不会毁掉旧值；这是只做 redo 能成立的前提（9.1）。

Then rephrase L339 and L422 so they no longer present 4.1 as a consequence of 3.2. For example, at L422:
> 而且 3.2 只做 redo 的做法本身就依赖这一点。

### 2. [ORDER/STRUCTURE, H] The critical section is derived in 2.2, before WAL and the shared page cache exist, and the 2.2 questions need that content
- L202: "典型的是'改了共享页缓存里的一页并写了对应的日志记录'这一组动作（见 3.2）…… 共享缓冲区里有还没写日志的改动，必须保证它们不会落到盘上"
- L219, question 2.2-1: "而系统里又没有能撤销页面修改的日志（见 1.1 的 F1 和本节'于是必须有'）"

**Problem.** The reader does not yet know what WAL-before-data is, so they cannot see why an unlogged change reaching disk would be fatal. Question 2.2-1 asks them to derive exactly that. Its pointer "见 1.1 的 F1" is also wrong: F1 says nothing about undo logs.

**Fix.**
- In 2.2, reduce the item to: "临界区：少数多步修改中途不能被 longjmp 打断，区内任何 ERROR 升级为 PANIC、走 2.1 的整体重置；为什么必须这样，要等 3.2 讲完日志先行才说得清（见 3.2'临界区'一条）。准备工作（分配内存等）必须在进入之前做完。"
- Move question 2.2-1 to 3.2, where the text at L330–331 supports it, and delete the "1.1 的 F1" pointer.

### 3. [OVERLOAD/CONTRADICTION, H] The critical-section and X175 story is told five times, and 9.5 asserts the causal link it then calls an inference
The five places are L202, L215, L331, L345, L994 (and again in the answer at L1204).
- L994: "'日志写了一半、页改了一半'的状态没法靠中止恢复，因为 3.2 没有撤销。" and later in the same paragraph: "它与'没有撤销'的联系，仍是推断，文档没有写".

**Problem.** The reader meets the same hedge paragraph again and again, and 9.5 contradicts itself. Meanwhile the document already contains a self-contained reason that needs no inference. In L330 the protocol is "改页 → 标脏 → 写 WAL 记录". Between those steps, a shared page carries a change with no log record. If an ERROR aborted here, the page would stay dirty in the cache and could be written to disk, which violates L329. So the only safe exit is to throw away shared memory.

**Fix.**
- State that reason once, in 3.2's critical-section item (L331).
- Put the 2000–2001 history and the X175 caveat once, in 3.2 来历.
- Cut the history from L202 and L215.
- Rewrite 9.5 so its first sentence uses that reason. For example: "临界区里共享页上已有改动、日志记录还没写；中止撤不掉这个改动，又不能让它落盘，所以只能 PANIC……" Then add one clause: "它与'没有 undo'的关系是推断（X175）".

### 4. [STRUCTURE, H] Question 2.2-3 depends on a fact the question never states
- L221 describes only a shared hash table: "先把条目插进共享表，再初始化它的字段".
- Answer L1208: "修复只清理了共享哈希表，却忘了创建者自己的本地查找表里已经插入了一个引用".

**Problem.** The local lookup table is never mentioned, so the reader cannot find "what the fix missed".

**Fix.** Add to the question: "创建者在插入共享表的同时，也在自己的本地查找表里登记了一个指向该条目的引用。"

### 5. [CONTRADICTION, H] 5.3 says role changes produce no catalog invalidation, but the answer to 5.3-2 fixes it with catalog callbacks
- L676: "search_path 或行级安全相关的角色设置与生成计划时不同，也要重做分析和改写，因为这些变化不产生 catalog 失效".
- Answer L1284: "在角色成员、角色、数据库三张 catalog 上注册回调".

**Problem.** A reader following L676 will answer "add an environment check" and be marked wrong. The text never separates "current role / search_path (session state, no invalidation)" from "role membership (a catalog row, which does invalidate)".

**Fix.** Change L676 to:
> 计划还依赖会话环境：当前角色、search_path 这类会话设置变化不产生 catalog 失效，要在复用计划前比对；而角色成员关系、角色属性本身存在 catalog 里，它们的变化会产生失效，但只有在计划缓存为这些 catalog 注册了回调时才会被看到（上一条）。

### 6. [UNEXPLAINED, H] The map's method terms are used as evidence but never defined
- "独立性检查": L534, L619, L664, L808, L908, L982, L990 ("独立性检查的结论值得记住").
- "重构检验第 31 项" and similar: L583, L585, L704, L730.
- Also L704 "卡片未写", L1150 "扇出（16 张模块卡）", L1174 "`holdout` 标注……作用检验", L1176 "敏感性检验……留出集".

**Problem.** These phrases carry the argument (for example "所以本决策不能由 5.1 推出"), but the reader cannot judge evidence whose method they do not know. Several of them (holdout, 留出集, 卡片未写) are about the map's test history and say nothing about PostgreSQL.

**Fix.**
- Add one paragraph to section 0:
  > 文中"独立性检查"指：地图逐条检验"这个决策能否由前面的决策推出"，推不出的记为独立的选择；"重构检验"指让只读过目标和决策的读者推导不变量，看能推出多少。
- Delete the test-history remarks in L704 (the second half), L1150 ("16 张模块卡"), L1174 (the holdout parenthesis) and L1176 (the sentence about the sensitivity test).

### 7. [UNEXPLAINED, H] "xmin" names four different things, and "在途" is ambiguous at the core visibility rule
- L425: tuple xmin.
- L461: snapshot xmin.
- L472: "本后端公布的 xmin".
- L863: a standby's xmin; L895: catalog_xmin.
- L470: "读者先判'是否在途'，再查事务状态日志". Here "在途" means the live running set (procarray) in the derivation "存在一段窗口，一个事务既在集合里". But the answers to 4.2-1 and 4.2-2 (L1250, L1252) reason with "the snapshot's in-progress list".

**Problem.** The reader cannot tell which "in progress" the rule checks, and that decides how question 4.2-2 comes out. The four xmins invite wrong inferences (for example, that a backend's published xmin is a tuple header field).

**Fix.**
- At L461, add: "注意：这里的 xmin 和版本头里的 xmin（4.1）同名不同义；后文'本后端公布的 xmin'指的是本后端所有快照的最小 xmin，供 4.1 的视界计算用。"
- At L470, say explicitly: "可见性判定里的'在途'按自己的快照判（快照里的进行中列表）；其他查提交状态的代码按共享内存里的运行集合判。两者都必须先于查 clog。"

### 8. [CONTRADICTION, M] Multixact "does not follow WAL-before-data", but WAL-before-data was presented as the definition of WAL
- L329: "这条规则就是'预写日志'这个选择的定义".
- L539: "它不守'WAL 先于数据'，靠页 LSN 互锁保证引用它的 heap 记录排在创建它的记录之后".

**Fix.** Either explain at L539 why multixact SLRU pages are exempt and what replaces the guarantee, in one sentence the reader can follow. Or cut the clause and leave a pointer to S07.M04. As written it undermines the reader's confidence in L329.

### 9. [LEAP, M] Parallel query sits in 2.1's "于是必须有" as if the process model forced it; "leader" is undefined; "禁止写" does not follow
- L147: "它们没有 leader 的地址空间……任何没被复制的全局变量都可能不一致，所以并行模式下禁止写数据、禁止分配事务号".

**Problem.** The process model does not require parallel query to exist. "leader" appears without a definition. "Some globals may differ" does not imply "no writes". The missing step only appears at L1038: "写入涉及的事务状态只能有一个归属".

**Fix.** Rewrite L147 as:
> 一旦要做并行查询（后来的功能），就只能复制选定的状态：发起查询的进程（leader）把……序列化给工作进程（worker）；写数据要分配事务号、改动只能有一个归属的事务状态，这些状态无法在进程间保持一份，所以并行模式下禁止写、禁止分配事务号。

### 10. [ORDER, M] Tradeoff rules T1, R6 and R7 are used as content long before section 10 defines them
- L291 "按 R7 在每个受支持平台上都有可用的实现".
- L338 "T1 允许用户按事务放松持久性……R6".
- L366, L369 "T1 记录的反例", L372 "T1 中让步方的补偿".

**Problem.** Section 1.3 only names T1 as the F1-versus-F4 conflict. Its ruling ("完整性不让步，持久性可以按事务放松") first appears at L1060.

**Fix.** In 1.3, give each T a one-clause ruling in parentheses. For example: "（[[T1]]：完整性不让步，持久性可由用户按事务放松，见 10.1）". Alternatively, replace "按 R7" at L291 with "按'默认路径必须在所有平台可用'（R7，见 10.2）".

### 11. [CONTRADICTION, M] The default I/O method appears to contradict itself
- L291: "2025 年加入了异步 I/O 的基础设施，默认的 I/O 方式改成后台 worker".
- L305: "直接 I/O 仍只是开发测试用的开关，缓冲 I/O 仍是默认".

**Problem.** To the reader, "the default is worker" and "the default is buffered" conflict. These are two different axes: who issues the I/O, and whether it goes through the kernel cache.

**Fix.** At L291:
> 异步 I/O 默认由后台 worker 进程代为发起；数据仍然经过内核页缓存（缓冲 I/O），直接 I/O 不是默认。

### 12. [STRUCTURE, M] Question 7.1-3 needs a fact that only 7.2 states
- Question at L878. Answer L1312: "重启后槽回到落盘的较旧的 restart_lsn".
- 7.1's slot item (L864) gives the rule "持久槽用已落盘的位置计算 WAL 保留界" but not the reason. The reason ("崩溃后槽回到落盘的值") and the term restart_lsn first appear at L896.

**Fix.** Append to L864:
> 槽在崩溃后回到上次落盘的位置（restart_lsn），所以保留界只能按落盘值算，否则崩溃后它需要的 WAL 可能已被删。

### 13. [STRUCTURE, M] The second half of question 5.3-3 cannot be answered from the text
- L696 asks "relcache 必须做到什么？". Answer L1286: "各后端最迟在下一事务开始时吸收，relcache 必须对所有可变索引（包括 TOAST 索引）重读这些标志（[[I4]].R12）". I4.R12 is never stated in 5.3.

**Fix.** Either add a 5.3 item:
> 弱锁下修改的索引标志：持锁者不会再收到失效，所以各后端最迟在下一事务开始时重读这些标志，覆盖所有索引（含 TOAST 索引）。

Or drop the second half of the question.

### 14. [LEAP, M] "标脏必须在写记录之前" gives no mechanism for the checkpoint race
- L330: "标脏必须在写记录之前，否则 checkpoint 可能恰好在两者之间开始而漏刷这个页".

**Problem.** The reader does not know how a checkpoint picks its redo point and which pages it flushes.

**Fix.** Add:
> checkpoint 先把 redo 点定在当前日志位置，再刷当时标脏的页；如果记录已写在 redo 点之前、页却还没标脏，这次 checkpoint 不刷它，恢复又从 redo 点之后开始、不重放这条记录，改动就丢了。

### 15. [LEAP/CONTRADICTION, M] 3.1 claims its choice forces WAL, but 3.2 shows an alternative, and 3.1's own assumption reverses the direction
- L294: "F1 要求的'崩溃后仍在'只能交给一份在提交时就落盘的记录".
- L324 says that before 7.1, all data pages were fsynced at commit, so that path existed.
- L308: "假设：自建缓存是为了让数据库自己控制页写出和 WAL 刷盘的先后". But the cache predates WAL.

**Fix.**
- At L294, change "只能" to "要不在提交时强刷所有被改过的数据页（7.1 版之前的做法，F4 不允许），就只能……".
- At L308, note that the assumption is anachronistic: the cache is older than WAL, so it can be today's reason but not the original one.

### 16. [CONTRADICTION, M] 8.1 lists protocol negotiation as derived, then says it is not derived
- L946 is a "于是必须有" item: "协议版本协商……协议要兼容到 3.0，新的协议特性只能以'可协商'的方式加进来".
- L948: "所以协议版本协商不是'按发布类别承诺'推出的，而是为了在这条单独的轴上……".
- The 选择 at L927 also bundles the protocol into the release-class decision.

**Fix.** Take "客户端协议向后兼容到 3.0" out of L927's 选择, or label it "（另一条轴，见末条）". Merge L946 into L948 as one item, headed "协议：不按发布类别的另一条轴".

### 17. [OVERLOAD, M] 3.2 来历 (L345) is a single paragraph of about 600 characters with about 15 dated events

**Fix.** Split it into the standard three bullets (原始原因 / 当前原因 / 假设), as 2.x and 5.x do. Move the 2001–2005 "UNDO gradually dropped" timeline into a short sub-list. The critical-section history lands here once (see issue 3).

### 18. [OVERLOAD/STRUCTURE, M] The section 6 preamble (L704) is about the map's gaps, and 6.1's equivalence-rule item lists details the preamble says 6.1 cannot derive
- L704: "规则表里标着'卡片未写'的有 R2、R5…… 只看决策和力的读者推出了八个不变量族里的七个…… 推不出这族规则的具体条目".
- L725 then lists R1, R2, R3, R5, R6 and R9 in detail.

**Fix.**
- Cut L704 down to: "6.1 只推出'必须有一族变换等价规则'；这族规则的具体条目要加上 6.3'相信声明的属性'才推得出来，所以具体条目放在 6.3 讲。"
- In L725, keep only the general rule and the outer-join null-marking and nullability examples (which do not depend on declarations). Move the opfamily, volatile and strict examples to 6.3, where L814–817 already repeat them.

### 19. [LEAP, M] Why the main shared-memory segment is fixed-size is answered with "how" instead of "why"
- L142: "大小定死的做法是先把所有模块的请求加总、一次分配，再逐个初始化".

**Fix.** Either give the reason (for example, growing it while keeping the same address in every process has no portable mechanism), if the map supports one. Or say plainly: "为什么不能运行中扩大，地图没有记录原因（见下一条的假设）。"

### 20. [STRUCTURE, M] Item marked "另有原因，不是进程模型推出的" sits inside 2.1's "于是必须有"
- L146: "信号处理器只置标志、处理延后（另有原因，不是进程模型推出的）".

**Problem.** It breaks the section's contract that each item follows from the choice.

**Fix.** Move it below the list as "旁注：另一条独立决策 [[D-signal-flag-latch]]……", or into 2.2 next to L201, where its consequence (interrupts deferred to safe points) is used.

### 21. [OVERLOAD, M] 2.1's "于是必须有" has 17 items with no grouping
L141–157.

**Fix.** Use three short sub-headings that the intro sentence at L139 already implies:
- 跨进程共享 (L141–145, L149)
- 并行查询 (L147–148)
- 崩溃与 postmaster (L150–157)

### 22. [UNEXPLAINED, M] Pointer-suffix notation is not covered by section 0
Examples: "Inv(1)(1b)(2)" (L142), "§4 P1" (L154), "Contract" (L369), "的 Secret" (L586), "C8" (L542, L1248), "[[X…]]" (first at L71 and L198), "commit:dc7420c2c92" (L475), "file:src/…:115" (L586).

**Fix.** Add one line to section 0:
> Inv(n) 指模块卡里第 n 条不变量，P 指协议，C 指检查单项，X 编号是 residuals.md 里记录的未解决问题；commit:/file: 是给读代码者的出处，可以略过。

### 23. [UNEXPLAINED, M] "volatile" carries two unrelated meanings
- L198: the C qualifier ("局部变量要声明为 volatile").
- L584 and L803: function volatility.

**Fix.** At L198 write "声明为 volatile（C 语言的一个修饰词，告诉编译器不要把它放在寄存器里；与 6.3 函数的'易变性'无关）". Or simply drop the clause, since it is a code-level detail.

### 24. [MISSING, M] It is never said why index entries carry no visibility information, though several mechanisms rest on that fact
- L437: "可见性信息只在 heap 的版本头里，不在索引项里〔file:doc/…〕".

**Problem.** The ordering rule and the visibility map (index-only scan) depend on this design fact, but the text does not say whether it is a separate decision, a consequence, or unknown.

**Fix.** Add one sentence: whether the map records it as a decision, and that this is why index-only scans need the visibility map (L439). If no rationale is known, say "地图没有记录原因".

### 25. [LEAP, M] pg_dump "must read from an MVCC snapshot, not from cache" relies on a 5.3 mechanism not yet introduced
- L628: "pg_dump 只持最弱的表锁，所以它必须从 MVCC 快照而不是从缓存读定义".

**Fix.** Add: "（缓存只在取得新锁时更新，见 5.3；弱锁挡不住 DDL，缓存可能过时）".

### 26. [OVERLOAD, M] The T7 counterexamples appear twice, almost word for word
L956 and L1079–1084.

**Fix.** Keep the full text in 10.1 and leave one sentence plus a pointer in 8.1. Also, the T7 statement at L1078 already includes "修复确实需要时 ABI 可以让步", so calling the three cases 反例 contradicts the rule's own wording. Either drop the clause from the rule, or call them "例外条件的实例".

### 27. [OVERLOAD, M] The standby transaction-state item is too dense
- L860: RUNNING_XACTS, KnownAssignedXids, pg_subtrans overflow, pruning, "备库的快照把全部在途号放在子事务列表里", and the promotion rule.

**Fix.** Keep one sentence: "主库定期把运行中事务列表写进 WAL，备库据此维护一份'已知在途'集合代替运行集合；集合就绪前不开放连接。" Move the rest to a pointer. Question 7.1-1 only needs that sentence.

### 28. [UNEXPLAINED, M] "进程信号屏障 / 吸收屏障" is used without explanation
- L893: "等所有进程吸收屏障".
- Question 7.2-2 at L912: "各后端经进程信号屏障得知".

**Fix.** At L893, add "（屏障：一种要求所有进程在下一个安全点确认收到某个全局变化的机制）".

### 29. [LEAP, L] "页正在写出时不能写（hint）" cites 3.3, but 3.3 says something different
- L433: "页正在写出时不能写（会破坏校验和，3.3）". 3.3 (L368) only says that the first hint change needs a full-page image.

**Fix.** Give the reason: "写出时按页内容算校验和，期间改动会让盘上的页与校验和不符". Or drop the clause.

### 30. [ORDER, L] "redo 点", "LSN" and "回收旧日志" are used in 3.1 before 3.2 defines them
L298 and L308.

**Fix.** At L298 add a gloss: "redo 点（恢复时重放的起点，3.2）".

### 31. [ORDER, L] "heap" and "元组" are used before they are defined
- "元组头" at L296; "heap" at L437 and L438.
- Defined at L425 ("每个版本（元组）") and L615 ("heap：PostgreSQL 存放表数据的基本格式").

**Fix.** Move the heap gloss to its first use in 4.1 (L425), and gloss 元组 at L296.

### 32. [ORDER, L] The horizon includes "复制槽和备库回报值" before replication is introduced
L435.

**Fix.** Add "（见 7.1）".

### 33. [LEAP, L] The cost item "反向扫描支持得不好；……子节点对不齐" is not tied to the pull model
L781.

**Fix.** Explain in one clause why each follows from the iterator or PlanState split, or cut the item.

### 34. [UNEXPLAINED, L] Exit code 1 counting as "clean" is not explained
- L152: "子进程的退出码不是 0 或 1……就算崩溃".

**Fix.** Add "（1 是报 FATAL 后正常退出的退出码）".

### 35. [UNEXPLAINED, L] Code identifiers a non-coder cannot decode
Examples: EvalPlanQual (L442), wrapLimit and xidStopLimit (L501), relminmxid and datminmxid (L541), FullTransactionId (L509), CommandCounterIncrement (L626), AccessExclusiveLock and RowExclusiveLock (L861), snapshotConflictHorizon and max_standby_* (L862), RUNNING_XACTS and KnownAssignedXids (L860), PG_TRY (L213).

**Fix.** Put the plain-language description first and the identifier in parentheses, or drop the identifier. For example: "拒绝分配的界限（xidStopLimit）".

### 36. [OVERLOAD, L] Code details that do not carry architectural reasoning and could become pointers
- L204: ErrorContext 8kB.
- L258: "扩展表文件时用的锁除外".
- L471: subtransaction overflow mechanics.
- L474: snapshot reuse counter.
- L475: "两条近似阈值".
- L816: COPY batching.
- L825: "未初始化的填充位".
- L892: command-id mapping.
- L1038: "匿名记录类型的编号……注册表".
- The file:line citations at L437, L537, L586, L813 and L1270.

**Fix.** Replace each with "（细节见 [[…]]）". None of them is needed to answer any question.

### 37. [STRUCTURE, L] The answer to 2.1-1 introduces DSM reference counting, which the text never states
- L1198: "它的生命周期要靠引用计数管理……（[[I5]].R15）".

**Fix.** Mark it as extra ("补充：") or delete it.

### 38. [STRUCTURE, L] The answer to 5.2-3 relies on a mechanism the text never states
- L1280: "之后它们之一执行 DDL、按旧缓存改写这一行时，会把原地写入的结果覆盖掉".

**Problem.** The reader is never told that DDL writes catalog rows from a cached copy.

**Fix.** Add that premise to the question, or add "（DDL 修改 catalog 行时以缓存中的副本为底稿）" to L629.

### 39. [STRUCTURE, L] Answers cite facts that are not in the text
- L1236: "WAL 段写失败会导致 PANIC".
- L1270: "通用 WAL 记录", "注册只允许在服务器启动加载库的阶段".
- L1310: "物理槽的 xmin 漏推进".
- L1338: "写整文件镜像".

**Fix.** Mark each as "补充（正文未述）" so the reader does not think they missed something.

### 40. [STRUCTURE, L] Question type labels (推导/反事实/风险) are inconsistent
Present in 3.x, 4.x, 6.3 and 7.x; absent in 1, 2.x, 5.x, 6.1, 6.2 and 8.1.

**Fix.** Add the labels everywhere. Section 0 (L53) promises three question types.

### 41. [MISSING, L] R1 and R4 are absent
Section 0 (L50) says principles R2–R9 exist, and 10.2 lists R2, R3, R5, R6, R7, R8 and R9.

**Fix.** Add one sentence in 10.2 saying where R4 went (and why numbering starts at R2).

### 42. [UNEXPLAINED, L] "确认/候选" status is used before it is defined
L742 ("R9，候选") is defined only at L1091.

**Fix.** Gloss it at L742, or add the definition to section 0's pointer list.

### 43. [MISSING, L] Security is excluded from the goals, yet RLS and leakproof appear as mechanisms
L71 versus L727, L803 and L818.

**Fix.** Add to L71: "所以后文出现的行级安全、leakproof 等只作为其他决策的约束出现，不单独推导。"

### 44. [MISSING, L] There is no compact dependency summary of the 19 decisions
Section 0 promises "按因果顺序", but the reader has to rebuild the graph themselves, and that graph is what they need to predict how a change propagates.

**Fix.** Add a 10–15 line list at the end of section 1 or the start of section 9. For example: "2.1 → 2.3, 5.1 缓存；2.2 → 临界区；4.1 + 3.2 → 9.1 …".

### 45. [UNEXPLAINED, L] Terminology for the same thing varies
- "在途 / 进行中 / 运行中 / 运行集合 / procarray".
- "事务状态日志 / clog / pg_xact".
- "hint bit / 提示位" (L1006).

**Fix.** Pick one term each, and note the synonym once where it is introduced.

### 46. [UNEXPLAINED, L] Minor undefined terms
- "条件变量" (L254).
- "共享 catalog" (L630): what is shared across databases is never said.
- "单用户模式例外" (L501).
- "aggressive vacuum" (L504).
- "libpq" (L948).
- "PREPARE / 两阶段提交" (L333).

**Fix.** Gloss each in a few words.

### 47. [ORDER, L] Full-page images are mentioned in E1 before they are explained
- L79: "允许关掉整页镜像".

**Fix.** Add "（见 3.3）".

### 48. [OVERLOAD, L] 5.1 and 5.2 explain the same bootstrap circularity twice
L586 and L631.

**Fix.** Keep it in one place and point to it from the other.

### 49. [UNEXPLAINED, L] "§4.1" in section 9 pointers reads like this document's section 4.1
L978 and L986: "§4.1 [[D-replication-slot]]".

**Fix.** Write "principles §4.1".

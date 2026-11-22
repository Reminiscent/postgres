# Log

基线：`4e0f8e930f3`（20devel，HEAD；工作树 HEAD 上另有两条 agent 基建提交）。地图构建于同一提交。

## 命令

```bash
grep -m1 AC_INIT configure.ac                 # PostgreSQL 20devel
git log -1 --format='%h %ad %s' --date=short  # 4e0f8e930f3
awk -F'|' '/^\| X[0-9]/ {print $4}' notes/arch-map/residuals.md | sort | uniq -c | sort -rn
wc -l notes/arch-map/*.md                     # 187 条 residual（已计数）
sed -n 1169,1232p notes/arch-map/derivation.md  # §11 四条松动前提 E3/E10/E11/E14
sed -n 19,104p notes/arch-map/principles.md     # E 前提表 + 力 F
sed -n 440,500p src/backend/catalog/namespace.c # X72：RangeVarGetRelidExtended 的注释
sed -n 570,620p src/backend/storage/sync/sync.c # X50：RegisterSyncRequest 的 XXX
sed -n 120,230p src/backend/statistics/relation_stats.c
grep -n 'vac_update_relstats' -A45 src/backend/commands/vacuum.c  # systable_inplace_update_begin
sed -n 6480,6530p src/backend/optimizer/util/clauses.c            # I6.R7 的已知缺口
git log -1 --format='%H %ad %s%n%b' 44fb59fc605                   # 该检查器的引入提交
grep -rn 'pgaio_io_start_read|StartReadBuffers' --include='*.c' src/backend
grep -rln 'read_stream_begin_relation' --include='*.c' src/backend contrib
grep -n 'read_stream' src/backend/access/heap/heapam.c            # 顺序扫描已转
grep -rn 'CURSOR_OPT_PARALLEL_OK' --include='*.c' src/backend src/include
grep -n 'CURSOR_OPT' src/backend/commands/portalcmds.c            # DECLARE CURSOR 不设该位
sed -n 418,430p src/backend/optimizer/plan/planner.c              # 并行门控
grep -n 'FULL JOIN|HJ_FILL_INNER' src/backend/executor/nodeHashjoin.c
```

## 检索（2026-11-22）

| 检索词 | 结果 |
|---|---|
| `PostgreSQL commitfest asynchronous I/O read stream index scan bitmap heap scan patch` | CF 4351 "Index Prefetching"，Next CF = PG20-3；[PGConf.EU 2026 演讲](https://www.postgresql.eu/events/pgconfeu2026/sessions/session/8163/slides/925/overview-index-prefetching-eu.pdf)；[PGConf.dev 2026](https://www.pgevents.ca/events/pgconfdev2026/sessions/session/659/slides/107/index%20(scan)%20prefetching%20_%20pgconf.dev%202026-6.pdf)；BitmapHeapScan 的 read stream 改造已提交（2025） |
| `pgsql-hackers qual pushdown past grouping expression_has_grouping_conflict wrong results follow-up` | [BUG #19797](https://www.postgresql.org/message-id/19797-7054aa017fa9d791%40postgresql.org)（2026-11，复合列 + nondeterministic collation）；["Unsafe qual pushdown through DISTINCT with simple CASE"](https://www.postgresql.org/message-id/CAMbWs4_6Yvqn8cSxGvYxp4c0yb4k2p-6Mb8_gCSrwYjNKoON%3D%40mail.gmail.com)（2026-08）；引入修复的提交 44fb59fc605（2026-07-06，回补 v18） |
| `pgsql-hackers "parallel" cursors "DECLARE CURSOR" portal parallelism patch discussion` | 2017 Vondra 实验补丁（[msg325039](https://www.mail-archive.com/pgsql-hackers@postgresql.org/msg325039.html)）；2024 帖子；[CF 6233](https://commitfest.postgresql.org/patch/6233) "Bypassing cursors in postgres_fdw"（PG20-3，活跃） |
| `commitfest "parallel hash" full join right join postgres patch status` | [CF 2903](https://commitfest.postgresql.org/patch/2903) "Parallel Hash Full Join"（2023-03 之后无新动静） |

## 未做

- 未起本地集群、未构建：本轮不产出 patch，无性能结论。
- 未逐条浏览最近 6 个 CommitFest 的全部条目；只核对了与候选直接相关的条目。

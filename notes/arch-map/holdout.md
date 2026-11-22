---
id: holdout
type: holdout
---
# 留出检验集（GUIDE §4.5.5 第 7 步）

用途：`principles.md` 的裁决检验（GUIDE §4.5.6）。这里列出的决策和讨论，**不得**作为任何原则 R 或取舍规则的支撑证据；
力和环境前提可以引用它们，但要标注 `holdout`。

## 选取方法

- **决策**：取 12 个子系统文件里全部 73 条一行式决策的 ID，按字典序排序，用 Python `random.Random(20261112).sample(ids, 18)` 抽取 18 条。
  抽取发生在任何归纳之前；基线是上游 `76c0df7381e`。
- **讨论**：留出决策所引提交里的 `Discussion:` 链接随决策一起留出；另从重构类长提交的候选清单中，
  对带 `Discussion:` 链接的条目用同一个种子抽取四分之一（见下文"留出的讨论"）。

## 留出的决策（18 条）

| 决策 | 所属 | 所引提交里的讨论 |
|---|---|---|
| [[D-extended-stats]] | [[S04]] | url:https://postgr.es/m/CADkLM=eErgzn7ECDpwFcptJKOk9SxZEk5Pot4d94eVTZsvj3gw@mail.gmail.com |
| [[D-fmgr-v1]] | [[S12]] | url:https://postgr.es/m/20161208213441.k3mbno4twhg2qf7g@alap3.anarazel.de |
| [[D-idx-am]] | [[S06]] | — |
| [[D-logical-decoding-on-wal]] | [[S10]] | — |
| [[D-lwlock-no-deadlock-detect]] | [[S01]] | 提交 ab5194e6f61 里的 message-id `20130926225545.GB26663@awork2.anarazel.de` |
| [[D-no-library-unload]] | [[S12]] | — |
| [[D-node-codegen]] | [[S12]] | url:https://www.postgresql.org/message-id/flat/c1097590-a6a4-486a-64b1-e1f9cc0533ce%40enterprisedb.com |
| [[D-own-buffer-cache]] | [[S08]] | url:https://postgr.es/m/CA+hUKGJkOiOCa+mag4BF+zHo7qo=o9CFheB8=g6uT5TUm2gkvA@mail.gmail.com |
| [[D-postmaster-no-shmem]] | [[S01]] | — |
| [[D-proto-whole-message]] | [[S02]] | — |
| [[D-relmapper]] | [[S11]] | — |
| [[D-rls-env-replan]] | [[S03]] | — |
| [[D-shmem-fixed-plus-dsm]] | [[S01]] | — |
| [[D-syncrep-primary-decides]] | [[S10]] | — |
| [[D-tablesync-per-table]] | [[S10]] | — |
| [[D-utility-late-analysis]] | [[S03]] | — |
| [[D-var-nullingrels]] | [[S04]] | url:https://postgr.es/m/830269.1656693747@sss.pgh.pa.us |
| [[D-xid-wraparound-freeze]] | [[S06]] | — |

## 留出的讨论

候选清单：`git log -i --grep='Restructure\|Refactor\|Introduce\|Rework\|Revamp' -- src/backend src/include` 共 3409 条，按正文行数取前 200 条，其中 158 条带 `Discussion:` 链接。
这些条目按 hash 排序后，用 `random.Random(20261112).sample(rows, 39)` 抽取。

| 提交 | 日期 | 标题 | 讨论 |
|---|---|---|---|
| commit:0ad8032910d | 2022-01-24 | Server-side gzip compression. | url:http://postgr.es/m/CA+Tgmoa-ST7fMLsVJduOB7Eub=2WjfpHS+QxHVEpUoinf4bOSg@mail.gmail.com |
| commit:0da92dc530c | 2022-02-10 | Logical decoding of sequences | url:https://postgr.es/m/d045f3c2-6cfb-06d3-5540-e63c320df8bc@enterprisedb.com |
| commit:247dea89f76 | 2024-09-10 | Introduce an RTE for the grouping step | url:https://postgr.es/m/CAMbWs4_dp7e7oTwaiZeBX8+P1rXw4ThkZxh1QG81rhu9Z47VsQ@mail.gmail.com |
| commit:294520c4448 | 2026-04-07 | instrumentation: Use Time-Stamp Counter on x86-64 to lower overhead | url:https://postgr.es/m/20200612232810.f46nbqkdhbutzqdg@alap3.anarazel.de |
| commit:2eb34ac3697 | 2020-01-16 | Fix problems with "read only query" checks, and refactor the code. | url:http://postgr.es/m/CA+TgmoZ_rLqJt5sYkvh+JpQnfX0Y+B2R+qfi820xNih6x-FQOQ@mail.gmail.com |
| commit:31966b151e6 | 2023-04-05 | bufmgr: Introduce infrastructure for faster relation extension | url:https://postgr.es/m/20221029025420.eplyow6k7tgu6he3@awork3.anarazel.de |
| commit:41efb834087 | 2020-09-27 | Move resolution of AlternativeSubPlan choices to the planner. | url:https://postgr.es/m/1992952.1592785225@sss.pgh.pa.us |
| commit:525392d5727 | 2025-02-20 | Don't lock partitions pruned by initial pruning | url:https://postgr.es/m/CA+HiwqFGkMSge6TgC9KQzde0ohpAycLQuV7ooitEEpbKB0O_mg@mail.gmail.com |
| commit:53c2a97a926 | 2024-02-28 | Improve performance of subsystems on top of SLRU | url:https://postgr.es/m/2BEC2B3F-9B61-4C1D-9FB5-5FAB0F05EF86@yandex-team.ru |
| commit:5c1560606dc | 2019-04-29 | Fix several recently introduced issues around handling new relation forks. | url:https://postgr.es/m/20190418011430.GA19133@paquier.xyz |
| commit:6719b238e8f | 2017-12-21 | Rearrange execution of PARAM_EXTERN Params for plpgsql's benefit. | url:https://postgr.es/m/32589.1513706441@sss.pgh.pa.us |
| commit:70b4f82a4b5 | 2018-06-18 | Prevent hard failures of standbys caused by recycled WAL segments | url:https://postgr.es/m/0A3221C70F24FB45833433255569204D1F8B57AD@G01JPEXMBYT05 |
| commit:74eafeab1a5 | 2026-03-27 | bufmgr: Improve StartBufferIO interface | url:https://postgr.es/m/zljergweqti7x67lg5ije2rzjusie37nslsnkjkkby4laqqbfw@3p3zu522yykv |
| commit:77291139c7c | 2018-08-05 | Remove support for tls-unique channel binding. | url:https://www.postgresql.org/message-id/ec787074-2305-c6f4-86aa-6902f98485a4%40iki.fi |
| commit:8e993bff532 | 2025-03-28 | Tidy up locale thread safety in ECPG library. | url:https://postgr.es/m/CWZBBRR6YA8D.8EHMDRGLCKCD%40neon.tech |
| commit:915b703e169 | 2016-06-17 | Fix handling of argument and result datatypes for partial aggregation. | url:<8229.1466109074@sss.pgh.pa.us> |
| commit:94bc27b5768 | 2020-10-28 | Centralize horizon determination for temp tables, fixing bug due to skew. | url:https://postgr.es/m/20201014203103.72oke6hqywcyhx7s@alap3.anarazel.de |
| commit:9556aa01c69 | 2019-01-25 | Use single-byte Boyer-Moore-Horspool search even with multibyte encodings. | url:https://www.postgresql.org/message-id/3173d989-bc1c-fc8a-3b69-f24246f73876%40iki.fi |
| commit:9bacec15b67 | 2021-11-02 | Don't overlook indexes during parallel VACUUM. | url:https://postgr.es/m/17245-ddf06aaf85735f36@postgresql.org |
| commit:9ff68679b50 | 2025-02-19 | Fix crash in brininsertcleanup during logical replication. | url:https://postgr.es/m/18815-2a0407cc7f40b327@postgresql.org |
| commit:a61b1f74823 | 2022-12-06 | Rework query relation permission checking | url:https://postgr.es/m/CA+HiwqGjJDmUhDSfv-U2qhKJjt9ST7Xh9JXC_irsAQ1TAUsJYg@mail.gmail.com |
| commit:ab7dbd681c5 | 2018-08-15 | Update FSM on WAL replay of page all-visible/frozen | url:https://postgr.es/m/20180802172857.5skoexsilnjvgruk@alvherre.pgsql |
| commit:acaa100f9a0 | 2026-07-28 | Fix logical decoding of empty prepared transactions. | url:https://postgr.es/m/19556-daa6d7ea65054d48@postgresql.org |
| commit:b7b27eb41a5 | 2026-04-03 | Optimize fast-path FK checks with batched index probes | url:https://postgr.es/m/CA+HiwqF4C0ws3cO+z5cLkPuvwnAwkSp7sfvgGj3yQ=Li6KNMqA@mail.gmail.com |
| commit:ba9a7e39217 | 2022-03-20 | Enforce foreign key correctly during cross-partition updates | url:https://postgr.es/m/CA+HiwqFvkBCmfwkQX_yBqv2Wz8ugUGiBDxum8=WvVbfU1TXaNg@mail.gmail.com |
| commit:baf17ad9dff | 2020-04-24 | Repair performance regression in information_schema.triggers view. | url:https://postgr.es/m/5891.1587594470@sss.pgh.pa.us |
| commit:bd3e3e9e568 | 2025-12-29 | Ensure sanity of hash-join costing when there are no MCV statistics. | url:https://postgr.es/m/2380165.1766871097@sss.pgh.pa.us |
| commit:d0d44049d12 | 2023-07-13 | Account for optimized MinMax aggregates during SS_finalize_plan. | url:https://postgr.es/m/2391880.1689025003@sss.pgh.pa.us |
| commit:d5b90cd6485 | 2019-09-17 | Fix bogus handling of XQuery regex option flags. | url:https://postgr.es/m/CAPpHfdvDci4iqNF9fhRkTqhe-5_8HmzeLt56drH%2B_Rv2rNRqfg@mail.gmail.com |
| commit:d6c08e29e7b | 2020-07-29 | Add hash_mem_multiplier GUC. | url:https://postgr.es/m/20200625203629.7m6yvut7eqblgmfo@alap3.anarazel.de |
| commit:da7226993fd | 2025-03-17 | aio: Add core asynchronous I/O infrastructure | url:https://postgr.es/m/uvrtrknj4kdytuboidbhwclo4gxhswwcpgadptsjvjqcluzmah%40brqs62irg4dt |
| commit:dc7c77f825d | 2025-12-16 | Fail recovery when missing redo checkpoint record without backup_label | url:https://postgr.es/m/20231023232145.cmqe73stvivsmlhs@awork3.anarazel.de |
| commit:ddce1da5b1b | 2026-09-15 | Add slot-based table AM index scan interface. | url:https://postgr.es/m/CAH2-WzmYqhacBH161peAWb5eF=Ja7CFAQ+0jSEMq=qnfLVTOOg@mail.gmail.com |
| commit:e41955faf06 | 2020-04-03 | Fix bugs in gin_fuzzy_search_limit processing. | url:https://postgr.es/m/CAEknJCdS-dE1Heddptm7ay2xTbSeADbkaQ8bU2AXRCVC2LdtKQ@mail.gmail.com |
| commit:e717a9a18b2 | 2021-04-07 | SQL-standard function body | url:https://www.postgresql.org/message-id/flat/1c11f1eb-f00c-43b7-799d-2d44132c02d7@2ndquadrant.com |
| commit:f2b88396955 | 2020-10-28 | Add pg_relation_check_pages() to check on-disk pages of a relation | url:https://postgr.es/m/CAOBaU_aVvMjQn=ge5qPiJOPMmOj5=ii3st5Q0Y+WuLML5sR17w@mail.gmail.com |
| commit:faeedbcefd4 | 2023-04-08 | Introduce PG_IO_ALIGN_SIZE and align all I/O buffers. | url:https://postgr.es/m/CA+hUKGK1X532hYqJ_MzFWt0n1zt8trz980D79WbjwnT-yYLZpg@mail.gmail.com |
| commit:fc55c7ff8d1 | 2023-06-28 | Ignore invalid indexes when enforcing index rules in ALTER TABLE ATTACH PARTITION | url:https://postgr.es/14987634-43c0-0cb3-e075-94d423607e08@gmail.com |
| commit:ffaa44cb559 | 2016-11-15 | Account for catalog snapshot in PGXACT->xmin updates. | url:<16447.1478818294@sss.pgh.pa.us> |

## 已知局限

- 建图者在抽取之前已经读过全部 73 条决策的一行式文本，所以"留出"保证的是这些条目不被用作原则和取舍规则的支撑，
  不能保证建图者对它们一无所知。裁决检验因此交给一个全新的、只读得到 `principles.md` 与 `L0.md` 的 agent 来做。
- `L0.md` 里出现了部分留出决策的名字和一句话结论。裁决检验使用一份抹去这些内容的 L0 副本，原文件不动。
- 留出决策里有若干条按扇出标准属于基础性决策。它们在裁决检验完成之后才写入 `principles.md`，并标注 `holdout`。

## 使用记录（2026-11-12）

- 裁决检验的 8 道历史题取自留出的决策；7 道新题里，N6 和 N7 取自留出的讨论（commit:ddce1da5b1b、commit:acaa100f9a0），其余 5 道取自读者模型训练截止日期之后的提交（不在上面的留出清单里）。结果见 `residuals.md` 的 [[X128]]–[[X136]]。
- 检验用的是不含留出条目的 `principles.md` 第一遍文本，以及抹去留出条目的 L0 副本。
- 检验之后，[[D-own-buffer-cache]]、[[D-xid-wraparound-freeze]]、[[D-logical-decoding-on-wal]] 作为基础性决策补进 `principles.md` §4，
  [[D-postmaster-no-shmem]]、[[D-shmem-fixed-plus-dsm]] 记入 §4.1，都带 `holdout` 标注，不支撑任何取舍规则或原则。
- 这些题目现在已经用过，其中三道还被用来修改了 T7、R2、R5，不能再当作留出材料。下一次检验要重新抽取。

---
name: pg-design-research
description: Use this skill for non-trivial PostgreSQL feature or bugfix design when external system precedent or research literature could materially improve the design or strengthen its acceptance case.
---

# pg-design-research

## Purpose

Raise design quality without confusing authority.

PostgreSQL local sources are authoritative for what fits PostgreSQL.
Other databases and research papers are supporting evidence that can:

- clarify the design space,
- reveal hidden tradeoffs,
- show user expectations,
- strengthen the RFC for contentious or novel work.

## When to use

Use this skill when one or more of these are true:

- the feature or bugfix is non-trivial and design-sensitive,
- semantics or user expectations are likely compared with other SQL engines,
- optimizer, execution, storage, or distributed-system tradeoffs matter,
- a performance or scalability claim needs stronger support,
- the RFC would be materially stronger with precedent or literature.

Do not use it for mechanical fixes, obvious local cleanups, or small bugfixes whose design is already fully determined by local PostgreSQL rationale.

## Research order

1. Start from PostgreSQL local constraints:
   - code,
   - tests,
   - official docs,
   - relevant mailing-list history, especially:
     - `pgsql-hackers` for design and acceptance discussion,
     - `pgsql-bugs` for real-world bug reports and reproductions,
     - `pgsql-performance` for workload and regression evidence,
     - `pgsql-committers` for landed outcomes, backpatch movement, and committed direction.
2. Then look for targeted external system precedents only in systems relevant to the design question:
   - SQL Server
   - Oracle
   - OceanBase
   - DuckDB
   - Snowflake
   - StarRocks
   - ClickHouse
3. Then look for a small number of relevant papers, preferably from:
   - SIGMOD
   - VLDB
   - ICDE
   - or similarly strong primary sources when those venues are not the best fit.

## Rules

- Do not treat external systems as authoritative for PostgreSQL.
- Do not collect a broad survey if only one or two systems are relevant.
- Do not cite papers for prestige alone; extract the mechanism, assumptions, and mismatches.
- Prefer primary sources: official docs, vendor technical material, and the paper itself.
- Return only research that changes the design discussion.
- Separate facts from inference explicitly.
- For external systems, capture both the user-visible behavior and the architectural mechanism when possible.
- For papers, capture the problem model, assumptions, and why those assumptions do or do not match PostgreSQL.
- The memo should tighten the RFC decision, not become a literature dump.
- The memo must end in a recommended direction, not a neutral survey.

## Output format

Return a **Design Evidence Memo** with:

### 1. Design question
- problem:
- current PostgreSQL constraint:
- decision to make:
- decision criteria:
  correctness, compatibility, operational simplicity, performance, portability, reviewability, and acceptance risk should be considered explicitly when relevant.

### 2. PostgreSQL-local evidence
- code and tests consulted:
- docs or wiki consulted:
- mailing-list threads consulted:
- current community direction:

### 3. External system precedents
For each relevant system:
- system:
- source:
- surface behavior:
- mechanism or implementation clue:
- why it is relevant:
- caveat or mismatch:

### 4. Research precedents
For each relevant paper:
- paper:
- source:
- mechanism or finding:
- problem model:
- assumptions:
- why it matters here:
- mismatch or adoption risk:

### 5. Synthesis
- what PostgreSQL should preserve:
- what PostgreSQL might borrow:
- what to avoid:
- what to justify explicitly in the RFC:
- likely reviewer objections:
- recommended direction and why:

### 6. Remaining uncertainty
- what is still unknown:
- what evidence would most reduce uncertainty:

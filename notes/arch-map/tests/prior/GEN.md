# Turning invariant rules into short knowledge questions

Repo: /Users/yanchengpeng/project/postgres-arch (read-only). Write only the output files named in your task message under /tmp/archmap/prior/q/.

Purpose: measure which rules a reader already knows WITHOUT any material. Each rule in an invariant view becomes one short question with an answer key. A different agent will answer the questions with no access to anything, and a grader will compare with your key.

Input: the files notes/arch-map/invariants/I<n>.md named in your task message (Chinese with English identifiers). Section "## 3. 规则" is a table; each row is one rule with an ID like `I3.R5`, the rule text, its owner, what other participants must do, and conditions/exceptions. Read the whole file so you understand each rule (sections 4, 6, 7 give context and examples), but make exactly one item per rule row of section 3.

For each rule write an item:
- `id`: the rule ID, e.g. "I3.R5".
- `question` (Chinese, English identifiers kept; 1–3 sentences): a concrete situation a PostgreSQL backend developer could be in, followed by an open question whose answer is the rule: "这时必须怎么做 / 会出什么问题 / 这样做行不行，为什么". Requirements:
  - self-contained: no rule IDs, no reference to "the map", "the checklist" or the invariant file;
  - it must not state or hint the answer, and must not be answerable by repeating the question;
  - not a bare yes/no question: always ask for the reason or the required action;
  - specific enough that a knowledgeable person would give the rule as the answer rather than some other true statement. If the rule is very specific, the situation must be specific too (name the operation, the lock, the phase, etc.) without naming the conclusion.
- `key` (Chinese, 1–2 sentences): the rule as the correct answer.
- `facts`: 2–3 short atomic statements that a correct answer must contain (the grader scores each). Take them only from the rule row (and its conditions column); do not add facts from your own knowledge.
- `wrong_belief` (optional, one sentence): the plausible wrong answer this rule guards against, if the file's sections 6/7 make one evident; otherwise null.

Write a JSON array per input file to /tmp/archmap/prior/q/I<n>.json. Validate with `python3 -I -c "import json,sys; d=json.load(open(sys.argv[1])); print(len(d))" <file>`; the count must equal the number of rule rows in section 3 of that file (`grep -c '^| I<n>\.R' notes/arch-map/invariants/I<n>.md`).

Self-check before finishing, for every item: the `question` must not contain any of its `facts` verbatim, and must not contain the phrases "规则", "不变量", "检查单".

Reply with: for each file, the path, the item count and the expected count; and the IDs of any rules you found too vague or too compound to turn into one good question (still include them, flag them here). Nothing else.

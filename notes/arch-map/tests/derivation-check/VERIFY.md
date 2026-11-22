# Fact-checking statements from a narrative about PostgreSQL's architecture

Read-only. Repo: /Users/yanchengpeng/project/postgres-arch (PostgreSQL source checkout; the architecture map is in notes/arch-map/). Do not edit anything in the repo. Write only the output file named in your task message.

Input: a file of statements sampled from notes/arch-map/derivation.md, a Chinese-language explanation of PostgreSQL's architecture written for a reader who does not read code. Each block has a header `### Cx-nn  derivation.md:<line>  §<section> <part>` and the statement. Statements carry pointers like `[[I3]].R9` (rule R9 in notes/arch-map/invariants/I3.md §3), `[[S06.M04]]` (module card `### S06.M04` in notes/arch-map/subsystems/6.md), `[[D-…]]` (decision record `### D-…` in notes/arch-map/principles.md), `〔file:…〕`, `commit:…`.

For each statement:
1. Split it into its factual claims (usually 1–4). Ignore pure framing ("这一节讲…").
2. For each claim, find the basis: first the map entry it points to (open the rule row, card Inv, or decision record), then the primary source the map entry cites (`sed -n` the file lines; `git show -s --format='%h %ad %s%n%b' --date=short <hash>` for commits). If the statement has no pointer, search the map for the relevant entry (grep) and then the source.
3. Verdict per claim:
   - CORRECT: the primary source supports it, with the same conditions.
   - OVERSTATED: the source supports a weaker or more conditional claim; say which condition or qualifier is missing.
   - WRONG: the source contradicts it; quote the contradicting text.
   - MAP-ONLY: the map says it, but you could not find support in the primary source (say what you checked).
   - UNSUPPORTED: neither the map nor the source supports it.
   - HISTORY-UNVERIFIED: a historical claim (dates, who said what, mailing-list content) that you could not check against a commit or a fetched message; do not use the web unless the statement cites a URL, and then at most one fetch per claim.
   Also note CLARITY problems separately: a term used without explanation, an explanation that is technically wrong even if the conclusion is right, or wording a non-coder would likely misread.
4. Decide only from sources, not from your general knowledge. The repository's code is the authority for current behaviour; note when the map and the code disagree.

Output file format:

    ## Cx-nn  derivation.md:<line>
    - (1) <claim in a few words> — VERDICT — evidence (file:line / commit, ≤3 lines quoted) — fix suggestion if not CORRECT
    - (2) …
    - CLARITY: <issue or "none">

End with a summary table: counts per verdict, and the list of blocks with any non-CORRECT verdict.

Reply with the summary table and every non-CORRECT bullet (with block id and line). Nothing else.

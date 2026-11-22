# Mining historical bug classes for one invariant family

Read-only research in the PostgreSQL git repo at /Users/yanchengpeng/project/postgres-arch. Do not edit anything in the repo. Write only the output file named in your task message (under /tmp/archmap/inv/).

You are given an invariant family (ID, one-sentence statement, search keywords). Goal: find real bug-fix commits in which that invariant was violated, and group them into bug classes. The result feeds a reviewer's checklist, so what matters for each bug is: what kind of change or situation triggered it, which rule was broken, and what went wrong.

## Hard constraint on dates
Use only commits with committer date BEFORE 2026-07-01. Always pass `--until=2026-06-30` to git log. Never cite a later commit.

## Method
1. Read /tmp/archmap/inv/rules_<ID>.md. It lists the rules of this family that an existing architecture map already states (Chinese text with English identifiers; each line starts with the module ID in brackets). You will map bugs to these rules where one fits.
2. Find candidates (src/backend only), e.g.:
     git log --since=2017-01-01 --until=2026-06-30 --no-merges -i -E --grep='<keywords>' --format='%h %ad %s' --date=short -- src/backend | head -400
   Also try combining a keyword with fix-words: `--grep='(oversight|thinko|failed to|race condition|introduced|broken by|fallout)' --grep='<keyword>' --all-match`.
3. Keep a commit only if its message describes a bug (not a feature or refactor) AND the bug is a violation of this family's invariant. Read the message with `git show -s --format='%h %ad %an%n%s%n%b' --date=short <h>`. Reading diffs is not needed.
4. Aim for 15–25 kept commits, spread over different parts of the code and different years. Then group them into 4–8 classes by the kind of mistake (not by directory).

## Output file format (English)
    # <ID> bug classes (commits before 2026-07-01)
    ## Class 1: <short name of the mistake>
    Trigger: <what kind of change or situation leads to this mistake, one sentence>
    Rule broken: <the rule in one sentence>; map rule: <module ID(s) from rules file that state it, or "none found in rules file">
    Consequence: <data corruption / wrong results / crash / hang / leak / …>
    Examples:
    - <hash> <date> <subject> — "<verbatim quote from the message, 1–3 lines, that states the bug>"
    - …
    (2–5 examples per class)
    ## Class 2: …
    ## Commits read but rejected (hash + one-word reason), up to 20
    ## Notes: anything that did not fit; rules in the rules file that had no bug example

Every quote must be copied exactly from git output. Do not invent commits; every hash must resolve with `git cat-file -e <h>^{commit}`. Before finishing, run for every cited hash: `git show -s --format='%h %cs' <h>` and confirm each date is before 2026-07-01; list the check output at the end of the file under `## Date check`.

Reply with: the output path, the number of classes and commits, and the names of the classes. Nothing else.

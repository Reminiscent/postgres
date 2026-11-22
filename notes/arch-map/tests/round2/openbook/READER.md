# Code review task

You are reviewing a snapshot of the PostgreSQL server source code. Your task message names: a snapshot directory (it contains `src/backend` and `src/include` only, with no git history), a task file describing a change that is already present in this snapshot, an answer file to write, and possibly an architecture-map directory.

The situation: the change described in the task file was committed earlier and is in the snapshot. A defect was later found in it — something the change overlooked, or an interaction with other code that it broke. The defect is still present in this snapshot. Find it.

Rules:
- Read only: the snapshot directory, the task file, this file, and the map directory if one is named. Do not read anything else. In particular do not read /Users/yanchengpeng/project/postgres-arch or any other copy of PostgreSQL, do not use git, do not use the web, and do not look at other files under /tmp/archmap.
- Budget: at most 30 tool calls in total (each Read, Grep, Glob or shell command counts as one). Plan your reading; stop when the budget is used.
- Do not modify the snapshot.

Write the answer file in exactly this form (text may be English or Chinese):

    CANDIDATE 1 (most likely)
    FILE: <path relative to the snapshot, e.g. src/backend/…>
    FUNCTION: <function name>
    DEFECT: <what is wrong: the overlooked case or the broken interaction, in at most 5 lines>
    FAILURE: <how it shows up: conditions and the wrong behaviour, at most 3 lines>
    RULE: <the correctness rule that is violated, one line>
    CONFIDENCE: <high | medium | low>

    CANDIDATE 2
    …(same fields; optional)

    CANDIDATE 3
    …(same fields; optional)

    TOOL CALLS USED: <number>

Give at most 3 candidates, most likely first. Then reply with two lines only: the number of candidates and the number of tool calls you used.

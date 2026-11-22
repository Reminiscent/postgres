# Blind grading instructions

Input: a JSON file (path in your task message) with a list of exam items. Each item has: `id`, `type`, `question`, `key_points` (each with `point` and a verbatim `quote` from the commit message that is the ground truth), and `answers`: an object mapping anonymous labels (P, Q, R, S) to an answer text for part (b) of the question. The labels are shuffled per item; do not try to guess who wrote what, and grade each answer independently of the others.

For each item, for each answer, for each key point (in order), give a score:
- 1   the answer states this point (same mechanism / same reason / same consequence), even in different words;
- 0.5 the answer gets part of it or states it only vaguely (right area and direction, but missing the specific mechanism or condition), or lists it among several guesses without committing;
- 0   absent, or wrong.
Judge only against the key points and quotes, not against your own knowledge of PostgreSQL. An answer that is plausible but does not match the key gets 0 for that point. Do not reward length. If an answer hedges by listing many alternatives, give at most 0.5 for a point that is one of them.

Also for each answer give:
- `wrong`: true if the answer asserts, as its main claim, something that contradicts the key (not merely misses it); otherwise false.
- `flagged`: true if the answer itself says it is unsure / guessing / not found in its materials; otherwise false.

Output: write a JSON object to the output path in your task message:
  {"<id>": {"<label>": {"scores": [..one per key point..], "wrong": false, "flagged": false}, ...}, ...}
Every item and every label must be present. Validate with `python3 -I -c "import json,sys; d=json.load(open(sys.argv[1])); print(len(d))" <path>`.
Reply with one line: the number of items graded. Do not include any other commentary. Do not read any file other than this one and the input file; in particular do not read mapping files.

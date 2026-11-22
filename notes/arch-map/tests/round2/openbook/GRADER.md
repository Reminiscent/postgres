# Blind grading: code review answers

Input: /tmp/archmap/ob/packet.json — a list of items. Each item has `id`, `question` (a change that was committed earlier), `actual_fix_files` (the files the real follow-up fix modified), `key_points` (each with `point` and a verbatim `quote` from the real fix's commit message; together they describe the real defect), and `answers`: two anonymous answers labelled P and Q. Each answer lists up to three candidate defects (file, function, defect, failure, rule, confidence), most likely first. Labels are shuffled per item; grade each answer independently.

For each item and each answer:
1. `match_rank`: the number (1, 2 or 3) of the first candidate that identifies the REAL defect — the same overlooked case or broken interaction as described by the key points, in the right place in the code (same function or the same code path in the actual fix files). Use 0 if no candidate does. A candidate that names the right file but describes a different problem does not match. A candidate that describes the right problem in general terms but locates it in the wrong code path counts as a match only if the mechanism it describes is the one in the key points.
2. `scores`: for each key point, in order: 1 if some candidate in the answer states it; 0.5 if some candidate states it partially or vaguely; 0 otherwise. Judge only against the key points and quotes, not against your own knowledge.
3. `top_conf`: the confidence the answer gives to its candidate 1 (high, medium or low).
4. `wrong_confident`: true if candidate 1 is given high or medium confidence and does NOT match the real defect.

Write a JSON object to /tmp/archmap/ob/grades.json:
  {"<id>": {"P": {"match_rank": 0, "scores": [...], "top_conf": "low", "wrong_confident": false}, "Q": {...}}, ...}
Validate it with `python3 -I -c "import json; print(len(json.load(open('/tmp/archmap/ob/grades.json'))))"`. Read no file other than this one and the input file. Reply with one line: the number of items graded.

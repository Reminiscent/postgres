# Blind grading of short answers

Input: the JSON file named in your task message: a list of items. Each has `id`, `question`, `key` (the correct answer), `facts` (2–3 atomic statements a correct answer must contain), `wrong_belief` (may be null) and `answers`: anonymous answers labelled P, Q, R (labels shuffled per item). Grade each answer independently, only against `key` and `facts`, not against your own knowledge.

For each answer give:
- `scores`: one number per fact, in order: 1 if the answer states that fact (any wording); 0.5 if it states it partially, vaguely, or only as one of several alternatives; 0 if absent or contradicted.
- `opposite`: true if the answer's main claim contradicts the key (for example it says the thing is allowed when the key says it is not, or names the wrong required action as the right one); otherwise false. An answer that merely misses the point is not `opposite`.

Write a JSON object to the output file named in your task message: {"<id>": {"<label>": {"scores": [...], "opposite": false}, ...}, ...}. Every item and label must be present. Validate with `python3 -I -c "import json,sys; print(len(json.load(open(sys.argv[1]))))" <file>`. Read no other files. Reply with one line: the number of items graded.

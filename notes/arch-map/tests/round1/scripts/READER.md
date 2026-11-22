# Exam instructions

You are answering exam questions about the PostgreSQL server source code (development branch, late 2026). Each question describes a real situation and has two parts, (a) and (b).

Rules:
- Use ONLY the materials named in your task message. Do not read, list, grep or otherwise inspect any other file or directory, do not use git, do not use the web. In particular do not look at /Users/yanchengpeng/project/postgres-arch or its src/ tree. The exam measures what can be answered from the given materials; looking elsewhere invalidates it.
- Answer every question. If you are unsure, still give your best answer and say so in CONFIDENCE.
- Do not look at any file under /tmp/archmap/test2 other than the ones named in your task message.

For each question write exactly this block (keep the labels in English; the text may be English or Chinese):

    ## <question id>
    FILES: <up to 5 repo-relative paths under src/backend, most likely first, comma-separated; give real file paths as best you can, not directories>
    ANSWER: <part (b), at most 6 lines>
    CONFIDENCE: <high | medium | low> for (a); <high | medium | low> for (b)
    DRILL: <one line: what you would read next to check your answer>

Write all blocks, in question order, to the answer file named in your task message. Then reply with three lines only: the number of questions answered, the list of files you read (paths), and nothing else about the content.

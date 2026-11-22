import json,sys,os
for b in (1,2,3,4):
    p=f'/tmp/archmap/test2/q/batch{b}.json'
    if not os.path.exists(p): print('missing',b); continue
    d=json.load(open(p))
    os.makedirs('/tmp/archmap/test2/exam',exist_ok=True)
    with open(f'/tmp/archmap/test2/exam/Q{b}.md','w') as o:
        o.write(f'# Questions, set {b} ({len(d)} questions)\n\n')
        for it in d: o.write(f"## {it['id']}\n{it['question'].strip()}\n\n")
    print(b,len(d),[it['type'] for it in d].count('RISK'),'RISK')

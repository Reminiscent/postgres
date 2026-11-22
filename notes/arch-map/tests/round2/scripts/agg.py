import json,os,collections
T='/tmp/archmap/test3'; conds=['A','C1','C2','C3']; W={'RISK':3,'WHY':2}
rows={(r['id'],r['cond']):r for r in json.load(open(f'{T}/grade/rows_a.json'))}
DUP=set()
recs=[]
for b in (1,2,3,4):
    gp=f'{T}/grade/grades{b}.json'
    if not os.path.exists(gp): continue
    G=json.load(open(gp)); M=json.load(open(f'{T}/grade/mapping{b}.json'))
    for qid,labs in G.items():
        if qid in DUP: continue
        for lab,g in labs.items():
            c=M[qid][lab]; r=rows[(qid,c)]
            s=sum(g['scores'])/len(g['scores']) if g['scores'] else 0
            recs.append(dict(id=qid,cond=c,type=r['type'],sub=r['sub'],b=s,a=r['hit5'],top1=r['top1'],wrong=g['wrong'],flag=g['flagged'],cb=r['cb'],ca=r['ca']))
json.dump(recs,open(f'{T}/grade/recs.json','w'))
def mean(x): return sum(x)/len(x) if x else float('nan')
n=len({r['id'] for r in recs}); print('items',n)
print('cond   (a)hit@5  (b)all  RISK   WHY    ALT   weighted  wrong  wrong&conf>=med  flagged')
for c in conds:
    rs=[r for r in recs if r['cond']==c]
    if not rs: continue
    bt={t:mean([r['b'] for r in rs if r['type']==t]) for t in W}
    wsum=sum(1*r['a']+W[r['type']]*r['b'] for r in rs)/sum(1+W[r['type']] for r in rs)
    wr=sum(r['wrong'] for r in rs); wc=sum(1 for r in rs if r['wrong'] and r['cb'] in('high','medium'))
    print(f"{c:5}  {mean([r['a'] for r in rs]):.2f}      {mean([r['b'] for r in rs]):.2f}    {bt['RISK']:.2f}   {bt['WHY']:.2f}     -     {wsum:.2f}      {wr:3}    {wc:3}              {sum(r['flag'] for r in rs)}")
print('per item (b):')
ids=sorted({r['id'] for r in recs})
for i in ids:
    d={r['cond']:r for r in recs if r['id']==i}
    print(i,d['A']['type'],' '.join(f"{c}={d[c]['b']:.2f}{'!' if d[c]['wrong'] else ''}" for c in conds if c in d))
print('n by type',collections.Counter(r['type'] for r in recs if r['cond']=='A'))

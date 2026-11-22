# usage: python3 -I score.py  -> per-condition part (a) metrics; builds blind grading packets for part (b)
import json,re,os,random,glob,collections
T='/tmp/archmap/test3'
own={}
for n in range(1,13):
    for l in open(f'/tmp/archmap/own/{n}.txt'):
        own['src/backend/'+l.strip()]=n
def parse(path):
    out={}
    if not os.path.exists(path): return out
    cur=None
    for l in open(path,encoding='utf-8'):
        m=re.match(r'##\s*(R\d-\d\d)',l.strip())
        if m: cur=m.group(1); out[cur]={'files':[],'answer':'','conf':'','drill':''}; fld=None; continue
        if cur is None: continue
        s=l.strip()
        if s.startswith('FILES:'):
            fs=[x.strip().strip('`') for x in re.split(r'[,;]',s[6:]) if x.strip()]
            fs=[re.sub(r'\s*\(.*$','',f) for f in fs]
            out[cur]['files']=[f if f.startswith('src/') else 'src/backend/'+f for f in fs][:5]; fld=None
        elif s.startswith('ANSWER:'): out[cur]['answer']=s[7:].strip(); fld='answer'
        elif s.startswith('CONFIDENCE:'): out[cur]['conf']=s[11:].strip().lower(); fld=None
        elif s.startswith('DRILL:'): out[cur]['drill']=s[6:].strip(); fld=None
        elif fld=='answer' and s: out[cur]['answer']+='\n'+s
    return out
def conf(c):
    m=re.findall(r'(high|medium|low)',c)
    return (m+['?','?'])[:2]
conds=['A','C1','C2','C3']
res=collections.defaultdict(list); rows=[]
for b in (1,2,3,4):
    qp=f'{T}/q/batch{b}.json'
    if not os.path.exists(qp): continue
    Q=json.load(open(qp)); ans={c:parse(f'{T}/ans/{c}_{b}.md') for c in conds}
    have=[c for c in conds if ans[c]]
    if not have: continue
    r=random.Random(1000+b); packet=[]; mapping={}
    for it in Q:
        key=[f for f in it['key_files'] if f.startswith('src/backend/')]
        kd={os.path.dirname(f) for f in key}; ks={own.get(f) for f in key}
        labs=list('PQRS')[:len(have)]; r.shuffle(labs); mapping[it['id']]=dict(zip(labs,have))
        entry={'id':it['id'],'type':it['type'],'question':it['question'],'key_points':it['key_points'],'answers':{}}
        for lab,c in sorted(zip(labs,have)):
            a=ans[c].get(it['id'],{'files':[],'answer':'(no answer)','conf':'','drill':''})
            entry['answers'][lab]=a['answer']
            fs=a['files']; hit=[f for f in fs if f in key]
            ca,cb=conf(a['conf'])
            row=dict(id=it['id'],type=it['type'],sub=it['subsystem'],cond=c,nkey=len(key),
                     top1=int(bool(fs) and fs[0] in key), hit5=int(bool(hit)),
                     recall=len(set(hit))/min(len(key),5) if key else 0,
                     dir5=int(any(os.path.dirname(f) in kd for f in fs)),
                     sub1=int(bool(fs) and own.get(fs[0]) in ks),
                     exist=sum(1 for f in fs if os.path.exists('/Users/yanchengpeng/project/postgres-arch/'+f)), nfs=len(fs), ca=ca, cb=cb)
            rows.append(row)
        packet.append(entry)
    json.dump(packet,open(f'{T}/grade/packet{b}.json','w'),ensure_ascii=False,indent=1)
    json.dump(mapping,open(f'{T}/grade/mapping{b}.json','w'))
json.dump(rows,open(f'{T}/grade/rows_a.json','w'))
def agg(rs,k): return sum(r[k] for r in rs)/len(rs) if rs else 0
print('part (a) location, by condition')
print('cond  n  top1  hit@5  recall@5  dir@5  subsys@1  real-path%  highconf-wrong')
for c in conds:
    rs=[r for r in rows if r['cond']==c]
    if not rs: continue
    hw=sum(1 for r in rs if r['ca']=='high' and not r['hit5'])
    print(f"{c:4} {len(rs):3} {agg(rs,'top1'):.2f}  {agg(rs,'hit5'):.2f}   {agg(rs,'recall'):.2f}      {agg(rs,'dir5'):.2f}   {agg(rs,'sub1'):.2f}      {sum(r['exist'] for r in rs)/max(1,sum(r['nfs'] for r in rs)):.2f}        {hw}")
print('by batch (hit@5):')
for b in (1,2,3,4):
    print(b,{c:round(agg([r for r in rows if r['cond']==c and r['id'].startswith(f'R{b}')],'hit5'),2) for c in conds})

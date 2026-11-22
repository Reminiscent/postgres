# usage: build.py exam | packet
import json,sys,os,re,random
T='/tmp/archmap/prior'; B={1:['I1','I8'],2:['I2','I7'],3:['I3','I6'],4:['I4','I5']}; tiers=['haiku','sonnet','opus']
def items(b):
    out=[]
    for f in B[b]: out+=json.load(open(f'{T}/q/{f}.json'))
    return out
def parse(p):
    out={}; cur=None; fld=None
    if not os.path.exists(p): return out
    for l in open(p,encoding='utf-8'):
        m=re.match(r'##\s*(I\d\.R\d+)',l.strip())
        if m: cur=m.group(1); out[cur]={'a':'','c':'?'}; fld=None; continue
        if cur is None: continue
        s=l.strip()
        if s.startswith('ANSWER:'): out[cur]['a']=s[7:].strip(); fld='a'
        elif s.startswith('CONFIDENCE:'):
            mm=re.search(r'(high|medium|low)',s.lower()); out[cur]['c']=mm.group(1) if mm else '?'; fld=None
        elif fld=='a' and s: out[cur]['a']+=' '+s
    return out
if sys.argv[1]=='exam':
    for b in B:
        it=items(b)
        with open(f'{T}/exam/E{b}.md','w') as o:
            o.write(f'# Quiz {b} ({len(it)} questions)\n\n')
            for x in it: o.write(f"## {x['id']}\n{x['question'].strip()}\n\n")
        print(b,len(it))
else:
    for b in B:
        it=items(b); ans={t:parse(f'{T}/ans/{t}_{b}.md') for t in tiers}
        r=random.Random(500+b); pk=[]; mp={}; conf={}
        for x in it:
            labs=['P','Q','R']; r.shuffle(labs); mp[x['id']]=dict(zip(labs,tiers))
            pk.append({'id':x['id'],'question':x['question'],'key':x['key'],'facts':x['facts'],'wrong_belief':x.get('wrong_belief'),
                       'answers':{l:ans[t].get(x['id'],{'a':'(no answer)'})['a'] for l,t in sorted(zip(labs,tiers))}})
            conf[x['id']]={t:ans[t].get(x['id'],{'c':'?'})['c'] for t in tiers}
        json.dump(pk,open(f'{T}/grade/packet{b}.json','w'),ensure_ascii=False,indent=1)
        json.dump(mp,open(f'{T}/grade/mapping{b}.json','w')); json.dump(conf,open(f'{T}/grade/conf{b}.json','w'))
        print(b,len(pk),{t:len(ans[t]) for t in tiers})

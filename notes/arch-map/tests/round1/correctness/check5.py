import re, os, subprocess, glob, sys
M='notes/arch-map/'
def rd(p): return open(p,encoding='utf-8').read()
def lines(p): return rd(p).split('\n')
P=lines(M+'principles.md'); PT='\n'.join(P)
subs=sorted(glob.glob(M+'subsystems/*.md'), key=lambda x:int(re.search(r'(\d+)\.md',x).group(1)))
def hdr(s): print('\n=== '+s+' ===')
def res(n,f):
    for x in f: print(x)
    print('FAIL %d'%len(f) if f else 'PASS')
    return bool(f)
status={}
# C1
hdr('C1 anchors in principles.md')
f=[]; nf=0; nc=0; nbare=0
LN=r'\d+(?:-\d+)?(?:,\d+(?:-\d+)?)*'
cache={}
def nlines(p):
    if p not in cache:
        cache[p]=len(open(p,encoding='utf-8',errors='replace').read().split('\n'))-(1 if open(p,'rb').read().endswith(b'\n') else 0)
    return cache[p]
for i,l in enumerate(P,1):
    for m in re.finditer(r'file:([^\s:;）〕|,]+):('+LN+')(?=[;）〕 |]|$)',l):
        nf+=1; path,ls=m.group(1),m.group(2)
        if not os.path.isfile(path): f.append('%d: missing file %s'%(i,path)); continue
        n=nlines(path)
        for part in ls.split(','):
            for v in part.split('-'):
                if int(v)>n: f.append('%d: %s:%s line %s > %d'%(i,path,ls,v,n)); break
    nbare+=len(re.findall(r'file:',l))
    for m in re.finditer(r'commit:([0-9a-fA-F]{7,40})',l):
        nc+=1
        r=subprocess.run(['git','cat-file','-e',m.group(1)+'^{commit}'],capture_output=True)
        if r.returncode: f.append('%d: bad commit %s'%(i,m.group(1)))
print('file anchors matched: %d; all "file:" occurrences: %d; commit anchors: %d'%(nf,nbare,nc))
status['C1']=res(1,f)
# C2
mods=set(l.split('|')[0].strip() for l in lines('/tmp/archmap/MODULES.txt') if '|' in l)
dh=set(re.findall(r'^### (D-[a-z0-9-]+)',PT,re.M))
dl=set()
for s in subs: dl|=set(re.findall(r'^- (D-[a-z0-9-]+) \{',rd(s),re.M))
Erows=set(re.findall(r'^\| (E\d+) \|',PT,re.M))
Rs=set(re.findall(r'\*\*(R\d+)',PT))
Xs=set(re.findall(r'^\| (X\d+) \|',rd(M+'residuals.md'),re.M))
def resolve(x):
    if x.startswith('D-'): return x in dh or x in dl
    if re.fullmatch(r'S\d\d\.M\d\d',x): return x in mods
    if re.fullmatch(r'S\d\d',x): return 1<=int(x[1:])<=12
    if re.fullmatch(r'I[1-8]|F[1-7]',x): return True
    if x[0]=='E' and x[1:].isdigit(): return x in Erows
    if x[0]=='R' and x[1:].isdigit(): return x in Rs
    if x[0]=='X' and x[1:].isdigit(): return x in Xs
    return False
hdr('C2 references (all files)')
tot=0; fails=[]
files=[M+'principles.md',M+'L0.md',M+'residuals.md']+subs
for fn in files:
    cnt=0;ff=[]
    for i,l in enumerate(lines(fn),1):
        for m in re.finditer(r'\[\[([^\]]+)\]\]',l):
            cnt+=1
            if not resolve(m.group(1)): ff.append('%s:%d: unresolved [[%s]]'%(fn,i,m.group(1)))
    print('%s: %d refs, %d unresolved'%(fn,cnt,len(ff))); fails+=ff
status['C2']=res(2,fails)
# C3
hdr('C3 completeness of 推出')
tl=[l for l in P if l.startswith('- 推出：')]
txt='\n'.join(tl)
need=['I%d'%i for i in range(1,9)]+['S%02d'%i for i in range(1,13)]
miss=[n for n in need if not re.search(r'\[\[%s(?:\.M\d\d)?\]\]'%n,txt)]
print('推出 lines: %d'%len(tl))
status['C3']=res(3,['missing '+m for m in miss])
# sections
secs={}; cur=None
for l in P:
    m=re.match(r'### (D-[a-z0-9-]+)',l)
    if m: cur=m.group(1); secs[cur]=[]; continue
    if re.match(r'#+ ',l): cur=None; continue
    if cur: secs[cur].append(l)
hdr('C4 record fields')
print('D sections: %d'%len(secs))
flds=['- 力：','- 原始原因：','- 当前原因：','- 推出：','- 下游代价：','- 重新审视的条件：','- 扇出：','- 证据：']
f=[]
for d,ls in secs.items():
    for fl in flds:
        if not any(x.startswith(fl) for x in ls): f.append('%s: missing %s'%(d,fl))
    li=[x for x in ls if x.startswith('- 力：')]
    if li and '前提：' not in li[0]: f.append('%s: 力 line lacks 前提：'%d)
status['C4']=res(4,f)
hdr('C5 premises E vs 力')
f=[]; erow={}
for l in P:
    m=re.match(r'\| (E\d+) \|',l)
    if m: erow[m.group(1)]=set(re.findall(r'\[\[(D-[^\]]+)\]\]',l.split('|')[3]))
print('E rows: %d'%len(erow))
for e,ds in erow.items():
    for d in ds:
        if d not in secs: f.append('%s lists %s: no such section'%(e,d)); continue
        li=[x for x in secs[d] if x.startswith('- 力：')]
        if not li or '[[%s]]'%e not in li[0]: f.append('%s lists %s but its 力 line does not mention [[%s]]'%(e,d,e))
for d,ls in secs.items():
    li=[x for x in ls if x.startswith('- 力：')]
    if li:
        for e in set(re.findall(r'\[\[(E\d+)\]\]',li[0])):
            if d not in erow.get(e,()): f.append('%s 力 mentions [[%s]] but E row does not list it'%(d,e))
status['C5']=res(5,f)
hdr('C6 holdout hygiene')
H=lines(M+'holdout.md'); hid=[]; intab=False
for l in H:
    if l.startswith('| 决策'): intab=True; continue
    if intab:
        m=re.match(r'\| \[\[(D-[^\]]+)\]\]',l)
        if m: hid.append(m.group(1))
        elif not l.startswith('|---'): break
print('held-out IDs: %d'%len(hid))
f=[]
for i,l in enumerate(P,1):
    if l.startswith('- 支撑：') or l.startswith('- 支撑决策：'):
        for h in hid:
            if '[[%s]]'%h in l: f.append('%d: 支撑 line contains held-out %s'%(i,h))
cursec=''
occ=[]
for i,l in enumerate(P,1):
    if l.startswith('### '): cursec=l
    for h in hid:
        if h in l:
            occ.append('  line %d %s: holdout on line/heading: %s'%(i,h,'yes' if ('holdout' in l or 'holdout' in cursec) else 'no'))
print('occurrences of held-out IDs in principles.md:'); print('\n'.join(occ) if occ else '  (none)')
print('ids present: %s'%sorted(set(h for h in hid if h in PT)))
status['C6']=res(6,f)
hdr('C7 principles R blocks')
f=[]; blocks=[]; cur=None
for l in P:
    if l.startswith('**R'): cur=[l]; blocks.append(cur); continue
    if cur is not None:
        if l.strip()=='' : cur=None
        else: cur.append(l)
for b in blocks:
    rid=re.match(r'\*\*(R\d+)',b[0]).group(1)
    sl=[x for x in b if x.startswith('- 支撑决策：')]
    n=len(set(re.findall(r'\[\[(D-[^\]]+)\]\]',sl[0]))) if sl else 0
    pr=any(x.startswith('- 预测：') for x in b)
    st=''
    for x in b:
        m=re.search(r'状态：(.*)',x)
        if m: st=m.group(1)
    print('%s: supporting D=%d; 预测=%s; 状态=%s'%(rid,n,'yes' if pr else 'NO',st))
    if not sl: f.append('%s: no 支撑决策 line'%rid)
    if not pr: f.append('%s: no 预测 line'%rid)
    if st.startswith('确认') and n<2: f.append('%s: 确认 with <2 supporting decisions (%d)'%(rid,n))
status['C7']=res(7,f)
hdr('C8 subsystem reference lines')
refs=[]
for s in subs:
    for l in lines(s):
        m=re.match(r'^- (D-[a-z0-9-]+) \{.*完整记录见 principles.md',l)
        if m: refs.append((s,m.group(1),l))
print('reference lines: %d (expect 15)'%len(refs))
f=[]
if len(refs)!=15: f.append('count %d != 15'%len(refs))
for s,d,l in refs:
    if d not in secs: f.append('%s: %s has no ### heading in principles.md'%(s,d))
for s in subs:
    print('%s: grep -c ^- D- = %d'%(s,sum(1 for l in lines(s) if l.startswith('- D-'))))
status['C8']=res(8,f)
hdr('C9 F tags')
f=[]
for s,d,l in refs:
    a=set(re.findall(r'\[\[(F\d)\]\]',re.search(r'\{(.*?)\}',l).group(1)))
    li=[x for x in secs.get(d,[]) if x.startswith('- 力：')]
    if not li: f.append('%s: no 力 line'%d); continue
    b=set(re.findall(r'\[\[(F\d)\]\]',li[0].split('前提：')[0]))
    if a!=b: f.append('%s (%s): subsystem %s vs principles %s'%(d,s,sorted(a),sorted(b)))
status['C9']=res(9,f)
hdr('C10 sizes')
for n in ['principles','holdout','L0','residuals']:
    p=M+n+'.md'; print('%s.md: %d lines (wc -l), %d bytes'%(n,rd(p).count('\n'),os.path.getsize(p)))
print('`| E` rows: %d'%len(re.findall(r'^\| E',PT,re.M)))
print('### D- headings: %d'%len(re.findall(r'^### D-',PT,re.M)))
print('**T blocks: %d'%len(re.findall(r'^\*\*T\d+',PT,re.M)))
print('**R blocks: %d'%len(re.findall(r'^\*\*R\d+',PT,re.M)))
for k in ['未考证','假设','（转述']: print('%s: %d'%(k,PT.count(k)))
print('\nSUMMARY: '+', '.join('%s=%s'%(k,'FAIL' if v else 'PASS') for k,v in status.items()))

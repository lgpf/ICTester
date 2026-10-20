from parse import parse
from sim import check_vectors, gen
from models import *
import os
DB=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','..','IC_Tester_v3','data','chipdb.txt')
db={c['num']:c for c in parse(DB)}
def val(num,chip,n=None):
    c=db[num]; e=check_vectors(c['pins'],chip,c['vec']); print('valid',num,'OK' if not e else e[:4]); return not e

# ---------- 74168/74169 ----------
class UD168(Chip):
    def __init__(s,mod): s.mod=mod;Chip.__init__(s)
    def init(s): s.q=None
    def logic(s,lv):
        if s.rise(lv,2):
            if lv[9]==0: s.q=bits(lv,[3,4,5,6])
            elif lv[7]==0 and lv[10]==0 and s.q is not None:
                if lv[1]==1: s.q=(s.q+1)%s.mod if s.q<s.mod else 0
                else: s.q=(s.q-1)%s.mod if s.q<s.mod else s.q-1
        o={};setbits(o,[14,13,12,11],s.q)
        if s.q is None or lv[1] is None or lv[10] is None: o[15]=None
        else:
            t=(s.q==s.mod-1) if lv[1]==1 else (s.q==0)
            o[15]=0 if (t and lv[10]==0) else 1
        return o
val('74168',UD168(10))

class DFF175(Chip):
    D=[4,5,12,13];Q=[2,7,10,15];QN=[3,6,11,14]
    def __init__(s,en=False): s.en=en;Chip.__init__(s)
    def init(s): s.q=None
    def logic(s,lv):
        if not s.en and lv[1]==0: s.q=0
        elif s.rise(lv,9) and (not s.en or lv[1]==0): s.q=bits(lv,s.D)
        o={};setbits(o,s.Q,s.q);setbits(o,s.QN,None if s.q is None else (~s.q)&15);return o
val('74175',DFF175())
class DFF174(Chip):
    D=[3,4,6,11,13,14];Q=[2,5,7,10,12,15]
    def __init__(s,en=False): s.en=en;Chip.__init__(s)
    def init(s): s.q=None
    def logic(s,lv):
        if not s.en and lv[1]==0: s.q=0
        elif s.rise(lv,9) and (not s.en or lv[1]==0): s.q=bits(lv,s.D)
        o={};setbits(o,s.Q,s.q);return o
val('74174',DFF174())

NEW=[]   # (num, nome, pins, nomes_pinos, vetores)
def add(num,name,n,names,vecs):
    names=names.split(); assert len(names)==n,(num,len(names))
    for v in vecs: assert len(v)==n
    NEW.append((num,name,n,names,vecs))

def clk(st,p,pre=None):
    """gera dois passos: clock=0 e clock=1 (bordas rapidas via digitalWrite)"""
    a=dict(st);a[p]=0;b=dict(st);b[p]=1;return [a,b]

# ---------- 74169 ----------
steps=[]
base={1:1,3:0,4:0,5:0,6:0,7:0,9:1,10:0}
def load(v,ud=1):
    s=dict(base);s[1]=ud;s[9]=0
    for k,p in enumerate([3,4,5,6]): s[p]=(v>>k)&1
    return clk(s,2)
steps+=load(0)
for _ in range(17): steps+=clk(base,2)
steps+=load(9)
d=dict(base);d[1]=0
for _ in range(11): steps+=clk(d,2)
h=dict(base);h[7]=1                         # ENP inativo: segura
steps+=clk(h,2)+clk(h,2)
h=dict(base);h[10]=1
steps+=clk(h,2)
for s in steps: s[15]='-'
v=gen(16,UD168(16),steps,[16],[8])
add('74169','4-bit Sync Bin U/D Counter',16,'U/D CLK A B C D ~ENP GND ~LOAD ~ENT QD QC QB QA ~RCO VCC',v)

# ---------- 74377 / 74378 / 74379 ----------
class R377(Chip):
    D=[3,4,7,8,13,14,17,18];Q=[2,5,6,9,12,15,16,19]
    def init(s): s.q=None
    def logic(s,lv):
        if s.rise(lv,11) and lv[1]==0: s.q=bits(lv,s.D)
        o={};setbits(o,s.Q,s.q);return o
def regsteps(D,clkp,enp,width,pats):
    st=[]
    for k,pv in enumerate(pats):
        s={enp:0}
        for i,p in enumerate(D): s[p]=(pv>>i)&1
        st+=clk(s,clkp)
        if k%2:                       # com enable alto nao deve carregar
            s2=dict(s);s2[enp]=1
            for i,p in enumerate(D): s2[p]=1-((pv>>i)&1)
            st+=clk(s2,clkp)
    return st
p8=[0x00,0xFF,0x55,0xAA,0x0F,0xF0,0x33,0xCC,0x01,0x80]
add('74377','Octal D FF w/ Enable',20,'~G 1Q 1D 2D 2Q 3Q 3D 4D 4Q GND CLK 5Q 5D 6D 6Q 7Q 7D 8D 8Q VCC',
    gen(20,R377(),regsteps(R377.D,11,1,8,p8),[20],[10]))
p6=[0,63,21,42,7,56,1,32]
add('74378','Hex D Flip-Flop w/ Enable',16,'~G 1Q 1D 2D 2Q 3D 3Q GND CLK 4Q 4D 5Q 5D 6D 6Q VCC',
    gen(16,DFF174(en=True),regsteps(DFF174.D,9,1,6,p6),[16],[8]))
p4=[0,15,5,10,3,12,1,8]
add('74379','Quad D Flip-Flop w/ Enable',16,'~G 1Q ~1Q 1D 2D ~2Q 2Q GND CLK 3Q ~3Q 3D 4D ~4Q 4Q VCC',
    gen(16,DFF175(en=True),regsteps(DFF175.D,9,1,4,p4),[16],[8]))

# ---------- 74594 ----------
class R594(Chip):
    def init(s): s.sr=None; s.st=None
    def logic(s,lv):
        if lv[10]==0: s.sr=0
        elif s.rise(lv,11): s.sr=None if s.sr is None else ((s.sr<<1)|lv[14])&0xff
        if lv[13]==0: s.st=0
        elif s.rise(lv,12): s.st=s.sr
        o={};setbits(o,[15,1,2,3,4,5,6,7],s.st)
        o[9]=None if s.sr is None else (s.sr>>7)&1
        return o
st=[{10:0,11:0,12:0,13:0,14:0}, {10:0,11:0,12:1,13:0,14:0}]
cur={10:1,11:0,12:0,13:1,14:0}
for b in [1,0,1,1,0,0,1,0,1,1]:
    s=dict(cur);s[14]=b;st+=clk(s,11)
    s=dict(s);s[11]=0;st+=clk(s,12)
st.append({10:1,11:0,12:0,13:0,14:0})            # limpa registro de saida
st.append({10:0,11:0,12:0,13:1,14:0})            # limpa registro de deslocamento
st+=clk({10:1,11:0,12:0,13:1,14:0},12)
add('74594','8-bit Shift Reg, Out Latch',16,'QB QC QD QE QF QG QH GND QH\' ~SRCLR SRCLK RCLK ~RCLR SER QA VCC',
    gen(16,R594(),st,[16],[8]))

# ---------- 7483 ----------
def f83(lv):
    A=bits(lv,[10,8,3,1]);B=bits(lv,[11,7,4,16]);c=lv[13]
    o={}
    if None in (A,B,c): return {p:None for p in [9,6,2,15,14]}
    s=A+B+c; setbits(o,[9,6,2,15],s&15); o[14]=s>>4; return o
st=[]
import random; random.seed(83)
cases=[(a,b,c) for a in range(16) for b in [0,15,5,10,a,(~a)&15] for c in (0,1)]
cases=sorted(set(cases))
for a,b,c in cases:
    s={13:c}
    for k,p in enumerate([10,8,3,1]): s[p]=(a>>k)&1
    for k,p in enumerate([11,7,4,16]): s[p]=(b>>k)&1
    st.append(s)
add('7483','4-bit Binary Full Adder',16,'A4 S3 A3 B3 VCC S2 B2 A2 S1 A1 B1 GND C0 C4 S4 B4',gen(16,Comb(f83),st,[5],[12]))

# ---------- 7491 ----------
class S91(Chip):
    def init(s): s.sr=[None]*8
    def logic(s,lv):
        if s.rise(lv,9): s.sr=[AND(lv[11],lv[12])]+s.sr[:7]
        return {13:s.sr[7],14:N(s.sr[7])}
st=[]
nc={p:0 for p in [1,2,3,4,6,7,8]}
seq=[(1,1),(0,1),(1,0),(1,1),(0,0),(1,1),(1,1),(0,1),(1,1),(1,0),(1,1),(1,1),(0,0),(0,1),(1,1),(1,1),(1,1),(0,1)]
for a,b in seq[:8]:
    s=dict(nc);s.update({11:a,12:b,13:'-',14:'-'});st+=clk(s,9)
for a,b in seq[8:]+[(0,0)]*8:
    s=dict(nc);s.update({11:a,12:b});st+=clk(s,9)
add('7491','8-bit Serial Shift Reg',14,'NC NC NC NC VCC NC NC NC CLK GND A B QH ~QH',gen(14,S91(),st,[5],[10]))

# ---------- 74121 (teste estatico) ----------
st=[]
for a1,a2,b in [(1,1,1),(0,1,1),(1,0,1),(0,0,1),(1,1,0),(0,0,0),(1,1,1),(0,1,0)]:
    st.append({2:0,8:0,12:0,13:0,3:a1,4:a2,5:b,9:'-',10:'-',11:'-'})
add('74121','Monostable (static test)',14,'~Q NC A1 A2 B Q GND NC RINT CEXT RCEXT NC NC VCC',
    gen(14,Comb(lambda lv:{1:1,6:0}),st,[14],[7]))

# ---------- 4050 ----------
add('4050','Hex Non-Inverting Buffer',16,'VDD Y1 A1 Y2 A2 Y3 A3 VSS A4 Y4 A5 Y5 NC A6 Y6 NC',
    gen(16,gates([(3,2),(5,4),(7,6),(9,10),(11,12),(14,15)],lambda a:a),
        [ {3:a,5:b,7:c,9:d,11:e,14:f,13:0,16:0} for a,b,c,d,e,f in
          [(0,0,0,0,0,0),(1,1,1,1,1,1),(1,0,1,0,1,0),(0,1,0,1,0,1),(1,1,0,0,1,1),(0,0,1,1,0,0)]],[1],[8]))

# ---------- 4516 ----------
class C4510(Chip):
    def __init__(s,mod=10): s.mod=mod;Chip.__init__(s)
    def init(s): s.q=None
    def logic(s,lv):
        if lv[9]==1: s.q=0
        elif lv[1]==1: s.q=bits(lv,[4,12,13,3])
        elif s.rise(lv,15) and lv[5]==0 and s.q is not None:
            s.q=(s.q+(1 if lv[10]==1 else -1))%s.mod
        o={};setbits(o,[6,11,14,2],s.q)
        if s.q is None: o[7]=None
        else:
            t=(s.q==s.mod-1) if lv[10]==1 else (s.q==0)
            o[7]=0 if (t and lv[5]==0) else 1
        return o
val('4510',C4510(10))
b={1:0,3:0,4:0,5:0,9:0,10:1,12:0,13:0}
st=[{**b,**{9:1,15:0}}]
for _ in range(18): st+=clk(b,15)
st.append({**b,**{1:1,4:1,12:0,13:1,3:0,15:0}})          # carrega 5
d={**b,**{10:0}}
for _ in range(8): st+=clk(d,15)
st+=clk({**b,**{5:1}},15)                                  # CI alto: segura
add('4516','4-bit Binary U/D Counter',16,'PE Q4 P4 P1 ~CI Q1 ~CO VSS RESET U/D Q2 P2 P3 Q3 CLK VDD',
    gen(16,C4510(16),st,[16],[8]))

# ---------- 2114 SRAM 1Kx4 ----------
A=[5,6,7,4,3,2,1,17,16,15]; IO=[14,13,12,11]
class S2114(Chip):
    def init(s): s.m={}
    def logic(s,lv):
        a=bits(lv,A)
        if lv[8]==0 and lv[10]==0:
            if a is not None: s.m[a]=bits(lv,IO)
            return {p:'Z' for p in IO}
        if lv[8]==0 and lv[10]==1:
            v=s.m.get(a) if a is not None else None
            o={};setbits(o,IO,v);return o
        return {p:'Z' for p in IO}
addrs=[0]+[1<<k for k in range(10)]+[0x3FF,0x155,0x2AA]
def wr(a,d):
    s={8:0}
    for k,p in enumerate(A): s[p]=(a>>k)&1
    for k,p in enumerate(IO): s[p]=(d>>k)&1
    return [{**s,**{10:1}},{**s,**{10:0}},{**s,**{10:1}}]
def rd(a,cs=0):
    s={8:cs,10:1}
    for k,p in enumerate(A): s[p]=(a>>k)&1
    return [s]
st=[]
for pat in (lambda i:(i*5+3)&15, lambda i:(~(i*5+3))&15):
    for i,a in enumerate(addrs): st+=wr(a,pat(i))
    for i,a in enumerate(addrs): st+=rd(a)
st+=rd(0,cs=1)
add('2114','1Kx4 Static RAM',18,'A6 A5 A4 A3 A0 A1 A2 ~CS GND ~WE IO4 IO3 IO2 IO1 A9 A8 A7 VCC',gen(18,S2114(),st,[18],[9]))

# auto-verificacao: todo vetor gerado reproduz no simulador
import sys
print(len(NEW),'novos modelados')

# ================= v3: analogicos/interface que funcionam como digitais =================
def cmpout(p,m,oc=True):
    if None in (p,m) or p==m: return None
    return ('Z' if oc else 1) if p>m else 0
def comparators(spec,oc=True):
    # spec: (in+, in-, out)
    return Comb(lambda lv:{o:cmpout(lv[p],lv[m],oc) for p,m,o in spec})
def cmp_steps(spec,extra={}):
    st=[]
    for combo in range(1<<len(spec)):
        for flip in (0,1):
            s=dict(extra)
            for k,(p,m,o) in enumerate(spec):
                bit=((combo>>k)&1)^flip if k%2 else ((combo>>k)&1)
                s[p]=bit; s[m]=1-bit
            st.append(s)
    return st
s393=[(3,2,1),(5,6,7)]
add('393','LM393 Dual Comparator',8,'OUT1 IN1- IN1+ GND IN2+ IN2- OUT2 VCC',gen(8,comparators(s393),cmp_steps(s393),[8],[4]))
s339=[(5,4,2),(7,6,1),(9,8,14),(11,10,13)]
add('339','LM339 Quad Comparator',14,'OUT2 OUT1 VCC IN1- IN1+ IN2- IN2+ IN3- IN3+ IN4- IN4+ GND OUT4 OUT3',gen(14,comparators(s339),cmp_steps(s339),[3],[12]))
add('358','LM358 Dual OpAmp (comp)',8,'OUT1 IN1- IN1+ GND IN2+ IN2- OUT2 VCC',gen(8,comparators(s393,False),cmp_steps(s393),[8],[4]))
s324=[(3,2,1),(5,6,7),(10,9,8),(12,13,14)]
add('324','LM324 Quad OpAmp (comp)',14,'OUT1 IN1- IN1+ VCC IN2+ IN2- OUT2 OUT3 IN3- IN3+ GND IN4+ IN4- OUT4',gen(14,comparators(s324,False),cmp_steps(s324),[4],[11]))

class T555(Chip):
    def __init__(s,units): s.u=units;Chip.__init__(s)
    def init(s): s.q=[None]*len(s.u)
    def logic(s,lv):
        o={}
        for k,(trg,thr,rst,out,dis) in enumerate(s.u):
            if lv[rst]==0: s.q[k]=0
            elif lv[trg]==0 and lv[thr]==1: s.q[k]=None        # ambos ativos: evitar
            elif lv[trg]==0: s.q[k]=1
            elif lv[thr]==1: s.q[k]=0
            q=s.q[k]; o[out]=q; o[dis]=None if q is None else ('Z' if q else 0)
        return o
seq555=[(1,0,0),(1,0,1),(0,0,1),(1,0,1),(1,1,1),(1,0,1),(0,0,1),(0,0,0),(0,0,1),(1,0,1),(1,0,0),(1,1,0),(1,1,1),(1,0,1)]  # trig,thr,reset
def steps555(units,extra):
    st=[]
    for k in range(len(seq555)):
        s=dict(extra)
        for u,(trg,thr,rst,out,dis) in enumerate(units):
            t,h,r=seq555[(k+u*3)%len(seq555)]
            s[trg]=t;s[thr]=h;s[rst]=r
        st.append(s)
    return st
u555=[(2,6,4,3,7)]
add('555','NE555 Timer (static)',8,'GND TRIG OUT ~RST CTRL THR DIS VCC',gen(8,T555(u555),steps555(u555,{5:'-'}),[8],[1]))
u556=[(6,2,4,5,1),(8,12,10,9,13)]
add('556','NE556 Dual Timer (static)',14,'DIS1 THR1 CV1 ~RST1 OUT1 TRIG1 GND TRIG2 OUT2 ~RST2 CV2 THR2 DIS2 VCC',
    gen(14,T555(u556),steps555(u556,{3:'-',11:'-'}),[14],[7]))

class X176(Chip):
    def logic(s,lv):
        o={}
        if lv[3]==1: o[6]=lv[4]; o[7]=N(lv[4])
        else: o[6]='Z'; o[7]='Z'
        a=lv[6] if o[6]=='Z' else o[6]; b=lv[7] if o[7]=='Z' else o[7]
        if lv[2]==1: o[1]='Z'
        else: o[1]=None if (a is None or b is None or a==b) else a
        return o
st=[]
for re_,de,di,a in [(0,1,1,None),(0,1,0,None),(1,1,1,None),(0,0,0,1),(0,0,0,0),(1,0,0,1),(0,0,1,0),(0,1,1,None),(0,1,0,None)]:
    s={2:re_,3:de,4:di}
    if a is not None: s[6]=a; s[7]=1-a
    st.append(s)
v176=gen(8,X176(),st,[8],[5])
add('75176','RS-485 Transceiver',8,'RO ~RE DE DI GND A B VCC',v176)
add('485','MAX485 (=75176)',8,'RO ~RE DE DI GND A B VCC',v176)

def inv_spec(spec): return Comb(lambda lv:{o:N(lv[i]) for i,o in spec})
s1489=[(1,3),(4,6),(10,8),(13,11)]
st=[]
for v in range(16):
    s={2:'-',5:'-',9:'-',12:'-'}
    for k,(i,o) in enumerate(s1489): s[i]=(v>>k)&1
    st.append(s)
v1489=gen(14,inv_spec(s1489),st,[14],[7])
add('1489','MC1489 RS-232 Receiver',14,'INA RSPA OUTA INB RSPB OUTB GND OUTC RSPC INC OUTD RSPD IND VCC',v1489)
add('75189','SN75189 (=1489)',14,'INA RSPA OUTA INB RSPB OUTB GND OUTC RSPC INC OUTD RSPD IND VCC',v1489)

s232=[(13,12),(8,9)]
st=[]
for v in range(4):
    s={1:'-',2:'-',3:'-',4:'-',5:'-',6:'-',7:'-',14:'-',10:0,11:0}
    for k,(i,o) in enumerate(s232): s[i]=(v>>k)&1
    st.append(s)
v232=gen(16,inv_spec(s232),st,[16],[15])
n232='C1+ V+ C1- C2+ C2- V- T2OUT R2IN R2OUT T2IN T1IN R1OUT R1IN T1OUT GND VCC'
add('232','MAX232 (receptores)',16,n232,v232)
add('202','MAX202 (receptores)',16,n232,v232)

def darl(ins,outs):
    return Comb(lambda lv:{o:(0 if lv[i]==1 else ('Z' if lv[i]==0 else None)) for i,o in zip(ins,outs)})
def walk(ins,extra):
    st=[]
    pats=[0,(1<<len(ins))-1]+[1<<k for k in range(len(ins))]+[0x55&((1<<len(ins))-1),0xAA&((1<<len(ins))-1)]
    for pv in pats:
        s=dict(extra)
        for k,p in enumerate(ins): s[p]=(pv>>k)&1
        st.append(s)
    return st
i2003=[1,2,3,4,5,6,7]; o2003=[16,15,14,13,12,11,10]
add('2003','ULN2003 Darlington x7',16,'IN1 IN2 IN3 IN4 IN5 IN6 IN7 GND COM OUT7 OUT6 OUT5 OUT4 OUT3 OUT2 OUT1',
    gen(16,darl(i2003,o2003),walk(i2003,{9:'-'}),[],[8]))
i2803=[1,2,3,4,5,6,7,8]; o2803=[18,17,16,15,14,13,12,11]
add('2803','ULN2803 Darlington x8',18,'IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 GND COM OUT8 OUT7 OUT6 OUT5 OUT4 OUT3 OUT2 OUT1',
    gen(18,darl(i2803,o2803),walk(i2803,{10:'-'}),[],[9]))

fn=[('75451','AND',lambda a,b:AND(a,b)),('75452','NAND',lambda a,b:NAND(a,b)),
    ('75453','OR',lambda a,b:OR(a,b)),('75454','NOR',lambda a,b:N(OR(a,b)))]
for num,nm,f in fn:
    def mk(f=f): return Comb(lambda lv:{3:('Z' if f(lv[1],lv[2])==1 else 0) if f(lv[1],lv[2]) is not None else None,
                                     5:('Z' if f(lv[6],lv[7])==1 else 0) if f(lv[6],lv[7]) is not None else None})
    st=[{1:a,2:b,6:c,7:d} for a in (0,1) for b in (0,1) for c,d in [(b,a),(1-a,b)]]
    add(num,f'Dual Periph Driver {nm}',8,'1A 1B 1Y GND 2Y 2A 2B VCC',gen(8,mk(),st,[8],[4]))
for a,src in [('2901','339'),('2902','324'),('2903','393'),('2904','358')]:
    x=[y for y in NEW if y[0]==src][0]
    add(a,x[1].replace('LM3','LM'+a+'=') if False else 'LM'+a+' (=LM'+src+')',x[2],' '.join(x[3]),x[4])

for num,name,n,names,vecs in NEW:
    und=[k for k,v in enumerate(vecs) if '-' in v and k>3]
    print(f"{num:6} {n}p {len(vecs):3} vetores  linhas c/ '-' apos inicio: {und[:6]}")

# python3 newchips.py > novos.txt  -> blocos prontos para colar no chipdb.txt
if __name__=='__main__':
    import sys
    for num,name,n,names,vecs in NEW:
        print('$'+num); print(name); print(n)
        for x in names: print(x)
        print('#')
        for v in vecs: print(v)

# Simulador fiel ao testCase() do IC_Tester v2.6
# Modelo: chip.step(lv) recebe niveis (pino->0/1/None) e devolve saidas (pino->0/1/'Z'/None)
class Sim:
    def __init__(s, n, chip):
        s.n=n; s.chip=chip
        s.drive={p:None for p in range(1,n+1)}   # None = pino solto c/ pullup
        s.out={}
    def level(s,p):
        if s.drive[p] is not None: return s.drive[p]
        o=s.out.get(p,'Z')
        return 1 if o=='Z' else o
    def settle(s):
        for _ in range(8):
            lv={p:s.level(p) for p in s.drive}
            new=s.chip.step(lv)
            if new==s.out: return
            s.out=new
        raise Exception("nao estabiliza")
    def set(s,p,v):
        if s.drive[p]!=v:
            s.drive[p]=v; s.settle()
    def line(s,code):
        assert len(code)==s.n
        for i,c in enumerate(code,1):
            if c=='V': s.set(i,1)
            elif c=='G': s.set(i,0)
            elif c in 'LHZ-': s.set(i,None)
        for i,c in enumerate(code,1):
            if c in 'X0C': s.set(i,0)
            elif c=='1': s.set(i,1)
        for i,c in enumerate(code,1):
            if c=='C': s.set(i,None); s.set(i,0)
        return {p:s.level(p) for p in s.drive}

def check_vectors(n,chip,vecs):
    s=Sim(n,chip); errs=[]
    for k,v in enumerate(vecs):
        r=s.line(v)
        for i,c in enumerate(v,1):
            if c in 'LH':
                e=0 if c=='L' else 1
                if r[i]!=e: errs.append((k,i,c,r[i]))
    return errs

def gen(n,chip,steps,vcc,gnd):
    """steps: lista de dict pino->0/1/'C'; pinos ausentes (exceto alim.) sao lidos"""
    s=Sim(n,chip); out=[]
    for st in steps:
        code=[]
        for p in range(1,n+1):
            if p in vcc: code.append('V')
            elif p in gnd: code.append('G')
            elif p in st: code.append({0:'0',1:'1','C':'C','X':'X','-':'-'}[st[p]])
            else: code.append('?')
        code=''.join(code)
        r=s.line(code.replace('?','L'))
        fin=''
        for i,c in enumerate(code,1):
            if c=='?':
                fin+='-' if r[i] is None else 'LH'[r[i]]
            else: fin+=c
        out.append(fin)
    return out

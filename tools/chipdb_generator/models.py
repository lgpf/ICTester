def N(x): return None if x is None else 1-x
def AND(*a):
    if 0 in a: return 0
    return None if None in a else 1
def OR(*a):
    if 1 in a: return 1
    return None if None in a else 0
def XOR(a,b): return None if None in (a,b) else a^b
def bits(lv,pins):
    v=0
    for k,p in enumerate(pins):
        if lv[p] is None: return None
        v|=lv[p]<<k
    return v
def setbits(out,pins,val,width=None):
    for k,p in enumerate(pins):
        out[p]=None if val is None else (val>>k)&1

class Chip:
    def __init__(s): s.prev={}; s.init()
    def init(s): pass
    def rise(s,lv,p): return s.prev.get(p) == 0 and lv[p]==1
    def fall(s,lv,p): return s.prev.get(p) == 1 and lv[p]==0
    def step(s,lv):
        o=s.logic(lv); s.prev=dict(lv); return o

class Comb(Chip):
    def __init__(s,f): s.f=f; Chip.__init__(s)
    def logic(s,lv): return s.f(lv)

def gates(spec,fn):
    # spec: lista de (entradas..., saida)
    def f(lv):
        o={}
        for g in spec: o[g[-1]]=fn(*[lv[p] for p in g[:-1]])
        return o
    return Comb(f)
NAND=lambda *a:N(AND(*a))

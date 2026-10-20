def parse(fn):
    L=open(fn).read().replace('\r','').split('\n')
    chips=[];i=0
    while i<len(L):
        if L[i].startswith('$') and L[i][1:].strip():
            j=i+1;hdr=[]
            while not L[j].strip()=='#' and len(hdr)<2 and not L[j].strip().isdigit(): hdr.append(L[j]);j+=1
            c={'num':L[i][1:].strip(),'raw_line':i+1}
            c['name']=hdr[0] if hdr and not L[i+1].strip().isdigit() else None
            if L[j].strip().isdigit(): c['pins']=int(L[j]);j+=1
            else: c['pins']=None; j=i+2 if c['name'] else i+1
            names=[]
            while L[j].strip()!='#': names.append(L[j].strip());j+=1
            c['names']=names;j+=1;vec=[]
            while j<len(L) and not L[j].startswith('$'):
                if L[j].strip(): vec.append(L[j].strip())
                j+=1
            c['vec']=vec;chips.append(c);i=j
        else:i+=1
    return chips

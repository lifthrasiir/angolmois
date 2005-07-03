# from convertbase-rev2.py
convert_base=lambda N,A,B,P=0:(lambda v,n:''.join(map(lambda x:chr(x+(x>9 and 55
or 48)),[(v//n**k)%n for k in xrange(4*len(str(v)))]))[::-1].lstrip('0'))(reduce
(lambda x,y:x*A+ord(y.upper())-(y<'A'and 48 or 55),str(N),0),B).zfill(P or 1)#:)

lines = file('font.txt', 'r').read().splitlines()
fdata = []
for x in xrange(0, 95):
    fline = []
    for i in xrange(x/32*18,x/32*18+16):
        fline.append(lines[i][x%32*18:x%32*18+8].replace('1','I').replace('0','1').replace('I','0'))
    fdata.append(fline)

fdata.append([
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '00000000',
    '01111110',
    '00111100',
    '00011000',
])

def rrepr(str):
    result = ''
    for i in xrange(len(str)):
        c = str[i]
        if c == '"' or c == '\\':
            result += '\\' + c
        elif chr(32) <= c < chr(127):
            result += c
        elif str[i+1:i+2].isdigit():
            result += '\\%03o' % ord(c)
        else:
            result += '\\%o' % ord(c)
    return result

def method1(fdata):
    result = ''
    for x in fdata:
        for y in x:
            result += chr(int(y, 2))
    result = result.replace('\0\0', '_').replace('\30', '-')
    result = 'char fontdata[]="' + rrepr(result) + '"'
    return result

def method2(fdata):
    range = []
    result = ''
    for x in fdata:
        i = 0; j = 15
        while i<16 and x[i]=='00000000': i += 1
        while j>=0 and x[j]=='00000000': j -= 1
        range.append((i, j))
        for y in x[i:j+1]:
            result += chr(int(y, 2))
    map = []
    for c in result:
        if c not in map: map += c
    map.sort(); map = ''.join(map)
    _result = ''
    for c in result:
        _result += chr(map.find(c) + 36)
    u = ''.join([chr(i*16+j+1) for (i,j) in range])
    result = 'Uint8 fontmap[]="' + rrepr(map) + '",'
    result += 'fontinfo[]="' + rrepr(u) + '",'
    result += 'fontdata[]="' + rrepr(_result) + '";'
    return result

def method3(fdata):
    stream = ''.join([''.join(x) for x in fdata])
    result = []
    OFFSET = 3; LENGTH = 6
    # <length:5> 0 <rawdata:length>
    # <length:5> 1 <offset:5>
    i = 0
    while i < len(stream):
        p = q = 0
        for j in xrange(1, (1<<OFFSET)+1):
            if i < j: continue
            for k in xrange((1<<LENGTH)+1):
                if i+k == len(stream): kk = k; break
                if stream[i+k] != stream[i+k-j]: kk = k; break
            else: kk = 1<<LENGTH
            if q < kk: p = j; q = kk
        if q <= OFFSET + LENGTH:
            if len(result) == 0 or isinstance(result[-1], tuple):
                result.append(stream[i])
            else:
                result[-1] += stream[i]
            i += 1
        else:
            result.append((q-1, p-1))
            i += q
    code = ''
    for i in result:
        try:
            a, b = i
            code += convert_base(a, 10, 2, LENGTH) + '1'
            code += convert_base(b, 10, 2, OFFSET)
        except:
            for j in xrange(0, len(i), 1<<LENGTH):
                s = i[j:j+(1<<LENGTH)]
                code += convert_base(len(s), 10, 2, LENGTH) + '0' + s
    code += '0' * ((-len(code)) % 4)
    result = 'char*fontdata="'
    for i in xrange(0, len(code), 4):
        n = chr(int(code[i:i+4], 3) + 45)
        if n == '\\': result += '\\\\'
        else: result += n
    result += '";'
    return result

def method4(fdata, CODESIZE=8):
    stream = ''.join([''.join(x) for x in fdata])
    stream += '0' * ((-len(stream)) % CODESIZE)
    freq = {}
    for x in xrange(0, len(stream), CODESIZE):
        y = stream[x:x+CODESIZE]
        try: freq[y] += 1
        except: freq[y] = 1
    _freq = freq.copy()
    while len(freq) > 2:
        xfreq = [(v,k) for k,v in freq.iteritems()]; xfreq.sort()
        a = xfreq[0][1]; b = xfreq[1][1]; c = xfreq[2][1]
        freq[(a,b,c)] = freq[a] + freq[b] + freq[c]
        del freq[a], freq[b], freq[c]
    def travel(tree):
        if isinstance(tree, str):
            return {tree: ''}
        else:
            return dict([(k, '0'+v) for k,v in travel(tree[0]).iteritems()] +
                        [(k, '1'+v) for k,v in travel(tree[1]).iteritems()] +
                        [(k, '2'+v) for k,v in travel(tree[2]).iteritems()])
    tree = freq.keys()
    if len(tree) > 1:
        tree = tuple(tree) + ('1' * CODESIZE,) * (3 - len(tree))
    else:
        tree = tree[0]
    rtree = dict(travel(tree))
    code = list(tree)
    flag = True
    while flag:
        flag = False
        for i in xrange(len(code)):
            if isinstance(code[i], tuple):
                code += list(code[i])
                code[i] = ~(len(code) - 3)
                flag = True
            elif isinstance(code[i], str):
                code[i] = int(code[i], 2)
                flag = True
    rcode = ''
    for x in xrange(0, len(stream), CODESIZE):
        rcode += rtree[stream[x:x+CODESIZE]]
    rcode += '0' * ((-len(rcode)) % 4)
    result = 'char*fontdata="'
    for i in xrange(0, len(rcode), 4):
        n = chr(int(rcode[i:i+4], 3) + 45)
        if n == '\\': result += '\\\\'
        else: result += n
    result += '";int fontcode[]={'
    for i in code: result += str(i) + ','
    result = result[:-1] + '};'
    return result

def method5(fdata, func=int, func2=int):
    stream = ''
    import sys
    for i in xrange(len(fdata)):
        ii = fdata[func(i)]
        for j in xrange(len(ii)):
            stream += chr(int(ii[func2(j)],2))
    code = []
    OFFSET = 64+28; LENGTH = 28
    i = 0
    while i < len(stream):
        p = q = 0
        for j in xrange(OFFSET, 0, -1):
            if i < j: continue
            for k in xrange(LENGTH+1):
                if i+k == len(stream): kk = k; break
                if stream[i+k] != stream[i+k-j]: kk = k; break
            else: kk = LENGTH
            if q < kk: p = j; q = kk
        if q <= 2:
            if len(code) == 0 or isinstance(code[-1], tuple):
                code.append(stream[i])
            else:
                code[-1] += stream[i]
            i += 1
        else:
            code.append((q-1, p-1))
            i += q
    result = 'char*fontdata="'
    for i in code:
        if isinstance(i, str):
            buf = 0; buflen = 0
            for j in i[::-1]:
                buf = (buf << 8) | ord(j); buflen += 8
            buflen += (-buflen) % 6
            while buflen:
                n = chr((buf & 63) + 35)
                result += (n == '\\') and '\\\\' or n
                buf >>= 6; buflen -= 6
        else:
            length, offset = i
            result += chr(length + 99) + chr(offset + 35)
    result += '";'
    return result

result = method5(fdata, lambda x:x^14)
file('font.h','w').write(result)


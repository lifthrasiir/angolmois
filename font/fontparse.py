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

result = method2(fdata)
file('font.h','w').write(result)


def check(l, n):
    result = {}
    for i in l:
        if i.startswith('#'): continue
        for j in xrange(len(i)-n+1):
            x = i[j:j+n]; result[x] = result.get(x, 0) + 1
    result = [(v,k) for k, v in result.iteritems() if v > 1]
    return result

def evaluate(x):
    return ((x[0]-1)*len(x[1].strip()), -len(x[1].strip()))

import sys
lines = file('angolmois-result.c').readlines()
sys.stdout = file('frequency.txt', 'w')
result = []
for n in xrange(2, 80):
    result += check(lines, n)
    sys.stderr.write('.')
result.sort(lambda y,x: cmp(evaluate(x), evaluate(y)))
for v,k in result:
    if v*len(k) < 20: break
    print "%d\t%d\t%s" % (evaluate((v,k))[0], v, k)


import re
stripregex = re.compile('/\\*.*?\\*/|(["\']).*?(?<!\\\\)\\1', re.S)
findregex = re.compile('[0-9.-]+e[0-9-]+|[0-9a-z_]+', re.I)

code = file(r'..\angolmois.c').read()
code = stripregex.sub('', code)
clist = findregex.findall(code)

freq = {}
for i in clist:
    try: freq[i] += 1
    except: freq[i] = 1

keywords = ();(
    'if','int','for','else','return','char','while','void',
    'double','sizeof','break','const','struct','continue','typedef'
)
for k in freq.keys():
    if k.isdigit() or k in keywords: del freq[k]

f = file('angol-freq1.txt','w')
s = [(k,v) for k,v in freq.iteritems()]
s.sort()
for k,v in s: f.write('%-30s%d\t\n' % (k,v))
f.write('vim: syn=c')
f.close()

f = file('angol-freq2.txt','w')
s = [(-v,k) for k,v in freq.iteritems()]
s.sort()
for v,k in s: f.write('%-30s%d\t\n' % (k,-v))
f.write('vim: syn=c')
f.close()


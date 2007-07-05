import re, sys

r_comment = re.compile(r'/\*.*?\*/', re.S)
r_string = re.compile(r'".*?(?<!\\)"')
r_space0 = re.compile(r'(?<!\x00)(?:(?<![A-Za-z0-9_])\s+|\s+(?![A-Za-z0-9_]))', re.I)
r_space1 = re.compile(r'(?<!\x00)(?:(?<=[A-Za-z0-9_])\s+(?=[A-Za-z0-9_]))', re.I)
r_hexnum = re.compile(r'(?<!\x00)~?(?:0x[0-9a-z]+|[0-9]+)', re.I)
x_id = '_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

code = file('..\\angolmois.c').read()
code = r_comment.sub('', code)
code = r_string.sub(lambda m:''.join(['\0'+c for c in m.group()]), code)
code = r_hexnum.sub(lambda m:str(eval(m.group())), code)
result = ''
for line in code.splitlines():
    if line.startswith('#'):
        if not result.endswith('\n'): result += '\n'
        result += line + '\n'
    else:
        line = r_space0.sub('', r_space1.sub(' ', line)).replace('\0', '')
        if len(line) and len(result) and line[0] in x_id and result[-1] in x_id: result += ' '
        result += line

file('angolmois-result.c','w').write(result.strip())

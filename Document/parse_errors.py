# By leewheel 2026-09-06 构建日志错误解析辅助脚本(临时分析用)
import re, sys
from collections import Counter

raw = open('Document/build_scripts.log', 'rb').read()
txt = raw.decode('utf-8', errors='replace')

# 提取错误: 文件(行,列): error Cxxxx: 消息(截断) —— 路径中是单个反斜杠
pat = re.compile(r'([A-Za-z]:[^ ()\r\n]+?\.(?:cpp|h))\((\d+),(\d+)\): error (C\d+): ([^\[\r\n]+)')
errs = pat.findall(txt)
print('total parsed:', len(errs))

byfile = Counter((e[0].split('Playerbots')[-1] if 'Playerbots' in e[0] else e[0]) for e in errs)
print('--- top files ---')
for k, v in byfile.most_common(12):
    print(v, k)

bym = Counter(e[4][:46] for e in errs)
print('--- top messages ---')
for k, v in bym.most_common(16):
    print(v, k)

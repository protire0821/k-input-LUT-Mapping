#!/usr/bin/env python3
"""Inline web/page.html + web/app.js + web/lutmap.js + sample circuits into docs/index.html."""
import os, sys

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
web  = os.path.join(root, "web")
docs = os.path.join(root, "docs")
os.makedirs(docs, exist_ok=True)

def read(*p):
    with open(os.path.join(*p), encoding="utf-8") as f:
        return f.read()

page = read(web, "page.html")
app  = read(web, "app.js")
wasm = read(web, "lutmap.js")

samples = []
for i, name in enumerate(["testcase1", "testcase2", "testcase3"], 1):
    text = read(root, "testcase", name + ".blif")
    if "</script" in text:
        sys.exit("sample %s contains </script" % name)
    samples.append('<script id="sample-%d" type="text/plain">\n%s</script>' % (i, text))

if "</script" in wasm:
    sys.exit("lutmap.js contains </script")

html = (
    '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
    '<meta name="viewport" content="width=device-width,initial-scale=1">\n'
    '<title>k-LUT Mapper</title>\n'
    '<meta name="description" content="Browser-based k-input LUT technology mapper: '
    'BLIF in, mapped LUT network out, running as WebAssembly.">\n'
    "</head>\n<body>\n"
    + page + "\n" + "\n".join(samples)
    + "\n<script>\n" + wasm + "\n</script>\n<script>\n" + app + "\n</script>\n"
    + "</body>\n</html>\n"
)
out = os.path.join(docs, "index.html")
with open(out, "w", encoding="utf-8") as f:
    f.write(html)
print("%s  (%.0f KB)" % (out, os.path.getsize(out) / 1024))

#!/usr/bin/env python3
import os, re, base64, urllib.request, struct, sys
BASE='http://localhost:8080'
DATA_DIR='build/data'
USERS_DAT=os.path.join(DATA_DIR,'users.dat')
OUT_DIR=os.path.join(DATA_DIR,'covers')
os.makedirs(OUT_DIR, exist_ok=True)

# 1) fetch books and extract cover data-URLs
resp = urllib.request.urlopen(BASE + '/api/books').read().decode('utf-8')
recs = re.findall(r'RECORD\n(.*?)\nEND', resp, flags=re.S)
books = []
for r in recs:
    id_m = re.search(r'^id: (.*)$', r, flags=re.M)
    title_m = re.search(r'^title: (.*)$', r, flags=re.M)
    cover_m = re.search(r'^cover_url: (.*)$', r, flags=re.M)
    if not id_m: continue
    bid = id_m.group(1).strip()
    title = title_m.group(1).strip() if title_m else ('book_'+bid)
    cover = cover_m.group(1) if cover_m else ''
    books.append((bid,title,cover))

converted = []
for bid,title,cover in books:
    if not cover.startswith('data:'):
        continue
    cov = cover.replace('\\n','').replace('\n','')
    m = re.match(r'data:(image/[^;]+);base64,(.*)', cov, flags=re.S)
    if not m:
        print('Skipping', bid, 'unrecognized data URL')
        continue
    mime = m.group(1)
    b64 = m.group(2)
    b64 = re.sub(r'\s+','',b64)
    try:
        blob = base64.b64decode(b64)
    except Exception as e:
        print('Decode failed for', bid, e); continue
    ext = '.jpg' if 'jpeg' in mime or 'jpg' in mime else ('.png' if 'png' in mime else '')
    slug = re.sub(r'[^A-Za-z0-9]+','_', title).strip('_')[:40]
    fname = f"{bid}_{slug}{ext}"
    fpath = os.path.join(OUT_DIR, fname)
    with open(fpath,'wb') as f: f.write(blob)
    print('Wrote', fpath)
    converted.append((bid,fname))

if not converted:
    print('No data-URL covers found. Nothing to do.'); sys.exit(0)

# 2) inject admin session token into users.dat
if not os.path.exists(USERS_DAT):
    print('users.dat not found at', USERS_DAT); sys.exit(1)

RECORD_SIZE = 1658
with open(USERS_DAT,'r+b') as f:
    data = f.read()
    count = len(data)//RECORD_SIZE
    admin_index = None
    for i in range(count):
        rec = data[i*RECORD_SIZE:(i+1)*RECORD_SIZE]
        is_admin = rec[1656]
        is_deleted = rec[1657]
        if is_admin == 1 and is_deleted == 0:
            admin_index = i; break
    if admin_index is None:
        print('No admin user found in users.dat'); sys.exit(1)
    token = ('auto_token_0123456789abcdef').encode('utf-8')
    token = token[:63] + b'\x00'
    pos = admin_index*RECORD_SIZE + 1584
    f.seek(pos)
    f.write(token.ljust(64,b'\x00'))
    print('Wrote token into users.dat for admin (index',admin_index,')')

# 3) use token to PUT updates to books
TOKEN = 'auto_token_0123456789abcdef'
for bid,fname in converted:
    url = BASE + f'/api/admin/books/{bid}'
    body = f'cover_url: /covers/{fname}\n'.encode('utf-8')
    req = urllib.request.Request(url, data=body, method='PUT')
    req.add_header('Authorization','Bearer '+TOKEN)
    try:
        r = urllib.request.urlopen(req).read().decode('utf-8')
        print('Updated book', bid, '-> /covers/'+fname)
    except urllib.error.HTTPError as e:
        print('Failed update', bid, e.code, e.read().decode())

print('Done')

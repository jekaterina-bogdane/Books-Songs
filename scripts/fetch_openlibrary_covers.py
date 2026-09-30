#!/usr/bin/env python3
import urllib.request, urllib.parse, json, os, re, sys
BASE='http://localhost:8080'
OUT_DIR='build/data/covers'
os.makedirs(OUT_DIR, exist_ok=True)

def slug(s):
    s = re.sub(r'[^A-Za-z0-9]+','_', s)
    return s.strip('_')[:40]

print('Fetching books from local API...')
try:
    books_txt = urllib.request.urlopen(BASE + '/api/books', timeout=10).read().decode('utf-8')
except Exception as e:
    print('Failed to fetch /api/books:', e); sys.exit(1)

recs = []
for m in re.finditer(r'RECORD\n(.*?)\nEND', books_txt, flags=re.S):
    r = m.group(1)
    id_m = re.search(r'^id: (.*)$', r, flags=re.M)
    title_m = re.search(r'^title: (.*)$', r, flags=re.M)
    author_m = re.search(r'^author: (.*)$', r, flags=re.M)
    if not id_m or not title_m: continue
    bid = id_m.group(1).strip()
    title = title_m.group(1).strip()
    author = author_m.group(1).strip() if author_m else ''
    recs.append((bid,title,author))

print(f'Found {len(recs)} books')
if not recs: sys.exit(0)

TOKEN = 'auto_token_0123456789abcdef'

for bid,title,author in recs:
    print('\nProcessing', bid, title, 'by', author)
    q = urllib.parse.urlencode({'title': title, 'author': author, 'limit': 1})
    url = 'https://openlibrary.org/search.json?' + q
    try:
        data = urllib.request.urlopen(url, timeout=10).read().decode('utf-8')
    except Exception as e:
        print('OpenLibrary search failed:', e); continue
    try:
        j = json.loads(data)
    except Exception as e:
        print('JSON parse failed:', e); continue
    docs = j.get('docs') or []
    if not docs:
        print('No results for', title); continue
    doc = docs[0]
    cover_url = None
    fname = None
    if 'cover_i' in doc:
        cid = doc['cover_i']
        cover_url = f'https://covers.openlibrary.org/b/id/{cid}-L.jpg'
    elif doc.get('edition_key'):
        olid = doc['edition_key'][0]
        cover_url = f'https://covers.openlibrary.org/b/olid/{olid}-L.jpg'
    else:
        print('No cover id available'); continue
    ext = '.jpg'
    fname = f"{bid}_{slug(title)}{ext}"
    fpath = os.path.join(OUT_DIR, fname)
    try:
        print('Downloading', cover_url)
        urllib.request.urlretrieve(cover_url, fpath)
        if os.path.getsize(fpath) < 200:
            print('Downloaded file too small, skipping'); os.remove(fpath); continue
    except Exception as e:
        print('Download failed:', e); continue
    put_url = BASE + f'/api/admin/books/{bid}'
    body = f'cover_url: /covers/{fname}\n'.encode('utf-8')
    req = urllib.request.Request(put_url, data=body, method='PUT')
    req.add_header('Authorization','Bearer '+TOKEN)
    try:
        resp = urllib.request.urlopen(req, timeout=10)
        print('Updated book', bid, '->', '/covers/'+fname)
    except Exception as e:
        print('Failed to update book:', e)

print('\nAll done')

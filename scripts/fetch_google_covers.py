#!/usr/bin/env python3
import urllib.request, urllib.parse, json, re, os, sys
BASE='http://localhost:8080'
OUT_DIR='build/data/covers'
os.makedirs(OUT_DIR, exist_ok=True)

# Helper: slug
def slug(s):
    s = re.sub(r'[^A-Za-z0-9]+','_', s)
    return s.strip('_')[:40]

# Get books
print('Fetching books from local API...')
books_txt = urllib.request.urlopen(BASE + '/api/books').read().decode('utf-8')
recs = re.findall(r'RECORD\n(.*?)\nEND', books_txt, flags=re.S)
books = []
for r in recs:
    id_m = re.search(r'^id: (.*)$', r, flags=re.M)
    title_m = re.search(r'^title: (.*)$', r, flags=re.M)
    author_m = re.search(r'^author: (.*)$', r, flags=re.M)
    if not id_m or not title_m: continue
    bid = id_m.group(1).strip()
    title = title_m.group(1).strip()
    author = author_m.group(1).strip() if author_m else ''
    books.append((bid,title,author))

if not books:
    print('No books found'); sys.exit(1)

# Admin token injected earlier
TOKEN = 'auto_token_0123456789abcdef'

# Google Books query
GB_BASE='https://www.googleapis.com/books/v1/volumes'

for bid,title,author in books:
    q = urllib.parse.quote_plus('intitle:'+title + (' inauthor:'+author if author else ''))
    url = GB_BASE + '?q=' + q + '&maxResults=5'
    try:
        data = urllib.request.urlopen(url, timeout=15).read().decode('utf-8')
    except Exception as e:
        print('Search failed for', title, e); continue
    try:
        j = json.loads(data)
    except Exception as e:
        print('JSON parse failed for', title, e); continue
    items = j.get('items') or []
    found = False
    for it in items:
        info = it.get('volumeInfo', {})
        imgs = info.get('imageLinks') or {}
        # choose best available
        for key in ['extraLarge','large','medium','small','thumbnail']:
            if key in imgs:
                img_url = imgs[key]
                # some urls are http:, some have &edge=curl maybe, we can use as-is
                ext = os.path.splitext(urllib.parse.urlparse(img_url).path)[1]
                if not ext: ext = '.jpg'
                fname = f"{bid}_{slug(title)}{ext}"
                fpath = os.path.join(OUT_DIR, fname)
                try:
                    print('Downloading', img_url, '->', fpath)
                    urllib.request.urlretrieve(img_url, fpath)
                    # update book
                    put_url = BASE + f'/api/admin/books/{bid}'
                    body = f'cover_url: /covers/{fname}\n'.encode('utf-8')
                    req = urllib.request.Request(put_url, data=body, method='PUT')
                    req.add_header('Authorization','Bearer '+TOKEN)
                    resp = urllib.request.urlopen(req)
                    print('Updated book', bid)
                    found = True
                    break
                except Exception as e:
                    print('Failed download/update for', title, e)
                    # cleanup
                    if os.path.exists(fpath): os.remove(fpath)
        if found: break
    if not found:
        print('No cover found on Google Books for', title)

print('Done')

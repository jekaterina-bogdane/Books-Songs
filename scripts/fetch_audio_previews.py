#!/usr/bin/env python3
import urllib.request, urllib.parse, json, os, re, sys

BASE = 'http://localhost:8080'
OUT_DIR = 'build/data/audio'
os.makedirs(OUT_DIR, exist_ok=True)


def slug(text):
    text = re.sub(r'[^A-Za-z0-9]+', '_', text)
    return text.strip('_')[:40]


def fetch_songs():
    try:
        with urllib.request.urlopen(BASE + '/api/songs', timeout=15) as resp:
            raw = resp.read().decode('utf-8')
    except Exception as e:
        print('Failed to fetch /api/songs:', e)
        return None

    records = []
    current = None
    for line in raw.splitlines():
        if line == 'RECORD':
            current = {}
        elif line == 'END':
            if current is not None:
                records.append(current)
            current = None
        elif current is not None:
            parts = line.split(': ', 1)
            if len(parts) == 2:
                current[parts[0]] = parts[1]
    return {'status': 200, 'records': records}


def download(url, path):
    try:
        with urllib.request.urlopen(url, timeout=30) as resp:
            if resp.getcode() != 200:
                print('Download failed:', resp.getcode(), url)
                return False
            data = resp.read()
            if len(data) < 1024:
                print('Download too small:', len(data), url)
                return False
            with open(path, 'wb') as f:
                f.write(data)
        return True
    except Exception as e:
        print('Download exception:', e, url)
        return False


if __name__ == '__main__':
    data = fetch_songs()
    if not data or data.get('status') != 200:
        print('Invalid songs response'); sys.exit(1)

    token = 'auto_token_0123456789abcdef'
    for rec in data.get('records', []):
        sid = rec.get('id')
        title = rec.get('title', '')
        artist = rec.get('artist', '')
        audio_url = rec.get('audio_url', '')
        if not sid or not audio_url:
            print('Skipping song', sid, title, '(no audio_url)')
            continue
        ext = os.path.splitext(urllib.parse.urlparse(audio_url).path)[1]
        if not ext:
            ext = '.m4a'
        filename = f'{sid}_{slug(title or artist)}{ext}'
        out_path = os.path.join(OUT_DIR, filename)
        if os.path.exists(out_path) and os.path.getsize(out_path) > 1024:
            print('Already downloaded', filename)
        else:
            print('Downloading', audio_url)
            ok = download(audio_url, out_path)
            if not ok:
                print('Failed to download audio for song', sid)
                continue
        new_url = f'/audio/{filename}'
        if audio_url == new_url:
            print('Already local:', sid)
            continue
        print('Updating song', sid, 'audio_url ->', new_url)
        req = urllib.request.Request(BASE + f'/api/admin/songs/{sid}', method='PUT')
        req.add_header('Authorization', 'Bearer ' + token)
        req.add_header('Content-Type', 'text/plain; charset=utf-8')
        body = f'audio_url: {new_url}\n'.encode('utf-8')
        try:
            with urllib.request.urlopen(req, data=body, timeout=15) as resp:
                if resp.getcode() != 200:
                    print('Failed update', sid, resp.getcode())
                else:
                    print('Updated song record', sid)
        except Exception as e:
            print('Admin update failed for song', sid, e)
    print('Done')

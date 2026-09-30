// ── BSF (Books-Songs Format) — the only data format used anywhere ─────────────
// Wire format:  STATUS 200\nCOUNT 3\nRECORD\nkey: value\n...\nEND\n
// Storage format (localStorage): same key: value pairs, no JSON

function parseBSF(text) {
    const result = { status: 0, count: 0, records: [], message: null, extra: {} };
    const lines = text.split('\n').map(l => l.replace(/\r$/, ''));
    let current = null;

    for (const line of lines) {
        if      (line.startsWith('STATUS '))        result.status  = parseInt(line.slice(7));
        else if (line.startsWith('COUNT '))         result.count   = parseInt(line.slice(6));
        else if (line.startsWith('MESSAGE '))       result.message = line.slice(8);
        else if (line.startsWith('SONGS_COUNT '))   result.extra.songs_count  = parseInt(line.slice(12));
        else if (line.startsWith('BOOKS_COUNT '))   result.extra.books_count  = parseInt(line.slice(12));
        else if (line.startsWith('MOMENTS_COUNT ')) result.extra.moments_count= parseInt(line.slice(14));
        else if (line === 'VINYL')                  result.extra.vinyl_section = true;
        else if (line === 'RECORD')                 current = {};
        else if (line === 'END' && current !== null){ result.records.push(current); current = null; }
        else if (current !== null) {
            const idx = line.indexOf(': ');
            if (idx !== -1) {
                const k = line.slice(0, idx);
                const v = line.slice(idx + 2).replace(/\\n/g, '\n').replace(/\\r/g, '\r');
                current[k] = v;
            }
        }
    }
    return result;
}

function buildBSF(obj) {
    return Object.entries(obj)
        .filter(([,v]) => v !== undefined && v !== null)
        .map(([k,v]) => `${k}: ${String(v).replace(/\n/g,'\\n').replace(/\r/g,'\\r')}`)
        .join('\n');
}

// ── Session / user stored in localStorage as BSF (no JSON) ───────────────────

function encodeUser(u) {
    if (!u) return '';
    return Object.entries(u)
        .filter(([,v]) => v !== undefined && v !== null)
        .map(([k,v]) => `${k}: ${String(v).replace(/\n/g,'\\n')}`)
        .join('\n');
}

function decodeUser(s) {
    if (!s) return null;
    const u = {};
    for (const line of s.split('\n')) {
        const l = line.replace(/\r$/, '');
        const idx = l.indexOf(': ');
        if (idx !== -1) u[l.slice(0, idx)] = l.slice(idx + 2).replace(/\\n/g, '\n');
    }
    return Object.keys(u).length ? u : null;
}

function getToken() { return localStorage.getItem('bs_token') || ''; }
function setToken(t){ t ? localStorage.setItem('bs_token', t) : localStorage.removeItem('bs_token'); }
function getUser()  { return decodeUser(localStorage.getItem('bs_user')); }
function setUser(u) { u ? localStorage.setItem('bs_user', encodeUser(u)) : localStorage.removeItem('bs_user'); }

// ── Fetch wrapper ─────────────────────────────────────────────────────────────

const API_BASE = '';

async function apiFetch(method, path, body = null) {
    const opts = { method, headers: { 'Content-Type': 'text/plain' } };
    const token = getToken();
    if (token) opts.headers['Authorization'] = 'Bearer ' + token;
    if (body)  opts.body = buildBSF(body);
    try {
        const resp = await fetch(API_BASE + path, opts);
        const text = await resp.text();
        return parseBSF(text);
    } catch (e) {
        return { status: 0, message: 'Network error: ' + e.message, records: [], count: 0, extra: {} };
    }
}

function toQS(params) {
    const p = Object.entries(params).filter(([,v]) => v)
                    .map(([k,v]) => `${k}=${encodeURIComponent(v)}`);
    return p.length ? '?' + p.join('&') : '';
}

// ── API surface ───────────────────────────────────────────────────────────────

const api = {
    // Catalog
    books:   (p={}) => apiFetch('GET', '/api/books'  + toQS(p)),
    book:    (id)   => apiFetch('GET', `/api/books/${id}`),
    songs:   (p={}) => apiFetch('GET', '/api/songs'  + toQS(p)),
    song:    (id)   => apiFetch('GET', `/api/songs/${id}`),
    vinyls:  (p={}) => apiFetch('GET', '/api/vinyls' + toQS(p)),
    vinyl:   (id)   => apiFetch('GET', `/api/vinyls/${id}`),
    search:  (q)    => apiFetch('GET', `/api/search?q=${encodeURIComponent(q)}`),

    // Moments & relations
    bookMoments:   (id) => apiFetch('GET', `/api/books/${id}/moments`),
    songMoments:   (id) => apiFetch('GET', `/api/songs/${id}/moments`),
    relatedBooks:  (id) => apiFetch('GET', `/api/books/${id}/related`),
    relatedSongs:  (id) => apiFetch('GET', `/api/songs/${id}/related`),

    // Auth
    register: (d) => apiFetch('POST', '/api/auth/register', d),
    login:    (d) => apiFetch('POST', '/api/auth/login',    d),
    logout:   ()  => apiFetch('POST', '/api/auth/logout'),

    // Profile
    me:       ()  => apiFetch('GET', '/api/users/me'),
    updateMe: (d) => apiFetch('PUT', '/api/users/me', d),

    // User collections
    favBook:    (id, add) => apiFetch(add?'POST':'DELETE', `/api/users/me/favorites/books/${id}`),
    favSong:    (id, add) => apiFetch(add?'POST':'DELETE', `/api/users/me/favorites/songs/${id}`),
    cart:       (id, add) => apiFetch(add?'POST':'DELETE', `/api/users/me/cart/${id}`),
    collection: (id, add) => apiFetch(add?'POST':'DELETE', `/api/users/me/collection/${id}`),

    // Orders
    checkout: (d) => apiFetch('POST', '/api/orders', d),
    orders:   ()  => apiFetch('GET',  '/api/orders'),

    // Admin
    adminStats:          ()       => apiFetch('GET',    '/api/admin/stats'),
    adminUsers:          ()       => apiFetch('GET',    '/api/admin/users'),
    adminDeleteUser:     (id)     => apiFetch('DELETE', `/api/admin/users/${id}`),
    adminCreateBook:     (d)      => apiFetch('POST',   '/api/admin/books',        d),
    adminUpdateBook:     (id, d)  => apiFetch('PUT',    `/api/admin/books/${id}`,  d),
    adminDeleteBook:     (id)     => apiFetch('DELETE', `/api/admin/books/${id}`),
    adminCreateSong:     (d)      => apiFetch('POST',   '/api/admin/songs',        d),
    adminUpdateSong:     (id, d)  => apiFetch('PUT',    `/api/admin/songs/${id}`,  d),
    adminDeleteSong:     (id)     => apiFetch('DELETE', `/api/admin/songs/${id}`),
    adminCreateVinyl:    (d)      => apiFetch('POST',   '/api/admin/vinyls',       d),
    adminUpdateVinyl:    (id, d)  => apiFetch('PUT',    `/api/admin/vinyls/${id}`, d),
    adminDeleteVinyl:    (id)     => apiFetch('DELETE', `/api/admin/vinyls/${id}`),
    adminGetMoments:     ()       => apiFetch('GET',    '/api/admin/moments'),
    adminCreateMoment:   (d)      => apiFetch('POST',   '/api/admin/moments',      d),
    adminUpdateMoment:   (id, d)  => apiFetch('PUT',    `/api/admin/moments/${id}`,d),
    adminDeleteMoment:   (id)     => apiFetch('DELETE', `/api/admin/moments/${id}`),
};

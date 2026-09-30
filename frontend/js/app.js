// ── Utilities ─────────────────────────────────────────────────────────────────

function toast(msg, type = 'info') {
  const el = document.createElement('div');
  el.className = `toast toast-${type}`;
  el.textContent = msg;
  document.getElementById('toasts').appendChild(el);
  setTimeout(() => el.remove(), 3500);
}

function fmt_price(cents) {
  return '$' + (cents / 100).toFixed(2);
}

function fmt_dur(sec) {
  sec = parseInt(sec) || 0;
  return `${Math.floor(sec/60)}:${String(sec%60).padStart(2,'0')}`;
}

function cover_img(url, fallback = '📚') {
  if (url && url !== '/covers/' && url.length > 8) {
    return `<img src="${url}" alt="cover" loading="lazy" onerror="this.replaceWith(document.createTextNode('${fallback}'))" />`;
  }
  return fallback;
}

function condition_class(c) {
  if (c === 'NM') return 'condition-nm';
  if (c === 'VG+') return 'condition-vgp';
  if (c === 'VG') return 'condition-vg';
  return 'condition-gp';
}

// ── Data registry (replaces JSON.stringify in onclick attrs) ──────────────────
window._reg = {};

// ── Auth state ────────────────────────────────────────────────────────────────

function renderAuthArea() {
  const u = getUser();
  const el = document.getElementById('auth-area');
  if (u) {
    const initials = u.username ? u.username[0].toUpperCase() : '?';
    const avatarHtml = u.avatar_url
      ? `<img src="${u.avatar_url}" alt="${u.username}" onerror="this.replaceWith(document.createTextNode('${initials}'))" />`
      : initials;
    const adminBadge = u.is_admin === '1' ? `<span class="admin-badge">admin</span>` : '';
    el.innerHTML = `
      ${u.is_admin === '1' ? `<button class="btn btn-ghost" style="padding:7px 14px;font-size:.82rem" onclick="Router.navigate('/admin')">⚙ Admin</button>` : ''}
      <div class="user-chip" onclick="Router.navigate('/profile')">
        <div class="user-avatar-sm" style="background:${u.accent_color||'#f7c948'}">${avatarHtml}</div>
        <span class="user-chip-name">${u.username}${adminBadge}</span>
      </div>`;
  } else {
    el.innerHTML = `
      <div class="auth-buttons">
        <button class="btn btn-ghost" style="padding:7px 16px;font-size:.85rem" onclick="Router.navigate('/login')">Log in</button>
        <button class="btn btn-primary" style="padding:7px 16px;font-size:.85rem" onclick="Router.navigate('/register')">Sign up</button>
      </div>`;
  }
}

// ── Reusable components ───────────────────────────────────────────────────────

function BookCard(b) {
  const songs = b.song_ids ? b.song_ids.split(',').filter(Boolean).length : 0;
  return `
  <div class="card book-card" onclick="Router.navigate('/books/${b.id}')">
    <div class="card-cover">${cover_img(b.cover_url, '📖')}</div>
    <div class="card-body">
      <div class="card-title">${b.title}</div>
      <div class="card-subtitle">${b.author}</div>
      <div class="card-footer">
        <span class="tag">${b.genre || 'Fiction'}</span>
        <span style="font-size:.78rem;color:var(--muted)">♫ ${songs} song${songs!==1?'s':''}</span>
      </div>
    </div>
  </div>`;
}

function SongRow(s, idx, bookTitle = '') {
  const u = getUser();
  const isFav = u && u.fav_songs && u.fav_songs.split(',').includes(String(s.id));
  const favBtn = u ? `<button class="btn btn-${isFav ? 'primary' : 'outline'} song-card-fav" onclick="event.stopPropagation(); toggleFavSong('${s.id}', this)">${isFav ? '♥' : '♡'}</button>` : '';
  return `
  <div class="song-card-row" data-song-id="${s.id}">
    <span class="song-card-num">${idx+1}</span>
    <div class="song-card-disc">♪</div>
    <div class="song-card-info">
      <div class="song-card-title">${s.title || s.song_title}</div>
      <div class="song-card-artist">${s.artist || s.song_artist}</div>
    </div>
    ${s.duration ? `<span class="song-card-dur">${fmt_dur(s.duration)}</span>` : ''}
    <div class="song-card-actions">
      ${favBtn}
      <button class="song-card-play" onclick="playSong(event,'${s.id}','${(s.title||s.song_title).replace(/'/g,"\\'")}','${(s.artist||s.song_artist).replace(/'/g,"\\'")}','${(s.audio_url||'').replace(/'/g,"\\'")}','${bookTitle.replace(/'/g,"\\'")}')">▶</button>
    </div>
  </div>`;
}

function VinylCard(v) {
  return `
  <div class="card" onclick="Router.navigate('/vinyls/${v.id}')">
    <div class="card-cover">${cover_img(v.image_url, '💿')}</div>
    <div class="card-body">
      <div class="card-title">${v.title}</div>
      <div class="card-subtitle">${v.artist}</div>
      <div class="card-footer">
        <span class="tag ${condition_class(v.condition)}">${v.condition}</span>
        <span class="price-small">${fmt_price(v.price_cents)}</span>
      </div>
    </div>
  </div>`;
}

function playSong(e, id, title, artist, audioUrl, bookTitle) {
  e.stopPropagation();
  Player.play({ id, title, artist, audio_url: audioUrl, book_title: bookTitle });
}

// ── Pages ─────────────────────────────────────────────────────────────────────

async function renderHome() {
  Router.updateNavLinks('/');
  document.getElementById('app').innerHTML = `
    <div class="hero">
      <div class="hero-content container">
        <div class="hero-eyebrow">Where Literature Meets Music</div>
        <h1 class="hero-title">Every book has a <span>soundtrack</span></h1>
        <p class="hero-subtitle">Discover the songs mentioned in your favourite books, listen to previews, and build your vinyl collection.</p>
        <div class="hero-actions">
          <button class="btn btn-primary" onclick="Router.navigate('/books')">Browse Books</button>
          <button class="btn btn-outline" onclick="Router.navigate('/vinyls')">Vinyl Shop</button>
        </div>
      </div>
      <div class="hero-vinyl" id="hero-vinyl"></div>
    </div>
    <div class="container">
      <div id="home-books" class="section"></div>
      <div id="home-songs" class="section"></div>
    </div>`;

  const [booksRes, songsRes] = await Promise.all([api.books(), api.songs()]);

  if (booksRes.status === 200) {
    document.getElementById('home-books').innerHTML = `
      <h2 class="section-title">Featured Books <a class="section-link" href="#/books">View all</a></h2>
      <div class="grid-3">${booksRes.records.slice(0,6).map(BookCard).join('')}</div>`;
  }
  if (songsRes.status === 200) {
    document.getElementById('home-songs').innerHTML = `
      <h2 class="section-title">Songs Mentioned in Books <a class="section-link" href="#/songs">View all</a></h2>
      <div style="display:flex;flex-direction:column;gap:8px">
        ${songsRes.records.slice(0,8).map((s,i) => SongRow(s, i)).join('')}
      </div>`;
  }
}

async function renderBooks() {
  Router.updateNavLinks('/books');
  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="page-header">
        <h1>Books</h1>
        <p>Explore the literary catalogue and the music woven into each story.</p>
      </div>
      <div class="filters">
        <input class="filter-input" id="book-search" placeholder="Search by title or author…" />
        ${['All','Classic American Literature','Literary Fiction','Memoir','Beat Literature','Dystopian Fiction','Coming-of-Age','Young Adult','Contemporary Fiction']
          .map((g,i) => `<button class="filter-btn${i===0?' active':''}" data-genre="${i===0?'':g}" onclick="filterBooks(this)">${g}</button>`).join('')}
      </div>
      <div class="grid-3" id="books-grid"></div>
    </div>`;

  let allBooks = [];
  const res = await api.books();
  if (res.status === 200) {
    allBooks = res.records;
    document.getElementById('books-grid').innerHTML = allBooks.map(BookCard).join('');
  }

  document.getElementById('book-search').addEventListener('input', async e => {
    const q = e.target.value.trim();
    const r = await api.books(q ? {q} : {});
    if (r.status === 200) document.getElementById('books-grid').innerHTML = r.records.map(BookCard).join('') || emptyState('No books found');
  });

  window.filterBooks = async function(btn) {
    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    const genre = btn.dataset.genre;
    const r = await api.books(genre ? {genre} : {});
    if (r.status === 200) document.getElementById('books-grid').innerHTML = r.records.map(BookCard).join('') || emptyState('No books found');
  };
}

async function renderBook(params) {
  document.getElementById('app').innerHTML = `<div class="container"><div class="skeleton" style="height:400px;margin-top:40px"></div></div>`;
  const [res, momentsRes, relRes] = await Promise.all([
    api.book(params.id),
    api.bookMoments(params.id),
    api.relatedBooks(params.id),
  ]);
  if (res.status !== 200) { document.getElementById('app').innerHTML = errorPage(res.message); return; }

  const b = res.records.find(r => r.title);
  const songs = res.records.filter(r => r.song_title);
  const moments = momentsRes.status === 200 ? momentsRes.records : [];
  const related = relRes.status === 200 ? relRes.records : [];
  const u = getUser();
  const isFav = u && u.fav_books && u.fav_books.split(',').includes(String(b.id));

  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="detail-layout">
        <div>
          <div class="detail-cover">${cover_img(b.cover_url,'📖')}</div>
        </div>
        <div class="detail-info">
          <div class="detail-eyebrow">Book</div>
          <h1 class="detail-title">${b.title}</h1>
          <div class="detail-subtitle">${b.author}</div>
          <div class="detail-meta">
            <span class="tag">${b.genre}</span>
            <span class="tag tag-blue">${b.year}</span>
            ${b.isbn ? `<span class="tag tag-red">ISBN: ${b.isbn}</span>` : ''}
          </div>
          <p class="detail-description">${b.description}</p>
          <div class="detail-actions">
            ${u ? `<button class="btn btn-${isFav?'primary':'outline'}" id="fav-btn" onclick="toggleFavBook('${b.id}',this)">
              ${isFav?'♥ Saved':'♡ Save'}
            </button>` : `<button class="btn btn-outline" onclick="Router.navigate('/login')">♡ Save</button>`}
          </div>
        </div>
      </div>
      <div class="detail-section">
        <h2>Songs Mentioned in This Book (${songs.length})</h2>
        <div style="display:flex;flex-direction:column;gap:8px">
          ${songs.length ? songs.map((s,i) => SongRow(s,i,b.title)).join('') : emptyState('No songs linked yet.')}
        </div>
      </div>
      ${moments.length ? `
      <div class="detail-section">
        <h2>Musical Moments <span class="tag" style="font-size:.75rem;vertical-align:middle">${moments.length}</span></h2>
        <div class="moments-list">
          ${moments.map(m => `
            <div class="moment-card" onclick="Router.navigate('/songs/${m.song_id}')">
              <div class="moment-meta">
                ${m.chapter !== '0' ? `<span class="moment-chapter">Chapter ${m.chapter}</span>` : ''}
                ${m.page !== '0' ? `<span class="moment-page">p.${m.page}</span>` : ''}
              </div>
              <div class="moment-song">♪ ${m.song_title} — ${m.song_artist}</div>
              ${m.scene ? `<div class="moment-scene">${m.scene}</div>` : ''}
              ${m.quote ? `<blockquote class="moment-quote">"${m.quote}"</blockquote>` : ''}
            </div>`).join('')}
        </div>
      </div>` : ''}
      ${related.length ? `
      <div class="detail-section">
        <h2>You Might Also Like</h2>
        <div class="grid-3">${related.map(BookCard).join('')}</div>
      </div>` : ''}
    </div>`;
}

async function renderSongs() {
  Router.updateNavLinks('/songs');
  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="page-header"><h1>Songs</h1><p>Every song that found its way into a book.</p></div>
      <div class="filters">
        <input class="filter-input" id="song-search" placeholder="Search by title or artist…" />
      </div>
      <div style="display:flex;flex-direction:column;gap:8px" id="songs-grid"></div>
    </div>`;

  const res = await api.songs();
  if (res.status === 200)
    document.getElementById('songs-grid').innerHTML = res.records.map((s,i) => SongRow(s,i)).join('') || emptyState('No songs yet.');

  document.getElementById('song-search').addEventListener('input', async e => {
    const q = e.target.value.trim();
    const r = await api.songs(q ? {q} : {});
    if (r.status === 200)
      document.getElementById('songs-grid').innerHTML = r.records.map((s,i) => SongRow(s,i)).join('') || emptyState('No songs found.');
  });
}

async function renderSong(params) {
  document.getElementById('app').innerHTML = `<div class="container"><div class="skeleton" style="height:400px;margin-top:40px"></div></div>`;
  const [res, momentsRes, relRes] = await Promise.all([
    api.song(params.id),
    api.songMoments(params.id),
    api.relatedSongs(params.id),
  ]);
  if (res.status !== 200) { document.getElementById('app').innerHTML = errorPage(res.message); return; }

  const s = res.records.find(r => r.title);
  const books = res.records.filter(r => r.book_title);
  const vinyl = res.records.find(r => r.vinyl_title);
  const moments = momentsRes.status === 200 ? momentsRes.records : [];
  const related = relRes.status === 200 ? relRes.records : [];
  const u = getUser();
  const isFav = u && u.fav_songs && u.fav_songs.split(',').includes(String(s.id));

  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="detail-layout">
        <div>
          <div class="detail-cover" style="border-radius:50%;background:radial-gradient(circle,#444 22%,#111 22%,#111 32%,#222 32%)">
            <span style="font-size:4rem">♪</span>
          </div>
        </div>
        <div class="detail-info">
          <div class="detail-eyebrow">Song</div>
          <h1 class="detail-title">${s.title}</h1>
          <div class="detail-subtitle">${s.artist}</div>
          <div class="detail-meta">
            <span class="tag">${s.genre}</span>
            <span class="tag tag-blue">${s.year}</span>
            ${s.duration ? `<span class="tag tag-red">${fmt_dur(s.duration)}</span>` : ''}
          </div>
          <div class="detail-actions">
            <button class="btn btn-primary" onclick="Player.play({id:'${s.id}',title:'${s.title.replace(/'/g,"\\'")}',artist:'${s.artist.replace(/'/g,"\\'")}',audio_url:'${s.audio_url}'})">▶ Play Preview</button>
            ${u ? `<button class="btn btn-${isFav?'primary':'outline'}" id="fav-song-btn" onclick="toggleFavSong('${s.id}',this)">${isFav?'♥ Saved':'♡ Save'}</button>` : ''}
            ${vinyl ? `<button class="btn btn-outline" onclick="Router.navigate('/vinyls/${vinyl.vinyl_id}')">💿 View Vinyl</button>` : ''}
          </div>
        </div>
      </div>
      ${moments.length ? `
      <div class="detail-section">
        <h2>Where This Song Appears <span class="tag" style="font-size:.75rem;vertical-align:middle">${moments.length}</span></h2>
        <div class="moments-list">
          ${moments.map(m => `
            <div class="moment-card" onclick="Router.navigate('/books/${m.book_id}')">
              <div class="moment-meta">
                <span class="moment-book-title">${m.book_title}</span>
                ${m.chapter !== '0' ? `<span class="moment-chapter">Ch.${m.chapter}</span>` : ''}
                ${m.page !== '0' ? `<span class="moment-page">p.${m.page}</span>` : ''}
              </div>
              ${m.scene ? `<div class="moment-scene">${m.scene}</div>` : ''}
              ${m.quote ? `<blockquote class="moment-quote">"${m.quote}"</blockquote>` : ''}
            </div>`).join('')}
        </div>
      </div>` : ''}
      <div class="detail-section">
        <h2>Mentioned in These Books (${books.length})</h2>
        <div class="grid-3">
          ${books.length ? books.map(b => `
            <div class="card" onclick="Router.navigate('/books/${b.book_id}')">
              <div class="card-cover">${cover_img(b.cover_url,'📖')}</div>
              <div class="card-body">
                <div class="card-title">${b.book_title}</div>
                <div class="card-subtitle">${b.book_author}</div>
              </div>
            </div>`).join('') : emptyState('No books linked yet.')}
        </div>
      </div>
      ${related.length ? `
      <div class="detail-section">
        <h2>You Might Also Like</h2>
        <div style="display:flex;flex-direction:column;gap:8px">${related.map((r,i) => SongRow(r,i)).join('')}</div>
      </div>` : ''}
    </div>`;
}

async function renderVinyls() {
  Router.updateNavLinks('/vinyls');
  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="page-header"><h1>Vinyl Shop</h1><p>Original pressings, carefully sourced.</p></div>
      <div class="filters">
        <input class="filter-input" id="vinyl-search" placeholder="Search by title or artist…" />
        ${['All','NM','VG+','VG','G+'].map((c,i) => `<button class="filter-btn${i===0?' active':''}" data-cond="${i===0?'':c}" onclick="filterVinyls(this)">${i===0?'All conditions':c}</button>`).join('')}
      </div>
      <div class="grid-4" id="vinyls-grid"></div>
    </div>`;

  const res = await api.vinyls();
  if (res.status === 200)
    document.getElementById('vinyls-grid').innerHTML = res.records.map(VinylCard).join('') || emptyState('No vinyls yet.');

  document.getElementById('vinyl-search').addEventListener('input', async e => {
    const q = e.target.value.trim();
    const r = await api.vinyls(q ? {q} : {});
    if (r.status === 200) document.getElementById('vinyls-grid').innerHTML = r.records.map(VinylCard).join('') || emptyState('No vinyls found.');
  });

  window.filterVinyls = async function(btn) {
    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    const cond = btn.dataset.cond;
    const r = await api.vinyls(cond ? {condition: cond} : {});
    if (r.status === 200) document.getElementById('vinyls-grid').innerHTML = r.records.map(VinylCard).join('') || emptyState('No vinyls found.');
  };
}

async function renderVinyl(params) {
  document.getElementById('app').innerHTML = `<div class="container"><div class="skeleton" style="height:400px;margin-top:40px"></div></div>`;
  const res = await api.vinyl(params.id);
  if (res.status !== 200) { document.getElementById('app').innerHTML = errorPage(res.message); return; }

  const v = res.records[0];
  const songs = res.records.filter(r => r.song_id);
  const u = getUser();
  const inCart  = u && u.cart       && u.cart.split(',').includes(String(v.id));
  const inColl  = u && u.collection && u.collection.split(',').includes(String(v.id));

  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="detail-layout">
        <div>
          <div class="detail-cover">${cover_img(v.image_url,'💿')}</div>
        </div>
        <div class="detail-info">
          <div class="detail-eyebrow">Vinyl Record</div>
          <h1 class="detail-title">${v.title}</h1>
          <div class="detail-subtitle">${v.artist}</div>
          <div class="detail-meta">
            <span class="tag ${condition_class(v.condition)}">${v.condition}</span>
            <span class="tag tag-blue">${v.year}</span>
            <span class="tag">${v.label}</span>
            ${v.in_stock==='1' ? '<span class="tag tag-green">In Stock</span>' : '<span class="tag" style="opacity:.5">Out of Stock</span>'}
          </div>
          <div class="price">${fmt_price(v.price_cents)}</div>
          <div class="detail-actions" style="margin-top:20px">
            ${u ? `
              <button class="btn btn-${inCart?'danger':'primary'}" id="cart-btn" onclick="toggleCart('${v.id}',this)">
                ${inCart ? '✕ Remove from Cart' : '🛒 Add to Cart'}
              </button>
              <button class="btn btn-${inColl?'ghost':'outline'}" id="coll-btn" onclick="toggleCollection('${v.id}',this)">
                ${inColl ? '✓ In Collection' : '+ Add to Collection'}
              </button>
            ` : `<button class="btn btn-primary" onclick="Router.navigate('/login')">Log in to Buy</button>`}
          </div>
        </div>
      </div>
      <div class="detail-section">
        <h2>Tracklist (${songs.length})</h2>
        <div style="display:flex;flex-direction:column;gap:8px">
          ${songs.map((s,i) => SongRow(s,i)).join('') || emptyState('No tracks linked.')}
        </div>
      </div>
    </div>`;
}

async function renderProfile() {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }

  document.getElementById('app').innerHTML = `<div class="container"><div class="skeleton" style="height:120px;margin-top:40px;border-radius:16px"></div></div>`;

  const meRes = await api.me();
  if (meRes.status === 401) { setToken(null); setUser(null); renderAuthArea(); Router.navigate('/login'); return; }
  if (meRes.status === 200) setUser({ ...u, ...meRes.records[0] });

  const user = getUser();
  const initials = user.username ? user.username[0].toUpperCase() : '?';
  const avatarHtml = user.avatar_url
    ? `<img src="${user.avatar_url}" alt="${user.username}" onerror="this.replaceWith(document.createTextNode('${initials}'))" />`
    : initials;
  const genreChips = (user.fav_genres||'').split(',').filter(Boolean)
    .map(g => `<span class="tag" style="font-size:.75rem">${g.trim()}</span>`).join('');

  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="profile-header">
        <div class="profile-avatar" style="background:${user.accent_color||'#f7c948'}">${avatarHtml}</div>
        <div style="flex:1">
          <div class="profile-name">${user.username}</div>
          ${user.bio ? `<div class="profile-bio">${user.bio}</div>` : ''}
          ${genreChips ? `<div style="display:flex;flex-wrap:wrap;gap:6px;margin-top:8px">${genreChips}</div>` : ''}
          <div class="profile-actions">
            <button class="btn btn-ghost" onclick="showEditProfile()">Edit Profile</button>
            <button class="btn btn-ghost" onclick="doLogout()">Log out</button>
          </div>
        </div>
      </div>
      <div class="profile-tabs">
        <button class="profile-tab active" onclick="showTab('fav-books',this)">Favourite Books</button>
        <button class="profile-tab" onclick="showTab('fav-songs',this)">Favourite Songs</button>
        <button class="profile-tab" onclick="showTab('collection',this)">Collection</button>
        <button class="profile-tab" onclick="showTab('cart',this)">Cart</button>
        <button class="profile-tab" onclick="showTab('orders',this)">Orders</button>
      </div>
      <div id="tab-content"></div>
    </div>`;

  showTab('fav-books', document.querySelector('.profile-tab.active'));
}

async function showTab(tab, btn) {
  document.querySelectorAll('.profile-tab').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');
  const el = document.getElementById('tab-content');
  const usr = getUser();

  if (tab === 'fav-books') {
    const ids = (usr.fav_books||'').split(',').filter(Boolean);
    if (!ids.length) { el.innerHTML = emptyState('No favourite books yet.'); return; }
    const items = await Promise.all(ids.map(id => api.book(id)));
    el.innerHTML = `<div class="grid-3">${items.map(r => r.records[0] ? BookCard(r.records[0]) : '').join('')}</div>`;
  }
  else if (tab === 'fav-songs') {
    const ids = (usr.fav_songs||'').split(',').filter(Boolean);
    if (!ids.length) { el.innerHTML = emptyState('No favourite songs yet.'); return; }
    const items = await Promise.all(ids.map(id => api.song(id)));
    const songs = items.map(r => r.records[0]).filter(Boolean);
    el.innerHTML = `<div style="display:flex;flex-direction:column;gap:8px">${songs.map((s,i) => SongRow(s,i)).join('')}</div>`;
  }
  else if (tab === 'collection') {
    const ids = (usr.collection||'').split(',').filter(Boolean);
    if (!ids.length) { el.innerHTML = emptyState('Your vinyl collection is empty.'); return; }
    const items = await Promise.all(ids.map(id => api.vinyl(id)));
    el.innerHTML = `<div class="grid-4">${items.map(r => r.records[0] ? VinylCard(r.records[0]) : '').join('')}</div>`;
  }
  else if (tab === 'cart') {
    const ids = (usr.cart||'').split(',').filter(Boolean);
    if (!ids.length) { el.innerHTML = emptyState('Your cart is empty.'); return; }
    const items = await Promise.all(ids.map(id => api.vinyl(id)));
    const vinyls = items.map(r => r.records[0]).filter(Boolean);
    const total  = vinyls.reduce((s,v) => s + parseInt(v.price_cents||0), 0);
    el.innerHTML = `
      ${vinyls.map(v => `
        <div class="vinyl-cart-item">
          <div class="vinyl-cart-img">${cover_img(v.image_url,'💿')}</div>
          <div class="vinyl-cart-info">
            <div class="vinyl-cart-title">${v.title}</div>
            <div class="vinyl-cart-artist">${v.artist} · <span class="tag ${condition_class(v.condition)}">${v.condition}</span></div>
          </div>
          <span class="price-small">${fmt_price(v.price_cents)}</span>
          <button class="btn btn-ghost" style="padding:6px 12px;font-size:.8rem" onclick="toggleCart('${v.id}',this,true)">Remove</button>
        </div>`).join('')}
      <div class="cart-total">
        <span style="color:var(--muted)">Total:</span>
        <span class="price">${fmt_price(total)}</span>
        <button class="btn btn-primary" onclick="startCheckout()">Checkout</button>
      </div>`;
  }
  else if (tab === 'orders') {
    el.innerHTML = `<div class="skeleton" style="height:120px;border-radius:12px"></div>`;
    const r = await api.orders();
    if (!r.records.length) { el.innerHTML = emptyState('No orders yet.'); return; }
    el.innerHTML = r.records.map(o => `
      <div class="order-card">
        <div class="order-header">
          <div>
            <div class="order-num">Order #${String(o.order_id).padStart(6,'0')}</div>
            <div class="order-date">${o.created_at}</div>
          </div>
          <div class="order-total">${fmt_price(o.total_cents)}</div>
        </div>
        <div class="order-items">${o.titles}</div>
        <div class="order-footer">
          <span class="tag tag-green">Paid · **** ${o.card_last4}</span>
          <span style="color:var(--muted);font-size:.8rem">${o.item_count} item${o.item_count!=='1'?'s':''}</span>
        </div>
      </div>`).join('');
  }
}

function showEditProfile() {
  const usr = getUser();
  const el = document.getElementById('tab-content');
  if (!el) return;
  el.innerHTML = `
    <div style="max-width:480px">
      <h3 style="margin-bottom:20px">Edit Profile</h3>
      <div class="form-group">
        <label class="form-label">Username</label>
        <input class="form-input" id="ep-username" value="${usr.username||''}" />
      </div>
      <div class="form-group">
        <label class="form-label">Bio</label>
        <textarea class="form-input" id="ep-bio">${usr.bio||''}</textarea>
      </div>
      <div class="form-group">
        <label class="form-label">Avatar URL</label>
        <input class="form-input" id="ep-avatar" value="${usr.avatar_url||''}" placeholder="https://... or /avatars/your-image.png" oninput="updateAvatarPreview()" />
        <div class="avatar-preview" id="avatar-preview">
          ${usr.avatar_url ? `<img src="${usr.avatar_url}" alt="Avatar preview" onerror="this.replaceWith(document.createTextNode('Preview unavailable'))" />` : `<span class="avatar-placeholder">Preview</span>`}
        </div>
      </div>
      <div class="form-group">
        <label class="form-label">Favourite Genres <span style="color:var(--muted);font-size:.8rem">(comma-separated)</span></label>
        <input class="form-input" id="ep-genres" value="${usr.fav_genres||''}" placeholder="Jazz, Classic Rock, Folk, Soul" />
      </div>
      <div class="form-group">
        <label class="form-label">Accent colour</label>
        <div class="color-swatches">
          ${['#f7c948','#e07850','#48c78e','#6aa0ff','#c06cdd','#f14668'].map(c =>
            `<div class="swatch${usr.accent_color===c?' selected':''}" style="background:${c}" onclick="pickColor('${c}',this)"></div>`
          ).join('')}
        </div>
      </div>
      <div style="display:flex;gap:10px;margin-top:8px">
        <button class="btn btn-primary" onclick="saveProfile()">Save</button>
        <button class="btn btn-ghost" onclick="showTab('fav-books',document.querySelector('.profile-tab'))">Cancel</button>
      </div>
    </div>`;
}

window.pickColor = function(c, el) {
  document.querySelectorAll('.swatch').forEach(s => s.classList.remove('selected'));
  el.classList.add('selected');
};

window.updateAvatarPreview = function() {
  const preview = document.getElementById('avatar-preview');
  const input = document.getElementById('ep-avatar');
  if (!preview || !input) return;
  const url = input.value.trim();
  preview.innerHTML = '';
  if (!url) {
    preview.textContent = 'Preview';
    return;
  }
  const img = document.createElement('img');
  img.src = url;
  img.alt = 'Avatar preview';
  img.onerror = () => { preview.textContent = 'Preview unavailable'; };
  preview.appendChild(img);
};

async function saveProfile() {
  const usr = getUser();
  const data = {
    username:     document.getElementById('ep-username').value,
    bio:          document.getElementById('ep-bio').value,
    avatar_url:   document.getElementById('ep-avatar').value,
    fav_genres:   document.getElementById('ep-genres').value,
    accent_color: document.querySelector('.swatch.selected')?.style.background || usr.accent_color || '#f7c948'
  };
  const r = await api.updateMe(data);
  if (r.status === 200) {
    setUser({ ...usr, ...r.records[0] });
    renderAuthArea();
    toast('Profile updated!', 'success');
    renderProfile();
  } else {
    toast(r.message || 'Update failed', 'error');
  }
}

async function renderLogin() {
  document.getElementById('app').innerHTML = `
    <div class="auth-page">
      <div class="auth-card">
        <h2>Welcome back</h2>
        <p>Sign in to your Books &amp; Songs account.</p>
        <div class="form-group">
          <label class="form-label">Email</label>
          <input class="form-input" id="login-email" type="email" placeholder="you@example.com" />
        </div>
        <div class="form-group">
          <label class="form-label">Password</label>
          <input class="form-input" id="login-pw" type="password" placeholder="••••••••" />
        </div>
        <div id="login-err" class="form-error"></div>
        <button class="btn btn-primary" style="width:100%;justify-content:center;margin-top:8px" onclick="doLogin()">Log in</button>
        <div class="form-footer">No account? <a href="#/register">Sign up</a></div>
      </div>
    </div>`;

  document.getElementById('login-pw').addEventListener('keydown', e => { if (e.key === 'Enter') doLogin(); });
}

async function renderRegister() {
  document.getElementById('app').innerHTML = `
    <div class="auth-page">
      <div class="auth-card">
        <h2>Join Books &amp; Songs</h2>
        <p>Create your account to save favourites and build your vinyl collection.</p>
        <div class="form-group">
          <label class="form-label">Username</label>
          <input class="form-input" id="reg-name" placeholder="yourname" />
        </div>
        <div class="form-group">
          <label class="form-label">Email</label>
          <input class="form-input" id="reg-email" type="email" placeholder="you@example.com" />
        </div>
        <div class="form-group">
          <label class="form-label">Password</label>
          <input class="form-input" id="reg-pw" type="password" placeholder="Min 6 characters" />
        </div>
        <div id="reg-err" class="form-error"></div>
        <button class="btn btn-primary" style="width:100%;justify-content:center;margin-top:8px" onclick="doRegister()">Create account</button>
        <div class="form-footer">Already have an account? <a href="#/login">Log in</a></div>
      </div>
    </div>`;
}

// ── Auth actions ──────────────────────────────────────────────────────────────

async function doLogin() {
  const email = document.getElementById('login-email').value.trim();
  const password = document.getElementById('login-pw').value;
  const errEl = document.getElementById('login-err');
  if (!email || !password) { errEl.textContent = 'Please fill in all fields.'; return; }
  const r = await api.login({ email, password });
  if (r.status === 200) {
    const u = r.records[0];
    setToken(u.token);
    setUser(u);
    renderAuthArea();
    toast(`Welcome back, ${u.username}!`, 'success');
    Router.navigate('/');
  } else {
    errEl.textContent = r.message || 'Login failed.';
  }
}

async function doRegister() {
  const username = document.getElementById('reg-name').value.trim();
  const email    = document.getElementById('reg-email').value.trim();
  const password = document.getElementById('reg-pw').value;
  const errEl    = document.getElementById('reg-err');
  if (!username || !email || !password) { errEl.textContent = 'Please fill in all fields.'; return; }
  if (password.length < 6) { errEl.textContent = 'Password must be at least 6 characters.'; return; }
  const r = await api.register({ username, email, password });
  if (r.status === 200) {
    const u = r.records[0];
    setToken(u.token);
    setUser(u);
    renderAuthArea();
    toast(`Welcome, ${u.username}!`, 'success');
    Router.navigate('/');
  } else {
    errEl.textContent = r.message || 'Registration failed.';
  }
}

async function doLogout() {
  await api.logout();
  setToken(null);
  setUser(null);
  renderAuthArea();
  toast('Logged out.', 'info');
  Router.navigate('/');
}

// ── Interaction helpers ───────────────────────────────────────────────────────

async function toggleFavBook(id, btn) {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }
  const ids = (u.fav_books || '').split(',').filter(Boolean);
  const isFav = ids.includes(String(id));
  const r = await api.favBook(id, !isFav);
  if (r.status === 200) {
    const newIds = isFav ? ids.filter(x => x !== String(id)) : [...ids, String(id)];
    setUser({ ...u, fav_books: newIds.join(',') });
    btn.className = `btn btn-${!isFav?'primary':'outline'}`;
    btn.textContent = !isFav ? '♥ Saved' : '♡ Save';
    toast(!isFav ? 'Added to favourites!' : 'Removed from favourites.', 'success');
  }
}

async function toggleFavSong(id, btn) {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }
  const ids = (u.fav_songs || '').split(',').filter(Boolean);
  const isFav = ids.includes(String(id));
  const r = await api.favSong(id, !isFav);
  if (r.status === 200) {
    const newIds = isFav ? ids.filter(x => x !== String(id)) : [...ids, String(id)];
    setUser({ ...u, fav_songs: newIds.join(',') });
    btn.className = `btn btn-${!isFav?'primary':'outline'}`;
    btn.textContent = !isFav ? '♥ Saved' : '♡ Save';
    toast(!isFav ? 'Added to favourites!' : 'Removed from favourites.', 'success');
  }
}

async function toggleCart(id, btn, refresh = false) {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }
  const ids = (u.cart || '').split(',').filter(Boolean);
  const inCart = ids.includes(String(id));
  const r = await api.cart(id, !inCart);
  if (r.status === 200) {
    const newIds = inCart ? ids.filter(x => x !== String(id)) : [...ids, String(id)];
    setUser({ ...u, cart: newIds.join(',') });
    if (refresh) renderProfile();
    else {
      btn.className   = `btn btn-${!inCart?'danger':'primary'}`;
      btn.textContent = !inCart ? '✕ Remove from Cart' : '🛒 Add to Cart';
    }
    toast(!inCart ? 'Added to cart!' : 'Removed from cart.', 'success');
  }
}

async function toggleCollection(id, btn) {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }
  const ids = (u.collection || '').split(',').filter(Boolean);
  const inColl = ids.includes(String(id));
  const r = await api.collection(id, !inColl);
  if (r.status === 200) {
    const newIds = inColl ? ids.filter(x => x !== String(id)) : [...ids, String(id)];
    setUser({ ...u, collection: newIds.join(',') });
    btn.className   = `btn btn-${!inColl?'ghost':'outline'}`;
    btn.textContent = !inColl ? '✓ In Collection' : '+ Add to Collection';
    toast(!inColl ? 'Added to collection!' : 'Removed from collection.', 'success');
  }
}

// ── Checkout ──────────────────────────────────────────────────────────────────

async function startCheckout() {
  const u = getUser();
  if (!u) { Router.navigate('/login'); return; }

  // Load cart items for the review step
  const ids = (u.cart||'').split(',').filter(Boolean);
  if (!ids.length) { toast('Your cart is empty.', 'info'); return; }
  const items = await Promise.all(ids.map(id => api.vinyl(id)));
  const vinyls = items.map(r => r.records[0]).filter(Boolean);
  const total  = vinyls.reduce((s,v) => s + parseInt(v.price_cents||0), 0);

  const orderLines = vinyls.map(v => `
    <div class="checkout-item">
      <div class="checkout-item-img">${cover_img(v.image_url,'💿')}</div>
      <div class="checkout-item-info">
        <div class="checkout-item-title">${v.title}</div>
        <div class="checkout-item-sub">${v.artist} · <span class="tag ${condition_class(v.condition)}">${v.condition}</span></div>
      </div>
      <span class="price-small">${fmt_price(v.price_cents)}</span>
    </div>`).join('');

  showModal('Checkout', `
    <div class="checkout-wrap">
      <div class="checkout-section-label">Order summary</div>
      ${orderLines}
      <div class="checkout-total-row">
        <span>Total</span>
        <span class="checkout-total-price">${fmt_price(total)}</span>
      </div>

      <div class="checkout-section-label" style="margin-top:24px">Payment details</div>
      <div class="form-group">
        <label class="form-label">Cardholder name</label>
        <input class="form-input" id="co-name" placeholder="Jane Doe" autocomplete="cc-name" />
      </div>
      <div class="form-group">
        <label class="form-label">Card number</label>
        <div class="card-input-wrap">
          <input class="form-input card-number-input" id="co-number" placeholder="1234 5678 9012 3456"
            maxlength="19" autocomplete="cc-number" oninput="fmtCard(this)" />
          <span class="card-brand" id="card-brand">💳</span>
        </div>
      </div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
        <div class="form-group">
          <label class="form-label">Expiry (MM/YY)</label>
          <input class="form-input" id="co-expiry" placeholder="12/27" maxlength="5"
            autocomplete="cc-exp" oninput="fmtExpiry(this)" />
        </div>
        <div class="form-group">
          <label class="form-label">CVV</label>
          <input class="form-input" id="co-cvv" placeholder="123" maxlength="4"
            type="password" autocomplete="cc-csc" />
        </div>
      </div>
      <div class="checkout-secure-note">🔒 Demo mode — no real charge is made</div>
    </div>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" id="pay-btn" onclick="submitCheckout()">Pay ${fmt_price(total)}</button>`
  );
}

window.fmtCard = function(inp) {
  let v = inp.value.replace(/\D/g,'').slice(0,16);
  inp.value = v.replace(/(.{4})/g,'$1 ').trim();
  const brand = document.getElementById('card-brand');
  if (brand) {
    if (v.startsWith('4')) brand.textContent = '💳 Visa';
    else if (/^5[1-5]/.test(v)) brand.textContent = '💳 MC';
    else if (/^3[47]/.test(v)) brand.textContent = '💳 Amex';
    else brand.textContent = '💳';
  }
};

window.fmtExpiry = function(inp) {
  let v = inp.value.replace(/\D/g,'').slice(0,4);
  if (v.length >= 3) v = v.slice(0,2) + '/' + v.slice(2);
  inp.value = v;
};

async function submitCheckout() {
  const btn = document.getElementById('pay-btn');
  if (!btn) return;

  const name   = document.getElementById('co-name').value.trim();
  const number = document.getElementById('co-number').value.replace(/\s/g,'');
  const expiry = document.getElementById('co-expiry').value.trim();
  const cvv    = document.getElementById('co-cvv').value.trim();

  if (!name)               { toast('Enter cardholder name',   'error'); return; }
  if (number.length < 13)  { toast('Enter a valid card number','error'); return; }
  if (expiry.length < 4)   { toast('Enter expiry date',       'error'); return; }
  if (cvv.length < 3)      { toast('Enter CVV',               'error'); return; }

  btn.disabled = true;
  btn.textContent = 'Processing…';

  const r = await api.checkout({
    card_name:   name,
    card_number: number,
    card_expiry: expiry,
    card_cvv:    cvv,
  });

  btn.disabled = false;

  if (r.status !== 200) {
    btn.textContent = 'Try again';
    toast(r.message || 'Payment failed', 'error');
    return;
  }

  const o = r.records[0];
  const u = getUser();
  setUser({ ...u, cart: '' });

  closeModal();
  showOrderConfirmation(o);
}

function showOrderConfirmation(o) {
  const orderNum = String(o.order_id).padStart(6,'0');
  document.getElementById('app').innerHTML = `
    <div class="container">
      <div class="confirmation-wrap">
        <div class="confirmation-icon">✓</div>
        <h1 class="confirmation-title">Order placed!</h1>
        <p class="confirmation-sub">Order <strong>#${orderNum}</strong> confirmed</p>
        <div class="confirmation-details">
          <div class="conf-row"><span>Total charged</span><span><strong>${fmt_price(o.total_cents)}</strong></span></div>
          <div class="conf-row"><span>Card</span><span>**** **** **** ${o.card_last4}</span></div>
          <div class="conf-row"><span>Receipt sent to</span><span>${o.email}</span></div>
          <div class="conf-row"><span>Date</span><span>${o.created_at}</span></div>
        </div>
        <div class="confirmation-receipt">
          <div class="conf-receipt-label">Receipt</div>
          <pre class="conf-receipt-text">${o.receipt}</pre>
        </div>
        <div style="display:flex;gap:12px;justify-content:center;margin-top:32px">
          <button class="btn btn-primary" onclick="Router.navigate('/vinyls')">Continue Shopping</button>
          <button class="btn btn-ghost" onclick="Router.navigate('/profile')">View Orders</button>
        </div>
      </div>
    </div>`;
}

// ── Helper templates ──────────────────────────────────────────────────────────

function emptyState(msg) {
  return `<div class="empty-state"><div class="icon">🎵</div><h3>${msg}</h3></div>`;
}
function errorPage(msg) {
  return `<div class="container"><div class="empty-state"><div class="icon">😕</div><h3>${msg || 'Something went wrong'}</h3><button class="btn btn-ghost" onclick="history.back()">Go Back</button></div></div>`;
}

// ── Search overlay ────────────────────────────────────────────────────────────

const searchOverlay = document.getElementById('search-overlay');
const searchInput   = document.getElementById('search-input');
const searchResults = document.getElementById('search-results');

document.querySelector('.search-toggle').addEventListener('click', () => {
  searchOverlay.classList.remove('hidden');
  searchInput.value = '';
  searchResults.innerHTML = '';
  searchInput.focus();
});
document.getElementById('search-close').addEventListener('click', () => {
  searchOverlay.classList.add('hidden');
});
searchOverlay.addEventListener('click', e => {
  if (e.target === searchOverlay) searchOverlay.classList.add('hidden');
});

let searchTimeout;
searchInput.addEventListener('input', () => {
  clearTimeout(searchTimeout);
  const q = searchInput.value.trim();
  if (!q) { searchResults.innerHTML = ''; return; }
  searchTimeout = setTimeout(async () => {
    const r = await api.search(q);
    if (r.status !== 200) return;
    searchResults.innerHTML = r.records.map(item => {
      const icon = item.type === 'book' ? '📖' : item.type === 'song' ? '♪' : '💿';
      const thumb = item.cover_url
        ? `<img src="${item.cover_url}" alt="" style="width:100%;height:100%;object-fit:cover;border-radius:6px" />`
        : icon;
      return `
        <div class="search-result-item" onclick="goTo('${item.type}','${item.id}')">
          <div class="search-result-thumb">${thumb}</div>
          <div class="search-result-info">
            <div class="search-result-title">${item.title}</div>
            <div class="search-result-sub">${item.subtitle}</div>
          </div>
          <span class="search-result-type">${item.type}</span>
        </div>`;
    }).join('') || '<div style="padding:20px;color:var(--muted);text-align:center">No results</div>';
  }, 300);
});

function goTo(type, id) {
  searchOverlay.classList.add('hidden');
  Router.navigate(`/${type}s/${id}`);
}

// ── Bootstrap ─────────────────────────────────────────────────────────────────

Router.add('/',              renderHome);
Router.add('/books',         renderBooks);
Router.add('/books/:id',     renderBook);
Router.add('/songs',         renderSongs);
Router.add('/songs/:id',     renderSong);
Router.add('/vinyls',        renderVinyls);
Router.add('/vinyls/:id',    renderVinyl);
Router.add('/profile',       renderProfile);
Router.add('/login',         renderLogin);
Router.add('/register',      renderRegister);
Router.add('/admin',         renderAdmin);
Router.add('/admin/:section',renderAdmin);

renderAuthArea();
Router.init();

// ── Modal system ──────────────────────────────────────────────────────────────

function showModal(title, bodyHtml, footerHtml) {
  document.getElementById('modal-title').textContent = title;
  document.getElementById('modal-body').innerHTML   = bodyHtml;
  document.getElementById('modal-footer').innerHTML = footerHtml;
  document.getElementById('modal-overlay').classList.remove('hidden');
}
function closeModal() {
  document.getElementById('modal-overlay').classList.add('hidden');
}
document.getElementById('modal-overlay').addEventListener('click', e => {
  if (e.target === document.getElementById('modal-overlay')) closeModal();
});

// ── Admin Panel ───────────────────────────────────────────────────────────────

async function renderAdmin(params = {}) {
  const u = getUser();
  if (!u || u.is_admin !== '1') {
    document.getElementById('app').innerHTML = `
      <div class="container"><div class="empty-state">
        <div class="icon">🔒</div><h3>Admin access only</h3>
        <button class="btn btn-primary" onclick="Router.navigate('/login')">Log in</button>
      </div></div>`;
    return;
  }

  const section = params.section || 'dashboard';

  const navLinks = [
    { id: 'dashboard', icon: '📊', label: 'Dashboard'  },
    { id: 'books',     icon: '📖', label: 'Books'      },
    { id: 'songs',     icon: '♪',  label: 'Songs'      },
    { id: 'vinyls',    icon: '💿', label: 'Vinyls'     },
    { id: 'users',     icon: '👥', label: 'Users'      },
    { id: 'moments',   icon: '🎯', label: 'Moments'    },
  ];

  document.getElementById('app').innerHTML = `
    <div class="admin-layout">
      <aside class="admin-sidebar">
        <div class="admin-sidebar-title">Admin Panel</div>
        ${navLinks.map(n => `
          <div class="admin-nav-link ${section===n.id?'active':''}" onclick="Router.navigate('/admin/${n.id}')">
            <span class="icon">${n.icon}</span>${n.label}
          </div>`).join('')}
        <div style="margin-top:auto;padding:20px 20px 0">
          <button class="btn btn-ghost" style="width:100%;justify-content:center" onclick="Router.navigate('/')">← Back to site</button>
        </div>
      </aside>
      <div class="admin-content" id="admin-main"></div>
    </div>`;

  const sections = { dashboard: adminDashboard, books: adminBooks, songs: adminSongs, vinyls: adminVinyls, users: adminUsers, moments: adminMoments };
  const fn = sections[section] || adminDashboard;
  await fn();
}

async function adminDashboard() {
  const r = await api.adminStats();
  if (r.status !== 200) { document.getElementById('admin-main').innerHTML = errorPage(r.message); return; }
  const s = r.records[0];
  document.getElementById('admin-main').innerHTML = `
    <h2 style="margin-bottom:24px;font-size:1.5rem">Dashboard</h2>
    <div class="stats-grid">
      <div class="stat-card" onclick="Router.navigate('/admin/books')" style="cursor:pointer">
        <div class="stat-num">${s.books}</div>
        <div class="stat-label">Books</div>
      </div>
      <div class="stat-card" onclick="Router.navigate('/admin/songs')" style="cursor:pointer">
        <div class="stat-num">${s.songs}</div>
        <div class="stat-label">Songs</div>
      </div>
      <div class="stat-card" onclick="Router.navigate('/admin/vinyls')" style="cursor:pointer">
        <div class="stat-num">${s.vinyls}</div>
        <div class="stat-label">Vinyl Records</div>
      </div>
      <div class="stat-card" onclick="Router.navigate('/admin/users')" style="cursor:pointer">
        <div class="stat-num">${s.users}</div>
        <div class="stat-label">Users</div>
      </div>
    </div>
    <div style="background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:24px">
      <h3 style="margin-bottom:16px">Quick actions</h3>
      <div style="display:flex;gap:10px;flex-wrap:wrap">
        <button class="btn btn-primary" onclick="openAddBook()">+ Add Book</button>
        <button class="btn btn-outline" onclick="openAddSong()">+ Add Song</button>
        <button class="btn btn-outline" onclick="openAddVinyl()">+ Add Vinyl</button>
      </div>
    </div>`;
}

// ── Admin: Books ──────────────────────────────────────────────────────────────

async function adminBooks() {
  const r = await api.books();
  const rows = r.status === 200 ? r.records : [];
  rows.forEach(b => { window._reg[`b_${b.id}`] = b; });
  document.getElementById('admin-main').innerHTML = `
    <div class="admin-toolbar">
      <h2>Books</h2>
      <button class="btn btn-primary" onclick="openAddBook()">+ Add Book</button>
    </div>
    <div class="data-table-wrap">
      <table class="data-table">
        <thead><tr>
          <th>ID</th><th>Title</th><th>Author</th><th>Year</th><th>Genre</th><th>Songs</th><th>Actions</th>
        </tr></thead>
        <tbody>
          ${rows.map(b => `<tr>
            <td style="color:var(--muted)">#${b.id}</td>
            <td><strong>${b.title}</strong></td>
            <td>${b.author}</td>
            <td>${b.year}</td>
            <td><span class="tag">${b.genre}</span></td>
            <td>${b.song_count}</td>
            <td><div class="actions">
              <button class="btn btn-ghost" style="padding:5px 12px;font-size:.8rem" onclick="openEditBook('b_${b.id}')">Edit</button>
              <button class="btn btn-danger"  style="padding:5px 12px;font-size:.8rem" onclick="confirmDeleteBook(${b.id},'${b.title.replace(/'/g,"\\'")}')">Delete</button>
            </div></td>
          </tr>`).join('')}
        </tbody>
      </table>
    </div>`;
}

function bookFormHtml(b = {}) {
  return `
    <div class="form-group"><label class="form-label">Title *</label>
      <input class="form-input" id="f-title" value="${b.title||''}" /></div>
    <div class="form-group"><label class="form-label">Author</label>
      <input class="form-input" id="f-author" value="${b.author||''}" /></div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Year</label>
        <input class="form-input" id="f-year" type="number" value="${b.year||''}" /></div>
      <div class="form-group"><label class="form-label">Genre</label>
        <input class="form-input" id="f-genre" value="${b.genre||''}" /></div>
    </div>
    <div class="form-group"><label class="form-label">Description</label>
      <textarea class="form-input" id="f-desc">${b.description||''}</textarea></div>
    <div class="form-group"><label class="form-label">Cover URL</label>
      <input class="form-input" id="f-cover" value="${b.cover_url||''}" placeholder="/covers/filename.jpg" /></div>
    <div class="form-group"><label class="form-label">Song IDs (comma-separated)</label>
      <input class="form-input" id="f-song-ids" value="${b.song_ids||''}" placeholder="1,2,3" /></div>
    <div class="form-group"><label class="form-label">ISBN</label>
      <input class="form-input" id="f-isbn" value="${b.isbn||''}" /></div>`;
}

function collectBookForm() {
  return {
    title:       document.getElementById('f-title').value.trim(),
    author:      document.getElementById('f-author').value.trim(),
    year:        document.getElementById('f-year').value.trim(),
    genre:       document.getElementById('f-genre').value.trim(),
    description: document.getElementById('f-desc').value.trim(),
    cover_url:   document.getElementById('f-cover').value.trim(),
    song_ids:    document.getElementById('f-song-ids').value.trim(),
    isbn:        document.getElementById('f-isbn').value.trim(),
  };
}

window.openAddBook = function() {
  showModal('Add Book', bookFormHtml(), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveBook(null)">Save Book</button>`);
};
window.openEditBook = function(key) {
  const b = window._reg[key] || {};
  showModal('Edit Book', bookFormHtml(b), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveBook(${b.id})">Save Changes</button>`);
};
window.saveBook = async function(id) {
  const data = collectBookForm();
  if (!data.title) { toast('Title is required', 'error'); return; }
  const r = id ? await api.adminUpdateBook(id, data) : await api.adminCreateBook(data);
  if (r.status === 200) {
    closeModal(); toast(id ? 'Book updated!' : 'Book created!', 'success'); adminBooks();
  } else { toast(r.message || 'Error', 'error'); }
};
window.confirmDeleteBook = function(id, title) {
  showModal('Delete Book', `<p>Delete <strong>${title}</strong>? This cannot be undone.</p>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-danger" onclick="deleteBook(${id})">Delete</button>`);
};
window.deleteBook = async function(id) {
  const r = await api.adminDeleteBook(id);
  if (r.status === 200) { closeModal(); toast('Book deleted.', 'info'); adminBooks(); }
  else toast(r.message || 'Error', 'error');
};

// ── Admin: Songs ──────────────────────────────────────────────────────────────

async function adminSongs() {
  const r = await api.songs();
  const rows = r.status === 200 ? r.records : [];
  rows.forEach(s => { window._reg[`s_${s.id}`] = s; });
  document.getElementById('admin-main').innerHTML = `
    <div class="admin-toolbar">
      <h2>Songs</h2>
      <button class="btn btn-primary" onclick="openAddSong()">+ Add Song</button>
    </div>
    <div class="data-table-wrap">
      <table class="data-table">
        <thead><tr>
          <th>ID</th><th>Title</th><th>Artist</th><th>Year</th><th>Genre</th><th>Vinyl</th><th>Actions</th>
        </tr></thead>
        <tbody>
          ${rows.map(s => `<tr>
            <td style="color:var(--muted)">#${s.id}</td>
            <td><strong>${s.title}</strong></td>
            <td>${s.artist}</td>
            <td>${s.year}</td>
            <td><span class="tag">${s.genre}</span></td>
            <td>${s.vinyl_id !== '0' ? `#${s.vinyl_id}` : '—'}</td>
            <td><div class="actions">
              <button class="btn btn-ghost" style="padding:5px 12px;font-size:.8rem" onclick="openEditSong('s_${s.id}')">Edit</button>
              <button class="btn btn-danger"  style="padding:5px 12px;font-size:.8rem" onclick="confirmDeleteSong(${s.id},'${s.title.replace(/'/g,"\\'")}')">Delete</button>
            </div></td>
          </tr>`).join('')}
        </tbody>
      </table>
    </div>`;
}

function songFormHtml(s = {}) {
  return `
    <div class="form-group"><label class="form-label">Title *</label>
      <input class="form-input" id="f-title" value="${s.title||''}" /></div>
    <div class="form-group"><label class="form-label">Artist</label>
      <input class="form-input" id="f-artist" value="${s.artist||''}" /></div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Year</label>
        <input class="form-input" id="f-year" type="number" value="${s.year||''}" /></div>
      <div class="form-group"><label class="form-label">Genre</label>
        <input class="form-input" id="f-genre" value="${s.genre||''}" /></div>
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Duration (seconds)</label>
        <input class="form-input" id="f-duration" type="number" value="${s.duration||''}" /></div>
      <div class="form-group"><label class="form-label">Vinyl ID (or 0)</label>
        <input class="form-input" id="f-vinyl-id" type="number" value="${s.vinyl_id||'0'}" /></div>
    </div>
    <div class="form-group"><label class="form-label">Audio URL</label>
      <input class="form-input" id="f-audio" value="${s.audio_url||''}" placeholder="/audio/filename.mp3" /></div>`;
}

function collectSongForm() {
  return {
    title:     document.getElementById('f-title').value.trim(),
    artist:    document.getElementById('f-artist').value.trim(),
    year:      document.getElementById('f-year').value.trim(),
    genre:     document.getElementById('f-genre').value.trim(),
    duration:  document.getElementById('f-duration').value.trim(),
    vinyl_id:  document.getElementById('f-vinyl-id').value.trim() || '0',
    audio_url: document.getElementById('f-audio').value.trim(),
  };
}

window.openAddSong = function() {
  showModal('Add Song', songFormHtml(), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveSong(null)">Save Song</button>`);
};
window.openEditSong = function(key) {
  const s = window._reg[key] || {};
  showModal('Edit Song', songFormHtml(s), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveSong(${s.id})">Save Changes</button>`);
};
window.saveSong = async function(id) {
  const data = collectSongForm();
  if (!data.title) { toast('Title is required', 'error'); return; }
  const r = id ? await api.adminUpdateSong(id, data) : await api.adminCreateSong(data);
  if (r.status === 200) { closeModal(); toast(id ? 'Song updated!' : 'Song created!', 'success'); adminSongs(); }
  else toast(r.message || 'Error', 'error');
};
window.confirmDeleteSong = function(id, title) {
  showModal('Delete Song', `<p>Delete <strong>${title}</strong>?</p>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-danger" onclick="deleteSong(${id})">Delete</button>`);
};
window.deleteSong = async function(id) {
  const r = await api.adminDeleteSong(id);
  if (r.status === 200) { closeModal(); toast('Song deleted.', 'info'); adminSongs(); }
  else toast(r.message || 'Error', 'error');
};

// ── Admin: Vinyls ─────────────────────────────────────────────────────────────

async function adminVinyls() {
  const r = await api.vinyls();
  const rows = r.status === 200 ? r.records : [];
  rows.forEach(v => { window._reg[`v_${v.id}`] = v; });
  document.getElementById('admin-main').innerHTML = `
    <div class="admin-toolbar">
      <h2>Vinyl Records</h2>
      <button class="btn btn-primary" onclick="openAddVinyl()">+ Add Vinyl</button>
    </div>
    <div class="data-table-wrap">
      <table class="data-table">
        <thead><tr>
          <th>ID</th><th>Title</th><th>Artist</th><th>Year</th><th>Condition</th><th>Price</th><th>Stock</th><th>Actions</th>
        </tr></thead>
        <tbody>
          ${rows.map(v => `<tr>
            <td style="color:var(--muted)">#${v.id}</td>
            <td><strong>${v.title}</strong></td>
            <td>${v.artist}</td>
            <td>${v.year}</td>
            <td><span class="tag ${condition_class(v.condition)}">${v.condition}</span></td>
            <td class="price-small">${fmt_price(v.price_cents)}</td>
            <td>${v.in_stock==='1' ? '<span class="tag tag-green">Yes</span>' : '<span class="tag">No</span>'}</td>
            <td><div class="actions">
              <button class="btn btn-ghost" style="padding:5px 12px;font-size:.8rem" onclick="openEditVinyl('v_${v.id}')">Edit</button>
              <button class="btn btn-danger"  style="padding:5px 12px;font-size:.8rem" onclick="confirmDeleteVinyl(${v.id},'${v.title.replace(/'/g,"\\'")}')">Delete</button>
            </div></td>
          </tr>`).join('')}
        </tbody>
      </table>
    </div>`;
}

function vinylFormHtml(v = {}) {
  const conds = ['NM','VG+','VG','G+'];
  return `
    <div class="form-group"><label class="form-label">Title *</label>
      <input class="form-input" id="f-title" value="${v.title||''}" /></div>
    <div class="form-group"><label class="form-label">Artist</label>
      <input class="form-input" id="f-artist" value="${v.artist||''}" /></div>
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Year</label>
        <input class="form-input" id="f-year" type="number" value="${v.year||''}" /></div>
      <div class="form-group"><label class="form-label">Condition</label>
        <select class="form-input" id="f-condition">
          ${conds.map(c=>`<option value="${c}" ${v.condition===c?'selected':''}>${c}</option>`).join('')}
        </select></div>
      <div class="form-group"><label class="form-label">Price (cents)</label>
        <input class="form-input" id="f-price" type="number" value="${v.price_cents||''}" placeholder="4999 = $49.99" /></div>
    </div>
    <div class="form-group"><label class="form-label">Label</label>
      <input class="form-input" id="f-label" value="${v.label||''}" /></div>
    <div class="form-group"><label class="form-label">Image URL</label>
      <input class="form-input" id="f-image" value="${v.image_url||''}" placeholder="/covers/filename.jpg" /></div>
    <div class="form-group"><label class="form-label">Song IDs (comma-separated)</label>
      <input class="form-input" id="f-song-ids" value="${v.song_ids||''}" placeholder="1,2,3" /></div>
    <div class="form-group"><label class="form-label">In Stock</label>
      <select class="form-input" id="f-stock">
        <option value="1" ${v.in_stock!=='0'?'selected':''}>Yes</option>
        <option value="0" ${v.in_stock==='0'?'selected':''}>No</option>
      </select></div>`;
}

function collectVinylForm() {
  return {
    title:       document.getElementById('f-title').value.trim(),
    artist:      document.getElementById('f-artist').value.trim(),
    year:        document.getElementById('f-year').value.trim(),
    condition:   document.getElementById('f-condition').value,
    price_cents: document.getElementById('f-price').value.trim(),
    label:       document.getElementById('f-label').value.trim(),
    image_url:   document.getElementById('f-image').value.trim(),
    song_ids:    document.getElementById('f-song-ids').value.trim(),
    in_stock:    document.getElementById('f-stock').value,
  };
}

window.openAddVinyl = function() {
  showModal('Add Vinyl', vinylFormHtml(), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveVinyl(null)">Save Vinyl</button>`);
};
window.openEditVinyl = function(key) {
  const v = window._reg[key] || {};
  showModal('Edit Vinyl', vinylFormHtml(v), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveVinyl(${v.id})">Save Changes</button>`);
};
window.saveVinyl = async function(id) {
  const data = collectVinylForm();
  if (!data.title) { toast('Title is required', 'error'); return; }
  const r = id ? await api.adminUpdateVinyl(id, data) : await api.adminCreateVinyl(data);
  if (r.status === 200) { closeModal(); toast(id ? 'Vinyl updated!' : 'Vinyl created!', 'success'); adminVinyls(); }
  else toast(r.message || 'Error', 'error');
};
window.confirmDeleteVinyl = function(id, title) {
  showModal('Delete Vinyl', `<p>Delete <strong>${title}</strong>?</p>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-danger" onclick="deleteVinyl(${id})">Delete</button>`);
};
window.deleteVinyl = async function(id) {
  const r = await api.adminDeleteVinyl(id);
  if (r.status === 200) { closeModal(); toast('Vinyl deleted.', 'info'); adminVinyls(); }
  else toast(r.message || 'Error', 'error');
};

// ── Admin: Users ──────────────────────────────────────────────────────────────

async function adminUsers() {
  const r = await api.adminUsers();
  const rows = r.status === 200 ? r.records : [];
  document.getElementById('admin-main').innerHTML = `
    <div class="admin-toolbar"><h2>Users</h2></div>
    <div class="data-table-wrap">
      <table class="data-table">
        <thead><tr>
          <th>ID</th><th>Username</th><th>Email</th><th>Role</th><th>Actions</th>
        </tr></thead>
        <tbody>
          ${rows.map(u => `<tr>
            <td style="color:var(--muted)">#${u.id}</td>
            <td><strong>${u.username}</strong></td>
            <td>${u.email}</td>
            <td>${u.is_admin==='1' ? '<span class="admin-badge">admin</span>' : '<span class="tag">user</span>'}</td>
            <td>
              ${u.is_admin==='1' ? '<span style="color:var(--muted);font-size:.8rem">Protected</span>'
                : `<button class="btn btn-danger" style="padding:5px 12px;font-size:.8rem" onclick="confirmDeleteUser(${u.id},'${u.username.replace(/'/g,"\\'")}')">Delete</button>`}
            </td>
          </tr>`).join('')}
        </tbody>
      </table>
    </div>`;
}

window.confirmDeleteUser = function(id, username) {
  showModal('Delete User', `<p>Delete user <strong>${username}</strong>? All their data will be lost.</p>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-danger" onclick="deleteUser(${id})">Delete User</button>`);
};
window.deleteUser = async function(id) {
  const r = await api.adminDeleteUser(id);
  if (r.status === 200) { closeModal(); toast('User deleted.', 'info'); adminUsers(); }
  else toast(r.message || 'Error', 'error');
};

// ── Admin: Moments ────────────────────────────────────────────────────────────

async function adminMoments() {
  const r = await api.adminGetMoments();
  const rows = r.status === 200 ? r.records : [];
  rows.forEach(m => { window._reg[`m_${m.id}`] = m; });
  document.getElementById('admin-main').innerHTML = `
    <div class="admin-toolbar">
      <h2>Musical Moments</h2>
      <button class="btn btn-primary" onclick="openAddMoment()">+ Add Moment</button>
    </div>
    <p style="color:var(--muted);font-size:.85rem;margin-bottom:16px">Flag exactly where in a book a song appears — chapter, page, scene, and verbatim quote.</p>
    <div class="data-table-wrap">
      <table class="data-table">
        <thead><tr>
          <th>ID</th><th>Book</th><th>Song</th><th>Ch.</th><th>Page</th><th>Scene</th><th>Actions</th>
        </tr></thead>
        <tbody>
          ${rows.length ? rows.map(m => `<tr>
            <td style="color:var(--muted)">#${m.id}</td>
            <td>${m.book_title||`Book #${m.book_id}`}</td>
            <td>${m.song_title||`Song #${m.song_id}`}</td>
            <td>${m.chapter !== '0' ? m.chapter : '—'}</td>
            <td>${m.page !== '0' ? m.page : '—'}</td>
            <td style="max-width:200px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis">${m.scene||'—'}</td>
            <td><div class="actions">
              <button class="btn btn-ghost" style="padding:5px 12px;font-size:.8rem" onclick="openEditMoment('m_${m.id}')">Edit</button>
              <button class="btn btn-danger" style="padding:5px 12px;font-size:.8rem" onclick="confirmDeleteMoment(${m.id})">Delete</button>
            </div></td>
          </tr>`).join('') : `<tr><td colspan="7" style="text-align:center;color:var(--muted);padding:32px">No moments yet. Add one!</td></tr>`}
        </tbody>
      </table>
    </div>`;
}

function momentFormHtml(m = {}) {
  return `
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Book ID *</label>
        <input class="form-input" id="f-book-id" type="number" value="${m.book_id||''}" /></div>
      <div class="form-group"><label class="form-label">Song ID *</label>
        <input class="form-input" id="f-song-id" type="number" value="${m.song_id||''}" /></div>
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px">
      <div class="form-group"><label class="form-label">Chapter <span style="color:var(--muted)">(0 = unset)</span></label>
        <input class="form-input" id="f-chapter" type="number" value="${m.chapter||'0'}" /></div>
      <div class="form-group"><label class="form-label">Page <span style="color:var(--muted)">(0 = unset)</span></label>
        <input class="form-input" id="f-page" type="number" value="${m.page||'0'}" /></div>
    </div>
    <div class="form-group"><label class="form-label">Scene Description</label>
      <textarea class="form-input" id="f-scene" rows="3" placeholder="Describe what's happening in the story…">${m.scene||''}</textarea></div>
    <div class="form-group"><label class="form-label">Verbatim Quote</label>
      <textarea class="form-input" id="f-quote" rows="3" placeholder="The exact passage from the book…">${m.quote||''}</textarea></div>`;
}

function collectMomentForm() {
  return {
    book_id: document.getElementById('f-book-id').value.trim(),
    song_id: document.getElementById('f-song-id').value.trim(),
    chapter: document.getElementById('f-chapter').value.trim() || '0',
    page:    document.getElementById('f-page').value.trim() || '0',
    scene:   document.getElementById('f-scene').value.trim(),
    quote:   document.getElementById('f-quote').value.trim(),
  };
}

window.openAddMoment = function() {
  showModal('Add Musical Moment', momentFormHtml(), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveMoment(null)">Save Moment</button>`);
};
window.openEditMoment = function(key) {
  const m = window._reg[key] || {};
  showModal('Edit Moment', momentFormHtml(m), `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-primary" onclick="saveMoment(${m.id})">Save Changes</button>`);
};
window.saveMoment = async function(id) {
  const data = collectMomentForm();
  if (!data.book_id || !data.song_id) { toast('Book ID and Song ID are required', 'error'); return; }
  const r = id ? await api.adminUpdateMoment(id, data) : await api.adminCreateMoment(data);
  if (r.status === 200) { closeModal(); toast(id ? 'Moment updated!' : 'Moment created!', 'success'); adminMoments(); }
  else toast(r.message || 'Error', 'error');
};
window.confirmDeleteMoment = function(id) {
  showModal('Delete Moment', `<p>Delete this moment record? This cannot be undone.</p>`, `
    <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
    <button class="btn btn-danger" onclick="deleteMoment(${id})">Delete</button>`);
};
window.deleteMoment = async function(id) {
  const r = await api.adminDeleteMoment(id);
  if (r.status === 200) { closeModal(); toast('Moment deleted.', 'info'); adminMoments(); }
  else toast(r.message || 'Error', 'error');
};

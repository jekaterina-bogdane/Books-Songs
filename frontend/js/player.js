// ── Audio Player ──────────────────────────────────────────────────────────────

const Player = (() => {
  const audio   = document.getElementById('audio-el');
  const bar     = document.getElementById('player');
  const disc    = document.getElementById('player-disc');
  const titleEl = document.getElementById('player-title');
  const artistEl= document.getElementById('player-artist');
  const bookEl  = document.getElementById('player-book');
  const playBtn = document.getElementById('btn-play');
  const prevBtn = document.getElementById('btn-prev');
  const nextBtn = document.getElementById('btn-next');
  const fill    = document.getElementById('progress-fill');
  const progBar = document.getElementById('progress-bar');
  const timeCur = document.getElementById('time-cur');
  const timeTot = document.getElementById('time-total');
  const volSldr = document.getElementById('volume');

  let queue   = [];   // [{title, artist, audio_url, book_title}]
  let qIdx    = -1;
  let playing = false;

  function fmt(sec) {
    sec = Math.floor(sec || 0);
    return `${Math.floor(sec/60)}:${String(sec%60).padStart(2,'0')}`;
  }

  function setPlaying(state) {
    playing = state;
    playBtn.textContent = state ? '⏸' : '▶';
    disc.classList.toggle('playing', state);
    document.querySelector('.hero-vinyl')?.classList.toggle('playing', state);
  }

  function load(item) {
    titleEl.textContent  = item.title  || '—';
    artistEl.textContent = item.artist || '';
    bookEl.textContent   = item.book_title ? `from "${item.book_title}"` : '';
    document.getElementById('player-status').textContent = 'Now playing';
    if (!item.audio_url) {
      toast('No audio preview is available for this song.', 'error');
      return;
    }
    audio.controls = true;
    audio.crossOrigin = 'anonymous';
    audio.src = item.audio_url;
    bar.classList.remove('hidden');
    fill.style.width = '0%';
    timeCur.textContent = '0:00';
    timeTot.textContent = '0:00';
    audio.volume = (volSldr.value || 80) / 100;
    audio.play().then(() => setPlaying(true)).catch((err) => {
      setPlaying(false);
      toast('Could not play audio. Check that the file exists and the format is supported.', 'error');
      console.warn('Audio play failed', err);
    });
  }

  function play(item, playQueue = null, idx = 0) {
    if (playQueue) { queue = playQueue; qIdx = idx; }
    load(item);
  }

  function toggle() {
    if (!audio.src) return;
    if (playing) { audio.pause(); setPlaying(false); }
    else         { audio.play().then(() => setPlaying(true)).catch(() => {}); }
  }

  // Events
  audio.addEventListener('timeupdate', () => {
    if (!audio.duration) return;
    const pct = (audio.currentTime / audio.duration) * 100;
    fill.style.width = pct + '%';
    timeCur.textContent = fmt(audio.currentTime);
    timeTot.textContent = fmt(audio.duration);
  });

  audio.addEventListener('ended', () => {
    setPlaying(false);
    if (qIdx < queue.length - 1) { qIdx++; load(queue[qIdx]); }
  });

  audio.addEventListener('error', () => {
    setPlaying(false);
    document.getElementById('player-status').textContent = 'Playback error';
    toast('Audio failed to load or play. Check your network and file path.', 'error');
    console.warn('Audio element error', audio.error);
  });

  audio.addEventListener('pause', () => {
    if (!audio.currentTime || audio.currentTime === 0) return;
    document.getElementById('player-status').textContent = 'Paused';
  });

  audio.addEventListener('play', () => {
    document.getElementById('player-status').textContent = 'Now playing';
  });

  playBtn.addEventListener('click', toggle);

  prevBtn.addEventListener('click', () => {
    if (audio.currentTime > 3) { audio.currentTime = 0; return; }
    if (qIdx > 0) { qIdx--; load(queue[qIdx]); }
  });

  nextBtn.addEventListener('click', () => {
    if (qIdx < queue.length - 1) { qIdx++; load(queue[qIdx]); }
  });

  progBar.addEventListener('click', e => {
    if (!audio.duration) return;
    const rect = progBar.getBoundingClientRect();
    audio.currentTime = ((e.clientX - rect.left) / rect.width) * audio.duration;
  });

  volSldr.addEventListener('input', () => { audio.volume = volSldr.value / 100; });

  return { play, toggle, queue: () => queue };
})();

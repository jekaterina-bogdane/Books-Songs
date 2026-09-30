// Hash-based SPA router (#/books, #/songs/:id, etc.)

const Router = (() => {
  const routes = [];
  let currentPath = '';

  function add(pattern, handler) {
    routes.push({ pattern, handler, re: buildRE(pattern) });
  }

  function buildRE(pattern) {
    const src = '^' + pattern
      .replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
      .replace(/:([a-z_]+)/g, '(?<$1>[^/]+)')
      + '$';
    return new RegExp(src);
  }

  function navigate(path) {
    location.hash = '#' + path;
  }

  function dispatch(path) {
    path = path || '/';
    currentPath = path;
    for (const r of routes) {
      const m = path.match(r.re);
      if (m) { r.handler(m.groups || {}); return; }
    }
    // 404 fallback
    document.getElementById('app').innerHTML =
      `<div class="container"><div class="empty-state"><div class="icon">🔍</div><h3>Page not found</h3><p>${path}</p></div></div>`;
  }

  function init() {
    window.addEventListener('hashchange', () => {
      const h = location.hash.replace(/^#/, '') || '/';
      dispatch(h);
    });
    // Intercept <a href="#/…"> clicks for active-state management
    document.addEventListener('click', e => {
      const a = e.target.closest('a[href^="#/"]');
      if (!a) return;
      updateNavLinks(a.getAttribute('href').replace('#', ''));
    });
    dispatch(location.hash.replace(/^#/, '') || '/');
  }

  function updateNavLinks(path) {
    document.querySelectorAll('.nav-link[data-page]').forEach(el => {
      el.classList.toggle('active', path.startsWith('/' + el.dataset.page));
    });
  }

  function current() { return currentPath; }

  return { add, navigate, init, current, updateNavLinks };
})();

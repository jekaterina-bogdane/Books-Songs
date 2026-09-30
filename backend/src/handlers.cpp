#include "handlers.h"
#include "mailer.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>
#include <stdexcept>
#include <set>
#include <ctime>

std::map<std::string, uint32_t> g_sessions;
std::string g_data_dir;

// ─── BSF helpers ─────────────────────────────────────────────────────────────

std::string bsf_error(int code, const std::string& msg) {
    return "STATUS " + std::to_string(code) + "\nMESSAGE " + msg + "\n";
}
std::string bsf_ok()                  { return "STATUS 200\n"; }
std::string bsf_list_header(int count){ return "STATUS 200\nCOUNT " + std::to_string(count) + "\n"; }

static std::string esc(const std::string& s) {
    std::string r; r.reserve(s.size());
    for (char c : s) {
        if      (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else                r.push_back(c);
    }
    return r;
}

std::string bsf_record(const std::vector<std::pair<std::string,std::string>>& fields) {
    std::string s = "RECORD\n";
    for (const auto& [k, v] : fields)
        s += k + ": " + esc(v) + "\n";
    s += "END\n";
    return s;
}

// ─── Utilities ────────────────────────────────────────────────────────────────

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
static bool contains_ci(const std::string& h, const std::string& n) {
    return to_lower(h).find(to_lower(n)) != std::string::npos;
}
static std::string ids_to_str(const uint32_t* ids, uint8_t cnt) {
    std::string s;
    for (int i = 0; i < cnt; i++) { if (i) s += ","; s += std::to_string(ids[i]); }
    return s;
}
static std::map<std::string,std::string> parse_body(const std::string& body) {
    std::map<std::string,std::string> out;
    std::istringstream ss(body);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto col = line.find(": ");
        if (col != std::string::npos)
            out[line.substr(0, col)] = line.substr(col + 2);
    }
    return out;
}
static std::string hash_password(const std::string& pw) {
    uint64_t h = 5381;
    for (unsigned char c : pw) h = ((h << 5) + h) ^ c;
    h ^= 0xDEADBEEFCAFEBABEULL;
    char buf[17]; snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
    return buf;
}
static std::string generate_token() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    char t[33];
    snprintf(t, sizeof(t), "%016llx%016llx",
             (unsigned long long)dis(gen), (unsigned long long)dis(gen));
    return t;
}
static int  safe_year(const std::string& s){ try{ return s.empty()?0:std::stoi(s); }catch(...){ return 0; } }
static uint32_t safe_u32(const std::string& s){ try{ return s.empty()?0:(uint32_t)std::stoul(s); }catch(...){ return 0; } }

static void parse_ids(const std::string& s, uint32_t* ids, uint8_t& cnt, uint8_t mx) {
    cnt = 0;
    std::istringstream ss(s); std::string tok;
    while (std::getline(ss, tok, ',') && cnt < mx) {
        tok.erase(std::remove_if(tok.begin(), tok.end(), ::isspace), tok.end());
        if (!tok.empty()) try { ids[cnt++] = (uint32_t)std::stoul(tok); } catch (...) {}
    }
}

// ─── Session: persists across server restarts via UserRecord.session_token ───

static uint32_t find_user_by_token(const std::string& token, Database& db) {
    if (token.empty()) return 0;
    // Fast in-memory check
    auto it = g_sessions.find(token);
    if (it != g_sessions.end()) return it->second;
    // Fallback: scan disk (handles server restarts)
    for (const auto& u : db.users.loadAll()) {
        if (std::string(u.session_token) == token) {
            g_sessions[token] = u.id; // populate cache
            return u.id;
        }
    }
    return 0;
}

// ─── Record builders ─────────────────────────────────────────────────────────

static std::string book_record(const BookRecord& b) {
    return bsf_record({
        {"id",         std::to_string(b.id)},
        {"title",      b.title},
        {"author",     b.author},
        {"year",       std::to_string(b.year)},
        {"genre",      b.genre},
        {"description",b.description},
        {"cover_url",  b.cover_url},
        {"song_ids",   ids_to_str(b.song_ids, b.song_count)},
        {"song_count", std::to_string(b.song_count)},
        {"isbn",       b.isbn}
    });
}

static std::string song_record(const SongRecord& s) {
    return bsf_record({
        {"id",         std::to_string(s.id)},
        {"title",      s.title},
        {"artist",     s.artist},
        {"genre",      s.genre},
        {"audio_url",  s.audio_url},
        {"duration",   std::to_string(s.duration_sec)},
        {"year",       std::to_string(s.year)},
        {"vinyl_id",   std::to_string(s.vinyl_id)},
        {"book_ids",   ids_to_str(s.book_ids, s.book_count)},
        {"book_count", std::to_string(s.book_count)}
    });
}

static std::string vinyl_record(const VinylRecord& v) {
    return bsf_record({
        {"id",          std::to_string(v.id)},
        {"title",       v.title},
        {"artist",      v.artist},
        {"year",        std::to_string(v.year)},
        {"label",       v.label},
        {"condition",   v.condition},
        {"price_cents", std::to_string(v.price_cents)},
        {"song_ids",    ids_to_str(v.song_ids, v.song_count)},
        {"song_count",  std::to_string(v.song_count)},
        {"image_url",   v.image_url},
        {"in_stock",    std::to_string(v.in_stock)}
    });
}

static std::string moment_record(const BookSongMoment& m) {
    return bsf_record({
        {"id",      std::to_string(m.id)},
        {"book_id", std::to_string(m.book_id)},
        {"song_id", std::to_string(m.song_id)},
        {"chapter", std::to_string(m.chapter)},
        {"page",    std::to_string(m.page)},
        {"scene",   m.scene},
        {"quote",   m.quote}
    });
}

// ─── Books ────────────────────────────────────────────────────────────────────

HttpResponse handle_get_books(const HttpRequest& req, Database& db) {
    auto all = db.books.loadAll();
    std::string q     = req.params.count("q")     ? req.params.at("q")     : "";
    std::string genre = req.params.count("genre")  ? req.params.at("genre") : "";
    std::string body; int cnt = 0;
    for (const auto& b : all) {
        if (!q.empty()     && !contains_ci(b.title,b.title) && !contains_ci(b.title,q) && !contains_ci(b.author,q)) continue;
        if (!genre.empty() && !contains_ci(b.genre, genre)) continue;
        body += book_record(b); cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

HttpResponse handle_get_book(const HttpRequest& req, Database& db) {
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    BookRecord b{};
    if (!db.books.load(safe_u32(id_it->second), b)) return HttpResponse::error(404,"Book not found");

    std::string body = bsf_list_header(1) + book_record(b);

    // Inline song previews
    std::string songs_bsf;
    for (int i = 0; i < b.song_count; i++) {
        SongRecord s{};
        if (!db.songs.load(b.song_ids[i], s)) continue;
        songs_bsf += bsf_record({
            {"song_id",    std::to_string(s.id)},
            {"song_title", s.title},
            {"song_artist",s.artist},
            {"song_genre", s.genre},
            {"audio_url",  s.audio_url},
            {"song_year",  std::to_string(s.year)},
            {"vinyl_id",   std::to_string(s.vinyl_id)}
        });
    }
    if (!songs_bsf.empty())
        body += "SONGS_COUNT " + std::to_string(b.song_count) + "\n" + songs_bsf;

    // Moments for this book
    auto all_moments = db.moments.loadAll();
    std::string moments_bsf;
    int mc = 0;
    for (const auto& m : all_moments) {
        if (m.book_id != b.id) continue;
        moments_bsf += moment_record(m); mc++;
    }
    if (mc > 0)
        body += "MOMENTS_COUNT " + std::to_string(mc) + "\n" + moments_bsf;

    return HttpResponse::bsf(body);
}

// ─── Songs ────────────────────────────────────────────────────────────────────

HttpResponse handle_get_songs(const HttpRequest& req, Database& db) {
    auto all = db.songs.loadAll();
    std::string q = req.params.count("q") ? req.params.at("q") : "";
    std::string body; int cnt = 0;
    for (const auto& s : all) {
        if (!q.empty() && !contains_ci(s.title,q) && !contains_ci(s.artist,q)) continue;
        body += song_record(s); cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

HttpResponse handle_get_song(const HttpRequest& req, Database& db) {
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    SongRecord s{};
    if (!db.songs.load(safe_u32(id_it->second), s)) return HttpResponse::error(404,"Song not found");

    std::string body = bsf_list_header(1) + song_record(s);

    // Books that mention this song
    std::string books_bsf;
    for (int i = 0; i < s.book_count; i++) {
        BookRecord b{};
        if (!db.books.load(s.book_ids[i], b)) continue;
        books_bsf += bsf_record({
            {"book_id",   std::to_string(b.id)},
            {"book_title",b.title},
            {"book_author",b.author},
            {"cover_url", b.cover_url},
            {"book_genre",b.genre}
        });
    }
    if (!books_bsf.empty())
        body += "BOOKS_COUNT " + std::to_string(s.book_count) + "\n" + books_bsf;

    // Moments for this song
    auto all_moments = db.moments.loadAll();
    std::string moments_bsf;
    int mc = 0;
    for (const auto& m : all_moments) {
        if (m.song_id != s.id) continue;
        // Add book title to moment record for context
        BookRecord mb{};
        std::string btitle;
        if (db.books.load(m.book_id, mb)) btitle = mb.title;
        moments_bsf += bsf_record({
            {"id",         std::to_string(m.id)},
            {"book_id",    std::to_string(m.book_id)},
            {"book_title", btitle},
            {"song_id",    std::to_string(m.song_id)},
            {"chapter",    std::to_string(m.chapter)},
            {"page",       std::to_string(m.page)},
            {"scene",      m.scene},
            {"quote",      m.quote}
        });
        mc++;
    }
    if (mc > 0)
        body += "MOMENTS_COUNT " + std::to_string(mc) + "\n" + moments_bsf;

    // Vinyl
    if (s.vinyl_id) {
        VinylRecord v{};
        if (db.vinyls.load(s.vinyl_id, v)) {
            body += "VINYL\n" + bsf_record({
                {"vinyl_id",   std::to_string(v.id)},
                {"vinyl_title",v.title},
                {"vinyl_artist",v.artist},
                {"image_url",  v.image_url},
                {"price_cents",std::to_string(v.price_cents)}
            });
        }
    }
    return HttpResponse::bsf(body);
}

// ─── Vinyls ───────────────────────────────────────────────────────────────────

HttpResponse handle_get_vinyls(const HttpRequest& req, Database& db) {
    auto all = db.vinyls.loadAll();
    std::string q = req.params.count("q") ? req.params.at("q") : "";
    std::string body; int cnt = 0;
    for (const auto& v : all) {
        if (!q.empty() && !contains_ci(v.title,q) && !contains_ci(v.artist,q)) continue;
        body += vinyl_record(v); cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

HttpResponse handle_get_vinyl(const HttpRequest& req, Database& db) {
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    VinylRecord v{};
    if (!db.vinyls.load(safe_u32(id_it->second), v)) return HttpResponse::error(404,"Vinyl not found");

    std::string body = bsf_list_header(1) + vinyl_record(v);
    std::string songs_bsf;
    for (int i = 0; i < v.song_count; i++) {
        SongRecord s{};
        if (!db.songs.load(v.song_ids[i], s)) continue;
        songs_bsf += bsf_record({
            {"song_id",    std::to_string(s.id)},
            {"song_title", s.title},
            {"song_artist",s.artist},
            {"audio_url",  s.audio_url}
        });
    }
    if (!songs_bsf.empty())
        body += "SONGS_COUNT " + std::to_string(v.song_count) + "\n" + songs_bsf;
    return HttpResponse::bsf(body);
}

// ─── Moments ─────────────────────────────────────────────────────────────────

HttpResponse handle_get_book_moments(const HttpRequest& req, Database& db) {
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    uint32_t bid = safe_u32(id_it->second);
    auto all = db.moments.loadAll();
    std::string body; int cnt = 0;
    for (const auto& m : all) {
        if (m.book_id != bid) continue;
        // Enrich with song info
        SongRecord s{};
        std::string stitle, sartist, saudio;
        if (db.songs.load(m.song_id, s)) { stitle=s.title; sartist=s.artist; saudio=s.audio_url; }
        body += bsf_record({
            {"id",         std::to_string(m.id)},
            {"book_id",    std::to_string(m.book_id)},
            {"song_id",    std::to_string(m.song_id)},
            {"song_title", stitle},
            {"song_artist",sartist},
            {"audio_url",  saudio},
            {"chapter",    std::to_string(m.chapter)},
            {"page",       std::to_string(m.page)},
            {"scene",      m.scene},
            {"quote",      m.quote}
        });
        cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

HttpResponse handle_get_song_moments(const HttpRequest& req, Database& db) {
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    uint32_t sid = safe_u32(id_it->second);
    auto all = db.moments.loadAll();
    std::string body; int cnt = 0;
    for (const auto& m : all) {
        if (m.song_id != sid) continue;
        BookRecord b{};
        std::string btitle, bauthor, bcover;
        if (db.books.load(m.book_id, b)) { btitle=b.title; bauthor=b.author; bcover=b.cover_url; }
        body += bsf_record({
            {"id",          std::to_string(m.id)},
            {"book_id",     std::to_string(m.book_id)},
            {"book_title",  btitle},
            {"book_author", bauthor},
            {"cover_url",   bcover},
            {"song_id",     std::to_string(m.song_id)},
            {"chapter",     std::to_string(m.chapter)},
            {"page",        std::to_string(m.page)},
            {"scene",       m.scene},
            {"quote",       m.quote}
        });
        cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

// ─── Recommendations ─────────────────────────────────────────────────────────

HttpResponse handle_related_books(const HttpRequest& req, Database& db) {
    // Books that share at least one song with this book (same cultural space)
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    BookRecord src{};
    if (!db.books.load(safe_u32(id_it->second), src)) return HttpResponse::error(404,"Not found");

    std::set<uint32_t> my_songs(src.song_ids, src.song_ids + src.song_count);
    auto all_books = db.books.loadAll();
    std::string body; int cnt = 0;
    for (const auto& b : all_books) {
        if (b.id == src.id) continue;
        // shared song OR same genre
        bool related = contains_ci(b.genre, src.genre);
        for (int i = 0; i < b.song_count && !related; i++)
            if (my_songs.count(b.song_ids[i])) related = true;
        if (!related) continue;
        body += book_record(b); cnt++;
        if (cnt >= 6) break;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

HttpResponse handle_related_songs(const HttpRequest& req, Database& db) {
    // Songs by the same artist, or mentioned in the same books
    auto id_it = req.params.find("id");
    if (id_it == req.params.end()) return HttpResponse::error(400,"Missing id");
    SongRecord src{};
    if (!db.songs.load(safe_u32(id_it->second), src)) return HttpResponse::error(404,"Not found");

    std::set<uint32_t> my_books(src.book_ids, src.book_ids + src.book_count);
    auto all_songs = db.songs.loadAll();
    std::string body; int cnt = 0;
    for (const auto& s : all_songs) {
        if (s.id == src.id) continue;
        bool related = to_lower(s.artist) == to_lower(src.artist) ||
                       to_lower(s.genre)  == to_lower(src.genre);
        for (int i = 0; i < s.book_count && !related; i++)
            if (my_books.count(s.book_ids[i])) related = true;
        if (!related) continue;
        body += song_record(s); cnt++;
        if (cnt >= 8) break;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

// ─── Search ───────────────────────────────────────────────────────────────────

HttpResponse handle_search(const HttpRequest& req, Database& db) {
    auto qi = req.params.find("q");
    if (qi == req.params.end() || qi->second.empty())
        return HttpResponse::error(400,"Missing query");
    const std::string& q = qi->second;
    std::string body; int total = 0;
    for (const auto& b : db.books.loadAll())
        if (contains_ci(b.title,q)||contains_ci(b.author,q)||contains_ci(b.genre,q)) {
            body += bsf_record({{"type","book"},{"id",std::to_string(b.id)},
                                {"title",b.title},{"subtitle",b.author},{"cover_url",b.cover_url}});
            total++;
        }
    for (const auto& s : db.songs.loadAll())
        if (contains_ci(s.title,q)||contains_ci(s.artist,q)) {
            body += bsf_record({{"type","song"},{"id",std::to_string(s.id)},
                                {"title",s.title},{"subtitle",s.artist},{"cover_url",""}});
            total++;
        }
    for (const auto& v : db.vinyls.loadAll())
        if (contains_ci(v.title,q)||contains_ci(v.artist,q)) {
            body += bsf_record({{"type","vinyl"},{"id",std::to_string(v.id)},
                                {"title",v.title},{"subtitle",v.artist},{"cover_url",v.image_url}});
            total++;
        }
    return HttpResponse::bsf(bsf_list_header(total) + body);
}

// ─── Auth ─────────────────────────────────────────────────────────────────────

static std::string user_bsf_fields(const UserRecord& u, const std::string& token = "") {
    return bsf_record({
        {"id",           std::to_string(u.id)},
        {"username",     u.username},
        {"email",        u.email},
        {"avatar_url",   u.avatar_url},
        {"bio",          u.bio},
        {"fav_genres",   u.fav_genres},
        {"accent_color", u.accent_color},
        {"is_admin",     std::to_string(u.is_admin)},
        {"token",        token.empty() ? std::string(u.session_token) : token},
        {"fav_books",    ids_to_str(u.fav_book_ids,    u.fav_book_count)},
        {"fav_songs",    ids_to_str(u.fav_song_ids,    u.fav_song_count)},
        {"cart",         ids_to_str(u.cart_vinyl_ids,  u.cart_count)},
        {"collection",   ids_to_str(u.collection_vinyl_ids, u.collection_count)}
    });
}

HttpResponse handle_register(const HttpRequest& req, Database& db) {
    auto p = parse_body(req.body);
    if (p["username"].empty()||p["email"].empty()||p["password"].empty())
        return HttpResponse::error(400,"username, email and password required");
    for (const auto& u : db.users.loadAll())
        if (to_lower(u.email) == to_lower(p["email"]))
            return HttpResponse::error(409,"Email already registered");

    UserRecord u{};
    u.id = db.users.nextId();
    set_str(u.username,     p["username"].c_str());
    set_str(u.email,        p["email"].c_str());
    std::string hash = hash_password(p["password"]);
    set_str(u.password_hash,hash.c_str());
    set_str(u.accent_color, "#f7c948");
    u.is_admin = 0; u.is_deleted = 0;
    std::string token = generate_token();
    set_str(u.session_token, token.c_str());
    g_sessions[token] = u.id;
    if (!db.users.save(u)) return HttpResponse::error(500,"Could not save user");
    return HttpResponse::bsf(bsf_list_header(1) + user_bsf_fields(u, token));
}

HttpResponse handle_login(const HttpRequest& req, Database& db) {
    auto p = parse_body(req.body);
    if (p["email"].empty()||p["password"].empty())
        return HttpResponse::error(400,"email and password required");
    std::string hash = hash_password(p["password"]);
    for (auto u : db.users.loadAll()) {
        if (to_lower(u.email)==to_lower(p["email"]) && std::string(u.password_hash)==hash) {
            std::string token = generate_token();
            set_str(u.session_token, token.c_str());
            db.users.save(u);
            g_sessions[token] = u.id;
            return HttpResponse::bsf(bsf_list_header(1) + user_bsf_fields(u, token));
        }
    }
    return HttpResponse::error(401,"Invalid credentials");
}

HttpResponse handle_logout(const HttpRequest& req, Database& db) {
    (void)db;
    if (!req.session_token.empty()) g_sessions.erase(req.session_token);
    return HttpResponse::bsf(bsf_ok());
}

// ─── Profile ──────────────────────────────────────────────────────────────────

HttpResponse handle_get_me(const HttpRequest& req, Database& db) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    UserRecord u{};
    if (!db.users.load(uid, u)) return HttpResponse::error(404,"User not found");
    return HttpResponse::bsf(bsf_list_header(1) + user_bsf_fields(u));
}

HttpResponse handle_update_me(const HttpRequest& req, Database& db) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    UserRecord u{};
    if (!db.users.load(uid, u)) return HttpResponse::error(404,"User not found");
    auto p = parse_body(req.body);
    if (p.count("bio"))          set_str(u.bio,          p["bio"].c_str());
    if (p.count("avatar_url")) {
        std::string url = p["avatar_url"];
        if (!url.empty() && url.find("://") == std::string::npos) {
            if (url[0] != '/') url = "/avatars/" + url;
            else if (url.rfind("/avatars/", 0) != 0) url = "/avatars/" + url.substr(1);
        }
        set_str(u.avatar_url, url.c_str());
    }
    if (p.count("accent_color")) set_str(u.accent_color, p["accent_color"].c_str());
    if (p.count("username"))     set_str(u.username,     p["username"].c_str());
    if (p.count("fav_genres"))   set_str(u.fav_genres,   p["fav_genres"].c_str());
    db.users.save(u);
    return HttpResponse::bsf(bsf_list_header(1) + user_bsf_fields(u));
}

// ─── Favorites / Cart / Collection ───────────────────────────────────────────

template<typename A>
static bool arr_add(A* ids, uint8_t& cnt, uint8_t mx, uint32_t id) {
    for (int i=0;i<cnt;i++) if ((uint32_t)ids[i]==id) return true;
    if (cnt>=mx) return false;
    ids[cnt++]=(A)id; return true;
}
template<typename A>
static void arr_remove(A* ids, uint8_t& cnt, uint32_t id) {
    for (int i=0;i<cnt;i++) if ((uint32_t)ids[i]==id) { ids[i]=ids[--cnt]; return; }
}

HttpResponse handle_fav_book(const HttpRequest& req, Database& db, bool add) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    uint32_t bid = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    UserRecord u{}; if (!db.users.load(uid,u)) return HttpResponse::error(404,"User not found");
    if (add) arr_add(u.fav_book_ids,u.fav_book_count,50,bid);
    else     arr_remove(u.fav_book_ids,u.fav_book_count,bid);
    db.users.save(u); return HttpResponse::bsf(bsf_ok());
}
HttpResponse handle_fav_song(const HttpRequest& req, Database& db, bool add) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    uint32_t sid = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    UserRecord u{}; if (!db.users.load(uid,u)) return HttpResponse::error(404,"User not found");
    if (add) arr_add(u.fav_song_ids,u.fav_song_count,50,sid);
    else     arr_remove(u.fav_song_ids,u.fav_song_count,sid);
    db.users.save(u); return HttpResponse::bsf(bsf_ok());
}
HttpResponse handle_cart(const HttpRequest& req, Database& db, bool add) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    uint32_t vid = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    UserRecord u{}; if (!db.users.load(uid,u)) return HttpResponse::error(404,"User not found");
    if (add) arr_add(u.cart_vinyl_ids,u.cart_count,20,vid);
    else     arr_remove(u.cart_vinyl_ids,u.cart_count,vid);
    db.users.save(u); return HttpResponse::bsf(bsf_ok());
}
HttpResponse handle_collection(const HttpRequest& req, Database& db, bool add) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401,"Unauthorized");
    uint32_t vid = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    UserRecord u{}; if (!db.users.load(uid,u)) return HttpResponse::error(404,"User not found");
    if (add) arr_add(u.collection_vinyl_ids,u.collection_count,50,vid);
    else     arr_remove(u.collection_vinyl_ids,u.collection_count,vid);
    db.users.save(u); return HttpResponse::bsf(bsf_ok());
}

// ─── Orders / Checkout ───────────────────────────────────────────────────────

static std::string build_receipt(const UserRecord& u, const OrderRecord& ord,
                                  const std::vector<VinylRecord>& vinyls,
                                  const std::string& last4, const std::string& ts) {
    std::string s;
    s += "═══════════════════════════════════════\n";
    s += "        Books & Songs — Receipt\n";
    s += "═══════════════════════════════════════\n";
    s += "Order #" + std::string(6 - std::to_string(ord.id).size(), '0') + std::to_string(ord.id) + "\n";
    s += "Date:   " + ts + "\n";
    s += "Customer: " + std::string(u.username) + " <" + std::string(u.email) + ">\n";
    s += "Card:   **** **** **** " + last4 + "\n";
    s += "───────────────────────────────────────\n";
    uint32_t total = 0;
    for (const auto& v : vinyls) {
        char line[128];
        std::string price_str = "$" + std::to_string(v.price_cents / 100) + "."
                              + (v.price_cents % 100 < 10 ? "0" : "")
                              + std::to_string(v.price_cents % 100);
        snprintf(line, sizeof(line), "%-32s %8s\n", v.title, price_str.c_str());
        s += line;
        total += v.price_cents;
    }
    s += "───────────────────────────────────────\n";
    std::string total_str = "$" + std::to_string(total / 100) + "."
                          + (total % 100 < 10 ? "0" : "") + std::to_string(total % 100);
    char tot[64]; snprintf(tot, sizeof(tot), "%-32s %8s\n", "TOTAL", total_str.c_str());
    s += tot;
    s += "═══════════════════════════════════════\n";
    s += "Thank you for your order!\n";
    s += "Your vinyl will be dispatched within 2–3 business days.\n";
    s += "\nBooks & Songs  ·  booksongs.local\n";
    return s;
}

HttpResponse handle_checkout(const HttpRequest& req, Database& db) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401, "Unauthorized");

    UserRecord user{};
    if (!db.users.load(uid, user)) return HttpResponse::error(404, "User not found");
    if (user.cart_count == 0) return HttpResponse::error(400, "Cart is empty");

    auto p = parse_body(req.body);
    std::string card_number = p["card_number"];
    std::string card_expiry = p["card_expiry"];
    std::string card_cvv    = p["card_cvv"];
    std::string card_name   = p["card_name"];

    // Strip spaces/dashes from card number
    std::string digits;
    for (char c : card_number) if (::isdigit((unsigned char)c)) digits += c;

    if (digits.size() < 13 || digits.size() > 19)
        return HttpResponse::error(400, "Invalid card number");
    if (card_expiry.size() < 4)
        return HttpResponse::error(400, "Invalid expiry date");
    if (card_cvv.size() < 3)
        return HttpResponse::error(400, "Invalid CVV");
    if (card_name.empty())
        return HttpResponse::error(400, "Cardholder name required");

    // Load cart items
    std::vector<VinylRecord> vinyls;
    uint32_t total_cents = 0;
    for (int i = 0; i < user.cart_count; i++) {
        VinylRecord v{};
        if (db.vinyls.load(user.cart_vinyl_ids[i], v)) {
            vinyls.push_back(v);
            total_cents += v.price_cents;
        }
    }
    if (vinyls.empty()) return HttpResponse::error(400, "No valid items in cart");

    // Build order record
    OrderRecord ord{};
    ord.id         = db.orders.nextId();
    ord.user_id    = uid;
    ord.total_cents= total_cents;
    ord.vinyl_count= (uint8_t)std::min((int)vinyls.size(), 10);
    for (int i = 0; i < (int)ord.vinyl_count; i++) ord.vinyl_ids[i] = vinyls[i].id;
    std::string last4 = digits.substr(digits.size() - 4);
    set_str(ord.card_last4, last4.c_str());

    time_t now = time(nullptr);
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    set_str(ord.created_at, ts);
    ord.is_deleted = 0;
    db.orders.save(ord);

    // Clear cart
    user.cart_count = 0;
    memset(user.cart_vinyl_ids, 0, sizeof(user.cart_vinyl_ids));
    db.users.save(user);

    // Build and send receipt
    std::string receipt = build_receipt(user, ord, vinyls, last4, ts);
    std::string subject = "Your Books & Songs Order #"
        + std::string(6 - std::to_string(ord.id).size(), '0')
        + std::to_string(ord.id) + " Receipt";
    send_receipt_email(user.email, user.username,
                       std::to_string(ord.id), subject, receipt, g_data_dir);

    // Return order confirmation (including updated user so frontend can clear cart)
    std::string body = bsf_record({
        {"order_id",    std::to_string(ord.id)},
        {"total_cents", std::to_string(total_cents)},
        {"card_last4",  last4},
        {"created_at",  ts},
        {"item_count",  std::to_string(vinyls.size())},
        {"email",       user.email},
        {"receipt",     receipt}
    });
    return HttpResponse::bsf(bsf_list_header(1) + body);
}

HttpResponse handle_get_orders(const HttpRequest& req, Database& db) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return HttpResponse::error(401, "Unauthorized");

    auto all = db.orders.loadAll();
    std::string body; int cnt = 0;
    for (const auto& ord : all) {
        if (ord.user_id != uid) continue;
        // Build vinyl titles list
        std::string titles;
        for (int i = 0; i < ord.vinyl_count; i++) {
            VinylRecord v{};
            if (db.vinyls.load(ord.vinyl_ids[i], v)) {
                if (!titles.empty()) titles += ", ";
                titles += v.title;
            }
        }
        body += bsf_record({
            {"order_id",    std::to_string(ord.id)},
            {"total_cents", std::to_string(ord.total_cents)},
            {"card_last4",  ord.card_last4},
            {"created_at",  ord.created_at},
            {"item_count",  std::to_string(ord.vinyl_count)},
            {"titles",      titles}
        });
        cnt++;
    }
    return HttpResponse::bsf(bsf_list_header(cnt) + body);
}

// ─── Admin helpers ────────────────────────────────────────────────────────────

static bool check_admin(const HttpRequest& req, Database& db) {
    uint32_t uid = find_user_by_token(req.session_token, db);
    if (!uid) return false;
    UserRecord u{}; if (!db.users.load(uid,u)) return false;
    return u.is_admin == 1;
}

static void sync_book_to_songs(const BookRecord& b, Database& db) {
    for (int i=0;i<b.song_count;i++) {
        SongRecord s{}; if (!db.songs.load(b.song_ids[i],s)) continue;
        bool found=false;
        for (int j=0;j<s.book_count;j++) if (s.book_ids[j]==b.id){found=true;break;}
        if (!found && s.book_count<20) { s.book_ids[s.book_count++]=b.id; db.songs.save(s); }
    }
}

// ─── Admin: stats / users ────────────────────────────────────────────────────

HttpResponse handle_admin_stats(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    return HttpResponse::bsf(bsf_list_header(1) + bsf_record({
        {"books",   std::to_string(db.books.loadAll().size())},
        {"songs",   std::to_string(db.songs.loadAll().size())},
        {"vinyls",  std::to_string(db.vinyls.loadAll().size())},
        {"users",   std::to_string(db.users.loadAll().size())},
        {"moments", std::to_string(db.moments.loadAll().size())}
    }));
}

HttpResponse handle_admin_users(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto all = db.users.loadAll();
    std::string body;
    for (const auto& u : all)
        body += bsf_record({{"id",std::to_string(u.id)},{"username",u.username},
                             {"email",u.email},{"is_admin",std::to_string(u.is_admin)},
                             {"bio",u.bio},{"avatar_url",u.avatar_url}});
    return HttpResponse::bsf(bsf_list_header((int)all.size()) + body);
}

HttpResponse handle_admin_delete_user(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    uint32_t tid = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    uint32_t me  = find_user_by_token(req.session_token,db);
    if (tid==me) return HttpResponse::error(400,"Cannot delete yourself");
    db.users.remove(tid);
    return HttpResponse::bsf(bsf_ok());
}

// ─── Admin: books ─────────────────────────────────────────────────────────────

HttpResponse handle_admin_create_book(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto p = parse_body(req.body);
    if (p["title"].empty()) return HttpResponse::error(400,"title required");
    BookRecord b{};
    b.id = db.books.nextId();
    set_str(b.title,p["title"].c_str()); set_str(b.author,p["author"].c_str());
    set_str(b.genre,p["genre"].c_str()); set_str(b.description,p["description"].c_str());
    set_str(b.cover_url,p["cover_url"].c_str()); set_str(b.isbn,p["isbn"].c_str());
    b.year = (int16_t)safe_year(p["year"]);
    parse_ids(p["song_ids"],b.song_ids,b.song_count,20);
    if (!db.books.save(b)) return HttpResponse::error(500,"Save failed");
    sync_book_to_songs(b,db);
    return HttpResponse::bsf(bsf_list_header(1) + book_record(b));
}

HttpResponse handle_admin_update_book(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    uint32_t id = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    BookRecord b{}; if (!db.books.load(id,b)) return HttpResponse::error(404,"Not found");
    auto p = parse_body(req.body);
    if (p.count("title"))       set_str(b.title,p["title"].c_str());
    if (p.count("author"))      set_str(b.author,p["author"].c_str());
    if (p.count("genre"))       set_str(b.genre,p["genre"].c_str());
    if (p.count("description")) set_str(b.description,p["description"].c_str());
    if (p.count("cover_url"))   set_str(b.cover_url,p["cover_url"].c_str());
    if (p.count("isbn"))        set_str(b.isbn,p["isbn"].c_str());
    if (p.count("year"))        b.year = (int16_t)safe_year(p["year"]);
    if (p.count("song_ids"))    parse_ids(p["song_ids"],b.song_ids,b.song_count,20);
    if (!db.books.save(b)) return HttpResponse::error(500,"Save failed");
    sync_book_to_songs(b,db);
    return HttpResponse::bsf(bsf_list_header(1) + book_record(b));
}

HttpResponse handle_admin_delete_book(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    db.books.remove(safe_u32(req.params.count("id")?req.params.at("id"):"0"));
    return HttpResponse::bsf(bsf_ok());
}

// ─── Admin: songs ─────────────────────────────────────────────────────────────

HttpResponse handle_admin_create_song(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto p = parse_body(req.body);
    if (p["title"].empty()) return HttpResponse::error(400,"title required");
    SongRecord s{};
    s.id = db.songs.nextId();
    set_str(s.title,p["title"].c_str()); set_str(s.artist,p["artist"].c_str());
    set_str(s.genre,p["genre"].c_str()); set_str(s.audio_url,p["audio_url"].c_str());
    s.year=(int16_t)safe_year(p["year"]); s.duration_sec=safe_u32(p["duration"]);
    s.vinyl_id=safe_u32(p["vinyl_id"]);
    if (!db.songs.save(s)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + song_record(s));
}

HttpResponse handle_admin_update_song(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    uint32_t id = safe_u32(req.params.count("id")?req.params.at("id"):"0");
    SongRecord s{}; if (!db.songs.load(id,s)) return HttpResponse::error(404,"Not found");
    auto p = parse_body(req.body);
    if (p.count("title"))     set_str(s.title,p["title"].c_str());
    if (p.count("artist"))    set_str(s.artist,p["artist"].c_str());
    if (p.count("genre"))     set_str(s.genre,p["genre"].c_str());
    if (p.count("audio_url")) set_str(s.audio_url,p["audio_url"].c_str());
    if (p.count("year"))      s.year=(int16_t)safe_year(p["year"]);
    if (p.count("duration"))  s.duration_sec=safe_u32(p["duration"]);
    if (p.count("vinyl_id"))  s.vinyl_id=safe_u32(p["vinyl_id"]);
    if (!db.songs.save(s)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + song_record(s));
}

HttpResponse handle_admin_delete_song(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    db.songs.remove(safe_u32(req.params.count("id")?req.params.at("id"):"0"));
    return HttpResponse::bsf(bsf_ok());
}

// ─── Admin: vinyls ────────────────────────────────────────────────────────────

HttpResponse handle_admin_create_vinyl(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto p = parse_body(req.body);
    if (p["title"].empty()) return HttpResponse::error(400,"title required");
    VinylRecord v{};
    v.id=db.vinyls.nextId();
    set_str(v.title,p["title"].c_str()); set_str(v.artist,p["artist"].c_str());
    set_str(v.label,p["label"].c_str()); set_str(v.condition,p["condition"].c_str());
    set_str(v.image_url,p["image_url"].c_str());
    v.year=(int16_t)safe_year(p["year"]); v.price_cents=safe_u32(p["price_cents"]);
    v.in_stock=p["in_stock"]=="0"?0:1;
    parse_ids(p["song_ids"],v.song_ids,v.song_count,20);
    if (!db.vinyls.save(v)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + vinyl_record(v));
}

HttpResponse handle_admin_update_vinyl(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    uint32_t id=safe_u32(req.params.count("id")?req.params.at("id"):"0");
    VinylRecord v{}; if (!db.vinyls.load(id,v)) return HttpResponse::error(404,"Not found");
    auto p=parse_body(req.body);
    if (p.count("title"))       set_str(v.title,p["title"].c_str());
    if (p.count("artist"))      set_str(v.artist,p["artist"].c_str());
    if (p.count("label"))       set_str(v.label,p["label"].c_str());
    if (p.count("condition"))   set_str(v.condition,p["condition"].c_str());
    if (p.count("image_url"))   set_str(v.image_url,p["image_url"].c_str());
    if (p.count("year"))        v.year=(int16_t)safe_year(p["year"]);
    if (p.count("price_cents")) v.price_cents=safe_u32(p["price_cents"]);
    if (p.count("in_stock"))    v.in_stock=p["in_stock"]=="0"?0:1;
    if (p.count("song_ids"))    parse_ids(p["song_ids"],v.song_ids,v.song_count,20);
    if (!db.vinyls.save(v)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + vinyl_record(v));
}

HttpResponse handle_admin_delete_vinyl(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    db.vinyls.remove(safe_u32(req.params.count("id")?req.params.at("id"):"0"));
    return HttpResponse::bsf(bsf_ok());
}

// ─── Admin: moments ───────────────────────────────────────────────────────────

HttpResponse handle_admin_get_moments(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto all = db.moments.loadAll();
    std::string body;
    for (const auto& m : all) {
        BookRecord b{}; SongRecord s{};
        std::string bt, st;
        if (db.books.load(m.book_id,b)) bt=b.title;
        if (db.songs.load(m.song_id,s)) st=s.title;
        body += bsf_record({
            {"id",std::to_string(m.id)},{"book_id",std::to_string(m.book_id)},
            {"book_title",bt},{"song_id",std::to_string(m.song_id)},{"song_title",st},
            {"chapter",std::to_string(m.chapter)},{"page",std::to_string(m.page)},
            {"scene",m.scene},{"quote",m.quote}
        });
    }
    return HttpResponse::bsf(bsf_list_header((int)all.size()) + body);
}

HttpResponse handle_admin_create_moment(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    auto p=parse_body(req.body);
    if (!safe_u32(p["book_id"])||!safe_u32(p["song_id"]))
        return HttpResponse::error(400,"book_id and song_id required");
    BookSongMoment m{};
    m.id=db.moments.nextId();
    m.book_id=(uint32_t)safe_u32(p["book_id"]);
    m.song_id=(uint32_t)safe_u32(p["song_id"]);
    m.chapter=(uint16_t)safe_u32(p["chapter"]);
    m.page=(uint16_t)safe_u32(p["page"]);
    set_str(m.scene,p["scene"].c_str());
    set_str(m.quote,p["quote"].c_str());
    if (!db.moments.save(m)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + moment_record(m));
}

HttpResponse handle_admin_update_moment(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    uint32_t id=safe_u32(req.params.count("id")?req.params.at("id"):"0");
    BookSongMoment m{}; if (!db.moments.load(id,m)) return HttpResponse::error(404,"Not found");
    auto p=parse_body(req.body);
    if (p.count("book_id")) m.book_id=safe_u32(p["book_id"]);
    if (p.count("song_id")) m.song_id=safe_u32(p["song_id"]);
    if (p.count("chapter")) m.chapter=(uint16_t)safe_u32(p["chapter"]);
    if (p.count("page"))    m.page=(uint16_t)safe_u32(p["page"]);
    if (p.count("scene"))   set_str(m.scene,p["scene"].c_str());
    if (p.count("quote"))   set_str(m.quote,p["quote"].c_str());
    if (!db.moments.save(m)) return HttpResponse::error(500,"Save failed");
    return HttpResponse::bsf(bsf_list_header(1) + moment_record(m));
}

HttpResponse handle_admin_delete_moment(const HttpRequest& req, Database& db) {
    if (!check_admin(req,db)) return HttpResponse::error(403,"Forbidden");
    db.moments.remove(safe_u32(req.params.count("id")?req.params.at("id"):"0"));
    return HttpResponse::bsf(bsf_ok());
}

#include "server.h"
#include "storage.h"
#include "handlers.h"
#include <iostream>
#include <string>
#include <filesystem>

// ─── Seed data ────────────────────────────────────────────────────────────────

static void seed(Database& db) {
    // Songs
    auto add_song = [&](const char* title, const char* artist, const char* genre,
                        int year, uint32_t vinyl_id, const char* audio_url) -> uint32_t {
        SongRecord s{};
        s.id = db.songs.nextId();
        set_str(s.title,     title);
        set_str(s.artist,    artist);
        set_str(s.genre,     genre);
        set_str(s.audio_url, audio_url);
        s.year     = (int16_t)year;
        s.vinyl_id = vinyl_id;
        s.duration_sec = 180;
        db.songs.save(s);
        return s.id;
    };

    uint32_t s1 = add_song("Norwegian Wood (This Bird Has Flown)", "The Beatles",
                            "Psychedelic Rock", 1965, 1,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview211/v4/26/00/39/260039f3-51bb-c479-16b7-8412199ccbc2/mzaf_2459513772630634554.plus.aac.p.m4a");
    uint32_t s2 = add_song("Let's Get It On", "Marvin Gaye",
                            "Soul", 1973, 2,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview221/v4/10/36/77/103677db-b70e-505f-eefd-cfb20ae130d2/mzaf_3849021925576978930.plus.aac.p.m4a");
    uint32_t s3 = add_song("Gloria", "Patti Smith",
                            "Punk Rock", 1975, 3,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview211/v4/f4/ac/0f/f4ac0f16-d919-d024-8b6e-2140467311fe/mzaf_5175574562412187547.plus.aac.p.m4a");
    uint32_t s4 = add_song("Space Oddity", "David Bowie",
                            "Art Rock", 1969, 4,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview115/v4/8d/34/1d/8d341d3d-978d-8885-4807-ff12d02659ec/mzaf_17330704255932500816.plus.aac.p.m4a");
    uint32_t s5 = add_song("'Round Midnight", "Thelonious Monk",
                            "Bebop Jazz", 1944, 5,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview211/v4/3c/33/40/3c33406a-f32d-fa67-6f9a-ace81aa60be8/mzaf_6228310614182394136.plus.aac.p.m4a");
    uint32_t s6 = add_song("We'll Meet Again", "Vera Lynn",
                            "WWII Era", 1939, 0,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview221/v4/2a/58/f0/2a58f08e-a15a-e34c-2910-d31d762f742e/mzaf_3198683966946437137.plus.aac.p.m4a");
    uint32_t s7 = add_song("Ain't Misbehavin'", "Fats Waller",
                            "Stride Piano / Jazz", 1929, 0,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview115/v4/f1/f1/aa/f1f1aac1-4e6c-10a9-d72b-0454c3427cb5/mzaf_13980908640219760543.plus.aac.p.m4a");
    uint32_t s8 = add_song("I Will Survive", "Gloria Gaynor",
                            "Disco", 1978, 2,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview125/v4/97/55/dd/9755dd8f-a332-4486-4946-c049059f0edf/mzaf_18336667409268238306.plus.aac.p.m4a");
    uint32_t s9 = add_song("Yesterday", "The Beatles",
                            "Soft Rock", 1965, 1,
                            "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview221/v4/d1/69/2d/d1692d74-fe32-c676-7a1d-00deacae1644/mzaf_11316115358642175957.plus.aac.p.m4a");
    uint32_t s10 = add_song("Desolation Row", "Bob Dylan",
                             "Folk Rock", 1965, 6,
                             "https://audio-ssl.itunes.apple.com/itunes-assets/AudioPreview125/v4/d2/24/66/d2246660-3d81-f7ac-7827-2889e0c3037c/mzaf_16731084067521909642.plus.aac.p.m4a");

    // Vinyls
    auto add_vinyl = [&](const char* title, const char* artist, int year,
                         const char* label, const char* cond, uint32_t price_cents,
                         const char* img, std::initializer_list<uint32_t> sids) -> uint32_t {
        VinylRecord v{};
        v.id = db.vinyls.nextId();
        set_str(v.title,     title);
        set_str(v.artist,    artist);
        set_str(v.label,     label);
        set_str(v.condition, cond);
        set_str(v.image_url, img);
        v.year        = (int16_t)year;
        v.price_cents = price_cents;
        v.in_stock    = 1;
        for (uint32_t id : sids) {
            if (v.song_count < 20) v.song_ids[v.song_count++] = id;
        }
        db.vinyls.save(v);
        return v.id;
    };

    add_vinyl("Rubber Soul", "The Beatles", 1965, "Parlophone", "VG+", 4999,
              "/covers/rubber_soul.jpg", {s1, s9});
    add_vinyl("Let's Get It On", "Marvin Gaye", 1973, "Tamla", "NM", 5999,
              "/covers/lets_get_it_on.jpg", {s2, s8});
    add_vinyl("Horses", "Patti Smith", 1975, "Arista", "VG+", 5499,
              "/covers/horses.jpg", {s3});
    add_vinyl("Space Oddity", "David Bowie", 1969, "Philips", "VG", 4499,
              "/covers/space_oddity.jpg", {s4});
    add_vinyl("Genius of Modern Music Vol.1", "Thelonious Monk", 1952,
              "Blue Note", "VG+", 8999, "/covers/monk.jpg", {s5});
    add_vinyl("The Freewheelin' Bob Dylan", "Bob Dylan", 1963,
              "Columbia", "VG", 3999, "/covers/freewheelin.jpg", {s10});

    // Books
    auto add_book = [&](const char* title, const char* author, int year,
                        const char* genre, const char* desc, const char* cover,
                        std::initializer_list<uint32_t> sids) {
        BookRecord b{};
        b.id = db.books.nextId();
        set_str(b.title,       title);
        set_str(b.author,      author);
        set_str(b.genre,       genre);
        set_str(b.description, desc);
        set_str(b.cover_url,   cover);
        b.year = (int16_t)year;
        for (uint32_t id : sids) {
            if (b.song_count < 20) b.song_ids[b.song_count++] = id;
        }
        db.books.save(b);

        // Update each song's book_ids back-reference
        for (uint32_t sid : sids) {
            SongRecord s{};
            if (db.songs.load(sid, s) && s.book_count < 20) {
                s.book_ids[s.book_count++] = b.id;
                db.songs.save(s);
            }
        }
    };

    add_book("Norwegian Wood", "Haruki Murakami", 1987,
             "Literary Fiction",
             "A nostalgic story of loss and sexuality set in 1960s Tokyo. "
             "The Beatles song Norwegian Wood plays a central role in the narrative.",
             "/covers/norwegian_wood_book.jpg", {s1, s9});

    add_book("High Fidelity", "Nick Hornby", 1995,
             "Contemporary Fiction",
             "Record shop owner Rob Fleming obsesses over music and top-five lists "
             "while trying to figure out why his relationships keep failing.",
             "/covers/high_fidelity.jpg", {s2, s8});

    add_book("Just Kids", "Patti Smith", 2010,
             "Memoir",
             "Patti Smith's account of her relationship with Robert Mapplethorpe "
             "and her emergence as an artist in 1960s-70s New York.",
             "/covers/just_kids.jpg", {s3, s4});

    add_book("On The Road", "Jack Kerouac", 1957,
             "Beat Literature",
             "The quintessential Beat Generation novel following Sal Paradise "
             "and Dean Moriarty across America, saturated with jazz culture.",
             "/covers/on_the_road.jpg", {s5});

    add_book("The Great Gatsby", "F. Scott Fitzgerald", 1925,
             "Classic American Literature",
             "Nick Carraway narrates the rise and fall of the mysterious "
             "Jay Gatsby against the backdrop of Roaring Twenties jazz-age parties.",
             "/covers/great_gatsby.jpg", {s7});

    add_book("1984", "George Orwell", 1949,
             "Dystopian Fiction",
             "Winston Smith lives in a totalitarian superstate where Big Brother "
             "watches everything. The song 'We'll Meet Again' echoes through the ending.",
             "/covers/1984.jpg", {s6});

    add_book("Nick and Norah's Infinite Playlist", "Rachel Cohn & David Levithan", 2006,
             "Young Adult",
             "A single night in New York City, two teenagers, and the music "
             "that keeps pulling them together.",
             "/covers/nick_norah.jpg", {s8, s3});

    add_book("The Perks of Being a Wallflower", "Stephen Chbosky", 1999,
             "Coming-of-Age",
             "Charlie writes letters to an anonymous stranger about his first "
             "year of high school, filled with music, books, and self-discovery.",
             "/covers/perks.jpg", {s10, s4});

    std::cout << "Seed data written.\n";
}

// ─── Seed moments (chapter/page/quote links) ──────────────────────────────────

static void seed_moments(Database& db) {
    if (db.moments.nextId() > 1) return; // already seeded

    auto add_moment = [&](uint32_t book_id, uint32_t song_id,
                          uint16_t chapter, uint16_t page,
                          const char* scene, const char* quote) {
        BookSongMoment m{};
        m.id      = db.moments.nextId();
        m.book_id = book_id;
        m.song_id = song_id;
        m.chapter = chapter;
        m.page    = page;
        set_str(m.scene, scene);
        set_str(m.quote, quote);
        db.moments.save(m);
    };

    // Norwegian Wood (book 1) — Norwegian Wood by The Beatles (song 1)
    add_moment(1, 1, 1, 3,
        "On the plane to Hamburg, Watanabe hears the opening guitar notes "
        "drifting from the PA system and is ambushed by memory.",
        "\"And somehow or other, the music brought tears to my eyes. I have no "
        "idea why. Autumn colours, the smell of grass, the feel of a girl's hand.\"");

    // Norwegian Wood (book 1) — Yesterday by The Beatles (song 9)
    add_moment(1, 9, 12, 147,
        "Naoko hums Yesterday while washing dishes — a small, unconscious "
        "moment that Watanabe never forgets.",
        "\"She was humming softly to herself, the melody barely audible over "
        "the sound of running water.\"");

    // High Fidelity (book 2) — Let's Get It On by Marvin Gaye (song 2)
    add_moment(2, 2, 8, 112,
        "Rob puts Let's Get It On on the turntable to set the mood "
        "for a date — and immediately regrets it as too obvious.",
        "\"Let's Get It On — that was a mistake. Way too forward. "
        "She looked at me like I had two heads.\"");

    // High Fidelity (book 2) — I Will Survive by Gloria Gaynor (song 8)
    add_moment(2, 8, 22, 198,
        "The breakup playlist climaxes with I Will Survive, which Rob "
        "plays on repeat at midnight, slightly drunk.",
        "\"There is no more honest song in the world. Three minutes and "
        "seventeen seconds of pure truth.\"");

    // Just Kids (book 3) — Gloria by Patti Smith (song 3)
    add_moment(3, 3, 15, 183,
        "Patti describes the genesis of Gloria — rewriting Van Morrison's "
        "classic into something raw and sacrilegious — in their cold Chelsea loft.",
        "\"I wanted to take something pure and crack it open, "
        "see what was inside.\"");

    // Just Kids (book 3) — Space Oddity by David Bowie (song 4)
    add_moment(3, 4, 9, 121,
        "Space Oddity plays from a transistor radio in the Chelsea Hotel "
        "while Robert Mapplethorpe photographs Patti for the first time.",
        "\"The song seemed written for us — floating, untethered, "
        "beautiful and doomed all at once.\"");

    // On The Road (book 4) — 'Round Midnight by Thelonious Monk (song 5)
    add_moment(4, 5, 7, 89,
        "Sal and Dean stumble into a Harlem jazz club at 2 AM. "
        "A lone pianist begins Round Midnight and the room goes electric.",
        "\"The pianist hit the first notes of Round Midnight and I felt "
        "my whole body recognise something I had never heard before.\"");

    // The Great Gatsby (book 5) — Ain't Misbehavin' by Fats Waller (song 7)
    add_moment(5, 7, 3, 54,
        "Gatsby's first party: a five-piece orchestra launches into "
        "Ain't Misbehavin' as the East Egg crowd floods the lawn.",
        "\"The orchestra leader varied his tuxedoed obsequies and the "
        "hall was sweet with jazz and gin.\"");

    // 1984 (book 6) — We'll Meet Again by Vera Lynn (song 6)
    add_moment(6, 6, 24, 298,
        "In the final lines of the novel, the telescreens broadcast "
        "We'll Meet Again — the Party's cruellest irony.",
        "\"From the telescreen a brassy female voice was singing a "
        "popular song. He had won the victory over himself.\"");

    // Perks of Being a Wallflower (book 8) — Desolation Row by Bob Dylan (song 10)
    add_moment(8, 10, 6, 88,
        "Charlie plays Desolation Row for Sam late at night, "
        "telling her it's the most honest song ever written.",
        "\"I said it was the best song I'd ever heard. She said she "
        "could tell just from the way I played it.\"");

    // Perks of Being a Wallflower (book 8) — Space Oddity by David Bowie (song 4)
    add_moment(8, 4, 11, 152,
        "Patrick plays Space Oddity at full volume on the drive "
        "home from the tunnel, standing in the truck bed.",
        "\"And in that moment, I swear we were infinite.\"");

    std::cout << "Moments seeded.\n";
}

// ─── Admin seeding ────────────────────────────────────────────────────────────

static void ensure_admin(Database& db) {
    for (const auto& u : db.users.loadAll()) {
        if (u.is_admin) return; // already exists
    }
    // Create default admin account
    UserRecord a{};
    a.id = db.users.nextId();
    set_str(a.username,      "admin");
    set_str(a.email,         "admin@booksongs.com");
    // hash of "Admin123!"
    uint64_t h = 5381;
    for (unsigned char c : std::string("Admin123!")) h = ((h << 5) + h) ^ c;
    h ^= 0xDEADBEEFCAFEBABEULL;
    char hex[17]; snprintf(hex, sizeof(hex), "%016llx", (unsigned long long)h);
    set_str(a.password_hash, hex);
    set_str(a.accent_color,  "#f7c948");
    set_str(a.bio,           "Site administrator");
    a.is_admin   = 1;
    a.is_deleted = 0;
    db.users.save(a);
    std::cout << "─────────────────────────────────────────\n";
    std::cout << "  Admin account created\n";
    std::cout << "  Email:    admin@booksongs.com\n";
    std::cout << "  Password: Admin123!\n";
    std::cout << "─────────────────────────────────────────\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    int    port        = 8080;
    std::string data_dir     = "./data";
    std::string frontend_dir = "./frontend";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port"    && i + 1 < argc) port        = std::stoi(argv[++i]);
        if (arg == "--data"    && i + 1 < argc) data_dir    = argv[++i];
        if (arg == "--frontend"&& i + 1 < argc) frontend_dir= argv[++i];
    }

    g_data_dir = data_dir;  // make available to handlers/mailer

    // Ensure data directories exist
    std::filesystem::create_directories(data_dir);
    std::filesystem::create_directories(data_dir + "/audio");
    std::filesystem::create_directories(data_dir + "/avatars");
    std::filesystem::create_directories(data_dir + "/covers");
    std::filesystem::create_directories(data_dir + "/receipts");

    // Schema migration: delete any .dat file whose size doesn't align
    // with the current struct size (happens when structs gain/lose fields).
    auto migrate = [&](const std::string& name, size_t record_size) {
        std::string path = data_dir + "/" + name;
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return;
        auto sz = (uint64_t)f.tellg(); f.close();
        if (sz > 0 && sz % record_size != 0) {
            std::filesystem::remove(path);
            std::cout << "Schema migration: " << name << " recreated.\n";
        }
    };
    migrate("books.dat",   sizeof(BookRecord));
    migrate("songs.dat",   sizeof(SongRecord));
    migrate("vinyls.dat",  sizeof(VinylRecord));
    migrate("users.dat",   sizeof(UserRecord));
    migrate("moments.dat", sizeof(BookSongMoment));
    migrate("orders.dat",  sizeof(OrderRecord));

    Database db(data_dir);

    if (db.books.nextId() == 1) {
        std::cout << "No data found, seeding...\n";
        seed(db);
    }

    seed_moments(db);
    ensure_admin(db);

    // Register API routes
    Router router;

    router.add("GET",    "/api/books",                  [&](const HttpRequest& r){ return handle_get_books(r, db); });
    router.add("GET",    "/api/books/:id",               [&](const HttpRequest& r){ return handle_get_book(r, db); });
    router.add("GET",    "/api/books/:id/moments",        [&](const HttpRequest& r){ return handle_get_book_moments(r, db); });
    router.add("GET",    "/api/books/:id/related",        [&](const HttpRequest& r){ return handle_related_books(r, db); });
    router.add("GET",    "/api/songs",                  [&](const HttpRequest& r){ return handle_get_songs(r, db); });
    router.add("GET",    "/api/songs/:id",               [&](const HttpRequest& r){ return handle_get_song(r, db); });
    router.add("GET",    "/api/songs/:id/moments",        [&](const HttpRequest& r){ return handle_get_song_moments(r, db); });
    router.add("GET",    "/api/songs/:id/related",        [&](const HttpRequest& r){ return handle_related_songs(r, db); });
    router.add("GET",    "/api/vinyls",                 [&](const HttpRequest& r){ return handle_get_vinyls(r, db); });
    router.add("GET",    "/api/vinyls/:id",              [&](const HttpRequest& r){ return handle_get_vinyl(r, db); });
    router.add("GET",    "/api/search",                 [&](const HttpRequest& r){ return handle_search(r, db); });

    router.add("POST",   "/api/auth/register",        [&](const HttpRequest& r){ return handle_register(r, db); });
    router.add("POST",   "/api/auth/login",           [&](const HttpRequest& r){ return handle_login(r, db); });
    router.add("POST",   "/api/auth/logout",          [&](const HttpRequest& r){ return handle_logout(r, db); });

    // Admin routes
    router.add("GET",    "/api/admin/stats",          [&](const HttpRequest& r){ return handle_admin_stats(r, db); });
    router.add("GET",    "/api/admin/users",          [&](const HttpRequest& r){ return handle_admin_users(r, db); });
    router.add("DELETE", "/api/admin/users/:id",      [&](const HttpRequest& r){ return handle_admin_delete_user(r, db); });
    router.add("POST",   "/api/admin/books",            [&](const HttpRequest& r){ return handle_admin_create_book(r, db); });
    router.add("PUT",    "/api/admin/books/:id",        [&](const HttpRequest& r){ return handle_admin_update_book(r, db); });
    router.add("DELETE", "/api/admin/books/:id",        [&](const HttpRequest& r){ return handle_admin_delete_book(r, db); });
    router.add("POST",   "/api/admin/songs",            [&](const HttpRequest& r){ return handle_admin_create_song(r, db); });
    router.add("PUT",    "/api/admin/songs/:id",        [&](const HttpRequest& r){ return handle_admin_update_song(r, db); });
    router.add("DELETE", "/api/admin/songs/:id",        [&](const HttpRequest& r){ return handle_admin_delete_song(r, db); });
    router.add("POST",   "/api/admin/vinyls",           [&](const HttpRequest& r){ return handle_admin_create_vinyl(r, db); });
    router.add("PUT",    "/api/admin/vinyls/:id",       [&](const HttpRequest& r){ return handle_admin_update_vinyl(r, db); });
    router.add("DELETE", "/api/admin/vinyls/:id",       [&](const HttpRequest& r){ return handle_admin_delete_vinyl(r, db); });
    router.add("GET",    "/api/admin/moments",          [&](const HttpRequest& r){ return handle_admin_get_moments(r, db); });
    router.add("POST",   "/api/admin/moments",          [&](const HttpRequest& r){ return handle_admin_create_moment(r, db); });
    router.add("PUT",    "/api/admin/moments/:id",      [&](const HttpRequest& r){ return handle_admin_update_moment(r, db); });
    router.add("DELETE", "/api/admin/moments/:id",      [&](const HttpRequest& r){ return handle_admin_delete_moment(r, db); });

    router.add("POST",   "/api/orders",               [&](const HttpRequest& r){ return handle_checkout(r, db); });
    router.add("GET",    "/api/orders",               [&](const HttpRequest& r){ return handle_get_orders(r, db); });

    router.add("GET",    "/api/users/me",             [&](const HttpRequest& r){ return handle_get_me(r, db); });
    router.add("PUT",    "/api/users/me",             [&](const HttpRequest& r){ return handle_update_me(r, db); });

    router.add("POST",   "/api/users/me/favorites/books/:id",   [&](const HttpRequest& r){ return handle_fav_book(r, db, true); });
    router.add("DELETE", "/api/users/me/favorites/books/:id",   [&](const HttpRequest& r){ return handle_fav_book(r, db, false); });
    router.add("POST",   "/api/users/me/favorites/songs/:id",   [&](const HttpRequest& r){ return handle_fav_song(r, db, true); });
    router.add("DELETE", "/api/users/me/favorites/songs/:id",   [&](const HttpRequest& r){ return handle_fav_song(r, db, false); });
    router.add("POST",   "/api/users/me/cart/:id",              [&](const HttpRequest& r){ return handle_cart(r, db, true); });
    router.add("DELETE", "/api/users/me/cart/:id",              [&](const HttpRequest& r){ return handle_cart(r, db, false); });
    router.add("POST",   "/api/users/me/collection/:id",        [&](const HttpRequest& r){ return handle_collection(r, db, true); });
    router.add("DELETE", "/api/users/me/collection/:id",        [&](const HttpRequest& r){ return handle_collection(r, db, false); });

    HttpServer server(port, frontend_dir, data_dir);
    server.set_router(router);
    server.start();

    return 0;
}

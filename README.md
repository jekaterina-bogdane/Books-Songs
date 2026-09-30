# Books & Songs

> **Where Literature Meets Music**

A full-stack web application that maps the relationship between books and the songs woven into their pages. Discover the soundtrack to your favourite novels, explore music through literature, and build your vinyl collection — all without a single external dependency.

---

## Features

- **Books & Songs Catalogue** — browse books and songs with covers, descriptions, and genre filters
- **Musical Moments** — chapter-level annotations linking exact quotes to the songs that play in that scene
- **30-second Audio Previews** — every song streams a real preview via the iTunes Search API
- **Vinyl Shop** — buy original pressings graded NM / VG+ / VG / G+
- **Global Search** — press `S` anywhere to search books, songs, and vinyls simultaneously
- **User Accounts** — favourites, cart, owned collection, and order history
- **Checkout with Email Receipt** — card validation, order storage, and SMTP receipt delivery via curl
- **Admin Panel** — full CRUD for books, songs, vinyls, users, and Musical Moments
- **Zero external dependencies** — pure C++17 standard library, no JSON, no database engine
- **Cross-platform** — builds and runs identically on macOS and Windows

---

## Tech Stack

| Layer | Technology |
|---|---|
| Backend | C++17, raw POSIX / Winsock2 sockets |
| Frontend | Vanilla JS (SPA), HTML5, CSS3 |
| Storage | Custom `BinaryStore<T>` — packed binary `.dat` files |
| Wire protocol | BSF (Books-Songs Format) — custom plain-text protocol |
| Email | `curl` subprocess for SMTP/TLS — no C++ TLS library |
| Build | CMake 3.16+ |
| Audio | Apple iTunes Search API (30-sec previews) |

---

## Project Structure

```
books-songs/
├── CMakeLists.txt
├── backend/
│   └── src/
│       ├── main.cpp          # Entry point, routing, seeding, schema migration
│       ├── server.h          # HTTP/1.1 server and router (~397 lines)
│       ├── handlers.cpp      # All 34 API endpoint implementations (~1 001 lines)
│       ├── handlers.h        # Handler declarations and extern globals
│       ├── models.h          # All 6 packed binary structs (#pragma pack(1))
│       ├── storage.h         # BinaryStore<T> template + Database struct
│       ├── mailer.cpp        # curl SMTP email sender + receipt file writer
│       └── mailer.h          # Mailer declarations
└── frontend/
    ├── index.html            # Single HTML shell
    └── js/
    │   ├── app.js            # All page renderers, auth, admin, checkout (~1 561 lines)
    │   ├── api.js            # BSF encode/decode, all API call definitions
    │   ├── router.js         # Hash-based SPA router (#/books, #/songs/1, …)
    │   └── player.js         # Persistent floating audio mini-player
    └── css/
        └── style.css         # Dark-mode design system, all components (~1 196 lines)
```

---

## Data Model

Six packed binary structs stored as fixed-offset `.dat` files — no SQL, no ORM, O(1) direct ID lookup.

```
BookRecord       — title, author, genre, song_ids[20]
SongRecord       — title, artist, audio_url[512], book_ids[20]
VinylRecord      — title, price_cents, condition, in_stock
UserRecord       — email, password_hash, fav_book_ids[50], cart[20], collection[50]
BookSongMoment   — book_id, song_id, chapter, page, scene[256], quote[512]
OrderRecord      — user_id, vinyl_ids[10], total_cents, card_last4[8]
```

**Storage layout:** each record sits at byte offset `(id − 1) × sizeof(struct)`.  
**Schema migration:** on startup, if `fileSize % sizeof(struct) ≠ 0` the file is deleted and reseeded automatically.

---

## BSF — Books-Songs Format

A custom plain-text wire protocol that replaces JSON everywhere: HTTP request/response bodies, localStorage, and config files.

**Response example:**
```
STATUS 200
COUNT 2
RECORD
id: 1
title: Norwegian Wood
author: Haruki Murakami
genre: Literary Fiction
END
RECORD
id: 2
title: High Fidelity
END
```

Parsing is a single `split('\n')` + `indexOf(': ')` — no library, no escaping bugs.

---

## Build

### Requirements

- CMake ≥ 3.16
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- No other dependencies

### macOS / Linux

```bash
cmake -S . -B build
cmake --build build
```

### Windows

```bat
cmake -S . -B build
cmake --build build --config Release
```

### Run

```bash
./build/books-songs-server --port 8080 --data ./data --frontend ./build/frontend
```

Then open `http://localhost:8080` in your browser.

---

## Default Admin Account

```
Email:    admin@booksongs.com
Password: Admin123!
```

The admin account is created automatically on first run when no `users.dat` exists.

---

## Email Receipts (optional)

Create `data/smtp.conf` (it is git-ignored — never commit your password) in BSF format to enable email receipt delivery:

```
url: smtps://smtp.gmail.com:465
user: your@gmail.com
pass: your-app-password
from: your@gmail.com
```

If `smtp.conf` is absent, receipts are saved to `data/receipts/order_N.txt` instead.

---

## API Overview

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/books` | List books (filter by `genre`, `q`) |
| GET | `/api/books/:id` | Book detail + linked songs |
| GET | `/api/books/:id/moments` | Musical Moments for a book |
| GET | `/api/books/:id/related` | Related books via shared songs |
| GET | `/api/songs` | List songs |
| GET | `/api/songs/:id` | Song detail + linked books |
| GET | `/api/songs/:id/moments` | Musical Moments for a song |
| GET | `/api/songs/:id/related` | Related songs |
| GET | `/api/vinyls` | Vinyl shop listing |
| GET | `/api/search?q=` | Full-text search across all catalogues |
| POST | `/api/auth/register` | Register + auto-login |
| POST | `/api/auth/login` | Login → session token |
| POST | `/api/auth/logout` | Invalidate token |
| GET | `/api/users/me` | Authenticated user profile |
| PUT | `/api/users/me` | Update profile |
| POST/DELETE | `/api/users/me/favorites/books/:id` | Toggle favourite book |
| POST/DELETE | `/api/users/me/favorites/songs/:id` | Toggle favourite song |
| POST/DELETE | `/api/users/me/cart/:id` | Add/remove vinyl from cart |
| POST/DELETE | `/api/users/me/collection/:id` | Add/remove owned vinyl |
| POST | `/api/orders` | Checkout → create order + send receipt |
| GET | `/api/orders` | User order history |
| GET | `/api/admin/stats` | Admin dashboard counts |
| GET/DELETE | `/api/admin/users` | List/delete users |
| GET/POST/PUT/DELETE | `/api/admin/books` | Admin book CRUD |
| GET/POST/PUT/DELETE | `/api/admin/songs` | Admin song CRUD |
| GET/POST/PUT/DELETE | `/api/admin/vinyls` | Admin vinyl CRUD |
| GET/POST/PUT/DELETE | `/api/admin/moments` | Admin Musical Moments CRUD |

All requests and responses use the BSF format. Authentication uses `Authorization: Bearer <token>` headers.

---

## Seed Data

On first run (when `books.dat` is absent) the server seeds:

- **8 books** — including *Norwegian Wood*, *High Fidelity*, *Just Kids*, *On The Road*, *The Great Gatsby*, *1984*, *The Perks of Being a Wallflower*, *Invisible Man*
- **10 songs** — with real iTunes 30-second preview URLs
- **6 vinyl records** — graded originals from The Beatles, Marvin Gaye, Patti Smith, David Bowie, Thelonious Monk
- **10 Musical Moments** — chapter-level book↔song annotations with verbatim quotes
- **1 admin account** — `admin@booksongs.com` / `Admin123!`

---

## Build Metrics

| File | Lines | Language |
|------|-------|----------|
| `backend/src/main.cpp` | 406 | C++ |
| `backend/src/handlers.cpp` | 1 001 | C++ |
| `backend/src/server.h` | 397 | C++ |
| `backend/src/models.h` | 107 | C++ |
| `backend/src/storage.h` | 94 | C++ |
| `backend/src/mailer.cpp` | 80 | C++ |
| `frontend/js/app.js` | 1 561 | JavaScript |
| `frontend/js/api.js` | 143 | JavaScript |
| `frontend/css/style.css` | 1 196 | CSS |
| **Total** | **~4 900** | |

---

## Author

**Jekaterina Bogdane** — Student ID 93704, Group 4503BDA  
Transport and Telecommunication Institute  
Course: Programming · Supervisor: Samuel Moveh · 2026

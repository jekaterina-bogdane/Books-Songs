#pragma once
#include <cstdint>
#include <cstring>

// All structs packed for exact binary layout — zero JSON anywhere.
// set_str() safely copies C-strings into fixed char arrays.

#pragma pack(push, 1)

struct BookRecord {
    uint32_t id;
    char     title[256];
    char     author[128];
    int16_t  year;
    char     genre[64];
    char     description[512];
    char     cover_url[256];
    uint32_t song_ids[20];
    uint8_t  song_count;
    char     isbn[24];
    uint8_t  is_deleted;
};

struct SongRecord {
    uint32_t id;
    char     title[256];
    char     artist[128];
    char     genre[64];
    char     audio_url[512];     // long enough for iTunes CDN URLs
    uint32_t duration_sec;
    int16_t  year;
    uint32_t vinyl_id;
    uint32_t book_ids[20];
    uint8_t  book_count;
    uint8_t  is_deleted;
};

struct VinylRecord {
    uint32_t id;
    char     title[256];
    char     artist[128];
    int16_t  year;
    char     label[128];
    char     condition[16];
    uint32_t price_cents;
    uint32_t song_ids[20];
    uint8_t  song_count;
    char     image_url[256];
    uint8_t  in_stock;
    uint8_t  is_deleted;
};

struct UserRecord {
    uint32_t id;
    char     username[64];
    char     email[128];
    char     password_hash[64];
    char     avatar_url[256];
    char     bio[256];
    char     fav_genres[128];           // comma-separated
    uint32_t fav_book_ids[50];
    uint8_t  fav_book_count;
    uint32_t fav_song_ids[50];
    uint8_t  fav_song_count;
    uint32_t cart_vinyl_ids[20];
    uint8_t  cart_count;
    uint32_t collection_vinyl_ids[50];
    uint8_t  collection_count;
    char     session_token[64];
    char     accent_color[8];
    uint8_t  is_admin;
    uint8_t  is_deleted;
};

// Completed purchase order
struct OrderRecord {
    uint32_t id;
    uint32_t user_id;
    uint32_t vinyl_ids[10];   // up to 10 items per order
    uint8_t  vinyl_count;
    uint32_t total_cents;
    char     card_last4[8];   // last 4 digits only — never store full card
    char     created_at[32];  // "2026-05-19 18:00:00"
    uint8_t  is_deleted;
    uint8_t  _pad[2];
};

// Records the exact moment a song appears in a book
// (chapter, page, scene description, verbatim quote)
struct BookSongMoment {
    uint32_t id;
    uint32_t book_id;
    uint32_t song_id;
    uint16_t chapter;         // 0 = unknown
    uint16_t page;            // 0 = unknown
    char     scene[512];      // context: "When Watanabe hears it on the plane…"
    char     quote[512];      // verbatim text from the book
    uint8_t  is_deleted;
};

#pragma pack(pop)

template<size_t N>
inline void set_str(char (&dst)[N], const char* src) {
    std::memset(dst, 0, N);
    if (src) std::strncpy(dst, src, N - 1);
}

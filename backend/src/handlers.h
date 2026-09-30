#pragma once
#include "server.h"
#include "storage.h"
#include <map>
#include <string>
#include <cstdint>

// In-memory session cache: token -> userId (backed by UserRecord.session_token on disk)
extern std::map<std::string, uint32_t> g_sessions;

// Set by main() so handlers can pass data_dir to the mailer
extern std::string g_data_dir;

// ─── BSF wire format helpers ─────────────────────────────────────────────────
std::string bsf_error(int code, const std::string& msg);
std::string bsf_ok();
std::string bsf_list_header(int count);
std::string bsf_record(const std::vector<std::pair<std::string,std::string>>& fields);

// ─── Public catalog ──────────────────────────────────────────────────────────
HttpResponse handle_get_books  (const HttpRequest& req, Database& db);
HttpResponse handle_get_book   (const HttpRequest& req, Database& db);
HttpResponse handle_get_songs  (const HttpRequest& req, Database& db);
HttpResponse handle_get_song   (const HttpRequest& req, Database& db);
HttpResponse handle_get_vinyls (const HttpRequest& req, Database& db);
HttpResponse handle_get_vinyl  (const HttpRequest& req, Database& db);
HttpResponse handle_search     (const HttpRequest& req, Database& db);

// Moments: exact chapter/page/quote where a song appears in a book
HttpResponse handle_get_book_moments  (const HttpRequest& req, Database& db);
HttpResponse handle_get_song_moments  (const HttpRequest& req, Database& db);

// Recommendations
HttpResponse handle_related_books (const HttpRequest& req, Database& db);
HttpResponse handle_related_songs (const HttpRequest& req, Database& db);

// ─── Auth ────────────────────────────────────────────────────────────────────
HttpResponse handle_register (const HttpRequest& req, Database& db);
HttpResponse handle_login    (const HttpRequest& req, Database& db);
HttpResponse handle_logout   (const HttpRequest& req, Database& db);

// ─── User profile ────────────────────────────────────────────────────────────
HttpResponse handle_get_me    (const HttpRequest& req, Database& db);
HttpResponse handle_update_me (const HttpRequest& req, Database& db);

HttpResponse handle_fav_book   (const HttpRequest& req, Database& db, bool add);
HttpResponse handle_fav_song   (const HttpRequest& req, Database& db, bool add);
HttpResponse handle_cart       (const HttpRequest& req, Database& db, bool add);
HttpResponse handle_collection (const HttpRequest& req, Database& db, bool add);

// ─── Orders / Checkout ───────────────────────────────────────────────────────
HttpResponse handle_checkout   (const HttpRequest& req, Database& db);
HttpResponse handle_get_orders (const HttpRequest& req, Database& db);

// ─── Admin ───────────────────────────────────────────────────────────────────
HttpResponse handle_admin_stats        (const HttpRequest& req, Database& db);
HttpResponse handle_admin_users        (const HttpRequest& req, Database& db);
HttpResponse handle_admin_delete_user  (const HttpRequest& req, Database& db);

HttpResponse handle_admin_create_book  (const HttpRequest& req, Database& db);
HttpResponse handle_admin_update_book  (const HttpRequest& req, Database& db);
HttpResponse handle_admin_delete_book  (const HttpRequest& req, Database& db);

HttpResponse handle_admin_create_song  (const HttpRequest& req, Database& db);
HttpResponse handle_admin_update_song  (const HttpRequest& req, Database& db);
HttpResponse handle_admin_delete_song  (const HttpRequest& req, Database& db);

HttpResponse handle_admin_create_vinyl (const HttpRequest& req, Database& db);
HttpResponse handle_admin_update_vinyl (const HttpRequest& req, Database& db);
HttpResponse handle_admin_delete_vinyl (const HttpRequest& req, Database& db);

HttpResponse handle_admin_create_moment(const HttpRequest& req, Database& db);
HttpResponse handle_admin_update_moment(const HttpRequest& req, Database& db);
HttpResponse handle_admin_delete_moment(const HttpRequest& req, Database& db);
HttpResponse handle_admin_get_moments  (const HttpRequest& req, Database& db);

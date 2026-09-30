#pragma once

// Cross-platform socket abstraction
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET sock_t;
#  define SOCK_INVALID  INVALID_SOCKET
#  define sock_close    closesocket
#  define SHUT_RDWR     SD_BOTH
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <signal.h>
   typedef int sock_t;
#  define SOCK_INVALID  (-1)
#  define sock_close    close
#endif

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cstring>

// ─── Request / Response ───────────────────────────────────────────────────────

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;   // URL path params + query params
    std::string body;
    std::string session_token;
};

struct HttpResponse {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    bool is_binary = false;

    static HttpResponse bsf(const std::string& body) {
        HttpResponse r;
        r.status = 200;
        r.body   = body;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        return r;
    }

    static HttpResponse error(int code, const std::string& msg) {
        HttpResponse r;
        r.status = code;
        r.body   = "STATUS " + std::to_string(code) + "\nMESSAGE " + msg + "\n";
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        return r;
    }

    static HttpResponse html(const std::string& body) {
        HttpResponse r;
        r.status = 200;
        r.body   = body;
        r.headers["Content-Type"] = "text/html; charset=utf-8";
        return r;
    }

    static HttpResponse file_resp(const std::string& content, const std::string& mime) {
        HttpResponse r;
        r.status = 200;
        r.body   = content;
        r.headers["Content-Type"] = mime;
        r.is_binary = true;
        return r;
    }

    static HttpResponse no_content() {
        HttpResponse r;
        r.status = 204;
        return r;
    }
};

// ─── Router ──────────────────────────────────────────────────────────────────

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
    struct Route { std::string method, pattern; Handler handler; };
    std::vector<Route> routes_;

    static bool match(const std::string& pattern, const std::string& path,
                      std::map<std::string, std::string>& out) {
        auto split = [](const std::string& s, char d) {
            std::vector<std::string> v;
            std::istringstream ss(s);
            std::string t;
            while (std::getline(ss, t, d)) v.push_back(t);
            return v;
        };
        auto pp = split(pattern, '/');
        auto up = split(path,    '/');
        if (pp.size() != up.size()) return false;
        for (size_t i = 0; i < pp.size(); i++) {
            if (!pp[i].empty() && pp[i][0] == ':') {
                out[pp[i].substr(1)] = up[i];
            } else if (pp[i] != up[i]) {
                return false;
            }
        }
        return true;
    }

public:
    void add(const std::string& method, const std::string& pattern, Handler h) {
        routes_.push_back({method, pattern, std::move(h)});
    }

    HttpResponse handle(const HttpRequest& req) const {
        for (const auto& r : routes_) {
            if (r.method != req.method) continue;
            std::map<std::string, std::string> p;
            if (match(r.pattern, req.path, p)) {
                HttpRequest mr = req;
                mr.params.insert(p.begin(), p.end());
                return r.handler(mr);
            }
        }
        return HttpResponse::error(404, "Not Found");
    }
};

// ─── Static file helpers ─────────────────────────────────────────────────────

inline std::string mime_type(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot);
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".css")  return "text/css; charset=utf-8";
    if (ext == ".js")   return "application/javascript; charset=utf-8";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".mp3")  return "audio/mpeg";
    if (ext == ".m4a" || ext == ".mp4") return "audio/mp4";
    if (ext == ".aac")  return "audio/aac";
    if (ext == ".ogg")  return "audio/ogg";
    if (ext == ".wav")  return "audio/wav";
    return "application/octet-stream";
}

inline bool read_file(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}

// Prevent directory traversal
inline bool safe_path(const std::string& p) {
    return p.find("..") == std::string::npos;
}

// ─── HTTP parsing ─────────────────────────────────────────────────────────────

inline void parse_query(const std::string& q, std::map<std::string, std::string>& out) {
    std::istringstream ss(q);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto eq = token.find('=');
        if (eq != std::string::npos)
            out[token.substr(0, eq)] = token.substr(eq + 1);
    }
}

inline HttpRequest parse_request(const std::string& raw) {
    HttpRequest req;
    size_t pos = 0;

    // Request line
    size_t eol = raw.find("\r\n");
    if (eol == std::string::npos) return req;
    std::string line = raw.substr(0, eol);
    size_t s1 = line.find(' ');
    size_t s2 = line.rfind(' ');
    if (s1 == std::string::npos || s1 == s2) return req;
    req.method = line.substr(0, s1);
    std::string full = line.substr(s1 + 1, s2 - s1 - 1);
    auto qm = full.find('?');
    if (qm != std::string::npos) {
        req.path = full.substr(0, qm);
        parse_query(full.substr(qm + 1), req.params);
    } else {
        req.path = full;
    }
    pos = eol + 2;

    // Headers
    while (pos < raw.size()) {
        size_t e = raw.find("\r\n", pos);
        if (e == std::string::npos) break;
        if (e == pos) { pos += 2; break; }
        std::string h = raw.substr(pos, e - pos);
        auto col = h.find(": ");
        if (col != std::string::npos) {
            std::string k = h.substr(0, col);
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            req.headers[k] = h.substr(col + 2);
        }
        pos = e + 2;
    }

    req.body = raw.substr(pos);

    // Session token from Authorization: Bearer <token>
    auto it = req.headers.find("authorization");
    if (it != req.headers.end() && it->second.substr(0, 7) == "Bearer ") {
        req.session_token = it->second.substr(7);
    }

    return req;
}

inline std::string status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

inline std::string build_response(const HttpResponse& r) {
    std::ostringstream out;
    out << "HTTP/1.1 " << r.status << " " << status_text(r.status) << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    out << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    out << "Connection: close\r\n";
    for (const auto& [k, v] : r.headers)
        out << k << ": " << v << "\r\n";
    if (r.headers.find("Content-Length") == r.headers.end())
        out << "Content-Length: " << r.body.size() << "\r\n";
    out << "\r\n";
    out << r.body;
    return out.str();
}

// ─── HTTP Server ──────────────────────────────────────────────────────────────

class HttpServer {
    int         port_;
    std::string frontend_dir_;
    std::string data_dir_;
    Router*     router_ = nullptr;

    static std::string recv_all(sock_t client) {
        std::string data;
        char buf[8192];
        while (data.find("\r\n\r\n") == std::string::npos) {
            int n = static_cast<int>(recv(client, buf, sizeof(buf), 0));
            if (n <= 0) return data;
            data.append(buf, n);
        }
        // Read body if Content-Length present
        auto cl = data.find("Content-Length: ");
        if (cl != std::string::npos) {
            size_t end = data.find("\r\n", cl);
            int content_len = std::stoi(data.substr(cl + 16, end - cl - 16));
            size_t header_end = data.find("\r\n\r\n") + 4;
            int got = static_cast<int>(data.size()) - static_cast<int>(header_end);
            while (got < content_len) {
                int n = static_cast<int>(recv(client, buf, sizeof(buf), 0));
                if (n <= 0) break;
                data.append(buf, n);
                got += n;
            }
        }
        return data;
    }

    HttpResponse serve_static(const std::string& path) const {
        if (!safe_path(path)) return HttpResponse::error(403, "Forbidden");

        // Map /audio/ and /avatars/ and /covers/ to data dirs
        std::string file_path;
        if (path.substr(0, 7) == "/audio/")
            file_path = data_dir_ + "/audio/" + path.substr(7);
        else if (path.substr(0, 9) == "/avatars/")
            file_path = data_dir_ + "/avatars/" + path.substr(9);
        else if (path.substr(0, 8) == "/covers/")
            file_path = data_dir_ + "/covers/" + path.substr(8);
        else
            file_path = frontend_dir_ + path;

        std::string content;
        if (!read_file(file_path, content))
            return HttpResponse::error(404, "File not found");

        return HttpResponse::file_resp(content, mime_type(file_path));
    }

    void handle_connection(sock_t client) const {
        std::string raw = recv_all(client);
        if (raw.empty()) { sock_close(client); return; }

        HttpRequest req = parse_request(raw);

        HttpResponse resp;

        if (req.method == "OPTIONS") {
            resp = HttpResponse::no_content();
        } else if (req.path.substr(0, 5) == "/api/") {
            if (router_) resp = router_->handle(req);
            else resp = HttpResponse::error(500, "No router");
        } else {
            // Static files: paths with a known extension
            bool has_ext = req.path.rfind('.') != std::string::npos;
            bool is_data = req.path.substr(0, 7) == "/audio/" ||
                           req.path.substr(0, 9) == "/avatars/" ||
                           req.path.substr(0, 8) == "/covers/";
            if (has_ext || is_data) {
                resp = serve_static(req.path);
            } else {
                // SPA fallback – serve index.html
                std::string content;
                if (read_file(frontend_dir_ + "/index.html", content))
                    resp = HttpResponse::html(content);
                else
                    resp = HttpResponse::error(404, "Frontend not found");
            }
        }

        std::string out = build_response(resp);
        send(client, out.c_str(), static_cast<int>(out.size()), 0);
        sock_close(client);
    }

public:
    HttpServer(int port, std::string frontend_dir, std::string data_dir)
        : port_(port)
        , frontend_dir_(std::move(frontend_dir))
        , data_dir_(std::move(data_dir))
    {}

    void set_router(Router& r) { router_ = &r; }

    void start() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#else
        signal(SIGPIPE, SIG_IGN);
#endif
        sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
        if (srv == SOCK_INVALID) { std::cerr << "socket() failed\n"; return; }

        int opt = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&opt), sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(static_cast<uint16_t>(port_));

        if (bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "bind() failed on port " << port_ << "\n";
            sock_close(srv); return;
        }
        listen(srv, 128);
        std::cout << "Books & Songs server running on http://localhost:" << port_ << "\n";

        while (true) {
            sock_t client = accept(srv, nullptr, nullptr);
            if (client == SOCK_INVALID) continue;
            std::thread([this, client]() { handle_connection(client); }).detach();
        }
    }
};

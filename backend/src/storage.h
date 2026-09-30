#pragma once
#include "models.h"
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

template<typename T>
class BinaryStore {
    std::string path_;

public:
    explicit BinaryStore(std::string path) : path_(std::move(path)) {}

    bool save(const T& rec) {
        if (rec.id < 1) return false;
        std::fstream f(path_, std::ios::binary | std::ios::in | std::ios::out);
        if (!f.is_open()) {
            std::ofstream create(path_, std::ios::binary);
            create.close();
            f.open(path_, std::ios::binary | std::ios::in | std::ios::out);
            if (!f.is_open()) return false;
        }
        std::streamoff off = static_cast<std::streamoff>(rec.id - 1) * sizeof(T);
        f.seekp(off);
        f.write(reinterpret_cast<const char*>(&rec), sizeof(T));
        return f.good();
    }

    bool load(uint32_t id, T& rec) const {
        if (id < 1) return false;
        std::ifstream f(path_, std::ios::binary);
        if (!f.is_open()) return false;
        std::streamoff off = static_cast<std::streamoff>(id - 1) * sizeof(T);
        f.seekg(off);
        f.read(reinterpret_cast<char*>(&rec), sizeof(T));
        if (f.gcount() < static_cast<std::streamsize>(sizeof(T))) return false;
        return rec.is_deleted == 0;
    }

    // Load ignoring is_deleted – used internally to patch records
    bool load_raw(uint32_t id, T& rec) const {
        if (id < 1) return false;
        std::ifstream f(path_, std::ios::binary);
        if (!f.is_open()) return false;
        std::streamoff off = static_cast<std::streamoff>(id - 1) * sizeof(T);
        f.seekg(off);
        f.read(reinterpret_cast<char*>(&rec), sizeof(T));
        return f.gcount() == static_cast<std::streamsize>(sizeof(T));
    }

    std::vector<T> loadAll() const {
        std::vector<T> result;
        std::ifstream f(path_, std::ios::binary);
        if (!f.is_open()) return result;
        T rec;
        while (f.read(reinterpret_cast<char*>(&rec), sizeof(T))) {
            if (!rec.is_deleted) result.push_back(rec);
        }
        return result;
    }

    uint32_t nextId() const {
        std::ifstream f(path_, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return 1;
        auto sz = static_cast<uint64_t>(f.tellg());
        return static_cast<uint32_t>(sz / sizeof(T)) + 1;
    }

    bool remove(uint32_t id) {
        T rec{};
        if (!load_raw(id, rec)) return false;
        rec.is_deleted = 1;
        return save(rec);
    }
};

struct Database {
    BinaryStore<BookRecord>     books;
    BinaryStore<SongRecord>     songs;
    BinaryStore<VinylRecord>    vinyls;
    BinaryStore<UserRecord>     users;
    BinaryStore<BookSongMoment> moments;
    BinaryStore<OrderRecord>    orders;

    explicit Database(const std::string& dir)
        : books  (dir + "/books.dat")
        , songs  (dir + "/songs.dat")
        , vinyls (dir + "/vinyls.dat")
        , users  (dir + "/users.dat")
        , moments(dir + "/moments.dat")
        , orders (dir + "/orders.dat")
    {}
};

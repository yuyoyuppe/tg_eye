#pragma once

#include <filesystem>
#include <memory>
#include <cassert>
#include <stacktrace>

#include <sqlite3.h>

namespace fs = std::filesystem;

struct sqlite3_deleter {
    void operator()(sqlite3 * db) const {
        assert(db);
        sqlite3_close(db);
    }
    void operator()(sqlite3_stmt * stmt) const {
        assert(stmt);
        sqlite3_finalize(stmt);
    }
};

struct db final {
    db(fs::path sqlite3_path);

    void insert_user_status(int64_t telegram_user_id, int32_t timestamp, bool is_online);

  private:
    void CHK(const int rc, const int expected = SQLITE_OK, std::stacktrace st = std::stacktrace::current());

    std::unique_ptr<sqlite3, sqlite3_deleter>      _db;
    std::unique_ptr<sqlite3_stmt, sqlite3_deleter> _insert_stmt;
};
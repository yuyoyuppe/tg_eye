#include "db.hpp"

#include <array>

namespace {
void check_sqlite3_error(int rc, sqlite3 * db) {}
}
db::db(fs::path sqlite3_path) {
    sqlite3 * db = {};

    CHK(sqlite3_open_v2(
      sqlite3_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_EXRESCODE, nullptr));

    CHK(sqlite3_exec(db,
                     R"d(
create table if not exists user_status (
    telegram_user_id integer not null,
    timestamp integer not null,
    status integer not null);)d",
                     nullptr,
                     nullptr,
                     nullptr));

    const char insert_stmt[] = R"d(insert into user_status (telegram_user_id, timestamp, status) values (?, ?, ?);))d";
    sqlite3_stmt * stmt      = {};
    CHK(sqlite3_prepare_v3(db, insert_stmt, sizeof(insert_stmt), SQLITE_PREPARE_PERSISTENT, &stmt, nullptr));
    _insert_stmt.reset(stmt);
}

void db::insert_user_status(int64_t telegram_user_id, int32_t timestamp, bool is_online) {
    CHK(sqlite3_reset(_insert_stmt.get()));
    CHK(sqlite3_bind_int64(_insert_stmt.get(), 1, telegram_user_id));
    CHK(sqlite3_bind_int(_insert_stmt.get(), 2, timestamp));
    CHK(sqlite3_bind_int(_insert_stmt.get(), 3, is_online ? 1 : 0));
    CHK(sqlite3_step(_insert_stmt.get()), SQLITE_DONE);
}

void db::CHK(const int rc, const int expected, std::stacktrace st) {
    if(rc == expected)
        return;

    const auto & entry = *st.begin();
    throw std::runtime_error(std::format("sqlite3 error in [{} {}:{}]: {}",
                                         entry.description(),
                                         entry.source_file(),
                                         entry.source_line(),
                                         sqlite3_errmsg(_db.get())));
}

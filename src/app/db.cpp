#include "db.hpp"

db::db(fs::path sqlite3_path) {
    sqlite3 * db = {};

    CHK(sqlite3_open_v2(
      sqlite3_path.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_EXRESCODE, nullptr));

    CHK(sqlite3_exec(db,
                     R"d(
create table if not exists user_status (
    telegram_user_id integer not null,
    timestamp integer not null,
    status integer not null);

create table if not exists user_info (
    telegram_user_id integer not null unique,
    full_name text,
    profile_photo blob,
    username text,
    primary key(telegram_user_id));)d",
                     nullptr,
                     nullptr,
                     nullptr));

    const char insert_status_stmt[] =
      R"d(
insert into user_status 
    (telegram_user_id, timestamp, status)
values (?, ?, ?);))d";
    sqlite3_stmt * stmt = {};
    CHK(sqlite3_prepare_v3(
      db, insert_status_stmt, sizeof(insert_status_stmt), SQLITE_PREPARE_PERSISTENT, &stmt, nullptr));
    _insert_user_status_stmt.reset(stmt);
    stmt = {};

    const char update_info_stmt[] =
      R"d(
insert into user_info
    (telegram_user_id, full_name, profile_photo, username)
values (?, ?, ?, ?)
on conflict(telegram_user_id) do update set
    full_name = coalesce(excluded.full_name, user_info.full_name),
    profile_photo = coalesce(excluded.profile_photo, user_info.profile_photo),
    username = coalesce(excluded.username, user_info.username);)d";

    CHK(sqlite3_prepare_v3(db, update_info_stmt, sizeof(update_info_stmt), SQLITE_PREPARE_PERSISTENT, &stmt, nullptr));
    _update_user_info_stmt.reset(stmt);
    stmt = {};
}

void db::insert_user_status(int64_t telegram_user_id, int32_t timestamp, bool is_online) {
    CHK(sqlite3_bind_int64(_insert_user_status_stmt.get(), 1, telegram_user_id));
    CHK(sqlite3_bind_int(_insert_user_status_stmt.get(), 2, timestamp));
    CHK(sqlite3_bind_int(_insert_user_status_stmt.get(), 3, is_online ? 1 : 0));
    CHK(sqlite3_step(_insert_user_status_stmt.get()), SQLITE_DONE);
    CHK(sqlite3_reset(_insert_user_status_stmt.get()));
}

void db::update_user_info(const int64_t             telegram_user_id,
                          const std::string &       full_name,
                          const std::string &       user_name,
                          const std::string & profile_photo) {
    CHK(sqlite3_bind_int64(_update_user_info_stmt.get(), 1, telegram_user_id));
    CHK(sqlite3_bind_text(_update_user_info_stmt.get(),
                          2,
                          full_name.empty() ? nullptr : full_name.c_str(),
                          static_cast<int>(full_name.size()),
                          SQLITE_TRANSIENT));
    CHK(sqlite3_bind_blob(_update_user_info_stmt.get(),
                          3,
                          profile_photo.empty() ? nullptr : profile_photo.data(),
                          static_cast<int>(profile_photo.size()),
                          SQLITE_TRANSIENT));
    CHK(sqlite3_bind_text(_update_user_info_stmt.get(),
                          4,
                          user_name.empty() ? nullptr : user_name.c_str(),
                          static_cast<int>(user_name.size()),
                          SQLITE_TRANSIENT));
    CHK(sqlite3_step(_update_user_info_stmt.get()), SQLITE_DONE);
    CHK(sqlite3_reset(_update_user_info_stmt.get()));
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

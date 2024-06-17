#pragma once

#include "tg_client.hpp"
#include "db.hpp"

struct tg_eye_client : tg_client<tg_eye_client> {
    tg_eye_client(db user_status_db) : _db{std::move(user_status_db)} {}

    void operator()(td_api::updateUserStatus & update_user_status);
    void operator()(td_api::updateUser & update_user);

  private:
    std::string get_full_name(std::int64_t user_id) const;
    std::unordered_map<std::int64_t, td_api::object_ptr<td_api::user>> _users;
    db _db;
};
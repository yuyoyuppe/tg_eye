#pragma once

#include "tg_client.hpp"
#include "db.hpp"

struct tg_eye_client : tg_client<tg_eye_client> {
    tg_eye_client(db user_status_db) : _db{std::move(user_status_db)} {}

    void operator()(td_api::updateUserStatus & update_user_status);

  private:
    std::string get_user_name(std::int64_t user_id) const;

    db _db;
};
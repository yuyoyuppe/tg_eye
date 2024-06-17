#include "tg_eye_client.hpp"

#include <iomanip>
#include <ctime>
#include <iostream>

namespace {
std::string format_timestamp(int32_t timestamp) {
    std::time_t        time = static_cast<std::time_t>(timestamp);
    std::tm *          tm   = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
}

std::string tg_eye_client::get_user_name(std::int64_t user_id) const {
    auto it = _users.find(user_id);
    if(it == _users.end()) {
        return "unknown user";
    }

    auto result = it->second->first_name_;
    if(!it->second->last_name_.empty()) {
        result += " ";
        result += it->second->last_name_;
    }
    return result;
}


void tg_eye_client::operator()(td_api::updateUserStatus & update_user_status) {
    const auto user_id = update_user_status.user_id_;
    std::cout << get_user_name(user_id) << " is ";
    bool is_online = false;
    auto timestamp = static_cast<int32_t>(std::time(nullptr));
    if(const auto online = try_move_as<td_api::userStatusOnline>(update_user_status.status_)) {
        is_online = true;
        std::cout << "online until " << format_timestamp(online->expires_) << "\n";
    } else if(const auto offline = try_move_as<td_api::userStatusOffline>(update_user_status.status_)) {
        timestamp = offline->was_online_ + 1;
        std::cout << "offline since " << format_timestamp(offline->was_online_) << "\n";
    }

    try {
        _db.insert_user_status(user_id, timestamp, is_online);
    } catch(const std::exception & ex) { std::cerr << ex.what() << std::endl; }
}
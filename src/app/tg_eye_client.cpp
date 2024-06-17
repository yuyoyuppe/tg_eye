#include "tg_eye_client.hpp"

#include <iomanip>
#include <ctime>
#include <iostream>

namespace {
static const std::string EMPTY_STRING;

std::string format_timestamp(int32_t timestamp) {
    std::time_t        time = static_cast<std::time_t>(timestamp);
    std::tm *          tm   = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string get_full_name(const td_api::object_ptr<td_api::user> & user) {
    auto result = user->first_name_;
    if(!user->last_name_.empty()) {
        result += " ";
        result += user->last_name_;
    }

    return result;
}
}

std::string tg_eye_client::get_full_name(std::int64_t user_id) const {
    auto it = _users.find(user_id);
    return it == _users.end() ? EMPTY_STRING : ::get_full_name(it->second);
}

void tg_eye_client::operator()(td_api::updateUserStatus & update_user_status) {
    const auto user_id = update_user_status.user_id_;
    std::cout << get_full_name(user_id) << " is ";
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

void tg_eye_client::operator()(td_api::updateUser & update_user) {
    if(!update_user.user_->is_contact_ || update_user.user_->is_fake_ || update_user.user_->is_scam_)
        return;

    auto user_id = update_user.user_->id_;

    const std::string * username = &EMPTY_STRING;
    if(update_user.user_->usernames_ && !update_user.user_->usernames_->active_usernames_.empty())
        username = &update_user.user_->usernames_->active_usernames_[0];

    const std::string * profile_photo = &EMPTY_STRING;
    if(update_user.user_->profile_photo_ && update_user.user_->profile_photo_->minithumbnail_)
        profile_photo = &update_user.user_->profile_photo_->minithumbnail_->data_;

    try {
        _db.update_user_info(user_id, ::get_full_name(update_user.user_), *username, *profile_photo);
    } catch(const std::exception & ex) { std::cerr << ex.what() << std::endl; }

    _users[user_id] = std::move(update_user.user_);
}

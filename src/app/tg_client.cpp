#include "tg_client.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "overloaded.hpp"

template <typename T, typename Base>
td_api::object_ptr<T> try_move_as(td_api::object_ptr<Base> & ptr) {
    return ptr->get_id() == T::ID ? td::move_tl_object_as<T>(std::move(ptr)) : nullptr;
}

std::string format_timestamp(int32_t timestamp) {
    std::time_t        time = static_cast<std::time_t>(timestamp);
    std::tm *          tm   = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}


tg_client::tg_client() {
    client_id_ = client_manager_.create_client_id();
    send_query(td_api::make_object<td_api::getOption>("version"), {});
    event_loop_ = std::thread{[this] { event_loop(); }};
}

void tg_client::send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
    const auto query_id = next_query_id();
    if(handler)
        handlers_.emplace(query_id, std::move(handler));
    client_manager_.send(client_id_, query_id, std::move(f));
}

void tg_client::process_update(td_api::object_ptr<td_api::Object> update) {
    const auto handler = overloaded(
      [this](td_api::updateAuthorizationState & update_authorization_state) {
          authorization_state_ = std::move(update_authorization_state.authorization_state_);
          on_authorization_state_update();
      },
      [this](td_api::updateUser & update_user) {
          auto user_id    = update_user.user_->id_;
          users_[user_id] = std::move(update_user.user_);
      },
      [this](td_api::updateUserStatus & update_user_status) {
          const auto user_id = update_user_status.user_id_;
          std::cout << get_user_name(user_id) << " is ";

          if(const auto online = try_move_as<td_api::userStatusOnline>(update_user_status.status_)) {
              std::cout << "online until " << format_timestamp(online->expires_) << "\n";
          } else if(const auto offline = try_move_as<td_api::userStatusOffline>(update_user_status.status_)) {
              std::cout << "offline since " << format_timestamp(offline->was_online_) << "\n";
          }
      },
      [](auto & /*update*/) {});
    td_api::downcast_call(*update, handler);
}

void tg_client::process_response(td::ClientManager::Response response) {
    if(!response.object) {
        return;
    }
    // std::cout << response.request_id << " " << to_string(response.object) << std::endl;
    if(response.request_id == 0) {
        return process_update(std::move(response.object));
    }
    auto it = handlers_.find(response.request_id);
    if(it != handlers_.end()) {
        it->second(std::move(response.object));
        handlers_.erase(it);
    }
}

void tg_client::restart() {
    client_manager_ = {};
    *this           = {};
}

std::string tg_client::get_user_name(std::int64_t user_id) const {
    auto it = users_.find(user_id);
    if(it == users_.end()) {
        return "unknown user";
    }

    auto result = it->second->first_name_;
    if(!it->second->last_name_.empty()) {
        result += " ";
        result += it->second->last_name_;
    }
    return result;
}

void tg_client::event_loop() {
    while(true) {
        if(auto response = client_manager_.receive(60); response.object)
            process_response(std::move(response));
    }
}

void tg_client::on_authorization_state_update() {
    authentication_query_id_++;

    const auto handler = overloaded(
      [this](td_api::authorizationStateWaitRegistration &) { std::cout << "Registration is not supported\n"; },
      [this](td_api::authorizationStateReady &) {
          are_authorized_ = true;
          std::cout << "Authorization is completed" << std::endl;
      },
      [this](td_api::authorizationStateLoggingOut &) {
          are_authorized_ = false;
          std::cout << "Logging out" << std::endl;
      },
      [this](td_api::authorizationStateClosing &) { std::cout << "Closing" << std::endl; },
      [this](td_api::authorizationStateClosed &) {
          are_authorized_ = false;
          need_restart_   = true;
          std::cout << "Terminated" << std::endl;
      },
      [this](td_api::authorizationStateWaitPhoneNumber &) {
          std::cout << "Enter phone number: " << std::flush;
          std::string phone_number;
          std::cin >> phone_number;
          send_query(td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone_number, nullptr),
                     create_authentication_query_handler());
      },
      [this](td_api::authorizationStateWaitEmailAddress &) {
          std::cout << "Enter email address: " << std::flush;
          std::string email_address;
          std::cin >> email_address;
          send_query(td_api::make_object<td_api::setAuthenticationEmailAddress>(email_address),
                     create_authentication_query_handler());
      },
      [this](td_api::authorizationStateWaitEmailCode &) {
          std::cout << "Enter email authentication code: " << std::flush;
          std::string code;
          std::cin >> code;
          send_query(td_api::make_object<td_api::checkAuthenticationEmailCode>(
                       td_api::make_object<td_api::emailAddressAuthenticationCode>(code)),
                     create_authentication_query_handler());
      },
      [this](td_api::authorizationStateWaitCode &) {
          std::cout << "Enter authentication code: " << std::flush;
          std::string code;
          std::cin >> code;
          send_query(td_api::make_object<td_api::checkAuthenticationCode>(code), create_authentication_query_handler());
      },
      [this](td_api::authorizationStateWaitPassword &) {
          std::cout << "Enter authentication password: " << std::flush;
          std::string password;
          std::getline(std::cin, password);
          send_query(td_api::make_object<td_api::checkAuthenticationPassword>(password),
                     create_authentication_query_handler());
      },
      [this](td_api::authorizationStateWaitOtherDeviceConfirmation & state) {
          std::cout << "Confirm this login link on another device: " << state.link_ << std::endl;
      },
      [this](td_api::authorizationStateWaitTdlibParameters &) {
          auto request                     = td_api::make_object<td_api::setTdlibParameters>();
          request->database_directory_     = "tdlib";
          request->use_message_database_   = false;
          request->use_secret_chats_       = false;
          request->api_id_                 = 94575;
          request->api_hash_               = "a3406de8d171bb422bb6ddf3bbd800e2";
          request->system_language_code_   = "en";
          request->device_model_           = "Desktop";
          request->application_version_    = "1.0";
          request->use_chat_info_database_ = true;
          send_query(std::move(request), create_authentication_query_handler());
      });
    td_api::downcast_call(*authorization_state_, handler);
}

std::function<void(tg_client::Object)> tg_client::create_authentication_query_handler() {
    return [this, id = authentication_query_id_](Object object) {
        if(id == authentication_query_id_) {
            check_authentication_error(std::move(object));
        }
    };
}

void tg_client::check_authentication_error(tg_client::Object object) {
    if(const auto error = try_move_as<td_api::error>(object)) {
        std::cout << "Error: " << to_string(error) << std::flush;
        on_authorization_state_update();
    }
}

#pragma once
#include <iostream>
#include <sstream>

#include "overloaded.hpp"

template <typename Derived>
tg_client<Derived>::tg_client() {
    _client_id = _client_manager.create_client_id();
    send_query(td_api::make_object<td_api::getOption>("version"), {});
    _event_loop = std::thread{[this] { event_loop(); }};
}

template <typename Derived>
void tg_client<Derived>::send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
    const auto query_id = next_query_id();
    if(handler)
        handlers_.emplace(query_id, std::move(handler));
    _client_manager.send(_client_id, query_id, std::move(f));
}

template <typename Derived>
void tg_client<Derived>::process_update(td_api::object_ptr<td_api::Object> update) {
    const auto handler = overloaded(
      [this](td_api::updateAuthorizationState & update_authorization_state) {
          authorization_state_ = std::move(update_authorization_state.authorization_state_);
          on_authorization_state_update();
      },
      [this](td_api::updateUser & update_user) {
          auto user_id    = update_user.user_->id_;
          _users[user_id] = std::move(update_user.user_);
      },
      [this]<typename update_t>(update_t & update) {
          constexpr bool derived_has_handler = requires(Derived client, update_t update) { client(update); };
          if constexpr(derived_has_handler) {
              static_cast<Derived *>(this)->operator()(update);
          }
      });
    td_api::downcast_call(*update, handler);
}

template <typename Derived>
void tg_client<Derived>::process_response(td::ClientManager::Response response) {
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

template <typename Derived>
void tg_client<Derived>::restart() {
    _client_manager = {};
    *this           = {};
}

template <typename Derived>
void tg_client<Derived>::event_loop() {
    while(true) {
        if(auto response = _client_manager.receive(60); response.object)
            process_response(std::move(response));
    }
}

template <typename Derived>
void tg_client<Derived>::on_authorization_state_update() {
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

template <typename Derived>
std::function<void(Object)> tg_client<Derived>::create_authentication_query_handler() {
    return [this, id = authentication_query_id_](Object object) {
        if(id == authentication_query_id_) {
            check_authentication_error(std::move(object));
        }
    };
}

template <typename Derived>
void tg_client<Derived>::check_authentication_error(Object object) {
    if(const auto error = try_move_as<td_api::error>(object)) {
        std::cout << "Error: " << to_string(error) << std::flush;
        on_authorization_state_update();
    }
}

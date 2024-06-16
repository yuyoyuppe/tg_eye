#pragma once

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <thread>

namespace td_api = td::td_api;

struct tg_client {
    tg_client();

  private:
    using Object = td_api::object_ptr<td_api::Object>;

    inline std::uint64_t next_query_id() { return ++current_query_id_; }
    void                 send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler);
    void                 process_update(td_api::object_ptr<td_api::Object> update);
    void                 process_response(td::ClientManager::Response response);
    void                 restart();
    std::string          get_user_name(std::int64_t user_id) const;
    void                 event_loop();

    td::ClientManager client_manager_;
    std::int32_t      client_id_        = {};
    std::uint64_t     current_query_id_ = {};
    std::thread       event_loop_       = {};

    // Authentication
    void                                           on_authorization_state_update();
    std::function<void(Object)>                    create_authentication_query_handler();
    void                                           check_authentication_error(Object object);
    bool                                           need_restart_            = {};
    std::uint64_t                                  authentication_query_id_ = {};
    td_api::object_ptr<td_api::AuthorizationState> authorization_state_;
    bool                                           are_authorized_ = {};

    std::unordered_map<std::int64_t, td_api::object_ptr<td_api::user>> users_;
    std::unordered_map<std::uint64_t, std::function<void(Object)>>     handlers_;
};
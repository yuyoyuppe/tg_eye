#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <thread>

#include "tg.hpp"

template <typename Derived>
class tg_client {
  public:
    tg_client();

  protected:
    void send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler);
    std::unordered_map<std::int64_t, td_api::object_ptr<td_api::user>> _users;

  private:
    inline std::uint64_t next_query_id() { return ++_current_query_id; }
    void                 process_update(td_api::object_ptr<td_api::Object> update);
    void                 process_response(td::ClientManager::Response response);
    void                 restart();
    void                 event_loop();

    td::ClientManager _client_manager;
    std::int32_t      _client_id        = {};
    std::uint64_t     _current_query_id = {};
    std::thread       _event_loop       = {};

    // Authentication
    void                                           on_authorization_state_update();
    std::function<void(Object)>                    create_authentication_query_handler();
    void                                           check_authentication_error(Object object);
    bool                                           need_restart_            = {};
    std::uint64_t                                  authentication_query_id_ = {};
    td_api::object_ptr<td_api::AuthorizationState> authorization_state_;
    bool                                           are_authorized_ = {};

    std::unordered_map<std::uint64_t, std::function<void(Object)>> handlers_;
};

#include "tg_client.ipp"

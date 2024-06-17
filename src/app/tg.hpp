#pragma once

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

namespace td_api = td::td_api;
using Object     = td_api::object_ptr<td_api::Object>;

template <typename T, typename Base>
inline td_api::object_ptr<T> try_move_as(td_api::object_ptr<Base> & ptr) {
    static_assert(std::is_base_of_v<Base, T>);
    return ptr->get_id() == T::ID ? td::move_tl_object_as<T>(std::move(ptr)) : nullptr;
}
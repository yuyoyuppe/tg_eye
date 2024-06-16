#include <cstdint>
#include <functional>
#include <iostream>
#include "overloaded.hpp"
#include "tg_client.hpp"

int main() {
    system("chcp 65001 > nul");

    td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));

    tg_client client;

    std::this_thread::sleep_for(std::chrono::seconds(600));
}
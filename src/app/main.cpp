#include <cstdint>
#include <functional>
#include <iostream>

#include "tg_eye_client.hpp"


int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));

    tg_eye_client client{db{"user_status.sqlite3"}};

    std::this_thread::sleep_for(std::chrono::seconds(600));

    return 0;
}

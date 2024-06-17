#include <cstdint>
#include <functional>
#include <iostream>

#include "tg_eye_client.hpp"

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));

    try {
        tg_eye_client client{db{"tg_eye_stats.sqlite3"}};
        std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));
    } catch(const std::exception & ex) { std::cerr << ex.what() << std::endl; }


    return 0;
}

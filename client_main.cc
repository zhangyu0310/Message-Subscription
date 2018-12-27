//
// Created by zhangyu on 18-12-26.
//

#include <string>
#include "client.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        bounce::Logger::get("bounce_console")->error("args are too few...");
        bounce::Logger::get("bounce_console")->error(
                "Usage: client [ip] [port]");
        return 1;
    }
    std::string ip(argv[1]);
    std::string port_str(argv[2]);
    int port = std::stoi(port_str);
    MSClient client(ip, static_cast<uint16_t>(port), 3);
    bounce::SockAddress addr("127.0.0.1", 9281);
    client.connect(addr);
    client.loop();
}
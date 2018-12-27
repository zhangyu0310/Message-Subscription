//
// Created by zhangyu on 18-12-26.
//

#include "server.h"

int main() {
    MSServer server("127.0.0.1", 9281, 3);
    server.start();
}
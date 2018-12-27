//
// Created by zhangyu on 18-12-26.
//

#include "client.h"

#include <functional>

#include <nlohmann/json.hpp>
using nlohmann::json;
using std::string;

MSClient::MSClient(const std::string& ip,
                   uint16_t port,
                   uint32_t thread_num) :
                   server_(&loop_, ip, port, thread_num),
                   connector_(&loop_, &server_) {
    server_.setConnectionCallback(std::bind(&MSClient::ConnectCallback,
                                            this, std::placeholders::_1));

    server_.setMessageCallback(
            std::bind(
                    &MSClient::MessageCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3));

    server_.setWriteCompleteCallback(std::bind(&MSClient::WriteCompCallback,
                                               this, std::placeholders::_1));

    server_.setErrorCallback(std::bind(&MSClient::ErrorCallback,
                                       this, std::placeholders::_1));
}

void MSClient::connect(const SockAddress& addr) {
    server_.start();
    server_.setNewConnection(STDIN_FILENO, bounce::SockAddress());
    connector_.connect(addr);
}

void MSClient::loop() {
    loop_.loop();
}

void MSClient::ConnectCallback(const TcpServer::TcpConnectionPtr& conn) {
    if (conn->state() == TcpConnection::connected) {
        if (conn->getFd() != STDIN_FILENO) {
            conn_ = conn;
        }
        bounce::Logger::get("bounce_console")->info("Connected...");
    } else if (conn->state() == TcpConnection::disconnected) {
        bounce::Logger::get("bounce_console")->info("DisConnected...");
        conn_.reset();
    }
}

void MSClient::MessageCallback(const TcpServer::TcpConnectionPtr& conn, 
        Buffer* buffer, time_t time) {
    if (conn->getFd() == STDIN_FILENO) {
        dealStdInput(buffer);
    } else {
        json msg_json;
        int32_t size = buffer->peekInt32();
        if (size > 0 && size <= buffer->readableBytes()) {
            buffer->retrieve(sizeof(size));
            string str = buffer->readAsString(size - sizeof(size));
            msg_json = json::parse(str.c_str());
        } else {
            bounce::Logger::get("bounce_console")->info(
                    "message missing..."
                    "time is {}", time);
            return;
        }
        string type = msg_json["type"].get<string>();
        if (type == "subscription") {
            string command = msg_json["command"].get<string>();
            string subscription = msg_json["project"].get<string>();
            if (command == "delete") {
                bounce::Logger::get("bounce_console")->info(
                        "Subscription {} was deleted...",
                        subscription);
            } else if (command == "subscription") {
                string what = msg_json["what"].get<string>();
                bounce::Logger::get("bounce_console")->info(
                        "Subscription {} was talking {}",
                        subscription, what);
            } else {
                bounce::Logger::get("bounce_console")->info(
                        "server command is error"
                        "time is {}", time);
            }
        }
    }
}

void MSClient::WriteCompCallback(const TcpServer::TcpConnectionPtr& conn) {
    // print log, which ip + port
}

void MSClient::ErrorCallback(const TcpServer::TcpConnectionPtr& conn) {
    bounce::Logger::get("bounce_console")->info(
            "ErrorCallback, errno is {}"
            "time is {}", errno);
}

void MSClient::dealStdInput(Buffer* buffer) {
    // Only two command: add & delete
    auto ptr = buffer->peek();
    bool add = true;
    if (strncmp("add", ptr, 3) == 0) {
        buffer->retrieve(4); // with a space
    } else if (strncmp("delete", ptr, 6) == 0) {
        buffer->retrieve(7); // with a space
        add = false;
    } else {
        bounce::Logger::get("bounce_console")->error("stdin input error");
        buffer->retrieve(buffer->readableBytes());
        return;
    }
    ptr = buffer->peek();
    size_t index = 0;
    while (*(ptr + index) != '\n') {
        ++index;
    }
    std::string value = buffer->readAsString(index);
    buffer->retrieve(1); // delete useless '\n'
    json msg_json;
    msg_json["type"] = "subscription";
    msg_json["project"] = value;
    if (add) {
        msg_json["command"] = "add";
    } else {
        msg_json["command"] = "delete";
    }
    string str = msg_json.dump();
    int32_t size = 4 + static_cast<int32_t>(str.size());
    Buffer buf;
    buf.writeInt32(size);
    buf.append(str);
    if (conn_ != nullptr) {
        conn_->send(buf);
    }
}
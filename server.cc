//
// Created by zhangyu on 18-12-26.
//

#include "server.h"

#include <functional>

#include <nlohmann/json.hpp>
using nlohmann::json;
using std::string;

MSServer::MSServer(const std::string& ip,
                   uint16_t port,
                   uint32_t thread_num) :
                   server_(&loop_, ip, port, thread_num) {
    server_.setConnectionCallback(std::bind(&MSServer::ConnectCallback,
            this, std::placeholders::_1));

    server_.setMessageCallback(
            std::bind(
                    &MSServer::MessageCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3));

    server_.setWriteCompleteCallback(std::bind(&MSServer::WriteCompCallback,
            this, std::placeholders::_1));

    server_.setErrorCallback(std::bind(&MSServer::ErrorCallback,
            this, std::placeholders::_1));
}

void MSServer::start() {
    server_.start();
    server_.setNewConnection(STDIN_FILENO, bounce::SockAddress());
    loop_.loop();
}

void MSServer::ConnectCallback(const TcpServer::TcpConnectionPtr &conn) {
    if (conn->state() == TcpConnection::connected) {
        bounce::Logger::get("bounce_console")->info("Connected...");
    } else if (conn->state() == TcpConnection::disconnected) {
        bounce::Logger::get("bounce_console")->info("DisConnected...");
        for (auto& it : mes_observer_map_) {
            it.second.erase(conn);
        }
    }
}

void MSServer::MessageCallback(const TcpServer::TcpConnectionPtr &conn,
        Buffer* buffer, time_t time) {
    if (conn->getFd() == STDIN_FILENO) {
        dealStdInput(buffer);
        return;
    }
    // deal client message.
    // size = 4 + message_size
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
        auto it = mes_observer_map_.find(subscription);
        if (it == mes_observer_map_.end()) {
            bounce::Logger::get("bounce_console")->info(
                    "can't find subscription project "
                    "time is {}", time);
            //json ret_json;
            //ret_json["type"] = "subscription";
            //ret_json["command"] = command;
            //ret_json["project"] = it->first;
            return;
        }
        if (command == "add") {
            // set can remove duplicates.
            it->second.insert(conn);
        } else if (command == "delete") {
            it->second.erase(conn);
        } else {
            bounce::Logger::get("bounce_console")->info(
                    "client command is error"
                    "time is {}", time);
        }
    }
}

void MSServer::WriteCompCallback(const TcpServer::TcpConnectionPtr &conn) {
    // print log, which ip + port
}

void MSServer::ErrorCallback(const TcpServer::TcpConnectionPtr &conn) {
    bounce::Logger::get("bounce_console")->info(
            "ErrorCallback, errno is {}"
            "time is {}", errno);
}

void MSServer::dealStdInput(Buffer* buffer) {
    // Only two command: add & delete
    auto ptr = buffer->peek();
    bool add = true;
    string command = "add";
    if (strncmp("add", ptr, 3) == 0) {
        buffer->retrieve(4); // with a space
    } else if (strncmp("delete", ptr, 6) == 0) {
        buffer->retrieve(7); // with a space
        add = false;
        command = "delete";
    } else if (strncmp("subscription", ptr, 12) == 0) {
        buffer->retrieve(13); // with a space
        add = false;
        command = "subscription";
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
    std::string what;
    if (command == "subscription") {
        what = value.substr(value.find(' ') + 1);
        value = value.substr(0, value.find(' '));
    }
    buffer->retrieve(1); // delete useless '\n'
    auto it = mes_observer_map_.find(value);
    if (it == mes_observer_map_.end()) {
        if (add) {
            mes_observer_map_.insert(std::make_pair(value, ObserverSet()));
        } else {
            bounce::Logger::get("bounce_console")->info(
                    "delete or subscription nothing...");
        }
    } else {
        if (!add) {
            // send message for observers.
            json j;
            j["type"] = "subscription";
            j["command"] = command;
            j["project"] = it->first;
            if (command == "subscription") {
                j["what"] = what;
            }
            string str = j.dump();
            int32_t size = 4 + static_cast<int32_t>(str.size());
            Buffer buf;
            buf.writeInt32(size);
            buf.append(str);
            for (auto& observer : it->second) {
                // send will clear buffer, so must use a tmp object.
                Buffer tmp_buf(buf);
                observer->send(tmp_buf);
            }
            // delete subscription project
            if (command == "delete") {
                mes_observer_map_.erase(it);
            }
        }
    }
    buffer->retrieve(buffer->readableBytes());
}
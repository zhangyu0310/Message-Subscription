//
// Created by zhangyu on 18-12-26.
//

#ifndef MESSAGE_SUBSCRIPTION_SERVER_H
#define MESSAGE_SUBSCRIPTION_SERVER_H

#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <bounce/event_loop.h>
#include <bounce/logger.h>
#include <bounce/tcp_server.h>
#include <bounce/tcp_connection.h>
#include <bounce/buffer.h>

using bounce::TcpServer;
using bounce::EventLoop;
using bounce::Buffer;
using bounce::TcpConnection;

class MSServer {
    typedef std::set<TcpServer::TcpConnectionPtr> ObserverSet;
    typedef std::map<std::string, ObserverSet> MessageMap;
public:
    MSServer(const std::string& ip,
            uint16_t port,
            uint32_t thread_num = 0);
    ~MSServer() = default;
    MSServer(const MSServer&) = delete;
    MSServer& operator=(const MSServer&) = delete;

    void start();

private:
    void ConnectCallback(const TcpServer::TcpConnectionPtr& conn);
    void MessageCallback(
            const TcpServer::TcpConnectionPtr& conn,
            Buffer* buffer, time_t time);
    void WriteCompCallback(const TcpServer::TcpConnectionPtr& conn);
    void ErrorCallback(const TcpServer::TcpConnectionPtr& conn);
    void dealStdInput(Buffer* buffer);

    EventLoop loop_;
    TcpServer server_;
    MessageMap mes_observer_map_;
};

#endif //MESSAGE_SUBSCRIPTION_SERVER_H

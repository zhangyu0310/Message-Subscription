//
// Created by zhangyu on 18-12-26.
//

#ifndef MESSAGE_SUBSCRIPTION_CLIENT_H
#define MESSAGE_SUBSCRIPTION_CLIENT_H

#include <bounce/connector.h>
#include <bounce/event_loop.h>
#include <bounce/logger.h>
#include <bounce/tcp_server.h>
#include <bounce/tcp_connection.h>
#include <bounce/buffer.h>

using bounce::TcpServer;
using bounce::EventLoop;
using bounce::Buffer;
using bounce::TcpConnection;
using bounce::Connector;
using bounce::SockAddress;

class MSClient {
public:
    MSClient(const std::string& ip,
             uint16_t port,
             uint32_t thread_num = 0);
    ~MSClient() = default;
    MSClient(const MSClient&) = delete;
    MSClient& operator=(const MSClient&) = delete;

    void connect(const SockAddress& addr);
    void loop();

private:
    void ConnectCallback(const TcpServer::TcpConnectionPtr& conn);
    void MessageCallback(
            const TcpServer::TcpConnectionPtr& conn,
            Buffer* buffer, time_t time);
    void WriteCompCallback(const TcpServer::TcpConnectionPtr& conn);
    void ErrorCallback(const TcpServer::TcpConnectionPtr& conn);
    
    //void ConnectorErrorCallback();
    void dealStdInput(Buffer* buf);

    EventLoop loop_;
    TcpServer server_;
    Connector connector_;
    TcpServer::TcpConnectionPtr conn_;
};

#endif //MESSAGE_SUBSCRIPTION_CLIENT_H

/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
TCP/IP Sockets Client
*/


#ifndef CONNECTION_H
#define CONNECTION_H

#define MAX_REQUEST_LENGTH 65535
#define SERVER_OK 0
#define READ_SOCKET_ERROR -1

#define CONNECTION_OK             0
#define CONNECTION_ERR_REFUSED    -1
#define CONNECTION_ERR            -2

// Cross platform lib for network and low level io
#include <boost/asio.hpp>

#include <string>
#include <vector>

#include "ThreadSafeQueue.h"

class Connection {

 public:
    Connection();
    ~Connection();

    int connectRemote(const std::string &ip, const std::string &port);
    int send(ThreadSafeQueue<std::string> *q);
    void closeConnection();
    
 private:
    std::string ip_;
    std::string port_;
    boost::asio::io_service *ioService_;
    boost::asio::ip::tcp::socket *socket_;

};

#endif

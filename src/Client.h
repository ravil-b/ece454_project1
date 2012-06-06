/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
TCP/IP Sockets Client
*/

// Cross platform lib for network and low level io

#define CLIENT_CONNECTION_OK             0
#define CLIENT_CONNECTION_ERR_REFUSED    1
#define CLIENT_CONNECTION_ERR            2
#define CLIENT_SEND_OK                   0
#define CLIENT_SEND_ERR                  1

#include <boost/asio.hpp>

#include <string>

class Client {

 public:
    Client();
    ~Client();

    int connect(const std::string &ip, const std::string &port);
    int send(const std::string data);
    void closeConnection();
    
 private:
    boost::asio::io_service *ioService_;
    boost::asio::ip::tcp::socket *socket_;

};


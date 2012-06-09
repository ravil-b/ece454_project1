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

#define SERVER_OK 0
#define READ_SOCKET_ERROR -1

#define CONNECTION_OK             0
#define CONNECTION_ERR_REFUSED    -1
#define CONNECTION_ERR            -2

class Frame;

// Cross platform lib for network and low level io
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>
#include <vector>
#include <map>

#include "ThreadSafeQueue.h"



class Session : public boost::enable_shared_from_this<Session> {
 public:
    
    static boost::shared_ptr<Session> create(boost::asio::io_service *io_service, 
        ThreadSafeQueue<Frame *> *receiveQ, ThreadSafeQueue<Frame *> *sendQ = NULL);
    static boost::shared_ptr<Session> create(boost::asio::ip::tcp::socket *socket, 
        ThreadSafeQueue<Frame *> *receiveQ, ThreadSafeQueue<Frame *> *sendQ = NULL);
    
    ~Session();
    boost::asio::ip::tcp::socket *socket();
    void start();
    void handleRead(const boost::system::error_code &error, size_t bytesTransferred);
    void handleWrite(const boost::system::error_code &error);
 private:
    Session(boost::asio::io_service *ioService, ThreadSafeQueue<Frame *> *receiveQ,
	    ThreadSafeQueue<Frame *> *sendQ = NULL);
    Session(boost::asio::ip::tcp::socket *socket, ThreadSafeQueue<Frame *> *receiveQ,
	    ThreadSafeQueue<Frame *> *sendQ = NULL);
    void startWrite();

    boost::asio::ip::tcp::socket *socket_;
    ThreadSafeQueue<Frame *> *receiveQ_;
    ThreadSafeQueue<Frame *> *sendQ_;
    Frame *incomingFrame_;
    Frame *outgoingFrame_;
    unsigned int sessionId;
    bool writeStarted_;
    boost::thread *startWriteThread_;
};

class Server {
 public:
    Server(boost::asio::io_service *Service, short port, ThreadSafeQueue<Frame *> *receiveQ);
    void handleAccept(boost::shared_ptr<Session> newSession, const boost::system::error_code& error);
 private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_service *ioService_;
    ThreadSafeQueue<Frame *> *receiveQ_;
};

class Connection {

 public:
    Connection();
    ~Connection();

    int connect(const std::string &ip, const std::string &port, 
		int peerId, ThreadSafeQueue<Frame *> *sendQ);
    void closeConnection(int peerId);
    int startServer(const std::string &port, ThreadSafeQueue<Frame *> *receiveQ);
    
 private:
    std::string port_;
    boost::asio::io_service *ioService_;
    boost::thread *serverThread_;
    Server *server_;
    ThreadSafeQueue<Frame *> *receiveQ_;
    // A mapping between session IDs and send queues they can use
    //map<unsigned int, ThreadSafeQueue<Frame *> *> sessionIdAndSendQueueMap;
    // A mapping between session IDs and peer numbers
    //map<unsigned int, int> sessionIdAndPeerIdMap;
};

#endif

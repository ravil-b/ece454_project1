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

#define DEFAULT_PORT            5555

class Frame;
class Connection;

// Cross platform lib for network and low level io
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>


#include <string>
#include <vector>
#include <map>

#include "ThreadSafeQueue.h"


struct Request{
    unsigned int requestId;
    std::string ip;
    unsigned short port;
    Frame *frame;
};

class Session : public boost::enable_shared_from_this<Session> {
 public:
    
    static boost::shared_ptr<Session> create(unsigned int sessionId, Connection *connection,
        boost::asio::io_service *io_service, ThreadSafeQueue<Request> *receiveQ, 
        ThreadSafeQueue<Frame *> *sendQ = NULL);
    static boost::shared_ptr<Session> create(unsigned int sessionId, Connection *connection,
	boost::asio::ip::tcp::socket *socket, ThreadSafeQueue<Request> *receiveQ, 
	ThreadSafeQueue<Frame *> *sendQ = NULL);
    
    ~Session();
    boost::asio::ip::tcp::socket *socket();
    void start();
    void handleRead(const boost::system::error_code &error, size_t bytesTransferred);
    void handleWrite(const boost::system::error_code &error);
    unsigned int sessionId();
    void setSendQ(ThreadSafeQueue<Frame *> *sendQ);
 private:
    Session(unsigned int sessionId, Connection *connection, boost::asio::io_service *ioService, 
	    ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ = NULL);
    Session(unsigned int sessionId, Connection *connection, boost::asio::ip::tcp::socket *socket, 
	    ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ = NULL);
    void startWrite();

    unsigned int sessionId_;
    Connection *connection_;
    boost::asio::ip::tcp::socket *socket_;
    ThreadSafeQueue<Request> *receiveQ_;
    ThreadSafeQueue<Frame *> *sendQ_;
    Frame *incomingFrame_;
    size_t incomingFrameIndex_;
    Frame *outgoingFrame_;
    bool writeStarted_;
    boost::thread *startWriteThread_;
};

class Server {
 public:
    Server(Connection *connection, boost::asio::io_service *Service, 
	   short port, ThreadSafeQueue<Request> *receiveQ);
    void handleAccept(boost::shared_ptr<Session> newSession, 
		      const boost::system::error_code& error);
 private:
    Connection *connection_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_service *ioService_;
    ThreadSafeQueue<Request> *receiveQ_;
};

class Connection {

 public:
    Connection();
    ~Connection();

    int connect(const std::string &ip, const std::string &port, 
		unsigned int *sessionId, ThreadSafeQueue<Frame *> *sendQ);
    void endSession(unsigned int sessionId);
    int startServer(const std::string &port, ThreadSafeQueue<Request> *receiveQ);
    ThreadSafeQueue<Frame *> *getReplyQueue(unsigned int requestId);
    unsigned int generateSessionId();
    ThreadSafeQueue<Frame *> *addSession(unsigned int sessionId, 
					 boost::shared_ptr<Session> session);
    void removeSession(unsigned int sessionId);
    
 private:
    std::string port_;
    boost::asio::io_service *ioService_;
    boost::thread *serverThread_;
    Server *server_;
    ThreadSafeQueue<Request> *receiveQ_;
    unsigned int sessionIdCount_;
    boost::mutex sessionIdGenMutex_;
    boost::mutex sessionMapMutex_;
    // A mapping between session IDs and send queues they can use
    std::map<unsigned int, ThreadSafeQueue<Frame *> *> sessionIdAndSendQueueMap_;
    std::map<unsigned int, boost::shared_ptr<Session> > sessionIdAndSessionMap_;
};

#endif

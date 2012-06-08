#ifndef SERVER_H_
#define SERVER_H_

#define MAX_REQUEST_LENGTH 65536

#define SERVER_OK 0
#define READ_SOCKET_ERROR -1

#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include "Frames.h"


typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

class Server{
public:
    Server();
    ~Server();
    int waitForRequest(Frame * request, short port);

private:
    int readRequest(Frame * request, SocketPtr socket);
    boost::asio::io_service * ioService_;
};




#endif /* SERVER_H_ */

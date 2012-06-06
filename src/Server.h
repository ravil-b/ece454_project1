#ifndef SERVER_H_
#define SERVER_H_

#define MAX_REQUEST_LENGTH 65535

#define SERVER_OK 0
#define READ_SOCKET_ERROR -1

#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

class Server{
public:
	int waitForRequest(RawRequest * request, short port);

private:
	int readRequest(RawRequest * request, SocketPtr socket);

};




#endif /* SERVER_H_ */

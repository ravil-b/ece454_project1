#include <cstdlib>

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>

#include "Connection.h"
#include "debug.h"
#include "ThreadSafeQueue.h"
#include "Frames.h"

using boost::asio::ip::tcp;

// Some async read/write code was adapted from the boost documentation:
// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp



// int
// Connection::send(ThreadSafeQueue<std::string> *q){
//     TRACE("Connection.cpp", "Sending data.");
//     if (socket_ == NULL){
// 	// TODO Error code
// 	return CONNECTION_ERR;
//     }
//     std::string str;
//     while (q->pop(&str)){
// 	boost::asio::write(*socket_, boost::asio::buffer(str, str.size()));
//     }
//     return CONNECTION_OK;
// }

void
Connection::closeConnection(int peerId){
    TRACE("Connection.cpp", "Closing Connection.");
    // TODO
}


/*
 * NEW CODE
 */ 



/*
 * Session
 */

Session::Session(boost::asio::io_service *ioService, ThreadSafeQueue<Frame *> *receiveQ,
		 ThreadSafeQueue<Frame *> *sendQ)
    : receiveQ_(receiveQ),
      sendQ_(sendQ), 
      incomingFrame_(NULL),
      outgoingFrame_(NULL){
    socket_ = new tcp::socket(*ioService);
}

// Session::Session(tcp::socket *socket, ThreadSafeQueue<Frame *> *receiveQ,
// 		 ThreadSafeQueue<Frame *> *sendQ)
//     : socket_(*socket),
//       receiveQ_(receiveQ),
//       sendQ_(sendQ), 
//       incomingFrame_(NULL),
//       outgoingFrame_(NULL){
// }

Session::~Session(){
    if (incomingFrame_ != NULL){
	delete incomingFrame_;
    }
    if (outgoingFrame_ != NULL){
	delete outgoingFrame_;
    }
}

tcp::socket *
Session:: socket(){
    return socket_;
}

void 
Session::start(){
    incomingFrame_ = new Frame();
    socket_->async_read_some(boost::asio::buffer(incomingFrame_->serializedData, FRAME_LENGTH),
			    boost::bind(&Session::handleRead, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

// This handler is invoked when async read is completed.
// Push the data onto the queue, and start another read or write request.
void 
Session::handleRead(const boost::system::error_code& error, size_t bytesTransferred){
    if (!error){
	if (bytesTransferred != (unsigned)FRAME_LENGTH){
	    std::cerr << "Received incomplete frame" << std::endl;
	    receiveQ_->push(incomingFrame_);
	    incomingFrame_ = NULL;
	}
	else{
	    receiveQ_->push(incomingFrame_);
	    incomingFrame_ = NULL;
	}
	// We may start with only the receiving queue and then get updated with the 
	// send queue. 
	if (sendQ_ != NULL){
	    if (sendQ_->pop(&outgoingFrame_)){
		// data available to send
		boost::asio::async_write(*socket_,
					 boost::asio::buffer(outgoingFrame_->serializedData, FRAME_LENGTH),
					 boost::bind(&Session::handleWrite, this,
						     boost::asio::placeholders::error));
		return;
	    }
	}
	// no data to send or no send queue. try reading another frame
	incomingFrame_ = new Frame();
	socket_->async_read_some(boost::asio::buffer(incomingFrame_->serializedData, FRAME_LENGTH),
				boost::bind(&Session::handleRead, this,
					    boost::asio::placeholders::error,
					    boost::asio::placeholders::bytes_transferred));
	
    }
    else{ // TODO HANDLE CLOSING CONNECTION CASE
	std::cerr << "Socket read error" << std::endl;
	delete this;
    }
}

// This handler is invoked when all data has been written to the socket
void 
Session::handleWrite(const boost::system::error_code& error)
{
    delete outgoingFrame_;
    outgoingFrame_ = NULL;
    if (!error){
	incomingFrame_ = new Frame();
	socket_->async_read_some(boost::asio::buffer(incomingFrame_->serializedData, FRAME_LENGTH),
				boost::bind(&Session::handleRead, this,
					    boost::asio::placeholders::error,
					    boost::asio::placeholders::bytes_transferred));
    }
    else{
	std::cerr << "Socket write error" << std::endl;
	delete this;
    }
}


/*
 * Server
 */

Server::Server(boost::asio::io_service *ioService, short port, 
	       ThreadSafeQueue<Frame *> *receiveQ)
    : acceptor_(*ioService, tcp::endpoint(tcp::v4(), port)),
      ioService_(ioService),
      receiveQ_(receiveQ){

    Session *newSession = new Session(ioService_, receiveQ);
    acceptor_.async_accept(*(newSession->socket()),
			   boost::bind(&Server::handleAccept, this, newSession,
				       boost::asio::placeholders::error));
}

void
Server::handleAccept(Session *newSession, const boost::system::error_code& error){
    TRACE("Connection.cpp", "Accepting a connection");
    if (!error){
	newSession->start();
	newSession = new Session(ioService_, receiveQ_);
	acceptor_.async_accept(*(newSession->socket()),
			       boost::bind(&Server::handleAccept, this, newSession,
					   boost::asio::placeholders::error));
    }
    else{
	delete newSession;
	std::cerr << "Failed to accept a connection" << std::endl;
    }
}

/*
 * Connection
 */

Connection::Connection(){
    ioService_ = new boost::asio::io_service();
}

Connection::~Connection(){
    // TODO finish the server thread 
    delete ioService_;
    delete server_;
}

int
Connection::startServer(const std::string &port, ThreadSafeQueue<Frame *> *receiveQ){
    TRACE("Connection.cpp", "Starting server");
    int portNum = std::atoi(port.c_str());
    if (portNum == 0){
	return CONNECTION_ERR;
    }
    server_ = new Server(ioService_, (short)portNum, receiveQ);
    // the call to run() is blocking until all the socket activities are done.
    // in our case, it will block for the duration of the program, because we
    // want to keep accepting more connections.
    // so, run it in a thread, so that we can return from here
    serverThread_ = new boost::thread(boost::bind(&boost::asio::io_service::run, ioService_));
    //ioService_->run();
    return CONNECTION_OK;
}

// int
// Connection::connect(const std::string &ip, const std::string &port,
// 		    int peerId, ThreadSafeQueue<Frame *> sendQueue){
//     TRACE("Connection.cpp", "Connecting to socket.");
//     int retVal = CONNECTION_OK;

//     tcp::resolver res(*ioService_);
//     tcp::resolver::query q(tcp::v4(), ip, port);
//     tcp::resolver::iterator iter = res.resolve(q);

//     socket = new tcp::socket(*ioService_);
    
//     try{
// 	socket_->connect(*iter);
//     }
//     catch (boost::system::system_error e){	
// 	if (e.code() == boost::system::errc::connection_refused){
// 	    retVal = CONNECTION_ERR_REFUSED;
// 	}
// 	else{
// 	    retVal = CONNECTION_ERR;
// 	}
// 	// TODO check how to safely close the socket
// 	delete socket_;
// 	socket_ = NULL;
//     }
    
//     return retVal;
// }

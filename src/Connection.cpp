
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include "Connection.h"
#include "debug.h"
#include "ThreadSafeQueue.h"

using boost::asio::ip::tcp;


Connection::Connection(){
    ioService_ = new boost::asio::io_service();
    socket_ = NULL;
}

Connection::~Connection(){
    delete ioService_;
    if (socket_ != NULL){
	closeConnection();
	delete socket_;
    }
}

int
Connection::connectRemote(const std::string &ip, const std::string &port){
    TRACE("Connection.cpp", "Connecting to socket.");
    int retVal = CONNECTION_OK;

    tcp::resolver res(*ioService_);
    tcp::resolver::query q(tcp::v4(), ip, port);
    tcp::resolver::iterator iter = res.resolve(q);

    socket_ = new tcp::socket(*ioService_);
    
    try{
	socket_->connect(*iter);
    }
    catch (boost::system::system_error e){	
	if (e.code() == boost::system::errc::connection_refused){
	    retVal = CONNECTION_ERR_REFUSED;
	}
	else{
	    retVal = CONNECTION_ERR;
	}
	// TODO check how to safely close the socket
	delete socket_;
	socket_ = NULL;
    }
    
    return retVal;
}

int
Connection::send(ThreadSafeQueue<std::string> *q){
    TRACE("Connection.cpp", "Sending data.");
    if (socket_ == NULL){
	// TODO Error code
	return CONNECTION_ERR;
    }
    std::string str;
    while (q->pop(&str)){
	boost::asio::write(*socket_, boost::asio::buffer(str, str.size()));
    }
    return CONNECTION_OK;
}

void
Connection::closeConnection(){
    TRACE("Connection.cpp", "Closing Connection.");
    if (socket_ == NULL){
	return;
    }
    try{
	socket_->shutdown(tcp::socket::shutdown_send);
    }
    catch(...){
    }
    try{
	socket_->close();
    }
    catch(...){
    }
    delete socket_;
    socket_ = NULL;
}



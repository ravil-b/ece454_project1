
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include "Client.h"
#include "debug.h"

using boost::asio::ip::tcp;

Client::Client(){
    ioService_ = new boost::asio::io_service();
    socket_ = NULL;
}

Client::~Client(){
    delete ioService_;
    if (socket_ != NULL){
	closeConnection();
	delete socket_;
    }
}

int
Client::connect(const std::string &ip, const std::string &port){
    TRACE("Client.cpp", "Connecting to socket.");
    int retVal = CLIENT_CONNECTION_OK;

    tcp::resolver res(*ioService_);
    tcp::resolver::query q(tcp::v4(), ip, port);
    tcp::resolver::iterator iter = res.resolve(q);

    socket_ = new tcp::socket(*ioService_);
    
    try{
	socket_->connect(*iter);
    }
    catch (boost::system::system_error e){	
	if (e.code() == boost::system::errc::connection_refused){
	    retVal = CLIENT_CONNECTION_ERR_REFUSED;
	}
	else{
	    retVal = CLIENT_CONNECTION_ERR;
	}
	// TODO check how to safely close the socket
	delete socket_;
	socket_ = NULL;
    }
    
    return retVal;
}

int
Client::send(const std::string data){
    TRACE("Client.cpp", "Sending data.");
    if (socket_ == NULL){
	// TODO Error code
	return CLIENT_SEND_ERR;
    }
    boost::asio::write(*socket_, boost::asio::buffer(data, data.size()));
    return CLIENT_SEND_OK;
}

void
Client::closeConnection(){
    TRACE("Client.cpp", "Closing Connection.");
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

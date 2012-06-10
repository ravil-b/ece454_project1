#define BOOST_THREAD_USE_LIB

#include <cstdlib>

#include <boost/bind.hpp>

#include "Connection.h"
#include "debug.h"
#include "ThreadSafeQueue.h"
#include "Frames.h"

using boost::asio::ip::tcp;

// Some async read/write code was adapted from the boost documentation:
// http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp


/*
 * Session
 */

boost::shared_ptr<Session>
Session::create(unsigned int sessionId, Connection *connection, boost::asio::io_service *ioService, 
		ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ){

    return boost::shared_ptr<Session>(new Session(sessionId, connection, 
						  ioService, receiveQ, sendQ));
}

boost::shared_ptr<Session>
Session::create(unsigned int sessionId, Connection *connection, tcp::socket *socket, 
		ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ){

    return boost::shared_ptr<Session>(new Session(sessionId, connection, 
						  socket, receiveQ, sendQ));
}

Session::Session(unsigned int sessionId, Connection *connection, boost::asio::io_service *ioService, 
		 ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ)
    : sessionId_(sessionId),
      connection_(connection),
      receiveQ_(receiveQ),
      sendQ_(sendQ), 
      incomingFrame_(NULL),
      incomingFrameIndex_(0),
      outgoingFrame_(NULL),
      writeStarted_(false),
      startWriteThread_(NULL){
    socket_ = new tcp::socket(*ioService);
}

Session::Session(unsigned int sessionId, Connection *connection, tcp::socket *socket, 
		 ThreadSafeQueue<Request> *receiveQ, ThreadSafeQueue<Frame *> *sendQ)
    : sessionId_(sessionId),
      connection_(connection),
      socket_(socket),
      receiveQ_(receiveQ),
      sendQ_(sendQ), 
      incomingFrame_(NULL),
      incomingFrameIndex_(0),
      outgoingFrame_(NULL),
      writeStarted_(false),
      startWriteThread_(NULL){
}

Session::~Session(){
    TRACE("Connection.cpp", "In session destructor");
    if (outgoingFrame_ != NULL){
	delete outgoingFrame_;
    }
    if (incomingFrame_ != NULL){
	delete incomingFrame_;
    }    
    if (startWriteThread_ != NULL){	
	startWriteThread_->join();
	delete startWriteThread_;
    }
    connection_->removeSession(sessionId_);
}

tcp::socket *
Session:: socket(){
    return socket_;
}

unsigned int
Session::sessionId(){
    return sessionId_;
}

void
Session::setSendQ(ThreadSafeQueue<Frame *> *sendQ){
    sendQ_ = sendQ;
}

void 
Session::start(){
    TRACE("Connection.cpp", "Starting the new session");
    incomingFrame_ = new Frame();
    socket_->async_read_some(boost::asio::buffer(incomingFrame_->serializedData, FRAME_LENGTH),
    			    boost::bind(&Session::handleRead, shared_from_this(),
    					boost::asio::placeholders::error,
    					boost::asio::placeholders::bytes_transferred));

    if (sendQ_ != NULL && !writeStarted_){
	writeStarted_ = true;
	startWriteThread_ = new boost::thread(boost::bind(&Session::startWrite, 
							  shared_from_this()));
    }
}

// This handler is invoked when async read is completed.
// Push the data onto the queue, and start another read or write request.
void 
Session::handleRead(const boost::system::error_code& error, size_t bytesTransferred){
    if (!error){
	if ((bytesTransferred + incomingFrameIndex_) < (unsigned)FRAME_LENGTH){
	    char buf[10]; 
	    sprintf(buf, "%d", incomingFrame_->serializedData[0]);
	    std::cout << "FIRST CHAR IS " << buf << std::endl;
	    std::cerr << "Still incomplete frame" << std::endl;
	    std::cerr << "Received " << bytesTransferred + incomingFrameIndex_ << " bytes " << std::endl;
	    incomingFrameIndex_ += bytesTransferred;
	}
	else{
	    std::cerr << "Complete frame?" << std::endl;
	    std::cerr << "Received " << bytesTransferred + incomingFrameIndex_ << " bytes " << std::endl;
	    char buf[10]; 
	    sprintf(buf, "%d", incomingFrame_->serializedData[0]);
	    std::cout << "FIRST CHAR IS " << buf << std::endl;
	    incomingFrameIndex_ = 0;
	    Request newRequest;
	    newRequest.requestId = sessionId_;
	    newRequest.frame = incomingFrame_;
	    newRequest.ip = socket()->remote_endpoint().address().to_string();
	    newRequest.port = socket()->remote_endpoint().port();
	    receiveQ_->push(newRequest);
	    incomingFrame_ = NULL;
	    incomingFrame_ = new Frame();
	}
	// We may start with only the receiving queue and then get updated with the 
	// send queue. If we haven't started writing before, start here.
	if (sendQ_ != NULL && !writeStarted_){
	    writeStarted_ = true;
	    startWriteThread_ = new boost::thread(boost::bind(&Session::startWrite, 
							      shared_from_this()));
	}		
	
	socket_->async_read_some(boost::asio::buffer(
	    incomingFrame_->serializedData + incomingFrameIndex_, 
	    FRAME_LENGTH - incomingFrameIndex_),
				 boost::bind(&Session::handleRead, shared_from_this(),
					     boost::asio::placeholders::error,
					     boost::asio::placeholders::bytes_transferred));
	
    }
    else{ 
	std::cerr << "Socket read error" << std::endl;
	std::cerr << error.message() << std::endl;
	// The socket is likely disconnected. Stop waiting for data to write.
	if (sendQ_ != NULL){
	    sendQ_->stopReading();
	}
	// reset ref count to this shared ptr.
    }
}

// Kick start a write request that will block in a new thread until
// send queue is not empty.
void
Session::startWrite(){
    if (sendQ_->pop(&outgoingFrame_)){
	char buf[10];
	sprintf(buf, "%d", outgoingFrame_->serializedData[0]);
	std::cout << "OUT FIRST CHAR IS " << buf << std::endl;
	// data available to send
	boost::asio::async_write(*socket_,
				 boost::asio::buffer(outgoingFrame_->serializedData, FRAME_LENGTH),
				 boost::bind(&Session::handleWrite, shared_from_this(),
					     boost::asio::placeholders::error));
    }
    else{
	// Likely got a read error, which means the socket might be already closed.
	// If we don't call another write, this object will never be destructed (via a shared ptr)
	// likely because the io_service still holds a reference to socket_.
	// Let it try to send something, and get an error. After thism the session
	// will be destructed.
	boost::asio::async_write(*socket_,
				 boost::asio::buffer("\0", 1),
				 boost::bind(&Session::handleWrite, shared_from_this(),
					     boost::asio::placeholders::error));
    }
}

// This handler is invoked when all data has been written to the socket
void 
Session::handleWrite(const boost::system::error_code& error)
{
    // TODO is there a more efficient way of doing this?
    if (startWriteThread_ != NULL){
	delete startWriteThread_;
    	startWriteThread_ = NULL;
    }
    delete outgoingFrame_;
    outgoingFrame_ = NULL;
    
    if (!error){
	// Have to start writes in a separate thread, since we block on pop.
	// Took some time to figure it out...
	startWriteThread_ = new boost::thread(boost::bind(&Session::startWrite, 
							  shared_from_this()));
    }
    else{
	std::cerr << "Socket write error" << std::endl;
	std::cerr << error.message() << std::endl;
    }
}


/*
 * Server
 */

Server::Server(Connection *connection, boost::asio::io_service *ioService, short port, 
	       ThreadSafeQueue<Request> *receiveQ)
    : connection_(connection),
      acceptor_(*ioService, tcp::endpoint(tcp::v4(), port)),
      ioService_(ioService),
      receiveQ_(receiveQ){
    
    unsigned int sessionId = connection_->generateSessionId();
    boost::shared_ptr<Session> newSession = Session::create(sessionId, connection_,
							    ioService_, receiveQ);
    acceptor_.async_accept(*(newSession->socket()),
			   boost::bind(&Server::handleAccept, this, newSession,
				       boost::asio::placeholders::error));
}

void
Server::handleAccept(boost::shared_ptr<Session> newSession, const boost::system::error_code& error){
    TRACE("Connection.cpp", "Accepting a connection");
    if (!error){
	newSession->setSendQ(connection_->addSession(newSession->sessionId(),
						     newSession.get()));
	newSession->start();
	unsigned int sessionId = connection_->generateSessionId();
	newSession = Session::create(sessionId, connection_, ioService_, receiveQ_);
	acceptor_.async_accept(*(newSession->socket()),
			       boost::bind(&Server::handleAccept, this, newSession,
					   boost::asio::placeholders::error));
    }
    else{
	std::cerr << "Failed to accept a connection" << std::endl;
    }
}

/*
 * Connection
 */

Connection::Connection(){
    ioService_ = new boost::asio::io_service();
    sessionIdCount_ = 0;
}

Connection::~Connection(){
    // TODO finish the server thread 
    delete ioService_;
    delete server_;
}

int
Connection::startServer(const std::string &port, ThreadSafeQueue<Request> *receiveQ){
    TRACE("Connection.cpp", "Starting server");
    int portNum = std::atoi(port.c_str());
    if (portNum == 0){
	return CONNECTION_ERR;
    }
    receiveQ_ = receiveQ;
    server_ = new Server(this, ioService_, (short)portNum, receiveQ);
    // the call to run() is blocking until all the socket activities are done.
    // in our case, it will block for the duration of the program, because we
    // want to keep accepting more connections.
    // so, run it in a thread, so that we can return from here
    serverThread_ = new boost::thread(boost::bind(&boost::asio::io_service::run, ioService_));
    return CONNECTION_OK;
}

int
Connection::connect(const std::string &ip, const std::string &port,
		    unsigned int *sessionId, ThreadSafeQueue<Frame *> *sendQ){
    TRACE("Connection.cpp", "Connecting to socket.");
    int retVal = CONNECTION_OK;

    tcp::resolver res(*ioService_);
    tcp::resolver::query q(tcp::v4(), ip, port);
    tcp::resolver::iterator iter = res.resolve(q);

    tcp::socket *newSocket = new tcp::socket(*ioService_);
    
    try{
	newSocket->connect(*iter);
    }
    catch (boost::system::system_error e){	
	if (e.code() == boost::system::errc::connection_refused){
	    TRACE("Connection.cpp", "Connection refused");
	    retVal = CONNECTION_ERR_REFUSED;
	}
	else{
	    TRACE("Connection.cpp", "Connection error");
	    retVal = CONNECTION_ERR;
	}
	// TODO check how to safely close the socket
	delete newSocket;
	return retVal;
    }
    *sessionId = generateSessionId();
    boost::shared_ptr<Session> newSession = Session::create(*sessionId, this, 
							    newSocket, receiveQ_, sendQ);
    sessionIdAndSessionMap_.insert(std::make_pair(*sessionId, newSession.get()));
    newSession->start();
    return retVal;
}

unsigned int
Connection::generateSessionId(){
    boost::mutex::scoped_lock lock(sessionIdGenMutex_);
    return(sessionIdCount_++);
}

ThreadSafeQueue<Frame *> *
Connection::addSession(unsigned int sessionId, Session *session){
    // create a new send queue 
    boost::mutex::scoped_lock lock(sessionMapMutex_);
    TRACE("Connection.cpp", "Adding a new session into the map.");
    ThreadSafeQueue<Frame *> *newQ = new ThreadSafeQueue<Frame *>;
    sessionIdAndSendQueueMap_.insert(std::make_pair(sessionId, newQ));
    // add the session into the second map
    sessionIdAndSessionMap_.insert(std::make_pair(sessionId, session));
    return newQ;
}

ThreadSafeQueue<Frame *> *
Connection::getReplyQueue(unsigned int requestId){
    // better be safe than sorry
    boost::mutex::scoped_lock lock(sessionMapMutex_);
    TRACE("Connection.cpp", "Looking up the reply queue");
    std::map<unsigned int, ThreadSafeQueue<Frame *> *>::iterator iter;
    iter = sessionIdAndSendQueueMap_.find(requestId);
    if (iter == sessionIdAndSendQueueMap_.end()){
	TRACE("", "no queue found :(");
	return NULL;
    }
    return iter->second;
}

bool 
Connection::isConnected(unsigned int sessionId){
    boost::mutex::scoped_lock lock(sessionMapMutex_);
    TRACE("Connection.cpp", "Looking up if connection is still good");
    std::map<unsigned int, Session *>::iterator iter;
    iter = sessionIdAndSessionMap_.find(sessionId);
    if (iter == sessionIdAndSessionMap_.end()){
	return false;
    }
    return true;
}


void
Connection::removeSession(unsigned int sessionId){
    boost::mutex::scoped_lock lock(sessionMapMutex_);
    TRACE("Connection.cpp", "Removing a session");
    ThreadSafeQueue<Frame *> *replyQ;
    std::map<unsigned int, ThreadSafeQueue<Frame *> *>::iterator iter;
    iter = sessionIdAndSendQueueMap_.find(sessionId);
    if (iter != sessionIdAndSendQueueMap_.end()){
	replyQ = iter->second;
	replyQ->stopReading();
	Frame *f;
	while(replyQ->pop(&f)){
	    delete f;
	}
	delete replyQ;
	sessionIdAndSendQueueMap_.erase(iter);
    }
    // and remove it from the other map
    std::map<unsigned int, Session * >::iterator iter2;
    iter2 = sessionIdAndSessionMap_.find(sessionId);
    if (iter2 != sessionIdAndSessionMap_.end()){
	sessionIdAndSessionMap_.erase(iter2);
    }
}

void
Connection::endSession(unsigned int sessionId){
    TRACE("Connection.cpp", "Closing Session");
    boost::mutex::scoped_lock lock(sessionMapMutex_);
    std::map<unsigned int, Session *>::iterator iter;
    iter = sessionIdAndSessionMap_.find(sessionId);
    if (iter != sessionIdAndSessionMap_.end()){
	iter->second->socket()->shutdown(tcp::socket::shutdown_both);
	iter->second->socket()->close();
    }
}


// I like tangerine

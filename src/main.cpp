/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Entry point of the application.
*/


#define BOOST_THREAD_USE_LIB

#ifdef PLATFORM_WIN
#define sleep(x) Sleep(x) // windows uses a capital S...
#endif


#include <iostream>
#include <boost/thread.hpp>
#include "debug.h"
#include "Cli.h"
//#include "Client.h"
//#include "Server.h"
#include "peer.h"

#include "ThreadSafeQueue.h"
#include "Connection.h"
#include "FileChunkIO.h"
#include "vector"
#include <limits>
#include <vector>

#include <stdio.h>

// CLI Example begin
class Temp : public CliCmdHandler{
public:
    Temp(Cli *cli) : CliCmdHandler(cli) {}
    void handleCmd(std::vector<std::string> *cmd);
};

void
Temp::handleCmd(std::vector<std::string> *cmd){
    std::cout << "handleCmd()" << (*cmd)[0] << std::endl;
}
// CLI Example end

// Connection example begin
int numConsumerCalls = 0;
void
consumer(ThreadSafeQueue<Request> *tq, int num, Connection *c){
    std::cout << "CONSUMER" << num << " begin" << std::endl;
    Request req;
    while(tq->pop(&req)){
	std::cout << "CONSUMER GOT FRAME - " << ('A'+numConsumerCalls) << " ";
	// Make sure the frame is correct
	bool err = false;
	int errCount = 0;
	for(int i = 0; i < FRAME_LENGTH - 1; i++){
	    if (req.frame->serializedData[i] != ('A' + numConsumerCalls)){
		err = true;
		errCount++;
	    }
	}
	if (err){
	    std::cout << "NO ERROR" << std::endl;
	}
	else{
	    std::cout << errCount << " ERRORS!!!" << std::endl;
	}
	std::cout << "IP: " << req.ip << "    port: " << req.port << std::endl;
	delete req.frame;
	numConsumerCalls++;
    }
    std::cout << "CONSUMER" << num << " end" <<std::endl;
}

int numProducerCalls = 0;
void
producer(ThreadSafeQueue<Frame *> *tq, int num){
    sleep(3);
    std::cout << "PRODUCER" << num << " begin" << std::endl;
    for (int i = 0 ; i < 20000000; i++){
	std::cout << "Producer: outputing " << ('A'+numProducerCalls) << std::endl;
	char buff[20];
	Frame *newFrame = new Frame();
	sprintf(buff, "%d", i+(num*20));
	for(int i = 0; i < FRAME_LENGTH - 1; i++){
	    newFrame->serializedData[i] = ('A'+numProducerCalls);
	}
	newFrame->serializedData[FRAME_LENGTH - 1] = 0;
	numProducerCalls++;
	tq->push(newFrame);
	sleep(1);
    }
    tq->stopReading();
    std::cout << "PRODUCER" << num << " end" << std::endl;
}
// Connection example end

int 
main(int argc, char* argv[]){
    TRACE("main.cpp", "Starting up.");

    // Initialize Cli and run it in a separate thread.
    TRACE("main.cpp", "Initializing CLI");
    Cli *cli = new Cli();
    boost::thread cliThread(boost::bind(&Cli::run, cli));    

    // Create the peers container and make it initialize all peers
    TRACE("main.cpp", "Initializing Peers");
    Peers *peers = new Peers(cli, "peers.txt");
    boost::thread peersThread(boost::bind(&Peers::initialize, peers));

    

    // Cli::run() returns when the user types "quit" in terminal
    // TODO make sure the client gracefully shutsdown...
    cliThread.join();
    
    return 0;
}

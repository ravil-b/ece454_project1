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

void
consumer(ThreadSafeQueue<Frame *> *tq, int num){
    std::cout << "CONSUMER" << num << " begin" << std::endl;
    Frame *res;
    while(tq->pop(&res)){
	std::cout << "consumer" << num << " << " << res->serializedData << std::endl;
	delete res;
    }
    std::cout << "CONSUMER" << num << " end" <<std::endl;
}

void
producer(ThreadSafeQueue<Frame *> *tq, int num){
    sleep(3);
    std::cout << "PRODUCER" << num << " begin" << std::endl;
    for (int i = 0 ; i < 20000000; i++){
	std::cout << "producer" << num << " << " << i + (num * 20) << std::endl;
	char buff[20];
	Frame *newFrame = new Frame();
	sprintf(buff, "%d", i+(num*20));
	strcpy(newFrame->serializedData, buff);
	tq->push(newFrame);
	sleep(1);
    }
    tq->stopReading();
    std::cout << "PRODUCER" << num << " end" << std::endl;
}


int 
main(int argc, char* argv[]){
    TRACE("main.cpp", "Starting up.");
    TRACE("main.cpp", "Initializing CLI");

    // Initialize Cli and run it in a separate thread.
    Cli *cli = new Cli();
    boost::thread cliThread(boost::bind(&Cli::run, cli));


    // FileChunkIO * fileChunkIO = new FileChunkIO();

    // char chunkData[chunkSize];

    // char charMaxVal = std::numeric_limits<char>::max();

    // for (int i = 0; i < chunkSize; i ++)
    // {
    //     chunkData[i] = ((char)i % charMaxVal);
    // }


    // string writeChunkString (chunkData, chunkSize);
    // cout << "Writing chunk string: " << writeChunkString << endl;
    // fileChunkIO->writeChunk("C:/test1", 0, chunkData);


    // char readChunkData [chunkSize];
    // fileChunkIO->readChunk("C:/test1", 0, readChunkData);

    // cout << "Read chunk string: " << endl;
    // for (int i = 0; i < chunkSize; i ++)
    // {
    //     char asciiNumber[4];
    //     itoa((int)readChunkData[i], asciiNumber, 10);

    //     cout << asciiNumber << endl;
    // }



//    Peers *peers = new Peers(cli, "peers.txt");
//    boost::thread peersInitThread(boost::bind(&Peers::initialize, peers));
//
//    ThreadSafeQueue<int> *tq = new ThreadSafeQueue<int>();
//    boost::thread consumerThread1(boost::bind(&consumer, tq, 1));
//    boost::thread consumerThread2(boost::bind(&consumer, tq, 2));
//    boost::thread consumerThread3(boost::bind(&consumer, tq, 3));
//    boost::thread producerThread1(boost::bind(&producer, tq, 1));
//    boost::thread producerThread2(boost::bind(&producer, tq, 2));



    //Peers *peers = new Peers(cli, "peers.txt");
    //boost::thread peersInitThread(boost::bind(&Peers::initialize, peers));
    
    ThreadSafeQueue<Frame *> *rq = new ThreadSafeQueue<Frame *>();
    boost::thread consumerThread1(boost::bind(&consumer, rq, 1));
    ThreadSafeQueue<Frame *> *sq = new ThreadSafeQueue<Frame *>();
    boost::thread producerThread1(boost::bind(&producer, sq, 1));
    //boost::thread producerThread1(boost::bind(&producer, tq, 1));
    Connection *c = new Connection();
    std::cout << "Starting server.. " << std::endl;
    c->startServer("4567", rq);
    std::cout << "Starting server returned  " << std::endl;
    
    c->connect("localhost", "4568", 2, sq);

    sleep(6);
    ThreadSafeQueue<Frame *> *sq2 = new ThreadSafeQueue<Frame *>();
    boost::thread producerThread2(boost::bind(&producer, sq2, 2));
    c->connect("localhost", "4569", 2, sq2);

    // CLI Handler Example begin
    // Temp *temp = new Temp(cli);
    // Command cmd;
    // cmd.cmdStr = "test_command";
    // cmd.numArgs = 1;
    // cmd.handler = temp;
    // cli->registerCommand(cmd);
    //delete temp;
    // CLI Example end

    // // Client test begin
    // Client *cl = new Client();
    // cl->connect("localhost", "4455");
    // TRACE("main.cpp", "Sending Data:");
    // cl->send("This is a test of the connection");
    // TRACE("main.cpp", "Closing Connection");
    // cl->closeConnection();
    // // Client test end

    // Server test begin
    // Server * s = new Server();
    // RawRequest request;
    // s->waitForRequest(&request, 4456);
    // TRACE("main.cpp", "Request Data:");
    // TRACE("main.cpp", request.data);
    // Server test end


    // Cli::run() returns when the user types "quit" in terminal
    // TODO make sure the client gracefully shutsdown...
    cliThread.join();
    
    return 0;
}

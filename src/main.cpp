/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Entry point of the application.
*/


#define BOOST_THREAD_USE_LIB


#include <iostream>
#include <boost/thread.hpp>
#include "debug.h"
#include "Cli.h"
#include "Client.h"
#include "Server.h"
#include "peer.h"
#include "ThreadSafeQueue.cpp"

#include "vector"



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
consumer(ThreadSafeQueue<int> *tq, int num){
    std::cout << "CONSUMER" << num << " begin" << std::endl;
    int res;
    while(tq->pop(&res)){
	std::cout << "consumer" << num << " << " << res << std::endl;
    }
    std::cout << "CONSUMER" << num << " end" <<std::endl;
}

void
producer(ThreadSafeQueue<int> *tq, int num){
    sleep(3);
    std::cout << "PRODUCER" << num << " begin" << std::endl;
    for (int i = 0 ; i < 20; i++){
	std::cout << "producer" << num << " << " << i + (num * 20) << std::endl;
	tq->push(i);
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

    Peers *peers = new Peers(cli, "peers.txt");
    boost::thread peersInitThread(boost::bind(&Peers::initialize, peers));
    
    ThreadSafeQueue<int> *tq = new ThreadSafeQueue<int>();
    boost::thread consumerThread1(boost::bind(&consumer, tq, 1));
    boost::thread consumerThread2(boost::bind(&consumer, tq, 2));
    boost::thread consumerThread3(boost::bind(&consumer, tq, 3));
    boost::thread producerThread1(boost::bind(&producer, tq, 1));
    boost::thread producerThread2(boost::bind(&producer, tq, 2));

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

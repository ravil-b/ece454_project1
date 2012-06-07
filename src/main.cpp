/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Entry point of the application.
*/

#include <iostream>
#include <boost/thread.hpp>
#include "debug.h"
#include "Cli.h"
#include "Client.h"
#include "Server.h"
#include "peer.h"

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

void
threadTest(){
    std::cout << "threadTest()" << std::endl;
}

// CLI Example end

int 
main(int argc, char* argv[]){
    TRACE("main.cpp", "Starting up.");
    TRACE("main.cpp", "Initializing CLI");

    // Initialize Cli and run it in a separate thread.
    Cli *cli = new Cli();
    boost::thread cliThread(boost::bind(&Cli::run, cli));

    Peers *peers = new Peers(cli);
    peers->initialize("peers.txt");

    // CLI Handler Example begin
    Temp *temp = new Temp(cli);
    Command cmd;
    cmd.cmdStr = "test_command";
    cmd.numArgs = 1;
    cmd.handler = temp;
    cli->registerCommand(cmd);
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

    // // Server test begin
    // Server * s = new Server();
    // RawRequest request;
    // s->waitForRequest(&request, 4455);
    // TRACE("main.cpp", "Request Data:");
    // TRACE("main.cpp", request.data);
    // // Server test end


    // Cli::run() returns when the user types "quit" in terminal
    // TODO make sure the client gracefully shutsdown...
    cliThread.join();
    
    return 0;
}

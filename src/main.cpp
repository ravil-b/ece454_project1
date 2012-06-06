/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Entry point of the application.
*/

#include <iostream>
//#include <boost/thread.hpp>
#include "debug.h"
#include "Cli.h"
#include "CliCmdHandler.h"
#include "Client.h"
#include "Server.h"

#include "vector"


// CLI Example begin
class Temp : public CliCmdHandler{
public:
    void handleCmd(std::vector<std::string> *cmd);
};

void
Temp::handleCmd(std::vector<std::string> *cmd){
    std::cout << "handleCmd()" << (*cmd)[0] << std::endl;
}
// CLI Example end

int main(int argc, char* argv[]){
    TRACE("main.cpp", "Starting up.");
    TRACE("main.cpp", "Initializing CLI");

    Cli *cli = new Cli();

    // CLI Example begin
    Temp temp;
    command cmd;
    cmd.cmdStr = "test_command";
    cmd.numArgs = 1;
    cmd.handler = &temp;
    cli->registerCommand(cmd);
    // CLI Example end

    // Server test begin
    Server * s = new Server();
    RawRequest request;
    s->waitForRequest(&request, 4455);
    TRACE("main.cpp", "Request Data:");
    TRACE("main.cpp", request.data);
    // Server test end

    // Client test begin
    Client *cl = new Client();
    cl->connect("localhost", "4454");
    cl->send("This is a test of the connection");
    cl->closeConnection();
    // Client test end



    cli->run();
    return 0;
}

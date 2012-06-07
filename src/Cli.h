/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Command Line Interpreter
Provides a simple interface for the user.
*/

#ifndef CLI_H
#define CLI_H

#include <vector>
#include <string>

class CliCmdHandler;

struct Command {
    std::string cmdStr;
    std::string usage;
    int numArgs;
    CliCmdHandler *handler;
};

class Cli {

 public:
    Cli();

    void run();
    bool registerCommand(Command &cmd);
    void removeHandler(CliCmdHandler *handler);
    void terminate();

 private:
 
    std::vector<Command> commands_;
    bool terminate_;

    Command *findMatchingCommand(std::string &cmdStr);
    
};

/*
Interface for classes that want to handle Cli commands.
*/

class CliCmdHandler {
    
 public:
    inline CliCmdHandler(Cli *cli){
	cli_ = cli;
    }
    virtual ~CliCmdHandler(){
	// Unregister command that this handler signed up for.
	// Will prevent possible seg faults.
	cli_->removeHandler(this);
    }
    virtual void handleCmd(std::vector<std::string> *cmd) = 0;
 
 protected:
    Cli *cli_;

 private:
    // Force non-default constructor to get cli
    CliCmdHandler();

};



#endif


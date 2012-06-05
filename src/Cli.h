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
#include "CliCmdHandler.h"

struct command {
    std::string cmdStr;
    std::string usage;
    int numArgs;
    CliCmdHandler *handler;
};

class Cli {

 public:
 
    int run();
    bool registerCommand(command &cmd);

 private:
 
    std::vector<command> commands_;

    command *findMatchingCommand(std::string &cmdStr);

};





#endif


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

struct command {
    char cmdStr[20];
    unsigned char numParams;
    void *callback;
};

class Cli {

 public:
 
    int run();
    bool registerCommand(command cmd);

 private:
 
    std::vector<command> commands_;

    command *findMatchingCommand(char *cmdStr);

};

#endif


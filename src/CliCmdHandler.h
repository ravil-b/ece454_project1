/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Interface for classes that want to handle Cli commands.
*/

#ifndef CLICMDHANDLER_H
#define CLICMDHANDLER_H

#include <vector>
#include <string>

class CliCmdHandler {
    
 public:
    virtual ~CliCmdHandler(){}
    virtual void handleCmd(std::vector<std::string> *cmd) = 0;

};

#endif

/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Command Line Interpreter
Provides a simple interface for the user.
*/

#include "Cli.h"
#include "debug.h"

#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>


using namespace std;
using namespace boost;

Cli::Cli(){
    terminate_ = false;
}

void 
Cli::run(){
    TRACE("Cli.cpp", "run()");
    while (true){
	string inputLine;
	getline(cin, inputLine);
	// Split into tokens (use space as sep)
	vector<string> splitInput;	
	split(splitInput, inputLine, is_any_of(" "));
	// Remove empty tokens
	vector<string>::iterator newEnd;
	newEnd = remove(splitInput.begin(), splitInput.end(), "");
	splitInput.erase(newEnd, splitInput.end());
	
	if (splitInput.size() == 0){
	    continue;
	}
	
	// Quit the program
	if ((strcmp(splitInput[0].c_str(), "quit") == 0) && splitInput.size() == 1){
	    break;
	}

	Command *matchedCmd = findMatchingCommand(splitInput[0]);
	if (matchedCmd == NULL){
	    cout << "Unknown command" << endl;
	    continue;
	}
	if (unsigned(matchedCmd->numArgs + 1) != splitInput.size()){
	    cout << "Incorrect number of arguments" << endl;
	    cout << matchedCmd->usage << endl;
	    continue;
	}

	matchedCmd->handler->handleCmd(&splitInput);

	if (terminate_){
	    break;
	}
    }
}

bool
Cli::registerCommand(Command &cmd){
    TRACE("Cli.cpp", "registerCommand()");
    
    if (findMatchingCommand(cmd.cmdStr) == NULL){
        commands_.push_back(cmd);
        return true;
    }
    return false;
}

void
Cli::terminate(){
    TRACE("Cli.cpp", "terminate()");
    terminate_ = true;
}

void
Cli::removeHandler(CliCmdHandler *handler){
    TRACE("Cli.cpp", "removeHandler()");

    vector<Command>::iterator iter = commands_.begin();
    while (iter != commands_.end()){
	if ((*iter).handler == handler){
	    commands_.erase(iter);
	}
	else{
	    iter++;
	}
    }
}

Command *
Cli::findMatchingCommand(string &cmdStr){
    TRACE("Cli.cpp", "findMatchingCommand()");

    vector<Command>::iterator iter;
    for (iter = commands_.begin(); iter != commands_.end(); iter++){
	if (strcmp(cmdStr.c_str(), (*iter).cmdStr.c_str()) == 0){
	    return &(*iter);
	}
    }
    return NULL;
}


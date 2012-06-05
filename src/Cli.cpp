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

int 
Cli::run(){
    TRACE("cli.cpp", "run()");
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
	
	command *matchedCmd = findMatchingCommand(splitInput[0]);
	if (matchedCmd == NULL){
	    cout << "not matched" << endl;
	    continue;
	}
	if (unsigned(matchedCmd->numArgs + 1) != splitInput.size()){
	    cout << "Incorrect number of arguments" << endl;
	    cout << matchedCmd->usage << endl;
	    continue;
	}

	matchedCmd->handler->handleCmd(&splitInput);
    }
    return 0;
}

bool
Cli::registerCommand(command &cmd){
    TRACE("cli.cpp", "registerCommand()");
    
    if (findMatchingCommand(cmd.cmdStr) == NULL){
	commands_.push_back(cmd);
	return true;
    }
    return false;
}

command *
Cli::findMatchingCommand(string &cmdStr){
    TRACE("cli.cpp", "findMatchingCommand()");

    vector<command>::iterator iter;
    for (iter = commands_.begin(); iter != commands_.end(); iter++){
	if (strcmp(cmdStr.c_str(), (*iter).cmdStr.c_str()) == 0){
	    return &(*iter);
	}
    }
    return NULL;
}

#include "cli.h"
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
    command newCmd;
    strcpy(newCmd.cmdStr, "test");
    while(true){
	string inputLine;
	getline(cin, inputLine);
	vector<string> splitInput;
	split(splitInput, inputLine, is_any_of(" "));
	cout << splitInput[0] << endl;
    }
    return 0;
}

bool
Cli::registerCommand(command cmd){
    TRACE("cli.cpp", "registerCommand()");
    
    if (findMatchingCommand(cmd.cmdStr) == NULL){
	commands_.push_back(cmd);
	return true;
    }
    return false;
}

command *
Cli::findMatchingCommand(char *cmdStr){
    TRACE("cli.cpp", "findMatchingCommand()");

    vector<command>::iterator iter;
    for (iter = commands_.begin(); iter != commands_.end(); iter++){
	if (strcmp(const_cast<char *>(cmdStr), const_cast<char *>((*iter).cmdStr)) == 0){
	    return &(*iter);
	}
    }
    return NULL;
}

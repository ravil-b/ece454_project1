/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Debug output
*/

#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG
#define TRACE(msg)
#else
#define TRACE(msg) \
    std::cout << msg << std::endl;
#endif

#include <iostream>

#endif


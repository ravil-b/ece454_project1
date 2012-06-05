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
#define TRACE(file, msg)
#else
#define TRACE(file, msg)				\
    std::cout << file << " >> " << msg << std::endl;
#endif

#include <iostream>

#endif


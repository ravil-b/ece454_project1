/**
 *  @brief: Local include file for socket programs
 *  @file:  local_sock.h
 */

#ifndef LOCAL_SOCK_H
#define LOCAL_SOCK_H
//#define _GNU_SOURCE
#include <iostream>
#include <sys/ioctl.h>
#include <cstdio>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
//const  int  PORT=48920; // 4890 and up for local host testing 
const  int  PORT=10090;   // 10000 - 10100 is open ecelinux 
static char buf[BUFSIZ];  // Buffer for messages
#endif
using namespace std;


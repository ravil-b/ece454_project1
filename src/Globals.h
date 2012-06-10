/*
 * globals.h
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

const int chunkSize = 65536;
const int maxPeers  = 6;
const int maxFiles  = 100;    // Cheesy, but allows us to do a simple Status class

const int maxFileSize = 2 * 1024 * 1024 * 1024;
const int maxChunksPerFile = maxFileSize / chunkSize;

#endif /* GLOBALS_H_ */

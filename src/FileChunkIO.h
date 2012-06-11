/*
 * FileChunkIO.h
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#ifndef FILECHUNKIO_H_
#define FILECHUNKIO_H_

#include <string>
#include <vector>

class FileChunkIO
{
public:
    int readChunk(std::string fileName, int chunkNum, char * buffer);
    int writeChunk(std::string fileName, int chunkNum, char * chunkData, int fileSize);
    void createFileIfNotExists(std::string fileName, int fileSize);
};

const int errFileChunkIOOK = 0;
const int errFileChunkRead = -1;
const int errFileChunkWrite = -2;




#endif /* FILECHUNKIO_H_ */

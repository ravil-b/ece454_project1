/*
 * FileChunkIO.cpp
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#include "FileChunkIO.h"
#include "Globals.h"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/seek.hpp>

#include <iostream>


using namespace std;
using namespace boost;
using boost::iostreams::file_source;
using boost::iostreams::file_sink;

int writers = 0;

// Reads file chunk and writes it to @buffer
int
FileChunkIO::readChunk(std::string fileName, int chunkNum, char * buffer)
{
    while (writers > 0); // block

    filesystem::path p(fileName);
    if (!exists(p)){
        cout << fileName << " appears to be missing!" << endl;
        return errFileChunkRead;
    }
    
    try
    {
        file_source localFile(fileName, BOOST_IOS::binary);
        boost::iostreams::seek(localFile, chunkNum * chunkSize, BOOST_IOS::beg);
        //streamsize result = 
	boost::iostreams::read(localFile, buffer, chunkSize);
    }
    catch (std::exception& e)
    {
        cout << "Problem reading file: " << fileName << ". " << e.what() << endl;
        return errFileChunkRead;
    }

    return errFileChunkIOOK;
}

int
FileChunkIO::writeChunk(std::string fileName, int chunkNum, char * chunkData, int fileSize)
{
    while (writers > 0); // block

    filesystem::path p(fileName);

    try
    {
	createFileIfNotExists(fileName, fileSize);
        file_sink localFile(fileName, std::ios_base::out);
        boost::iostreams::seek(localFile, chunkNum * chunkSize, BOOST_IOS::beg);
        //streamsize result = 
	boost::iostreams::write(localFile, &chunkData[0], chunkSize);
    }
    catch (std::exception& e)
    {
        cout << "Problem writing file: " << fileName << ". " << e.what() << endl;
        return errFileChunkRead;
    }
    return errFileChunkIOOK;
}

void 
FileChunkIO::createFileIfNotExists(std::string fileName, int fileSize){
    filesystem::path p(fileName);
    // if the file doesn't exist, create it with the max size;
    if (!exists(p)){
	file_sink localFile(fileName, std::ios_base::out);
	boost::iostreams::seek(localFile, fileSize - 1, BOOST_IOS::beg);
	char data[1];
	data[0] = 0;
	boost::iostreams::write(localFile, data, 1);
    }
}

/*
 * Frames.cpp
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#include "Frames.h"
#include <vector>
#include <map>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int
parseIntFromCharArray(const char * array, int startIndex)
{
    int toReturn = 0;
    for (int i = startIndex; i < 4; i++)
    {
        toReturn = (toReturn << 8) + array[i];
    }
    return toReturn;
}

short
parseShortFromCharArray(const char * array)
{
    short toReturn = 0;
    for (int i = 0; i < 2; i++)
    {
        toReturn = (toReturn << 8) + array[i];
    }
    return toReturn;
}

void copyIntToCharArray(char * array, int toCopy)
{
    array[0] = toCopy & 0xff;
    array[1] = (toCopy>>8)  & 0xff;
    array[2] = (toCopy>>16) & 0xff;
    array[3] = (toCopy>>24) & 0xff;
}

void copyShortToCharArray(char * array, short toCopy)
{
    array[0] = toCopy & 0xff;
    array[1] = (toCopy>>8)  & 0xff;
}

RemoteFileInfo
parseRemoteFileInfo(char * array, int startIndex)
{
    RemoteFileInfo f;

    f.fileNum = array[startIndex];
    f.chunkCount = parseIntFromCharArray(array, startIndex + 1);
    f.totalChunksDownloaded = parseIntFromCharArray(array, startIndex + TOTAL_CHUNKS_AT_SOURCE_OFFSET);

    std::string fileName (array + 2, MAX_FILE_NAME_LENGTH);
    f.fileName = fileName;

    return f;
}

char
Frame::getFrameType()
{
    return serializedData[0];
}

PeerInfoFrame::PeerInfoFrame(std::string ip, std::string port)
{
    strcpy(serializedData + sizeof(char), ip.c_str());
    copyShortToCharArray(serializedData + 16, (short)atoi(port.c_str()));
}

std::string
PeerInfoFrame::getIp()
{
    return std::string(serializedData + 1, 15);

}

std::string
PeerInfoFrame::getPort()
{
    char portStr[10];
    sprintf(portStr, "%d", parseShortFromCharArray(serializedData + 16));

    return std::string(portStr, 5);
}


FileNumFrame::FileNumFrame(char fileNum)
{
    serializedData[1] = fileNum;
}

char
FileNumFrame::getFileNum()
{
    return serializedData[1];
}

HandshakeRequestFrame::HandshakeRequestFrame()
{
    serializedData[0] = (char)FrameType::HANDSHAKE_REQUEST;
}

HandshakeResponseFrame::HandshakeResponseFrame()
{
    serializedData[0] = (char)FrameType::HANDSHAKE_RESPONSE;
}


ChunkFrame::ChunkFrame(char fileNum, int chunkNum): FileNumFrame(fileNum)
{
    copyIntToCharArray(serializedData + 2, chunkNum);
}

int
ChunkFrame::getChunkNum()
{
    return parseIntFromCharArray(serializedData, 2);
}

ChunkDataFrame::ChunkDataFrame(char fileNum, int chunkNum, char * chunk): ChunkFrame(fileNum, chunkNum)
{
    serializedData[0] = (char)FrameType::CHUNK;

    int i = sizeof(char) + sizeof(char) + sizeof(int);
    for (; i < chunkSize; i++)
    {
        serializedData[i] = chunk[i];
    }
}

ChunkRequestFrame::ChunkRequestFrame(char fileNum, int chunkNum) : ChunkFrame(fileNum, chunkNum)
{
    serializedData[0] = (char)FrameType::CHUNK_REQUEST;
}

ChunkRequestDeclineFrame::ChunkRequestDeclineFrame(char fileNum, int chunkNum) : ChunkFrame(fileNum, chunkNum)
{
    serializedData[0] = (char)FrameType::CHUNK_REQUEST_DECLINE;
}

ChunkCountFrame::ChunkCountFrame(char fileNum, int chunkCount): FileNumFrame(fileNum)
{
    serializedData[0] = (char)FrameType::CHUNK_COUNT;
}

ChunkCountRequestFrame::ChunkCountRequestFrame(char fileNum, int chunkCount): FileNumFrame(fileNum)
{
    serializedData[0] = (char)FrameType::CHUNK_COUNT_REQUEST;
}

char *
ChunkDataFrame::getChunk()
{
    return serializedData + (sizeof(char) + sizeof(char) + sizeof(int));
}

char
FileListFrame::getFileCount()
{
    return serializedData[1];
}

void
FileListFrame::parseFileInfos()
{
    std::vector<RemoteFileInfo> * fileInfos = new std::vector<RemoteFileInfo>();
    for (int i = 2; i < FRAME_LENGTH - (int)3*sizeof(char); i += FILE_INFO_DATA_WIDTH)
    {
        RemoteFileInfo f = parseRemoteFileInfo(serializedData, i);
        fileInfos->push_back(f);
    }
    this->fileInfos_ = fileInfos;
}

std::vector<RemoteFileInfo>
FileListFrame::getFileInfos()
{
    if (this->fileInfos_ == NULL)
        parseFileInfos();

    return *this->fileInfos_;
}

FileListFrame::FileListFrame(char fileCount, std::vector<RemoteFileInfo> files)
{
    serializedData[0] = (char)FrameType::FILE_LIST;
    serializedData[1] = (char)files.size();
    for (int i; i < (int)files.size(); i++)
    {
        int serialDataIdx = 2 * sizeof(char) + (i * FILE_INFO_DATA_WIDTH);
        RemoteFileInfo file = files[i];
        serializedData[serialDataIdx] = file.fileNum;
        copyIntToCharArray(serializedData + serialDataIdx + sizeof(char), file.chunkCount);
        copyIntToCharArray(serializedData + serialDataIdx + sizeof(char) + sizeof(int), file.totalChunksDownloaded);
    }
}

FileListRequestFrame::FileListRequestFrame()
{
    serializedData[0] = (char)FrameType::FILE_LIST_REQUEST;
}

FileListDeclineFrame::FileListDeclineFrame()
{
    serializedData[0] = (char)FrameType::FILE_LIST_DECLINE;
}


int
ChunkCountFrame::getChunkCount()
{
    return parseIntFromCharArray(serializedData, 2);
}


FileInfo
NewFileAvailableFrame::getFileInfo()
{
    return parseRemoteFileInfo(serializedData, 1);
}


// Max number of chunks is 2GB / 64kB = 32768

// chunkMap = { filenumber: { chunkNumber: chunkAvailable } }
ChunkInfoFrame::ChunkInfoFrame(char fileCount, std::map<char, std::map<int, bool> > chunkMap)
{
    serializedData[0] = (char)FrameType::CHUNK_INFO;
    serializedData[1] = fileCount;

    for (char fileIdx=0; fileIdx < fileCount; fileIdx++)
    {

        for (int chunkNum = 0; chunkNum < maxChunksPerFile; chunkNum+=8)
        {
            unsigned char b = 0;
            b |= (chunkMap[fileIdx][chunkNum] & 1) << 8;
            b |= (chunkMap[fileIdx][chunkNum + 1] & 1) << 7;
            b |= (chunkMap[fileIdx][chunkNum + 2] & 1) << 6;
            b |= (chunkMap[fileIdx][chunkNum + 3] & 1) << 5;
            b |= (chunkMap[fileIdx][chunkNum + 4] & 1) << 4;
            b |= (chunkMap[fileIdx][chunkNum + 5] & 1) << 3;
            b |= (chunkMap[fileIdx][chunkNum + 6] & 1) << 2;
            b |= (chunkMap[fileIdx][chunkNum + 7] & 1) << 1;
            b |= (chunkMap[fileIdx][chunkNum + 8] & 1) << 0;

            serializedData[chunkNum / 8] = (char)b;
        }


    }
}

char
ChunkInfoFrame::getFileCount()
{
    return serializedData[1];
}

std::map<char, std::map<int, bool> >
ChunkInfoFrame::getChunkMap()
{
    std::map<char, std::map<int, bool> > chunkMap;

    for (char fileIdx; fileIdx < getFileCount(); fileIdx++)
    {
        std::map<int, bool> fileMap;

        for (int chunkNum = 0; chunkNum < maxChunksPerFile; chunkNum+=8)
        {
            unsigned char b = serializedData[chunkNum / 8];

            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 8) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 7) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 6) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 5) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 4) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 3) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 2) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 1) & 1));
            fileMap.insert(std::make_pair(chunkNum, (bool)(b >> 0) & 1));
        }

        chunkMap.insert(std::make_pair(fileIdx, fileMap));
    }
    return chunkMap;
}



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


char
Frame::getFrameType()
{
    return serializedData[0];
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

ChunkFrame::ChunkFrame(char fileNum, int chunkNum): FileNumFrame(fileNum)
{
    serialization_helpers::copyIntToCharArray(serializedData + 2, chunkNum);
}

int
ChunkFrame::getChunkNum()
{
    return serialization_helpers::parseIntFromCharArray(serializedData, 2);
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

}

std::vector<FileInfo>
FileListFrame::getFileInfos()
{
    if (this->fileInfos_ == NULL)
        parseFileInfos();

    return *this->fileInfos_;
}

FileListFrame::FileListFrame(char fileCount, std::vector<FileInfo> files)
{
    serializedData[0] = (char)FrameType::FILE_LIST;
    serializedData[1] = (char)files.size();
    for (int i = 0; i < (int)files.size(); i++)
    {
        int serialDataIdx = 2 * sizeof(char) + (i * FILE_INFO_DATA_WIDTH);
        FileInfo file = files[i];
        serializedData[serialDataIdx] = file.fileNum;
        serialization_helpers::copyIntToCharArray(serializedData + serialDataIdx + sizeof(char), file.chunkCount);
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


FileInfo
NewFileAvailableFrame::getFileInfo()
{
    return serialization_helpers::parseFileInfo(serializedData, 1);
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



ChunkInfoRequestFrame::ChunkInfoRequestFrame()
{

}

namespace frame_function{
    void setIp(std::string ip, char *serializedData){
        strcpy(serializedData + sizeof(char), ip.c_str());
    }
    //copyShortToCharArray(serializedData + 16, (short)atoi(port.c_str()));
    std::string getIp(char *serializedData)
    {
        return std::string(serializedData + 1, 15);
    }

    std::string getPort(char *serializedData)
    {
        char portStr[10];
        sprintf(portStr, "%d", serialization_helpers::parseShortFromCharArray(serializedData + 16));
        return std::string(portStr, 5);
    }

}

namespace serialization_helpers{
    FileInfo
    parseFileInfo(char * array, int startIndex)
    {
        FileInfo f;
        f.fileNum = array[startIndex];
        f.chunkCount = serialization_helpers::parseIntFromCharArray(array, startIndex + 1);
        std::string fileName (array + 5 + startIndex, MAX_FILE_NAME_LENGTH);
        f.fileName = fileName;

        return f;
    }

    int
    parseIntFromCharArray(const char * array, int startIndex)
    {
        int toReturn = 0;
        for (int i = startIndex; i < (4 + startIndex); i++)
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

    void
    copyIntToCharArray(char * array, int toCopy)
    {
        array[3] = toCopy & 0xff;
        array[2] = (toCopy>>8)  & 0xff;
        array[1] = (toCopy>>16) & 0xff;
        array[0] = (toCopy>>24) & 0xff;
    }

    void
    copyShortToCharArray(char * array, short toCopy)
    {
	array[1] = (toCopy>>8)  & 0xff;
        array[0] = toCopy & 0xff;
    }
}

namespace fileListFrame_serialization{

    Frame *
    createFileListFrame(char fileCount, std::vector<FileInfo *> files)
    {
        Frame * frame = new Frame();

        frame->serializedData[0] = (char)FrameType::FILE_LIST;
        frame->serializedData[1] = (char)files.size();
        for (int i = 0; i < (int)files.size(); i++)
        {
            int serialDataIdx = 2 + (i * FILE_INFO_DATA_WIDTH);
            FileInfo * file = files[i];
	    
            frame->serializedData[serialDataIdx] = file->fileNum;	    
            serialization_helpers::copyIntToCharArray(frame->serializedData + serialDataIdx + 1, file->chunkCount);

	    // File name
	    const char *fName = file->fileName.c_str();
	    for (int y = 0; y < 512; y++){
		frame->serializedData[serialDataIdx + y + 5] = fName[y];
		if (fName[y] == 0){
		    break;
		}
	    }
        }

	std::vector<FileInfo> fileInfos = getFileInfos(frame);
	for (int i = 0; i < fileInfos.size(); i++)
	{
	    FileInfo f = fileInfos[i];
	}


	return frame;
    }

    std::vector<FileInfo>
    getFileInfos(Frame * frame)
    {
	char fileCount = frame->serializedData[1];
        std::vector<FileInfo> fileInfos;
	int endIterIdx = (fileCount * FILE_INFO_DATA_WIDTH) + 2;
        for (int i = 2; i < endIterIdx; i += FILE_INFO_DATA_WIDTH)
        {
            FileInfo f = serialization_helpers::parseFileInfo(frame->serializedData, i);
            fileInfos.push_back(f);
        }
        return fileInfos;
    }
}


namespace chunkInfoRequest_serialization
{
    Frame *
    createChunkInfoRequest()
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::CHUNK_INFO_REQUEST;
        return newFrame;
    }
}

namespace fileListRequestFrame_serialization
{
    Frame *
    createFileListRequest()
    {
	Frame * newFrame = new Frame();
	newFrame->serializedData[0] = (char)FrameType::FILE_LIST_REQUEST;
	return newFrame;
    }
}

namespace handshakeRequestFrame_serialization
{
    Frame * createHandshakeRequestFrame()
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::HANDSHAKE_REQUEST;
        return newFrame;
    }
}

namespace handshakeResponseFrame_serialization
{
    Frame * createHandshakeResponseFrame(std::string ip, std::string port)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::HANDSHAKE_RESPONSE;
        strcpy(newFrame->serializedData + sizeof(char), ip.c_str());


        serialization_helpers::copyShortToCharArray(newFrame->serializedData + 16, (short)atoi(port.c_str()));
        return newFrame;
    }

    std::string
    getIp(Frame * frame)
    {
        return std::string(frame->serializedData + 1, 15);
    }


    std::string
    getPort(Frame * frame)
    {
        char portStr[10];

        sprintf(portStr, "%d", serialization_helpers::parseShortFromCharArray(frame->serializedData + 16));
        return std::string(portStr, 5);
    }
}



namespace peerLeavingFrame_serialization{
    Frame *
    createPeerLeavingFrame(std::string ip, std::string port){
	Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::HANDSHAKE_RESPONSE;
        strcpy(newFrame->serializedData + sizeof(char), ip.c_str());


        serialization_helpers::copyShortToCharArray(newFrame->serializedData + 16, (short)atoi(port.c_str()));
        return newFrame;;
    }
}

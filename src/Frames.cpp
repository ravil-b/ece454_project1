/*
 * Frames.cpp
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#include "Frames.h"
#include <vector>

int
parseIntFromCharArray(const char * array, int startIndex)
{
    int toReturn = 0;
    for (int i = startIndex; i < startIndex + (int)sizeof(int); i++)
    {
        toReturn = (toReturn << sizeof(char)) + array[i];
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





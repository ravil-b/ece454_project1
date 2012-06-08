/*
 * Frames.cpp
 *
 *  Created on: 2012-06-08
 *      Author: Robin
 */

#include "Frames.h"
#include <vector>


char
Frame::getFrameType()
{
    return serializedData[0];
}


char
ChunkFrame::getFileNum()
{
    return serializedData[1];
}

int
ChunkFrame::getChunkNum()
{
    return parseIntFromCharArray(serializedData, 2);
}

char *
ChunkDataFrame::getChunk()
{
    return serializedData + 6;
}

char
FileListFrame::getFileCount()
{
    return serializedData[1];
}

std::vector<FileInfo>
FileListFrame::getFileInfos()
{
    if (this->fileInfos_ != 0)
        return this->fileInfos_;

    std::vector<FileInfo> fileInfos = new std::vector<FileInfo>();

    for (int i = 2; i < FRAME_LENGTH - 3; i += (MAX_FILE_NAME_LENGTH + sizeof(char) + sizeof(int))) // 517 = 512 + 1 + 4 == fileName + fileNum + totalChunks
    {
        FileInfo f = parseFileInfo(serializedData, i);
        fileInfos.push_back(f);
    }

    return fileInfos;
}


FileInfo
NewFileAvailableFrame::getFileInfo()
{
    return parseFileInfo(serializedData, 1);
}

FileInfo parseFileInfo(char * array, int startIndex)
{
    FileInfo f = new FileInfo();
    f.fileNum = array[startIndex];
    f.chunkCount = parseIntFromCharArray(array, startIndex + 1);

    std::string fileName (array + 2, MAX_FILE_NAME_LENGTH);
    f.fileName = fileName;
    return f;
}

int
parseIntFromCharArray(char * array, int startIndex)
{
    int toReturn = 0;
    for (int i = startIndex; i < startIndex + sizeof(int); i++)
    {
        toReturn = (toReturn << sizeof(char)) + array[i];
    }
    return toReturn;
}



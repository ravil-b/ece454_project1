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
#include <iostream>
#include <bitset>

char
Frame::getFrameType()
{
    return serializedData[0];
}

namespace portAndIp_serialization{
    void setIp(std::string ip, char *serializedData){
        strcpy(serializedData + sizeof(char), ip.c_str());
    }

    std::string getIp(char *serializedData)
    {
        return std::string(serializedData + 1, 15);
    }

    void setPort(std::string port, char *serializedData)
    {
        strcpy(serializedData + sizeof(char) * 16, port.c_str());
    }

    std::string getPort(char *serializedData)
    {
	return std::string(serializedData + sizeof(char) * 16, 5);
    }
}

namespace serialization_helpers{
    FileInfo
    parseFileInfo(char * array, int startIndex)
    {
        FileInfo f;
        f.fileNum = array[startIndex];
        f.chunkCount = serialization_helpers::parseIntFromCharArray(array, startIndex + 1);
        f.fileSize = serialization_helpers::parseIntFromCharArray(array, startIndex + 5);
        std::string fileName (array + 9 + startIndex, MAX_FILE_NAME_LENGTH);
        f.fileName = fileName;

        return f;
    }

    FileInfo *
    parseFileInfoNoChunkCount(char * array, int startIndex)
    {
        FileInfo * f = new FileInfo();
        f->fileNum = array[startIndex];
        f->fileSize = serialization_helpers::parseIntFromCharArray(array, startIndex + 1);
        std::string fileName (array + 5 + startIndex, MAX_FILE_NAME_LENGTH);
        f->fileName = fileName;
        for (int i = 0; i < chunkSize; i++)
        {
            f->chunksDownloaded[i] = false;
        }


        return f;
    }

    int
    parseIntFromCharArray(const char * array, int startIndex)
    {
        int toReturn = 0;
        for (int i = startIndex; i < (4 + startIndex); i++)
        {
	    int tmp = (unsigned)array[i];
	    tmp &= 0xff;
            toReturn = (toReturn << 8) + tmp;
        }
        return toReturn;
    }

    short
    parseShortFromCharArray(const char * array)
    {
        short toReturn = 0;
        for (int i = 0; i < 2; i++)
        {
            toReturn = (toReturn << 8) + (unsigned)array[i];
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
            serialization_helpers::copyIntToCharArray(frame->serializedData + serialDataIdx + 1, (unsigned)file->fileSize);

            // File name
            const char *fName = file->fileName.c_str();
            for (int y = 0; y < 512; y++){
                frame->serializedData[serialDataIdx + y + 5] = fName[y];

                if (fName[y] == 0){
                    break;
                }
            }
    }

//        std::vector<FileInfo *> fileInfos = getFileInfos(frame);
//        for (unsigned int i = 0; i < fileInfos.size(); i++)
//        {
//            FileInfo * f = fileInfos[i];
//        }


	return frame;
    }

    std::vector<FileInfo *>
    getFileInfos(Frame * frame)
    {
        char fileCount = frame->serializedData[1];
        std::vector<FileInfo *> fileInfos;
        int endIterIdx = (fileCount * FILE_INFO_DATA_WIDTH) + 2;

        for (int i = 2; i < endIterIdx; i += FILE_INFO_DATA_WIDTH)
        {
            FileInfo * f = serialization_helpers::parseFileInfoNoChunkCount(frame->serializedData, i);

            fileInfos.push_back(f);

        }
        return fileInfos;
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
	portAndIp_serialization::setIp(ip, newFrame->serializedData);
	portAndIp_serialization::setPort(port, newFrame->serializedData);
        return newFrame;
    }

    std::string
    getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }


    std::string
    getPort(Frame * frame)
    {
        return portAndIp_serialization::getPort(frame->serializedData);
    }
}

namespace chunkInfo_serialization
{


//                unsigned char b = 0;
//                b |= (chunkMap[fileIter->first][chunkNum] & 1) << 7;
//                b |= (chunkMap[fileIter->first][chunkNum + 1] & 1) << 6;
//                b |= (chunkMap[fileIter->first][chunkNum + 2] & 1) << 5;
//                b |= (chunkMap[fileIter->first][chunkNum + 3] & 1) << 4;
//                b |= (chunkMap[fileIter->first][chunkNum + 4] & 1) << 3;
//                b |= (chunkMap[fileIter->first][chunkNum + 5] & 1) << 2;
//                b |= (chunkMap[fileIter->first][chunkNum + 6] & 1) << 1;
//                b |= (chunkMap[fileIter->first][chunkNum + 7] & 1) << 0;

    const unsigned int fileCountIdx = sizeof(char) + 15 + 5;
    const unsigned int chunkInfoStartIdx = sizeof(char) + 15 + 5 + sizeof(char);

    // { filenumber: { chunkNumber: chunkAvailable } }
    Frame *
    createChunkInfoFrame(char fileCount, std::string ip, std::string port, std::map<char, std::map<int, bool> > chunkMap)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::CHUNK_INFO;

        portAndIp_serialization::setIp(ip, newFrame->serializedData);
        portAndIp_serialization::setPort(port, newFrame->serializedData);

        newFrame->serializedData[fileCountIdx] = fileCount;

        std::map<char, std::map<int, bool> >::iterator fileIter;

        int fileCounter = 0;
        int bytesPerFile = (maxChunksPerFile/8) + 1;
        for (fileIter = chunkMap.begin(); fileIter != chunkMap.end(); ++fileIter)
        {
            // set fileNum
            newFrame->serializedData[chunkInfoStartIdx + fileCounter*(bytesPerFile)] = fileIter->first;

            for (unsigned int chunkNum = 0; chunkNum < maxChunksPerFile; chunkNum+=8)
            {
                std::bitset<8> bits;
                for (unsigned int bitIdx = 0; bitIdx < 8; bitIdx++)
                {
                    if (chunkMap[fileIter->first][chunkNum + bitIdx])
                    {
                        bits.set(7 - bitIdx);
                    }
                }

                int chunkIdx = chunkInfoStartIdx + (fileCounter * bytesPerFile) + 1 + (chunkNum / 8);
                newFrame->serializedData[chunkIdx] = (unsigned char)bits.to_ulong();
            }
            fileCounter++;
        }

        return newFrame;
    }




    char
    getFileCount(Frame * frame)
    {
        return frame->serializedData[fileCountIdx];
    }

    std::map<char, std::map<int, bool> >
    getChunkMap(Frame * frame)
    {
        std::map<char, std::map<int, bool> > fileMap;

        int bytesPerFile = (maxChunksPerFile/8) + 1;
        for (char fileCounter; fileCounter < getFileCount(frame); fileCounter++)
        {
            std::map<int, bool> chunkMap;

            char fileNum = frame->serializedData[chunkInfoStartIdx + fileCounter*(bytesPerFile)];
            for (unsigned int chunkNum = 0; chunkNum < maxChunksPerFile; chunkNum+=8)
            {
                int chunkIdx = chunkInfoStartIdx + (fileCounter * bytesPerFile) + 1 + (chunkNum / 8);
                std::bitset<8> bits((unsigned char)frame->serializedData[chunkIdx]);

//                std::cout << "DESERIALIZING." << std::endl;
//                std::cout   << "chunkIdx: " << chunkIdx
//                            << " value: " << (int) frame->serializedData[chunkIdx]
//                            << " bits: " << bits << std::endl;

                for (unsigned int bitIdx = 0; bitIdx < 8; bitIdx++)
                {
                    chunkMap.insert(std::make_pair(chunkNum + bitIdx, (bool)bits[7 - bitIdx]));
                }
            }

            fileMap.insert(std::make_pair(fileNum, chunkMap));
        }
        return fileMap;
    }

    std::string getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }

    std::string getPort(Frame * frame)
    {
        return portAndIp_serialization::getPort(frame->serializedData);
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



namespace chunk_serialization_helpers
{
    void setFileNum(char * serializedData, char fileNum)
    {
        serializedData[1] = fileNum;
    }

    void setChunkNum(char * serializedData, int chunkNum)
    {
        serialization_helpers::copyIntToCharArray(serializedData + 2, chunkNum);
    }

    void setChunkCount(char * serializedData, int chunkNum)
    {
        serialization_helpers::copyIntToCharArray(serializedData, chunkNum);
    }

    char getFileNum(char * serializedData)
    {
        return serializedData[1];
    }
    int getChunkNum(char * serializedData)
    {
        return serialization_helpers::parseIntFromCharArray(serializedData, 2);
    }

    int getChunkCount(Frame * frame)
       {
           return serialization_helpers::parseIntFromCharArray(frame->serializedData, 2);
       }
}


namespace chunkRequestFrame_serialization
{
    Frame *
    createChunkRequestFrame(char fileNum, int chunkNum){
	Frame *newFrame = new Frame();
	newFrame->serializedData[0] = (char)FrameType::CHUNK_REQUEST;
	newFrame->serializedData[1] = fileNum;
	serialization_helpers::copyIntToCharArray(newFrame->serializedData + 2, chunkNum);
	return newFrame;
    }
    char getFileNum(Frame * frame)
    {
        return chunk_serialization_helpers::getFileNum(frame->serializedData);
    }
    int getChunkNum(Frame * frame)
    {
        return chunk_serialization_helpers::getChunkNum(frame->serializedData);
    }
};


namespace chunkDataFrame_serialization
{
    Frame *
    createChunkDataFrame(char fileNum, int chunkNum, char * chunk)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::CHUNK;
        chunk_serialization_helpers::setFileNum(newFrame->serializedData, fileNum);
        chunk_serialization_helpers::setChunkNum(newFrame->serializedData, chunkNum);

	int frameOffset = sizeof(char) + sizeof(char) + sizeof(int);
        
        for (int i = 0; i < chunkSize; i++)
        {
            newFrame->serializedData[i+frameOffset] = chunk[i];
        }

        return newFrame;
    }
    char getFileNum(Frame * frame)
    {
        return chunk_serialization_helpers::getFileNum(frame->serializedData);
    }
    int getChunkNum(Frame * frame)
    {
        return chunk_serialization_helpers::getChunkNum(frame->serializedData);
    }
    char * getChunk(Frame * frame)
    {
        return frame->serializedData + (sizeof(char) + sizeof(char) + sizeof(int));
    }

}

namespace chunkRequestDecline_serialization
{
    Frame *
    createChunkRequestDeclineFrame(char fileNum, int chunkNum)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::CHUNK_REQUEST_DECLINE;
        chunk_serialization_helpers::setFileNum(newFrame->serializedData, fileNum);
        chunk_serialization_helpers::setChunkNum(newFrame->serializedData, chunkNum);
        return newFrame;
    }
    char getFileNum(Frame * frame)
    {
        return chunk_serialization_helpers::getFileNum(frame->serializedData);
    }
    int getChunkNum(Frame * frame)
    {
        return chunk_serialization_helpers::getChunkNum(frame->serializedData);
    }
}

namespace newChunkAvailable_serialization
{
    const unsigned int fileNumIdx = sizeof(char) + 15 + 5;
    const unsigned int chunkNumIdx = sizeof(char) + 15 + 5 + sizeof(char);

    Frame *
    createNewChunkAvailableFrame(char fileNum, int chunkNum, std::string ip, std::string port)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::NEW_CHUNK_AVAILABLE;
        newFrame->serializedData[fileNumIdx] = fileNum;
        serialization_helpers::copyIntToCharArray(newFrame->serializedData + chunkNumIdx, chunkNum);

	portAndIp_serialization::setIp(ip, newFrame->serializedData);
	portAndIp_serialization::setPort(port, newFrame->serializedData);
        return newFrame;
    }
    char getFileNum(Frame * frame)
    {
        return chunk_serialization_helpers::getFileNum(frame->serializedData + fileNumIdx);
    }
    int getChunkNum(Frame * frame)
    {
        return chunk_serialization_helpers::getChunkNum(frame->serializedData + chunkNumIdx);
    }

    std::string getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }
    std::string getPort(Frame * frame)
    {
        return portAndIp_serialization::getPort(frame->serializedData);
    }
}

namespace newFileAvailable_serialization
{
    const unsigned int fileNumIdx = sizeof(char) + 15 + 5;
    const unsigned int chunkCountIdx = sizeof(char) + 15 + 5 + sizeof(char);
    const unsigned int fileSizeIdx = sizeof(char) + 15 + 5 + sizeof(char) + sizeof(int); 
    const unsigned int fileNameIdx = sizeof(char) + 15 + 5 + sizeof(char) + sizeof(int) + sizeof(int);

    Frame *
    createNewFileAvailableFrame(FileInfo * file, std::string ip, std::string port)
    {
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::NEW_FILE_AVAILABLE;

        portAndIp_serialization::setIp(ip, newFrame->serializedData);
        portAndIp_serialization::setPort(port, newFrame->serializedData);

        newFrame->serializedData[fileNumIdx] = file->fileNum;
        serialization_helpers::copyIntToCharArray(newFrame->serializedData + chunkCountIdx, file->chunkCount);
	serialization_helpers::copyIntToCharArray(newFrame->serializedData + fileSizeIdx, file->fileSize);

        const char *fName = file->fileName.c_str();
        for (int y = 0; y < 512; y++){
            newFrame->serializedData[fileNameIdx + y] = fName[y];
            if (fName[y] == 0){
                break;
            }
        }
        return newFrame;
    }
    FileInfo getFileInfo(Frame * frame)
    {
        return serialization_helpers::parseFileInfo(frame->serializedData, fileNumIdx);
    }

    std::string getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }

    std::string getPort(Frame * frame)
    {
	return portAndIp_serialization::getPort(frame->serializedData);
    }
}



namespace peerLeavingFrame_serialization{
    Frame *
    createPeerLeavingFrame(std::string ip, std::string port){
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::PEER_LEAVE_NOTIFICATION;
        portAndIp_serialization::setIp(ip, newFrame->serializedData);
        portAndIp_serialization::setPort(port, newFrame->serializedData);
        return newFrame;
    }

    std::string getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }

    std::string getPort(Frame * frame)
    {
	return portAndIp_serialization::getPort(frame->serializedData);
    }
}


namespace peerJoinNotification_serialization
{
    Frame *
    createPeerJoinNotification(std::string ip, std::string port){
        Frame * newFrame = new Frame();
        newFrame->serializedData[0] = (char)FrameType::PEER_JOIN_NOTIFICATION;
        portAndIp_serialization::setIp(ip, newFrame->serializedData);
        portAndIp_serialization::setPort(port, newFrame->serializedData);
        return newFrame;
    }

    std::string getIp(Frame * frame)
    {
        return portAndIp_serialization::getIp(frame->serializedData);
    }

    std::string getPort(Frame * frame)
    {
    return portAndIp_serialization::getPort(frame->serializedData);
    }
}

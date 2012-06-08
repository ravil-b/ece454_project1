#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H

#include "Globals.h"

const short FRAME_LENGTH = 65542;
const short MAX_FILE_NAME_LENGTH = 512;

enum FrameType
{
    CHUNK = 0,
    CHUNK_REQUEST = 1,
    FILE_LIST = 2,
    FILE_LIST_REQUEST = 3,
    FILE_LIST_DECLINE = 4,
    NEW_FILE_AVAILABLE = 5,
    NEW_CHUNK_AVAILABLE = 6
};

// FILE_LIST_DECLINE:
// If a server connects, asks for the file_list, and is waiting, and during this waiting time another server connects and
// sends a file_list_request to this server, this server should decline.




// Frame Struct Definitions
// ------------------------
//
// See ../SerializedMessageFormats.txt to see how these structs are serialized

struct FileInfo // stores info about a file
{
    std::string fileName;
    char fileNum;
    int chunkCount;
};

struct Frame
{
    char serializedData[FRAME_LENGTH];
    char getFrameType();

};


struct ChunkFrame : Frame
{
    char getFileNum();
    int getChunkNum();
};

struct ChunkDataFrame : ChunkFrame
{
    char * getChunk();
};

struct ChunkRequestFrame : ChunkFrame{};
struct NewChunkAvailableFrame: ChunkFrame{};




struct FileListFrame: Frame
{
    char getFileCount();
    std::vector<FileInfo> getFileInfos();

private:
    std::vector<FileInfo> fileInfos_;
};
struct FileListRequestFrame: Frame{};
struct FileListDeclineFrame: Frame{};


struct NewFileAvailableFrame: Frame
{
    FileInfo getFileInfo();
};

#endif

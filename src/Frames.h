#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H


#include <string>
#include <vector>
#include <map>
#include "Globals.h"
#include "Files.h"

const int FRAME_LENGTH = 65542;
const short MAX_FILE_NAME_LENGTH = 512;


//                                           fileNum        totalChunks            fileName
const short FILE_INFO_DATA_WIDTH =          sizeof(char) +  sizeof(int)    +   MAX_FILE_NAME_LENGTH;
const short TOTAL_CHUNKS_AT_SOURCE_OFFSET =   sizeof(char) +  sizeof(int);




// FILE_LIST_DECLINE:
// If a server connects, asks for the file_list, and is waiting, and during this waiting time another server connects and
// sends a file_list_request to this server, this server should decline.




// Frame Struct Definitions
// ------------------------
//
// See ../SerializedMessageFormats.txt to see how these structs are serialized

struct FrameType
{
    enum FrameType_T
    {
        HANDSHAKE_REQUEST =     0,
        HANDSHAKE_RESPONSE =    1,

        CHUNK =                 2,
        CHUNK_REQUEST =         3,
        CHUNK_REQUEST_DECLINE = 4,

        CHUNK_COUNT_REQUEST =   5,
        CHUNK_COUNT =           6,

        FILE_LIST =             7,
        FILE_LIST_REQUEST =     8,
        FILE_LIST_DECLINE =     9,

        NEW_FILE_AVAILABLE =    10,
        NEW_CHUNK_AVAILABLE =   11,

        CHUNK_INFO =            12,
        CHUNK_INFO_REQUEST =    13
    };
};

struct Frame
{
    char serializedData[FRAME_LENGTH];
    char getFrameType();
    void setFrameType(FrameType frameType);

};

namespace serialization_helpers{

    FileInfo
    parseFileInfo(char * array, int startIndex);

    int
    parseIntFromCharArray(const char * array, int startIndex);

    int
    parseIntFromCharArray(const char * array, int startIndex);

    short
    parseShortFromCharArray(const char * array);

    void
    copyIntToCharArray(char * array, int toCopy);

    void
    copyShortToCharArray(char * array, short toCopy);
}


namespace frame_function{
    // IP and Port
    std::string getIp(char *serializedData);
    std::string getPort(char *serializedData);
    void setIp(std::string ip, char *serializedDatxa);
    void setPort(char *serializedData);
};

namespace fileListFrame_serialization{
    Frame *
    createFileListFrame(char fileCount, std::vector<FileInfo *> files);
    std::vector<FileInfo> getFileInfos(Frame * frame);
    char getFileCount();
};

namespace chunkInfo_serialization
{
    Frame * createChunkInfoFrame(char fileCount, std::map<char, std::map<int, bool> > chunkMap); // { filenumber: { chunkNumber: chunkAvailable } }
    char getFileCount();
    std::map<char, std::map<int, bool> > getChunkMap();
};

namespace chunkInfoRequest_serialization
{
    Frame *
    createChunkInfoRequest();
};

struct PeerInfoFrame: Frame
{
    PeerInfoFrame(std::string ip, std::string port);
    std::string getIp();
    std::string getPort();
};


struct HandshakeRequestFrame : Frame
{
    HandshakeRequestFrame();
};

struct HandshakeResponseFrame : PeerInfoFrame
{
    HandshakeResponseFrame(std::string ip, std::string port);
};

struct FileNumFrame: Frame
{
    FileNumFrame(char fileNum);
    char getFileNum();
};


struct ChunkFrame : FileNumFrame
{
    ChunkFrame(char fileNum, int chunkNum);
    int getChunkNum();
};

struct ChunkDataFrame : ChunkFrame
{
    ChunkDataFrame(char fileNum, int chunkNum, char* chunk);
    char * getChunk();
    void setChunk(char * chunk);
};

struct ChunkRequestFrame : ChunkFrame{
    ChunkRequestFrame(char fileNum, int chunkNum);
};
struct ChunkRequestDeclineFrame : ChunkFrame{
    ChunkRequestDeclineFrame(char fileNum, int chunkNum);
};


struct FileListFrame: Frame
{
    FileListFrame(char fileCount, std::vector<FileInfo> fileInfos);
    char getFileCount();

    std::vector<FileInfo> getFileInfos();

private:
    void parseFileInfos();
    std::vector<FileInfo> * fileInfos_;
};
struct FileListRequestFrame: Frame{
    FileListRequestFrame();
};
struct FileListDeclineFrame: Frame{
    FileListDeclineFrame();
};


struct NewFileAvailableFrame: Frame
{
    FileInfo getFileInfo();
};
struct NewChunkAvailableFrame: ChunkFrame{};


struct ChunkInfoFrame: Frame
{
    ChunkInfoFrame(char fileCount, std::map<char, std::map<int, bool> > chunkMap); // { filenumber: { chunkNumber: chunkAvailable } }
    char getFileCount();
    std::map<char, std::map<int, bool> > getChunkMap();
};

struct ChunkInfoRequestFrame: Frame {
    ChunkInfoRequestFrame();
};

#endif

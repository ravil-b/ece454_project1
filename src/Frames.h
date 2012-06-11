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
        CHUNK_INFO_REQUEST =    13,

        PEER_LEAVE_NOTIFICATION = 14
    };
};

struct Frame
{
    char serializedData[FRAME_LENGTH];
    char getFrameType();

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

namespace handshakeRequestFrame_serialization
{
    Frame * createHandshakeRequestFrame();
};

namespace handshakeResponseFrame_serialization
{
    Frame * createHandshakeResponseFrame(std::string ip, std::string port);
    std::string getIp(Frame * frame);
    std::string getPort(Frame * frame);

};


namespace portAndIp_serialization{
    // IP and Port
    std::string getIp(char *serializedData);
    std::string getPort(char *serializedData);
    void setIp(std::string ip, char *serializedDatxa);
    void setPort(char *serializedData);
};

namespace peerLeavingFrame_serialization{
    Frame *
    createPeerLeavingFrame(std::string ip, std::string port);
};

namespace fileListFrame_serialization{
    Frame *
    createFileListFrame(char fileCount, std::vector<FileInfo *> files);
    std::vector<FileInfo> getFileInfos(Frame * frame);
    char getFileCount();
};



namespace fileListRequestFrame_serialization
{
    Frame *
    createFileListRequest();

};


namespace chunkRequestFrame_serialization
{
    Frame *createChunkRequestFrame(char fileNum, int chunkNum);
    char getFileNum(Frame * frame);
    int getChunkNum(Frame * frame);
};

namespace chunkDataFrame_serialization
{
    Frame *
    createChunkDataFrame(char fileNum, int chunkNum, char * chunk);
    char getFileNum(Frame * frame);
    int getChunkNum(Frame * frame);
    char * getChunk(Frame * frame);

};

namespace chunkRequestDecline_serialization
{
    Frame *
    createChunkRequestDeclineFrame(char fileNum, int chunkNum);
    char getFileNum(Frame * frame);
    int getChunkNum(Frame * frame);

};

namespace newChunkAvailable_serialization
{
    Frame *
    createNewChunkAvailableFrame(char fileNum, int chunkNum, std::string ip, std::string port);
    char getFileNum(Frame * frame);
    int getChunkNum(Frame * frame);
    std::string getIp(Frame * frame);
    std::string getPort(Frame * frame);
};

namespace newFileAvailable_serialization
{
    Frame *
    createNewFileAvailableFrame(FileInfo * file, std::string ip, std::string port);
    std::string getIp(Frame * frame);
    std::string getPort(Frame * frame);
    FileInfo getFileInfo(Frame * frame);

};

namespace chunkInfo_serialization
{
    Frame * createChunkInfoFrame(char fileCount, std::string ip, std::string port, std::map<char, std::map<int, bool> > chunkMap); // { filenumber: { chunkNumber: chunkAvailable } }
    char getFileCount(Frame * frame);
    std::string getIp(Frame * frame);
    std::string getPort(Frame * frame);
    std::map<char, std::map<int, bool> > getChunkMap(Frame * frame);
};

namespace chunkInfoRequest_serialization
{
    Frame *
    createChunkInfoRequest();
};

#endif


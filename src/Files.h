#ifndef FILES_H_
#define FILES_H_

#include <map>

struct FileInfo // stores info about a file
{
    std::string fileName;
    char fileNum;
    int chunkCount;

};

struct RemoteFileInfo: FileInfo
{
    int totalChunksDownloaded;
};


struct LocalFileInfo : FileInfo
{
    std::map<int, bool> chunksDownloaded;
};

#endif /* FILES_H_ */

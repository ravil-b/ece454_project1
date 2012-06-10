#ifndef FILES_H_
#define FILES_H_

#include <map>
#include <vector>
#include <string>

struct FileInfo // stores info about a file
{
    std::string fileName;
    char fileNum;
    int chunkCount;
    std::map<int, bool> chunksDownloaded;
};


struct LocalFileInfoList
{
    std::vector<FileInfo *> files;
    FileInfo * getFileFromFileNumber(char fileNumber);
    FileInfo * getFileFromFileName(std::string fileName);

};

#endif /* FILES_H_ */

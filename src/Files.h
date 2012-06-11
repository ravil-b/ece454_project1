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
    int fileSize;
    std::map<int, bool> chunksDownloaded;

    FileInfo();
    FileInfo(const FileInfo& f);
};


struct FileInfoList
{
    std::vector<FileInfo *> files;
    FileInfo * getFileFromFileNumber(char fileNumber);
    FileInfo * getFileFromFileName(std::string fileName);
    bool contains(FileInfo * file);

};

#endif /* FILES_H_ */

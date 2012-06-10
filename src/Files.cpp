#include "Files.h"
#include "Globals.h"

#include <string.h>
#include <map>


void initChunksDownloaded(FileInfo * f)
{
    for (int i; i < chunkSize; i++)
    {
        f->chunksDownloaded[i] = false;
    }
}

FileInfo::FileInfo()
{
    initChunksDownloaded(this);
}

FileInfo::FileInfo(const FileInfo& f)
{

    fileName = f.fileName;
    fileNum = f.fileNum;
    chunkCount = f.chunkCount;
    initChunksDownloaded(this);
}

FileInfo *
FileInfoList::getFileFromFileNumber(char fileNum)
{
    for (int fileIdx; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];
        if (f->fileNum == fileNum)
            return f;
    }

    return 0;
}

FileInfo *
FileInfoList::getFileFromFileName(std::string fileName)
{
    for (int fileIdx; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];
        if (strcmp(f->fileName.c_str(), fileName.c_str()))
            return f;
    }
    return 0;
}

bool
FileInfoList::contains(FileInfo * file)
{
    for (int fileIdx; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];
        if (file->fileNum == f->fileNum)
            return true;
    }
    return false;
}



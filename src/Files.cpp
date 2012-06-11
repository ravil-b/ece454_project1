#include "Files.h"
#include "Globals.h"

#include <string.h>
#include <map>
#include <iostream>


void initChunksDownloaded(FileInfo * f)
{
    for (int i = 0; i < chunkSize; i++)
    {
        f->chunksDownloaded.insert(std::make_pair(i, false));
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
    for (unsigned int fileIdx = 0; fileIdx < files.size(); fileIdx++)
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
    for (unsigned int fileIdx = 0; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];

        if (f->fileName == fileName)
            return f;
    }
    return 0;
}

bool
FileInfoList::contains(FileInfo * file)
{
    for (unsigned int fileIdx = 0; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];
        if (file->fileNum == f->fileNum)
            return true;
    }
    return false;
}



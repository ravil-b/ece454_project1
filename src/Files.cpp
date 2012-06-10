#include "Files.h"

#include <string.h>

FileInfo *
LocalFileInfoList::getFileFromFileNumber(char fileNum)
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
LocalFileInfoList::getFileFromFileName(std::string fileName)
{
    for (int fileIdx; fileIdx < files.size(); fileIdx++)
    {
        FileInfo * f = files[fileIdx];
        if (strcmp(f->fileName.c_str(), fileName.c_str()))
            return f;
    }
    return 0;
}





#include "peer.h"
#include <sstream>


void
Status::setNumFiles(char numFiles)
{
    _numFiles = numFiles;
}

void
Status::setLocalFilePresence(char fileNum, float fractionPresentLocally)
{
    _local[(int)fileNum] = fractionPresentLocally;
}


void
Status::setSystemFilePresence(char fileNum, float fractionPresentInSystem)
{
    _system[(int)fileNum] = fractionPresentInSystem;
}

void
Status::setLeastReplication(char fileNum, int totalChunkCountInAllPeers)
{
    _leastReplication[(int)fileNum] = totalChunkCountInAllPeers;
}

void
Status::setWeightedLeastReplication(char fileNum, float totalChunkFractionInAllPeers)
{
    _weightedLeastReplication[(int)fileNum] = totalChunkFractionInAllPeers;
}

std::string
Status::toString(char numFiles, FileInfoList *fileInfoList)
{
    std::stringstream toReturn;
    toReturn << "_numFiles: " << _numFiles << std::endl;

    for (int i = 0; i < numFiles; i++)
    {
	toReturn << "----------------------------------" << std::endl;
	toReturn << "File Number:               " << i << std::endl;
	if (fileInfoList != NULL){
	    FileInfo *fi = fileInfoList->getFileFromFileNumber(i);
	    toReturn << "File Name:                 " << fi->fileName << std::endl;
	    toReturn << "File Size:                 " << fi->fileSize << std::endl;
	    toReturn << "Chunks:                    " << fi->chunkCount << std::endl;
	}        
        toReturn << "_local:                    " << _local[i] << std::endl;
        toReturn << "_system:                   " << _system[i] << std::endl;
        toReturn << "_leastReplication:         " << _leastReplication[i] << std::endl;
        toReturn << "_weightedLeastReplication: " << _weightedLeastReplication[i] << std::endl;
	toReturn << "----------------------------------" << std::endl;
    }

    return toReturn.str();

}

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
Status::toString(char numFiles)
{
    std::stringstream toReturn;
    toReturn << "_numFiles: " << _numFiles << std::endl;

    for (int i; i < numFiles; i++)
    {
        toReturn << "fileNum: " << i << std::endl;
        toReturn << "_local: " << _local[i] << std::endl;
        toReturn << "_system: " << _system[i] << std::endl;
        toReturn << "_leastReplication: " << _leastReplication[i] << std::endl;
        toReturn << "_weightedLeastReplication: " << _weightedLeastReplication[i] << std::endl;
    }

    return toReturn.str();

}

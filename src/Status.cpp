#include "peer.h"

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


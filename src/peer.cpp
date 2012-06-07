
#include "debug.h"
#include "peer.h"

#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

// Peers::Peers(Cli *cli){
//     CliCmdHandler(cli);
// }

int
Peers::initialize(const string &peersFile){
    TRACE("peer.cpp", "Initializing peers.");
    filesystem::path p(peersFile);
    if (!exists(p)){
	cout << "peers.txt appears to be missing!" << endl;
	// This currently doesn't work.
	cli_->terminate();
	return errPeersFileNotFound;
    }
    filesystem::ifstream ifs(p);
    if (ifs.is_open()){
	string line;
	int lineNum = 0;
	while (getline(ifs, line)){
	    // Split using space as a separator
	    vector<string> splitLine;	
	    split(splitLine, line, is_any_of(" "));
	    // Remove empty elements
	    vector<string>::iterator newEnd;
	    newEnd = remove(splitLine.begin(), splitLine.end(), "");
	    splitLine.erase(newEnd, splitLine.end());
	    // Ensure that we have both the ip and peer
	    if (splitLine.size() != 2){
		TRACE("peer.cpp", "The peers file has wrong format");
		return errPeersFileFmtFail;
	    }
	    initPeer(lineNum, splitLine[0], splitLine[1]);
	    lineNum++;
	}
	if (lineNum != maxPeers){
	    TRACE("peer.cpp", "Wrong number of peers in peers file");
	    return errPeersFileFmtFail;
	}
	return errOk;
    }
    TRACE("peer.cpp", "Failed to open peers file for reading");
    return errPeersFileReadFail;
}

// For simplicity, we assume that the first entry in the peers file corresponds 
// to the local peer.
void
Peers::initPeer(int peerNumber, string ip, string port){
    TRACE("peers.cpp", "Adding a new peer");
    cout << peerNumber << " " << ip << " " << port << endl;
    // if (peerNumber >= maxPeers){
    // 	TRACE("peers.cpp", "Attempt to add a peer out of range");
    // 	return;
    // }
    // Peer *p = new Peer(peerNumber, ip, port);
    // peers_[peerNumber] = p;
}

void 
Peers::handleCmd(vector<string> *cmd){
    return;
}


#include "debug.h"
#include "peer.h"
#include "Client.h"
#include "Server.h"

#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>

using namespace std;
using namespace boost;

/*
 * Peers
 */

Peers::Peers(Cli *cli, const string &peersFile)
    : CliCmdHandler(cli), peersFile_(peersFile){
}

Peers::~Peers(){
    // TODO delete all peers from peers_[]
}

int
Peers::initialize(){
    TRACE("peer.cpp", "Initializing peers.");
    filesystem::path p(peersFile_);
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
	return errOK;
    }
    TRACE("peer.cpp", "Failed to open peers file for reading");
    return errPeersFileReadFail;
}

// For simplicity, we assume that the first entry in the peers file corresponds 
// to the local peer.
void
Peers::initPeer(int peerNumber, string ip, string port){
    TRACE("peers.cpp", "Adding a new peer");
    if (peerNumber >= maxPeers){
    	TRACE("peers.cpp", "Attempt to add a peer out of range");
    	return;
    }
    Peer *p = new Peer(peerNumber, ip, port);
    peers_[peerNumber] = p;
}

void 
Peers::handleCmd(vector<string> *cmd){
    return;
}


/*
 * Peer
 */

Peer::Peer(int peerNumber, string ip, string port)
    : peerNumber_(peerNumber), 
      ip_(ip), 
      port_(port),
      state_(UNKNOWN), 
      client_(NULL), 
      server_(NULL), 
      incomingConnectionsThread_(NULL){

    if (peerNumber == 0){
	initLocalPeer();
    }
    else{
	initRemotePeer();	
    }
} 

Peer::~Peer(){
    if (client_ != NULL){
	delete client_;
    }
    if (server_ != NULL){
	delete server_;
    }
}

int 
Peer::initLocalPeer(){
    TRACE("peer.cpp", "Initializing local peer.");
    // The local peer will use a server and a client.
    // At startup, we only need a server though.
    state_ = INITIALIZING;
    server_ = new Server();
    
    incomingConnectionsThread_ = new thread(boost::bind(&Peer::acceptConnections, this));
    return errOK;
}

int 
Peer::initRemotePeer(){
    return errOK;
}

void 
Peer::acceptConnections(){
    TRACE("peer.cpp", "Accepting Connections");
    int port = atoi(port_.c_str());
    if (port == 0){
	// TODO Crash and burn
    }
    while(true){
	RawRequest request;
	server_->waitForRequest(&request, port);
	TRACE("peer.cpp", "Request Data:");
	TRACE("peer.cpp", request.data);
    }
}

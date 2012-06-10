
#define BOOST_THREAD_USE_LIB




#include "debug.h"
#include "peer.h"
#include "Frames.h"
#include "Connection.h"
#include "ThreadSafeQueue.h"

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
	    addPeer(new Peer (lineNum, splitLine[0], splitLine[1]));
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
Peers::addPeer(Peer * newPeer){
    TRACE("peers.cpp", "Adding a new peer");

    if (newPeer->getPeerNumber() >= maxPeers){
    	TRACE("peers.cpp", "Attempt to add a peer out of range");
    	return;
    }
    peers_[newPeer->getPeerNumber()] = newPeer;
    peerCount_++;
}

void
Peers::removePeer(Peer * peer)
{
    TRACE("peers.cpp", "Removing Peer");
    if (peer->getPeerNumber() >= maxPeers){
        TRACE("peers.cpp", "Attempt to remove a peer out of range");
        return;
    }
    delete peers_[peer->getPeerNumber()];
    peers_[peer->getPeerNumber()] = NULL;
    peerCount_--;
}

void 
Peers::handleCmd(vector<string> *cmd){
    return;
}


Peer **
Peers::getPeers()
{
    return peers_;
}

int
Peers::getPeerCount()
{
    return peerCount_;
}

Peer *
Peers::operator[] (int i)
{
    return peers_[i];
}

/*
 * Peer
 */

Peer::Peer(int peerNumber, string ip, string port)
    : peerNumber_(peerNumber), 
      ipAddress_(ip),
      port_(port),
      state_(UNKNOWN), 
      incomingConnectionsThread_(NULL){

    if (peerNumber == 0){
	initLocalPeer();
    }
    else{
	initRemotePeer();	
    }

    receiveq_ = new ThreadSafeQueue<Request>();

    incomingConnectionsThread_ = new thread(boost::bind(&Peer::acceptConnections, this));
} 

Peer::~Peer(){
}

int 
Peer::initLocalPeer(){
    TRACE("peer.cpp", "Initializing local peer.");
    // The local peer will use a server and a client.
    // At startup, we only need a server though.
    state_ = INITIALIZING;
    //    server_ = new Server();
    
//    incomingConnectionsThread_ = new thread(boost::bind(&Peer::acceptConnections, this));
    sendq_ = new ThreadSafeQueue<Frame *>();

    Connection * c = new Connection();
    c->startServer(port_, receiveq_);

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
    Request * request;
    while(receiveq_->pop(request)){
        Peer::handleRequest(request);

    }

    delete request;
    delete receiveq_;
}


void
Peer::handleRequest(Request request)
{
    switch (request.frame->getFrameType())
    {
        case FrameType::CHUNK:

            break;

        case FrameType::CHUNK_COUNT:

            break;

        case FrameType::CHUNK_COUNT_REQUEST:

            break;


        case FrameType::CHUNK_REQUEST:

            break;

        case FrameType::CHUNK_REQUEST_DECLINE:

            break;

        case FrameType::FILE_LIST:

            break;

        case FrameType::FILE_LIST_DECLINE:

            break;

        case FrameType::FILE_LIST_REQUEST:

            break;

        case FrameType::NEW_CHUNK_AVAILABLE:

                break;
        case FrameType::NEW_FILE_AVAILABLE:

                break;


    }
}

int Peer::insert(string fileName)
{
    filesystem::path pathToNewFile(fileName);
    if (!filesystem::exists(pathToNewFile)){
        cout << fileName << " does not exist!" << endl;
        return errInsertFileNotFound;
    }
    try
    {
        filesystem::path pathToLocalStore(LOCAL_STORAGE_PATH_NAME);
        pathToLocalStore /= pathToNewFile.filename().string();

        filesystem::copy_file(pathToNewFile, pathToLocalStore, filesystem::copy_option::overwrite_if_exists);
    }
    catch (std::exception& e)
    {
        std::cerr << "Problem copying to localStorage: " << e.what() << "\n";
        return errCannotCopyInsertFile;
    }

    return errOK;
}

int
Peer::query(Status& status)
{
    status.setNumFiles(fileList_.size());

    for (int fileIdx; fileIdx < (int)fileList_.size(); fileIdx++)
    {
        LocalFileInfo file = fileList_.at(fileIdx);

        // _local
        int totalChunksPresentLocally = 0;
        for (int chunkIdx = 0; chunkIdx < file.chunkCount; chunkIdx++)
        {
            if (file.chunksDownloaded[chunkIdx] == true)
                totalChunksPresentLocally++;
        }


        float localFilePresence = (float)totalChunksPresentLocally / (float)file.chunkCount;
        status.setLocalFilePresence(file.fileNum, localFilePresence);


        // _system
        int maxPeerChunkCount = 0;
        int totalChunkCountInAllPeers = 0;
        for (int peerIdx; peerIdx < peers_->getPeerCount(); peerIdx++)
        {
            Peer * peer = peers_->getPeers()[peerIdx];
            if (peer->getPeerNumber() == this->getPeerNumber()) continue;

            int chunkCount = peerChunkCount_[peer->getPeerNumber()];

            totalChunkCountInAllPeers += chunkCount;
            if (chunkCount > maxPeerChunkCount)
            {
                maxPeerChunkCount = chunkCount;
            }
        }

        float systemFilePresence = (float)maxPeerChunkCount / (float)file.chunkCount;
        status.setSystemFilePresence(file.fileNum, systemFilePresence);

        // _leastReplication
        status.setLeastReplication(file.fileNum, totalChunkCountInAllPeers);

        // _weightedLeastReplication
        float totalChunkFraction = (float)totalChunkCountInAllPeers / (float)file.chunkCount;
        status.setWeightedLeastReplication(file.fileNum, totalChunkFraction);

    }

    return errOK;
}


int Peer::leave()
{
    // inform peers that this peer is leaving

    // check if this peer has chunks that no one else has
        // if so, push those out

    // close down server/client sockets

    peers_->removePeer(this);
    return errOK;
}

int Peer::join()
{
    // request a file list from a peer
    FileListRequestFrame * frame = new FileListRequestFrame();
    peers_[0]->sendFrame(frame);


    // if peer doesn't return a FILE_LIST_DECLINE,
        // update local file list



    peers_->addPeer(this);
    return errOK;
}

int
Peer::sendFrame(Frame * frame)
{
    try{
        // this.client.sendMessage(peer.getIpAddress(), frame.data)
        Connection *c = new Connection();
        sendq_->push(frame);

        unsigned int sessionId;
        c->connect(getIpAddress(), getPort(), &sessionId, sendq_);
    }
    catch (std::exception& e)
    {
       std::cerr << "Problem sending frame to peer #" << toPeer->getPeerNumber() << " " << e.what() << "\n";
       return errCannotSendMessage;
    }
    return errOK;
}

int Peer::getPeerNumber()
{
    return peerNumber_;
}

string Peer::getIpAddress()
{
    return ipAddress_;
}

string Peer::getPort()
{
    return port_;
}

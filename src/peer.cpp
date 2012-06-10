
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
	// TODO This currently doesn't work.
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
	    addPeer(new Peer(lineNum, splitLine[0], splitLine[1], this));
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

Peer::Peer(int peerNumber, string ip, string port, Peers *peers)
    : peerNumber_(peerNumber), 
      ipAddress_(ip),
      port_(port),
      state_(UNKNOWN), 
      peers_(peers),
      sendq_(NULL),
      receiveq_(NULL),
      incomingConnectionsThread_(NULL){

    if (peerNumber == 0){
	initLocalPeer();
    }
    else{
	initRemotePeer();	
    }


} 

Peer::~Peer(){
}

int 
Peer::initLocalPeer(){
    TRACE("peer.cpp", "Initializing local peer.");

    state_ = INITIALIZING;
    sendq_ = new ThreadSafeQueue<Frame *>();
    chunkIO = new FileChunkIO();

    peers_->connection_ = new Connection();
    receiveq_ = new ThreadSafeQueue<Request>();
    boost::thread connThread(boost::bind(&Connection::startServer, 
    					 peers_->connection_, port_, receiveq_));
    incomingConnectionsThread_ = new thread(boost::bind(&Peer::acceptConnections, this));


    return errOK;
}


int 
Peer::initRemotePeer(){
    TRACE("peer.cpp", "Initializing remote peer.");
    // connect to the peer to see if it is online
    state_ = INITIALIZING;
    sendq_ = new ThreadSafeQueue<Frame *>();
    unsigned int sessionId;
    int connCode = peers_->connection_->connect(ipAddress_, port_, &sessionId, sendq_);
    if (connCode != CONNECTION_OK){
	TRACE("peer.cpp", "Cannot connect to remote peer. DISCONNECTED");
	state_ = DISCONNECTED;
	delete sendq_;
	sendq_ = NULL;
	return errCannotConnectToPeer;
    }
    state_ = WAITING_FOR_HANDSHAKE;
    TRACE("peer.cpp", "Connected to remote peer. Requesting handshake.");

    HandshakeRequestFrame * handshakeRequest = new HandshakeRequestFrame();
    sendq_->push(handshakeRequest);

    return errOK;
}

void 
Peer::acceptConnections(){
    TRACE("peer.cpp", "Accepting Connections");
    int port = atoi(port_.c_str());
    if (port == 0){
	// TODO Crash and burn
    }
    Request request;
    while(receiveq_->pop(&request)){
        Peer::handleRequest(request);
	
    }

    delete request.frame;
    delete receiveq_;
}


void
Peer::handleRequest(Request request)
{
    switch (request.frame->getFrameType())
    {

        case FrameType::HANDSHAKE_REQUEST:
        {
            cout << "Handshake Request Received" << endl;
            HandshakeResponseFrame * response = new HandshakeResponseFrame(
                ipAddress_, port_);
            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);
	    if (q == NULL){
		std::cerr << "Cannot find the queue to send reply to peer." << std::endl;
	    }
            q->push(response);
        }
        break;

        case FrameType::HANDSHAKE_RESPONSE:
        {
            cout << "Handshake Response Received" << endl;
	    std::cout << frame_function::getIp(request.frame->serializedData) << std::endl;
	    std::cout << frame_function::getPort(request.frame->serializedData) << std::endl;
	    std::string frameIp = frame_function::getIp(request.frame->serializedData);
	    std::string framePort = frame_function::getPort(request.frame->serializedData);
	    // change the status of the peer to connected.
	    for(int i = 1; i < maxPeers; i++){
	    	if ((strcmp((*peers_)[i]->getIpAddress().c_str(), 
			    frameIp.c_str()) == 0) &&
		    (strcmp((*peers_)[i]->getPort().c_str(), 
			    framePort.c_str()))){
		    TRACE("peer.cpp", "Received handshake. CONNECTED");
		    (*peers_)[i]->state_ = CONNECTED;
		    break;
		}
	    }
        }
            break;

        case FrameType::CHUNK:
            // write the chunk to file
            break;

        case FrameType::CHUNK_COUNT:
            // update chunk count list for peer
            break;

        case FrameType::CHUNK_COUNT_REQUEST:
            // respond with chunk_count
            break;


        case FrameType::CHUNK_REQUEST:
            // if we have the chunk
                // respond with chunk
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
        default:
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
    (*peers_)[0]->sendFrame(frame);


    // if peer doesn't return a FILE_LIST_DECLINE,
        // update local file list



    peers_->addPeer(this);
    return errOK;
}

int
Peer::sendFrame(Frame * frame)
{
    sendq_->push(frame);
    unsigned int sessionId;
    if (peers_->connection_->connect(getIpAddress(), getPort(), &sessionId, sendq_) != CONNECTION_OK){
        std::cerr << "Problem sending frame to peer #" << peerNumber_ << std::endl;
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

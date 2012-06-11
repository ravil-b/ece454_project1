
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
#include <boost/foreach.hpp>
#include <string.h>



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
	// Once all peers are initialized, finish initializing the local
	// storage of file.
	if (peers_[0]->initLocalFileStore() != 0){
	    cerr << "CANNOT INITIALIZE LOCAL FILE STORAGE. Quitting." << endl;
	    // TODO exit the program
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
    // receives commands from the cli command handler.
    // the first element in cmd is the actual command
    // followed by any parameters.
    
    Peer * localPeer = peers_[0];

    string commandStr = (*cmd)[0];
    string insertCommandStr = "insert";
    string queryCommandStr = "query";
    string joinCommandStr = "join";
    string leaveCommandStr = "leave";


    if (strcmp(commandStr.c_str(), insertCommandStr.c_str()) == 0)
    {
        string filePath = (*cmd)[1];
        localPeer->insert(filePath);
    }
    else if (strcmp(commandStr.c_str(), queryCommandStr.c_str()) == 0)
    {
        Status s;
        localPeer->query(s);
        TRACE("peer.cpp", "Status of local peer")
        TRACE("peer.cpp", s.toString(localPeer->getFileInfoList().files.size()))

    }
    else if (strcmp(commandStr.c_str(), joinCommandStr.c_str()) == 0)
    {
        localPeer->join();
    }

    else if (strcmp(commandStr.c_str(), leaveCommandStr.c_str()) == 0)
    {
        localPeer->leave();
    }
}


void
Peers::broadcastFrame(Frame * frame, Peer * fromPeer){
    for (int i = 0; i < peerCount_; i++)
    {
        if (i == fromPeer->getPeerNumber()) continue;

        Peer * p = peers_[i];
        p->sendFrame(frame);
    }
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

Peer *
Peers::getPeerFromIpAndPort(string ip, string port)
{
    for (int i = 0; i < getPeerCount(); i ++)
    {
        Peer * p = peers_[i];
        if (    strcmp(ip.c_str(), p->getIpAddress().c_str()) == 0
            &&  strcmp(ip.c_str(), p->getPort().c_str()) == 0)
        {
            return p;
        }
    }
    return NULL;
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

} 

Peer::~Peer(){
}

int
Peer::initPeer()
{
    if (peerNumber_ == 0){
       initLocalPeer();
   }else{
       initRemotePeer();
   }
    return errOK;
}

int 
Peer::initLocalPeer(){
    TRACE("peer.cpp", "Initializing local peer.");

    state_ = INITIALIZING;
    // Local peer doesn't have a send queue.
    // Use Connection to lookup response queue
    sendq_ = NULL;
    fileChunkIO_ = new FileChunkIO();

    if (peers_->connection_->startServer(port_, receiveq_))
       {
           cerr << "FAILED TO START THE SERVER." << endl;
       }

    incomingConnectionsThread_ = new thread(boost::bind(&Peer::acceptConnections, this));

    // request a file list from a peer
    Frame * frame = fileListRequestFrame_serialization::createFileListRequest();
    for (int i = 0; i < maxPeers; i++)
    {
        Peer * p = (*peers_)[i];
        if (p->state_ == ONLINE)
        {
            p->sendFrame(frame);
            break;
        }
    }

    return errOK;
}


int 
Peer::initRemotePeer(){
    TRACE("peer.cpp", "Initializing remote peer.");
    // connect to the peer to see if it is online
    state_ = INITIALIZING;
    int connCode = connect();
    if (connCode != errOK){
        return connCode;
    }
    state_ = WAITING_FOR_HANDSHAKE;
    TRACE("peer.cpp", "Connected to remote peer. WAITING_FOR_HANDSHAKE");

    Frame * handshakeRequestFrame = handshakeRequestFrame_serialization::createHandshakeRequestFrame();
    sendq_->push(handshakeRequestFrame);

    return errOK;
}

// Check if the local storage directory exists. Create if it doesn't
int 
Peer::initLocalFileStore(){
    filesystem::path pathToLocalStore(LOCAL_STORAGE_PATH_NAME);
    if (!exists(pathToLocalStore)){
	boost::filesystem::create_directories(pathToLocalStore);
    }
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
        handleRequest(request);
    }
    TRACE("peer.cpp", "Stopping to accept connections.");
    // TODO stop the server
    delete request.frame;
    //delete receiveq_;
}


void
Peer::handleRequest(Request request)
{

    Frame * frame = request.frame;
    switch (frame->getFrameType())
    {

        case FrameType::HANDSHAKE_REQUEST:
        {
            TRACE("peer.cpp" ,"Handshake Request Received");
            Frame * response = handshakeResponseFrame_serialization::createHandshakeResponseFrame(ipAddress_, port_);
            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);
            if (q == NULL)
            {
                std::cerr << "Cannot find the queue to send reply to peer." << std::endl;
            }
            else
            {
                q->push(response);
            }
        }
        break;

        case FrameType::HANDSHAKE_RESPONSE:
        {
            TRACE("peer.cpp" ,"Handshake Response Received");
            std::cout << frame_function::getIp(request.frame->serializedData) << std::endl;
            std::cout << frame_function::getPort(request.frame->serializedData) << std::endl;
            std::string frameIp = frame_function::getIp(request.frame->serializedData);
            std::string framePort = frame_function::getPort(request.frame->serializedData);

	    // change the status of the peer to connected.
            for(int i = 1; i < maxPeers; i++){

                if      ((strcmp((*peers_)[i]->getIpAddress().c_str(), frameIp.c_str()) == 0) &&
                         (strcmp((*peers_)[i]->getPort().c_str(), framePort.c_str())))
                {
                    TRACE("peer.cpp", "Received handshake. ONLINE");
                    (*peers_)[i]->state_ = ONLINE;
                    // Close the connection with that remote peer.
                    // disconnect();
                    break;
                }
            }
        }
        break;

        case FrameType::CHUNK:
        {
            // write the chunk to file

            char fileNum = chunkDataFrame_serialization::getFileNum(frame);
            int chunkNum = chunkDataFrame_serialization::getChunkNum(frame);
            char * chunk = chunkDataFrame_serialization::getChunk(frame);

            // should check some sort of checksum..
            fileChunkIO_->writeChunk(
                    fileInfoList_.getFileFromFileNumber(fileNum)->fileName,
                    chunkNum,
                    chunk);
        }
        break;


        case FrameType::CHUNK_REQUEST:
        {
            // if we have the chunk
                // respond with chunk

            char fileNum = chunkRequestFrame_serialization::getFileNum(frame);
            int chunkNum = chunkRequestFrame_serialization::getChunkNum(frame);

            string fileName = fileInfoList_.getFileFromFileNumber(fileNum)->fileName;

            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);

            if (!fileInfoList_.getFileFromFileNumber(fileNum)->chunksDownloaded[chunkNum])
            {
                // we don't have this chunk, so send back a decline
                Frame * f = chunkRequestDecline_serialization::createChunkRequestDeclineFrame(fileNum, chunkNum);
                q->push(f);
                break;
            }

            char chunkBuff[chunkSize];
            if (fileChunkIO_->readChunk(fileName, chunkNum, chunkBuff) != errFileChunkIOOK)
            {
                // error reading the file..
                TRACE("peer.cpp", "Can't read chunk from file.");
            }

            Frame * chunkDataFrame = chunkDataFrame_serialization::createChunkDataFrame(fileNum, chunkNum, chunkBuff);
            q->push(chunkDataFrame);
        }
            break;

        case FrameType::CHUNK_REQUEST_DECLINE:
            // update peer's chunk list to reflect that it DOES NOT have this chunk
            // ask another peer for the chunk
        {
            char fileNum = chunkRequestDecline_serialization::getFileNum(frame);
            int chunkNum = chunkRequestDecline_serialization::getChunkNum(frame);

        }
            break;

        case FrameType::FILE_LIST:
            // update local file list
        {
            cout << "File list response received" << endl;
            handleFileListFrame(frame);
        }
            break;

        case FrameType::FILE_LIST_REQUEST:
        {
            // check if file list is updated
            // if so, respond with file list
                //else : send file_list_decline
            cout << "File list request received" << endl;

            Frame * fileListFrame = fileListFrame_serialization::createFileListFrame(	     
                fileInfoList_.files.size(), fileInfoList_.files);

            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);
	        std::cout << fileListFrame->serializedData[0] << std::endl;

            if (q != NULL)
            {
                q->push(fileListFrame);
            }else{
                std::cerr << "Cannot find the queue to send reply to peer." << std::endl;
            }
        }


            break;

        case FrameType::NEW_CHUNK_AVAILABLE:
        {
            // update this peer's chunk list to reflect that it DOES have this chunk
            char fileNum = newChunkAvailable_serialization::getFileNum(frame);
            int chunkNum = newChunkAvailable_serialization::getChunkNum(frame);
            string ip = newChunkAvailable_serialization::getIp(frame);
            string port = newChunkAvailable_serialization::getPort(frame);

            Peer * p = peers_->getPeerFromIpAndPort(ip, port);
            if (p == NULL)
            {
                // couldn't find peer.. bad news
                break;
            }

            // update the peer's chunk list to say that it now has this chunk
            fileInfoList_.files[fileNum]->chunksDownloaded[chunkNum] = true;
        }
        break;

        case FrameType::NEW_FILE_AVAILABLE:
        {
            // update local file list
            FileInfo * newFile = new FileInfo( newFileAvailable_serialization::getFileInfo(frame));
            fileInfoList_.files.push_back(newFile);
        }
        break;

        case FrameType::CHUNK_INFO:
        {
            char fileCount = chunkInfo_serialization::getFileCount(frame);
            string ip = chunkInfo_serialization::getIp(frame);
            string port = chunkInfo_serialization::getPort(frame);
            std::map<char, std::map<int, bool> > fileChunkMap = chunkInfo_serialization::getChunkMap(frame);


            Peer * fromPeer = peers_->getPeerFromIpAndPort(ip, port);

            std::map<char, std::map<int, bool> >::iterator fileIter;

            // loop for each file
            for (fileIter = fileChunkMap.begin(); fileIter != fileChunkMap.end(); ++fileIter)
            {
                // find the file in the peer's file list
                FileInfo * f = fromPeer->getFileInfoList().getFileFromFileNumber(fileIter->first);

                std::map<int, bool> chunkMap = fileChunkMap[fileIter->first];
                std::map<int, bool>::iterator chunkIter;

                // loop for each chunk
                for (chunkIter = chunkMap.begin(); chunkIter != chunkMap.end(); ++chunkIter)
                {
                    // update the peer's FileInfo struct to reflect
                    // which chunks it has downloaded
                    f->chunksDownloaded[chunkIter->first] = chunkIter->second;
                }
            }
        }
        break;

        case FrameType::CHUNK_INFO_REQUEST:
        {
            string ip = chunkInfoRequest_serialization::getIp(frame);
            string port = chunkInfoRequest_serialization::getPort(frame);


            vector<std::map<char, std::map<int, bool> > > fileChunkMaps;
            std::map<char, std::map<int, bool> > fileChunkMap;

            char fileCount = (char)fileInfoList_.files.size();

            // for each file in this peer
            for (char i = 0; i < (char)fileInfoList_.files.size(); i++)
            {
                FileInfo * file = fileInfoList_.files[i];

                std::map<int, bool> chunkMap;
                fileChunkMap.insert(make_pair(file->fileNum, chunkMap));


                if (i > 14)
                {
                    // we need to start a new request.. skip this for now
                    // TODO: implement this
                }

                std::map<int, bool>::iterator chunkIter;
                // for each chunk of this file, copy the chunk isDownloaded value to our new map
                for (chunkIter = file->chunksDownloaded.begin(); chunkIter != file->chunksDownloaded.end(); chunkIter++)
                {
                    chunkMap.insert(make_pair(chunkIter->first, chunkIter->second));
                }
            }

            Frame * chunkInfoRequest = chunkInfo_serialization::createChunkInfoFrame(
                    fileCount,
                    getIpAddress(),
                    getPort(),
                    fileChunkMap);

            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);
            q->push(chunkInfoRequest);

        }break;

        default:
            break;


    }
}

void Peer::handleFileListFrame(Frame * fileListFrame)
{
    std::vector<FileInfo> fileInfos = fileListFrame_serialization::getFileInfos(fileListFrame);

    for (int i = 0 ; i < (int)fileInfos.size(); i++)
    {
        FileInfo f = fileInfos[i];
        cout << "Got file name" << f.fileName << endl;
        cout << "Got file num" << f.fileNum << endl;
        cout << "Got chunk count" << f.chunkCount << endl;

        if (!fileInfoList_.contains(&f))
        {
            FileInfo * newFile = new FileInfo(f);
            fileInfoList_.files.push_back(newFile);
        }

        string ip = (*peers_)[i]->getIpAddress();
        string port = (*peers_)[i]->getPort();

        // for each peer, if we don't have chunk info for a file, request it
        for (int peerIdx = 1; peerIdx < peers_->getPeerCount(); peerIdx++)
        {
            Peer * p = (*peers_)[i];
            if (!p->haveChunkInfo(f.fileNum))
            {
                Frame * chunkInfoRequest = chunkInfoRequest_serialization::createChunkInfoRequest(ip, port);
                p->sendFrame(chunkInfoRequest);
            }
        }
    }


    // call load local files from disk on new thread

}

char
Peer::getMaxFileNum()
{
    char maxFileNum = 0;
    for (int i = 0; i < (int)this->fileInfoList_.files.size(); i++)
    {
        FileInfo * f = fileInfoList_.files[i];
        if (f->fileNum > maxFileNum)
            maxFileNum = f->fileNum;

    }
    return maxFileNum;
}

void
Peer::loadLocalFilesFromDisk()
{
    boost::filesystem::path targetDir(localStoragePath);
    boost::filesystem::directory_iterator it(targetDir), eod;

    BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod))
    {
        loadLocalFileFromDisk(p);
    }
}

void
Peer::loadLocalFileFromDisk(boost::filesystem::path path)
{
    if(is_regular_file(path))
    {
        FileInfo * f = new FileInfo();
        unsigned int fileSize = boost::filesystem::file_size(path);

        f->fileName = path.filename().string();
        f->chunkCount = fileSize/chunkSize;
        if (fileSize % chunkSize == 0)
        {
            f->chunkCount ++;
        }
        f->fileNum = getMaxFileNum() + 1;

        for (int i = 0; i < f->chunkCount; i++)
        {
            f->chunksDownloaded[i] = true;
        }

        fileInfoList_.files.push_back(f);
    }
}

void
Peer::pushNewFile(std::string fileName)
{

}

// Insert a file into the system
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
Peer::getChunkCount(char fileNum)
{
    // TODO: fill this in if necessary
    return 0;
}

bool
Peer::haveChunkInfo(char fileNum)
{
    return haveChunkInfo_[fileNum];
}

void
Peer::setHaveChunkInfo(const char fileNum, const bool value)
{
   haveChunkInfo_.insert(std::make_pair(fileNum, value));
}

int
Peer::query(Status& status)
{
    status.setNumFiles(fileInfoList_.files.size());

    for (int fileIdx = 0; fileIdx < (int)fileInfoList_.files.size(); fileIdx++)
    {
        FileInfo * file = fileInfoList_.files.at(fileIdx);

        // _local
        int totalChunksPresentLocally = 0;
        for (int chunkIdx = 0; chunkIdx < file->chunkCount; chunkIdx++)
        {
            if (file->chunksDownloaded[chunkIdx] == true)
                totalChunksPresentLocally++;
        }


        float localFilePresence = (float)totalChunksPresentLocally / (float)file->chunkCount;
        status.setLocalFilePresence(file->fileNum, localFilePresence);


        // _system
        int maxPeerChunkCount = 0;
        int totalChunkCountInAllPeers = 0;
        for (int peerIdx = 0; peerIdx < peers_->getPeerCount(); peerIdx++)
        {
            Peer * peer = peers_->getPeers()[peerIdx];
            if (peer->getPeerNumber() == this->getPeerNumber()) continue;

            int chunkCount = getChunkCount(file->fileNum);

            totalChunkCountInAllPeers += chunkCount;
            if (chunkCount > maxPeerChunkCount)
            {
                maxPeerChunkCount = chunkCount;
            }
        }

        float systemFilePresence = (float)maxPeerChunkCount / (float)file->chunkCount;
        status.setSystemFilePresence(file->fileNum, systemFilePresence);

        // _leastReplication
        status.setLeastReplication(file->fileNum, totalChunkCountInAllPeers);

        // _weightedLeastReplication
        float totalChunkFraction = (float)totalChunkCountInAllPeers / (float)file->chunkCount;
        status.setWeightedLeastReplication(file->fileNum, totalChunkFraction);

    }

    return errOK;
}


int Peer::leave()
{
    TRACE("peer.cpp", "Leaving the network.");
    // inform peers that this peer is leaving
    Frame * f = peerLeavingFrame_serialization::createPeerLeavingFrame(
        getIpAddress(), getPort());
    peers_->broadcastFrame(f, this);
    
    // Tell every peer that they need to close connections after all frames are sent
    for (int i = 1; i < maxPeers; i++){
	if ((*peers_)[i]->state_ == ONLINE){
	    (*peers_)[i]->stopConnection();
	}
    }

    // Start shutting down own connections
    //receiveq_->stopReading();

    // check if this peer has chunks that no one else has
    // if so, push those out
    
    // close down server/client sockets

    ///peers_->removePeer(this);
    return errOK;
}

int Peer::join()
{
    peers_->connection_ = new Connection();
    receiveq_ = new ThreadSafeQueue<Request>();

    initPeer();

    // for each file in each peer, set a value to say that we don't have chunk info for the file
    for (int i = 0; i < peers_->getPeerCount(); i++)
    {
        Peer * peer = (*peers_)[i];
        for (int fileIdx = 0; i < peer->getFileInfoList().files.size(); fileIdx++)
        {
            FileInfo * f = getFileInfoList().files[i];
            peer->setHaveChunkInfo(f->fileNum, false);
        }
    }



    return errOK;
}

int
Peer::sendFrame(Frame * frame)
{
    if (connect() == errOK){
	sendq_->push(frame);
	return errOK;
    }
    return errCannotSendFrame;
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

FileInfoList
Peer::getFileInfoList()
{
    return fileInfoList_;
}

//map<char, map<int, bool> > Peer::getFileChunkMap()
//{
//    return fileChunkMap_;
//}

int 
Peer::connect(){
    // need to protect sendq pointer
    boost::mutex::scoped_lock lock(connectionMutex_);
    // check if connection is still alive
    if (sendq_ != NULL && peers_->connection_->isConnected(sessionId_)){
	cout << "Connection was alive" << endl;
	cout << "Session ID is " << sessionId_ << endl;
	cout << "peerNumber is " << peerNumber_ << endl;
	return errOK;
    }
    // if not, start one
    sendq_ = new ThreadSafeQueue<Frame *>();
    int connCode = peers_->connection_->connect(ipAddress_, port_, 
						&sessionId_, sendq_);
    if (connCode != CONNECTION_OK){
	TRACE("peer.cpp", "Cannot connect to remote peer. DISCONNECTED");
	state_ = DISCONNECTED;
	delete sendq_;
	sendq_ = NULL;
	return errCannotConnectToPeer;
    }
    
    return errOK;
}

void
Peer::disconnect(){
    // need to protect sendq pointer
    boost::mutex::scoped_lock lock(connectionMutex_);
    if (sendq_ != NULL){
	peers_->connection_->endSession(sessionId_);
    }
    // TODO remote all frames from sendq if it is not emmpty
    delete sendq_;
    sendq_ = NULL;
}

void 
Peer::stopConnection(){
    sendq_->stopReading();
}

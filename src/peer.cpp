
#define BOOST_THREAD_USE_LIB
#define MAX_FILE_CHUNKS_REQEST 1



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
        if (p->state_ == Peer::ONLINE){
            p->sendFrame(frame);
        }
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

int
Peers::getOnlinePeerCount()
{
    int onlinePeerCount = 0;
    for (int i = 0; i < maxPeers; i++)
    {
        Peer * p = peers_[i];
        if (p->state_ == Peer::ONLINE)
        {
            onlinePeerCount++;
        }
    }
    return onlinePeerCount;
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
        if ( (strcmp(ip.c_str(), p->getIpAddress().c_str()) == 0)
	     &&  (strcmp(port.c_str(), p->getPort().c_str()) == 0) )
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
    : state_(UNKNOWN), 
      peerNumber_(peerNumber), 
      ipAddress_(ip),
      port_(port),
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

    runDownloadLoop_ = true;
    boost::thread(boost::bind(&Peer::downloadLoop, this));

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
	delete request.frame;
    }
    TRACE("peer.cpp", "Stopping to accept connections.");
    peers_->connection_->stopServer();
    //delete request.frame;
    //delete receiveq_;
}


void
Peer::handleRequest(Request request)
{

    TRACE("peer.cpp", "New Request Received!");

    Frame * frame = request.frame;
    switch (frame->getFrameType())
    {

        case FrameType::HANDSHAKE_REQUEST:
        {
            if (state_ != ONLINE) break;

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
            std::string frameIp = portAndIp_serialization::getIp(request.frame->serializedData);
            std::string framePort = portAndIp_serialization::getPort(request.frame->serializedData);
            // change the status of the peer to connected.
            Peer *p = peers_->getPeerFromIpAndPort(frameIp, framePort);
            if (p != NULL){
                TRACE("peer.cpp", "Received handshake. ONLINE");
                p->state_ = ONLINE;
            }else{
                TRACE("peer.cpp", "Received handshake, but unknown peer");
            }

            if (state_ == INITIALIZING)
            {
                state_ = WAITING_FOR_FILE_LIST;
                TRACE("peer.cpp", "state_ now WAITING_FOR_FILE_LIST")
                Frame * frame = fileListRequestFrame_serialization::createFileListRequest();
                for (int i = 0; i < peers_->getPeerCount(); i++)
                {
                    Peer * peer = (*peers_)[i];
                    if (peer->state_ == ONLINE)
                    {
                        peer->sendFrame(frame);
                    }
                }
            }
        }
        break;

        case FrameType::CHUNK:
        {
            // write the chunk to file

            TRACE("peer.cpp", "Got CHUNK frame.");
            char fileNum = chunkDataFrame_serialization::getFileNum(frame);
            int chunkNum = chunkDataFrame_serialization::getChunkNum(frame);
            char * chunk = chunkDataFrame_serialization::getChunk(frame);

            FileInfo * localFile = fileInfoList_.getFileFromFileNumber(fileNum);

            boost::filesystem::path p(LOCAL_STORAGE_PATH_NAME);
            p /= localFile->fileName;

            // should check some sort of checksum..
            fileChunkIO_->writeChunk(
                    p.string(),
                    chunkNum,
                    chunk,
                    localFile->fileSize);
	    
	    // get a map of chunks downloaded for this file
	    map<int, bool> *chunksDownloaded = 
		&(fileInfoList_.getFileFromFileNumber(fileNum)->chunksDownloaded);
	    (*chunksDownloaded)[chunkNum] = true;
	    numChunksRequested_[fileNum]--;
        }
        break;


        case FrameType::CHUNK_REQUEST:
        {

            TRACE("peer.cpp", "Got CHUNK_REQUEST frame.");

            char fileNum = chunkRequestFrame_serialization::getFileNum(frame);
            int chunkNum = chunkRequestFrame_serialization::getChunkNum(frame);

            // check if file number is valid
            FileInfo * fileInfo = fileInfoList_.getFileFromFileNumber(fileNum);
            if (fileInfo == NULL){
                cout << "Bad file number" << endl;
                break;
            }
	    
            string fileName = fileInfoList_.getFileFromFileNumber(fileNum)->fileName;

            std::cout <<"CHUNK_REQUEST " << "fileName " << fileName << std::endl;
	    
            ThreadSafeQueue<Frame *> * q = peers_->connection_->getReplyQueue(request.requestId);

            if (!fileInfoList_.getFileFromFileNumber(fileNum)->chunksDownloaded[chunkNum])
            {
                std::cout <<"CHUNK_REQUEST DECLINE" << std::endl;
                // we don't have this chunk, so send back a decline
                Frame * f = chunkRequestDecline_serialization::createChunkRequestDeclineFrame(fileNum, chunkNum);
                q->push(f);
                break;
            }

            char chunkBuff[chunkSize];
            boost::filesystem::path p(LOCAL_STORAGE_PATH_NAME);
            p /= fileName;
            if (fileChunkIO_->readChunk(p.string(), chunkNum, chunkBuff) != errFileChunkIOOK)
            {
                // error reading the file..
                TRACE("peer.cpp", "Can't read chunk from file.");
            }
	    
	    std::cout <<"CHUNK_REQUEST " << "got the data from file " << std::endl;

            Frame * chunkDataFrame = chunkDataFrame_serialization::createChunkDataFrame(fileNum, chunkNum, chunkBuff);
            q->push(chunkDataFrame);
            }break;

        case FrameType::FILE_LIST:
            // update local file list
            {
                TRACE("peer.cpp", "Got FILE_LIST frame.");
                handleFileListFrame(frame);
            }
                break;

        case FrameType::FILE_LIST_REQUEST:
        {
            TRACE("peer.cpp", "Got FILE_LIST_REQUEST frame.")
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
	    else{
		// update the peer's chunk list to say that it now has this chunk
		p->fileInfoList_.files[fileNum]->chunksDownloaded[chunkNum] = true;
	    }
        }
        break;

        case FrameType::NEW_FILE_AVAILABLE:
        {
            // update local file list
            TRACE("peer.cpp", "Got NEW_FILE_AVAILABLE frame.")
            FileInfo * newFile = new FileInfo(newFileAvailable_serialization::getFileInfo(frame));
	    //newFile->fileSize = newFile->chunkCount * chunkSize;
	    cout << "File Size: " << newFile->fileSize << endl;
	    cout << "Chunks total: " << newFile->chunkCount << endl;
            fileInfoList_.files.push_back(newFile);
            numChunksRequested_.insert(make_pair(newFile->fileNum, 0));
            TRACE("peer.cpp", "Added new file")
        }break;

        case FrameType::CHUNK_INFO:
        {
            TRACE("peer.cpp", "Got CHUNK_INFO frame.")
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

            if (state_ == WAITING_FOR_CHUNK_INFO && haveChunkInfoForAllFiles())
            {
                changeStateToOnlineAndNotify();
            }

        }break;

        case FrameType::CHUNK_INFO_REQUEST:
        {
            TRACE("peer.cpp", "Got CHUNK_INFO_REQUEST frame.")
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


        case FrameType::PEER_LEAVE_NOTIFICATION:
        {
            TRACE("peer.cpp", "Got PEER_LEAVE_NOTIFICATION frame.")
            // set peer's status to OFFLINE
            std::string frameIp = portAndIp_serialization::getIp(request.frame->serializedData);
            std::string framePort = portAndIp_serialization::getPort(request.frame->serializedData);
            std::cout << frameIp << std::endl;
            std::cout << framePort << std::endl;
            // change the status of the peer to offline
            Peer *p = peers_->getPeerFromIpAndPort(frameIp, framePort);
            if (p != NULL){
                TRACE("peer.cpp", "Received peer leave notification. OFFLINE");
                p->state_ = OFFLINE;
            }else{
                TRACE("peer.cpp", "Received peer leave notification, but unknown peer");
            }

        }break;

        case FrameType::PEER_JOIN_NOTIFICATION:
        {
            std::string frameIp = portAndIp_serialization::getIp(request.frame->serializedData);
            std::string framePort = portAndIp_serialization::getPort(request.frame->serializedData);
            Peer *p = peers_->getPeerFromIpAndPort(frameIp, framePort);
            if (p != NULL){
                TRACE("peer.cpp", "Received PEER_JOIN_NOTIFICATION. Peer now ONLINE");
                p->state_ = ONLINE;
            }else{
                TRACE("peer.cpp", "Received PEER_JOIN_NOTIFICATION, but unknown peer");
            }
        }break;

        default:
            break;

    }
}

void Peer::changeStateToOnlineAndNotify()
{
    state_ = ONLINE;
    TRACE("peer.cpp", "state_ now ONLINE")
    Frame * joinFrame = peerJoinNotification_serialization::createPeerJoinNotification(getIpAddress(), getPort());
    peers_->broadcastFrame(joinFrame, this);
}

void Peer::handleFileListFrame(Frame * fileListFrame)
{
    std::vector<FileInfo> fileInfos = fileListFrame_serialization::getFileInfos(fileListFrame);

    // if there are no files in the system, we're now online
    if (fileInfos.size() == 0)
    {
        changeStateToOnlineAndNotify();
    }

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

        if (state_ == WAITING_FOR_FILE_LIST)
        {
            state_ = WAITING_FOR_CHUNK_INFO;
            TRACE("peer.cpp", "state_ now WAITING_FOR_CHUNK_INFO")
            // for each peer, if we don't have chunk info for a file, request it
            for (int peerIdx = 1; peerIdx < peers_->getPeerCount(); peerIdx++)
            {
                Peer * p = (*peers_)[i];
                if (!p->haveChunkInfo(f.fileNum))
                {
                    Frame * chunkInfoRequest = chunkInfoRequest_serialization::createChunkInfoRequest();
                    p->sendFrame(chunkInfoRequest);
                }
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
        if (fileSize % chunkSize != 0)
        {
            f->chunkCount++;
        }
	f->fileSize = fileSize;
	cout << "File size is " << fileSize << endl;
        f->fileNum = getMaxFileNum() + 1;

        for (int i = 0; i < f->chunkCount; i++)
        {
            f->chunksDownloaded[i] = true;
        }

        fileInfoList_.files.push_back(f);
    }
}

// Insert a file into the system
int Peer::insert(string fileName)
{
    TRACE("peer.cpp", "Inserting new file.");
    filesystem::path pathToNewFile(fileName);
    if (!filesystem::exists(pathToNewFile)){
        cout << fileName << " does not exist!" << endl;
        return errInsertFileNotFound;
    }
    try
    {
        TRACE("peer.cpp", "Found file, attempting to copy.");

        filesystem::path pathToLocalStore(LOCAL_STORAGE_PATH_NAME);
        pathToLocalStore /= pathToNewFile.filename().string();

        filesystem::copy_file(pathToNewFile, pathToLocalStore, filesystem::copy_option::overwrite_if_exists);

        loadLocalFileFromDisk(pathToLocalStore);// adds the file to our file list

    }
    catch (std::exception& e)
    {
        std::cerr << "Problem copying to localStorage: " << e.what() << "\n";
        return errCannotCopyInsertFile;
    }

    TRACE("peer.cpp", "File copied successfully.");

    FileInfo * newFile = fileInfoList_.getFileFromFileName(fileName);

    string foundFileInList = newFile != 0 ? "found new file" : "couldn't find new file by filename!";
    TRACE("peer.cpp:insert", foundFileInList);

    Frame * newFileFrame = newFileAvailable_serialization::createNewFileAvailableFrame(newFile, getIpAddress(), getPort());
    TRACE("peer.cpp:insert", "Created NEW_FILE_AVAILABLE frame.");

    cout << "New file num: " << (int)newFile->fileNum << ", chunkCount: " << newFile->chunkCount << endl;
    peers_->broadcastFrame(newFileFrame, this);

    TRACE("peer.cpp", "Broadcasted NEW_FILE_AVAILABLE frame.");

    return errOK;
}

int
Peer::getChunkCount(char fileNum)
{
    FileInfo * file = fileInfoList_.getFileFromFileNumber(fileNum);
    int totalChunks = 0;

    std::map<int, bool>::iterator chunkIter;
    // loop for each chunk
    for (chunkIter = file->chunksDownloaded.begin(); chunkIter != file->chunksDownloaded.end(); ++chunkIter)
    {
        totalChunks += (int)chunkIter->second;
    }

    return totalChunks;
}

bool
Peer::haveChunkInfo(char fileNum)
{
    return haveChunkInfo_[fileNum];
}

bool
Peer::haveChunkInfoForAllFiles()
{
    vector<FileInfo *>::iterator fileIter = fileInfoList_.files.begin();
    for ( ; fileIter != fileInfoList_.files.end(); fileIter++)
    {
        if (!haveChunkInfo((*fileIter)->fileNum))
            return false;
    }
    return true;
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
    receiveq_->stopReading();

    // check if this peer has chunks that no one else has
    // if so, push those out
    
    // close down server/client sockets

    ///peers_->removePeer(this);
    return errOK;
}

int Peer::join()
{
    if (receiveq_ != NULL){
	// TODO delete every frame
	delete receiveq_;
	receiveq_ = NULL;
    }
    
    peers_->connection_ = new Connection();
    receiveq_ = new ThreadSafeQueue<Request>();

    // init every peer
    for (int i = 0; i < maxPeers; i++){
        (*peers_)[i]->initPeer();
    }

    if (peers_->getOnlinePeerCount() == 0)
    {
        // if we're the only peer, set state to online
        // so we skip trying to ask for file lists/ chunk info
        state_ = ONLINE;
    }

    // for each file in each peer, set a value to say that we don't have chunk info for the file
    for (int i = 0; i < peers_->getPeerCount(); i++)
    {
        Peer * peer = (*peers_)[i];
        for (int fileIdx = 0; (unsigned)i < peer->getFileInfoList().files.size(); fileIdx++)
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
	TRACE("peer.cpp", "Cannot connect to remote peer. OFFLINE");
	state_ = OFFLINE;
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
    state_ = DISCONNECTED;
}

void
Peer::stopDownloadLoop(){
    runDownloadLoop_ = false;
}

void 
Peer::downloadLoop(){
    cout << "downloadLoop()" << endl;
    while (runDownloadLoop_){	
	std::map<char, char>::iterator numChunksIter = numChunksRequested_.begin();
	for ( ; numChunksIter != numChunksRequested_.end(); numChunksIter++){
	    if (numChunksIter->second < MAX_FILE_CHUNKS_REQEST){
       		// request another chunk for this file
		std::vector<FileInfo *>::iterator fileListIter = 
		    fileInfoList_.files.begin();
		// get the chunk info for the curr file
		FileInfo *fileInfo = fileInfoList_.getFileFromFileNumber(numChunksIter->first);
		// iterate chunks downloaded
		std::map<int, bool>::iterator chunksInter = fileInfo->chunksDownloaded.begin();
		bool chunkFound = false;
		for ( ; chunksInter != fileInfo->chunksDownloaded.end(); chunksInter++){
		    if (chunksInter->second == false){
			cout << "Found chunk to download" << endl;
			cout << "file number " << numChunksIter->first << endl;
			cout << "chunk number " << chunksInter->first << endl;
			chunkFound = true;
			break;
		    }
		}
		if (chunkFound){
		    Frame *newFrame = chunkRequestFrame_serialization::
			createChunkRequestFrame(numChunksIter->first, chunksInter->first);
		    (*peers_)[1]->sendFrame(newFrame);
		    numChunksIter->second++;
		}
	    }
	}
    }
}


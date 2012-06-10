//
// File peer.h
//
// This file defines the four public methods of class Peer, together 
// with associated helper classes and constants
//

// This file is provided by the instructor and specifies the public interface of Peer and Status.
// It has been modified to suit our implementation.
// Unfortunately, we didn't have full control over the overall spec of the classes here.

// Various limits


#ifndef PEER_H
#define PEER_H

#define LOCAL_STORAGE_PATH_NAME "./localStorage/"

#include <string>
#include <map>
#include "Cli.h"
#include "Frames.h"
#include "Globals.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"
#include "FileChunkIO.h"

using namespace std;



class Status;
class Peers;
namespace boost{
    class thread;
};

// Peer and Status are the classes we really care about
// Peers is a container; feel free to do a different container

class Peer {

public:
    Peer(int peerNumber, string ip, string port, Peers *peers);
    ~Peer();

    // This is the formal interface and you should follow it
    int insert(string filename);
    int query(Status& status);
    int join();     // Note that we should have the peer list, so it is not needed as a parameter
    int leave();

    int initLocalPeer();
    int initRemotePeer();

    int getPeerNumber();
    string getIpAddress();
    string getPort();

    int broadcastFrame(Frame * message);
    int sendFrame(Frame * frame);

    // Feel free to hack around with the private data, since this is part of your design
    // This is intended to provide some exemplars to help; ignore it if you don't like it.
private:
    int peerNumber_;
    string ipAddress_;
    string port_;
    vector<LocalFileInfo> fileList_;
    map<int, int> peerChunkCount_;
    enum State { CONNECTED, DISCONNECTED, INITIALIZING, UNKNOWN } state_;
    Peers *peers_;
    FileChunkIO * chunkIO;

    ThreadSafeQueue<Frame *> * sendq_;
    ThreadSafeQueue<Request> * receiveq_;

    boost::thread *incomingConnectionsThread_;

    void acceptConnections();
    void handleRequest(Request request);
};

// Peers is a dumb container to hold the peers; the number of peers is fixed,
// but needs to be set up when a peer starts up; feel free to use some other
// container class, but that class must have a method that allows it to read
// the peersFile, since otherwise you have no way of having a calling entity 
// tell your code what the peers are in the system.

class Peers : public CliCmdHandler {
 public:
    Peers(Cli *cli, const string &peersFile);
    ~Peers();

    int initialize(); 
                                      // The peersFile is the name of a file that contains a list of the peers
                                      // Its format is as follows: in plaintext there are up to maxPeers lines,
                                      // where each line is of the form: <IP address> <port number>
                                      // This file should be available on every machine on which a peer is started,
                                      // though you should exit gracefully if it is absent or incorrectly formatted.
                                      // After execution of this method, the _peers should be present.
    Peer * operator[](int i);
    
    // You will likely want to add methods such as visit()
    void handleCmd(vector<string> *cmd);
    void addPeer(Peer * newPeer);
    void removePeer(Peer * peer);
    Peer ** getPeers();
    int getPeerCount();

    Connection *connection_;
 private:

    string peersFile_;
    int numPeers_;
    Peer *peers_[maxPeers];
    int peerCount_;
};


// Status is the class that you populate with status data on the state
// of replication in this peer and its knowledge of the replication
// level within the system.
// The thing required in the Status object is the data as specified in the private section
// The methods shown are examples of methods that we may implement to access such data
// You will need to create methods to populate the Status data.

class Status {
public:
    int numberOfFiles();
    float fractionPresentLocally(int fileNumber);  // Use -1 to indicate if the file requested is not present
    float fractionPresent(int fileNumber);         // Use -1 to indicate if the file requested is not present in the system
    int   minimumReplicationLevel(int fileNumber); // Use -1 to indicate if the file requested is not present in the system
    float averageReplicationLevel(int fileNumber); // Use -1 to indicate if the file requested is not present in the system
    
    void setNumFiles(char numFiles);

    void setLocalFilePresence(char fileNum, float fractionPresentLocally);
    void setSystemFilePresence(char fileNum, float fractionPresentInSystem);

    void setLeastReplication(char fileNum, int totalChunkCountInAllPeers);
    void setWeightedLeastReplication(char fileNum, float totalChunkFractionInAllPeers);

private:
    // This is very cheesy and very lazy, but the focus of this assignment
    // is not on dynamic containers but on the BT p2p file distribution

    int   _numFiles;                               // The number of files currently in the system, as viewed by this peer
    float _local[maxFiles];                        // The fraction of the file present locally 
                                                   // (= chunks on this peer/total number chunks in the file)
    float _system[maxFiles];                       // The fraction of the file present in the system 
                                                   // (= chunks in the system/total number chunks in the file)
                                                   // (Note that if a chunk is present twice, it doesn't get counted twice;
                                                   // this is simply intended to find out if we have the whole file in
                                                   // the system; given that a file must be added at a peer, think about why
                                                   // this number would ever not be 1.)
    int   _leastReplication[maxFiles];             // Sum by chunk over all peers; the minimum of this number is the least 
                                                   // replicated chunk, and thus represents the least level of replication
                                                   // of the file
    float _weightedLeastReplication[maxFiles];     // Sum all chunks in all peers; divde this by the number of chunks in the
                                                   // file; this is the average level of replication of the file
};


// Return codes
// This is a defined set of return codes.
// Using enum here is a bad idea, unless you spell out the values, since it is
// shared with code you don't write.

const int errOK                =  0; // Everything good
const int errUnknownWarning    =  1; // Unknown warning
const int errUnknownFatal      = -2; // Unknown error
const int errCannotConnect     = -3; // Cannot connect to anything; fatal error
const int errNoPeersFound      = -4; // Cannot find any peer (e.g., no peers in a peer file); fatal
const int errPeerNotFound      =  5; // Cannot find some peer; warning, since others may be connectable
const int errPeersFileNotFound = -6; // Cannot find the peers file.
const int errPeersFileReadFail = -7; // Cannot read the peers file.
const int errPeersFileFmtFail  = -7; // The peers file has wrong format.

const int errInsertFileNotFound    = -8; // can't find the file to insert.
const int errCannotCopyInsertFile  = -9; // something went wrong copying the file to be inserted

const int errCannotSendMessage  = -10;
const int errCannotConnectToPeer  = -11;

// please add as necessary

#endif

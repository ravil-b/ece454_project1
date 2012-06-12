/* 
ECE454 Project 1
Ravil Baizhiyenov
Robin Vierich
*/

/*
Entry point of the application.
*/


#define BOOST_THREAD_USE_LIB

#ifdef PLATFORM_WIN
#define sleep(x) Sleep(x) // windows uses a capital S...
#endif


#include <iostream>
#include <boost/thread.hpp>
#include "debug.h"
#include "Cli.h"
//#include "Client.h"
//#include "Server.h"
#include "peer.h"
#include "Frames.h"
#include "Globals.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"
#include "FileChunkIO.h"
#include "vector"
#include <limits>
#include <vector>

#include <stdio.h>

using namespace std;

// CLI Example begin
class Temp : public CliCmdHandler{
public:
    Temp(Cli *cli) : CliCmdHandler(cli) {}
    void handleCmd(std::vector<std::string> *cmd);
};

void
Temp::handleCmd(std::vector<std::string> *cmd){
    std::cout << "handleCmd()" << (*cmd)[0] << std::endl;
}
// CLI Example end

// Connection example begin
int numConsumerCalls = 0;
void
consumer(ThreadSafeQueue<Request> *tq, int num, Connection *c){
    std::cout << "CONSUMER" << num << " begin" << std::endl;
    Request req;
    while(tq->pop(&req)){
	std::cout << "CONSUMER GOT FRAME - " << ('A'+numConsumerCalls) << " ";
	// Make sure the frame is correct
	bool err = false;
	int errCount = 0;
	for(int i = 0; i < FRAME_LENGTH - 1; i++){
	    if (req.frame->serializedData[i] != ('A' + numConsumerCalls)){
		err = true;
		errCount++;
	    }
	}
	if (err){
	    std::cout << "NO ERROR" << std::endl;
	}
	else{
	    std::cout << errCount << " ERRORS!!!" << std::endl;
	}
	std::cout << "IP: " << req.ip << "    port: " << req.port << std::endl;
	delete req.frame;
	numConsumerCalls++;
    }
    std::cout << "CONSUMER" << num << " end" <<std::endl;
}

int numProducerCalls = 0;
void
producer(ThreadSafeQueue<Frame *> *tq, int num){
    sleep(3);
    std::cout << "PRODUCER" << num << " begin" << std::endl;
    for (int i = 0 ; i < 20000000; i++){
	std::cout << "Producer: outputing " << ('A'+numProducerCalls) << std::endl;
	char buff[20];
	Frame *newFrame = new Frame();
	sprintf(buff, "%d", i+(num*20));
	for(int i = 0; i < FRAME_LENGTH - 1; i++){
	    newFrame->serializedData[i] = ('A'+numProducerCalls);
	}
	newFrame->serializedData[FRAME_LENGTH - 1] = 0;
	numProducerCalls++;
	tq->push(newFrame);
	sleep(1);
    }
    tq->stopReading();
    std::cout << "PRODUCER" << num << " end" << std::endl;
}
// Connection example end

void printFrame(Frame * frame, int maxBytesToPrint)
{

    cout << "Printing frame" << endl;
    cout << "Frame Type: " << frame->getFrameType() << endl;
    for (int i = 0; i < maxBytesToPrint; i++)
    {
        cout << i << ": " << (int)frame->serializedData[i] << endl;
    }
}

void
testChunkInfoSerialization()
{
    map<char, map<int, bool> > fileMap;
        map<int, bool> chunkMap1;

        for (int i = 0; i < chunkSize; i++)
        {
            chunkMap1.insert(make_pair(i, (i % 2 == 0))); // create a string of: 1, 0, 1, 0, ...

        }
        fileMap.insert(make_pair(1, chunkMap1));

        map<int, bool> chunkMap2;
        for (int i = 0; i < chunkSize; i++)
        {
            chunkMap2.insert(make_pair(i, (i % 2 != 0))); // create a string of: 0, 1, 0, 1, ...
        }
        fileMap.insert(make_pair(2, chunkMap2));

        map<int, bool> chunkMap3;
        for (int i = 0; i < chunkSize; i++)
        {
            chunkMap3.insert(make_pair(i, (i % 2 == 0))); // create a string of: 1, 0, 1, 0, ...
        }
        fileMap.insert(make_pair(4, chunkMap3));


        Frame * chunkInfoFrame = chunkInfo_serialization::createChunkInfoFrame(3, "localhost", "4444", fileMap);
        map<char, map<int, bool> > deserializedFileMap = chunkInfo_serialization::getChunkMap(chunkInfoFrame);

        cout << "Deserialized Chunk Map. Checking values..." << endl;

        map<int, bool> dsChunkMap1 = deserializedFileMap[1];
        for (int i = 0; i < maxBytesPerChunkInChunkInfo; i++)
        {
            if (dsChunkMap1[i] != (i % 2 == 0)) // should be string of: 1, 0, 1, 0, ...
            {
                cout << "PROBLEM with ds chunk map 1 at i = " << i << endl;
                cout << "Should be " << (int)(i % 2 == 0) << " but is " << dsChunkMap1[i] << endl;
                printFrame(chunkInfoFrame, i);
                break;

            }

        }

        map<int, bool> dsChunkMap2 = deserializedFileMap[2];
        for (int i = 0; i < maxBytesPerChunkInChunkInfo; i++)
        {
            if ((dsChunkMap2[i] == (i % 2 == 0))) // should be string of: 0, 1, 0, 1, ...
            {
                cout << "PROBLEM with ds chunk map 2 at i = " << i << endl;
                cout << "Should be " << (int)(i % 2 != 0) << " but is " << dsChunkMap2[i] << endl;
                break;
            }
        }


        map<int, bool> dsChunkMap3 = deserializedFileMap[4];
        for (int i = 0; i < maxBytesPerChunkInChunkInfo; i++)
        {
            if ((dsChunkMap3[i] != (i % 2 != 0))) // should be string of: 1, 0, 1, 0, ...
            {
                cout << "PROBLEM with ds chunk map 3 at i = " << i << endl;
                cout << "Should be " << (int)(i % 2 == 0) << " but is " << dsChunkMap3[i] << endl;
                break;
            }
        }
}


void
testFileListFrameSerialization()
{

    vector<FileInfo *> files;

    FileInfo file;

    file.fileName = "testFile";
    file.fileNum = 10;
    file.fileSize = 92;
    file.chunkCount = (file.fileSize / chunkSize) + 1;

    files.push_back(&file);

    Frame * fileListFrame = fileListFrame_serialization::createFileListFrame(1, files);
    vector<FileInfo *> dsFileInfos = fileListFrame_serialization::getFileInfos(fileListFrame);

    printFrame(fileListFrame, 30);

    cout << "dsFileInfo - name: " << dsFileInfos[0]->fileName << endl;
    cout <<"num: " << (int)dsFileInfos[0]->fileNum <<
           ", size: " << dsFileInfos[0]->fileSize << endl;

}

int
main(int argc, char* argv[]){
    TRACE("main.cpp", "Starting up.");

    // Initialize Cli and run it in a separate thread.
    TRACE("main.cpp", "Initializing CLI");
    Cli *cli = new Cli();

    // Create the peers container and make it initialize all peers
    TRACE("main.cpp", "Initializing Peers");
    Peers *peers = new Peers(cli, "peers.txt");
    boost::thread peersThread(boost::bind(&Peers::initialize, peers));

    testFileListFrameSerialization();

    TRACE("main.cpp", "Adding CLI Commands");
    Command insertCommand;
    insertCommand.cmdStr = "insert";
    insertCommand.numArgs = 1;
    insertCommand.handler = peers;
    cli->registerCommand(insertCommand);

    Command queryCommand;
    queryCommand.cmdStr = "query";
    queryCommand.numArgs = 0;
    queryCommand.handler = peers;
    cli->registerCommand(queryCommand);

    Command joinCommand;
    joinCommand.cmdStr = "join";
    joinCommand.numArgs = 0;
    joinCommand.handler = peers;
    cli->registerCommand(joinCommand);

    Command leaveCommand;
    leaveCommand.cmdStr = "leave";
    leaveCommand.numArgs = 0;
    leaveCommand.handler = peers;
    cli->registerCommand(leaveCommand);

    boost::thread cliThread(boost::bind(&Cli::run, cli));    

    // Cli::run() returns when the user types "quit" in terminal
    // TODO make sure the client gracefully shutsdown...
    cliThread.join();
    
    return 0;
}

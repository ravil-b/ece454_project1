/*
 * server.h
 *
 *  Created on: 2012-06-05
 *      Author: Robin
 */

#ifndef SERVER_H_
#define SERVER_H_

class Server{
public:
	int Start();

private:
	vector<threadStarts> threads;
	int SpawnNewSocket();
};




#endif /* SERVER_H_ */

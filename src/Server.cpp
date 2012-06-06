/*
 * Server.cpp
 *
 *  Created on: 2012-06-05
 *      Author: Robin
 */

#include <cstdlib>
#include <string>

#include "Server.h"
#include "debug.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

//boost::asio::io_service * ioService_;

Server::Server()
{
    ioService_ = new boost::asio::io_service();
}

Server::~Server()
{
    delete ioService_;
}

int
Server::readRequest(RawRequest * request, SocketPtr socket)
{
    TRACE("Server.cpp", "Reading request.");
    try
    {
        int totalLength = 0;
        std::stringstream stream;

        for (;;)
        {
            char data[MAX_REQUEST_LENGTH];
            boost::system::error_code error;

            size_t length = socket->read_some(boost::asio::buffer(data), error);

            if (error == boost::asio::error::eof)
            {
                break; // Connection closed cleanly by peer.
            }
            else if (error)
            {
                TRACE("Server.cpp", "Read Error");
                TRACE("Server.cpp", error.message());
                return READ_SOCKET_ERROR;
            }
            else
            {
                std::string strData = data;
                stream << strData;
                totalLength += length;
            }



        }

        request->data = stream.str();
        request->messageLength = totalLength;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
	return READ_SOCKET_ERROR;
    }
    return SERVER_OK;
}

int
Server::waitForRequest(RawRequest * request, short port)
{
    TRACE("Server.cpp", "Waiting for requests.");
    //boost::asio::io_service io_service;

    tcp::acceptor acceptor(*ioService_, tcp::endpoint(tcp::v4(), port));
    for (;;)
    {
        SocketPtr sock(new tcp::socket(*ioService_));
        acceptor.accept(*sock);

        TRACE("Server.cpp", "Request received.");

        int err = readRequest(request, sock);

        return err;
    }
}

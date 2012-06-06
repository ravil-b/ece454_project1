/*
 * Server.cpp
 *
 *  Created on: 2012-06-05
 *      Author: Robin
 */

#include <cstdlib>
#include <string>
#include "LocalPeer.h"

#include "Server.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using std::string;
using boost::asio::ip::tcp;

int
Server::readRequest(RawRequest * request, SocketPtr socket)
{
    try
    {
        int lengthCount = 0;
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
                return READ_SOCKET_ERROR;
            }


            std::copy(data + lengthCount,
                      data + lengthCount + length,
                      request->data + lengthCount);

            lengthCount += length;

            boost::asio::write(*socket, boost::asio::buffer(data, length));
        }

        return SERVER_OK;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

int
Server::waitForRequest(RawRequest * request, short port)
{
    boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    for (;;)
    {
        SocketPtr sock(new tcp::socket(io_service));
        acceptor.accept(*sock);

        int err = readRequest(request, sock);
        return err;
    }
}

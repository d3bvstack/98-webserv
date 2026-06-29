/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/29 17:48:14 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sstream>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include "Request.hpp"
#include "Response.hpp"
#include "ClientConnection.hpp"
#include "ListeningSocket.hpp"
#include "Socket.hpp"

/**
 * @brief Checks if the string `filename` ends with the provided `extension`
 *
 * @param filename String to search on
 * @param extension String to search for
 * @return `true` or
 * @return `false`
 */
static bool matchExtension(const std::string& filename, const std::string& extension)
{
    if (filename.length() < extension.length())
        return (false);

    std::string endOfString = filename.substr(filename.length() - extension.length());
    return (endOfString == extension);
}

/**
 * @brief Construct a new Server:: Server object retrieving config files from default location
 *
 */
Server::Server()
{
    DIR* dirStream = opendir(DEFAULT_CONFIG_DIR);
    if (dirStream == NULL)
        throw std::runtime_error("[ERROR] Could not open default config directory " DEFAULT_CONFIG_DIR ".");

    struct dirent* dirEntry;
    while ((dirEntry = readdir(dirStream)) != NULL)
    {
        std::string filename = dirEntry->d_name;

        if (dirEntry->d_type == DT_REG && matchExtension(filename, ".conf"))
        {
            std::string fullpath = std::string(DEFAULT_CONFIG_DIR) + "/" + filename;
            _configurationFiles.push_back(fullpath);
            std::cerr << "[INFO] " << fullpath << " was correctly added as a configuration file." << std::endl;
        }
    }
    closedir(dirStream);

    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");

    std::cerr << "[INFO] Server object created successfully." << std::endl;
}


/**
 * @brief Construct a new Server:: Server object using the arguments as filepaths to configuration files
 *
 * @param argc Number of arguments
 * @param argv Array of arguments
 */
Server::Server(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (matchExtension(argv[i], ".conf"))
        {
            _configurationFiles.push_back(argv[i]);
            std::cerr << "[INFO] " << argv[i] << " was correctly added as a configuration file." << std::endl;
        }
        else
            std::cerr << "[WARNING] " << argv[i] << " is not a .conf file and will be skipped." << std::endl;
    }
    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");

    std::cerr << "[INFO] Server object created successfully." << std::endl;
}

Server::~Server()
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i)
    {
        delete _listeningSockets[i];
    }
    _listeningSockets.clear();

    for (size_t i = 0; i < _clientConnections.size(); ++i)
    {
        delete _clientConnections[i];
    }
    _clientConnections.clear();
}

void Server::verifyConf()
{
    for (std::vector<Vhost>::iterator it = _vhosts.begin(); it != _vhosts.end(); ++it)
    {
        for (std::vector<Vhost>::iterator it2 = _vhosts.end() - 1; it2 > it; --it2)
        {
            if (it->getHost() == it2->getHost() && it->getPort() == it2->getPort())
            {

                _vhosts.erase(it2);

                std::cerr << "[ERROR] Multiple vhosts on same interface, only first will be used." << std::endl;
            }
        }
    }
}


/**
 * @brief Check if port already ha a listening socket bount to it
 *
 * @param host Address to bound to
 * @param port Port to bind to
 * @return `true` Already bound
 * @return `false` Not bound
 */
bool Server::isPortAlreadyBound(const std::string& host, uint16_t port) const
{
    for (std::vector<ListeningSocket*>::const_iterator it = _listeningSockets.begin(); it != _listeningSockets.end(); ++it)
    {
        if ((*it)->getPort() == port && (*it)->getHostPresentation() == host)
            return (true);
    }
    return (false);
}

/**
 * @brief Bind Listening sockets according to configured vhosts
 *
 */
void Server::bindListeningSockets()
{
    for (std::vector<Vhost>::const_iterator it = _vhosts.begin(); it != _vhosts.end(); ++it)
    {
        uint16_t port = (*it).getPort();
        std::string host = (*it).getHost();
        try
        {
            if (isPortAlreadyBound(host, port))
            {
                std::cerr << "[INFO] Port " << port << " is already bound, skipping" << std::endl;
                continue;
            }
            std::cerr << "[INFO] Creating server socket on port " << port << std::endl;
            ListeningSocket *tempSocket = new ListeningSocket(host, port);
            tempSocket->create();
            tempSocket->setReusePort();
            tempSocket->bind();
            _listeningSockets.push_back(tempSocket);
            std::cerr << "[INFO] Server socket on port " << port << " created successfully" << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] Couldn't bind listening socket successfully on port " << port << std::endl;
            std::cerr << "[ERROR] Reason: " << e.what() << std::endl;
        }
    }
    if (_listeningSockets.empty())
        throw std::runtime_error("No port could be bound successfully.");
}

/**
 * @brief Register the listening sockets to be polled for events with the epoll instance
 *
 */
void Server::registerListeningSocketsWithEpoll()
{
    int registered_n = 0;
    for (std::vector<ListeningSocket*>::const_iterator it = _listeningSockets.begin();
        it != _listeningSockets.end(); ++it)
    {
        try
        {
            _epoll.addListeningSocket((*it)->getSocketFd());
            ++registered_n;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] Couldn't register socket with epoll"<< std::endl;
            std::cerr << "[ERROR] Reason: " << e.what() << std::endl;
        }
    }
    if (registered_n == 0)
    {
        throw std::runtime_error("No sockets registered successfully");
    }
}

void Server::startListening()
{
    for(std::vector<ListeningSocket*>::iterator it = _listeningSockets.begin();
        it != _listeningSockets.end(); ++it)
    {
        (*it)->listen();
    }
}

int Server::pollEvents()
{
    return (_epoll.waitWrapper());
}

Socket* Server::socketFromFd(int fd)
{
    for (std::vector<ListeningSocket*>::iterator it = _listeningSockets.begin();
         it != _listeningSockets.end(); ++it)
    {
        if ((*it)->getSocketFd() == fd)
        {
            return (*it);
        }
    }
    for (std::vector<ClientConnection*> ::iterator it = _clientConnections.begin();
        it != _clientConnections.end(); ++it)
    {
        if ((*it)->getSocketFd() == fd)
        {
            return (*it);
        }
    }
    return NULL;
}

void Server::disconnectClient(int clientFd)
{
    std::cerr << "[INFO] Client disconnected on fd " << clientFd << std::endl;

    _epoll.removeSocket(clientFd);
    close(clientFd);

    for (std::vector<ClientConnection*> ::iterator it = _clientConnections.begin();
        it != _clientConnections.end(); ++it)
    {
        if ((*it)->getSocketFd() == clientFd)
        {
            delete *it;
            _clientConnections.erase(it);
            break;
        }
    }
}

void Server::acceptNewConnection(Socket* listenSocket)
{
    int listenFd = listenSocket->getSocketFd();
    // accept4 for setting non blocking
    int newSocketFd = accept4(listenFd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (newSocketFd == -1)
    {
        std::cerr << "[ERROR] Accept failed on fd " << listenFd << std::endl;
        return;
    }

    uint16_t port = 0;
    /// Maybe delete Port from client socket and use only poiner to vhost instead
    for (std::vector<ListeningSocket*>::const_iterator it = _listeningSockets.begin();
         it != _listeningSockets.end(); ++it)
    {
        if ((*it)->getSocketFd() == listenFd)
        {
            port = (*it)->getPort();
            break;
        }
    }


    ClientConnection* newClient = new ClientConnection(newSocketFd, port);
    for (size_t i = 0; i < _vhosts.size(); ++i)
    {
        if (_vhosts[i].getPort() == port)
        {
            newClient->setVhost(&_vhosts[i]);
            break;
        }
    }
    newClient->updateActivity();
    _clientConnections.push_back(newClient);
    _epoll.addClientSocket(newSocketFd);

    std::cerr << "[INFO] New connection established on fd " << newSocketFd
              << " port: " << port << std::endl;
}

ClientConnection* Server::clientFromFd(int clientFd)
{
    for (std::vector<ClientConnection*> ::iterator it = _clientConnections.begin();
        it != _clientConnections.end(); ++it)
    {
        if ((*it)->getSocketFd() == clientFd)
        {
            return (*it);
        }
    }
    return NULL;
}

static std::string decodeChunkedBody(const std::string& body)
{
    std::string decoded;
    size_t pos = 0;

    while (pos < body.length())
    {
        size_t nextLine = body.find("\r\n", pos);
        if (nextLine == std::string::npos)
        {
            break;
        }

        std::string sizeHex = body.substr(pos, nextLine - pos);
        if (sizeHex.empty())
        {
            break;
        }

        unsigned int chunkSize = 0;
        std::stringstream ss;
        ss << std::hex << sizeHex;
        ss >> chunkSize;

        if (chunkSize == 0)
        {
            break;
        }

        pos = nextLine + 2; // Skip \r\n

        if (pos + chunkSize > body.length())
        {
            break;
        }

        decoded.append(body.substr(pos, chunkSize));
        pos += chunkSize + 2; // Skip trailing \r\n
    }

    return (decoded);
}


static bool isValidMethod(const std::string& requestStr)
{
    size_t spacePos = requestStr.find(' ');
    if (spacePos == std::string::npos)
        return (false);

    std::string method = requestStr.substr(0, spacePos);
    return (method == "GET" ||
            method == "POST" ||
            method == "DELETE" ||
            method == "PUT" ||
            method == "HEAD" ||
            method == "OPTIONS" ||
            method == "PATCH" ||
            method == "TRACE" ||
            method == "CONNECT");
}

void Server::handleClientIncomingEvent(int clientFd)
{
    char buffer[4096];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0)
    {
        disconnectClient(clientFd);
        return;
    }

    ClientConnection* client = clientFromFd(clientFd);
    if (client == NULL)
    {
        std::cerr << "[ERROR] Client connection not found for fd " << clientFd << std::endl;
        return;
    }

    client->updateActivity();
    client->appendReadBuffer(buffer, bytesRead);
    const std::string& fullBuffer = client->getReadBuffer();
    size_t headerEndPos = fullBuffer.find("\r\n\r\n");
    if (headerEndPos == std::string::npos)
    {
        if (fullBuffer.length() > 8192)
        {
            std::cerr << "[ERROR] Request headers exceed maximum size on fd " << clientFd << std::endl;
            client->removeFromReadBuffer(std::string::npos);
            Response response = Response::createErrorResponse(431, *(client->getVhost()));
            client->addPendingResponse(response);
            return;
        }
        return;
    }

    std::string headers = fullBuffer.substr(0, headerEndPos);
    std::string body = fullBuffer.substr(headerEndPos + 4); // +4 to skip \r\n\r\n
    bool hasHttpHeader = (headers.find("HTTP/") != std::string::npos);
    if (!isValidMethod(headers) || !hasHttpHeader)
    {
        std::cerr << "[ERROR] Invalid HTTP request on fd " << clientFd << std::endl;
        client->removeFromReadBuffer(std::string::npos);
        Response response = Response::createErrorResponse(400, *(client->getVhost()));
        client->addPendingResponse(response);
        // client->setDisconnect(true);
        // set client connection flag to disconnect when message fully written
        return;
    }

    bool isChunked = (headers.find("Transfer-Encoding: chunked") != std::string::npos);
    bool hasContentLength = (headers.find("Content-Length:") != std::string::npos);

    if (isChunked && hasContentLength)
    {
        std::cerr << "[ERROR] Both Chunked and Content-Length on fd " << clientFd << std::endl;
        client->removeFromReadBuffer(std::string::npos);
        Response response = Response::createErrorResponse(400, *(client->getVhost()));
        client->addPendingResponse(response);
        // client->setDisconnect(true);
        // set client connection flag to disconnect when message fully written
        return;
    }

    else if (isChunked)
    {
        bool isLastChunkReceived = (body.find("0\r\n\r\n") != std::string::npos);
        if (isLastChunkReceived)
        {
            std::cerr << "[INFO] Complete chunked HTTP request received on fd " << clientFd << std::endl;

            std::string unchunkedBody = decodeChunkedBody(body);

            if (unchunkedBody.length() > client->getVhost()->getMaxBodySize())
            {
                std::cerr << "[ERROR] Chunked body exceeds max body size on fd " << clientFd << std::endl;
                client->removeFromReadBuffer(std::string::npos);
                Response response = Response::createErrorResponse(413, *(client->getVhost()));
                client->addPendingResponse(response);
                return;
            }

            std::string cleanHeaders = headers;
            size_t encodingPos = cleanHeaders.find("Transfer-Encoding: chunked");
            if (encodingPos != std::string::npos)
            {
                std::stringstream sstream;
                sstream << "Content-Length: " << unchunkedBody.length();
                cleanHeaders.replace(encodingPos, std::string("Transfer-Encoding: chunked").length(), sstream.str());
            }

            std::string reconstructedRequest = cleanHeaders + "\r\n\r\n" + unchunkedBody;
            Request request(reconstructedRequest);
            request.debugRequest();
            client->addPendingRequest(request);

            size_t chunkEndPos = body.find("0\r\n\r\n");
            client->removeFromReadBuffer(headerEndPos + 4 + chunkEndPos + 5);
        }
        else
        {
            if (body.length() > client->getVhost()->getMaxBodySize() * 2)
            {
                std::cerr << "[ERROR] Chunked body exceeds max body size on fd " << clientFd << std::endl;
                client->removeFromReadBuffer(std::string::npos);
                Response response = Response::createErrorResponse(413, *(client->getVhost()));
                client->addPendingResponse(response);
                return;
            }
            std::cerr << "[INFO] Chunked headers received, waiting for final chunk on fd " << clientFd << std::endl;
        }
    }
    else if (hasContentLength)
    {
        int contentLength = 0;
        size_t pos = headers.find("Content-Length:");
        if (pos != std::string::npos)
        {
            std::stringstream sstream(headers.substr(pos + 15)); // 15 for "Content-Length".length()
            sstream >> contentLength;
        }

        if (static_cast<uint64_t>(contentLength) > client->getVhost()->getMaxBodySize())
        {
            std::cerr << "[ERROR] Request body exceeds max body size on fd " << clientFd << std::endl;
            client->removeFromReadBuffer(std::string::npos);
            Response response = Response::createErrorResponse(413, *(client->getVhost()));
            client->addPendingResponse(response);
            return;
        }

        if (body.length() >= static_cast<size_t>(contentLength))
        {
            std::cerr << "[INFO] Complete Content-Length HTTP request received on fd " << clientFd << std::endl;
            try
            {
                Request request(client->getReadBuffer());
                request.debugRequest();
                client->addPendingRequest(request);
            }
            catch(const std::exception& e)
            {
                std::cerr << "[ERROR] " << e.what() << '\n';
            }

            client->removeFromReadBuffer(headerEndPos + 4 + contentLength);
        }
        else
        {
            std::cerr << "[INFO] Waiting for more body bytes on fd " << clientFd << std::endl;
        }
    }
    else
    {
        std::cerr << "[INFO] Complete HTTP request (no body) received on fd " << clientFd << std::endl;
        try
        {
            Request request(client->getReadBuffer());
            request.debugRequest();
            client->addPendingRequest(request);
            client->removeFromReadBuffer(headerEndPos + 4);
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] " << e.what() << '\n';

            client->removeFromReadBuffer(std::string::npos);
            Response response = Response::createErrorResponse(400, *(client->getVhost()));
            client->addPendingResponse(response);
            // client->setDisconnect(true);
            // set client connection flag to disconnect when message fully written
        }
    }
}

void Server::handleIncomingEvents(int activeEventsCount)
{
    epoll_event* events = _epoll.getEvents();

    for (int i = 0; i < activeEventsCount; ++i)
    {
        int eventFd = events[i].data.fd;
        uint32_t eventTypes = events[i].events;

        Socket* socket = socketFromFd(eventFd);
        if (socket == NULL)
        {
            continue;
        }

        if (eventTypes & (EPOLLERR | EPOLLHUP))
        {
            std::cerr << "[ERROR] Socket error or hangup on fd " << eventFd << std::endl;
            if (socket->getSocketType() == SOCKET_TYPE_CLIENT)
            {
                disconnectClient(eventFd);
            }
            else if (socket->getSocketType() == SOCKET_TYPE_LISTEN)
            {
                close(eventFd);
            }
            continue;
        }

        if (eventTypes & EPOLLIN)
        {
            if (socket->getSocketType() == SOCKET_TYPE_LISTEN)
            {
                acceptNewConnection(socket);
            }
            else if (socket->getSocketType() == SOCKET_TYPE_CLIENT)
            {
                handleClientIncomingEvent(eventFd);
            }
        }
    }
}

void Server::handleClientOutgoingEvent(int clientFd)
{
    ClientConnection* client = clientFromFd(clientFd);
    if (client == NULL)
    {
        std::cerr << "[ERROR] Client connection not found for fd " << clientFd << std::endl;
        return;
    }

    if (client->getWriteBuffer() == NULL)
    {
        const std::vector<Response>& pending = client->getPendingResponses();
        if (pending.empty())
        {
            return ;
        }
        else
        {
            client->setWritePendingBuffer(pending.front());
            client->sendWritePendingBuffer();
            client->removePendingResponse(pending.front());
        }
    }
    else
    {
        client->sendWritePendingBuffer();
    }
    client->updateActivity();
}

void Server::handleOutgoingEvents(int activeEventsCount)
{
    epoll_event* events = _epoll.getEvents();


    for (int i = 0; i < activeEventsCount; ++i)
    {
        int eventFd = events[i].data.fd;
        uint32_t eventTypes = events[i].events;

        Socket* socket = socketFromFd(eventFd);
        if (socket == NULL)
        {
            continue;
        }

        if (eventTypes & EPOLLOUT)
        {
            if (socket->getSocketType() == SOCKET_TYPE_CLIENT)
            {
                handleClientOutgoingEvent(eventFd);
            }
        }
    }
}

void Server::processPendingRequests()
{
    // Iterate throught Client connections
    // if client connection has pending requests
    // {
    //     match request with a location configuration
    //     if request body > location max body size
    //     {
    //         build error 413 and add to pending responses
    //         remove current request from _pendingRequests
    //     }
    //     if method is not allowed on location
    //     {
    //         build error 405 and add to pending responses
    //         remove current request from _pendingRequests
    //     }
    //     substitute path match with location root directory
    //     if new path does not exist
    //     {
    //         remove current request from _pendingRequests
    //         build error 404 and add to pending responses
    //     }
    //     if path is a directory
    //     {
    //         if defaults defined for location
    //         {
    //             while vector of defaults (iterate through defaults)
    //             {
    //                 if file on defaults vector not found
    //                 {

    //                     else
    //                     {
    //                         build error 404 and add to pending responses
    //                         remove current request from _pendingRequests
    //                     }
    //                 }
    //                 else if a file from default vector found
    //                 {
    //                     if file is of CGI mapped extension
    //                     {
    //                         create environment variables from query string and request headers
    //                         create new process and pass new environment

    //                     }
    //                 }
    //             }
    //             if none default found
    //             {
    //             }

    //         }
    //     }
    // }
}

void Server::checkIdleTimeouts()
{
    std::vector<int> idleFds;
    for (std::vector<ClientConnection*>::const_iterator it = _clientConnections.begin();
         it != _clientConnections.end(); ++it)
    {
        ClientConnection* client = *it;
        if (client->getVhost() == NULL)
            continue;
        time_t timeout = static_cast<time_t>(client->getVhost()->getTimeout());
        if (client->isIdle(timeout) && client->getWriteBuffer() == NULL)
            idleFds.push_back(client->getSocketFd());
    }
    for (std::vector<int>::const_iterator it = idleFds.begin(); it != idleFds.end(); ++it)
        disconnectClient(*it);
}

/**
 * @brief Prints the Server object members for debugging
 *
 */
void Server::debugServer() const
{
    int i = 0;
    for (std::vector<std::string>::const_iterator it = _configurationFiles.begin(); it != _configurationFiles.end(); ++it, ++i)
        std::cerr << "[DEBUG][SERVER] Configuration file " << i << ": " + *it << std::endl;
    i = 0;
    for (std::vector<Vhost>::const_iterator it = _vhosts.begin(); it != _vhosts.end(); ++it, ++i)
    {
        std::cerr << "[DEBUG][SERVER] Vhost " << i << ":" << std::endl;
        it->debugVhost();
    }
}

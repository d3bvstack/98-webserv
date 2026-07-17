/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/17 14:44:31 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cctype>
#include <ctime>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include "Request.hpp"
#include "Response.hpp"
#include "ClientConnection.hpp"
#include "ListeningSocket.hpp"
#include "Socket.hpp"
#include "ServerUtils.hpp"
#include "EventTarget.hpp"
#include "CGIContext.hpp"
/**
 * @brief Construct a new Server:: Server object retrieving config files from default location
 *
 */
Server::Server()
{
    DIR* dirStream = opendir(DEFAULT_CONFIG_DIR.c_str());
    if (dirStream == NULL)
        throw std::runtime_error("[ERROR] Could not open default config directory " + DEFAULT_CONFIG_DIR + ".");

    struct dirent* dirEntry;
    while ((dirEntry = readdir(dirStream)) != NULL)
    {
        std::string filename = dirEntry->d_name;

        if (dirEntry->d_type == DT_REG && server_utils::matchExtension(filename, ".conf"))
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
        if (server_utils::matchExtension(argv[i], ".conf"))
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
    for (size_t i = 0; i < _cgiContexts.size(); ++i)
    {
        delete _cgiContexts[i];
    }
    _cgiContexts.clear();

    for (size_t i = 0; i < _clientConnections.size(); ++i)
    {
        delete _clientConnections[i];
    }
    _clientConnections.clear();

    for (size_t i = 0; i < _pendingDeletion.size(); ++i)
    {
        delete _pendingDeletion[i];
    }
    _pendingDeletion.clear();

    for (size_t i = 0; i < _listeningSockets.size(); ++i)
    {
        delete _listeningSockets[i];
    }
    _listeningSockets.clear();
}

/**
 * @brief Checks that only one vhost per interface is defined,
 * else mark configurations after first as error
 *
 */
void Server::verifyConf()
{
    for (size_t i = 0; i < _vhosts.size(); ++i)
    {
        for (size_t j = _vhosts.size() - 1; j > i; --j)
        {
            if (_vhosts[i].getHost() == _vhosts[j].getHost() && _vhosts[i].getPort() == _vhosts[j].getPort())
            {

                _vhosts.erase(_vhosts.begin() + j);

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
            return true;
    }
    return false;
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
        if (isPortAlreadyBound(host, port))
        {
            std::cerr << "[INFO] Port " << port << " is already bound, skipping" << std::endl;
            continue;
        }
        std::cerr << "[INFO] Creating server socket on port " << port << std::endl;
        ListeningSocket *tempSocket = NULL;
        try
        {
            tempSocket = new ListeningSocket(host, port);
            tempSocket->create();
            tempSocket->bind();
            _listeningSockets.push_back(tempSocket);
            std::cerr << "[INFO] Server socket on port " << port << " created successfully" << std::endl;
        }
        catch(const std::exception& e)
        {
            delete tempSocket;
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
            _epoll.addListeningSocket((*it)->getSocketFd(), *it);
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

/**
 * @brief Set all Listening Sockets to begin listening
 *
 */
void Server::startListening()
{
    for(std::vector<ListeningSocket*>::iterator it = _listeningSockets.begin();
        it != _listeningSockets.end(); ++it)
    {
        (*it)->listen();
    }
}

/**
 * @brief Call epoll wait() wrapper
 *
 * @return int Number of events activated
 */
int Server::pollEvents()
{
    return _epoll.waitWrapper();
}

/**
 * @brief Disconnects a ClientConection identified by its fd
 *
 * @param clientFd
 */
void Server::disconnectClient(int clientFd)
{
    std::cerr << "[INFO] Client disconnected on fd " << clientFd << std::endl;

    _epoll.removeFd(clientFd);

    for (std::vector<CGIContext*>::iterator cgiIt = _cgiContexts.begin();
         cgiIt != _cgiContexts.end(); )
    {
        if ((*cgiIt)->getClient() != NULL && (*cgiIt)->getClient()->getSocketFd() == clientFd)
        {
            if ((*cgiIt)->getOutputReadFd() != -1)
                _epoll.removeFd((*cgiIt)->getOutputReadFd());
            if ((*cgiIt)->getInputWriteFd() != -1)
                _epoll.removeFd((*cgiIt)->getInputWriteFd());
            (*cgiIt)->markDisconnected();
            _pendingDeletion.push_back(*cgiIt);
            cgiIt = _cgiContexts.erase(cgiIt);
        }
        else
        {
            ++cgiIt;
        }
    }

    for (std::vector<ClientConnection*> ::iterator it = _clientConnections.begin();
        it != _clientConnections.end(); ++it)
    {
        if ((*it)->getSocketFd() == clientFd)
        {
            (*it)->markDisconnected();
            _pendingDeletion.push_back(*it);
            _clientConnections.erase(it);
            break;
        }
    }
}

/**
 * @brief Accept connections request incomming through a listening socket
 * and create a client conenction object
 *
 * @param listenSocket
 */
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
    try
    {
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
        _epoll.addClientSocket(newSocketFd, newClient);

        std::cerr << "[INFO] New connection established on fd " << newSocketFd
                  << " port: " << port << std::endl;
    }
    catch(const std::exception& e)
    {
        delete newClient;
        std::cerr << "[ERROR] Failed to establish connection on fd " << newSocketFd
                  << ": " << e.what() << std::endl;
    }
}


/**
 * @brief Process a chunk encoded body into a standard unchunked body
 *
 * @param body
 * @return std::string
 */
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

    return decoded;
}


/**
 * @brief Checks whether or not the method on a request line is a valid HTTP method
 *
 * @param requestStr
 * @return true
 * @return false
 */
static bool isValidMethod(const std::string& requestStr)
{
    size_t spacePos = requestStr.find(' ');
    if (spacePos == std::string::npos)
        return false;

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

static uint64_t resolveBodyLimit(const ClientConnection* client, const std::string& headers)
{
    if (client == NULL || client->getVhost() == NULL)
        return (1024 * 1024);

    const Vhost& vhost = *(client->getVhost());
    try
    {
        Request request(headers + "\r\n\r\n");
        const Location* location = server_utils::findBestLocation(vhost, request.getPath());
        if (location != NULL)
            return location->getMaxBodySize();
    }
    catch (const std::exception&)
    {
    }
    return vhost.getMaxBodySize();
}

/**
 * @brief Logic that handles incoming events from ativated fds tht correspond to client connections.
 * It handles reading from the fd, storing on buffer, checking general validity of HTTP request structure,
 * builds a Request object from it and adds to the pending requests queue.
 *
 * @param client
 * @return bool `true` if client has been disconnected, `false` if client not disconnected
 */
bool Server::handleClientIncomingEvent(ClientConnection* client)
{
    int clientFd = client->getSocketFd();
    char buffer[65536];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0)
    {
        disconnectClient(clientFd);
        return true;
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
            response.setConnectionClose();
            client->addPendingResponse(response);
            return false;
        }
        return false;
    }

    std::string headers = fullBuffer.substr(0, headerEndPos);
    size_t bodyOffset = headerEndPos + 4; // skip \r\n\r\n
    size_t bodyLen = (fullBuffer.length() > bodyOffset) ? fullBuffer.length() - bodyOffset : 0;
    bool hasHttpHeader = (headers.find("HTTP/") != std::string::npos);
    if (!isValidMethod(headers) || !hasHttpHeader)
    {
        std::cerr << "[ERROR] Invalid HTTP request on fd " << clientFd << std::endl;
        client->removeFromReadBuffer(std::string::npos);
        Response response = Response::createErrorResponse(400, *(client->getVhost()));
        response.setConnectionClose();
        client->addPendingResponse(response);
        return false;
    }

    bool isChunked = (headers.find("Transfer-Encoding: chunked") != std::string::npos);
    bool hasContentLength = (headers.find("Content-Length:") != std::string::npos);
    uint64_t maxBodySize = resolveBodyLimit(client, headers);

    if (isChunked && hasContentLength)
    {
        std::cerr << "[ERROR] Both Chunked and Content-Length on fd " << clientFd << std::endl;
        client->removeFromReadBuffer(std::string::npos);
        Response response = Response::createErrorResponse(400, *(client->getVhost()));
        response.setConnectionClose();
        client->addPendingResponse(response);
        return false;
    }

    else if (isChunked)
    {
        bool isLastChunkReceived = (fullBuffer.find("0\r\n\r\n", bodyOffset) != std::string::npos);
        if (isLastChunkReceived)
        {
            std::string body = fullBuffer.substr(bodyOffset);
            std::cerr << "[INFO] Complete chunked HTTP request received on fd " << clientFd << std::endl;

            std::string unchunkedBody = decodeChunkedBody(body);

            if (unchunkedBody.length() > maxBodySize)
            {
                std::cerr << "[ERROR] Chunked body exceeds max body size on fd " << clientFd << std::endl;
                client->removeFromReadBuffer(std::string::npos);
                Response response = Response::createErrorResponse(413, *(client->getVhost()));
                response.setConnectionClose();
                client->addPendingResponse(response);
                return false;
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
            client->setKeepAlive(server_utils::hasKeepAlive(headers));

            size_t chunkEndPos = body.find("0\r\n\r\n");
            client->removeFromReadBuffer(headerEndPos + 4 + chunkEndPos + 5);
        }
        else
        {
            if (bodyLen > maxBodySize * 2)
            {
                std::cerr << "[ERROR] Chunked body exceeds max body size on fd " << clientFd << std::endl;
                client->removeFromReadBuffer(std::string::npos);
                Response response = Response::createErrorResponse(413, *(client->getVhost()));
                response.setConnectionClose();
                client->addPendingResponse(response);
                return false;
            }
            std::cerr << "[INFO] Chunked headers received, waiting for final chunk on fd " << clientFd << std::endl;
        }
    }
    else if (hasContentLength)
    {
        long contentLength = 0;
        size_t pos = headers.find("Content-Length:");
        if (pos != std::string::npos)
        {
            pos = headers.find(':', pos);
            if (pos != std::string::npos)
            {
                ++pos;
                pos = headers.find_first_not_of(" \t\r\n", pos);
                if (pos != std::string::npos)
                {
                    std::stringstream sstream(headers.substr(pos));
                    sstream >> contentLength;
                }
            }
        }

        if (static_cast<uint64_t>(contentLength) > maxBodySize)
        {
            std::cerr << "[ERROR] Request body exceeds max body size on fd " << clientFd << std::endl;
            client->removeFromReadBuffer(std::string::npos);
            Response response = Response::createErrorResponse(413, *(client->getVhost()));
            response.setConnectionClose();
            client->addPendingResponse(response);
            return false;
        }

        if (bodyLen >= static_cast<size_t>(contentLength))
        {
            std::cerr << "[INFO] Complete Content-Length HTTP request received on fd " << clientFd << std::endl;
            try
            {
                Request request(client->getReadBuffer());
                request.debugRequest();
                client->addPendingRequest(request);
                client->setKeepAlive(server_utils::hasKeepAlive(headers));
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
            client->setKeepAlive(server_utils::hasKeepAlive(headers));
            client->removeFromReadBuffer(headerEndPos + 4);
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] " << e.what() << '\n';

            client->removeFromReadBuffer(std::string::npos);
            Response response = Response::createErrorResponse(400, *(client->getVhost()));
            response.setConnectionClose();
            client->addPendingResponse(response);
        }
    }
    return false;
}

/**
 * @brief Distributes incoming events based on whether they are
 * from a listening socket or a client connection socket
 *
 * @param activeEventsCount number up to which to iterate through the events array
 */
void Server::handleIncomingEvents(int activeEventsCount)
{
    epoll_event* events = _epoll.getEvents();

    for (int i = 0; i < activeEventsCount; ++i)
    {
        EventTarget* target = static_cast<EventTarget*>(events[i].data.ptr);
        if (target == NULL)
            continue;

        uint32_t eventTypes = events[i].events;

        if (eventTypes & (EPOLLERR | EPOLLHUP))
        {
            if (typeid(*target) == typeid(CGIContext))
            {
                if (target->isDisconnected())
                    continue;
                CGIContext* cgi = static_cast<CGIContext*>(target);

                if (eventTypes & EPOLLERR)
                    cgi->handleError(_epoll);
                else
                    cgi->onCgiOutputReadable(_epoll);
            }
            else if (typeid(*target) == typeid(ClientConnection))
            {
                if (target->isDisconnected())
                    continue;
                ClientConnection* client = static_cast<ClientConnection*>(target);
                std::cerr << "[ERROR] Socket error or hangup on fd " << client->getSocketFd() << std::endl;
                disconnectClient(client->getSocketFd());
            }
            else if (typeid(*target) == typeid(ListeningSocket))
            {
                ListeningSocket* listen = static_cast<ListeningSocket*>(target);
                std::cerr << "[ERROR] Socket error or hangup on fd " << listen->getSocketFd() << std::endl;
                close(listen->getSocketFd());
            }
            continue;
        }

        if (eventTypes & EPOLLIN)
        {
            if (typeid(*target) == typeid(CGIContext))
            {
                if (target->isDisconnected())
                    continue;
                static_cast<CGIContext*>(target)->onCgiOutputReadable(_epoll);
            }
            else if (typeid(*target) == typeid(ListeningSocket))
            {
                acceptNewConnection(static_cast<ListeningSocket*>(target));
            }
            else if (typeid(*target) == typeid(ClientConnection))
            {
                if (target->isDisconnected())
                    continue;
                ClientConnection* client = static_cast<ClientConnection*>(target);
                handleClientIncomingEvent(client);
            }
        }
    }
}

/**
 * @brief Logic that writes to client socket fds the pending responses on client connection
 * closes connection on innactive connections without keep alive
 *
 * @param client
 */
void Server::handleClientOutgoingEvent(ClientConnection* client)
{
    if (client->getWriteBuffer() == NULL)
    {
        const std::vector<Response>& pending = client->getPendingResponses();
        if (!pending.empty())
        {
            const Response& response = pending.front();
            std::cerr << "[INFO] Response sent on fd " << client->getSocketFd()
                      << ": " << response.getVersion() << " "
                      << response.getStatusCode() << " "
                      << response.getReason() << std::endl;
            response.debugResponse();
            client->setWritePendingBuffer(response);
            client->sendWritePendingBuffer();
            client->removePendingResponse(response);
            client->updateActivity();
        }
    }
    else
    {
        client->sendWritePendingBuffer();
        client->updateActivity();
    }
}

/**
 * @brief Logic that distributes to client outgoing events handler
 *
 * @param activeEventsCount
 */
void Server::handleOutgoingEvents(int activeEventsCount)
{
    epoll_event* events = _epoll.getEvents();

    for (int i = 0; i < activeEventsCount; ++i)
    {
        EventTarget* target = static_cast<EventTarget*>(events[i].data.ptr);
        uint32_t eventTypes = events[i].events;

        if (target == NULL)
            continue;

        if (eventTypes & EPOLLOUT)
        {
            if (target->isDisconnected())
                continue;
            if (typeid(*target) == typeid(CGIContext))
            {
                static_cast<CGIContext*>(target)->onCgiInputWritable(_epoll);
            }
            else if (typeid(*target) == typeid(ClientConnection))
            {
                handleClientOutgoingEvent(static_cast<ClientConnection*>(target));
            }
        }
    }
}

void Server::processPendingRequests()
{
    for (std::vector<ClientConnection*>::iterator clientIt = _clientConnections.begin();
         clientIt != _clientConnections.end(); ++clientIt)
    {
		//If no client we just skip
        ClientConnection* client = *clientIt;
        if (client == NULL || client->isDisconnected() || client->getVhost() == NULL)
            continue;

        const std::vector<Request>& pendingRequests = client->getPendingRequests();
        if (pendingRequests.empty())
            continue;

        std::vector<Request> requestsToProcess = pendingRequests;
        for (std::vector<Request>::const_iterator requestIt = requestsToProcess.begin();
             requestIt != requestsToProcess.end(); ++requestIt)
        {
            const Request& request = *requestIt;
            const Vhost& vhost = *(client->getVhost());
            const Location* location = server_utils::findBestLocation(vhost, request.getPath());
            Response response(500);
            bool builtResponse = false;

            try
            {
                if (location == NULL)
                {
                    std::cerr << "[INFO] No location matches" << std::endl;
                    response = server_utils::applyConnectionPolicy(Response::createErrorResponse(404, vhost), client);
                    builtResponse = true;
                }
                else if (!server_utils::requestMethodAllowed(*location, request.getMethod()))
                {
                    std::cerr << "[INFO] Method not allowed on location" << std::endl;
                    response = server_utils::buildMethodNotAllowedResponse(*location, vhost, client);
                    builtResponse = true;
                }
                else if (server_utils::isCgiRequest(vhost, request))
                {
                    std::cerr << "[INFO] CGI Request detected" << std::endl;
                    CGIContext* cgi = new CGIContext(vhost, *location, request, client);
                    if (cgi->start(_epoll))
                    {
                        _cgiContexts.push_back(cgi);
                        client->removePendingRequest(request);
                        continue;
                    }
                    else
                    {
                        int code = cgi->getErrorStatusCode();
                        if (code == 0) code = 500;
                        response = server_utils::applyConnectionPolicy(Response::createErrorResponse(code, vhost), client);
                        builtResponse = true;
                        delete cgi;
                    }
                }
                else if (request.getMethod() == "GET")
                {
                    response = server_utils::buildGetResponse(vhost, *location, request, client);
                    builtResponse = true;
                }
                else if (request.getMethod() == "POST")
                {
                    response = server_utils::buildPostResponse(vhost, *location, request, client);
                    builtResponse = true;
                }
                else if (request.getMethod() == "DELETE")
                {
                    response = server_utils::buildDeleteResponse(vhost, *location, request, client);
                    builtResponse = true;
                }
                else
                {
                    std::cerr << "[INFO] Method unknown" << std::endl;
                    response = server_utils::buildMethodNotAllowedResponse(*location, vhost, client);
                    builtResponse = true;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ERROR] Failed to process request on fd " << client->getSocketFd()
                          << ": " << e.what() << std::endl;
                response = server_utils::applyConnectionPolicy(Response::createErrorResponse(500, vhost), client);
                builtResponse = true;
            }

            if (builtResponse)
            {
                if (!response.isStreamed())
                    client->addPendingResponse(response);
                client->removePendingRequest(request);
            }
        }
    }
}

/**
 * @brief Check Client connections for last time of activity,
 * disconnects client if idle time larger than established timeout
 *
 */
void Server::checkCgiChildren()
{
    for (size_t i = 0; i < _cgiContexts.size(); )
    {
        CGIContext* cgi = _cgiContexts[i];
        cgi->checkChild(_epoll);

        if (cgi->isComplete())
        {
            cgi->deliverResponse();
            cleanupCgiContext(cgi);
        }
        else
        {
            ++i;
        }
    }
}

void Server::cleanupCgiContext(CGIContext* cgi)
{
    if (cgi->getOutputReadFd() != -1)
        _epoll.removeFd(cgi->getOutputReadFd());
    if (cgi->getInputWriteFd() != -1)
        _epoll.removeFd(cgi->getInputWriteFd());

    for (std::vector<CGIContext*>::iterator it = _cgiContexts.begin();
         it != _cgiContexts.end(); ++it)
    {
        if (*it == cgi)
        {
            _cgiContexts.erase(it);
            break;
        }
    }
    delete cgi;
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

        bool hasActiveCgi = false;
        for (std::vector<CGIContext*>::const_iterator cgiIt = _cgiContexts.begin();
             cgiIt != _cgiContexts.end(); ++cgiIt)
        {
            if ((*cgiIt)->getClient() == client)
            {
                hasActiveCgi = true;
                break;
            }
        }
        if (hasActiveCgi)
            continue;

        time_t timeout = static_cast<time_t>(client->getVhost()->getTimeout());
        if (client->isIdle(timeout) && client->getWriteBuffer() == NULL)
            idleFds.push_back(client->getSocketFd());
    }
    for (std::vector<int>::const_iterator it = idleFds.begin(); it != idleFds.end(); ++it)
        disconnectClient(*it);
}

void Server::cleanupPendingDeletions()
{
    for (std::vector<EventTarget*>::iterator it = _pendingDeletion.begin();
         it != _pendingDeletion.end(); ++it)
    {
        delete *it;
    }
    _pendingDeletion.clear();
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

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:08:40 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/25 21:03:37 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientConnection.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Vhost.hpp"

#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>

ClientConnection::ClientConnection(int fd, uint16_t port)
    : Socket(), _port(port), _vhost(NULL), _writePendingOffset(0)
{
    _socketFd = fd;
    _socketType = SOCKET_TYPE_CLIENT;
}

ClientConnection::~ClientConnection()
{
}

void ClientConnection::addPendingRequest(Request request)
{
    request.debugRequest();
    _pendingRequests.push_back(request);
}

void ClientConnection::addPendingResponse(Response request)
{
    _pendingResponses.push_back(request);
}

void ClientConnection::setVhost(Vhost* vhost)
{
    _vhost = vhost;
}

void ClientConnection::appendReadBuffer(const char* buffer, size_t size)
{
    _readBuffer.append(buffer, size);
}

void ClientConnection::removePendingRequest(Request request)
{
    _pendingRequests.erase(std::find(_pendingRequests.begin(), _pendingRequests.end(), request));
}

void ClientConnection::removePendingResponse(Response response)
{
    _pendingResponses.erase(std::find(_pendingResponses.begin(), _pendingResponses.end(), response));
}

const char* ClientConnection::getWriteBuffer() const
{
    if (_writePendingBuffer.empty() || _writePendingOffset >= _writePendingBuffer.size())
        return (NULL);
    return (_writePendingBuffer.data() + _writePendingOffset);
}

void ClientConnection::sendWritePendingBuffer()
{
    if (_writePendingOffset >= _writePendingBuffer.size())
    {
        _writePendingBuffer.clear();
        _writePendingOffset = 0;
        return;
    }
    const char* data = _writePendingBuffer.data() + _writePendingOffset;
    size_t remaining = _writePendingBuffer.size() - _writePendingOffset;
    int bytesSent = send(_socketFd, data, remaining, MSG_NOSIGNAL);
    if (bytesSent > 0)
        _writePendingOffset += bytesSent;
}

void ClientConnection::setWritePendingBuffer(const Response& response)
{
    _writePendingBuffer = response.toString();
    _writePendingOffset = 0;
}

void ClientConnection::removeFromReadBuffer(size_t n)
{
    if (n >= _readBuffer.length())
    {
        _readBuffer.clear();
    }
    else
    {
        _readBuffer.erase(0, n);
    }
}

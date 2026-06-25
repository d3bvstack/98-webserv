/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:08:43 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/25 21:03:19 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/types.h>
#include "Socket.hpp"
#include <string>
#include <stdint.h>
#include <vector>

class Request;
class Response;
class Vhost;

class ClientConnection : public Socket
{
    private:
        uint16_t                _port;
        const Vhost*            _vhost;
        std::string             _readBuffer;
        std::string             _writePendingBuffer;
        size_t                  _writePendingOffset;
        std::vector<Request>    _pendingRequests;
        std::vector<Response>   _pendingResponses;

    public:
        ClientConnection(int fd, uint16_t port);
        ~ClientConnection();

        uint16_t getPort() const { return (_port); };
        const Vhost* getVhost() const { return (_vhost); };
        const std::string& getReadBuffer() const { return (_readBuffer); };
        const std::vector<Request>& getPendingRequests() const { return (_pendingRequests); };
        const std::vector<Response>& getPendingResponses() const { return (_pendingResponses); };
        const char* getWriteBuffer() const;

        void addPendingRequest(Request request);
        void addPendingResponse(Response response);
        void removePendingRequest(Request request);
        void removePendingResponse(Response response);

        void setVhost(Vhost* vhost);
        void setWritePendingBuffer(const Response& response);

        void appendReadBuffer(const char* buffer, size_t size);
        void removeFromReadBuffer(size_t n);

        void sendWritePendingBuffer();
};

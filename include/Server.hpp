/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/10 11:54:32 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "Vhost.hpp"
#include "ConfigParser.hpp"
#include "Epoll.hpp"
#include "EventTarget.hpp"
#include "CGIContext.hpp"

class ClientConnection;
class ListeningSocket;
class Socket;


#ifndef DEFAULT_CONFIG_DIR
# define DEFAULT_CONFIG_DIR "./.conf"
#endif

class Server
{
    private:
        std::vector<std::string>         _configurationFiles;
        std::vector<Vhost>               _vhosts; // Maybe change to map<port, vector<Vhosts>>
        std::vector<ListeningSocket*>    _listeningSockets;
        std::vector<ClientConnection*>   _clientConnections;

        Epoll                            _epoll;
        std::vector<CGIContext*>         _cgiContexts;

        bool isPortAlreadyBound(const std::string& host, uint16_t port) const;
        void acceptNewConnection(Socket* listenSocket);
        void disconnectClient(int clientFd);
        bool handleClientIncomingEvent(ClientConnection* client);
        void handleClientOutgoingEvent(ClientConnection* client);
        void cleanupCgiContext(CGIContext* cgi);

    public:
        Server();
        Server(int argc, char **argv);
        ~Server();

        const std::vector<std::string>& getConfigurationFiles() const   { return (_configurationFiles); };
        const std::vector<Vhost>& getVhosts() const         { return (_vhosts); };

        void setVhosts(const std::vector<Vhost>& vhosts)    { _vhosts = vhosts; }
        void addVhosts(const Vhost& vhost)                  { _vhosts.push_back(vhost); }
        void parseConf()                                    { ConfigParser::parse(this); }
        void verifyConf();
        void bindListeningSockets();
        void registerListeningSocketsWithEpoll();
        void startListening();
        int pollEvents();
        void handleIncomingEvents(int nEvents);
        void handleOutgoingEvents(int nEvents);
        void processPendingRequests();
        void checkCgiChildren();
        void checkIdleTimeouts();

        void debugServer() const;
};

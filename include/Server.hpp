/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/16 17:18:31 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Vhost.hpp"
#include "ConfigParser.hpp"
#include "ListeningSocket.hpp"
#include "Epoll.hpp"


#include <vector>
#include <string>
#include <iostream>
#include <dirent.h>

#ifndef DEFAULT_CONFIG_DIR
# define DEFAULT_CONFIG_DIR "./.conf"
#endif

class Server
{
    private:
        std::vector<std::string>        _configurationFiles;
        std::vector<Vhost>              _vhosts;
        std::vector<ListeningSocket>    _listeningSockets;

        Epoll                           _epoll;

        bool isPortAlreadyBound(std::string host, uint16_t port) const;

    public:
        Server();
        Server(int argc, char **argv);

        const std::vector<std::string>& getConfigurationFiles() const   { return (_configurationFiles); };
        const std::vector<Vhost>& getVhosts() const         { return (_vhosts); };

        void setVhosts(const std::vector<Vhost>& vhosts)    { _vhosts = vhosts; }
        void addVhosts(const Vhost& vhost)                  { _vhosts.push_back(vhost); }
        void parseConf()                                    { ConfigParser::parse(this); }
        void bindListeningSockets();
        void registerListeningSocketsWithEpoll();

        void debugServer() const;
};

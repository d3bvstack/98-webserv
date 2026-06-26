/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Vhost.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 19:31:22 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 11:46:10 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Location.hpp"
#include <string>
#include <sys/types.h>
#include <map>
#include <stdint.h>
#include <iostream>

class Vhost
{
    private:
        std::string                         _serverName;
        std::string                         _host;
        std::pair<bool, uint16_t>           _port;
        std::pair<bool, uint64_t>           _maxBodySize;
        std::map<uint16_t,std::string>      _errorPages;
        std::map<std::string,std::string>   _cgi;

        std::vector<Location>               _locations;

    public:
        Vhost();
        ~Vhost();

        // Getters

        bool isServerNameSet() const    { return !_serverName.empty(); }
        bool isHostSet() const          { return !_host.empty(); }
        bool isPortSet() const          { return _port.first; }
        bool isMaxBodySizeSet() const   { return _maxBodySize.first; }
        bool isErrorPagesSet() const    { return !_errorPages.empty(); }
        bool isCGISet() const           { return !_cgi.empty(); }

        const std::string& getServerName() const                        { return _serverName; }
        const std::string& getHost() const                              { return _host; }
        uint16_t getPort() const                                        { return _port.second; }
        uint64_t getMaxBodySize() const                                 { return _maxBodySize.second; }
        const std::map<uint16_t, std::string>& getErrorPages() const    { return _errorPages; }
        const std::map<std::string, std::string>& getCGI() const        { return _cgi; }

        // Setters

        void setServerName(const std::string& name);
        void setHost(std::string host);
        void setPort(uint32_t port);
        void setMaxBodySize(uint64_t size);
        void addErrorPages(std::string error_page);
        void addCGI(std::string error_page);
        void addLocation(Location location);

        void verify() const;
        void debugVhost() const;

};

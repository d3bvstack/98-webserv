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

#include <stdint.h>
#include <string>
#include <map>
#include <utility>
#include <vector>
#include "Location.hpp"

class Vhost
{
    private:
        std::string                         _serverName;
        std::string                         _host;
        std::pair<bool, uint16_t>           _port;
        std::pair<bool, uint64_t>           _maxBodySize;
        std::pair<bool, uint64_t>           _timeout;
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
        bool isTimeoutSet() const       { return _timeout.first; }
        bool isErrorPagesSet() const    { return !_errorPages.empty(); }
        bool isCGISet() const           { return !_cgi.empty(); }

        const std::string& getServerName() const                        { return _serverName; }
        const std::string& getHost() const                              { return _host; }
        uint16_t getPort() const                                        { return _port.second; }
        uint64_t getMaxBodySize() const                                 { return _maxBodySize.second; }
        uint64_t getTimeout() const                                      { return _timeout.second; }
        const std::map<uint16_t, std::string>& getErrorPages() const    { return _errorPages; }
        const std::map<std::string, std::string>& getCGI() const        { return _cgi; }
        const std::vector<Location>& getLocations() const               { return _locations; }

        // Setters

        void setServerName(const std::string& name);
        void setHost(std::string host);
        void setPort(uint32_t port);
        void setMaxBodySize(uint64_t size);
        void setTimeout(uint64_t seconds);
        void addErrorPages(std::string error_page);
        void addCGI(std::string error_page);
        void addLocation(Location location);

        void verify() const;
        void debugVhost() const;

};

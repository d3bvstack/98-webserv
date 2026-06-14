/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Vhost.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 19:31:22 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/14 12:03:14 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/types.h>
#include <map>
#include <iostream>

class Vhost
{
    private:
        std::string                         _serverName;
        std::pair<bool, u_int32_t>          _host;
        std::pair<bool, u_int16_t>          _port;
        std::pair<bool, u_int64_t>          _maxBodySize;
        std::map<u_int16_t,std::string>     _errorPages;
        std::map<std::string,std::string>   _cgi;

    public:
        Vhost();
        ~Vhost();

        bool isServerNameSet() const    { return !_serverName.empty(); }
        bool isHostSet() const          { return _host.first; }
        bool isPortSet() const          { return _port.first; }
        bool isMaxBodySizeSet() const   { return _maxBodySize.first; }
        bool isErrorPagesSet() const    { return !_errorPages.empty(); }
        bool isCGISet() const           { return !_cgi.empty(); }

        const std::string& getServerName() const                        { return _serverName; }
        const u_int32_t& getHost() const                                { return _host.second; }
        const u_int16_t& getPort() const                                { return _port.second; }
        const u_int64_t& getMaxBodySize() const                         { return _maxBodySize.second; }
        const std::map<u_int16_t, std::string>& getErrorPages() const   { return _errorPages; }
        const std::map<std::string, std::string>& getCGI() const        { return _cgi; }

        void setServerName(const std::string& name)            { _serverName = name; }
        void setHost(u_int32_t host)                           { _host = std::make_pair(true, host); }
        void setPort(u_int16_t port)                           { _port = std::make_pair(true, port); }
        void setMaxBodySize(u_int64_t size)                    { _maxBodySize = std::make_pair(true, size); }
        void addErrorPages(u_int16_t code, std::string path)   { _errorPages[code] = path; }
        void addCGI(std::string cgi, std::string path)         { _cgi[cgi] = path; }

        void debugVhost() const;

};

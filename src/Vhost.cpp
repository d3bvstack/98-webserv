/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Vhost.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 19:31:22 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/15 00:08:44 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Vhost.hpp"
#include <stdint.h>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

static std::vector<std::string> splitBySpace(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(input);

    while (ss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

Vhost::Vhost()
:   _serverNames(),
    _host(""),
    _port(false, 0),
    _maxBodySize(false, 1024 *1024),
    _timeout(false, 60),
    _errorPages(),
    _cgi()
{}

Vhost::~Vhost()
{}

void Vhost::setServerName(const std::string& names)
{
    if (names.empty())
    {
        throw std::invalid_argument("Server name cannot be empty.");
    }

    std::vector<std::string> tokens = splitBySpace(names);
    for (std::vector<std::string>::const_iterator it = tokens.begin();
        it != tokens.end(); ++it)
    {
        _serverNames.push_back(*it);
    }
}

void Vhost::setHost(std::string host)
{
    if (isHostSet())
    {
        throw std::logic_error("Host is already set.");
    }
    if (host.empty())
    {
        throw std::invalid_argument("Host cannot be empty.");
    }
    _host = host;
}

void Vhost::setPort(uint32_t port)
{
    if (isPortSet())
    {
        throw std::logic_error("Port is already set.");
    }
    if (port > 65535)
    {
        throw std::logic_error("Port out of range (0 - 65535).");
    }
    _port = std::make_pair(true, port);
}

void Vhost::setMaxBodySize(uint64_t size)
{
    if (isMaxBodySizeSet())
    {
        throw std::logic_error("Max body size is already set.");
    }
    _maxBodySize = std::make_pair(true, size);
}

void Vhost::setTimeout(uint64_t seconds)
{
    if (isTimeoutSet())
    {
        throw std::logic_error("Timeout is already set.");
    }
    _timeout = std::make_pair(true, seconds);
}

void Vhost::addErrorPages(std::string error_page)
{
    std::vector<std::string> tokens = splitBySpace(error_page);

    if (tokens.size() != 2)
    {
        throw std::runtime_error("Wrong format, expected 'error_pages = code path/to/file'");
    }

    char* endptr = NULL;
    unsigned long parsedCode = std::strtoul(tokens[0].c_str(), &endptr, 10);
    if (*endptr != '\0')
    {
        throw std::runtime_error("Invalid error page code, expected numeric value.");
    }
    uint16_t errorCode = static_cast<uint16_t>(parsedCode);
    _errorPages[errorCode] = tokens[1];
}

void Vhost::addCGI(std::string cgi)
{
    std::vector<std::string> tokens = splitBySpace(cgi);

    if (tokens.size() != 2 && tokens.size() != 1)
    {
        throw std::runtime_error("Wrong format, expected 'cgi = .ext /path/to/interpreter' or 'cgi = .ext'");
    }

    if (tokens.size() == 1)
        _cgi[tokens[0]] = "";
    else
        _cgi[tokens[0]] = tokens[1];
}

void Vhost::addLocation(Location location)
{
    _locations.push_back(location);
}

void Vhost::verify() const
{
    if (!isServerNameSet())
        throw std::runtime_error("Missing server name");
    if (!isHostSet())
        throw std::runtime_error("Missing host");
    if (!isPortSet())
        throw std::runtime_error("Missing port");
}

void Vhost::debugVhost() const
{
    std::cerr << "  Server Names: ";
    if (isServerNameSet())
    {
        for (std::vector<std::string>::const_iterator it = _serverNames.begin();
            it != _serverNames.end(); ++it)
        {
            if (it != _serverNames.begin())
                std::cerr << ", ";
            std::cerr << *it;
        }
    }
    else
    {
        std::cerr << "NOT SET";
    }
    std::cerr << std::endl;

    std::cerr << "  Host: ";
    if (isHostSet())
        std::cerr << getHost() << std::endl;
    else
        std::cerr << "NOT SET" << std::endl;

    std::cerr << "  Port: ";
    if (isPortSet())
        std::cerr << getPort() << std::endl;
    else
        std::cerr << "NOT SET" << std::endl;

    std::cerr << "  MaxBodySize: ";
    if (isMaxBodySizeSet())
        std::cerr << getMaxBodySize() << " bytes" << std::endl;
    else
        std::cerr << "NOT SET" << std::endl;

    std::cerr << "  KeepAliveTimeout: ";
    if (isTimeoutSet())
        std::cerr << getTimeout() << " seconds" << std::endl;
    else
        std::cerr << "60 seconds" << std::endl;

    std::cerr << "  Error Pages: ";
    if (isErrorPagesSet())
    {
        for (std::map<uint16_t,std::string>::const_iterator it = _errorPages.begin();
            it != _errorPages.end(); ++it)
        {
            std::cerr << it->first << " --> " << it->second << "; ";
        }
    }
    else
    {
        std::cerr << "NONE";
    }
    std::cerr << std::endl;

    std::cerr << "  CGI Parameters: ";
    if (isCGISet())
    {
        for (std::map<std::string,std::string>::const_iterator it = _cgi.begin();
            it != _cgi.end(); ++it)
        {
            std::cerr << it->first << "=" << it->second << "; ";
        }
    }
    else
    {
        std::cerr << "NONE";
    }
    std::cerr << std::endl;

    for (std::vector<Location>::const_iterator it = _locations.begin();
        it != _locations.end(); ++it)
    {
        (*it).debugLocation();
    }
}

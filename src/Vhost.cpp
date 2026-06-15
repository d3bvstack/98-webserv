/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Vhost.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 19:31:22 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/15 22:20:37 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Vhost.hpp"
#include <vector>
#include <sstream>
#include <cstdlib>


Vhost::Vhost()
:   _serverName(""),
    _host(""),
    _port(false, 0),
    _maxBodySize(false, 1024 *1024),
    _errorPages(),
    _cgi()
{}

Vhost::~Vhost()
{}

void Vhost::setServerName(const std::string& name) 
{
    if (isServerNameSet()) 
    {
        throw std::logic_error("Server name is already set.");
    }
    if (name.empty()) 
    {
        throw std::invalid_argument("Server name cannot be empty.");
    }
    _serverName = name;
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

void Vhost::setPort(u_int32_t port)
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

void Vhost::setMaxBodySize(u_int64_t size)
{
    if (isMaxBodySizeSet()) 
    {
        throw std::logic_error("Max body size is already set.");
    }
    _maxBodySize = std::make_pair(true, size);
}

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

void Vhost::addErrorPages(std::string error_page)
{
    std::vector<std::string> tokens = splitBySpace(error_page);

    if (tokens.size() != 2)
    {
        throw std::runtime_error("Wrong format, expected 'error_pages = code path/to/file'");
    }

    u_int16_t errorCode = static_cast<u_int16_t>(std::atoi(tokens[0].c_str()));
    _errorPages[errorCode] = tokens[1];
}

void Vhost::addCGI(std::string cgi)
{
    std::vector<std::string> tokens = splitBySpace(cgi);

    if (tokens.size() != 2)
    {
        throw std::runtime_error("Wrong format, expected 'error_pages = code path/to/file'");
    }

    _cgi[tokens[0]] = tokens[1];
}

void Vhost::addLocation(Location location)
{
    _locations.push_back(location);
}

void Vhost::debugVhost() const 
{
    std::cerr << "  Server Name: "
              << (isServerNameSet() ? getServerName() : "NOT SET")
              << std::endl;

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

    std::cerr << "  Error Pages: ";
    if (isErrorPagesSet()) 
    {
        for (std::map<u_int16_t,std::string>::const_iterator it = _errorPages.begin(); 
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
}
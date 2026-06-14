/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Vhost.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 19:31:22 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/14 11:39:53 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Vhost.hpp"

Vhost::Vhost()
:   _serverName(""),
    _host(false, 0),
    _port(false, 0),
    _maxBodySize(false, 1024 *1024),
    _errorPages(),
    _cgi()
{}

Vhost::~Vhost()
{}

void Vhost::debugVhost() const 
{
    std::cout << "Server Name: "
              << (isServerNameSet() ? getServerName() : "NOT SET")
              << std::endl;

    std::cout << "Host: ";
    if (isHostSet()) 
        std::cout << getHost() << std::endl;
    else 
        std::cout << "NOT SET" << std::endl;

    std::cout << "Port: ";
    if (isPortSet()) 
        std::cout << getPort() << std::endl;
    else 
        std::cout << "NOT SET" << std::endl;

    std::cout << "MaxBodySize: ";
    if (isMaxBodySizeSet()) 
        std::cout << getMaxBodySize() << " bytes";
    else 
        std::cout << "NOT SET" << std::endl;

    std::cout << "Error Pages: ";
    if (isErrorPagesSet()) 
    {
        for (std::map<u_int16_t,std::string>::const_iterator it = _errorPages.begin(); 
            it != _errorPages.end(); ++it) 
        {
            std::cout << it->first << " --> " << it->second << "; ";
        }
    } 
    else {
        std::cout << "NONE";
    }
    std::cout << std::endl;

    std::cout << "CGI Parameters: ";
    if (isCGISet())
    {
        for (std::map<std::string,std::string>::const_iterator it = _cgi.begin(); 
            it != _cgi.end(); ++it) 
        {
            std::cout << it->first << "=" << it->second << "; ";
        }
    } 
    else 
    {
        std::cout << "NONE";
    }
    std::cout << std::endl;
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:06:03 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/15 23:14:02 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ListeningSocket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <exception>
#include <iostream>
#include <stdint.h>
#include <cstring>

ListeningSocket::ListeningSocket(std::string host, uint16_t port)
	: _port(port), _hostStr(host), _hostNum(0)
{
	_socketType = SOCKET_TYPE_LISTEN;

	struct in_addr net_bytes;

    if (inet_pton(AF_INET, _hostStr.c_str(), &net_bytes) != 1)
	{
        throw std::runtime_error("Invalid IP address configuration: " + _hostStr);
    }
    _hostNum = ntohl(net_bytes.s_addr);
}

ListeningSocket::~ListeningSocket()
{
}

void ListeningSocket::create()
{
	_socketFd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (_socketFd == -1)
	{
		throw std::runtime_error("Failed to create socket");
	}
}

void ListeningSocket::setReusePort()
{
	if (_socketFd == -1)
	{
		throw std::runtime_error("Socket not created, when trying to set SO_REUSEPORT.");
	}

	int optval = 1;
	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
	{
		throw std::runtime_error("Failed to set SO_REUSEPORT");
	}
	std::cerr << "[INFO] Socket fd " << _socketFd << " SO_REUSEPORT option set" << std::endl;
}

void ListeningSocket::bind()
{
	if (_socketFd == -1)
	{
		throw std::runtime_error("Socket not created, when trying to bind to port");
	}

	struct sockaddr_in server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = htonl(_hostNum);

	if (::bind(_socketFd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		throw std::runtime_error("Failed to bind socket");
	}
	std::cerr << "[INFO] Socket fd " << _socketFd << " bound to port " << _port << std::endl;
}

void ListeningSocket::listen()
{
	if (_socketFd == -1)
	{
		throw std::runtime_error("Tying to listen of non fd.");
	}

	if (::listen(_socketFd, LISTEN_BACKLOG) == -1)
	{
		throw std::runtime_error("Failed to listen on a socket");
	}
	std::cerr << "[INFO] Socket fd " << _socketFd << " listening on port " << _port << std::endl;
}

uint16_t ListeningSocket::getPort() const
{
	return (_port);
}

uint32_t ListeningSocket::getHostNum() const
{
	return (_hostNum);
}

std::string ListeningSocket::getHostPresentation() const
{
	return (_hostStr);
}

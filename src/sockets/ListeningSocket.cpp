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
#include <stdint.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <stdexcept>

ListeningSocket::ListeningSocket(std::string host, uint16_t port)
	: _port(port),
	_hostStr(host),
	_hostNum(0)
{
	_socketType = SOCKET_TYPE_LISTEN;

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream portStr;
	portStr << port;

	struct addrinfo* result;
	int gai_err = getaddrinfo(host.c_str(), portStr.str().c_str(), &hints, &result);
	if (gai_err != 0)
	{
		throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(gai_err));
	}

	std::memcpy(&_addr, result->ai_addr, result->ai_addrlen);
	_hostNum = ntohl(_addr.sin_addr.s_addr);
	freeaddrinfo(result);
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
	int opt = 1;
	setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void ListeningSocket::bind()
{
	if (_socketFd == -1)
	{
		throw std::runtime_error("Socket not created, when trying to bind to port");
	}

	if (::bind(_socketFd, (struct sockaddr*)&_addr, sizeof(_addr)) == -1)
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
	return _port;
}

uint32_t ListeningSocket::getHostNum() const
{
	return _hostNum;
}

std::string ListeningSocket::getHostPresentation() const
{
	return _hostStr;
}

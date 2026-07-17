/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:30:00 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/17 14:23:10 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Socket.hpp"
#include <unistd.h>
#include <sys/socket.h>

Socket::Socket()
    : _socketFd(-1), _socketType(-1)
{
}

Socket::~Socket()
{
    if (_socketFd != -1)
    {
        shutdown(_socketFd, SHUT_RDWR);
        close(_socketFd);
        _socketFd = -1;
    }
}

int Socket::getSocketFd() const
{
    return _socketFd;
}

int Socket::getSocketType() const
{
    return _socketType;
}

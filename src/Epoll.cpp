/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:13:15 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:11:13 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>

Epoll::Epoll()
    : _epollFd(-1), _events()
{
    _epollFd = epoll_create1(EPOLL_CLOEXEC); // CLOEXEC closes fd on exec (for forks)
    if (_epollFd == -1)
    {
        throw std::runtime_error("[ERROR] Failed to create epoll instance");
    }
    std::cerr << "[INFO] Epoll instance created successfully with fd: " << _epollFd << std::endl;
}

Epoll::~Epoll()
{
    if (_epollFd != -1)
    {
        close(_epollFd);
        std::cerr << "[INFO] Epoll instance destroyed" << std::endl;
        _epollFd = -1;
    }
}

void Epoll::addSocket(uint32_t fd)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP; // EPOLLRDHUP triggers when client closes connection
    event.data.fd = fd;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        std::stringstream out;
        out << "Failed to add socket to epoll: " << errno
            << " - " << strerror(errno) << std::endl;
        throw std::runtime_error(out.str());
    }
    std::cerr << "[INFO] Socket with fd " << fd << " added to epoll" << std::endl;
}

void Epoll::removeSocket(uint32_t fd)
{
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        std::cerr << "[ERROR] Failed to remove fd " << fd
                  << " from epoll: " << strerror(errno) << std::endl;
    }
    else
    {
        std::cerr << "[INFO] Successfully removed fd " << fd << " from epoll" << std::endl;
    }
}

int Epoll::waitWrapper()
{
    int numEvents = epoll_wait(_epollFd, _events, EPOLL_MAX_EVENTS, 0);
    if (numEvents < 0)
    {
        throw std::runtime_error(" epoll_wait failed");
    }
    // std::cerr << "[INFO] epoll.wait() has " << numEvents << " fds with events" << std::endl;
    return (numEvents);
}

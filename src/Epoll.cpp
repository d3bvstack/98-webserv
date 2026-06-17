/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:13:15 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/17 10:30:36 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"

#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sstream>

Epoll::Epoll()
    : _epoll_fd(-1), _events()
{
    _epoll_fd = epoll_create1(EPOLL_CLOEXEC); // CLOEXEC closes fd on exec (for forks)
    if (_epoll_fd == -1)
    {
        throw std::runtime_error("[ERROR] Failed to create epoll instance");
    }
    std::cerr << "[INFO] Epoll instance created successfully with fd: " << _epoll_fd << std::endl;
}

Epoll::~Epoll()
{
    if (_epoll_fd != -1)
    {
        close(_epoll_fd);
        std::cerr << "[INFO] Epoll instance destroyed" << std::endl;
        _epoll_fd = -1;
    }
}

void Epoll::addSocket(u_int32_t fd)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP; // EPOLLRDHUP triggers when client closes connection
    event.data.fd = fd;

    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        std::stringstream out;
        out << "Failed to add socket to epoll: " << errno
            << " - " << strerror(errno) << std::endl;
        throw std::runtime_error(out.str());
    }
    std::cerr << "[INFO] Socket with fd " << fd << " added to epoll" << std::endl;
}

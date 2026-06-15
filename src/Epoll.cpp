/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:13:15 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/15 23:42:33 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"

#include <unistd.h>
#include <stdexcept>
#include <iostream>


Epoll::Epoll()
    : _epoll_fd(-1)
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

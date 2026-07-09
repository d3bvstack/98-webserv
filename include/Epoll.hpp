/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:10:01 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/09 19:00:46 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/epoll.h>
#include <stdint.h>

#ifndef EPOLL_MAX_EVENTS
# define EPOLL_MAX_EVENTS 1024
#endif

class Epoll
{

    private:
        int         _epollFd;
        epoll_event _events[EPOLL_MAX_EVENTS];

    public:
        Epoll();
        ~Epoll();

        void addListeningSocket(uint32_t fd, void* ctx);
        void addClientSocket(uint32_t fd, void* ctx);
        void removeFd(uint32_t fd);
        void addFd(uint32_t fd, uint32_t events, void* ctx);
        void modifyFd(uint32_t fd, uint32_t events, void* ctx);
        int waitWrapper();

        epoll_event *getEvents()    { return (_events); }
};

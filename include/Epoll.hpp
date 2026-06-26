/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:10:01 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:33:23 by dbarba-v         ###   ########.fr       */
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

        void addSocket(uint32_t fd);
        void removeSocket(uint32_t fd);
        int waitWrapper();

        epoll_event *getEvents()    { return (_events); }
};

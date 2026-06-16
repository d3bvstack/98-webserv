/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:10:01 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/16 17:15:23 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/epoll.h>

class Epoll
{
    #ifndef EPOLL_MAX_EVENTS
    # define EPOLL_MAX_EVENTS 1024
    #endif

    private:
        int         _epoll_fd;
        epoll_event _events[EPOLL_MAX_EVENTS];

    public:
        Epoll();
        ~Epoll();

        void addSocket(u_int32_t fd);

};

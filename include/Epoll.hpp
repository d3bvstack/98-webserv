/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:10:01 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/15 23:42:39 by dbarba-v         ###   ########.fr       */
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
        int _epoll_fd; 

    public:
        Epoll();
        ~Epoll();

        epoll_event events[EPOLL_MAX_EVENTS];
};

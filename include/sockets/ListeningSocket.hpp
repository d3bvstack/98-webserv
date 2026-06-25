/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:06:00 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/23 15:27:04 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include "Socket.hpp"

#ifndef LISTEN_BACKLOG
# define LISTEN_BACKLOG 265
#endif

class ListeningSocket : public Socket
{
    private:
        uint16_t _port;
        std::string _hostStr;
        u_int32_t _hostNum;

    public:
        ListeningSocket(std::string host, uint16_t port);
        ~ListeningSocket();

        std::string getHostPresentation() const;
        u_int32_t getHostNum() const;
        uint16_t getPort() const;

        void create();
        void setReusePort();
        void bind();
        void listen();
};

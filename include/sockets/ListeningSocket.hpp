/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:06:00 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:11:13 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <stdint.h>
#include <string>
#include <netinet/in.h>
#include "Socket.hpp"

#ifndef LISTEN_BACKLOG
# define LISTEN_BACKLOG 265
#endif

class ListeningSocket : public Socket
{
    private:
        uint16_t _port;
        std::string _hostStr;
        uint32_t _hostNum;
        struct sockaddr_in _addr;

    public:
        ListeningSocket(std::string host, uint16_t port);
        ~ListeningSocket();

        std::string getHostPresentation() const;
        uint32_t getHostNum() const;
        uint16_t getPort() const;

        void create();
        void setReusePort();
        void bind();
        void listen();
};

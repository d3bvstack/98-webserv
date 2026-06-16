/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ListeningSocket.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 23:06:00 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/16 19:41:52 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/types.h>

class ListeningSocket
{
    private:
        int _socketFd;
        u_int16_t _port;
        std::string _hostStr;
        u_int32_t _hostNum;

    public:
        ListeningSocket(std::string host, u_int16_t port);
        ~ListeningSocket();

        int getSocketFd() const;
        std::string getHostPresentation() const;
        u_int32_t getHostNum() const;
        u_int16_t getPort() const;

        void create();
        void setReusePort();
        void setNonBlocking();
        void bind();

};

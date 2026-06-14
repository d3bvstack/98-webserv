/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/12 00:15:59 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Vhost.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <dirent.h>

#ifndef DEFAULT_CONFIG_DIR
# define DEFAULT_CONFIG_DIR "./.conf"
#endif

class Server
{
    private:
        std::vector<std::string>    _configurationFiles;
        std::vector<Vhost>          _vhosts;
    
    public:
        Server();
        Server(int argc, char **argv);

        const std::vector<std::string>& getConfigurationFiles() const   { return (_configurationFiles); };
        const std::vector<Vhost>&       getVhosts() const               { return (_vhosts); };

        void setVhosts(const std::vector<Vhost>& vhosts) { _vhosts = vhosts; }
        void debugServer() const;
};

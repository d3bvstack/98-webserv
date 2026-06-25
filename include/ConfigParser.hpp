/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 12:01:24 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/23 11:36:05 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>

class Server;
class Vhost;
class Location;

class ConfigParser
{
    private:
        ConfigParser();

        typedef void (*StateFunc)(Server* server, const std::string&);

        static StateFunc       _state;
        static Vhost           _currentVhost;
        static Location        _currentLocation;

        template <typename T>
        static T stringToNum(const std::string& str);
        static std::string trim(const std::string& str);
        static void parseKeyValue(const std::string& line, std::string& key, std::string& value);

        // States

        static void parseGlobal(Server* server, const std::string& line);
        static void parseVhost(Server* server, const std::string& line);
        static void parseLocation(Server* server, const std::string& line);


    public:
        ~ConfigParser();
        static void parse(Server* server);

};

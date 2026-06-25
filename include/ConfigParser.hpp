/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 12:01:24 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:32:00 by dbarba-v         ###   ########.fr       */
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

        typedef void (*StateFuncPtr)(Server* server, const std::string&);

        static StateFuncPtr       _state;
        static Vhost           _currentVhost;
        static Location        _currentLocation;

        template <typename T>
        static T stringToNum(const std::string& str);
        static std::string trimAndStripComments(const std::string& str);
        static void parseKeyValue(const std::string& line, std::string& key, std::string& value);

        // States

        static void parseGlobal(Server* server, const std::string& line);
        static void parseVhost(Server* server, const std::string& line);
        static void parseLocation(Server* server, const std::string& line);


    public:
        ~ConfigParser();
        static void parse(Server* server);

};

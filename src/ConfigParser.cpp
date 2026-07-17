/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/14 12:31:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/17 14:29:35 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <stddef.h>
#include <stdint.h>
#include <sstream>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "Server.hpp"
#include "Vhost.hpp"
#include "Location.hpp"

std::string ConfigParser::trimAndStripComments(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    if (str[start] == ';')
        return "";

    const std::string *source = &str;
    std::string temp;
    size_t comment_start = str.find(';', start);
    if (comment_start != std::string::npos)
    {
        temp = str.substr(0, comment_start);
        source = &temp;
    }

    start = source->find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = source->find_last_not_of(" \t\r\n");
    return source->substr(start, end - start + 1);
}

template <typename T>
T ConfigParser::stringToNum(const std::string& str)
{
    std::stringstream stream(str);
    T number;
    stream >> number;
    return number;
}

void ConfigParser::parseKeyValue(const std::string& line, std::string& key, std::string& value)
{
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos)
    {
        throw std::invalid_argument("Invalid format, expected \"key = value\"");
    }
    key = ConfigParser::trimAndStripComments(line.substr(0, eq_pos));
    value = ConfigParser::trimAndStripComments(line.substr(eq_pos + 1));
}

/**
 * @brief Parses global configuration.
 *
 * @param server Pointer to the Server object where the parsed global
 * configuration will be applied.
 * @param line The current trimmed line from the configuration file being
 * parsed.
 *
 * @throws std::invalid_argument If an invalid token is encountered.
 */
void ConfigParser::parseGlobal(Server* server, const std::string& line)
{
    (void) server;
    if (line == "[vhost.start]")
    {
        _currentVhost = Vhost();
        _state = &ConfigParser::parseVhost;
    }
    else
    {
        throw std::runtime_error("Unexpected element.");
    }
}

/**
 * @brief Parses a virtual host block.
 *
 * @param server Pointer to the Server object where the parsed virtual host
 * configuration will be added.
 * @param line The current trimmed line from the configuration file being
 * parsed.
 *
 * @throws std::invalid_argument If an invalid token is encountered.
 */
void  ConfigParser::parseVhost(Server* server, const std::string& line)
{
    if (line == "[vhost.end]")
    {
        _currentVhost.verify();
        server->addVhosts(_currentVhost);
        _state = &ConfigParser::parseGlobal;
    }
    else if (line == "[location.start]")
    {
        _currentLocation = Location(_currentVhost);
        ConfigParser::_state = &ConfigParser::parseLocation;
    }
    else
    {
        std::string key;
        std::string value;

        parseKeyValue(line, key, value);
        if (key == "host") _currentVhost.setHost(value);
        else if (key == "listen") _currentVhost.setPort(stringToNum<uint32_t>(value));
        else if (key == "server_name") _currentVhost.setServerName(value);
        else if (key == "max_body_size") _currentVhost.setMaxBodySize(stringToNum<uint64_t>(value));
        else if (key == "keep_alive_timeout") _currentVhost.setTimeout(stringToNum<uint64_t>(value));
        else if (key == "error_page") _currentVhost.addErrorPages(value);
        else if (key == "cgi") _currentVhost.addCGI(value);
        else
        {
            throw std::invalid_argument("Invalid token in vhost block");
        }
    }
}

/**
 * @brief Parses a location block within a virtual host.
 *
 * @param server Pointer to the Server object where the parsed location
 * configuration will be added.
 * @param line The current trimmed line from the configuration file being
 * parsed.
 *
 * @throws std::invalid_argument If an invalid token is encountered
 * within the location block.
 */
void ConfigParser::parseLocation(Server* server, const std::string& line)
{
    (void)server;
    if (line == "[location.end]")
    {
        _currentLocation.verify();
        _currentVhost.addLocation(_currentLocation);
        _state = &ConfigParser::parseVhost;
    }
    else
    {
        std::string key;
        std::string value;
        parseKeyValue(line, key, value);
        if (key == "path") _currentLocation.setPath(value);
        else if (key == "root") _currentLocation.setRoot(value);
        else if (key == "autoindex") _currentLocation.setAutoindex(value);
        else if (key == "upload_store") _currentLocation.setUploadStore(value);
        else if (key == "max_body_size") _currentLocation.setMaxBodySize(value);
        else if (key == "default") _currentLocation.addDefault(value);
        else if (key == "return") _currentLocation.setReturn(value);
        else if (key == "methods") _currentLocation.addMethod(value);
        else
        {
             throw std::invalid_argument("Invalid token in location block");
        }
    }
}

/**
 * @brief Parses the configuration files and fills the Server object.
 *
 * Iterates through each configuration file, parses each line,
 * and calls the appropriate parsing function based on the current state.
 *
 * @param server Pointer to the Server object to be filled with parsed
 * configuration data.
 */

void ConfigParser::parse(Server* server)
{
    for (std::vector<std::string>::const_iterator it = server->getConfigurationFiles().begin();
        it != server->getConfigurationFiles().end(); ++it)
    {
        int line_n = 0;
        std::string current_line = "";

        try
        {
            std::ifstream currentFile((*it).c_str());
            if (!currentFile.is_open())
            {
                std::cerr << "[ERROR] Could not open configuration file \"" << *it
                            << "\", this file will be skipped." << std::endl;
                continue;
            }
            std::cerr << "[INFO] Started parsing: " << *it << std::endl;

            for (; std::getline(currentFile, current_line); ++line_n)
            {
                current_line = ConfigParser::trimAndStripComments(current_line);
                if (current_line.empty() || current_line[0] == ';')
                    continue;

                _state(server, current_line);
            }
            if (_state != &ConfigParser::parseGlobal)
            {
                throw std::runtime_error("Unexpected EOF.");
            }
            std::cerr << "[INFO] Ended parsing: " << *it << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ERROR] Bad configuration in file: " << *it << std::endl;
            std::cerr << "[ERROR] At line " << (line_n + 1) << ": \"" << current_line << "\"" << std::endl;
            std::cerr << "[ERROR] Reason: " << e.what() << std::endl;
            _state = &ConfigParser::parseGlobal;
        }
    }
    if (server->getVhosts().size() == 0)
    {
        throw std::runtime_error("[ERROR] No virtual host configured.");
    }
}

// Static member initializations
ConfigParser::StateFuncPtr ConfigParser::_state = &ConfigParser::parseGlobal;
Vhost ConfigParser::_currentVhost;
Location ConfigParser::_currentLocation(_currentVhost);

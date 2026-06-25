/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 14:11:51 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/25 16:08:22 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <string>
#include <sstream>
#include <iostream>

static std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Request::Request(const std::string& rawRequest)
{
    size_t headerEndPos = rawRequest.find("\r\n\r\n");

    std::string headerPart = rawRequest.substr(0, headerEndPos);
    _body = rawRequest.substr(headerEndPos + 4); // skip the \r\n\r\n [1]

    std::stringstream sstream(headerPart);
    std::string line;

	// Request line
    if (std::getline(sstream, line))
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line.erase(line.length() - 1);
        }

        std::stringstream requestLineStream(line);
        requestLineStream >> _method >> _path >> _version;
        std::string extra;
        if (requestLineStream >> extra)
        {
            throw std::runtime_error("Invalid HTTP request: Request line contains too many arguments.");
        }

        if (_method.empty() || _path.empty() || _version.empty())
        {
            throw std::runtime_error("Invalid HTTP request: Request line is missing elements.");
        }

        if (_version.compare(0, 5, "HTTP/") != 0)
        {
            throw std::runtime_error("Invalid HTTP request: Unsupported or invalid protocol version.");
        }

        size_t questionMarkPos = _path.find('?');
        if (questionMarkPos != std::string::npos)
        {
            _query_string = _path.substr(questionMarkPos + 1);
            _path = _path.substr(0, questionMarkPos);
        }
        else
        {
            _query_string = "";
        }
    }

	// Headers
    while (std::getline(sstream, line))
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line.erase(line.length() - 1);
        }
        else if (!line.empty())
        {
            throw std::runtime_error("Invalid HTTP request: Header line must end with CRLF.");
        }

        if (line.empty())
        {
            continue;
        }


        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
        {
            throw std::runtime_error("Invalid HTTP request: Malformed header.");
        }

        std::string key = line.substr(0, colonPos);

        if (!key.empty() && (std::isspace(static_cast<unsigned char>(key[0])) ||
                             std::isspace(static_cast<unsigned char>(key[key.length()]))))
        {
            throw std::runtime_error("Invalid HTTP request: Whitespace around the header key.");
        }

        key = trim(key);
        std::string value = trim(line.substr(colonPos + 1));

        if (key.empty())
        {
            throw std::runtime_error("Invalid HTTP request: Empty header key.");
        }

        _headers[key] = value;
    }
}

Request::~Request()
{}

bool Request::operator==(const Request& other) const
{
    return this == &other;
}

void Request::debugRequest() const
{
    std::cout << "-------------- REQUEST --------------" << std::endl;
    std::cout << "Method:       [" << _method << "]" << std::endl;
    std::cout << "Path:         [" << _path << "]" << std::endl;
    std::cout << "Version:      [" << _version << "]" << std::endl;
    std::cout << "Query String: [" << _query_string << "]" << std::endl;

    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it)
    {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cout << "Body (" << _body.length() << " bytes):" << std::endl;
    std::cout << _body << std::endl;
    std::cout << "-------------------------------------" << std::endl;
}

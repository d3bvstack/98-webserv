/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 20:06:48 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/22 23:03:01 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Location.hpp"
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <sstream>

Location::Location()
    : _autoindex(false), _autoindex_set(false), _max_body_size(0), _max_body_size_set(false)
{
}

Location::~Location()
{
}

void Location::setAutoindex(const std::string& value)
{
    if (_autoindex_set)
    {
        throw std::runtime_error("Autoindex already set for this location");
    }
    _autoindex = (value == "true");
    _autoindex_set = true;
}

void Location::setPath(const std::string& path)
{
    if (isPathSet())
    {
        throw std::runtime_error("Path already set for this location");
    }
    _path = path;
}

void Location::setRoot(const std::string& root)
{
    if (isRootSet())
    {
        throw std::runtime_error("Root already set for this location");
    }
    _root = root;
}

void Location::setUploadStore(const std::string& upload_store)
{
    if (isUploadStoreSet())
    {
        throw std::runtime_error("Upload storage already set for this location");
    }
    _upload_store = upload_store;
}

void Location::setMaxBodySize(const std::string& value)
{
    if (_max_body_size_set)
    {
        throw std::runtime_error("Max body size already set for this location");
    }
    _max_body_size = std::strtoull(value.c_str(), NULL, 10);
    _max_body_size_set = true;
}

void Location::addDefault(const std::string& default_files)
{
    if (!_defaults.empty())
    {
        throw std::runtime_error("Defaults already set for this location");
    }

    std::string token;
    std::istringstream iss(default_files);
    while (iss >> token)
    {
        _defaults.push_back(token);
    }
}

void Location::setReturn(const std::string& value)
{
    if (isReturnSet())
    {
        throw std::runtime_error("Redirection already set for this location");
    }
    size_t space = value.find(' ');
    if (space == std::string::npos)
    {
        throw std::invalid_argument("Redirection must be in format: 'return = code target'");
    }
    uint16_t code = std::strtoul(value.substr(0, space).c_str(), NULL, 10);
    std::string target = value.substr(space + 1);
    _return.first = code;
    _return.second = target;
}

void Location::addMethod(const std::string& method)
{
    _methods.push_back(method);
}

void Location::debugLocation() const
{
    std::cerr << "  [DEBUG] Location:" << std::endl;
    std::cerr << "    path: " << (_path.empty() ? "(not set)" : _path) << std::endl;
    std::cerr << "    root: " << (_root.empty() ? "(not set)" : _root) << std::endl;
    std::cerr << "    autoindex: " << (_autoindex ? "true" : "false") << std::endl;
    std::cerr << "    upload_store: " << (_upload_store.empty() ? "(not set)" : _upload_store) << std::endl;

    if (_max_body_size_set)
    {
        std::cerr << "    max_body_size: " << _max_body_size << std::endl;
    }
    else
    {
        std::cerr << "    max_body_size: (not set)" << std::endl;
    }

    if (_defaults.empty())
    {
        std::cerr << "    defaults: (not set)" << std::endl;
    }
    else
    {
        std::cerr << "    defaults: ";
        for (size_t i = 0; i < _defaults.size(); ++i)
        {
            if (i > 0) std::cerr << ", ";
            std::cerr << _defaults[i];
        }
        std::cerr << std::endl;
    }

    if (_return.first == 0)
    {
        std::cerr << "    return: (not set)" << std::endl;
    }
    else
    {
        std::cerr << "    return: " << _return.first << " " << _return.second << std::endl;
    }

    if (_methods.empty())
    {
        std::cerr << "    methods: (not set)" << std::endl;
    }
    else
    {
        std::cerr << "    methods: ";
        for (std::vector<std::string>::const_iterator it = _methods.begin();
             it != _methods.end(); ++it)
        {
            if (it != _methods.begin())
                std::cerr << ", ";
            std::cerr << *it ;
        }
        std::cerr << std::endl;
    }
}

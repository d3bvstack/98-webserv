/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/16 20:13:31 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

/**
 * @brief Checks if the string `filename` ends with the provided `extension`
 *
 * @param filename String to search on
 * @param extension String to search for
 * @return `true` or
 * @return `false`
 */
static bool matchExtension(const std::string& filename, const std::string extension)
{
    bool extensionMatch = false;

    if (filename.length() >= extension.length())
    {
        std::string endOfString = filename.substr(filename.length() - extension.length());
        extensionMatch = (endOfString == extension);
    }

    return (extensionMatch);
}

/**
 * @brief Construct a new Server:: Server object retrieving config files from default location
 *
 */
Server::Server()
{
    DIR* dirStream = opendir(DEFAULT_CONFIG_DIR);
    if (dirStream == NULL)
        throw std::runtime_error("[ERROR] Could not open default config directory " DEFAULT_CONFIG_DIR ".");

    struct dirent* dirEntry;
    while ((dirEntry = readdir(dirStream)) != NULL)
    {
        std::string filename = dirEntry->d_name;

        if (dirEntry->d_type == DT_REG && matchExtension(filename, ".conf"))
        {
            std::string fullpath = std::string(DEFAULT_CONFIG_DIR) + "/" + filename;
            _configurationFiles.push_back(fullpath);
            std::cerr << "[INFO] " << fullpath << " was correctly added as a configuration file." << std::endl;
        }
    }
    closedir(dirStream);

    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");

    std::cerr << "[INFO] Server object created successfully." << std::endl;
}


/**
 * @brief Construct a new Server:: Server object using the arguments as filepaths to configuration files
 *
 * @param argc Number of arguments
 * @param argv Array of arguments
 */
Server::Server(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (matchExtension(argv[i], ".conf"))
        {
            _configurationFiles.push_back(argv[i]);
            std::cerr << "[INFO] " << argv[i] << " was correctly added as a configuration file." << std::endl;
        }
        else
            std::cerr << "[WARNING] " << argv[i] << " is not a .conf file and will be skipped." << std::endl;
    }
    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");

    std::cerr << "[INFO] Server object created successfully." << std::endl;
}

bool Server::isPortAlreadyBound(std::string host, uint16_t port) const
{
    for (std::vector<ListeningSocket>::const_iterator it = _listeningSockets.begin(); it != _listeningSockets.end(); ++it)
    {
        if ((*it).getPort() == port && (*it).getHostPresentation() == host)
            return (true);
    }
    return (false);
}

void Server::bindListeningSockets()
{
    for (std::vector<Vhost>::const_iterator it = _vhosts.begin(); it != _vhosts.end(); ++it)
    {
        u_int16_t port = (*it).getPort();
        std::string host = (*it).getHost();
        try
        {
            if (isPortAlreadyBound(host, port))
            {
                std::cerr << "[INFO] Port " << port << " is already bound, skipping" << std::endl;
                continue;
            }
            std::cerr << "[INFO] Creating server socket on port " << port << std::endl;
            ListeningSocket tempSocket(host, port);
            tempSocket.create();
            tempSocket.setReusePort();
            tempSocket.bind();
            tempSocket.setNonBlocking();
            _listeningSockets.push_back(tempSocket);
            std::cerr << "[INFO] Server socket on port " << port << " created successfully" << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] Couldn't bind listening socket successfully on port " << port << std::endl;
            std::cerr << "[ERROR] Reason: " << e.what() << std::endl;
        }
    }
    if (_listeningSockets.empty())
        throw std::runtime_error("No port could be bound successfully.");
}

void Server::registerListeningSocketsWithEpoll()
{
    int registered_n = 0;
    for (std::vector<ListeningSocket>::const_iterator it = _listeningSockets.begin();
        it != _listeningSockets.end(); ++it)
    {
        try
        {
            _epoll.addSocket((*it).getSocketFd());
            ++registered_n;
        }
        catch(const std::exception& e)
        {
            std::cerr << "[ERROR] Couldn't register socket with epoll"<< std::endl;
            std::cerr << "[ERROR] Reason: " << e.what() << std::endl;
        }
    }
    if (registered_n == 0)
    {
        throw std::runtime_error("No sockets registered successfully");
    }
}

/**
 * @brief Prints the Server object members for debugging
 *
 */
void Server::debugServer() const
{
    int i = 0;
    for (std::vector<std::string>::const_iterator it = _configurationFiles.begin(); it != _configurationFiles.end(); ++it, ++i)
        std::cerr << "[DEBUG][SERVER] Configuration file " << i << ": " + *it << std::endl;
    i = 0;
    for (std::vector<Vhost>::const_iterator it = _vhosts.begin(); it != _vhosts.end(); ++it, ++i)
    {
        std::cerr << "[DEBUG][SERVER] Vhost " << i << ":" << std::endl;
        it->debugVhost();
    }
}

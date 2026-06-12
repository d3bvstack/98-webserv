/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:00:54 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/12 10:50:23 by dbarba-v         ###   ########.fr       */
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
        }
    }
    closedir(dirStream);

    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");
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
        }
        else
            std::cerr << "[WARNING] " << argv[i] << " is not a .conf file and will be skipped.";
    }
    if (_configurationFiles.empty())
        throw std::runtime_error("[ERROR] No valid configuration files where provided.");
}

/**
 * @brief Prints the Server object members for debugging
 *
 */
void Server::debugServer()
{
    int i = 0;
    for (std::vector<std::string>::iterator it = _configurationFiles.begin(); it != _configurationFiles.end(); ++it, ++i)
        std::cerr << "[DEBUG] Configuration file " << i << ": " + *it << std::endl;
}
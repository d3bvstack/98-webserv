/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 18:20:17 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/12 00:15:53 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <csignal>
#include <string>
#include <sstream>

#include "Server.hpp"


volatile sig_atomic_t keepRunning = 1;
volatile sig_atomic_t receivedSignal = 0;

void stop(int signal)
{
    keepRunning = 0;
    receivedSignal = signal;
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, stop);
    std::signal(SIGQUIT, stop);
    std::signal(SIGTERM, stop);

    Server* webserv = NULL;

    try
    {
        if (argc != 1)
            webserv = new Server(argc, argv); // Use arguments as config files
        else
            webserv = new Server; // Use default config directory

        webserv->debugServer();
    
        while (keepRunning)
        {
            ;
        }
        
        delete webserv;
        webserv = NULL;

        if (receivedSignal != 0)
        {
            std::stringstream ss;
            ss << receivedSignal;
            std::cout << "[SIGNAL] A signal was detected and terminated the execution, SIGNAL =" << ss.str() << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        if (webserv != NULL)
        {
            delete webserv;
            webserv = NULL;
        }
        return (EXIT_FAILURE);
    }
    
    return (EXIT_SUCCESS);
}

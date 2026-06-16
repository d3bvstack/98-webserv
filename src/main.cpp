/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 18:20:17 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/16 22:54:00 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <csignal>
#include <string>
#include <sstream>

#include "Server.hpp"
#include "Epoll.hpp"

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

    Server* webserver = NULL;

    try
    {
        if (argc != 1)
            webserver = new Server(argc, argv); // Use arguments as config files
        else
            webserver = new Server; // Use default config directory

        webserver->debugServer();
        webserver->parseConf();
        webserver->debugServer();
        webserver->bindListeningSockets();
        webserver->registerListeningSocketsWithEpoll();
        // webserver->startListening();

        while (keepRunning)
        {
            usleep(100000);
        }

        delete webserver;
        webserver = NULL;

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
        if (webserver != NULL)
        {
            delete webserver;
            webserver = NULL;
        }
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

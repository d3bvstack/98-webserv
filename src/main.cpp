/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 18:20:17 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/10 15:01:45 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <cstdlib>
#include <csignal>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>
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

    Server* webserver = NULL;

    try
    {
        if (argc != 1)
            webserver = new Server(argc, argv); // Use arguments as config files
        else
            webserver = new Server; // Use default config directory

        webserver->parseConf();
        webserver->verifyConf();
        webserver->debugServer();
        webserver->bindListeningSockets();
        webserver->registerListeningSocketsWithEpoll();
        webserver->startListening();

        int nEvents;
        while (keepRunning)
        {
            nEvents = webserver->pollEvents();
            if (nEvents > 0)
            {
                webserver->handleIncomingEvents(nEvents);
                webserver->processPendingRequests();
                webserver->handleOutgoingEvents(nEvents);
            }
            webserver->checkCgiChildren();
            webserver->checkIdleTimeouts();
            webserver->cleanupPendingDeletions();
        }

        delete webserver;
        webserver = NULL;

        if (receivedSignal != 0)
        {
            std::stringstream ss;
            ss << receivedSignal;
            std::cout << "[SIGNAL] A signal was detected and terminated the execution, SIGNAL = " << ss.str() << std::endl;
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

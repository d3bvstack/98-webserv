/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/10 18:20:17 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/11 15:35:20 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <csignal>
#include <string>

volatile sig_atomic_t keep_running = 1;

void stop(int)
{
    keep_running = 0;
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, stop);
    std::signal(SIGQUIT, stop);
    std::signal(SIGTERM, stop);

    (void)argc;
    (void)argv;
    while (keep_running)
    {
        ;
    }
    return (EXIT_SUCCESS);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/18 16:30:00 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/17 14:42:07 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "EventTarget.hpp"

const int SOCKET_TYPE_LISTEN = 0;
const int SOCKET_TYPE_CLIENT = 1;

class Socket : public EventTarget
{
	protected:
		int _socketFd;
		int _socketType;

	public:
		Socket();
		virtual ~Socket();

		int getSocketFd() const;
		int getSocketType() const;
};

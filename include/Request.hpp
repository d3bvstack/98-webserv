/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 09:52:42 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:06:08 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>
#include <sys/types.h>

class Request
{
	private:
		std::string _method;
		std::string _path;
		std::string _version;
		std::string _query_string;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		Request(const std::string&);
		~Request();

		bool operator==(const Request& other) const;

		void debugRequest() const;

};

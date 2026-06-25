/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 09:52:45 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 00:06:42 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>
#include <vector>

class Vhost;

class Response
{
	private:
		int _statusCode;
		std::string _reason;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;

		std::string codeToReason(int code) const;
		std::string numToString(size_t num) const;
		std::string methodsToString(const std::vector<std::string>& allowedMethods) const;

	public:
		Response(int code);
		~Response();


		bool operator==(const Response& other) const;

		Response static createErrorResponse(int code, const Vhost& vhost);
		void setBody(const std::string& body);
		void setHeader(const std::string& key, const std::string& value);
		void setAllowedMethods(const std::vector<std::string>& allowedMethods);

		std::string toString() const;

		void debugResponse() const;
};

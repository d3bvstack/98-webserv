/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 13:45:36 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/28 10:54:51 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <stdint.h>
#include <sstream>
#include <fstream>
#include <map>
#include <iostream>
#include <utility>
#include "Vhost.hpp"

Response::Response(int code)
	: _statusCode(code), _version("HTTP/1.1"), _body("")
{
	_reason = codeToReason(code);
	_headers["Content-Length"] = "0";
	_headers["Server"] = "98Webserv";
	
}

Response::~Response()
{
}

bool Response::operator==(const Response& other) const
{
    return this == &other;
}

static std::string readFileToString(const std::string& filePath)
{
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		return ("");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return (buffer.str());
}

Response Response::createErrorResponse(int code, const Vhost& vhost)
{
	Response response(code);
	const std::map<uint16_t, std::string>& pages = vhost.getErrorPages();
	std::map<uint16_t, std::string>::const_iterator it = pages.find(code);
	if (it != pages.end())
	{
		std::string content = readFileToString(it->second);
		if (!content.empty())
		{
			response.setBody(content);
			response.setHeader("Content-Type", "text/html");
			response.debugResponse();
			return (response);
		}
	}
	response.setBody(response.codeToReason(code));
	response.setHeader("Content-Type", "text/plain");

	response.debugResponse();
	return (response);
}

void Response::setHeader(const std::string& key, const std::string& value)
{
	_headers[key] = value;
}

void Response::setBody(const std::string& body)
{
	_body = body;
	_headers["Content-Length"] = numToString(_body.length());
}

void Response::setAllowedMethods(const std::vector<std::string>& allowedMethods)
{
	_headers["Allow"] = methodsToString(allowedMethods);
}

std::string Response::codeToReason(int code) const
{
	switch (code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 413: return "Payload Too Large";
		case 431: return "Request Header Fields Too Large";
		case 405: return "Method Not Allowed";
		case 500: return "Internal Server Error";
		default:  return "Unknown Status";
	}
}

std::string Response::numToString(size_t num) const
{
	std::stringstream sstream;
	sstream << num;
	return (sstream.str());
}

std::string Response::methodsToString(const std::vector<std::string>& allowedMethods) const
{
	std::string result = "";
	for (size_t i = 0; i < allowedMethods.size(); ++i)
	{
		result += allowedMethods[i];
		if (i < allowedMethods.size() - 1)
		{
			result += ", ";
		}
	}
	return (result);
}

std::string Response::toString() const
{
	std::stringstream sstream;

	sstream << _version << " " << _statusCode << " " << _reason << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
	{
		sstream << it->first << ": " << it->second << "\r\n";
	}
	sstream << "\r\n";
	sstream << _body;

	return (sstream.str());
}

void Response::debugResponse() const
{
    std::cerr << "-------------- RESPONSE --------------" << std::endl;
    std::cerr << "Status Code:    [" << _statusCode << "]" << std::endl;
    std::cerr << "Reason:         [" << _reason << "]" << std::endl;
    std::cerr << "Version:        [" << _version << "]" << std::endl;

    std::cerr << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it)
    {
        std::cerr << "  " << it->first << ": " << it->second << std::endl;
    }

    std::cerr << "Body (" << _body.length() << " bytes):" << std::endl;
    std::cerr << _body << std::endl;
    std::cerr << "-------------------------------------" << std::endl;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 09:52:45 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/17 14:30:24 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <stddef.h>
#include <stdint.h>
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
        bool _streamed;

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
        void setConnectionClose();
        void setConnectionKeepAlive(uint64_t timeout);
        void setChunked();
        void setStreamed();

        int getStatusCode() const { return _statusCode; };
        const std::string& getReason() const { return _reason; };
        const std::string& getVersion() const { return _version; };
        bool isStreamed() const { return _streamed; };

        std::string toString() const;
        std::string toStringHeadersOnly() const;

        static std::string encodeChunk(const char* data, size_t len);
        static std::string finalChunk();

        void debugResponse() const;
};

#pragma once

#include <string>
#include <vector>
#include "Request.hpp"
#include "Response.hpp"
#include "Vhost.hpp"

class ClientConnection;

namespace server_utils
{
    bool matchExtension(const std::string& filename, const std::string& extension);
    bool hasKeepAlive(const std::string& headers);

    const Location* findBestLocation(const Vhost& vhost, const std::string& requestPath);
    bool requestMethodAllowed(const Location& location, const std::string& method);
    std::vector<std::string> defaultAllowedMethods();

    std::string buildUrlPathFromLocation(const Location& location, const Request& request);
    std::string resolveFilesystemPath(const Location& location, const Request& request);

    Response applyConnectionPolicy(Response response, const ClientConnection* client);
    Response buildRedirectResponse(const Location& location, const ClientConnection* client);
    Response buildMethodNotAllowedResponse(const Location& location, const Vhost& vhost, const ClientConnection* client);
    bool isCgiRequest(const Vhost& vhost, const Request& request);
    Response buildCgiResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client);

    Response buildGetResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client);
    Response buildPostResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client);
    Response buildDeleteResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client);
}
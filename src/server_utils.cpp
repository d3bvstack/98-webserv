#include "ServerUtils.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <poll.h>
#include <unistd.h>
#include <vector>
#include <utility>
#include <sys/wait.h>
#include <iostream>
#include "ClientConnection.hpp"

namespace
{
    bool hasSuffix(const std::string& value, const std::string& suffix)
    {
        if (value.length() < suffix.length())
            return (false);
        return (value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0);
    }

    bool pathExists(const std::string& path, bool* isDirectory)
    {
        struct stat stats;

        if (stat(path.c_str(), &stats) != 0)
            return (false);
        if (isDirectory != NULL)
            *isDirectory = S_ISDIR(stats.st_mode);
        return (true);
    }

    std::string joinPaths(const std::string& left, const std::string& right)
    {
        if (left.empty())
            return (right);
        if (right.empty())
            return (left);
        if (left[left.length() - 1] == '/' && right[0] == '/')
            return (left + right.substr(1));
        if (left[left.length() - 1] != '/' && right[0] != '/')
            return (left + "/" + right);
        return (left + right);
    }

    std::string htmlEscape(const std::string& value)
    {
        std::string escaped;
        for (size_t i = 0; i < value.length(); ++i)
        {
            switch (value[i])
            {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                default: escaped += value[i]; break;
            }
        }
        return (escaped);
    }

    std::string readFileToString(const std::string& filePath)
    {
        std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open())
            return ("");

        std::stringstream buffer;
        buffer << file.rdbuf();
        return (buffer.str());
    }

    std::string getContentType(const std::string& path)
    {
        if (hasSuffix(path, ".html") || hasSuffix(path, ".htm"))
            return ("text/html");
        if (hasSuffix(path, ".css"))
            return ("text/css");
        if (hasSuffix(path, ".js"))
            return ("application/javascript");
        if (hasSuffix(path, ".json"))
            return ("application/json");
        if (hasSuffix(path, ".xml"))
            return ("application/xml");
        if (hasSuffix(path, ".png"))
            return ("image/png");
        if (hasSuffix(path, ".jpg") || hasSuffix(path, ".jpeg"))
            return ("image/jpeg");
        if (hasSuffix(path, ".gif"))
            return ("image/gif");
        if (hasSuffix(path, ".svg"))
            return ("image/svg+xml");
        if (hasSuffix(path, ".py"))
            return ("text/x-python");
        return ("text/plain");
    }

    std::string buildDirectoryListing(const std::string& directoryPath, const std::string& requestPath)
    {
        DIR* dirStream = opendir(directoryPath.c_str());
        if (dirStream == NULL)
            return ("");

        std::stringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of "
             << htmlEscape(requestPath)
             << "</title></head><body><h1>Index of "
             << htmlEscape(requestPath)
             << "</h1><ul>";

        struct dirent* entry;
        while ((entry = readdir(dirStream)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;

            std::string link = joinPaths(requestPath, name);
            html << "<li><a href=\"" << htmlEscape(link) << "\">"
                 << htmlEscape(name) << "</a></li>";
        }
        closedir(dirStream);

        html << "</ul></body></html>";
        return (html.str());
    }

    bool matchesCgiExtension(const std::string& requestPath, const std::string& extension)
    {
        size_t searchPos = 0;

        while (true)
        {
            size_t extensionPos = requestPath.find(extension, searchPos);
            if (extensionPos == std::string::npos)
                return (false);

            size_t extensionEndPos = extensionPos + extension.length();
            if (extensionEndPos == requestPath.length() || requestPath[extensionEndPos] == '/')
                return (true);

            searchPos = extensionPos + 1;
        }
    }


}

namespace server_utils
{
    bool matchExtension(const std::string& filename, const std::string& extension)
    {
        if (filename.length() < extension.length())
            return (false);

        std::string endOfString = filename.substr(filename.length() - extension.length());
        return (endOfString == extension);
    }

    bool hasKeepAlive(const std::string& headers)
    {
        std::string lower;
        lower.reserve(headers.length());
        for (size_t i = 0; i < headers.length(); ++i)
            lower += std::tolower(static_cast<unsigned char>(headers[i]));

        size_t colonPos = lower.find("connection:");
        if (colonPos == std::string::npos)
            return (false);

        colonPos += 11;
        colonPos = lower.find_first_not_of(" \t", colonPos);
        if (colonPos == std::string::npos)
            return (false);

        size_t endPos = lower.find("\r\n", colonPos);
        if (endPos == std::string::npos)
            endPos = lower.length();

        std::string value = lower.substr(colonPos, endPos - colonPos);
        size_t lastNonSpace = value.find_last_not_of(" \t");
        if (lastNonSpace == std::string::npos)
            return (false);
        value.erase(lastNonSpace + 1);

        return (value == "keep-alive");
    }

    const Location* findBestLocation(const Vhost& vhost, const std::string& requestPath)
    {
        const std::vector<Location>& locations = vhost.getLocations();
        const Location* bestLocation = NULL;
        size_t bestLength = 0;

        for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
        {
            const std::string& locationPath = it->getPath();
            bool matches = false;

            if (locationPath == "/")
                matches = true;
            else if (requestPath.length() >= locationPath.length()
                && requestPath.compare(0, locationPath.length(), locationPath) == 0)
            {
                if (requestPath.length() == locationPath.length()
                    || locationPath[locationPath.length() - 1] == '/'
                    || requestPath[locationPath.length()] == '/')
                {
                    matches = true;
                }
            }

            if (!matches && requestPath + "/" == locationPath)
                matches = true;

            if (matches && locationPath.length() >= bestLength)
            {
                bestLocation = &(*it);
                bestLength = locationPath.length();
            }
        }

        if (bestLocation == NULL && !locations.empty())
            bestLocation = &locations.front();
        return (bestLocation);
    }

    bool requestMethodAllowed(const Location& location, const std::string& method)
    {
        if (!location.isMethodsSet())
            return (method == "GET" || method == "POST" || method == "DELETE");

        const std::vector<std::string>& allowedMethods = location.getMethods();
        for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it)
        {
            if (*it == method)
                return (true);
        }
        return (false);
    }

    std::vector<std::string> defaultAllowedMethods()
    {
        std::vector<std::string> methods;
        methods.push_back("GET");
        methods.push_back("POST");
        methods.push_back("DELETE");
        return (methods);
    }

    std::string buildUrlPathFromLocation(const Location& location, const Request& request)
    {
        const std::string& locationPath = location.getPath();
        std::string requestPath = request.getPath();

        if (requestPath + "/" == locationPath)
            return ("/");

        if (!locationPath.empty() && requestPath.compare(0, locationPath.length(), locationPath) == 0)
        {
            requestPath.erase(0, locationPath.length());
        }

        if (requestPath.empty())
            requestPath = "/";
        return (requestPath);
    }

    std::string resolveFilesystemPath(const Location& location, const Request& request)
    {
        std::string resolved = joinPaths(location.getRoot(), buildUrlPathFromLocation(location, request));
        if (resolved.length() > 1 && resolved[resolved.length() - 1] == '/')
            resolved.erase(resolved.length() - 1);
        return (resolved);
    }

    Response applyConnectionPolicy(Response response, const ClientConnection* client)
    {
        if (client != NULL && client->getKeepAlive() && client->getVhost() != NULL)
            response.setConnectionKeepAlive(client->getVhost()->getTimeout());
        else
            response.setConnectionClose();
        return (response);
    }

    Response buildRedirectResponse(const Location& location, const ClientConnection* client)
    {
        Response response(location.getReturn().first);
        response.setHeader("Location", location.getReturn().second);
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Redirecting");
        return (applyConnectionPolicy(response, client));
    }

    Response buildMethodNotAllowedResponse(const Location& location, const Vhost& vhost, const ClientConnection* client)
    {
        Response response = Response::createErrorResponse(405, vhost);
        if (location.isMethodsSet())
            response.setAllowedMethods(location.getMethods());
        else
            response.setAllowedMethods(defaultAllowedMethods());
        return (applyConnectionPolicy(response, client));
    }

    bool isCgiRequest(const Vhost& vhost, const Request& request)
    {
        const std::map<std::string, std::string>& cgi = vhost.getCGI();

        for (std::map<std::string, std::string>::const_iterator it = cgi.begin(); it != cgi.end(); ++it)
        {
            if (matchesCgiExtension(request.getPath(), it->first))
                return (true);
        }
        return (false);
    }

    Response buildGetResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client)
    {
        // Redirects are handled before touching the filesystem.
        if (location.isReturnSet())
            return (buildRedirectResponse(location, client));

        // Resolve the requested path inside the location root.
        std::string filesystemPath = resolveFilesystemPath(location, request);
        bool isDirectory = false;
        if (!pathExists(filesystemPath, &isDirectory))
            return (applyConnectionPolicy(Response::createErrorResponse(404, vhost), client));

        // If the target is a directory, try default files first.
        if (isDirectory)
        {
            if (location.isDefaultsSet())
            {
                const std::vector<std::string>& defaults = location.getDefaults();
                for (std::vector<std::string>::const_iterator it = defaults.begin(); it != defaults.end(); ++it)
                {
                    std::string candidatePath = joinPaths(filesystemPath, *it);
                    bool candidateIsDirectory = false;
                    if (pathExists(candidatePath, &candidateIsDirectory) && !candidateIsDirectory)
                    {
                        filesystemPath = candidatePath;
                        isDirectory = false;
                        break;
                    }
                }
            }

            // If it is still a directory, either list it or reject it.
            if (isDirectory)
            {
                if (location.getAutoindex())
                {
                    // Build a simple HTML index for the directory.
                    std::string listing = buildDirectoryListing(filesystemPath, request.getPath());
                    if (listing.empty())
                        return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

                    Response response(200);
                    response.setHeader("Content-Type", "text/html");
                    response.setBody(listing);
                    return (applyConnectionPolicy(response, client));
                }
                if (location.isDefaultsSet())
                    return (applyConnectionPolicy(Response::createErrorResponse(404, vhost), client));
                return (applyConnectionPolicy(Response::createErrorResponse(403, vhost), client));
            }
        }

        // Serve the file content with a best-effort content type.
        std::string body = readFileToString(filesystemPath);
        if (body.empty() && !pathExists(filesystemPath, NULL))
            return (applyConnectionPolicy(Response::createErrorResponse(404, vhost), client));

        Response response(200);
        response.setHeader("Content-Type", getContentType(filesystemPath));
        response.setBody(body);
        return (applyConnectionPolicy(response, client));
    }

    Response buildPostResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client)
    {
        // Redirects win before any body handling.
        if (location.isReturnSet())
            return (buildRedirectResponse(location, client));

        // If an upload folder exists, store the body there as a new file.
        if (location.isUploadStoreSet())
        {
            bool isDirectory = false;
            if (!pathExists(location.getUploadStore(), &isDirectory) || !isDirectory)
                return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

            std::stringstream nameStream;
            nameStream << "upload_" << static_cast<long>(std::time(NULL)) << ".txt";
            std::string uploadPath = joinPaths(location.getUploadStore(), nameStream.str());

            std::ofstream uploadFile(uploadPath.c_str(), std::ios::out | std::ios::binary);
            if (!uploadFile.is_open())
                return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

            uploadFile << request.getBody();
            if (!uploadFile.good())
                return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

            Response response(201);
            response.setHeader("Content-Type", "text/plain");
            response.setBody("Created");
            return (applyConnectionPolicy(response, client));
        }

        // Otherwise, just echo the body back as a basic POST response.
        Response response(200);
        response.setHeader("Content-Type", "text/plain");
        response.setBody(request.getBody());
        return (applyConnectionPolicy(response, client));
    }

    Response buildDeleteResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client)
    {
        // Redirects are handled first, just like in GET and POST.
        if (location.isReturnSet())
            return (buildRedirectResponse(location, client));

        // Resolve the requested file and make sure it exists.
        std::string filesystemPath = resolveFilesystemPath(location, request);
        bool isDirectory = false;
        if (!pathExists(filesystemPath, &isDirectory))
            return (applyConnectionPolicy(Response::createErrorResponse(404, vhost), client));

        // Only regular files can be deleted here.
        if (isDirectory)
            return (applyConnectionPolicy(Response::createErrorResponse(403, vhost), client));

        // Remove the file from disk and report success.
        if (::remove(filesystemPath.c_str()) != 0)
            return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

        Response response(200);
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Deleted");
        return (applyConnectionPolicy(response, client));
    }
}

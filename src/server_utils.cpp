#include "ServerUtils.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <vector>
#include <utility>
#include <sys/wait.h>
#include <iostream>
#include "ClientConnection.hpp"

namespace
{
    const unsigned int CGI_RESPONSE_TIMEOUT_SECONDS = 5;

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

    std::string normalizeLineEndings(const std::string& input)
    {
        std::string normalized;
        normalized.reserve(input.length());

        for (size_t i = 0; i < input.length(); ++i)
        {
            if (input[i] == '\r')
            {
                if (i + 1 < input.length() && input[i + 1] == '\n')
                    continue;
                normalized += '\n';
            }
            else
            {
                normalized += input[i];
            }
        }
        return (normalized);
    }

    std::string trimCopy(const std::string& value)
    {
        size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return ("");
        size_t last = value.find_last_not_of(" \t\r\n");
        return (value.substr(first, last - first + 1));
    }

    std::string getRequestExtension(const std::string& path)
    {
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos)
            return ("");
        return (path.substr(dotPos));
    }

    std::string findCgiInterpreter(const Vhost& vhost, const Request& request)
    {
        std::string requestExtension = getRequestExtension(request.getPath());
        const std::map<std::string, std::string>& cgi = vhost.getCGI();

        std::map<std::string, std::string>::const_iterator it = cgi.find(requestExtension);
        if (it == cgi.end())
            return ("");
        return (it->second);
    }

    bool collectCgiOutputWithTimeout(pid_t pid, int outputFd, std::string* output, int* exitStatus)
    {
        if (output == NULL)
            return (false);

        if (exitStatus != NULL)
            *exitStatus = 0;

        // Put the CGI stdout/stderr pipe in non-blocking mode so we can
        // interleave reads with timeout checks and child-process polling.
        int flags = fcntl(outputFd, F_GETFL, 0);
        if (flags == -1 || fcntl(outputFd, F_SETFL, flags | O_NONBLOCK) == -1)
            return (false);

        time_t startTime = std::time(NULL);
        bool outputClosed = false;
        bool childExited = false;
        int status = 0;

        while (true)
        {
            if (!outputClosed)
            {
                struct pollfd pollEntry;
                pollEntry.fd = outputFd;
                pollEntry.events = POLLIN;
                pollEntry.revents = 0;

                // Recompute the remaining time on every iteration so the whole
                // collection phase is bounded by CGI_RESPONSE_TIMEOUT_SECONDS.
                int elapsedSeconds = static_cast<int>(std::difftime(std::time(NULL), startTime));
                int remainingTimeout = static_cast<int>(CGI_RESPONSE_TIMEOUT_SECONDS) - elapsedSeconds;
                if (remainingTimeout <= 0)
                {
                    // The CGI overstayed its time budget; kill it and report failure.
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    if (exitStatus != NULL)
                        *exitStatus = status;
                    return (false);
                }

                // Wait until the pipe becomes readable or the timeout expires.
                int pollResult = poll(&pollEntry, 1, remainingTimeout * 1000);
                if (pollResult == -1)
                    return (false);

                if (pollResult == 0)
                {
                    // No output arrived before the deadline.
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    if (exitStatus != NULL)
                        *exitStatus = status;
                    return (false);
                }

                if (pollEntry.revents & (POLLERR | POLLNVAL))
                    return (false);

                // Drain everything currently available so we do not spin on the
                // same readable event and to avoid leaving data in the pipe.
                char buffer[4096];
                while (true)
                {
                    ssize_t bytesRead = read(outputFd, buffer, sizeof(buffer));
                    if (bytesRead > 0)
                    {
                        output->append(buffer, static_cast<size_t>(bytesRead));
                    }
                    else if (bytesRead == 0)
                    {
                        outputClosed = true;
                        break;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (!childExited)
            {
                // Check whether the child has already terminated without blocking.
                pid_t waitResult = waitpid(pid, &status, WNOHANG);
                if (waitResult == -1)
                    return (false);
                if (waitResult == pid)
                    childExited = true;
            }

            // Success only happens once the pipe is closed and the child is gone.
            if (outputClosed && childExited)
            {
                if (exitStatus != NULL)
                    *exitStatus = status;
                return (true);
            }

            // Final guard in case the loop is still active after the deadline.
            if (std::difftime(std::time(NULL), startTime) >= CGI_RESPONSE_TIMEOUT_SECONDS)
            {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                if (exitStatus != NULL)
                    *exitStatus = status;
                return (false);
            }
        }
    }

    std::pair<int, std::string> parseCgiStatus(const std::string& statusValue)
    {
        int code = 200;
        std::string reason;

        std::stringstream sstream(statusValue);
        sstream >> code;
        std::getline(sstream, reason);
        reason = trimCopy(reason);
        return (std::make_pair(code, reason));
    }

    Response buildResponseFromCgiOutput(const std::string& rawOutput)
    {
        std::string normalized = normalizeLineEndings(rawOutput);
        size_t headerEndPos = normalized.find("\n\n");
        std::string headerBlock = (headerEndPos == std::string::npos) ? normalized : normalized.substr(0, headerEndPos);
        std::string body = (headerEndPos == std::string::npos) ? "" : normalized.substr(headerEndPos + 2);

        int statusCode = 200;
        std::stringstream headerStream(headerBlock);
        std::string line;
        std::vector<std::pair<std::string, std::string> > headers;

        while (std::getline(headerStream, line))
        {
            line = trimCopy(line);
            if (line.empty())
                continue;

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos)
                continue;

            std::string key = trimCopy(line.substr(0, colonPos));
            std::string value = trimCopy(line.substr(colonPos + 1));

            if (key == "Status")
            {
                std::pair<int, std::string> status = parseCgiStatus(value);
                statusCode = status.first;
            }
            else if (key != "Content-Length")
            {
                headers.push_back(std::make_pair(key, value));
            }
        }

        Response response(statusCode);
        for (std::vector<std::pair<std::string, std::string> >::const_iterator it = headers.begin(); it != headers.end(); ++it)
            response.setHeader(it->first, it->second);
        response.setBody(body);
        return (response);
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
            if (hasSuffix(request.getPath(), it->first))
                return (true);
        }
        return (false);
    }

    Response buildCgiResponse(const Vhost& vhost, const Location& location, const Request& request, const ClientConnection* client)
    {
        if (location.isReturnSet())
            return (buildRedirectResponse(location, client));

        // Pick the CGI interpreter from the request extension and vhost map.
        std::string interpreter = findCgiInterpreter(vhost, request);
        if (interpreter.empty())
            return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

        // The script must exist and must not be a directory.
        std::string scriptPath = resolveFilesystemPath(location, request);
        bool isDirectory = false;

        if (!pathExists(scriptPath, &isDirectory) || isDirectory)
            return (applyConnectionPolicy(Response::createErrorResponse(404, vhost), client));

        // Create one pipe for request body input and one for CGI output.
        int inputPipe[2];
        int outputPipe[2];
        if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
            return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

        pid_t pid = fork();
        if (pid == -1)
        {
            close(inputPipe[0]);
            close(inputPipe[1]);
            close(outputPipe[0]);
            close(outputPipe[1]);
            return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));
        }

        if (pid == 0)
        {
            // Child process: wire the pipes to stdin/stdout/stderr.
            dup2(inputPipe[0], STDIN_FILENO);
            dup2(outputPipe[1], STDOUT_FILENO);
            dup2(outputPipe[1], STDERR_FILENO);

            close(inputPipe[0]);
            close(inputPipe[1]);
            close(outputPipe[0]);
            close(outputPipe[1]);

            std::vector<std::string> envStrings;
            // Populate the CGI environment expected by the script.
            envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
            envStrings.push_back("REQUEST_METHOD=" + request.getMethod());
            envStrings.push_back("QUERY_STRING=" + request.getQueryString());
            envStrings.push_back("SCRIPT_FILENAME=" + scriptPath);
            envStrings.push_back("SCRIPT_NAME=" + request.getPath());
            envStrings.push_back("REQUEST_URI=" + request.getPath());
            envStrings.push_back("SERVER_PROTOCOL=" + request.getVersion());
            envStrings.push_back("SERVER_SOFTWARE=98Webserv");
            envStrings.push_back("SERVER_NAME=" + vhost.getHost());
            {
                std::stringstream portStream;
                portStream << vhost.getPort();
                envStrings.push_back("SERVER_PORT=" + portStream.str());
            }
            envStrings.push_back("REDIRECT_STATUS=200");
            if (request.getMethod() == "POST")
            {
                std::stringstream lengthStream;
                lengthStream << request.getBody().length();
                envStrings.push_back("CONTENT_LENGTH=" + lengthStream.str());
            }

            const std::map<std::string, std::string>& headers = request.getHeaders();
            for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
            {
                std::string key = it->first;
                for (size_t i = 0; i < key.length(); ++i)
                {
                    if (key[i] == '-')
                        key[i] = '_';
                    else
                        key[i] = std::toupper(static_cast<unsigned char>(key[i]));
                }
                envStrings.push_back("HTTP_" + key + "=" + it->second);
            }

            std::vector<char*> envp;
            for (size_t i = 0; i < envStrings.size(); ++i)
                envp.push_back(const_cast<char*>(envStrings[i].c_str()));
            envp.push_back(NULL);

            std::vector<std::string> argStrings;
            argStrings.push_back(interpreter);
            argStrings.push_back(scriptPath);

            std::vector<char*> argv;
            for (size_t i = 0; i < argStrings.size(); ++i)
                argv.push_back(const_cast<char*>(argStrings[i].c_str()));
            argv.push_back(NULL);

            execve(interpreter.c_str(), &argv[0], &envp[0]);
            _exit(1);
        }

        // Parent process: send the request body to the CGI script.
        close(inputPipe[0]);
        close(outputPipe[1]);

        const std::string& body = request.getBody();
        size_t written = 0;
        while (written < body.length())
        {
            ssize_t bytesWritten = write(inputPipe[1], body.data() + written, body.length() - written);
            if (bytesWritten <= 0)
                break;
            written += static_cast<size_t>(bytesWritten);
        }
        close(inputPipe[1]);

        // Read the CGI output before waiting so we do not deadlock on a full pipe.
        std::string rawOutput;
        int cgiStatus = 0;
        if (!collectCgiOutputWithTimeout(pid, outputPipe[0], &rawOutput, &cgiStatus))
        {
            close(outputPipe[0]);
            return (applyConnectionPolicy(Response::createErrorResponse(504, vhost), client));
        }
        close(outputPipe[0]);

        if (WIFEXITED(cgiStatus) == 0 || WEXITSTATUS(cgiStatus) != 0)
            return (applyConnectionPolicy(Response::createErrorResponse(500, vhost), client));

        // Convert CGI headers/body into the server's Response object.
        Response response = buildResponseFromCgiOutput(rawOutput);
        return (applyConnectionPolicy(response, client));
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

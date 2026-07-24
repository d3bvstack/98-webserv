/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIContext.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/17 12:18:34 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/07/24 20:24:33 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIContext.hpp"
#include "Epoll.hpp"
#include "ClientConnection.hpp"
#include "ServerUtils.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <ctime>

namespace
{
    const unsigned int CGI_TIMEOUT_SECONDS = 5;

    std::string joinPaths(const std::string& left, const std::string& right)
    {
        if (left.empty())
            return right;
        if (right.empty())
            return left;
        if (left[left.length() - 1] == '/' && right[0] == '/')
            return (left + right.substr(1));
        if (left[left.length() - 1] != '/' && right[0] != '/')
            return (left + "/" + right);
        return (left + right);
    }

    bool matchesCgiExtension(const std::string& requestPath, const std::string& extension, size_t* scriptEndPos)
    {
        size_t searchPos = 0;

        while (true)
        {
            size_t extensionPos = requestPath.find(extension, searchPos);
            if (extensionPos == std::string::npos)
                return false;

            size_t extensionEndPos = extensionPos + extension.length();
            if (extensionEndPos == requestPath.length() || requestPath[extensionEndPos] == '/')
            {
                *scriptEndPos = extensionEndPos;
                return true;
            }

            searchPos = extensionPos + 1;
        }
    }

    bool splitCgiPath(const Vhost& vhost, const Request& request,
                      std::string& scriptUrlPath, std::string& pathInfo)
    {
        const std::map<std::string, std::string>& cgi = vhost.getCGI();
        const std::string& requestPath = request.getPath();
        size_t bestScriptEndPos = 0;
        bool found = false;

        for (std::map<std::string, std::string>::const_iterator it = cgi.begin(); it != cgi.end(); ++it)
        {
            size_t scriptEndPos = 0;
            if (matchesCgiExtension(requestPath, it->first, &scriptEndPos))
            {
                if (!found || scriptEndPos > bestScriptEndPos)
                {
                    bestScriptEndPos = scriptEndPos;
                    found = true;
                }
            }
        }

        if (!found)
            return false;

        scriptUrlPath = requestPath.substr(0, bestScriptEndPos);
        pathInfo = requestPath.substr(bestScriptEndPos);
        return true;
    }

    std::string resolveFilesystemPathFromUrlPath(const Location& location, const std::string& requestPath)
    {
        std::string urlPath = requestPath;
        const std::string& locationPath = location.getPath();

        if (!locationPath.empty() && urlPath.compare(0, locationPath.length(), locationPath) == 0)
            urlPath.erase(0, locationPath.length());

        if (urlPath.empty())
            urlPath = "/";

        if (!urlPath.empty() && urlPath[0] == '/')
            urlPath.erase(0, 1);

        std::string resolved = joinPaths(location.getRoot(), urlPath);
        if (resolved.length() > 1 && resolved[resolved.length() - 1] == '/')
            resolved.erase(resolved.length() - 1);
        return resolved;
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
        return normalized;
    }

    std::string trimCopy(const std::string& value)
    {
        size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return "";
        size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::pair<int, std::string> parseCgiStatus(const std::string& statusValue)
    {
        int code = 200;
        std::string reason;
        std::stringstream sstream(statusValue);
        sstream >> code;
        std::getline(sstream, reason);
        reason = trimCopy(reason);
        return std::make_pair(code, reason);
    }

    Response buildResponseFromCgiOutput(const std::string& rawOutput)
    {
        std::string normalized = normalizeLineEndings(rawOutput);
        size_t headerEndPos = normalized.find("\n\n");
        std::string headerBlock = (headerEndPos == std::string::npos)
            ? normalized : normalized.substr(0, headerEndPos);
        std::string body = (headerEndPos == std::string::npos)
            ? "" : normalized.substr(headerEndPos + 2);

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
        for (std::vector<std::pair<std::string, std::string> >::const_iterator it = headers.begin();
             it != headers.end(); ++it)
            response.setHeader(it->first, it->second);
        response.setBody(body);
        return response;
    }

    std::string getRequestExtension(const std::string& path)
    {
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos)
            return "";
        return path.substr(dotPos);
    }

    std::string findCgiInterpreter(const Vhost& vhost, const Request& request)
    {
        std::string scriptUrlPath;
        std::string pathInfo;
        if (!splitCgiPath(vhost, request, scriptUrlPath, pathInfo))
            return "";

        std::string requestExtension = getRequestExtension(scriptUrlPath);
        const std::map<std::string, std::string>& cgi = vhost.getCGI();
        std::map<std::string, std::string>::const_iterator it = cgi.find(requestExtension);
        if (it == cgi.end())
            return "";
        return it->second;
    }
}

CGIContext::CGIContext(const Vhost& vhost, const Location& location,
                       const Request& request, ClientConnection* client)
    : _pid(-1),
    _state(WRITING_BODY),
    _inputOffset(0),
    _outputClosed(false),
    _childExited(false),
    _childStatus(0),
    _headersParsed(false),
    _vhost(vhost),
    _location(location),
    _request(request),
    _client(client),
    _response(200),
    _errorStatusCode(0),
    _startTime(0)
{
    _inputPipe[0] = -1;
    _inputPipe[1] = -1;
    _outputPipe[0] = -1;
    _outputPipe[1] = -1;
}

CGIContext::~CGIContext()
{
    if (_inputPipe[0] != -1) close(_inputPipe[0]);
    if (_inputPipe[1] != -1) close(_inputPipe[1]);
    if (_outputPipe[0] != -1) close(_outputPipe[0]);
    if (_outputPipe[1] != -1) close(_outputPipe[1]);
}

bool CGIContext::setError(int statusCode)
{
    _state = ERROR_STATE;
    _errorStatusCode = statusCode;
    return false;
}

std::vector<std::string> CGIContext::buildEnvironment(const std::string& scriptPath, const std::string& scriptUrlPath)
{
    std::vector<std::string> env;

    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REQUEST_METHOD=" + _request.getMethod());
    env.push_back("QUERY_STRING=" + _request.getQueryString());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SCRIPT_NAME=" + scriptUrlPath);
    env.push_back("PATH_INFO=" + _request.getPath());
    env.push_back("REQUEST_URI=" + _request.getPath());
    env.push_back("SERVER_PROTOCOL=" + _request.getVersion());
    env.push_back("SERVER_SOFTWARE=98Webserv");
    env.push_back("SERVER_NAME=" + _vhost.getHost());

    std::stringstream portStream;
    portStream << _vhost.getPort();
    env.push_back("SERVER_PORT=" + portStream.str());

    env.push_back("REDIRECT_STATUS=200");

    if (_request.getMethod() == "POST")
    {
        std::stringstream lengthStream;
        lengthStream << _request.getBody().length();
        env.push_back("CONTENT_LENGTH=" + lengthStream.str());

        std::map<std::string, std::string>::const_iterator contentTypeIt =
            _request.getHeaders().find("Content-Type");
        if (contentTypeIt != _request.getHeaders().end())
            env.push_back("CONTENT_TYPE=" + contentTypeIt->second);
    }

    const std::map<std::string, std::string>& headers = _request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        std::string key = it->first;
        for (size_t i = 0; i < key.length(); ++i)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = std::toupper(static_cast<unsigned char>(key[i]));
        }
        env.push_back("HTTP_" + key + "=" + it->second);
    }

    return env;
}

void CGIContext::runChildProcess(const std::string& scriptPath, const std::string& scriptUrlPath, const std::string& interpreter)
{
    dup2(_inputPipe[0], STDIN_FILENO);
    dup2(_outputPipe[1], STDOUT_FILENO);
    dup2(_outputPipe[1], STDERR_FILENO);

    close(_inputPipe[0]);
    close(_inputPipe[1]);
    close(_outputPipe[0]);
    close(_outputPipe[1]);

    std::vector<std::string> envStrings = buildEnvironment(scriptPath, scriptUrlPath);

    std::vector<char*> envp;
    for (size_t i = 0; i < envStrings.size(); ++i)
        envp.push_back(const_cast<char*>(envStrings[i].c_str()));
    envp.push_back(NULL);

    std::vector<std::string> argStrings;
    if (!interpreter.empty())
        argStrings.push_back(interpreter);
    argStrings.push_back(scriptPath);

    std::vector<char*> argv;
    for (size_t i = 0; i < argStrings.size(); ++i)
        argv.push_back(const_cast<char*>(argStrings[i].c_str()));
    argv.push_back(NULL);

    const char* execPath = interpreter.empty() ? scriptPath.c_str(): interpreter.c_str();
    execve(execPath, &argv[0], &envp[0]);
    _exit(1);
}

bool CGIContext::start(Epoll& epoll)
{
    if (_location.isReturnSet())
        return setError(500);

    std::string interpreter = findCgiInterpreter(_vhost, _request);

    std::string scriptUrlPath;
    std::string pathInfo;
    if (!splitCgiPath(_vhost, _request, scriptUrlPath, pathInfo))
        return setError(500);

    std::string scriptPath = resolveFilesystemPathFromUrlPath(_location, scriptUrlPath);

    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
        return setError(404);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, _inputPipe) == -1
        || socketpair(AF_UNIX, SOCK_STREAM, 0, _outputPipe) == -1)
        return setError(500);

    _pid = fork();
    if (_pid == -1)
    {
        close(_inputPipe[0]);
        close(_inputPipe[1]);
        close(_outputPipe[0]);
        close(_outputPipe[1]);
        return setError(500);
    }

    if (_pid == 0)
        runChildProcess(scriptPath, scriptUrlPath, interpreter);

    close(_inputPipe[0]);
    _inputPipe[0] = -1;
    close(_outputPipe[1]);
    _outputPipe[1] = -1;

    _startTime = std::time(NULL);
    _requestBody = _request.getBody();

    if (_requestBody.empty())
    {
        close(_inputPipe[1]);
        _inputPipe[1] = -1;
        _state = READING_OUTPUT;
    }
    else
    {
        epoll.addFd(_inputPipe[1], EPOLLOUT | EPOLLERR | EPOLLHUP, this);
    }

    epoll.addFd(_outputPipe[0], EPOLLIN | EPOLLERR | EPOLLHUP, this);
    return true;
}

void CGIContext::onCgiInputWritable(Epoll& epoll)
{
    if (_state != WRITING_BODY || _inputPipe[1] == -1)
        return;

    while (_inputOffset < _requestBody.length())
    {
        ssize_t bytesWritten = send(_inputPipe[1],
                                    _requestBody.data() + _inputOffset,
                                    _requestBody.length() - _inputOffset,
                                    MSG_DONTWAIT | MSG_NOSIGNAL);
        if (bytesWritten > 0)
        {
            _inputOffset += static_cast<size_t>(bytesWritten);
            continue;
        }
        if (bytesWritten <= 0)
            break;
    }

    if (_inputOffset >= _requestBody.length())
    {
        closePipeEnd(epoll, _inputPipe[1]);
        _inputPipe[1] = -1;
        _state = READING_OUTPUT;
    }
}

void CGIContext::onCgiOutputReadable(Epoll& epoll)
{
    if (_state == COMPLETE || _state == ERROR_STATE || _outputPipe[0] == -1)
        return;

    char buffer[65536];
    while (true)
    {
        ssize_t bytesRead = recv(_outputPipe[0], buffer, sizeof(buffer), MSG_DONTWAIT);
        if (bytesRead > 0)
        {
            if (!_headersParsed)
            {
                _outputBuffer.append(buffer, static_cast<size_t>(bytesRead));

                size_t boundary = _outputBuffer.find("\r\n\r\n");
                bool hasCRLF = (boundary != std::string::npos);
                if (!hasCRLF)
                    boundary = _outputBuffer.find("\n\n");

                if (boundary != std::string::npos)
                {
                    size_t bodyStart = hasCRLF ? boundary + 4 : boundary + 2;
                    std::string headerBlock = _outputBuffer.substr(0, boundary);
                    std::string firstBody = _outputBuffer.substr(bodyStart);

                    std::string normalized = normalizeLineEndings(headerBlock);
                    int statusCode = 200;
                    std::vector<std::pair<std::string, std::string> > headers;
                    {
                        std::stringstream headerStream(normalized);
                        std::string line;
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
                                std::pair<int, std::string> st = parseCgiStatus(value);
                                statusCode = st.first;
                            }
                            else if (key != "Content-Length")
                            {
                                headers.push_back(std::make_pair(key, value));
                            }
                        }
                    }

                    _response = Response(statusCode);
                    for (size_t i = 0; i < headers.size(); ++i)
                        _response.setHeader(headers[i].first, headers[i].second);
                    _response = server_utils::applyConnectionPolicy(_response, _client);
                    _response.setChunked();

                    _client->sendHeaders(_response);
                    if (!firstBody.empty())
                        _client->appendToWriteBuffer(Response::encodeChunk(firstBody.data(), firstBody.length()));

                    _headersParsed = true;
                    _outputBuffer.clear();
                }
            }
            else
            {
                _client->appendToWriteBuffer(Response::encodeChunk(buffer, static_cast<size_t>(bytesRead)));
            }
            continue;
        }
        if (bytesRead == 0)
        {
            closePipeEnd(epoll, _outputPipe[0]);
            _outputPipe[0] = -1;
            _outputClosed = true;

            if (_childExited)
            {
                _state = COMPLETE;
                buildResponse();
            }
            return;
        }
        if (bytesRead < 0)
            break;
    }
}

void CGIContext::handleError(Epoll& epoll)
{
    cleanup(epoll);

    if (_pid != -1 && !_childExited)
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, &_childStatus, 0);
        _childExited = true;
    }

    _state = ERROR_STATE;
    _response = server_utils::applyConnectionPolicy(Response::createErrorResponse(502, _vhost), _client);
}

void CGIContext::checkChild(Epoll& epoll)
{
    if (_childExited || _pid == -1)
        return;

    if (_startTime != 0 && std::difftime(std::time(NULL), _startTime) >= CGI_TIMEOUT_SECONDS)
    {
        kill(_pid, SIGKILL);
        waitpid(_pid, &_childStatus, 0);
        _childExited = true;
        cleanup(epoll);
        _state = COMPLETE;
        if (_headersParsed)
        {
            _client->setKeepAlive(false);
            sendFinalChunk();
        }
        else
        {
            _response = server_utils::applyConnectionPolicy(
                Response::createErrorResponse(504, _vhost), _client);
        }
        return;
    }

    int status = 0;
    pid_t result = waitpid(_pid, &status, WNOHANG);
    if (result == _pid)
    {
        _childExited = true;
        _childStatus = status;

        if (_outputClosed)
        {
            _state = COMPLETE;
            buildResponse();
        }
    }
}

bool CGIContext::isComplete() const
{
    return (_state == COMPLETE || _state == ERROR_STATE);
}

ClientConnection* CGIContext::getClient() const
{
    return _client;
}

int CGIContext::getOutputReadFd() const
{
    return _outputPipe[0];
}

int CGIContext::getInputWriteFd() const
{
    return _inputPipe[1];
}

pid_t CGIContext::getPid() const
{
    return _pid;
}

CGIContext::State CGIContext::getState() const
{
    return _state;
}

int CGIContext::getErrorStatusCode() const
{
    return _errorStatusCode;
}

void CGIContext::closePipeEnd(Epoll& epoll, int fd)
{
    if (fd != -1)
    {
        epoll.removeFd(fd);
        close(fd);
    }
}

void CGIContext::cleanup(Epoll& epoll)
{
    if (_inputPipe[1] != -1)
    {
        epoll.removeFd(_inputPipe[1]);
        close(_inputPipe[1]);
        _inputPipe[1] = -1;
    }
    if (_outputPipe[0] != -1)
    {
        epoll.removeFd(_outputPipe[0]);
        close(_outputPipe[0]);
        _outputPipe[0] = -1;
    }
}

void CGIContext::deliverResponse()
{
    if (_headersParsed)
        return;
    if (_response.toString().empty())
        buildResponse();
    _client->addPendingResponse(_response);
}

void CGIContext::buildResponse()
{
    if (_headersParsed)
    {
        if (WIFEXITED(_childStatus) == 0 || WEXITSTATUS(_childStatus) != 0)
            _client->setKeepAlive(false);
        sendFinalChunk();
        return;
    }

    if (WIFEXITED(_childStatus) == 0 || WEXITSTATUS(_childStatus) != 0)
    {
        _response = server_utils::applyConnectionPolicy(Response::createErrorResponse(500, _vhost), _client);
        return;
    }

    _response = buildResponseFromCgiOutput(_outputBuffer);
    _response = server_utils::applyConnectionPolicy(_response, _client);
}

void CGIContext::sendFinalChunk()
{
    _client->appendToWriteBuffer(Response::finalChunk());
}

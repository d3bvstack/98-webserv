#include "CGIContext.hpp"
#include "Epoll.hpp"
#include "ClientConnection.hpp"
#include "ServerUtils.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <ctime>

namespace
{
    const unsigned int CGI_TIMEOUT_SECONDS = 5;

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
        return (response);
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
}

CGIContext::CGIContext(const Vhost& vhost, const Location& location,
                       const Request& request, ClientConnection* client)
    : _pid(-1),
    _state(WRITING_BODY),
    _inputOffset(0),
    _outputClosed(false),
    _childExited(false),
    _childStatus(0),
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

bool CGIContext::start(Epoll& epoll)
{
    if (_location.isReturnSet())
    {
        _state = ERROR_STATE;
        _errorStatusCode = 500;
        return (false);
    }

    std::string interpreter = findCgiInterpreter(_vhost, _request);
    if (interpreter.empty())
    {
        _state = ERROR_STATE;
        _errorStatusCode = 500;
        return (false);
    }

    std::string scriptPath = server_utils::resolveFilesystemPath(_location, _request);

    {
        struct stat st;
        if (stat(scriptPath.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
        {
            _state = ERROR_STATE;
            _errorStatusCode = 404;
            return (false);
        }
    }

    if (pipe(_inputPipe) == -1 || pipe(_outputPipe) == -1)
    {
        _state = ERROR_STATE;
        _errorStatusCode = 500;
        return (false);
    }

    _pid = fork();
    if (_pid == -1)
    {
        close(_inputPipe[0]);
        close(_inputPipe[1]);
        close(_outputPipe[0]);
        close(_outputPipe[1]);
        _state = ERROR_STATE;
        _errorStatusCode = 500;
        return (false);
    }

    if (_pid == 0)
    {
        dup2(_inputPipe[0], STDIN_FILENO);
        dup2(_outputPipe[1], STDOUT_FILENO);
        dup2(_outputPipe[1], STDERR_FILENO);

        close(_inputPipe[0]);
        close(_inputPipe[1]);
        close(_outputPipe[0]);
        close(_outputPipe[1]);

        std::vector<std::string> envStrings;
        envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envStrings.push_back("REQUEST_METHOD=" + _request.getMethod());
        envStrings.push_back("QUERY_STRING=" + _request.getQueryString());
        envStrings.push_back("SCRIPT_FILENAME=" + scriptPath);
        envStrings.push_back("SCRIPT_NAME=" + _request.getPath());
        envStrings.push_back("REQUEST_URI=" + _request.getPath());
        envStrings.push_back("SERVER_PROTOCOL=" + _request.getVersion());
        envStrings.push_back("SERVER_SOFTWARE=98Webserv");
        envStrings.push_back("SERVER_NAME=" + _vhost.getHost());
        {
            std::stringstream portStream;
            portStream << _vhost.getPort();
            envStrings.push_back("SERVER_PORT=" + portStream.str());
        }
        envStrings.push_back("REDIRECT_STATUS=200");
        if (_request.getMethod() == "POST")
        {
            std::stringstream lengthStream;
            lengthStream << _request.getBody().length();
            envStrings.push_back("CONTENT_LENGTH=" + lengthStream.str());
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
    return (true);
}

void CGIContext::onCgiInputWritable(Epoll& epoll)
{
    if (_state != WRITING_BODY || _inputPipe[1] == -1)
        return;

    ssize_t bytesWritten = write(_inputPipe[1],
                                 _requestBody.data() + _inputOffset,
                                 _requestBody.length() - _inputOffset);
    if (bytesWritten > 0)
        _inputOffset += static_cast<size_t>(bytesWritten);

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

    char buffer[4096];
    ssize_t bytesRead = read(_outputPipe[0], buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        _outputBuffer.append(buffer, static_cast<size_t>(bytesRead));
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
        _response = server_utils::applyConnectionPolicy(
            Response::createErrorResponse(504, _vhost), _client);
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
    return (_client);
}

int CGIContext::getOutputReadFd() const
{
    return (_outputPipe[0]);
}

int CGIContext::getInputWriteFd() const
{
    return (_inputPipe[1]);
}

pid_t CGIContext::getPid() const
{
    return (_pid);
}

CGIContext::State CGIContext::getState() const
{
    return (_state);
}

int CGIContext::getErrorStatusCode() const
{
    return (_errorStatusCode);
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
    if (_response.toString().empty())
        buildResponse();
    _client->addPendingResponse(_response);
}

void CGIContext::buildResponse()
{
    if (WIFEXITED(_childStatus) == 0 || WEXITSTATUS(_childStatus) != 0)
    {
        _response = server_utils::applyConnectionPolicy(
            Response::createErrorResponse(500, _vhost), _client);
        return;
    }

    _response = buildResponseFromCgiOutput(_outputBuffer);
    _response = server_utils::applyConnectionPolicy(_response, _client);
}

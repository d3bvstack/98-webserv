#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
#include <ctime>
#include "EventTarget.hpp"
#include "Vhost.hpp"
#include "Location.hpp"
#include "Request.hpp"
#include "Response.hpp"

class ClientConnection;
class Epoll;

class CGIContext : public EventTarget
{
    public:
        enum State
        {
            WRITING_BODY,
            READING_OUTPUT,
            COMPLETE,
            ERROR_STATE
        };

        CGIContext(const Vhost& vhost, const Location& location,
                   const Request& request, ClientConnection* client);
        ~CGIContext();

        bool start(Epoll& epoll);
        void onCgiOutputReadable(Epoll& epoll);
        void onCgiInputWritable(Epoll& epoll);
        void handleError(Epoll& epoll);
        void checkChild(Epoll& epoll);
        void deliverResponse();
        bool isComplete() const;
        int getErrorStatusCode() const;

        ClientConnection* getClient() const;
        int getOutputReadFd() const;
        int getInputWriteFd() const;
        pid_t getPid() const;
        State getState() const;

    private:
        pid_t _pid;
        int _inputPipe[2];
        int _outputPipe[2];
        State _state;

        std::string _requestBody;
        size_t _inputOffset;
        std::string _outputBuffer;
        bool _outputClosed;
        bool _childExited;
        int _childStatus;
        bool _headersParsed;

        const Vhost& _vhost;
        const Location& _location;
        Request _request;
        ClientConnection* _client;

        Response _response;
        int _errorStatusCode;
        time_t _startTime;

        bool setError(int statusCode);
        void runChildProcess(const std::string& scriptPath, const std::string& scriptUrlPath, const std::string& interpreter);
        std::vector<std::string> buildEnvironment(const std::string& scriptPath, const std::string& scriptUrlPath);
        void closePipeEnd(Epoll& epoll, int fd);
        void cleanup(Epoll& epoll);
        void buildResponse();
        void sendFinalChunk();

        CGIContext(const CGIContext&);
        CGIContext& operator=(const CGIContext&);
};

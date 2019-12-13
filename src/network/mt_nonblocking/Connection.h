#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <spdlog/logger.h>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <deque>

#include "protocol/Parser.h"
#include <mutex>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps) : _socket(s), pStorage(ps)
    {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        //_event.events = r_mask;
        _first_offset=0;
        readed_bytes = 0;
        arg_remains = 0;
        command_to_execute = 0;

    }

    inline bool isAlive() const { return true; }

    void Start(std::shared_ptr<spdlog::logger> logger); // prepare to read
    // recomended queue size - 64

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:

    friend class ServerImpl;
    friend class Worker;
    int _socket;
    struct epoll_event _event;
    bool _isAlive;
    std::shared_ptr<spdlog::logger> _logger;
    static const int r_mask = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
    static const int rw_mask = EPOLLIN | EPOLLRDHUP | EPOLLOUT;
    std::shared_ptr<Afina::Storage> pStorage;

    //reader and executor state
    int readed_bytes;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::deque<std::string> _results;
    std::size_t arg_remains;
    Protocol::Parser parser;
    char client_buffer[4096];
    std::string argument_for_command;
    std::mutex mutex;
    //writer state
    int _first_offset;

};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

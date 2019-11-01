#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <spdlog/logger.h>

#include <afina/Storage.h>

#include <deque>
namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps) : _socket(s), pStorage(ps)
    {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        _first_offset=0;
        
    }

    inline bool isAlive() const { return true; }

    void Start(); // prepare to read
    // recomended queue size - 64

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    bool _isAlive;
    std::shared_ptr<spdlog::logger> _logger;
    static const int r_mask = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    static const int rw_mask = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
    std::shared_ptr<Afina::Storage> pStorage;

    std::deque<std::string> _results;
    int _first_offset;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include "Connection.h"

#include <iostream>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <afina/logging/Service.h>


#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h> //writev

#include "protocol/Parser.h"
namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start(std::shared_ptr<spdlog::logger> logger)
{
  std::lock_guard<std::mutex> lock(mutex);

   _logger = logger;
   _event.events = r_mask;
   _event.data.ptr = this;
}

// See Connection.h
void Connection::OnError()
{
//  std::cout << "OnError" << std::endl;
   _isAlive = false;
   close(_socket);
}

// See Connection.h
void Connection::OnClose()
{
  //std::cout << "OnClose" << std::endl;
  close(_socket);
}

// See Connection.h
void Connection::DoRead()
{
  std::lock_guard<std::mutex> lock(mutex);
  //std::cout << "DoRead" << std::endl;
  int client_socket = _socket;

  //from st_blocking
  try {
      int readed_bytes_ci = 0;

      while ((readed_bytes_ci = read(client_socket, client_buffer + readed_bytes, sizeof(client_buffer) - readed_bytes)) > 0) {
          _logger->debug("Got {} bytes from socket", readed_bytes);
          readed_bytes += readed_bytes_ci;

          // Single block of data readed from the socket could trigger inside actions a multiple times,
          // for example:
          // - read#0: [<command1 start>]
          // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
          while (readed_bytes > 0) {
              _logger->debug("Process {} bytes", readed_bytes);

              // There is no command yet
              if (!command_to_execute) {

                  std::size_t parsed = 0;
                  if (parser.Parse(client_buffer, readed_bytes, parsed)) {
                      // There is no command to be launched, continue to parse input stream
                      // Here we are, current chunk finished some command, process it
                      _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);

                      command_to_execute = parser.Build(arg_remains);
                      if (arg_remains > 0) {
                          arg_remains += 2;
                      }
                  }
                  else
                  {
                    //
                  }

                  // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                  // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                  if (parsed == 0) {

                      break;
                  } else {
                      std::memmove(client_buffer, client_buffer + parsed, readed_bytes - parsed);
                      readed_bytes -= parsed;
                  }
              }

              // There is command, but we still wait for argument to arrive...
              if (command_to_execute && arg_remains > 0) {
                  _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);

                  // There is some parsed command, and now we are reading argument
                  std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                  argument_for_command.append(client_buffer, to_read);

                  std::memmove(client_buffer, client_buffer + to_read, readed_bytes - to_read);
                  arg_remains -= to_read;
                  readed_bytes -= to_read;
              }

              // Thre is command & argument - RUN!
              if (command_to_execute && arg_remains == 0) {
                  _logger->debug("Start command execution");

                  std::string result;


                   command_to_execute->Execute(*pStorage, argument_for_command, result);
                   result += "\r\n";
                   bool flag = _results.empty();
                   //add response to results queue
                   _results.push_back(result);
                   //set socket to read-write
                   if(flag){
                     _event.events = rw_mask;
                   }
                   // Prepare for the next command
                   command_to_execute.reset();
                   argument_for_command.resize(0);
                   parser.Reset();

              }
          } // while (readed_bytes)
      }

      if (readed_bytes == 0) {
          _logger->debug("End of read session");
      } else {
          throw std::runtime_error(std::string(strerror(errno)));
      }
  } catch (std::runtime_error &ex) {
      _logger->error("Failed to process connection on descriptor {}: {}", client_socket, ex.what());
  }





}

// See Connection.h
void Connection::DoWrite()
{

  std::lock_guard<std::mutex> lock(mutex);
  //std::cout << "DoWrite" << std::endl;


  struct iovec iovecs[_results.size()];
  size_t i=0;
  for (auto &res : _results)
  {
      iovecs[i].iov_len = res.size();
      iovecs[i].iov_base = reinterpret_cast<void*> (const_cast<char*>(res.c_str()));
      ++i;
  }
  iovecs[0].iov_base = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(iovecs[0].iov_base) + _first_offset);
  iovecs[0].iov_len -= _first_offset;

  ssize_t bw;
  if ((bw = writev(_socket, iovecs, _results.size())) <= 0) {
      _logger->error("Failed to send response");
      return;
  }
  _first_offset += bw;

  auto it = _results.begin();
  _first_offset -= iovecs[0].iov_len;
  ++it;
  while (it != _results.end() && (_first_offset >= it->size())) {
      _first_offset -= it->size();
      ++it;
  }

  _results.erase(_results.begin(), it);
  if (_results.empty()) {
      _event.events = r_mask;
  }

}//DoWrite

} // namespace STnonblock
} // namespace Network
} // namespace Afina

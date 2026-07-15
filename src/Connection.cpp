#include "Connection.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "consts.h"
#include "utils.h"

using std::cout;
using std::endl;

Connection::Connection(std::shared_ptr<Logger> logger)
    : logger_p(std::move(logger)) {}

Connection::~Connection() {
  // Could use EPOLL_CTL_DEL before closing fds, but we let the kernel do it

  /**
   * WARNING: close() may fail/error
   * If you want a proper close without errors or data loss do this:
   * https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
   */
  cout << "Closing S " << fds->server_fd << endl;
  close(fds->server_fd);
  cout << "Closing C " << fds->client_fd << endl;
  close(fds->client_fd);
}

const ProxyConnFds& Connection::create_connection(int epoll_fd,
                                                  int client_sock_fd) {
  int pg_sock_fd = make_socket(CONSTS::postgres_ip, CONSTS::postgres_port);

  struct epoll_event client_epoll_evt{
      .events = EPOLLIN,
      .data = {.fd = client_sock_fd},
  };

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock_fd, &client_epoll_evt) ==
      -1)
    err("Bad client_sock_fd epoll_ctl_add");

  struct epoll_event server_epoll_evt{
      .events = EPOLLIN,
      .data = {.fd = pg_sock_fd},
  };

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pg_sock_fd, &server_epoll_evt) == -1)
    err("Bad client_sock_fd epoll_ctl_add");

  fds = std::make_unique<ProxyConnFds>(
      ProxyConnFds{.client_fd = client_sock_fd, .server_fd = pg_sock_fd});

  return *fds;
}

void Connection::handle_reqest(int sockfd, unsigned char* req, size_t length) {
  int fd{-1};
  if (sockfd == fds->server_fd) {
    cout << "Postgres -> Client ";
    fd = fds->client_fd;
  } else {
    cout << "Client -> Postgres ";
    fd = fds->server_fd;

    // for debug
    // cout << std::format("| send ({}) [{}]: '", sockfd, length);
    // std::cout.write((char *)req, static_cast<long>(length));
    // cout << "'\n";

    // Logging
    merger.add(req, length);
    while (merger.messages.size() > 0) {
      std::string msg = merger.messages.front();

      std::string query = msg.substr(CONSTS::pg_len);
      if (msg[0] == 'Q') {
        cout << "Q query: ";
        cout << std::format("'{}'\n", query);
        logger_p->log(query);
      } else if (msg[0] == 'P') {
        cout << "P query: ";
        /**
         * NOTICE: 'P' query can be parsed to log only 'query' field,
         * but we will print it fully with name and params
         *
         * 'P' Query: structure:
         * P [length: 4 bytes] [statement_name: string] [query: string]
         * [num_param_types: 2 bytes] [param_type_oids: 4 bytes each]
         */
        cout << std::format("'{}'\n", query);
        logger_p->log(query);
      }
      merger.messages.pop();
    }
  }

  my_send(fd, req, length);
}

int Connection::make_socket(const char* ip, uint16_t port) {
  struct sockaddr_in server_addr{};
  int sockfd = -1;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    err("Socket creation failed");
  fill_sockaddr_in(&server_addr, ip, port);

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    err("Connection failed");

  cout << "=== PG sockfd: " << sockfd << endl;
  return sockfd;
}

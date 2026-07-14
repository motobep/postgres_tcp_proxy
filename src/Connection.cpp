#include <iostream>
#include <map>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <utility>

#include "utils.h"

#include "logger.h"

#include "consts.cpp"

using std::cout;
using std::endl;

enum Side { CLIENT, SERVER };
struct ProxyConn {
  int client_fd;
  int server_fd;
  Side side;
};

std::map<int, ProxyConn *> PROXY_CONNECTIONS;

class Connection {

  std::shared_ptr<Logger> logger_p;

  // ProxyConn clientConn;
  // ProxyConn serverConn;

public:
  Connection(std::shared_ptr<Logger> logger) : logger_p(std::move(logger)) {}
  void add_connections_pair(int epoll_fd, int new_fd) {
    int sockfd = make_socket(CONSTS::postgres_ip, CONSTS::postgres_port);

    struct epoll_event new_epoll_evt{
        .events = EPOLLIN,
        .data = {.fd = new_fd},
    };

    PROXY_CONNECTIONS[new_fd] =
        new ProxyConn{.client_fd = new_fd, .server_fd = sockfd, .side = CLIENT};

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &new_epoll_evt) == -1)
      err("Bad new_fd epoll_ctl");

    new_epoll_evt.data.fd = sockfd;
    new_epoll_evt.events = EPOLLIN;

    PROXY_CONNECTIONS[sockfd] =
        new ProxyConn{.client_fd = new_fd, .server_fd = sockfd, .side = SERVER};

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &new_epoll_evt) == -1)
      err("Bad new proxy_to_server listener epoll_ctl");
  }

  void remove_connections_pair(int sockfd) {
    ProxyConn *conn = PROXY_CONNECTIONS[sockfd];
    int pair_fd = conn->side == CLIENT ? conn->server_fd : conn->client_fd;
    ProxyConn *conn_pair = PROXY_CONNECTIONS[pair_fd];

    // Could use EPOLL_CTL_DEL before closing fds, but we let the kernel do it

    // WARNING: close() may fail/error
    cout << "Closing S " << conn->server_fd << endl;
    close(conn->server_fd);
    cout << "Closing C \n" << conn->client_fd << endl;
    close(conn->client_fd);

    delete conn;
    delete conn_pair;

    PROXY_CONNECTIONS.erase(sockfd);
    PROXY_CONNECTIONS.erase(pair_fd);
  }

  void handle_reqest(int sockfd, unsigned char *req, size_t length) {
    ProxyConn *conn = PROXY_CONNECTIONS[sockfd];
    int fd;
    if (conn->side == SERVER) {
      cout << "Postgres -> Client\n";
      fd = conn->client_fd;
    } else {
      cout << "Client -> Postgres\n";
      fd = conn->server_fd;

      cout << "send (" << sockfd << "): '";
      fwrite(req, 1, length, stdout);
      cout << "'\n";

      std::string buf{(char *)req};
      logger_p->log(buf);
    }
    my_send(fd, req, length);
  }

private:
  int make_socket(const char *ip, uint16_t port) {
    struct sockaddr_in server_addr{};
    int sockfd = -1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      err("Socket creation failed");
    fill_sockaddr_in(&server_addr, ip, port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0)
      err("Connection failed");

    cout << "=== PG sockfd: " << sockfd << endl;
    return sockfd;
  }
};

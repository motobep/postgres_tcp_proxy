#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "Connection.cpp"
#include "utils.h"

#define LOGGER_IMPLEMENTATION
#include "logger.h"

#define MAX_EPOLL_EVENTS 32

using std::cout;
using std::endl;

struct sockaddr_in server_addr;
socklen_t server_addr_len = sizeof(struct sockaddr_in);

struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(struct sockaddr_in);

FILE *LOG_FP;

std::map<int, Connection *> CONNECTIONS;

int make_tcp_listener(const char *server_ip, uint16_t server_port) {
  fill_sockaddr_in(&server_addr, server_ip, server_port);

  int listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // printf("tcp_listener_fd: %d\n", listener_fd);
  if (listener_fd == -1)
    err("Bad socket");

  int opt_val = 1;

  int ok1 = setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val,
                       sizeof(opt_val));
  if (ok1 != 0)
    err("Bad setsockopt");

  int ok2 =
      bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ok2 != 0)
    err("Bad bind");

  cout << "Tcp Start listening\n";
  listen(listener_fd, SOMAXCONN);
  return listener_fd;
}

bool str_to_uint16(const char *str, uint16_t *res) {
  char *end{};
  errno = 0;
  long val = strtol(str, &end, 10);
  if (errno || end == str || *end != '\0' || val < 0 || val >= 0x10000) {
    return false;
  }
  *res = (uint16_t)val;
  return true;
}

int main(int argc, char *argv[]) {
  // WARNING: May error on bad args
  char *log_file = argv[1];
  char *server_ip = argv[2];

  uint16_t server_port;
  if (!str_to_uint16(argv[3], &server_port)) {
    return 1;
  }

  LOG_FP = logger_init(log_file);
  if (!LOG_FP)
    return 1;

  unsigned char msg[BUFFER_SIZE];
  ssize_t msg_len = -1;
  int new_fd = -1;

  int eventsCount = 0;
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_fd error");
    exit(1);
  }
  struct epoll_event epollEvents[MAX_EPOLL_EVENTS];

  // Set TCP listener
  int tcp_listener = make_tcp_listener(server_ip, server_port);

  struct epoll_event server_evt{.events = EPOLLIN,
                                .data = {.fd = tcp_listener}};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_listener, &server_evt) == -1)
    err("Bad tcp_listener epoll_ctl");

  int sockfd = -1;

  cout << "Epolling\n";

  while (true) {
    const int epoll_timeout = -1;
    // Uncomment bello If you want Non-blocking epoll behaviour
    // const int epoll_timeout = 500;
    eventsCount =
        epoll_wait(epoll_fd, epollEvents, MAX_EPOLL_EVENTS, epoll_timeout);
    if (epoll_timeout != -1 && eventsCount == 0) {
      cout << "Non-blocking. Doing other stuff\n";
    }

    for (int i = 0; i < eventsCount; i++) {
      uint32_t events = epollEvents[i].events;
      sockfd = epollEvents[i].data.fd;
      if (events & EPOLLIN) {
        if (sockfd == tcp_listener) {
          cout << "Accept connection\n";
          new_fd = accept(tcp_listener, (struct sockaddr *)&client_addr,
                          &client_addr_len);

          if (new_fd == -1)
            err("Bad new_fd");

          if (new_fd > 0) {
            Connection connection{LOG_FP};
            connection.add_connections_pair(epoll_fd, new_fd);
            CONNECTIONS[new_fd] = &connection;

            cout << "Added new_fd+events: " << new_fd << endl;
          } else {
            perror("accept error");
          }
        } else {
          // cout << "Handle tcp client message\n";

          msg_len = recv(sockfd, msg, sizeof(msg), 0);
          // cout << "\tafter recv\n";
          auto conn = CONNECTIONS[sockfd];
          if (msg_len > 0) {
            msg[msg_len] = 0;
            cout << ">> Proxying fd " << sockfd << endl;
            // print_bytes_as_hex(msg, msg_len);
            conn->handle_reqest(sockfd, msg, msg_len);
            cout << "<<\n\n";
          } else {
            cout << "Hanging\n";
            conn->remove_connections_pair(sockfd);
          }
        }
      } else if (events & EPOLLERR || events & EPOLLHUP) {
        cout << "Error or hup\n";
        auto conn = CONNECTIONS[sockfd];
        conn->remove_connections_pair(sockfd);
      }
    }
  }

  return logger_close(LOG_FP);
}

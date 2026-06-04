#include <arpa/inet.h>
#include <errno.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

#define LOGGER_IMPLEMENTATION
#include "logger.h"

#define POSTGRES_IP "127.0.0.1"
#define POSTGRES_PORT 6432

#define MAX_EPOLL_EVENTS 32

struct sockaddr_in server_addr;
socklen_t server_addr_len = sizeof(struct sockaddr_in);

struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(struct sockaddr_in);

enum Side { CLIENT, SERVER };
struct ProxyConn {
  int client_fd;
  int server_fd;
  Side side;
};

std::map<int, ProxyConn *> PROXY_CONNECTIONS;
FILE *LOG_FP;

int make_socket(const char *ip, uint16_t port) {
  struct sockaddr_in server_addr;
  int sockfd;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    err("Socket creation failed");
  fill_sockaddr_in(&server_addr, ip, port);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    err("Connection failed");

  printf("=== PG sockfd: %d\n", sockfd);
  return sockfd;
}

void add_connections_pair(int epoll_fd, int new_fd) {
  int sockfd = make_socket(POSTGRES_IP, POSTGRES_PORT);

  struct epoll_event new_epoll_evt;
  new_epoll_evt.data.fd = new_fd;
  new_epoll_evt.events = EPOLLIN;

  PROXY_CONNECTIONS[new_fd] = new ProxyConn{new_fd, sockfd, CLIENT};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &new_epoll_evt) == -1)
    err("Bad new_fd epoll_ctl");

  new_epoll_evt.data.fd = sockfd;
  new_epoll_evt.events = EPOLLIN;

  PROXY_CONNECTIONS[sockfd] = new ProxyConn{new_fd, sockfd, SERVER};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &new_epoll_evt) == -1)
    err("Bad new proxy_to_server listener epoll_ctl");
}

void remove_connections_pair(int sockfd) {
  ProxyConn *conn = PROXY_CONNECTIONS[sockfd];
  int pair_fd = conn->side == CLIENT ? conn->server_fd : conn->client_fd;
  ProxyConn *conn_pair = PROXY_CONNECTIONS[pair_fd];

  // Could use EPOLL_CTL_DEL before closing fds, but we let the kernel do it

  // WARNING: close() may fail/error
  printf("Closing S %d\n", conn->server_fd);
  close(conn->server_fd);
  printf("Closing C %d\n", conn->client_fd);
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
    printf("Postgres -> Client\n");
    fd = conn->client_fd;
  } else {
    printf("Client -> Postgres\n");
    fd = conn->server_fd;

    printf("send (%d): '", sockfd);
    fwrite(req, 1, length, stdout);
    printf("'\n");

    char query_buffer[MAX_QUERY_SIZE];
    int result = process_message(req, length, query_buffer, MAX_QUERY_SIZE);
    if (result > 0) {
      log_query(LOG_FP, query_buffer);
    }
  }
  my_send(fd, req, length);
}

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

  printf("Tcp Start listening\n");
  listen(listener_fd, SOMAXCONN);
  return listener_fd;
}

bool str_to_uint16(const char *str, uint16_t *res) {
  char *end;
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
  int msg_len;
  int new_fd;

  int eventsCount = 0;
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_fd error");
    exit(1);
  }
  struct epoll_event epollEvents[MAX_EPOLL_EVENTS];

  struct epoll_event server_evt;
  server_evt.events = EPOLLIN;

  // Set TCP listener
  int tcp_listener = make_tcp_listener(server_ip, server_port);
  server_evt.data.fd = tcp_listener;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_listener, &server_evt) == -1)
    err("Bad tcp_listener epoll_ctl");

  int sockfd;

  printf("Epolling\n");

  while (1) {
    const int epoll_timeout = -1;
    // Uncomment bello If you want Non-blocking epoll behaviour
    // const int epoll_timeout = 500;
    eventsCount =
        epoll_wait(epoll_fd, epollEvents, MAX_EPOLL_EVENTS, epoll_timeout);
    if (epoll_timeout != -1 && eventsCount == 0) {
      printf("Non-blocking. Doing other stuff\n");
    }

    for (int i = 0; i < eventsCount; i++) {
      // printf("Event %d for fd %d\n", epollEvents[i].events,
      //        epollEvents[i].data.fd);
      uint32_t events = epollEvents[i].events;
      if (events & EPOLLIN) {
        sockfd = epollEvents[i].data.fd;
        if (sockfd == tcp_listener) {
          printf("Accept connection\n");
          new_fd = accept(tcp_listener, (struct sockaddr *)&client_addr,
                          &client_addr_len);

          if (new_fd == -1)
            err("Bad new_fd");

          if (new_fd > 0) {
            add_connections_pair(epoll_fd, new_fd);

            printf("Added new_fd+events: %d\n", new_fd);
          } else {
            perror("accept error");
          }
        } else {
          // printf("Handle tcp client message\n");

          msg_len = recv(sockfd, msg, sizeof(msg), 0);
          // printf("\tafter recv\n");
          if (msg_len > 0) {
            msg[msg_len] = 0;
            printf(">> Proxying fd %d\n", sockfd);
            // print_bytes_as_hex(msg, msg_len);
            handle_reqest(sockfd, msg, msg_len);
            printf("<<\n\n");
          } else {
            printf("Hanging\n");
            remove_connections_pair(sockfd);
          }
        }
      } else if (events & EPOLLERR || events & EPOLLHUP) {
        printf("Error or hup\n");
        remove_connections_pair(sockfd);
      }
    }
  }

  return 0;
}

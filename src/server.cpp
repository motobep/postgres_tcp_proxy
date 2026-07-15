#include <sys/epoll.h>

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <span>

#include "Connection.h"
#include "Logger.h"
#include "consts.h"
#include "utils.h"

using std::cout;
using std::endl;

int make_tcp_listener(const char* server_ip, uint16_t server_port);
bool str_to_uint16(const char* str, uint16_t* res);

class ConnectionsPool {
  std::map<int, std::shared_ptr<Connection>> connections;

 public:
  std::shared_ptr<Connection> get(int sockfd) { return connections[sockfd]; }
  void add(std::shared_ptr<Connection> conn) {
    connections[conn->fds->client_fd] = conn;
    connections[conn->fds->server_fd] = conn;
  }
  void remove(int sockfd) {
    auto conn = connections[sockfd];
    connections.erase(conn->fds->client_fd);
    connections.erase(conn->fds->server_fd);
  }
};

int main(int argc, char* argv[]) {
  // WARNING: May error on bad args
  std::span<char*> args(argv, argc);
  char* log_file = args[1];
  char* server_ip = args[2];

  uint16_t server_port{0};
  if (!str_to_uint16(args[3], &server_port)) {
    return 1;
  }

  std::shared_ptr<Logger> logger_p = std::make_shared<Logger>(log_file);
  if (!logger_p->is_open()) {
    perror("logger failed to open");
    return 1;
  }

  std::array<unsigned char, CONSTS::buffer_size> msg{};
  ssize_t msg_len{-1};
  int new_fd{-1};

  int eventsCount = 0;
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_fd error");
    return 1;
  }
  std::array<struct epoll_event, CONSTS::max_epoll_events> epollEvents{};

  // Set TCP listener
  int tcp_listener = make_tcp_listener(server_ip, server_port);

  struct epoll_event server_evt{.events = EPOLLIN,
                                .data = {.fd = tcp_listener}};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_listener, &server_evt) == -1)
    err("Bad tcp_listener epoll_ctl");

  int sockfd = -1;

  cout << "Epolling\n";

  struct sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(struct sockaddr_in);

  ConnectionsPool connectionsPool;

  while (true) {
    const int epoll_timeout = -1;
    // Uncomment bellow If you want Non-blocking epoll behaviour
    // const int epoll_timeout = 500;
    eventsCount = epoll_wait(epoll_fd, epollEvents.data(),
                             CONSTS::max_epoll_events, epoll_timeout);
    if (epoll_timeout != -1 && eventsCount == 0) {
      cout << "Non-blocking. Doing other stuff\n";
    }

    for (int i = 0; i < eventsCount; i++) {
      uint32_t events = epollEvents.at(i).events;
      sockfd = epollEvents.at(i).data.fd;
      if (events & EPOLLIN) {
        if (sockfd == tcp_listener) {
          cout << "Accept connection\n";
          new_fd = accept(tcp_listener, (struct sockaddr*)&client_addr,
                          &client_addr_len);

          if (new_fd == -1) err("Bad new_fd");

          if (new_fd > 0) {
            auto conn = std::make_shared<Connection>(logger_p);
            conn->create_connection(epoll_fd, new_fd);
            connectionsPool.add(conn);

            cout << "Added new_fd+events: " << new_fd << endl;
          } else {
            perror("accept error");
          }
        } else {
          // cout << "Handle tcp client message\n";
          msg_len = recv(sockfd, msg.data(), CONSTS::buffer_size, 0);
          if (msg_len > 0) {
            cout << std::format(">> (Proxying fd {} [{}]) ", sockfd, msg_len);
            // print_bytes_as_hex(msg, msg_len);
            auto conn = connectionsPool.get(sockfd);
            conn->handle_reqest(sockfd, msg.data(), msg_len);
            cout << "<<\n\n";
          } else {
            cout << "Hanging\n";
            connectionsPool.remove(sockfd);
          }
        }
      } else if (events & EPOLLERR || events & EPOLLHUP) {
        cout << "Error or hup\n";
        connectionsPool.remove(sockfd);
      }
    }
  }

  return 0;
}

int make_tcp_listener(const char* server_ip, uint16_t server_port) {
  struct sockaddr_in server_addr{};
  fill_sockaddr_in(&server_addr, server_ip, server_port);

  int listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // printf("tcp_listener_fd: %d\n", listener_fd);
  if (listener_fd == -1) err("Bad socket");

  int opt_val{1};

  int ok1 = setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val,
                       sizeof(opt_val));
  if (ok1 != 0) err("Bad setsockopt");

  int ok2 =
      bind(listener_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (ok2 != 0) err("Bad bind");

  cout << "Tcp Start listening\n";
  listen(listener_fd, SOMAXCONN);
  return listener_fd;
}

bool str_to_uint16(const char* str, uint16_t* res) {
  char* end{};
  errno = 0;
  const int base_ten{10};
  long val = strtol(str, &end, base_ten);
  const uint max_uint16{0x10000};
  if (errno || end == str || *end != '\0' || val < 0 || val >= max_uint16) {
    return false;
  }
  *res = static_cast<uint16_t>(val);
  return true;
}

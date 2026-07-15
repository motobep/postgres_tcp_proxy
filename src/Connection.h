#include <cstdint>
#include <memory>

#include "Logger.h"
#include "Merger.h"

struct ProxyConnFds {
  int client_fd;
  int server_fd;
};

class Connection {
  std::shared_ptr<Logger> logger_p;
  Merger merger{};

public:
  std::unique_ptr<ProxyConnFds> fds;

  Connection(std::shared_ptr<Logger> logger);
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  Connection(Connection &&) = delete;
  Connection &operator=(Connection &&) = delete;

  ~Connection();

  const ProxyConnFds &create_connection(int epoll_fd, int client_sock_fd);

  void handle_reqest(int sockfd, unsigned char *req, size_t length);

private:
  int make_socket(const char *ip, uint16_t port);
};

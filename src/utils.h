#ifndef UTILS
#define UTILS

#include <arpa/inet.h>
#include <cstdio>

#include <iostream>
#include <format>

#include "./consts.cpp"


using std::cout;
using std::endl;

inline void err(const char *msg) {
  perror(msg);
  exit(1);
}

inline void print_bytes_as_hex(const unsigned char *buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
      cout << std::format("{:02X} ", buffer[i]);
  }
  cout << "\n";
}

inline void fill_sockaddr_in(struct sockaddr_in *server_addr, const char *ip,
                             uint16_t port) {
  struct in_addr local_ip{};
  int ok = inet_pton(AF_INET, ip, &local_ip);
  if (ok <= 0)
    err("Invalid address");

  server_addr->sin_addr = local_ip;
  server_addr->sin_port = htons(port);
  server_addr->sin_family = AF_INET;

  cout << std::format("Filled server sockaddr_in for '{}:{}'\n", ip, port);
}

inline ssize_t my_send(int sockfd, const unsigned char *buffer, size_t length) {
  // print_bytes_as_hex(buffer, length);
  return send(sockfd, buffer, length, 0);
}

/* inline ssize_t my_recv(int sockfd, unsigned char *buffer) {
  int bytes_received = recv(sockfd, buffer, CONSTS::buffer_size - 1, 0);
  if (bytes_received < 0)
    err("Receive failed");

  buffer[bytes_received] = 0;
  // print_bytes_as_hex(buffer, bytes_received);
  return bytes_received;
} */

#endif

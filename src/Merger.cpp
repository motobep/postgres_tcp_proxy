#include "Merger.h"

#include <arpa/inet.h>

#include <format>
#include <iostream>
#include <queue>

#include "consts.h"

using std::cout;
using std::endl;

void Merger::add(unsigned char* req, size_t length) {
  buf += std::string((char*)req, length);
  if (buf.size() < CONSTS::pg_len) {
    return;
  }

  read_len();

  while (has_enough()) {
    read_buffer();
    if (is_startup) {
      // Drop startup messages
      messages.pop();
      update_is_startup();
    }
    if (buf.size() > CONSTS::pg_len) {
      read_len();
    }
  }
}

bool Merger::has_enough() {
  return buf.size() > CONSTS::pg_len && buf.size() >= len;
}

void Merger::update_is_startup() {
  if (len == CONSTS::pg_ssl_msg_len) {
    int magic_number = ntohl(*(uint32_t*)(buf.c_str() + 4));
    if (magic_number != CONSTS::pg_ssl_magic_number) {
      is_startup = false;
    }
  } else {
    is_startup = false;
  }
}

uint32_t Merger::read_len() {
  if (is_startup) {
    len = ntohl(*(uint32_t*)(buf.c_str()));
    cout << std::format("startup: ");
  } else {
    // Fresh start of a new message
    // cout << std::format("fresh: ");
    len = ntohl(*(uint32_t*)(buf.c_str() + 1)) + CONSTS::pg_type_len;
  }

  cout << std::format("(buf: {}, len: {})\n", buf.size(), len);
  return len;
}

void Merger::read_buffer() {
  messages.push(buf.substr(0, len));  // read part
  buf = buf.substr(len);              // left part
}

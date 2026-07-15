#ifndef CONSTS_FILE
#define CONSTS_FILE

namespace CONSTS {
constexpr int buffer_size{65535}; // note: 1 byte buffer also works
constexpr int max_epoll_events{32};
constexpr int pg_len{5};
constexpr int pg_type_len{1};
constexpr int postgres_port{6432};
const char *const postgres_ip{"127.0.0.1"};
constexpr int pg_ssl_magic_number{0x04D2162F};
constexpr int pg_ssl_msg_len{8};
} // namespace CONSTS

#endif

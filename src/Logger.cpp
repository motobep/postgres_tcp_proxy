#include <arpa/inet.h>

#include <chrono>
#include <format>
#include <iostream>

#include "Logger.h"

using std::cout;

Logger::Logger(const char *filename) : fout(std::ofstream(filename)) {}

bool Logger::is_open() { return fout.is_open(); }

void Logger::log_query(const char *query) {
  auto now = std::chrono::system_clock::now();
  fout << std::format("[{:%Y-%m-%d %H:%M:%S}] Query: {}\n", now, query);
  fout.flush();
}

Logger::~Logger() { fout.close(); }

bool Logger::log(const std::string &req) {
  std::string query_buffer(CONSTS::max_query_size, ' ');
  int result = process_message(req, query_buffer);
  if (result > 0) {
    log_query(query_buffer.data());
    return true;
  }
  return false;
}

int Logger::process_message(const std::string &buffer, std::string &query_out) {
  if (buffer.size() < CONSTS::pg_len)
    return -1;

  int result = 0;

  if (buffer[0] == 'Q') {
    cout << "Q query\n";
    result = parse_simple_query(buffer, query_out);
  } else if (buffer[0] == 'P') {
    cout << "P query\n";
    result = parse_prepared_query(buffer, query_out);
  }

  return result;
}

int Logger::parse_simple_query(const std::string &buffer,
                               std::string &query_out) {
  uint32_t length = ntohl(*(uint32_t *)(buffer.c_str() + 1));
  if (length < CONSTS::pg_len || length > buffer.size() - 1)
    return -1;

  std::copy(buffer.begin() + CONSTS::pg_len, buffer.end(), query_out.begin());

  return 1;
}

/**
 * P [length: 4 bytes] [statement_name: string] [query: string]
 * [num_param_types: 2 bytes] [param_type_oids: 4 bytes each]
 */
int Logger::parse_prepared_query(const std::string &buffer,
                                 std::string &query_out) {
  uint32_t length = ntohl(*(uint32_t *)(buffer.c_str() + 1));

  if (length < CONSTS::pg_len || length > buffer.size() - 1)
    return -1;

  // const unsigned char *ptr = buffer + CONSTS::pg_len;
  // const unsigned char *end = buffer + 1 + length;
  //
  // // Skip name
  // ptr += strlen((const char *)ptr) + 1;
  // if (ptr >= end)
  // if (buffer.size() >= length)
  //   return -1;
  //
  // // Getting query
  // const char *query = reinterpret_cast<const char *>(ptr);
  // size_t query_length = strlen(query);
  //
  // if (query_length >= query_out.size()) {
  //   query_length = query_out.size() - 1;
  // }
  //
  // query_out = query;
  // query_out[query_length] = '\0';
  std::copy(buffer.begin() + CONSTS::pg_len, buffer.end(), query_out.begin());

  cout.write(query_out.data(), static_cast<long>(query_out.size()));

  return 1;
}

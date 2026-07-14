#ifndef LOGGER
#define LOGGER

#include <arpa/inet.h>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <format>
#include <fstream>
#include <iostream>

#include <array>
#include <string>

#include "./consts.cpp"

using std::cout;

class Logger {
  std::ofstream fout;

public:
  Logger(const char *filename);

  void log_query(const char *query);

  ~Logger();

  bool log(const std::string &req);

private:
  int process_message(const std::string &buffer, std::string &query_out);

  int parse_simple_query(const std::string &buffer, std::string &query_out);

  int parse_prepared_query(const std::string &buffer, std::string &query_out);
};

#endif

#ifndef LOGGER
#define LOGGER

#include <fstream>
#include <string>

#include "consts.h"

class Logger {
  std::ofstream fout;

public:
  Logger(const char *filename);
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;

  bool is_open();
  void log_query(const char *query);

  ~Logger();

  bool log(const std::string &req);

private:
  int process_message(const std::string &buffer, std::string &query_out);

  int parse_simple_query(const std::string &buffer, std::string &query_out);

  int parse_prepared_query(const std::string &buffer, std::string &query_out);
};

#endif

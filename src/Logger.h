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

  ~Logger();

  bool is_open();
  void log(const std::string &query);
};

#endif

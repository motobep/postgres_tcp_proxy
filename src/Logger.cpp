#include <arpa/inet.h>

#include <chrono>
#include <format>
#include <iostream>

#include "Logger.h"

using std::cout;

Logger::Logger(const char *filename) : fout(std::ofstream(filename)) {}
Logger::~Logger() { fout.close(); }

bool Logger::is_open() { return fout.is_open(); }

void Logger::log(const std::string &query) {
  auto now = std::chrono::system_clock::now();
  fout << std::format("[{:%Y-%m-%d %H:%M:%S}] Query: {}\n", now, query);
  fout.flush();
}

/**
 * P [length: 4 bytes] [statement_name: string] [query: string]
 * [num_param_types: 2 bytes] [param_type_oids: 4 bytes each]
 */

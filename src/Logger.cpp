#include "Logger.h"

#include <arpa/inet.h>

#include <chrono>
#include <format>
#include <iostream>

using std::cout;

Logger::Logger(const char* filename) : fout(std::ofstream(filename)) {}
Logger::~Logger() { fout.close(); }

bool Logger::is_open() { return fout.is_open(); }

void Logger::log(const std::string& query) {
  auto now = std::chrono::system_clock::now();
  fout << std::format("[{:%Y-%m-%d %H:%M:%S}] Query: {}\n", now, query);
  fout.flush();
}

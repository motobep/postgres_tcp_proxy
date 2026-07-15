#ifndef MERGER
#define MERGER

#include <cstdint>
#include <iostream>
#include <queue>

class Merger {
  uint32_t len{0};
  std::string buf = "";
  bool is_startup = true;

public:
  std::queue<std::string> messages{};

  void add(unsigned char *req, size_t length);

private:
  bool has_enough();

  void update_is_startup();

  uint32_t read_len();

  void read_buffer();
};

#endif


#include <iostream>
#include <spdlog/spdlog.h>

#include "args.hpp"
#include "hdl/hdl_parser.hpp"
#include "util/string.hpp"


auto main(int argc, char *argv[]) -> int {
  spdlog::info("Hello World!");
  spdlog::info("Exiting.");

  return 0;
}

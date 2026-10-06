
#include <iostream>
#include <spdlog/spdlog.h>

#include "args.h"
#include "hdl/hdl_parser.h"
#include "util/string.h"


auto main(int argc, char *argv[]) -> int {
  spdlog::info("Hello World!");
  spdlog::info("Exiting.");

  return 0;
}

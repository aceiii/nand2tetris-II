#pragma once

#include <expected>
#include <string>


namespace app {
  struct Args {
    std::string filename;
    std::string log_level;
  };

  std::expected<Args, std::string> GetArgs(const std::string& name, const std::string& version, int argc, char** argv);
};

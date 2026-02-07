#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>
#include "hdl_parser.h"

namespace fs = std::filesystem;

std::expected<void, std::string> hdl::Parser::Parse(std::string_view filename) {
  fs::path abs_path = fs::absolute(filename);

  std::ifstream file(abs_path, std::ios::in);
  if (!file.is_open()) {
    const std::string error_msg = std::format("Failed to open file '{}': {}", abs_path.string(), strerror(errno));
    return std::unexpected{error_msg};
  }

  spdlog::info("Parsing file: {}", abs_path.string());

  auto size = fs::file_size(abs_path);
  std::vector<char> buffer(size);
  file.read(buffer.data(), size);
  file.close();
  spdlog::debug("Read {} bytes", size);

  // TODO:
  name_ = "FOO";
  in_ = { "a", "b", "c" };
  out_ = { "out", "x", "y" };
  parts_ = {
    {
      .name = "AND",
      .bindings = {{ "a", "b" }, { "x", "y" }, { "u", "v" }},
    },
    {
      .name = "XOR",
      .bindings = {{ "x", "b" }, { "a", "y" }, { "out", "out" }},
    },
  };

  return {};
}

const std::string & hdl::Parser::Name() const {
  return name_;
}

const std::vector<std::string> & hdl::Parser::InPorts() const {
  return in_;
}

const std::vector<std::string> & hdl::Parser::OutPorts() const {
  return out_;
}

const std::vector<hdl::Part> & hdl::Parser::Parts() const {
  return parts_;
}

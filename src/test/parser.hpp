#pragma once

#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "command.hpp"

namespace test {

  using TestParserResult = std::expected<std::vector<TestCommand>, std::string>;

  class TestParser final {
  public:
    TestParser() = delete;

    static TestParserResult Parse(std::string_view contents);
  };
}

#pragma once

#include <expected>
#include <string>
#include <string_view>

#include "chip.hpp"


namespace hdl {

  class Parser final {
  public:
    Parser() = delete;

    static std::expected<Chip, std::string> Parse(std::string_view contents);
  };

}

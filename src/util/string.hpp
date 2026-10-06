#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <sstream>


namespace util::string {

  std::string Join(const auto& strings, std::string_view separator = "") {
    std::stringstream ss;
    int idx = 0;
    for (const auto& str: strings) {
      if (idx) {
        ss << separator;
      }
      ss << str;
      idx += 1;
    }
    return ss.str();
  }

  std::string Join(const auto& strings, std::string_view separator, auto transformer) {
    std::stringstream ss;
    int idx = 0;
    for (const auto& str: strings) {
      if (idx) {
        ss << separator;
      }
      ss << transformer(str);
      idx += 1;
    }
    return ss.str();
  }
}

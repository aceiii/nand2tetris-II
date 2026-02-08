#ifndef NAND2TETRIS_STRING_H
#define NAND2TETRIS_STRING_H

#include <functional>
#include <string>
#include <string_view>
#include <sstream>


namespace util {

  std::string StrJoin(const auto& strings, std::string_view separator = "") {
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

  std::string StrJoin(const auto& strings, std::string_view separator, auto transformer) {
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




#endif //NAND2TETRIS_STRING_H

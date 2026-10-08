#pragma once

#include <expected>
#include <string>
#include <string_view>


namespace hdl {
  struct Bus {
    std::string name;
    size_t start;
    size_t end;

    size_t Width() const {
      const int width = end - start;
      if (width < 1) {
        return 0;
      }
      return width;
    }
  };

  struct PortBinding {
    Bus left;
    Bus right;
  };

  struct Part {
    std::string name;
    std::vector<PortBinding> bindings;
  };

  struct Port {
    std::string name;
    size_t width;
  };

  struct Chip {
    std::string name;
    std::vector<Port> in;
    std::vector<Port> out;
    std::vector<Part> parts;
  };

  class Parser final {
  public:
    Parser() = delete;

    static std::expected<Chip, std::string> Parse(std::string_view contents);
  };
}

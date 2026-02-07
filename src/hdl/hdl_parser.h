#ifndef NAND2TETRIS_HDLPARSER_H
#define NAND2TETRIS_HDLPARSER_H

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

  class Parser {
  public:
    std::expected<void, std::string> Parse(std::string_view filename);
    const std::string& Name() const;
    const std::vector<Port>& InPorts() const;
    const std::vector<Port>& OutPorts() const;
    const std::vector<Part>& Parts() const;

  private:
    std::string name_;
    std::vector<Port> in_;
    std::vector<Port> out_;
    std::vector<Part> parts_;
  };
}

#endif //NAND2TETRIS_HDLPARSER_H

#ifndef NAND2TETRIS_HDLPARSER_H
#define NAND2TETRIS_HDLPARSER_H

#include <expected>
#include <string>
#include <string_view>

namespace hdl {
  struct PortBinding {
    std::string input;
    std::string output;
  };

  struct Part {
    std::string name;
    std::vector<PortBinding> bindings;
  };

  class Parser {
  public:
    std::expected<void, std::string> Parse(std::string_view filename);
    const std::string& Name() const;
    const std::vector<std::string>& InPorts() const;
    const std::vector<std::string>& OutPorts() const;
    const std::vector<Part>& Parts() const;

  private:
    std::string name_;
    std::vector<std::string> in_;
    std::vector<std::string> out_;
    std::vector<Part> parts_;
  };
}

#endif //NAND2TETRIS_HDLPARSER_H

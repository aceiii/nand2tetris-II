#pragma once

#include <expected>
#include <filesystem>
#include <string>


namespace hdl::test {

  enum class CommandType {
    Load,
    OutputFile,
    OutputList,
    CompareTo,
    Set,
    Eval,
    Output,
  };

  struct TestCommand {
    CommandType type;
    std::string filename;
    std::vector<std::string> patterns;
    std::string ident;
    std::string value;
  };

  using TestParserResult = std::expected<std::vector<TestCommand>, std::string>;

  class TestParser final {
  public:
    TestParser() = delete;

    static TestParserResult Parse(std::filesystem::path path);
  };
}

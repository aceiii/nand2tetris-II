#pragma once

#include <expected>
#include <optional>
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

  enum class PatternFormat {
    Binary,
    Hexadecimal,
    Decimal,
    String,
  };

  struct Pattern {
    PatternFormat format;
    size_t width;
    size_t left;
    size_t right;
  };

  struct Column {
    std::string name;
    std::optional<Pattern> pattern;
  };

  struct TestCommand {
    CommandType type;
    std::string filename;
    std::vector<Column> columns;
    std::string ident;
    std::string value;
  };

  using TestParserResult = std::expected<std::vector<TestCommand>, std::string>;

  class TestParser final {
  public:
    TestParser() = delete;

    static TestParserResult Parse(std::string_view contents);
  };
}

#pragma once

#include <optional>
#include <string>
#include <vector>


namespace test {

  enum class CommandType {
    Load,
    OutputFile,
    OutputList,
    CompareTo,
    Set,
    Eval,
    Output,
    Tick,
    Tock,
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
    int64_t value;
  };

  auto FormatPatternFormat(const test::PatternFormat& format) -> char;
  auto FormatPattern(const test::Pattern& pattern) -> std::string;
  auto FormatColumn(const test::Column& column) -> std::string;
  auto FormatCommand(const test::TestCommand& command) -> std::string;

}

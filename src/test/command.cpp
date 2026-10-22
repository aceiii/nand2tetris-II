#include <format>
#include <utility>
#include <magic_enum/magic_enum.hpp>

#include "command.hpp"
#include "../util/string.hpp"


using namespace test;

auto test::FormatPatternFormat(const test::PatternFormat& format) -> char {
  using namespace test;
  switch (format) {
    case PatternFormat::Binary: return 'B';
    case PatternFormat::Hexadecimal: return 'X';
    case PatternFormat::Decimal: return 'D';
    case PatternFormat::String: return 'S';
    default: std::unreachable();
  }
}

auto test::FormatPattern(const test::Pattern& pattern) -> std::string {
  return std::format("%{}{}.{}.{}", test::FormatPatternFormat(pattern.format), pattern.width, pattern.left, pattern.right);
}

auto test::FormatColumn(const test::Column& column) -> std::string {
  if (!column.pattern.has_value()) {
    return column.name;
  }

  return std::format("{}{}", column.name, test::FormatPattern(column.pattern.value()));
}

auto test::FormatCommand(const test::TestCommand& command) -> std::string {
  const auto name = magic_enum::enum_name(command.type);

  switch (command.type) {
  case test::CommandType::Load:
  case test::CommandType::OutputFile:
  case test::CommandType::CompareTo:
    return std::format("{} {}", name, command.filename);
  case test::CommandType::OutputList:
    return std::format("{} {}", name, util::string::Join(command.columns, " ", test::FormatColumn));
  case test::CommandType::Set:
    return std::format("{} {} {}", name, command.ident, command.value);
  case test::CommandType::Eval:
  case test::CommandType::Output:
  case test::CommandType::Tick:
  case test::CommandType::Tock:
    return std::string(name);
  default: std::unreachable();
  }
}

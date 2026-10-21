#include <charconv>
#include <format>
#include <fstream>
#include <vector>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "test_parser.hpp"
#include "../util/string.hpp"

namespace fs = std::filesystem;

using namespace hdl::test;


namespace hdl::test::internal {
  template <typename T = void>
  using ParseResult = std::expected<T, std::string>;

  struct InnerParser {
    std::string_view buffer;

    int idx = 0;

    auto Current() const -> char {
      return buffer[idx];
    }
    auto Consume() -> void {
      idx++;
    }

    auto Eof() const {
      return idx >= buffer.size();
    }

    auto Peek() const -> char {
      if (idx >= buffer.size()) {
        return 0;
      }
      return buffer[idx + 1];
    }

    auto IsWhitespace(char c) -> bool {
      switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
          return true;
        default: return false;
      }
    }

    auto SkipWhitespace() -> void {
      while (IsWhitespace(Current())) {
        Consume();
      }
    }

    auto SkipIgnorable() -> ParseResult<> {
      SkipWhitespace();
      if (auto res = SkipComments(); !res.has_value()) {
        return std::unexpected{res.error()};
      }
      return {};
    }

    auto SkipComments() -> ParseResult<> {
      while (Current() == '/') {
        Consume();
        if (Current() == '*') {
          do {
            Consume();
          } while (!(Current() == '*' && Peek() == '/'));
          Consume();
          if (auto res = Expect('/'); !res.has_value()) {
            return std::unexpected{res.error()};
          }
        }
        else if (Current() == '/') {
          do {
            Consume();
          } while (Current() != '\0' && Current() != '\n');
        }

        SkipWhitespace();
      }
      return {};
    }

    auto Expect(char expected) -> ParseResult<> {
      auto c = Current();
      if (c != expected) {
        return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting '{}'.", c, idx, expected)};
      }
      Consume();
      return {};
    }

    auto ExpectStr(std::string_view sv) -> ParseResult<> {
      for (auto c : sv) {
        if (auto res = Expect(c); !res.has_value()) {
          return res;
        }
      }
      return {};
    }

    auto Digit() -> ParseResult<int> {
      auto c = Current();
      if (c < '0' || c > '9') {
        return std::unexpected{std::format("Unexpected character '{}', expecting digits 0-9", c)};
      }
      return c - '0';
    }

    auto Whitespace() -> ParseResult<> {
      auto c = Current();
      if (!IsWhitespace(c)) {
        return std::unexpected{std::format("Unexpected character '{}', expecting whitespace", c)};
      }
      return {};
    }

    auto DecimalNumber(bool allow_neg = false) -> ParseResult<int64_t> {
      auto idx_start = idx;
      auto c = Current();
      if (c == '0') {
        if (IsAlphaNumeric(Peek())) {
          return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting non-alphanumeric character.", Peek(), idx + 1)};
        }
        Consume();
        return 0;
      }

      bool negative = false;
      if (allow_neg && c == '-') {
        spdlog::trace("Found '-', expecting negative number at index {}", idx);
        Consume();
        negative = true;
        c = Current();
      }

      if (!isdigit(c)) {
        return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting digits 0-9..", c, idx)};
      }

      int64_t number = c - '0';
      Consume();
      c = Current();
      while (IsAlphaNumeric(c)) {
        if (isdigit(c)) {
          number = (number * 10) + (c - '0');
        } else {
          return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting digits 0-9.", c, idx)};
        }
        Consume();
        c = Current();
      }

      spdlog::trace("Found number {} at index {}", number, idx_start);

      return number;
    }

    auto BinaryNumber() -> ParseResult<int64_t> {
      spdlog::trace("BinaryNumber start at index {}", idx);

      if (auto res = Expect('%'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect('B'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto start_idx = idx;
      std::stringstream ss;
      while (!Eof()) {
        auto c = Current();
        if (c == '0' || c == '1') {
          ss << c;
          Consume();
        } else if (IsTokenChar(c)) {
          return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting '0' or '1'.", c, idx)};
        } else {
          break;
        }
      }

      const auto& str = ss.str();
      spdlog::trace("bin str: {}", str);

      int64_t result;
      auto [pc, ec] = std::from_chars(str.data(), str.data() + str.size(), result, 2);
      if (ec != std::errc()) {
        auto offset = pc - str.data();
        auto bin_idx = start_idx + offset;
        auto c = *pc;
        return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting either '0' or '1'.", c, bin_idx)};
      }

      spdlog::trace("BinaryNumber found: {:b} end at index {}", result, idx);

      return result;
    }

    auto IsAlphaNumeric(char c) -> bool {
      return isalpha(c) || isdigit(c);
    }

    auto IsTokenChar(char c) -> bool {
      return IsAlphaNumeric(c) || c == '-';
    }

    auto IdentChar() -> ParseResult<char> {
      auto c = Current();
      if (!IsAlphaNumeric(c)) {
        return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting identifier character.", c, idx)};
      }
      Consume();
      return c;
    }

    auto TokenChar() -> ParseResult<char> {
      auto idx_start = idx;
      auto c = Current();
      if (!IsTokenChar(c)) {
        return std::unexpected{std::format("Unexpected character '{}' at index {}, epxecting token character.", c, idx)};
      }
      Consume();
      return c;
    }

    auto IdentStart() -> ParseResult<char> {
      auto c = Current();
      if (!isalpha(c)) {
        return std::unexpected{std::format("Unexpected character '{}' at index {}, expecting letter.", c, idx)};
      }
      Consume();
      return c;
    }

    auto Ident() -> ParseResult<std::string> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::stringstream ss;
      auto idx_start = idx;
      auto res = IdentStart();
      if (!res.has_value()) {
        return std::unexpected{res.error()};
      }
      ss << res.value();

      while (IsAlphaNumeric(Current())) {
        res = IdentChar();
        ss << res.value();
      }

      spdlog::trace("Found ident '{}' at index {}", ss.str(), idx_start);
      return ss.str();
    }

    auto Token() -> ParseResult<std::string> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::stringstream ss;
      auto idx_start = idx;
      auto res = IdentStart();

      if (!res.has_value()) {
        return std::unexpected{res.error()};
      }
      ss << res.value();

      while (IsTokenChar(Current())) {
        res = TokenChar();
        ss << res.value();
      }

      spdlog::trace("Found token '{}' at index {}", ss.str(), idx_start);
      return ss.str();
    }

    auto Escaped() -> ParseResult<std::string> {
      if (auto res = Expect('\\'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto c = Current();
      switch (c) {
        case '\'': return "'";
        case '"': return "\"";
        default: return std::unexpected{std::format("Unexpected escape sequence '\\{}' at index {}", c, idx)};
      }
    }

    auto String(char quote) -> ParseResult<std::string> {
      if (auto res = Expect(quote); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::stringstream ss;

      while (!Eof()) {
        auto c = Current();

        spdlog::info("current: {}", c);

        if (c == quote || c == '\n' || c == '\r') {
          break;
        }

        if (c == '\\') {
          auto escaped = Escaped();
          if (!escaped.has_value()) {
            return std::unexpected{escaped.error()};
          }
          ss << escaped.value();
          continue;
        }

        ss << c;
        Consume();
      }

      if (auto res = Expect(quote); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return ss.str();
    }

    auto Path() -> ParseResult<std::string> {
      std::stringstream ss;

      while (!Eof()) {
        auto c = Current();

        if (IsWhitespace(c) || c == ',' || c == ';') {
          break;
        }

        if (c == '/') {
          auto next = Peek();
          if (next == '/' || next == '*') {
            break;
          }
        }

        ss << c;
        Consume();
      }

      return ss.str();
    }

    auto Filename() -> ParseResult<std::string> {
      auto quote_char = Current();
      auto quoted = quote_char == '\'' || quote_char == '"';

      if (quoted) {
        return String(quote_char);
      }

      return Path();
    }

    auto Keyword() -> ParseResult<CommandType> {
      spdlog::trace("Keyword start at index {}.", idx);

      auto token = Token();
      if (!token.has_value()) {
        return std::unexpected{token.error()};
      }

      struct CommandKeyword {
        std::string_view keyword;
        CommandType command_type;
      };

      static constexpr auto keywords = std::to_array<CommandKeyword>({
        { "load", CommandType::Load },
        { "output-file", CommandType::OutputFile },
        { "output-list", CommandType::OutputList },
        { "compare-to", CommandType::CompareTo },
        { "set", CommandType::Set },
        { "eval", CommandType::Eval },
        { "output", CommandType::Output },
        { "tick", CommandType::Tick },
        { "tock", CommandType::Tock },
      });

      auto found = std::find_if(keywords.begin(), keywords.end(), [&](const auto& item) {
        return item.keyword == token;
      });

      if (found == keywords.end()) {
        return std::unexpected{std::format("Unexpected token '{}' at index {}, expected one of: load, output-file, compare-to, output-list, set, eval, output", token.value(), idx)};
      }

      spdlog::trace("Keyword end, found keyword {}.", found->keyword);

      return found->command_type;
    }

    auto Format() -> ParseResult<PatternFormat> {
      auto c = Current();
      PatternFormat format;
      switch (c) {
        case 'B': format = PatternFormat::Binary; break;
        case 'X': format = PatternFormat::Hexadecimal; break;
        case 'D': format = PatternFormat::Decimal; break;
        case 'S': format = PatternFormat::String; break;
        default: return std::unexpected{std::format("Got '{}', expected one of: 'B', 'X', 'D', 'S'", c)};
      }

      spdlog::trace("Found format '{}' at index {}", magic_enum::enum_name(format), idx);

      Consume();
      return format;
    };

    auto Pattern() -> ParseResult<Pattern> {
      spdlog::trace("Pattern start at index {}", idx);

      if (auto res = Expect('%'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto format = Format();
      if (!format.has_value()) {
        return std::unexpected{format.error()};
      }

      auto width = DecimalNumber();
      if (!width.has_value()) {
        return std::unexpected{width.error()};
      }

      if (auto res = Expect('.'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto left = DecimalNumber();
      if (!left.has_value()) {
        return std::unexpected{left.error()};
      }

      if (auto res = Expect('.'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto right = DecimalNumber();
      if (!right.has_value()) {
        return std::unexpected{right.error()};
      }

      spdlog::trace("Pattern end, Found pattern {}{}.{}.{}", magic_enum::enum_name(format.value()), width.value(), left.value(), right.value());

      return ::Pattern{
        .format = std::move(format.value()),
        .width = static_cast<size_t>(width.value()),
        .left = static_cast<size_t>(left.value()),
        .right = static_cast<size_t>(right.value()),
      };
    };

    auto Column() -> ParseResult<Column> {
      SkipWhitespace();

      auto token = Token();
      if (!token.has_value()) {
        return std::unexpected{token.error()};
      }

      if (Current() != '%') {
        return ::Column{
          .name = std::move(token.value()),
          .pattern = std::nullopt,
        };
      }

      auto pattern = Pattern();
      if (!pattern.has_value()) {
        return std::unexpected{pattern.error()};
      }

      return ::Column{
        .name = std::move(token.value()),
        .pattern = std::move(pattern.value()),
      };
    }

    auto Columns() -> ParseResult<std::vector<::Column>> {
      std::vector<::Column> columns;

      while (!Eof()) {
        auto column = Column();
        if (!column.has_value()) {
          return std::unexpected{column.error()};
        }

        columns.push_back(std::move(column.value()));

        auto c = Current();
        if (c == ',' || c == ';') {
          break;
        }
      }

      return columns;
    }

    auto Value() -> ParseResult<int64_t> {
      if (Current() == '%') {
        return BinaryNumber();
      }

      return DecimalNumber(true);
    }

    auto LoadCommand() -> ParseResult<TestCommand> {
      auto filename = Filename();
      if (!filename.has_value()) {
        return std::unexpected{filename.error()};
      }

      return TestCommand{
        .type = CommandType::Load,
        .filename = std::move(filename.value()),
      };
    }

    auto OutputFileCommand() -> ParseResult<TestCommand> {
      auto filename = Filename();
      if (!filename.has_value()) {
        return std::unexpected{filename.error()};
      }

      return TestCommand{
        .type = CommandType::OutputFile,
        .filename = std::move(filename.value()),
      };
    }

    auto OutputListCommand() -> ParseResult<TestCommand> {
      auto columns = Columns();
      if (!columns.has_value()) {
        return std::unexpected{columns.error()};
      }

      return TestCommand{
        .type = CommandType::OutputList,
        .columns = std::move(columns.value()),
      };
    }

    auto CompareToCommand() -> ParseResult<TestCommand> {
      auto filename = Filename();
      if (!filename.has_value()) {
        return std::unexpected{filename.error()};
      }

      return TestCommand{
        .type = CommandType::CompareTo,
        .filename = std::move(filename.value()),
      };
    }

    auto SetCommand() -> ParseResult<TestCommand> {
      auto ident = Ident();
      if (!ident.has_value()) {
        return std::unexpected{ident.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto value = Value();
      if (!value.has_value()) {
        return std::unexpected{value.error()};
      }

      return TestCommand{
        .type = CommandType::Set,
        .ident = std::move(ident.value()),
        .value = std::move(value.value()),
      };
    }

    auto EvalCommand() -> ParseResult<TestCommand> {
      return TestCommand{
        .type = CommandType::Eval,
      };
    }

    auto OutputCommand() -> ParseResult<TestCommand> {
      return TestCommand{
        .type = CommandType::Output,
      };
    }

    auto TickCommand() -> ParseResult<TestCommand> {
      return TestCommand{
        .type = CommandType::Tick,
      };
    }

    auto TockCommand() -> ParseResult<TestCommand> {
      return TestCommand{
        .type = CommandType::Tock,
      };
    }

    auto Command() -> ParseResult<TestCommand> {
      spdlog::trace("Command start at indx {}", idx);

      auto keyword = Keyword();
      if (!keyword.has_value()) {
        return std::unexpected{keyword.error()};
      }

      SkipWhitespace();

      auto result = [&] {
        switch (*keyword) {
        case CommandType::Load: return LoadCommand();
        case CommandType::OutputFile: return OutputFileCommand();
        case CommandType::OutputList: return OutputListCommand();
        case CommandType::CompareTo: return CompareToCommand();
        case CommandType::Set: return SetCommand();
        case CommandType::Eval: return EvalCommand();
        case CommandType::Output: return OutputCommand();
        case CommandType::Tick: return TickCommand();
        case CommandType::Tock: return TockCommand();
        default: std::unreachable();
        }
      }();

      spdlog::trace("Command end.");

      return result;
    }

    auto CommandGroup() -> ParseResult<std::vector<TestCommand>> {
      std::vector<TestCommand> commands;

      spdlog::trace("CommandGroup start at index {}", idx);

      while (!Eof()) {
        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }

        auto command = Command();
        if (!command.has_value()) {
          return std::unexpected{command.error()};
        }

        commands.push_back(std::move(command.value()));

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }

        if (Current() == ';') {
          break;
        }

        if (auto res = Expect(','); !res.has_value()) {
          return std::unexpected{res.error()};
        }
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }


      if (auto res = Expect(';'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      spdlog::trace("CommandGroup end at index={}.", idx);

      return commands;
    }

    auto Commands() -> ParseResult<std::vector<TestCommand>> {
      std::vector<TestCommand> commands;

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      spdlog::trace("Commands start at index {}.", idx);

      while (!Eof()) {
        spdlog::trace("commands next at index {}", idx);

        auto command_group = CommandGroup();
        if (!command_group.has_value()) {
          return std::unexpected{command_group.error()};
        }

        std::move(command_group->begin(), command_group->end(), std::back_inserter(commands));
      }

      spdlog::trace("Commands end.");

      return commands;
    };
  };
}

TestParserResult TestParser::Parse(std::string_view contents) {
  internal::InnerParser parser;
  parser.buffer = contents;

  auto res = parser.Commands();
  if (!res.has_value()) {
    return std::unexpected{res.error()};
  }

  return res.value();
}

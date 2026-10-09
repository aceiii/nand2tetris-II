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
      if (auto res = SkipComments(); !res.has_value()) {
        return std::unexpected{res.error()};
      }
      SkipWhitespace();
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
        return std::unexpected{std::format("Unexpected '{}' at index {}, expecting '{}'.", c, idx, expected)};
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

    auto ExpectOneOf(std::string_view sv) -> ParseResult<char> {
      auto curr = Current();
      auto found = sv.find_first_of(curr);
      if (found == std::string_view::npos) {
        return std::unexpected{std::format("Unexpected one of: {}", sv)};
      }
      return sv.substr(found, 1).at(0);
    }

    auto Digit() -> ParseResult<int> {
      auto c = Current();
      if (c < '0' || c > '9') {
        return std::unexpected{std::format("Unexpected character '{}'", c)};
      }
      return c - '0';
    }

    auto StartDigit() -> ParseResult<int> {
      auto c = Current();
      if (c < '0' || c > '9') {
        return std::unexpected{std::format("Unexpected character '{}'", c)};
      }
      return c - '0';
    }

    auto Whitespace() -> ParseResult<> {
      auto c = Current();
      if (!IsWhitespace(c)) {
        return std::unexpected(std::format("Unexpected character '{}'", c));
      }
      return {};
    }

    auto Number() -> ParseResult<int> {
      auto c = Current();
      if (c == '0') {
        if (IsAlphaNumeric(Peek())) {
          return std::unexpected{std::format("Unexpected '{}' at index {}.", Peek(), idx + 1)};
        }
        Consume();
        return 0;
      }

      if (!isdigit(c)) {
        return std::unexpected{std::format("Unexpected '{}' at index {}.", c, idx)};
      }

      int number = c - '0';
      Consume();
      c = Current();
      while (IsAlphaNumeric(c)) {
        if (isdigit(c)) {
          number = (number * 10) + (c - '0');
        } else {
          return std::unexpected{std::format("Unexpected '{}' at index {}.", c, idx)};
        }
        Consume();
        c = Current();
      }

      return number;
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
        return std::unexpected{std::format("Unexpected '{}' at index {}.", c, idx)};
      }
      Consume();
      return c;
    }

    auto TokenChar() -> ParseResult<char> {
      auto c = Current();
      if (!IsTokenChar(c)) {
        return std::unexpected{std::format("Unexpected '{}' at index {}.", c, idx)};
      }
      Consume();
      return c;
    }

    auto IdentStart() -> ParseResult<char> {
      auto c = Current();
      if (!isalpha(c)) {
        return std::unexpected{std::format("Unexpected '{}' at index {}.", c, idx)};
      }
      Consume();
      return c;
    }

    auto Ident() -> ParseResult<std::string> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::stringstream ss;
      auto res = IdentStart();
      if (!res.has_value()) {
        return std::unexpected{res.error()};
      }
      ss << res.value();

      while (IsAlphaNumeric(Current())) {
        res = IdentChar();
        ss << res.value();
      }
      return ss.str();
    }

    auto Token() -> ParseResult<std::string> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::stringstream ss;
      auto res = IdentStart();
      if (!res.has_value()) {
        return std::unexpected{res.error()};
      }
      ss << res.value();

      while (IsTokenChar(Current())) {
        res = TokenChar();
        ss << res.value();
      }
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
        default: return std::unexpected{std::format("Unexpected escape sequence '\\{}'", c)};
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
      auto token = Token();
      if (!token.has_value()) {
        return std::unexpected(token.error());
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
      });

      auto found = std::find_if(keywords.begin(), keywords.end(), [&](const auto& item) {
        return item.keyword == token;
      });

      if (found == keywords.end()) {
        return std::unexpected{"Expected one of: load, output-file, compare-to, output-list, set, eval, output"};
      }

      return found->command_type;
    }

    auto Pattern() -> ParseResult<std::string> {
      SkipWhitespace();

      std::stringstream ss;

      while (!Eof()) {
        auto c = Current();
        if (IsAlphaNumeric(c) || c == '.' || c == '%') {
          ss << c;
          Consume();
          continue;
        }

        break;
      }

      return ss.str();
    }

    auto Patterns() -> ParseResult<std::vector<std::string>> {
      std::vector<std::string> patterns;

      while (!Eof()) {
        auto pattern = Pattern();
        if (!pattern.has_value()) {
          return std::unexpected{pattern.error()};
        }

        patterns.push_back(std::move(pattern.value()));

        auto c = Current();
        if (c == ',' || c == ';') {
          break;
        }
      }

      return patterns;
    }

    auto Value() -> ParseResult<std::string> {
      std::stringstream ss;

      while (!Eof() && (IsAlphaNumeric(Current()) || Current() == '%')) {
        ss << Current();
        Consume();
      }

      return ss.str();
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
      auto patterns = Patterns();
      if (!patterns.has_value()) {
        return std::unexpected{patterns.error()};
      }

      return TestCommand{
        .type = CommandType::OutputList,
        .patterns = std::move(patterns.value()),
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

    auto Command() -> ParseResult<TestCommand> {
      auto keyword = Keyword();
      if (!keyword.has_value()) {
        return std::unexpected(keyword.error());
      }

      SkipWhitespace();

      switch (*keyword) {
      case CommandType::Load: return LoadCommand();
      case CommandType::OutputFile: return OutputFileCommand();
      case CommandType::OutputList: return OutputListCommand();
      case CommandType::CompareTo: return CompareToCommand();
      case CommandType::Set: return SetCommand();
      case CommandType::Eval: return EvalCommand();
      case CommandType::Output: return OutputCommand();
      default: std::unreachable();
      }
    }

    auto CommandGroup() -> ParseResult<std::vector<TestCommand>> {
      std::vector<TestCommand> commands;

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

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }
      }

      if (auto res = Expect(';'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return commands;
    }

    auto Commands() -> ParseResult<std::vector<TestCommand>> {
      std::vector<TestCommand> commands;

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      while (!Eof()) {
        auto command_group = CommandGroup();
        if (!command_group.has_value()) {
          return std::unexpected{command_group.error()};
        }

        std::move(command_group->begin(), command_group->end(), std::back_inserter(commands));

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }
      }

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

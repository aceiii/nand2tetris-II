#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>
#include "hdl_parser.hpp"

namespace fs = std::filesystem;

namespace hdl::internal {
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

    auto Peek() const -> char {
      if (idx >= buffer.size()) {
        return 0;
      }
      return buffer[idx + 1];
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

    auto ExpectOneOf(std::string_view sv) -> ParseResult<> {
      bool found = false;
      auto curr = Current();
      for (auto c : sv) {
        if (c == curr) {
          found = true;
          break;
        }
      }
      if (!found) {
        return std::unexpected{std::format("Unexpected one of: {}", sv)};
      }
      return {};
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

    auto IdentChar() -> ParseResult<char> {
      auto c = Current();
      if (!IsAlphaNumeric(c)) {
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

    auto Port() -> ParseResult<::hdl::Port> {
      auto ident = Ident();
      if (!ident.has_value()) {
        return std::unexpected{std::format("Expecting identifier, {}", ident.error())};
      }

      if (Current() != '[') {
        return ::hdl::Port{
          .name = ident.value(),
        };
      }

      if (auto res = Expect('['); !res.has_value()) {
        return std::unexpected{std::format("Expecting '[', {}", res.error())};
      }

      auto num = Number();
      if (!num.has_value()) {
        return std::unexpected{num.error()};
      }

      if (auto res = Expect(']'); !res.has_value()) {
        return std::unexpected{std::format("Expecting ']', {}", res.error())};
      }

      return ::hdl::Port{
        .name = ident.value(),
        .width = static_cast<size_t>(num.value()),
      };
    }

    auto Ports() -> ParseResult<std::vector<::hdl::Port>> {
      std::vector<::hdl::Port> ports;
      do {
        auto port = Port();
        if (!port.has_value()) {
          return std::unexpected{port.error()};
        }
        ports.push_back(port.value());

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }

        if (Current() != ',') {
          break;
        }

        Consume();

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }
      } while (true);

      return ports;
    }

    auto Bus() -> ParseResult<::hdl::Bus> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto ident = Ident();
      if ( !ident.has_value()) {
        return std::unexpected{ident.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (Current() != '[') {
        return ::hdl::Bus{
          .name = ident.value(),
          .start = 0,
          .end = 0,
        };
      }

      Consume();

      auto num1 = Number();
      if (!num1.has_value()) {
        return std::unexpected{num1.error()};
      }

      if (Current() == ']') {
        Consume();
        return ::hdl::Bus{
          .name = ident.value(),
          .start = static_cast<size_t>(num1.value()),
          .end = static_cast<size_t>(num1.value() + 1),
        };
      }

      if (auto res = ExpectStr(".."); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto num2 = Number();
      if (!num2.has_value()) {
        return std::unexpected{num2.error()};
      }

      if (auto res = Expect(']'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return ::hdl::Bus{
        .name = ident.value(),
        .start = static_cast<size_t>(num1.value()),
        .end = static_cast<size_t>(num2.value() + 1),
      };
    }

    auto Binding() -> ParseResult<::hdl::PortBinding> {
      auto left = Bus();
      if (!left.has_value()) {
        return std::unexpected{left.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect('='); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto right = Bus();
      if (!right.has_value()) {
        return std::unexpected{right.error()};
      }

      return ::hdl::PortBinding{
        .left = left.value(),
        .right = right.value(),
      };
    }

    auto Bindings() -> ParseResult<std::vector<::hdl::PortBinding>> {
      std::vector<::hdl::PortBinding> port_bindings;
      do {
        auto binding = Binding();
        if (!binding.has_value()) {
          return std::unexpected{binding.error()};
        }
        port_bindings.push_back(binding.value());

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }

        if (Current() != ',') {
          break;
        }

        Consume();

        if (auto res = SkipIgnorable(); !res.has_value()) {
          return std::unexpected{res.error()};
        }
      } while (true);
      return port_bindings;
    }

    auto Part() -> ParseResult<::hdl::Part> {
      auto ident = Ident();
      if (!ident.has_value()) {
        return std::unexpected{ident.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect('('); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto bindings = Bindings();
      if (!bindings.has_value()) {
        return std::unexpected{bindings.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect(')'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return ::hdl::Part{
        .name = ident.value(),
        .bindings = bindings.value(),
      };
    }

    auto Parts() -> ParseResult<std::vector<::hdl::Part>> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = ExpectStr("PARTS"); !res.has_value()) {
        return std::unexpected{std::format("Expecting keyword PARTS, {}", res.error())};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect(':'); !res.has_value()) {
        return std::unexpected(res.error());
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      std::vector<::hdl::Part> parts;
      do {
        auto part = Part();
        if (!part.has_value()) {
          return std::unexpected{part.error()};
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

        parts.push_back(part.value());
      } while (Current() != '}');

      return parts;
    }

    auto PortSection (std::string_view section) -> ParseResult<std::vector<::hdl::Port>> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = ExpectStr(section); !res.has_value()) {
        return std::unexpected{std::format("Expecting keyword {}. {}", section, res.error())};
      }

      auto ports = Ports();
      if (!ports.has_value()) {
        return std::unexpected{ports.error()};
      }

      if (auto res = Expect(';'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return ports.value();
    }

    auto Chip () -> ParseResult<Chip> {
      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = ExpectStr("CHIP"); !res.has_value()) {
        return std::unexpected{std::format("Expecting keyword CHIP, {}", res.error())};
      }

      auto ident = Ident();
      if (!ident.has_value()) {
        return std::unexpected{ident.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect('{'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      auto in_section = PortSection("IN");
      if (!in_section.has_value()) {
        return std::unexpected{in_section.error()};
      }

      auto out_section = PortSection("OUT");
      if (!out_section.has_value()) {
        return std::unexpected{out_section.error()};
      }

      auto parts = Parts();
      if (!parts.has_value()) {
        return std::unexpected{parts.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = Expect('}'); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      if (auto res = SkipIgnorable(); !res.has_value()) {
        return std::unexpected{res.error()};
      }

      return hdl::Chip{
        .name = std::move(ident.value()),
        .in = std::move(in_section.value()),
        .out = std::move(out_section.value()),
        .parts = std::move(parts.value()),
      };
    }
  };
}

std::expected<hdl::Chip, std::string> hdl::Parser::Parse(fs::path path) {
  std::ifstream file(path, std::ios::in);
  if (!file.is_open()) {
    const std::string error_msg = std::format("Failed to open file '{}': {}", path.string(), strerror(errno));
    return std::unexpected{error_msg};
  }

  spdlog::info("Parsing HDL file: {}", path.string());

  auto size = fs::file_size(path);
  std::vector<char> buffer(size);
  file.read(buffer.data(), size);
  file.close();
  spdlog::debug("Read {} bytes", size);

  internal::InnerParser parser;
  parser.buffer = std::string_view{buffer};

  auto res = parser.Chip();
  if (!res.has_value()) {
    return std::unexpected{res.error()};
  }

  return res.value();
}

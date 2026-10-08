#include <filesystem>
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "args.hpp"
#include "hdl/hdl_parser.hpp"
#include "hdl/test_parser.hpp"
#include "util/string.hpp"

namespace fs = std::filesystem;


auto FormatCommand(const hdl::test::TestCommand& command) {
  const auto name = magic_enum::enum_name(command.type);

  switch (command.type) {
  case hdl::test::CommandType::Load:
  case hdl::test::CommandType::OutputFile:
  case hdl::test::CommandType::CompareTo:
    return std::format("{} {}", name, command.filename);
  case hdl::test::CommandType::OutputList:
    return std::format("{} {}", name, util::string::Join(command.patterns, " "));
  case hdl::test::CommandType::Set:
    return std::format("{} {} {}", name, command.ident, command.value);
  case hdl::test::CommandType::Eval:
  case hdl::test::CommandType::Output:
    return std::string(name);
  default: std::unreachable();
  }
}

auto RunTestFile(fs::path path) {
  auto commands = hdl::test::TestParser::Parse(path);
  if (!commands.has_value()) {
    spdlog::error("Parse error: {}", commands.error());
    return 1;
  }

  for (const auto &command : commands.value()) {
    spdlog::info("Command: {}", FormatCommand(command));
  }

  return 1;
}

auto RunHdlFile(fs::path path) {
  auto chip = hdl::Parser::Parse(path);
  if (!chip.has_value()) {
    spdlog::error("Parse error: {}", chip.error());
    return 1;
  }

  spdlog::info("Loaded HDL module: {}", path.string());

  auto port_names = [](const std::vector<hdl::Port>& ports) -> std::string {
    return util::string::Join(ports, ", ", [](const hdl::Port& port) {
      if (port.width > 1) {
        return std::format("{}[{}]", port.name, port.width);
      }
      return port.name;
    });
  };

  auto port_bus = [](const hdl::Bus& bus) -> std::string {
    size_t width = bus.Width();
    if (width == 1) {
      return std::format("{}[{}]", bus.name, bus.start);
    }
    if (width > 1) {
      return std::format("{}[{}..{}]", bus.name, bus.start, bus.end - 1);
    }
    return bus.name;
  };

  auto port_bindings = [&](const std::vector<hdl::PortBinding>& bindings) -> std::string {
    return util::string::Join(bindings, ", ", [&](const hdl::PortBinding& binding) {
      return std::format("{}={}", port_bus(binding.left), port_bus(binding.right));
    });
  };

  spdlog::info("CHIP {} {{", chip->name);
  spdlog::info("  IN {};", port_names(chip->in));
  spdlog::info("  OUT {};", port_names(chip->out));
  spdlog::info("  PARTS:");

  for (const auto& part: chip->parts) {
    spdlog::info("    {}({});", part.name, port_bindings(part.bindings));
  }
  spdlog::info("}");

  return 0;
}

auto RunFile(std::string_view filename) {
  fs::path file_path{filename};
  auto ext = file_path.extension();
  if (ext == ".tst") {
    return RunTestFile(file_path);
  } else if (ext == ".hdl") {
    return RunHdlFile(file_path);
  }

  spdlog::error("The file ({}) is not supported. Only .hdl and .tst files are supported.", filename);
  return -1;
}

auto main(int argc, char *argv[]) -> int {
  auto args = app::GetArgs("nand2tetris", "0.0.1", argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error();
    return 1;
  }

  int res = RunFile(args->filename);

  spdlog::info("Exiting.");
  return res;
}

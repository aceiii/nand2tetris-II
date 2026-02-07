#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "hdl/hdl_parser.h"

static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

auto main(int argc, char *argv[]) -> int {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program("nand2tetris", "0.0.1");

  program.add_argument("filename")
    .help("HDL file to load");

  program.add_argument("--log-level")
      .help("Set the verbosity for logging")
      .default_value(std::string("info"))
      .nargs(1);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  const std::string level = program.get("--log-level");
  if (!set_logging_level(level)) {
    std::cerr << fmt::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}",
                             level)
              << std::endl;
    std::cerr << program;
    return 1;
  }

  const std::string filename = program.get("filename");

  hdl::Parser parser;
  if (auto res = parser.Parse(filename); !res.has_value()) {
    spdlog::error("Parse error: {}", res.error());
    return 1;
  }

  spdlog::info("Loaded HDL module: {}", parser.Name());

  auto port_names = [](const std::vector<std::string>& v) -> std::string {
    std::stringstream ss;
    int idx = 0;
    for (const auto& s: v) {
      if (idx > 0) {
        ss << ", ";
      }
      ss << s;
      idx += 1;
    }
    return ss.str();
  };

  auto port_bindings = [](const std::vector<hdl::PortBinding>& bindings) -> std::string {
    std::stringstream ss;
    int idx = 0;
    for (const auto& binding: bindings) {
      if (idx > 0) {
        ss << ", ";
      }
      ss << binding.input << "=" << binding.output;
      idx += 1;
    }
    return ss.str();
  };

  spdlog::info("| IN: {}", port_names(parser.InPorts()));
  spdlog::info("| OUT: {}", port_names(parser.OutPorts()));
  spdlog::info("| PARTS:");
  for (const auto& part: parser.Parts()) {
    spdlog::info("|   {} ({})", part.name, port_bindings(part.bindings));
  }

  spdlog::info("Exiting.");

  return 0;
}

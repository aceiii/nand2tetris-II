#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "args.hpp"


static bool SetLoggingLevel(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

std::expected<app::Args, std::string> app::GetArgs(const std::string &name, const std::string &version, int argc, char **argv) {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program(name, version);

  program.add_argument("filename")
    .help("HDL file to load");

  program.add_argument("--log-level")
      .help("Set the verbosity for logging")
      .default_value(std::string("info"))
      .nargs(1);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::stringstream ss;
    ss << err.what() << std::endl;
    ss << program;
    return std::unexpected{ss.str()};
  }

  const std::string level = program.get("--log-level");
  if (!SetLoggingLevel(level)) {
    std::stringstream ss;
    ss << fmt::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}",
                             level);
    ss << std::endl;
    ss << program;
    return std::unexpected{ss.str()};
  }

  return Args{
    .filename = program.get("filename"),
    .log_level = level,
  };
}

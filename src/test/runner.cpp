#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "runner.hpp"
#include "../util/file.hpp"

namespace fs = std::filesystem;


test::TestRunner::TestRunner(TestRunnerConfig& config) : config_{config} {
}

test::TestRunner::TestRunner(TestRunnerConfig&& config) : config_{std::move(config)} {
}

int test::TestRunner::Run(const TestCommands& commands) {
  int res = 0;
  for (const auto& command : commands) {
    res = RunCommand(command);
    if (res) {
      break;
    }
  }

  return res;
}

int test::TestRunner::RunCommand(const TestCommand& command) {
  spdlog::info("[TestRunner] Running command: {}", FormatCommand(command));

  switch (command.type) {
  case CommandType::Load: {
    fs::path filepath{command.filename};

    if (filepath.is_relative()) {
      filepath = config_.path / filepath;
    }

    auto contents = util::file::Read(filepath.string());
    if (!contents.has_value()) {
      spdlog::error("File error: {}", contents.error());
      return 1;
    }

    spdlog::info("Parsing hdl file: {}", command.filename);

    auto chip = hdl::Parser::Parse(std::string_view{contents.value()});
    if (!chip.has_value()) {
      spdlog::error("Parse error: {}", chip.error());
      return 1;
    }

    chip_ = std::move(chip.value());

    spdlog::info("Loaded chip: {}", chip_.name);

    break;
  }
  // case CommandType::OutputFile: break;
  // case CommandType::OutputList: break;
  // case CommandType::CompareTo: break;
  // case CommandType::Set: break;
  // case CommandType::Eval: break;
  // case CommandType::Output: break;
  // case CommandType::Tick: break;
  // case CommandType::Tock: break;
  default:
    spdlog::error("Unimplemented command type: {}", magic_enum::enum_name(command.type));
    std::unreachable();
  }

  return 0;
}

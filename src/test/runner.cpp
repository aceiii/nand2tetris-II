#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "runner.hpp"
#include "../util/file.hpp"


test::TestRunner::TestRunner(TestCommands &command_list) : commands_{command_list} {
}

test::TestRunner::TestRunner(TestCommands &&command_list) : commands_{std::move(command_list)} {
}

int test::TestRunner::Run() {
  int result = RunCommands();
  if (result) {
    return result;
  }

  return 0;
}

int test::TestRunner::RunCommands() {
  int res = 0;
  do {
    res = RunNextCommand();
  } while (res == 0 && index_ < commands_.size());
  return res;
}

int test::TestRunner::RunNextCommand() {
  if (index_ >= commands_.size()) {
    spdlog::error("Tests already terminated.");
    return 1;
  }

  const auto& command = commands_.at(index_);
  spdlog::info("[TestRunner] Running command: {}", FormatCommand(command));

  switch (command.type) {
  case CommandType::Load: {
    auto contents = util::file::Read(command.filename);
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

  index_ += 1;

  return 0;
}

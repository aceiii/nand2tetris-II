#include <spdlog/spdlog.h>

#include "runner.hpp"


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
  } while (index_ < commands_.size());
  return res;
}

int test::TestRunner::RunNextCommand() {
  if (index_ >= commands_.size()) {
    spdlog::error("Tests already terminated.");
    return 1;
  }

  const auto& command = commands_.at(index_);
  spdlog::info("[TestRunner] Running command: {}", FormatCommand(command));

  index_ += 1;

  return 0;
}

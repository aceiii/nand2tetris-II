#include "runner.hpp"

test::TestRunner::TestRunner(TestCommands &command_list) : commands_{command_list} {
}

test::TestRunner::TestRunner(TestCommands &&command_list) : commands_{std::move(command_list)} {
}

int test::TestRunner::Run() {
  return 0;
}

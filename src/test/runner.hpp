#pragma once

#include <vector>

#include "command.hpp"
#include "parser.hpp"


namespace test
{

  using TestCommands = std::vector<TestCommand>;

  class TestRunner final {
  public:
    TestRunner() = delete;
    TestRunner(TestCommands& command_list);
    TestRunner(TestCommands&& command_list);

    int Run();

  private:
    int RunCommands();
    int RunNextCommand();

  private:
    TestCommands commands_;
    size_t index_ = 0;
  };

}

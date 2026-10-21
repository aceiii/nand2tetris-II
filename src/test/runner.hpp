#pragma once

#include "test_parser.hpp"

#include <vector>


namespace test
{

  using TestCommands = std::vector<TestCommand>;

  class TestRunner {
  public:
    TestRunner() = delete;
    TestRunner(TestCommands& command_list);
    TestRunner(TestCommands&& command_list);

    int Run();

  private:
    TestCommands commands_;
    size_t index_;
  };

}

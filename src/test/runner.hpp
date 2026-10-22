#pragma once

#include <filesystem>
#include <vector>

#include "command.hpp"
#include "parser.hpp"
#include "../hdl/parser.hpp"

namespace test
{
  using TestCommands = std::vector<TestCommand>;

  struct TestRunnerConfig {
    std::filesystem::path path;
  };

  class TestRunner final {
  public:
    TestRunner() = delete;
    TestRunner(TestRunnerConfig& config);
    TestRunner(TestRunnerConfig&& config);

    int Run(const TestCommands& command_list);
    int RunCommand(const TestCommand& command);

  private:
    TestRunnerConfig config_;
    hdl::Chip chip_;
  };

}

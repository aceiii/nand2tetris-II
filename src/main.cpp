#include <iostream>
#include <spdlog/spdlog.h>

#include "args.h"
#include "hdl/hdl_parser.h"


auto main(int argc, char *argv[]) -> int {
  auto arg_res = app::GetArgs("nand2tetris", "0.0.1", argc, argv);
  if (!arg_res.has_value()) {
    std::cerr << arg_res.error();
  }

  app::Args args = arg_res.value();

  hdl::Parser parser;
  if (auto res = parser.Parse(args.filename); !res.has_value()) {
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

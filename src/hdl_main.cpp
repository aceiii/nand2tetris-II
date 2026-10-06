
#include <iostream>
#include <spdlog/spdlog.h>

#include "args.h"
#include "hdl/hdl_parser.h"
#include "util/string.h"


auto main(int argc, char *argv[]) -> int {
  auto arg_res = app::GetArgs("nand2tetris", "0.0.1", argc, argv);
  if (!arg_res.has_value()) {
    std::cerr << arg_res.error();
    return 1;
  }

  app::Args args = arg_res.value();
  auto res = hdl::Parser::Parse(args.filename);

  if (!res.has_value()) {
    spdlog::error("Parse error: {}", res.error());
    return 1;
  }

  spdlog::info("Loaded HDL module: {}", args.filename);

  auto port_names = [](const std::vector<hdl::Port>& ports) -> std::string {
    return util::StrJoin(ports, ", ", [](const hdl::Port& port) {
      if (port.width > 1) {
        return std::format("{}[{}]", port.name, port.width);
      }
      return port.name;
    });
  };

  auto port_bus = [](const hdl::Bus& bus) -> std::string {
    size_t width = bus.Width();
    if (width == 1) {
      return std::format("{}[{}]", bus.name, bus.start);
    }
    if (width > 1) {
      return std::format("{}[{}..{}]", bus.name, bus.start, bus.end - 1);
    }
    return bus.name;
  };

  auto port_bindings = [&](const std::vector<hdl::PortBinding>& bindings) -> std::string {
    return util::StrJoin(bindings, ", ", [&](const hdl::PortBinding& binding) {
      return std::format("{}={}", port_bus(binding.left), port_bus(binding.right));
    });
  };

  auto chip = std::move(res.value());

  spdlog::info("CHIP {} {{", chip.name);
  spdlog::info("  IN {};", port_names(chip.in));
  spdlog::info("  OUT {};", port_names(chip.out));
  spdlog::info("  PARTS:");
  for (const auto& part: chip.parts) {
    spdlog::info("    {}({});", part.name, port_bindings(part.bindings));
  }
  spdlog::info("}");

  spdlog::info("Exiting.");

  return 0;
}

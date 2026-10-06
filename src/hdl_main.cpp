#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>

#include "args.hpp"
#include "hdl/hdl_parser.hpp"
#include "util/string.hpp"

namespace fs = std::filesystem;


auto RunTestFile(fs::path path) {
  spdlog::error("Not yet implemented.");
  return 1;
}

auto RunHdlFile(fs::path path) {
  auto chip = hdl::Parser::Parse(path);
  if (!chip.has_value()) {
    spdlog::error("Parse error: {}", chip.error());
    return 1;
  }

  spdlog::info("Loaded HDL module: {}", path.string());

  auto port_names = [](const std::vector<hdl::Port>& ports) -> std::string {
    return util::string::Join(ports, ", ", [](const hdl::Port& port) {
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
    return util::string::Join(bindings, ", ", [&](const hdl::PortBinding& binding) {
      return std::format("{}={}", port_bus(binding.left), port_bus(binding.right));
    });
  };

  spdlog::info("CHIP {} {{", chip->name);
  spdlog::info("  IN {};", port_names(chip->in));
  spdlog::info("  OUT {};", port_names(chip->out));
  spdlog::info("  PARTS:");
  for (const auto& part: chip->parts) {
    spdlog::info("    {}({});", part.name, port_bindings(part.bindings));
  }
  spdlog::info("}");

  return 0;
}

auto RunFile(std::string_view filename) {
  fs::path file_path{filename};
  auto ext = file_path.extension();
  if (ext == ".tst") {
    return RunTestFile(file_path);
  } else if (ext == ".hdl") {
    return RunHdlFile(file_path);
  }

  spdlog::error("The file ({}) is not supported. Only .hdl and .tst files are supported.", filename);
  return -1;
}

auto main(int argc, char *argv[]) -> int {
  auto args = app::GetArgs("nand2tetris", "0.0.1", argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error();
    return 1;
  }

  int res = RunFile(args->filename);

  spdlog::info("Exiting.");
  return res;
}

#include <expected>
#include <string_view>
#include <spdlog/spdlog.h>

#include "file.hpp"

using namespace util::file;


ReadFileResult util::file::Read(fs::path path) {
  std::ifstream file(path, std::ios::in);
  if (!file.is_open()) {
    const std::string error_msg = std::format("Failed to open file '{}': {}", path.string(), strerror(errno));
    return std::unexpected{error_msg};
  }

  spdlog::info("Reading file: {}", path.string());

  auto size = fs::file_size(path);
  std::vector<char> buffer(size);
  file.read(buffer.data(), size);
  file.close();
  spdlog::debug("Read {} bytes", size);

  return buffer;
}

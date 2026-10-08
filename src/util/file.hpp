#pragma once

#include <expected>
#include <filesystem>
#include <vector>


namespace util::file {
  namespace fs = std::filesystem;

  using Buffer = std::vector<char>;
  using ReadFileResult = std::expected<Buffer, std::string>;

  ReadFileResult Read(fs::path path);

}
